/**
 * Copyright 2013-2022 iSignal Research Labs Pvt Ltd.
 *
 * This file is part of isrRAN.
 *
 * isrRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * isrRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "isrran/phy/ue/ue_sync_nbiot.h"
#include "isrran/phy/io/filesource.h"
#include "isrran/phy/utils/debug.h"
#include "isrran/phy/utils/vector.h"
#include <assert.h>
#include <isrran/isrran.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define REPEAT_FROM_FILE 0
#define TIME_ALIGN_FROM_FILE 1
#define MAX_TIME_OFFSET 128

#define TRACK_MAX_LOST 4
#define TRACK_FRAME_SIZE 32
#define FIND_NOF_AVG_FRAMES 4
#define DEFAULT_SFO_EMA_COEFF 0.1

static cf_t  dummy_buffer_nbiot0[15 * 2048 / 2];
static cf_t  dummy_buffer_nbiot1[15 * 2048 / 2];
static cf_t* dummy_offset_buffer_nbiot[ISRRAN_MAX_PORTS] = {dummy_buffer_nbiot0, dummy_buffer_nbiot1, NULL, NULL};

///< This is a list of CFO candidates that the sync object uses to pre-compensate the received signal
static const float cfo_cands[] =
    {0.0, 1000.0, -1000.0, 2000.0, -2000.0, 3000.0, -3000.0, 4000.0, -4000.0, 5000.0, -5000.0};

int isrran_ue_sync_nbiot_init_file(isrran_nbiot_ue_sync_t* q,
                                   isrran_nbiot_cell_t     cell,
                                   char*                   file_name,
                                   int                     offset_time,
                                   float                   offset_freq)
{
  return isrran_ue_sync_nbiot_init_file_multi(q, cell, file_name, offset_time, offset_freq, 1);
}

int isrran_ue_sync_nbiot_init_file_multi(isrran_nbiot_ue_sync_t* q,
                                         isrran_nbiot_cell_t     cell,
                                         char*                   file_name,
                                         int                     offset_time,
                                         float                   offset_freq,
                                         uint32_t                nof_rx_ant)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL && file_name != NULL && isrran_nofprb_isvalid(cell.base.nof_prb)) {
    ret = ISRRAN_ERROR;
    bzero(q, sizeof(isrran_nbiot_ue_sync_t));
    q->file_mode   = true;
    q->sf_len      = ISRRAN_SF_LEN(isrran_symbol_sz(cell.base.nof_prb));
    q->file_cfo    = -offset_freq;
    q->correct_cfo = true;
    q->fft_size    = isrran_symbol_sz(cell.base.nof_prb);

    if (nof_rx_ant != 1) {
      fprintf(stderr, "With file input, only single Rx antenna is supported.\n");
      goto clean_exit;
    }
    q->nof_rx_antennas = nof_rx_ant;

    if (isrran_cfo_init(&q->file_cfo_correct, 2 * q->sf_len)) {
      fprintf(stderr, "Error initiating CFO\n");
      goto clean_exit;
    }

    if (isrran_filesource_init(&q->file_source, file_name, ISRRAN_COMPLEX_FLOAT_BIN)) {
      fprintf(stderr, "Error opening file %s\n", file_name);
      goto clean_exit;
    }

#if TIME_ALIGN_FROM_FILE
    q->frame_len = ISRRAN_NOF_SF_X_FRAME * q->sf_len;
    if (isrran_sync_nbiot_init(&q->sfind, q->frame_len, q->frame_len, q->fft_size)) {
      fprintf(stderr, "Error initiating sync find\n");
      goto clean_exit;
    }
    int n = isrran_filesource_read(&q->file_source, dummy_buffer_nbiot0, q->frame_len);
    if (n != q->frame_len) {
      fprintf(stderr, "Error reading frame from file.\n");
      exit(-1);
    }

    // find NPSS and set offset time parameter to beginning of next frame
    uint32_t peak_idx;
    if (isrran_sync_nbiot_find(&q->sfind, dummy_buffer_nbiot0, 0, &peak_idx) == ISRRAN_SYNC_ERROR) {
      fprintf(stderr, "Error finding NPSS peak\n");
      exit(-1);
    }
    offset_time = (peak_idx - ISRRAN_NPSS_CORR_OFFSET + q->frame_len / 2) % q->frame_len;
    isrran_sync_nbiot_free(&q->sfind);
#endif

    INFO("Offseting input file by %d samples and %.1f kHz", offset_time, offset_freq / 1000);

    isrran_filesource_read(&q->file_source, dummy_buffer_nbiot0, offset_time);
    isrran_ue_sync_nbiot_reset(q);

    ret = ISRRAN_SUCCESS;
  }
clean_exit:
  if (ret == ISRRAN_ERROR) {
    isrran_ue_sync_nbiot_free(q);
  }
  return ret;
}

int isrran_ue_sync_nbiot_start_agc(isrran_nbiot_ue_sync_t* q,
                                   ISRRAN_AGC_CALLBACK(set_gain_callback),
                                   float init_gain_value)
{
  uint32_t nframes = 0;
  if (q->nof_recv_sf == 1) {
    nframes = 10;
  }
  int n     = isrran_agc_init_uhd(&q->agc, ISRRAN_AGC_MODE_PEAK_AMPLITUDE, nframes, set_gain_callback, q->stream);
  q->do_agc = n == 0 ? true : false;
  if (q->do_agc) {
    isrran_agc_set_gain(&q->agc, init_gain_value);
  }
  return n;
}

int recv_callback_nbiot_multi_to_single(void* h, cf_t* x[ISRRAN_MAX_PORTS], uint32_t nsamples, isrran_timestamp_t* t)
{
  isrran_nbiot_ue_sync_t* q = (isrran_nbiot_ue_sync_t*)h;
  return q->recv_callback_single(q->stream_single, (void*)x[0], nsamples, t);
}

int isrran_ue_sync_nbiot_init(isrran_nbiot_ue_sync_t* q,
                              isrran_nbiot_cell_t     cell,
                              int(recv_callback)(void*, void*, uint32_t, isrran_timestamp_t*),
                              void* stream_handler)
{
  int ret = isrran_ue_sync_nbiot_init_multi(
      q, ISRRAN_NBIOT_MAX_PRB, recv_callback_nbiot_multi_to_single, ISRRAN_NBIOT_NUM_RX_ANTENNAS, (void*)q);
  q->recv_callback_single = recv_callback;
  q->stream_single        = stream_handler;
  return ret;
}

int isrran_ue_sync_nbiot_init_multi(isrran_nbiot_ue_sync_t* q,
                                    uint32_t                max_prb,
                                    int(recv_callback)(void*, cf_t* [ISRRAN_MAX_PORTS], uint32_t, isrran_timestamp_t*),
                                    uint32_t nof_rx_antennas,
                                    void*    stream_handler)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL && stream_handler != NULL && isrran_nofprb_isvalid(max_prb) && recv_callback != NULL) {
    ret = ISRRAN_ERROR;

    bzero(q, sizeof(isrran_nbiot_ue_sync_t));

    q->stream                       = stream_handler;
    q->recv_callback                = recv_callback;
    q->fft_size                     = isrran_symbol_sz(max_prb);
    q->sf_len                       = ISRRAN_SF_LEN(q->fft_size);
    q->file_mode                    = false;
    q->correct_cfo                  = true;
    q->nof_rx_antennas              = nof_rx_antennas;
    q->agc_period                   = 0;
    q->sample_offset_correct_period = DEFAULT_SAMPLE_OFFSET_CORRECT_PERIOD;
    q->sfo_ema                      = DEFAULT_SFO_EMA_COEFF;
    q->max_prb                      = max_prb;

    // we search for NPSS/NSSS in a full frame
    q->nof_recv_sf = 10;
    q->frame_len   = q->nof_recv_sf * q->sf_len;

    if (isrran_sync_nbiot_init(&q->sfind, q->frame_len, q->frame_len, q->fft_size)) {
      fprintf(stderr, "Error initiating sync find\n");
      goto clean_exit;
    }

    // in tracking phase we only sample for one subframe but still use the entire
    // subframe to run the correlation (TODO: use only one symbol?)
    if (isrran_sync_nbiot_init(&q->strack, q->sf_len, q->sf_len, q->fft_size)) {
      fprintf(stderr, "Error initiating sync track\n");
      goto clean_exit;
    }

    isrran_ue_sync_nbiot_reset(q);

    ret = ISRRAN_SUCCESS;
  }

clean_exit:
  if (ret == ISRRAN_ERROR) {
    isrran_ue_sync_nbiot_free(q);
  }
  return ret;
}

uint32_t isrran_ue_sync_nbiot_sf_len(isrran_nbiot_ue_sync_t* q)
{
  return q->frame_len;
}

void isrran_ue_sync_nbiot_free(isrran_nbiot_ue_sync_t* q)
{
  if (q->do_agc) {
    isrran_agc_free(&q->agc);
  }
  if (!q->file_mode) {
    isrran_sync_nbiot_free(&q->sfind);
    isrran_sync_nbiot_free(&q->strack);
  } else {
    isrran_filesource_free(&q->file_source);
  }
  bzero(q, sizeof(isrran_nbiot_ue_sync_t));
}

int isrran_ue_sync_nbiot_set_cell(isrran_nbiot_ue_sync_t* q, isrran_nbiot_cell_t cell)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL && isrran_nofprb_isvalid(cell.base.nof_prb)) {
    ret = ISRRAN_ERROR;

    if (cell.base.nof_prb > q->max_prb) {
      fprintf(stderr, "Error in nbiot_ue_sync_set_cell(): cell.base.nof_prb must be lower than initialized\n");
      return ISRRAN_ERROR;
    }

    q->cell       = cell;
    q->fft_size   = isrran_symbol_sz(q->cell.base.nof_prb);
    q->sf_len     = ISRRAN_SF_LEN(q->fft_size);
    q->agc_period = 0;

    // we search for NPSS/NSSS in a full frame
    q->nof_recv_sf = ISRRAN_NOF_SF_X_FRAME;
    q->frame_len   = q->nof_recv_sf * q->sf_len;

    if (isrran_sync_nbiot_resize(&q->sfind, q->frame_len, q->frame_len, q->fft_size)) {
      fprintf(stderr, "Error resizing sync find\n");
      return ISRRAN_ERROR;
    }

    if (isrran_sync_nbiot_resize(&q->strack, q->sf_len, q->sf_len, q->fft_size)) {
      fprintf(stderr, "Error resizing sync find\n");
      return ISRRAN_ERROR;
    }

    isrran_ue_sync_nbiot_reset(q);

    ret = ISRRAN_SUCCESS;
  }

  return ret;
}

void isrran_ue_sync_nbiot_get_last_timestamp(isrran_nbiot_ue_sync_t* q, isrran_timestamp_t* timestamp)
{
  memcpy(timestamp, &q->last_timestamp, sizeof(isrran_timestamp_t));
}

uint32_t isrran_ue_sync_nbiot_peak_idx(isrran_nbiot_ue_sync_t* q)
{
  return q->peak_idx;
}

isrran_nbiot_ue_sync_state_t isrran_ue_sync_nbiot_get_state(isrran_nbiot_ue_sync_t* q)
{
  return q->state;
}
uint32_t isrran_ue_sync_nbiot_get_sfidx(isrran_nbiot_ue_sync_t* q)
{
  return q->sf_idx;
}

void isrran_ue_sync_nbiot_set_cfo_enable(isrran_nbiot_ue_sync_t* q, bool enable)
{
  isrran_sync_nbiot_set_cfo_enable(&q->sfind, enable);
  isrran_sync_nbiot_set_cfo_enable(&q->strack, enable);
}

float isrran_ue_sync_nbiot_get_cfo(isrran_nbiot_ue_sync_t* q)
{
  return 15000 * (q->state == SF_TRACK ? isrran_sync_nbiot_get_cfo(&q->strack) : isrran_sync_nbiot_get_cfo(&q->sfind));
}

void isrran_ue_sync_nbiot_set_cfo(isrran_nbiot_ue_sync_t* q, float cfo)
{
  isrran_sync_nbiot_set_cfo(&q->sfind, cfo / 15000);
  isrran_sync_nbiot_set_cfo(&q->strack, cfo / 15000);
}

float isrran_ue_sync_nbiot_get_sfo(isrran_nbiot_ue_sync_t* q)
{
  return q->mean_sfo / 5e-3;
}

int isrran_ue_sync_nbiot_get_last_sample_offset(isrran_nbiot_ue_sync_t* q)
{
  return q->last_sample_offset;
}

void isrran_ue_sync_nbiot_set_sample_offset_correct_period(isrran_nbiot_ue_sync_t* q, uint32_t nof_subframes)
{
  q->sample_offset_correct_period = nof_subframes;
}

void isrran_ue_sync_nbiot_set_cfo_ema(isrran_nbiot_ue_sync_t* q, float ema)
{
  isrran_sync_nbiot_set_cfo_ema_alpha(&q->sfind, ema);
  isrran_sync_nbiot_set_cfo_ema_alpha(&q->strack, ema);
}

void isrran_ue_sync_nbiot_set_cfo_tol(isrran_nbiot_ue_sync_t* q, float cfo_tol)
{
  isrran_sync_nbiot_set_cfo_tol(&q->sfind, cfo_tol);
  isrran_sync_nbiot_set_cfo_tol(&q->strack, cfo_tol);
}

void isrran_ue_sync_nbiot_set_sfo_ema(isrran_nbiot_ue_sync_t* q, float ema_coefficient)
{
  q->sfo_ema = ema_coefficient;
}

void isrran_ue_sync_nbiot_set_agc_period(isrran_nbiot_ue_sync_t* q, uint32_t period)
{
  q->agc_period = period;
}

static int find_peak_ok(isrran_nbiot_ue_sync_t* q, cf_t* input_buffer[ISRRAN_MAX_PORTS])
{
  // set subframe idx to NPSS position
  q->sf_idx = 5;
  q->frame_find_cnt++;
  DEBUG("Found peak %d at %d, value %.3f, n_id_ncell: %d",
        q->frame_find_cnt,
        q->peak_idx,
        isrran_sync_nbiot_get_peak_value(&q->sfind),
        q->cell.n_id_ncell);

  if (q->frame_find_cnt >= q->nof_avg_find_frames || q->peak_idx < 2 * q->fft_size) {
    int num_drop = (q->peak_idx - ISRRAN_NPSS_CORR_OFFSET + q->frame_len / 2) % q->frame_len;
    INFO("Realigning frame, reading %d samples", num_drop);
    /* Receive the rest of the subframe so that we are subframe aligned */
    if (q->recv_callback(q->stream, input_buffer, num_drop, &q->last_timestamp) < 0) {
      return ISRRAN_ERROR;
    }

    ///< reset variables
    q->frame_ok_cnt       = 0;
    q->frame_no_cnt       = 0;
    q->frame_total_cnt    = 0;
    q->frame_find_cnt     = 0;
    q->mean_sample_offset = 0;
    q->sf_idx             = 9;

    ///< adjust sampling parameters
    q->nof_recv_sf = 1;
    q->frame_len   = q->nof_recv_sf * q->sf_len;

    ///< go to tracking state
    q->state = SF_TRACK;

    ///< Initialize track state CFO
    q->strack.mean_cfo = q->sfind.mean_cfo;
    q->strack.cfo_i    = q->sfind.cfo_i;
  }

  return 0;
}

static int track_peak_ok(isrran_nbiot_ue_sync_t* q, uint32_t track_idx)
{
  // Get sampling time offset
  q->last_sample_offset = ((int)track_idx - ISRRAN_NPSS_CORR_OFFSET);

  // Adjust sampling time every q->sample_offset_correct_period subframes
  uint32_t frame_idx = 0;
  if (q->sample_offset_correct_period) {
    frame_idx = q->frame_ok_cnt % q->sample_offset_correct_period;
    q->mean_sample_offset += (float)q->last_sample_offset / q->sample_offset_correct_period;
  } else {
    q->mean_sample_offset = q->last_sample_offset;
  }

  // Compute cumulative moving average time offset
  if (!frame_idx) {
    // Adjust RF sampling time based on the mean sampling offset
    q->next_rf_sample_offset = (int)round(q->mean_sample_offset);

    // Reset PSS averaging if correcting every a period longer than 1
    if (q->sample_offset_correct_period > 1) {
      isrran_sync_nbiot_reset(&q->strack);
    }

    // Compute SFO based on mean sample offset
    if (q->sample_offset_correct_period) {
      q->mean_sample_offset /= q->sample_offset_correct_period;
    }
    q->mean_sfo = ISRRAN_VEC_EMA(q->mean_sample_offset, q->mean_sfo, q->sfo_ema);

    if (q->next_rf_sample_offset) {
      INFO("Time offset adjustment: %d samples (%.2f), mean SFO: %.2f Hz, %.5f samples/10-sf, ema=%f, length=%d",
           q->next_rf_sample_offset,
           q->mean_sample_offset,
           isrran_ue_sync_nbiot_get_sfo(q),
           q->mean_sfo,
           q->sfo_ema,
           q->sample_offset_correct_period);
    }
    q->mean_sample_offset = 0;
  }

  ///< If the NPSS peak is beyond the frame we sample too slow, discard the offseted samples to align next frame
  if (q->next_rf_sample_offset > 0 && q->next_rf_sample_offset < MAX_TIME_OFFSET) {
    DEBUG("Positive time offset %d samples.", q->next_rf_sample_offset);
    if (q->recv_callback(
            q->stream, &dummy_offset_buffer_nbiot[0], (uint32_t)q->next_rf_sample_offset, &q->last_timestamp) < 0) {
      fprintf(stderr, "Error receiving from USRP\n");
      return ISRRAN_ERROR;
    }
    q->next_rf_sample_offset = 0;
  }

  q->peak_idx = q->sf_len / 2 + q->last_sample_offset;
  q->frame_ok_cnt++;

  return 1;
}

static int track_peak_no(isrran_nbiot_ue_sync_t* q)
{
  ///< if we missed too many NPSS, we go back to FIND and consider this frame unsynchronized
  q->frame_no_cnt++;
  if (q->frame_no_cnt >= TRACK_MAX_LOST) {
    INFO("%d frames lost. Going back to FIND", (int)q->frame_no_cnt);
    q->nof_recv_sf = 10;
    q->frame_len   = q->nof_recv_sf * q->sf_len;
    q->state       = SF_FIND;
    return 0;
  } else {
    INFO("Tracking peak not found. Peak %.3f, %d lost",
         isrran_sync_nbiot_get_peak_value(&q->strack),
         (int)q->frame_no_cnt);
    /*
    printf("Saving files: pss_corr (%d), input (%d)\n", q->strack.pss.frame_size, ISRRAN_SF_LEN_PRB(q->cell.nof_prb));
    isrran_vec_save_file("pss_corr", q->strack.pss.conv_output_avg, q->strack.pss.frame_size*sizeof(float));
    isrran_vec_save_file("input", q->input_buffer, ISRRAN_SF_LEN_PRB(q->cell.nof_prb)*sizeof(cf_t));
    exit(-1);
    */
    return 1;
  }
}

static int receive_samples(isrran_nbiot_ue_sync_t* q, cf_t* input_buffer[ISRRAN_MAX_PORTS])
{
  ///< A negative time offset means there are samples in our buffer for the next subframe, because we are sampling too
  ///< fast
  if (q->next_rf_sample_offset < 0) {
    q->next_rf_sample_offset = -q->next_rf_sample_offset;
  }

  ///< Get N subframes from the USRP getting more samples and keeping the previous samples, if any
  cf_t* ptr[ISRRAN_MAX_PORTS] = {NULL, NULL, NULL, NULL};
  for (int i = 0; i < q->nof_rx_antennas; i++) {
    ptr[i] = &input_buffer[i][q->next_rf_sample_offset];
  }

  // assure  next_rf_sample_offset isn't larger than frame_len
  q->next_rf_sample_offset = ISRRAN_MIN(q->next_rf_sample_offset, q->frame_len);

  if (q->recv_callback(q->stream, ptr, q->frame_len - q->next_rf_sample_offset, &q->last_timestamp) < 0) {
    return ISRRAN_ERROR;
  }
  ///< reset time offset
  q->next_rf_sample_offset = 0;

  return ISRRAN_SUCCESS;
}

int isrran_ue_sync_nbiot_zerocopy(isrran_nbiot_ue_sync_t* q, cf_t* input_buffer)
{
  cf_t* _input_buffer[ISRRAN_MAX_PORTS] = {NULL};
  _input_buffer[0]                      = input_buffer;
  return isrran_ue_sync_nbiot_zerocopy_multi(q, _input_buffer);
}

/* Returns 1 if the subframe is synchronized in time, 0 otherwise */
int isrran_ue_sync_nbiot_zerocopy_multi(isrran_nbiot_ue_sync_t* q, cf_t** input_buffer)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL) {
    if (q->file_mode) {
      int n = isrran_filesource_read(&q->file_source, input_buffer[0], q->sf_len);
      if (n < q->sf_len) {
        fprintf(stderr, "Error reading input file, read %d bytes\n", n);
#if REPEAT_FROM_FILE
        isrran_filesource_seek(&q->file_source, 0);
#else
        return ISRRAN_ERROR;
#endif
      }
      if (q->correct_cfo) {
        for (int i = 0; i < q->nof_rx_antennas; i++) {
          isrran_cfo_correct(&q->file_cfo_correct, input_buffer[i], input_buffer[i], q->file_cfo / 15000 / q->fft_size);
        }
      }
      q->sf_idx++;
      if (q->sf_idx == 10) {
        q->sf_idx = 0;
      }
      DEBUG("Reading %d samples. sf_idx = %d", q->sf_len, q->sf_idx);
      ret = 1;
    } else {
      if (receive_samples(q, input_buffer)) {
        fprintf(stderr, "Error receiving samples\n");
        return ISRRAN_ERROR;
      }

      switch (q->state) {
        case SF_FIND:
          switch (isrran_sync_nbiot_find(&q->sfind, input_buffer[0], 0, &q->peak_idx)) {
            case ISRRAN_SYNC_ERROR:
              ret = ISRRAN_ERROR;
              fprintf(stderr, "Error finding correlation peak (%d)\n", ret);
              return ISRRAN_ERROR;
            case ISRRAN_SYNC_FOUND:
              ret = find_peak_ok(q, input_buffer);
              break;
            case ISRRAN_SYNC_FOUND_NOSPACE:
              /* If a peak was found but there is not enough space for SSS/CP detection, discard a few samples */
              printf("No space for SSS/CP detection. Realigning frame...\n");
              q->recv_callback(q->stream, dummy_offset_buffer_nbiot, q->frame_len / 2, NULL);
              isrran_sync_nbiot_reset(&q->sfind);
              ret = ISRRAN_SUCCESS;
              break;
            default:
              ret = ISRRAN_SUCCESS;
              break;
          }
          if (q->do_agc) {
            isrran_agc_process(&q->agc, input_buffer[0], q->sf_len);
          }
          break;
        case SF_TRACK:
          ret       = 1;
          q->sf_idx = (q->sf_idx + q->nof_recv_sf) % ISRRAN_NOF_SF_X_FRAME;

          ///< Every SF idx 5, find peak around known position q->peak_idx
          if (q->sf_idx == 5) {
            if (q->do_agc && (q->agc_period == 0 || (q->agc_period && (q->frame_total_cnt % q->agc_period) == 0))) {
              isrran_agc_process(&q->agc, input_buffer[0], q->sf_len);
            }

#ifdef MEASURE_EXEC_TIME
            struct timeval t[3];
            gettimeofday(&t[1], NULL);
#endif
            uint32_t track_idx = 0;

            // Track NPSS around the expected position
            uint32_t find_offset = q->frame_len / 2 - q->strack.max_offset / 2;
            switch (isrran_sync_nbiot_find(&q->strack, input_buffer[0], find_offset, &track_idx)) {
              case ISRRAN_SYNC_ERROR:
                ret = ISRRAN_ERROR;
                fprintf(stderr, "Error tracking correlation peak\n");
                return ISRRAN_ERROR;
              case ISRRAN_SYNC_FOUND:
                ret = track_peak_ok(q, track_idx);
                break;
              case ISRRAN_SYNC_FOUND_NOSPACE:
                // It's very very unlikely that we fall here because this event should happen at FIND phase only
                ret      = 0;
                q->state = SF_FIND;
                printf("Warning: No space for SSS/CP while in tracking phase\n");
                break;
              case ISRRAN_SYNC_NOFOUND:
                ret = track_peak_no(q);
                break;
            }

#ifdef MEASURE_EXEC_TIME
            gettimeofday(&t[2], NULL);
            get_time_interval(t);
            q->mean_exec_time = (float)ISRRAN_VEC_CMA((float)t[0].tv_usec, q->mean_exec_time, q->frame_total_cnt);
#endif

            if (ret == ISRRAN_ERROR) {
              fprintf(stderr, "Error processing tracking peak\n");
              q->state = SF_FIND;
              return ISRRAN_SUCCESS;
            }

            q->frame_total_cnt++;
          } else {
            if (q->correct_cfo) {
              for (int i = 0; i < q->nof_rx_antennas; i++) {
                isrran_cfo_correct(&q->strack.cfocorr,
                                   input_buffer[i],
                                   input_buffer[i],
                                   -isrran_sync_nbiot_get_cfo(&q->strack) / q->fft_size);
              }
            }
          }
          break;
      }
    }
  }
  return ret;
}

void isrran_ue_sync_nbiot_reset(isrran_nbiot_ue_sync_t* q)
{
  ///< Set default params
  isrran_sync_nbiot_set_cfo_enable(&q->sfind, true);
  isrran_sync_nbiot_set_cfo_enable(&q->strack, true);

  isrran_sync_nbiot_set_cfo_ema_alpha(&q->sfind, 0.15);
  isrran_sync_nbiot_set_cfo_ema_alpha(&q->strack, 0.01);

  ///< In find phase and if the cell is known, do not average NPSS correlation because we only capture 1 subframe and
  ///< do not know where the peak is.
  q->nof_avg_find_frames = 1;
  isrran_sync_nbiot_set_npss_ema_alpha(&q->sfind, 1.0);
  isrran_sync_nbiot_set_threshold(&q->sfind, 2.5);

  isrran_sync_nbiot_set_cfo_cand(&q->sfind, cfo_cands, sizeof(cfo_cands) / sizeof(float));
  isrran_sync_nbiot_set_cfo_cand_test_enable(&q->sfind, true);

  isrran_sync_nbiot_set_npss_ema_alpha(&q->strack, 0.1);
  isrran_sync_nbiot_set_threshold(&q->strack, 1.2);

  if (!q->file_mode) {
    isrran_sync_nbiot_reset(&q->sfind);
    isrran_sync_nbiot_reset(&q->strack);
  } else {
    q->sf_idx = 9;
  }
  q->state                 = SF_FIND;
  q->frame_ok_cnt          = 0;
  q->frame_no_cnt          = 0;
  q->frame_total_cnt       = 0;
  q->mean_sample_offset    = 0.0;
  q->next_rf_sample_offset = 0;
  q->frame_find_cnt        = 0;
#ifdef MEASURE_EXEC_TIME
  q->mean_exec_time = 0;
#endif
}
