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

#include "isrran/phy/ue/ue_dl_nbiot.h"

#include <assert.h>
#include <complex.h>
#include <math.h>
#include <string.h>

#define DUMP_SIGNALS 0

static isrran_dci_format_t nb_ue_formats[]   = {ISRRAN_DCI_FORMATN0, ISRRAN_DCI_FORMATN1};
const uint32_t             nof_nb_ue_formats = 2;

static isrran_dci_format_t nb_common_formats[]   = {ISRRAN_DCI_FORMATN1, ISRRAN_DCI_FORMATN2};
const uint32_t             nb_nof_common_formats = 2;

int isrran_nbiot_ue_dl_init(isrran_nbiot_ue_dl_t* q,
                            cf_t*                 in_buffer[ISRRAN_MAX_PORTS],
                            uint32_t              max_prb,
                            uint32_t              nof_rx_antennas)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL) {
    ret = ISRRAN_ERROR;

    bzero(q, sizeof(isrran_nbiot_ue_dl_t));

    q->pkt_errors    = 0;
    q->pkts_total    = 0;
    q->pkts_ok       = 0;
    q->bits_total    = 0;
    q->sample_offset = 0;
    q->mib_set       = false;
    q->nof_re        = ISRRAN_SF_LEN_RE(ISRRAN_NBIOT_MAX_PRB, ISRRAN_CP_NORM);

    // for transmissions using only single subframe
    q->sf_symbols = isrran_vec_cf_malloc(q->nof_re);
    if (!q->sf_symbols) {
      perror("malloc");
      goto clean_exit;
    }
    isrran_vec_cf_zero(q->sf_symbols, q->nof_re);

    for (uint32_t i = 0; i < ISRRAN_MAX_PORTS; i++) {
      q->ce[i] = isrran_vec_cf_malloc(q->nof_re);
      if (!q->ce[i]) {
        perror("malloc");
        goto clean_exit;
      }
      for (int k = 0; k < q->nof_re; k++) {
        q->ce[i][k] = 1;
      }
    }

    // allocate memory for symbols and estimates for tx spanning multiple subframes
    // TODO: only buffer softbits rather than raw samples
    q->sf_buffer = isrran_vec_cf_malloc(q->nof_re * ISRRAN_NPDSCH_MAX_NOF_SF);
    if (!q->sf_buffer) {
      perror("malloc");
      goto clean_exit;
    }
    isrran_vec_cf_zero(q->sf_buffer, q->nof_re * ISRRAN_NPDSCH_MAX_NOF_SF);

    for (uint32_t i = 0; i < ISRRAN_MAX_PORTS; i++) {
      q->ce_buffer[i] = isrran_vec_cf_malloc(q->nof_re * ISRRAN_NPDSCH_MAX_NOF_SF);
      if (!q->ce_buffer[i]) {
        perror("malloc");
        goto clean_exit;
      }
      for (int k = 0; k < (q->nof_re * ISRRAN_NPDSCH_MAX_NOF_SF); k++) {
        q->ce_buffer[i][k] = 1;
      }
    }

    // allocate memory for soft-bits
    q->llr = isrran_vec_f_malloc(q->nof_re * ISRRAN_NPDSCH_MAX_NOF_SF * 2);
    if (!q->llr) {
      goto clean_exit;
    }
    isrran_vec_f_zero(q->llr, q->nof_re * ISRRAN_NPDSCH_MAX_NOF_SF * 2);

    // initialize memory
    if (isrran_ofdm_rx_init(&q->fft, ISRRAN_CP_NORM, in_buffer[0], q->sf_symbols, max_prb)) {
      fprintf(stderr, "Error initiating FFT\n");
      goto clean_exit;
    }
    isrran_ofdm_set_freq_shift(&q->fft, ISRRAN_NBIOT_FREQ_SHIFT_FACTOR);
    // isrran_ofdm_set_normalize(&q->fft, true);

    if (isrran_chest_dl_nbiot_init(&q->chest, max_prb)) {
      fprintf(stderr, "Error initiating channel estimator\n");
      goto clean_exit;
    }

    if (isrran_npdcch_init(&q->npdcch)) {
      fprintf(stderr, "Error creating PDCCH object\n");
      goto clean_exit;
    }

    if (isrran_npdsch_init(&q->npdsch)) {
      fprintf(stderr, "Error creating PDSCH object\n");
      goto clean_exit;
    }
    if (isrran_softbuffer_rx_init(&q->softbuffer, max_prb)) {
      fprintf(stderr, "Error initiating soft buffer\n");
      goto clean_exit;
    }
    if (isrran_cfo_init(&q->sfo_correct, max_prb * ISRRAN_NRE)) {
      fprintf(stderr, "Error initiating SFO correct\n");
      goto clean_exit;
    }
    isrran_cfo_set_tol(&q->sfo_correct, 1e-5 / q->fft.cfg.symbol_sz);

    ret = ISRRAN_SUCCESS;
  } else {
    fprintf(stderr, "Invalid UE DL object\n");
  }

clean_exit:
  if (ret == ISRRAN_ERROR) {
    isrran_nbiot_ue_dl_free(q);
  }
  return ret;
}

void isrran_nbiot_ue_dl_free(isrran_nbiot_ue_dl_t* q)
{
  if (q) {
    isrran_ofdm_rx_free(&q->fft);
    isrran_chest_dl_nbiot_free(&q->chest);
    isrran_npdcch_free(&q->npdcch);
    isrran_npdsch_free(&q->npdsch);
    isrran_cfo_free(&q->sfo_correct);
    isrran_softbuffer_rx_free(&q->softbuffer);
    if (q->sf_symbols) {
      free(q->sf_symbols);
    }
    for (uint32_t i = 0; i < ISRRAN_MAX_PORTS; i++) {
      if (q->ce[i]) {
        free(q->ce[i]);
      }
    }
    if (q->sf_buffer) {
      free(q->sf_buffer);
    }
    for (uint32_t i = 0; i < ISRRAN_MAX_PORTS; i++) {
      if (q->ce_buffer[i]) {
        free(q->ce_buffer[i]);
      }
    }
    if (q->llr) {
      free(q->llr);
    }
    bzero(q, sizeof(isrran_nbiot_ue_dl_t));
  }
}

int isrran_nbiot_ue_dl_set_cell(isrran_nbiot_ue_dl_t* q, isrran_nbiot_cell_t cell)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL && isrran_nbiot_cell_isvalid(&cell)) {
    q->pkt_errors    = 0;
    q->pkts_total    = 0;
    q->sample_offset = 0;

    if (q->cell.n_id_ncell != cell.n_id_ncell || q->cell.base.nof_prb == 0) {
      q->cell = cell;

      if (isrran_chest_dl_nbiot_set_cell(&q->chest, q->cell)) {
        fprintf(stderr, "Error setting channel estimator's cell configuration\n");
        return -1;
      }

      if (isrran_npdcch_set_cell(&q->npdcch, q->cell)) {
        fprintf(stderr, "Error resizing NPDCCH object\n");
        return ISRRAN_ERROR;
      }

      if (isrran_npdsch_set_cell(&q->npdsch, q->cell)) {
        fprintf(stderr, "Error creating NPDSCH object\n");
        return ISRRAN_ERROR;
      }
      q->current_rnti = 0;
    }
    ret = ISRRAN_SUCCESS;
  } else {
    fprintf(stderr,
            "Invalid cell properties: n_id_ncell=%d, Ports=%d, base cell's PRBs=%d\n",
            cell.n_id_ncell,
            cell.nof_ports,
            cell.base.nof_prb);
  }
  return ret;
}

// Section 5.2.1.2a in 3GPP 36.331
void isrran_nbiot_ue_dl_set_mib(isrran_nbiot_ue_dl_t* q, isrran_mib_nb_t mib)
{
  // compute frames that contain SIB1-NB transmissions (there are 4 SIB1 cycles before SFN counter wraps)
  uint32_t num_rep    = isrran_ra_n_rep_sib1_nb(&mib);
  uint32_t sib1_start = isrran_ra_nbiot_get_starting_sib1_frame(q->cell.n_id_ncell, &mib);
  uint32_t idx        = 0;

  for (int k = 0; k < 4; k++) {
    // in each period, the SIB is transmitted nrep-times over 16 consecutive frames in every second frame
    for (int i = 0; i < num_rep; i++) {
      q->sib1_sfn[idx] = k * SIB1_NB_TTI + sib1_start + (i * SIB1_NB_TTI / num_rep);
      idx++;
    }
  }
  assert(idx <= 4 * SIB1_NB_MAX_REP && idx == 4 * num_rep);

  if (ISRRAN_VERBOSE_ISINFO()) {
    INFO("%d DL SIB1-NB SFNs: ", 4 * num_rep);
    isrran_vec_fprint_i(stdout, (int*)q->sib1_sfn, num_rep * 4);
  }

  // compute _all_ SIB1 subframes and store them in lookup table
  for (int i = 0; i < SIB1_NB_MAX_REP * 4; i++) {
    // all even SFN within the next 16 SFN starting at q->sib1_sfn[i] are valid
    for (int k = 0; k < 16; k += 2) {
      uint32_t sib1_tti   = (q->sib1_sfn[i] + k) * 10 + 4;
      q->si_tti[sib1_tti] = true;
    }
  }

  // store MIB
  q->mib     = mib;
  q->mib_set = true;
}

void isrran_nbiot_ue_dl_set_si_params(isrran_nbiot_ue_dl_t*    q,
                                      isrran_nbiot_si_type_t   type,
                                      isrran_nbiot_si_params_t params)
{
  if (type < ISRRAN_NBIOT_SI_TYPE_NITEMS) {
    if (params.n != 0) {
      // calculate TTIs for this SI
      int x = (params.n - 1) * params.si_window_length;

      for (int sfn = 0; sfn < 1024; sfn++) {
        int tmp_hfn = 0;
        if (((tmp_hfn * 1024 + sfn) % params.si_periodicity) == (floor(x / 10.0) + params.si_radio_frame_offset)) {
          // this flags the start of the SI

          // SI is always transmitted in subframe 1
          int si_start_tti = 10 * sfn + 1;
          INFO("Start SI tx in TTI=%d", si_start_tti);

          do {
            // TBS 56 and 120 are sent over 2 sub-frames, all other over 8
            int num_sf = (params.si_tb > 120) ? 8 : 2;
            int si_tti = si_start_tti;
            while (num_sf > 0) {
              // only add if it's a valid DL SF and is not used for SIB1
              if (isrran_ra_nbiot_is_valid_dl_sf(si_tti) &&
                  !isrran_nbiot_ue_dl_is_sib1_sf(q, si_tti / 10, si_tti % 10)) {
                q->si_tti[si_tti] = true;
                num_sf--;
              }
              si_tti = (si_tti + 1) % 10240;
            }
            // advance start TTI to beginning of next repetition
            si_start_tti += params.si_repetition_pattern * 10;
          } while (si_start_tti <= sfn * 10 + params.si_window_length); // SI is repeated within it's window
        }
      }
    }

    // store parameter
    q->si_params[type] = params;
  }
}

bool isrran_nbiot_ue_dl_is_sib1_sf(isrran_nbiot_ue_dl_t* q, uint32_t sfn, uint32_t sf_idx)
{
  bool ret = false;
  if (sf_idx != 4)
    return ret;

  if (q->mib_set) {
    // TODO: replace linear search
    for (int i = 0; i < SIB1_NB_MAX_REP * 4; i++) {
      // every second frame within the next 16 SFN starting at q->sib1_sfn[i] are valid
      uint32_t valid_si_sfn = sfn + isrran_ra_nbiot_sib1_start(q->cell.n_id_ncell, &q->mib);
      if ((sfn >= q->sib1_sfn[i]) && (sfn < q->sib1_sfn[i] + 16) && (valid_si_sfn % 2 == 0)) {
        ret = true;
        break;
      }
    }
  } else {
    fprintf(stderr, "Can't compute SIB1 location because MIB-NB not set.\n");
  }
  return ret;
}

bool isrran_nbiot_ue_dl_is_si_tti(isrran_nbiot_ue_dl_t* q, uint32_t sfn, uint32_t sf_idx)
{
  return q->si_tti[sfn * 10 + sf_idx];
}

uint32_t isrran_nbiot_ue_dl_get_next_sib1_start(isrran_nbiot_ue_dl_t* q, uint32_t current_sfn)
{
  if (q->mib_set) {
    uint32_t offset = isrran_ra_nbiot_get_starting_sib1_frame(q->cell.n_id_ncell, &q->mib);
    for (int k = 0; k < 4; k++) {
      uint32_t period = k * SIB1_NB_TTI + offset;
      if (period > current_sfn)
        return period;
    }
    return q->sib1_sfn[0];
  } else {
    fprintf(stderr, "Can't compute SIB1 start because MIB-NB not set.\n");
  }
  return ISRRAN_SUCCESS;
}

void isrran_nbiot_ue_dl_get_sib1_grant(isrran_nbiot_ue_dl_t* q, uint32_t sfn, isrran_ra_nbiot_dl_grant_t* grant)
{
  // compute parameter for SIB1 transmissions
  isrran_ra_nbiot_dl_dci_t ra_dl_sib1;
  bzero(&ra_dl_sib1, sizeof(isrran_ra_nbiot_dl_dci_t));
  ra_dl_sib1.alloc.has_sib1        = true;
  ra_dl_sib1.alloc.sched_info_sib1 = q->mib.sched_info_sib1;
  isrran_ra_nbiot_dl_dci_to_grant(
      &ra_dl_sib1, grant, isrran_nbiot_ue_dl_get_next_sib1_start(q, sfn), 4, DUMMY_R_MAX, true, q->cell.mode);
  grant->has_sib1 = true;
}

// Calculate next SIB window as in 5.2.3a in TS 36.331
// TODO: get rid of this brute-force calculation
void isrran_nbiot_ue_dl_get_next_si_sfn(uint32_t                 current_hfn,
                                        uint32_t                 current_sfn,
                                        isrran_nbiot_si_params_t params,
                                        uint32_t*                si_hfn,
                                        uint32_t*                si_sfn)
{
  int x       = (params.n - 1) * params.si_window_length;
  int tmp_sfn = current_sfn + 1;
  int tmp_hfn = current_hfn;
  while (((tmp_hfn * 1024 + tmp_sfn) % params.si_periodicity) != (floor(x / 10.0) + params.si_radio_frame_offset)) {
    tmp_sfn = (tmp_sfn + 1) % 1024;
    if (si_sfn == 0) {
      tmp_hfn = (tmp_hfn + 1) % 1024;
    }
  }
  if (si_hfn && si_sfn) {
    *si_hfn = tmp_hfn % 1024;
    *si_sfn = tmp_sfn % 1024;
  }
}

void isrran_nbiot_ue_dl_get_sib_grant(isrran_nbiot_ue_dl_t*       q,
                                      uint32_t                    hfn,
                                      uint32_t                    sfn,
                                      isrran_nbiot_si_params_t    params,
                                      isrran_ra_nbiot_dl_grant_t* grant)
{
  isrran_ra_nbiot_dl_dci_t si_dci;
  bzero(&si_dci, sizeof(isrran_ra_nbiot_dl_dci_t));

  si_dci.alloc.has_sib1  = false;
  si_dci.alloc.is_ra     = false;
  si_dci.alloc.i_delay   = 0;
  si_dci.alloc.i_sf      = (params.si_tb > 120) ? 6 : 1; // TBS 56 and 120 are sent over 2 sub-frames, all other over 8
  si_dci.alloc.i_rep     = 0;
  si_dci.alloc.harq_ack  = 1;
  si_dci.alloc.i_n_start = 0;

  isrran_ra_nbiot_dl_dci_to_grant(&si_dci, grant, sfn, sfn % 2, DUMMY_R_MAX, true, q->cell.mode);
  grant->mcs[0].tbs = params.si_tb; // The TBS is given by SI parameters set in SIB1
  isrran_nbiot_ue_dl_get_next_si_sfn(hfn, sfn, params, &grant->start_hfn, &grant->start_sfn);
  grant->start_sfidx = 1;
}

void isrran_nbiot_ue_dl_decode_sib1(isrran_nbiot_ue_dl_t* q, uint32_t current_sfn)
{
  // activate SIB1 grant and configure NPDSCH
  INFO(
      "%d.x: Activated SIB1 decoding in sfn=%d\n", current_sfn, isrran_nbiot_ue_dl_get_next_sib1_start(q, current_sfn));
  isrran_ra_nbiot_dl_grant_t grant = {};
  isrran_nbiot_ue_dl_get_sib1_grant(q, current_sfn, &grant);
  isrran_nbiot_ue_dl_set_grant(q, &grant);
}

void isrran_nbiot_ue_dl_decode_sib(isrran_nbiot_ue_dl_t*    q,
                                   uint32_t                 hfn,
                                   uint32_t                 sfn,
                                   isrran_nbiot_si_type_t   type,
                                   isrran_nbiot_si_params_t params)
{
  if (q->has_dl_grant) {
    INFO("Already processing grant, skipping this.");
    return;
  }

  if (type == ISRRAN_NBIOT_SI_TYPE_SIB2) {
    assert(params.n == 1);
    // calculate SIB2 params
    isrran_ra_nbiot_dl_grant_t grant = {};
    isrran_nbiot_ue_dl_get_sib_grant(q, hfn, sfn, params, &grant);
    isrran_nbiot_ue_dl_set_grant(q, &grant);
    INFO("%d.x: Activated SIB2 reception in hfn=%d, sfn=%d",
         sfn,
         q->npdsch_cfg.grant.start_hfn,
         q->npdsch_cfg.grant.start_sfn);
  } else {
    INFO("Not handling this SI type.");
  }
}

void isrran_nbiot_ue_dl_set_grant(isrran_nbiot_ue_dl_t* q, isrran_ra_nbiot_dl_grant_t* grant)
{
  // configure NPDSCH object
  isrran_npdsch_cfg(&q->npdsch_cfg, q->cell, grant, grant->start_sfidx);
  q->has_dl_grant = true;
}

void isrran_nbiot_ue_dl_flush_grant(isrran_nbiot_ue_dl_t* q)
{
  bzero(&q->npdsch_cfg, sizeof(isrran_npdsch_cfg_t));
  q->has_dl_grant = false;
}

bool isrran_nbiot_ue_dl_has_grant(isrran_nbiot_ue_dl_t* q)
{
  return q->has_dl_grant;
}

/* Helper function to verify a DL grant does not start
 * on a subframe that is not a valid DL subframe, i.e., contains
 * synchronization signal or is an SI subframe.
 *
 * \param q pointer to the UE DL object
 * \param grant the DL grant to check
 */
void isrran_nbiot_ue_dl_check_grant(isrran_nbiot_ue_dl_t* q, isrran_ra_nbiot_dl_grant_t* grant)
{
  // make sure NPDSCH start is not on a SI subframe
  while (isrran_nbiot_ue_dl_is_si_tti(q, grant->start_sfn, grant->start_sfidx) ||
         !isrran_ra_nbiot_is_valid_dl_sf(grant->start_sfn * 10 + grant->start_sfidx)) {
    grant->start_sfidx++;
    if (grant->start_sfidx == 10) {
      grant->start_sfidx = 0;
      grant->start_sfn   = (grant->start_sfn + 1) % 1024;
    }
  }
}

/* Precalculate the NPDSCH scramble sequences for a given RNTI. This function takes a while
 * to execute, so shall be called once the final C-RNTI has been allocated for the session.
 */
void isrran_nbiot_ue_dl_set_rnti(isrran_nbiot_ue_dl_t* q, uint16_t rnti)
{
  isrran_npdsch_set_rnti(&q->npdsch, rnti);
  q->current_rnti = rnti;
}

void isrran_nbiot_ue_dl_reset(isrran_nbiot_ue_dl_t* q)
{
  isrran_softbuffer_rx_reset(&q->softbuffer);
  bzero(&q->npdsch_cfg, sizeof(isrran_npdsch_cfg_t));
}

void isrran_nbiot_ue_dl_set_sample_offset(isrran_nbiot_ue_dl_t* q, float sample_offset)
{
  q->sample_offset = sample_offset;
}

int isrran_nbiot_ue_dl_decode_fft_estimate(isrran_nbiot_ue_dl_t* q, uint32_t sf_idx, bool is_dl_sf)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q != NULL && sf_idx < ISRRAN_NOF_SF_X_FRAME) {
    ret = ISRRAN_ERROR;

    // Run FFT for all subframe data
    isrran_ofdm_rx_sf(&q->fft);

    // Correct SFO multiplying by complex exponential in the time domain
    if (q->sample_offset) {
      for (int i = 0; i < 2 * ISRRAN_CP_NSYMB(q->cell.base.cp); i++) {
        isrran_cfo_correct(&q->sfo_correct,
                           &q->sf_symbols[i * q->cell.base.nof_prb * ISRRAN_NRE],
                           &q->sf_symbols[i * q->cell.base.nof_prb * ISRRAN_NRE],
                           q->sample_offset / q->fft.cfg.symbol_sz);
      }
    }

    bool sf_has_nrs = false;

    if (q->cell.nof_ports == 0) {
      sf_has_nrs = isrran_ra_nbiot_dl_has_ref_signal(sf_idx);
    } else if (q->cell.mode == ISRRAN_NBIOT_MODE_STANDALONE || q->cell.mode == ISRRAN_NBIOT_MODE_GUARDBAND) {
      // TODO: differentiate between before and after SIB reception
      sf_has_nrs = isrran_ra_nbiot_dl_has_ref_signal_standalone(sf_idx);
    } else {
      sf_has_nrs = isrran_ra_nbiot_dl_has_ref_signal_inband(sf_idx);
    }

    if (sf_has_nrs || is_dl_sf) {
      assert(sf_idx != 5);
      ret = isrran_nbiot_ue_dl_decode_estimate(q, sf_idx);
    } else {
      ret = ISRRAN_SUCCESS;
    }
  }

  return ret;
}

int isrran_nbiot_ue_dl_decode_estimate(isrran_nbiot_ue_dl_t* q, uint32_t sf_idx)
{
  if (q && sf_idx < ISRRAN_NOF_SF_X_FRAME) {
    // Get channel estimates for each port
    isrran_chest_dl_nbiot_estimate(&q->chest, q->sf_symbols, q->ce, sf_idx);
    return ISRRAN_SUCCESS;
  } else {
    return ISRRAN_ERROR_INVALID_INPUTS;
  }
}

int isrran_nbiot_ue_dl_decode_rnti_packet(isrran_nbiot_ue_dl_t*       q,
                                          isrran_ra_nbiot_dl_grant_t* grant,
                                          uint8_t*                    data,
                                          uint32_t                    sfn,
                                          uint32_t                    sf_idx,
                                          uint16_t                    rnti,
                                          cf_t*                       symbols,
                                          cf_t*                       ce[ISRRAN_MAX_PORTS],
                                          uint32_t                    rep_counter)
{
  // Uncomment next line to do ZF by default
  // float noise_estimate = 0;
  float noise_estimate = isrran_chest_dl_nbiot_get_noise_estimate(&q->chest);
  return isrran_npdsch_decode_rnti(
      &q->npdsch, &q->npdsch_cfg, &q->softbuffer, symbols, ce, noise_estimate, rnti, sfn, data, rep_counter);
}

void isrran_nbiot_ue_dl_tb_decoded(isrran_nbiot_ue_dl_t* q, uint8_t* data)
{
  // print decoded message
  if (ISRRAN_VERBOSE_ISINFO()) {
    INFO("Decoded Message: ");
    isrran_vec_fprint_byte(stdout, data, q->npdsch_cfg.grant.mcs[0].tbs / 8);
  }
  // do book-keeping
  q->pkts_ok++;
  q->bits_total += q->npdsch_cfg.grant.mcs[0].tbs;

  // de-activate grant
  q->has_dl_grant = false;
}

uint32_t isrran_nbiot_ue_dl_get_ncce(isrran_nbiot_ue_dl_t* q)
{
  return q->last_n_cce;
}

#define ISRRAN_MAX_CANDIDATES_UE 3 // From 36.213 Table 16.6-1 NPDCCH Format0 and NPDCCH Format1
#define ISRRAN_MAX_CANDIDATES_COM                                                                                      \
  1 // From 36.213 Table 16.6-2 and Table 16.6-3, only AL2 is defined here which uses NPDCCH Format1
#define ISRRAN_MAX_CANDIDATES (ISRRAN_MAX_CANDIDATES_UE + ISRRAN_MAX_CANDIDATES_COM)

typedef struct {
  isrran_dci_format_t   format;
  isrran_dci_location_t loc[ISRRAN_MAX_CANDIDATES];
  uint32_t              nof_locations;
} dci_blind_search_t;

static int
dci_blind_search(isrran_nbiot_ue_dl_t* q, dci_blind_search_t* search_space, uint16_t rnti, isrran_dci_msg_t* dci_msg)
{
  int      ret     = ISRRAN_ERROR;
  uint16_t crc_rem = 0;
  if (rnti) {
    ret   = 0;
    int i = 0;
    while (!ret && i < search_space->nof_locations) {
      DEBUG("Searching format %s in %d,%d",
            isrran_dci_format_string(search_space->format),
            search_space->loc[i].ncce,
            search_space->loc[i].L);
      if (isrran_npdcch_decode_msg(&q->npdcch, dci_msg, &search_space->loc[i], search_space->format, &crc_rem)) {
        fprintf(stderr, "Error decoding DCI msg\n");
        return ISRRAN_ERROR;
      }
      if (crc_rem == rnti) {
        if (dci_msg->format == search_space->format) {
          ret = 1;
          memcpy(&q->last_location, &search_space->loc[i], sizeof(isrran_dci_location_t));
        }
      }
      i++;
    }
  } else {
    fprintf(stderr, "RNTI not specified\n");
  }
  return ret;
}

int isrran_nbiot_ue_dl_find_dl_dci(isrran_nbiot_ue_dl_t* q, uint32_t sf_idx, uint16_t rnti, isrran_dci_msg_t* dci_msg)
{
  if (rnti == ISRRAN_SIRNTI || rnti == ISRRAN_PRNTI || ISRRAN_RNTI_ISRAR(rnti)) {
    return isrran_nbiot_ue_dl_find_dl_dci_type_siprarnti(q, rnti, dci_msg);
  } else {
    return isrran_nbiot_ue_dl_find_dl_dci_type_crnti(q, sf_idx, rnti, dci_msg);
  }
}

// Blind search for SI/P/RA-RNTI
int isrran_nbiot_ue_dl_find_dl_dci_type_siprarnti(isrran_nbiot_ue_dl_t* q, uint16_t rnti, isrran_dci_msg_t* dci_msg)
{
  int ret = ISRRAN_SUCCESS;
  // Configure and run DCI blind search
  dci_blind_search_t search_space;

  search_space.nof_locations = isrran_npdcch_common_locations(search_space.loc, ISRRAN_MAX_CANDIDATES_COM);
  DEBUG("Searching SI/P/RA-RNTI in %d common locations, %d formats", search_space.nof_locations, nb_nof_common_formats);
  // Search for RNTI only if there is room for the common search space
  if (search_space.nof_locations > 0) {
    for (int f = 0; f < nb_nof_common_formats; f++) {
      search_space.format = nb_common_formats[f];
      if ((ret = dci_blind_search(q, &search_space, rnti, dci_msg))) {
        return ret;
      }
    }
  }
  return ISRRAN_SUCCESS;
}

// Blind search for C-RNTI
int isrran_nbiot_ue_dl_find_dl_dci_type_crnti(isrran_nbiot_ue_dl_t* q,
                                              uint32_t              sf_idx,
                                              uint16_t              rnti,
                                              isrran_dci_msg_t*     dci_msg)
{
  int ret = ISRRAN_SUCCESS;

  // Search UE-specific search space
  dci_blind_search_t search_space;
  search_space.nof_locations = isrran_npdcch_ue_locations(search_space.loc, ISRRAN_MAX_CANDIDATES_UE);
  DEBUG("x.%d: Searching DL C-RNTI=0x%x in %d locations, %d formats",
        sf_idx,
        rnti,
        search_space.nof_locations,
        nof_nb_ue_formats);
  for (int f = 0; f < nof_nb_ue_formats; f++) {
    search_space.format = nb_ue_formats[f];
    if ((ret = dci_blind_search(q, &search_space, rnti, dci_msg))) {
      return ret;
    }
  }
  return ret;
}

/** Attempts to decode NPDSCH with pre-configured grant
 *  - It does not decode SIBs if no explicit grant is present
 */
int isrran_nbiot_ue_dl_decode_npdsch(isrran_nbiot_ue_dl_t* q,
                                     cf_t*                 input,
                                     uint8_t*              data,
                                     uint32_t              sfn,
                                     uint32_t              sf_idx,
                                     uint16_t              rnti)
{
  int ret = ISRRAN_ERROR;

  // skip subframe without grant and if it's not a valid downlink subframe
  if (q->has_dl_grant == false || isrran_ra_nbiot_is_valid_dl_sf(sfn * 10 + sf_idx) == false) {
    DEBUG("%d.%d: Skipping NPDSCH processing.", sfn, sf_idx);
    return ISRRAN_NBIOT_UE_DL_SKIP_SF;
  }

  // run FFT and estimate channel
  DEBUG("%d.%d: Estimating channel.", sfn, sf_idx);
  if ((isrran_nbiot_ue_dl_decode_fft_estimate(q, sf_idx, true)) < 0) {
    return ret;
  }

  // skip SIB1 sub-frames
  if (isrran_nbiot_ue_dl_is_sib1_sf(q, sfn, sf_idx) && q->has_dl_grant) {
    if (q->npdsch_cfg.grant.has_sib1 == false) {
      // skip SIB1 decoding if grant is being processed
      DEBUG("%d.%d: Skipping SIB1 due to ongoing DL reception.", sfn, sf_idx);
      return ISRRAN_NBIOT_EXPECT_MORE_SF;
    }
  }

  // Wait with reception until subframe specified in grant (TODO: add hfn check)
  if ((q->npdsch_cfg.grant.start_sfn == sfn && q->npdsch_cfg.grant.start_sfidx == sf_idx) || q->npdsch_cfg.num_sf > 0) {
    // count first NPDSCH subframe as received frame
    if (q->npdsch_cfg.num_sf == 0) {
      q->pkts_total++;
    }

    // handle actual reception
    if (q->npdsch_cfg.has_bcch) {
      ret = isrran_nbiot_ue_dl_decode_npdsch_bcch(q, data, sfn * 10 + sf_idx);
    } else {
      ret = isrran_nbiot_ue_dl_decode_npdsch_no_bcch(q, data, sfn * 10 + sf_idx, rnti);
    }
  } else {
    DEBUG("%d.%d: WARNING: DL grant still active. Possibly needs to be deactivated.", sfn, sf_idx);
    ret = ISRRAN_NBIOT_UE_DL_SKIP_SF;
  }

  return ret;
}

/** Handles subframe reception of a NPDSCH which doesn't carry the BCCH
 *  - In this NPDSCH config, up to four repetitons are transmitted one after another
 */
int isrran_nbiot_ue_dl_decode_npdsch_no_bcch(isrran_nbiot_ue_dl_t* q, uint8_t* data, uint32_t tti, uint16_t rnti)
{
  int ret = ISRRAN_ERROR_INVALID_INPUTS;

  if (q->npdsch_cfg.sf_idx >= ISRRAN_NPDSCH_MAX_NOF_SF) {
    ERROR("Invalid npdsch_cfg.sf_idx=%d", q->npdsch_cfg.sf_idx);
    return ret;
  }

  INFO("%d.%d: NPDSCH processing sf_idx=%d/%d rep=%d/%d tot=%d/%d",
       tti / 10,
       tti % 10,
       q->npdsch_cfg.sf_idx + 1,
       q->npdsch_cfg.grant.nof_sf,
       q->npdsch_cfg.rep_idx + 1,
       q->npdsch_cfg.grant.nof_rep,
       q->npdsch_cfg.num_sf + 1,
       q->npdsch_cfg.grant.nof_sf * q->npdsch_cfg.grant.nof_rep);

  if (q->npdsch_cfg.num_sf % q->npdsch_cfg.grant.nof_rep == 0) {
    // copy data and ce symbols for first repetition of each subframe
    isrran_vec_cf_copy(&q->sf_buffer[q->npdsch_cfg.sf_idx * q->nof_re], q->sf_symbols, q->nof_re);
    for (int i = 0; i < q->cell.nof_ports; i++) {
      isrran_vec_cf_copy(&q->ce_buffer[i][q->npdsch_cfg.sf_idx * q->nof_re], q->ce[i], q->nof_re);
    }
  } else {
    // accumulate subframe samples and channel estimates
    isrran_vec_sum_ccc(&q->sf_buffer[q->npdsch_cfg.sf_idx * q->nof_re],
                       q->sf_symbols,
                       &q->sf_buffer[q->npdsch_cfg.sf_idx * q->nof_re],
                       q->nof_re);
    for (int i = 0; i < q->cell.nof_ports; i++) {
      isrran_vec_sum_ccc(&q->ce_buffer[i][q->npdsch_cfg.sf_idx * q->nof_re],
                         q->ce[i],
                         &q->ce_buffer[i][q->npdsch_cfg.sf_idx * q->nof_re],
                         q->nof_re);
    }
  }
  q->npdsch_cfg.num_sf++;
  // isrran_nbiot_ue_dl_save_signal(q, input, sfn, sf_idx);

  q->npdsch_cfg.rep_idx++;
  int m = ISRRAN_MIN(q->npdsch_cfg.grant.nof_rep, 4);
  if (q->npdsch_cfg.rep_idx % m == 0) {
    // average accumulated samples
    isrran_vec_sc_prod_ccc(&q->sf_buffer[q->npdsch_cfg.sf_idx * q->nof_re],
                           1.0 / m,
                           &q->sf_buffer[q->npdsch_cfg.sf_idx * q->nof_re],
                           q->nof_re);
    for (int i = 0; i < q->cell.nof_ports; i++) {
      isrran_vec_sc_prod_ccc(&q->ce_buffer[i][q->npdsch_cfg.sf_idx * q->nof_re],
                             1.0 / m,
                             &q->ce_buffer[i][q->npdsch_cfg.sf_idx * q->nof_re],
                             q->nof_re);
    }

    q->npdsch_cfg.sf_idx++;
    if (q->npdsch_cfg.sf_idx == q->npdsch_cfg.grant.nof_sf) {
      q->npdsch_cfg.sf_idx = 0;
    } else {
      q->npdsch_cfg.rep_idx -= m;
    }
  }

  if (q->npdsch_cfg.num_sf == q->npdsch_cfg.grant.nof_sf * q->npdsch_cfg.grant.nof_rep) {
    // try to decode NPDSCH
    INFO("%d.%d: Trying to decode NPDSCH with %d subframe(s).", tti / 10, tti % 10, q->npdsch_cfg.grant.nof_sf);
    if (isrran_nbiot_ue_dl_decode_rnti_packet(q,
                                              &q->npdsch_cfg.grant,
                                              data,
                                              tti / 10,
                                              tti % 10,
                                              rnti,
                                              q->sf_buffer,
                                              q->ce_buffer,
                                              q->npdsch_cfg.rep_idx) != ISRRAN_SUCCESS) {
      // decoding failed
      INFO("%d.%d: Error decoding NPDSCH with %d repetitions.", tti / 10, tti % 10, q->npdsch_cfg.rep_idx);
      q->pkt_errors++;
      q->has_dl_grant = false;
      ret             = ISRRAN_ERROR;
    } else {
      isrran_nbiot_ue_dl_tb_decoded(q, data);
      ret = ISRRAN_SUCCESS;
    }
  } else {
    DEBUG("%d.%d: Waiting for %d more subframes.",
          tti / 10,
          tti % 10,
          q->npdsch_cfg.grant.nof_sf * q->npdsch_cfg.grant.nof_rep - q->npdsch_cfg.num_sf);
    ret = ISRRAN_NBIOT_EXPECT_MORE_SF;
  }
  return ret;
}

/** Handles subframe reception of a NPDSCH carrying the BCCH
 *  - According to TS36.211 Section 10.2.3.4 for NPDSCH carrying BCCH, all SF are
 *    transmitted in sequence before repetions are transmitted
 */
int isrran_nbiot_ue_dl_decode_npdsch_bcch(isrran_nbiot_ue_dl_t* q, uint8_t* data, uint32_t tti)
{
  int ret = ISRRAN_NBIOT_EXPECT_MORE_SF;

  // TODO: this only captures SIB1, not other SIBs
  // only subframe 4 of every second frame is an SIB1
  // make sure we also look on odd SFNs (if n_rep=16 and n_id_ncell%2=1)
  uint32_t valid_si_sfn = tti / 10 + isrran_ra_nbiot_sib1_start(q->cell.n_id_ncell, &q->mib);
  if (valid_si_sfn % 2 == 0 && tti % 10 == 4) {
    INFO("%d.%d: NPDSCH processing sf_idx=%d/%d rep=%d/%d tot=%d/%d",
         tti / 10,
         tti % 10,
         q->npdsch_cfg.sf_idx + 1,
         q->npdsch_cfg.grant.nof_sf,
         q->npdsch_cfg.rep_idx + 1,
         q->npdsch_cfg.grant.nof_rep,
         q->npdsch_cfg.num_sf + 1,
         q->npdsch_cfg.grant.nof_sf * q->npdsch_cfg.grant.nof_rep);

    // copy data and ce symbols
    isrran_vec_cf_copy(&q->sf_buffer[q->npdsch_cfg.sf_idx * q->nof_re], q->sf_symbols, q->nof_re);
    for (int i = 0; i < q->cell.nof_ports; i++) {
      isrran_vec_cf_copy(&q->ce_buffer[i][q->npdsch_cfg.sf_idx * q->nof_re], q->ce[i], q->nof_re);
    }
    q->npdsch_cfg.num_sf++;
    q->npdsch_cfg.sf_idx++;

    // check if we already have received the entire transmission
    if (q->npdsch_cfg.num_sf % q->npdsch_cfg.grant.nof_sf == 0) {
      // try to decode NPDSCH
      INFO("%d.%d: Trying to decode NPDSCH with %d subframe(s).", tti / 10, tti % 10, q->npdsch_cfg.grant.nof_sf);
      if (isrran_nbiot_ue_dl_decode_rnti_packet(q,
                                                &q->npdsch_cfg.grant,
                                                data,
                                                tti / 10,
                                                tti % 10,
                                                ISRRAN_SIRNTI,
                                                q->sf_buffer,
                                                q->ce_buffer,
                                                q->npdsch_cfg.rep_idx) != ISRRAN_SUCCESS) {
        // decoding failed, check for possible repetitions
        if (q->npdsch_cfg.rep_idx == 0) {
          // store soft-bits of first repetition
          isrran_vec_f_copy(q->llr, q->npdsch.llr, q->npdsch_cfg.grant.nof_sf * q->npdsch_cfg.nbits.nof_bits);
        } else {
          INFO("Soft-combining NPDSCH repetition %d", q->npdsch_cfg.rep_idx);
          isrran_vec_sum_fff(q->llr, q->npdsch.llr, q->llr, q->npdsch_cfg.grant.nof_sf * q->npdsch_cfg.nbits.nof_bits);

          // try to decode combined soft-bits
          INFO("%d.%d: Trying to decode NPDSCH with %d subframe(s) after %d repetitions.",
               tti / 10,
               tti % 10,
               q->npdsch_cfg.grant.nof_sf,
               q->npdsch_cfg.rep_idx);

          if (isrran_npdsch_rm_and_decode(&q->npdsch, &q->npdsch_cfg, q->llr, data) == ISRRAN_SUCCESS) {
            // frame decoded ok after one or more repetitions
            isrran_nbiot_ue_dl_tb_decoded(q, data);
            q->has_dl_grant = false;
            return ISRRAN_SUCCESS;
          } else {
            ret = ISRRAN_NBIOT_EXPECT_MORE_SF;
          }
        }

        // NPDSCH carrying the BCCH starts with subframe zero again
        q->npdsch_cfg.sf_idx = 0;
        q->npdsch_cfg.rep_idx++;

        if (q->npdsch_cfg.rep_idx < q->npdsch_cfg.grant.nof_rep) {
          DEBUG("%d.%d: Couldn't decode NPDSCH, waiting for next repetition", tti / 10, tti % 10);
          ret = ISRRAN_NBIOT_EXPECT_MORE_SF;
        } else {
          INFO("%d.%d: Error decoding NPDSCH with %d repetitions.", tti / 10, tti % 10, q->npdsch_cfg.rep_idx);
          ret = ISRRAN_ERROR;
          q->pkt_errors++; // count as error after all repetitons failed
          q->has_dl_grant = false;
        }
      } else {
        // frame decoded ok after first transmission
        isrran_nbiot_ue_dl_tb_decoded(q, data);
        return ISRRAN_SUCCESS;
      }
    } else {
      DEBUG("%d.%d: Waiting for more subframes.", tti / 10, tti % 10);
      ret = ISRRAN_NBIOT_EXPECT_MORE_SF;
    }
  }
  return ret;
}

/** Applies the following operations to a subframe of synchronized samples:
 *    - OFDM demodulation
 *    - Channel estimation (only for subframes containing NRS)
 *    - NPDCCH decoding: Find DCI for RNTI given by rnti paramter
 */
int isrran_nbiot_ue_dl_decode_npdcch(isrran_nbiot_ue_dl_t* q,
                                     cf_t*                 input,
                                     uint32_t              sfn,
                                     uint32_t              sf_idx,
                                     uint16_t              rnti,
                                     isrran_dci_msg_t*     dci_msg)
{
  int ret = ISRRAN_ERROR;

  // skip subframe with grant and if it's not a valid downlink subframe
  if (q->has_dl_grant || isrran_ra_nbiot_is_valid_dl_sf(sfn * 10 + sf_idx) == false) {
    DEBUG("%d.%d: Skipping NPDCCH processing.", sfn, sf_idx);
    return ret;
  }

  // run FFT and estimate channel
  DEBUG("%d.%d: Estimating channel.", sfn, sf_idx);
  if ((isrran_nbiot_ue_dl_decode_fft_estimate(q, sf_idx, true)) < 0) {
    return ret;
  }

  // handle SI sub-frames
  if (isrran_nbiot_ue_dl_is_si_tti(q, sfn, sf_idx) && q->has_dl_grant) {
    if (q->npdsch_cfg.grant.has_sib1 == false) {
      // skip SI decoding if grant is being processed
      DEBUG("%d.%d: Skipping SI SF due to ongoing DL reception.", sfn, sf_idx);
      return ISRRAN_NBIOT_EXPECT_MORE_SF;
    }
  }

  // If no grant is present, try to decode NPDCCH
  float noise_est = isrran_chest_dl_nbiot_get_noise_estimate(&q->chest);
  if (isrran_npdcch_extract_llr(&q->npdcch, q->sf_symbols, q->ce, noise_est, sf_idx)) {
    fprintf(stderr, "Error extracting LLRs\n");
    return ISRRAN_ERROR;
  }

  DEBUG("%d.%d: Looking for DCI for RNTI=0x%x.", sfn, sf_idx, rnti);
  if (isrran_nbiot_ue_dl_find_dl_dci(q, sf_idx, rnti, dci_msg) == 1) {
    // a DCI was found
    INFO("%d.%d: Found DCI for RNTI=0x%x.", sfn, sf_idx, rnti);
    q->nof_detected++;
    ret = ISRRAN_NBIOT_UE_DL_FOUND_DCI;
  }
#if DUMP_SIGNALS
  isrran_nbiot_ue_dl_save_signal(q, input, sfn, sf_idx);
#endif

  return ret;
}

// This funtion searches for UL DCI's, i.e., Format N0, for a given RNTI
// assuming that the equalized subframe symbols are already stored in the DL object,
// i.e. isrran_nbiot_ue_dl_decode_npdcch() needs to be called before
int isrran_nbiot_ue_dl_find_ul_dci(isrran_nbiot_ue_dl_t* q, uint32_t tti, uint32_t rnti, isrran_dci_msg_t* dci_msg)
{
  DEBUG("%d.%d: Looking for UL DCI for RNTI=0x%x.", tti / 10, tti % 10, rnti);

  // Search UE-specific search space
  dci_blind_search_t search_space;
  search_space.nof_locations = isrran_npdcch_ue_locations(search_space.loc, ISRRAN_MAX_CANDIDATES_UE);
  DEBUG("x.%d: Searching UL C-RNTI=0x%x in %d locations, %d formats",
        tti % 10,
        rnti,
        search_space.nof_locations,
        nof_nb_ue_formats);
  for (int f = 0; f < nof_nb_ue_formats; f++) {
    search_space.format = nb_ue_formats[f];
    if ((dci_blind_search(q, &search_space, rnti, dci_msg)) == 1) {
      q->nof_detected++;
      return ISRRAN_NBIOT_UE_DL_FOUND_DCI;
    }
  }
  return ISRRAN_ERROR;
}

void isrran_nbiot_ue_dl_save_signal(isrran_nbiot_ue_dl_t* q, cf_t* input, uint32_t sfn, uint32_t sf_idx)
{
  uint32_t num_symbols = ISRRAN_SF_LEN(ISRRAN_NBIOT_FFT_SIZE);
#define MAX_FNAME_LEN 50
  char fname[MAX_FNAME_LEN];

  // RAW samples
  snprintf(fname, MAX_FNAME_LEN, "nb_ue_dl_sfn%d_sf%d_raw_samples.bin", sfn, sf_idx);
  DEBUG("SAVED FILE %s: rx'ed samples", fname);
  isrran_vec_save_file(fname, input, num_symbols * sizeof(cf_t));

  // NPDCCH
  num_symbols = q->npdcch.num_decoded_symbols;
  snprintf(fname, MAX_FNAME_LEN, "nb_ue_dl_sfn%d_sf%d_npdcch_symbols.bin", sfn, sf_idx);
  DEBUG("SAVED FILE %s: rx'ed downlink symbols", fname);
  isrran_vec_save_file(fname, q->npdcch.symbols[0], num_symbols * sizeof(cf_t));

  snprintf(fname, MAX_FNAME_LEN, "nb_ue_dl_sfn%d_sf%d_npdcch_symbols_eq.bin", sfn, sf_idx);
  DEBUG("SAVED FILE %s: eq rx'ed downlink symbols", fname);
  isrran_vec_save_file(fname, q->npdcch.d, num_symbols * sizeof(cf_t));

  // NPDSCH
  num_symbols = q->npdsch_cfg.nbits.nof_re * q->npdsch_cfg.grant.nof_sf;
  snprintf(fname, MAX_FNAME_LEN, "nb_ue_dl_sfn%d_sf%d_npdsch_symbols.bin", sfn, sf_idx);
  DEBUG("SAVED FILE %s: rx'ed downlink symbols", fname);
  isrran_vec_save_file(fname, q->npdsch.symbols[0], num_symbols * sizeof(cf_t));

  snprintf(fname, MAX_FNAME_LEN, "nb_ue_dl_sfn%d_sf%d_npdsch_symbols_eq.bin", sfn, sf_idx);
  DEBUG("SAVED FILE %s: eq rx'ed downlink symbols", fname);
  isrran_vec_save_file(fname, q->npdsch.d, num_symbols * sizeof(cf_t));

  // CE
  snprintf(fname, MAX_FNAME_LEN, "nb_ue_dl_sfn%d_sf%d_ce0.bin", sfn, sf_idx);
  DEBUG("SAVED FILE %s: downlink channel estimates port 0", fname);
  isrran_vec_save_file(fname, q->ce_buffer[0], num_symbols * sizeof(cf_t));

  if (q->cell.nof_ports > 1) {
    snprintf(fname, MAX_FNAME_LEN, "nb_ue_dl_sfn%d_sf%d_ce1.bin", sfn, sf_idx);
    DEBUG("SAVED FILE %s: downlink channel estimates port 1", fname);
    isrran_vec_save_file(fname, q->ce_buffer[1], num_symbols * sizeof(cf_t));
  }
}
