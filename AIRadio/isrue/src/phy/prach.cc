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

#include "isrue/hdr/phy/prach.h"
#include "isrran/common/standard_streams.h"
#include "isrran/interfaces/phy_interface_types.h"
#include "isrran/isrran.h"

#define Error(fmt, ...)                                                                                                \
  if (ISRRAN_DEBUG_ENABLED)                                                                                            \
  logger.error(fmt, ##__VA_ARGS__)
#define Warning(fmt, ...)                                                                                              \
  if (ISRRAN_DEBUG_ENABLED)                                                                                            \
  logger.warning(fmt, ##__VA_ARGS__)
#define Info(fmt, ...)                                                                                                 \
  if (ISRRAN_DEBUG_ENABLED)                                                                                            \
  logger.info(fmt, ##__VA_ARGS__)
#define Debug(fmt, ...)                                                                                                \
  if (ISRRAN_DEBUG_ENABLED)                                                                                            \
  logger.debug(fmt, ##__VA_ARGS__)

using namespace isrue;

void prach::init(uint32_t max_prb)
{
  std::lock_guard<std::mutex> lock(mutex);

  if (isrran_cfo_init(&cfo_h, ISRRAN_PRACH_MAX_LEN)) {
    ERROR("PRACH: Error initiating CFO");
    return;
  }

  isrran_cfo_set_tol(&cfo_h, 0);

  signal_buffer = isrran_vec_cf_malloc(ISRRAN_MAX(MAX_LEN_SF * 30720U, ISRRAN_PRACH_MAX_LEN));
  if (!signal_buffer) {
    perror("malloc");
    return;
  }

  if (isrran_prach_init(&prach_obj, isrran_symbol_sz(max_prb))) {
    Error("Initiating PRACH library");
    return;
  }

  mem_initiated = true;
}

void prach::stop()
{
  std::lock_guard<std::mutex> lock(mutex);
  if (!mem_initiated) {
    return;
  }

  free(signal_buffer);
  isrran_cfo_free(&cfo_h);
  isrran_prach_free(&prach_obj);
  mem_initiated = false;
}

bool prach::set_cell(isrran_cell_t cell_, isrran_prach_cfg_t prach_cfg)
{
  std::lock_guard<std::mutex> lock(mutex);

  if (!mem_initiated) {
    ERROR("PRACH: Error must call init() first");
    return false;
  }

  if (cell.id == cell_.id && cell_initiated && prach_cfg == cfg) {
    return true;
  }

  cell = cell_;
  cfg  = prach_cfg;
  // We must not reset preamble_idx here, MAC might have already called prepare_to_send()

  if (6 + prach_cfg.freq_offset > cell.nof_prb) {
    isrran::console("Error no space for PRACH: frequency offset=%d, N_rb_ul=%d\n", prach_cfg.freq_offset, cell.nof_prb);
    logger.error("Error no space for PRACH: frequency offset=%d, N_rb_ul=%d", prach_cfg.freq_offset, cell.nof_prb);
    return false;
  }

  Info("PRACH: cell.id=%d, configIdx=%d, rootSequence=%d, zeroCorrelationConfig=%d, freqOffset=%d",
       cell.id,
       prach_cfg.config_idx,
       prach_cfg.root_seq_idx,
       prach_cfg.zero_corr_zone,
       prach_cfg.freq_offset);

  if (isrran_prach_set_cfg(&prach_obj, &prach_cfg, cell.nof_prb)) {
    Error("Initiating PRACH library");
    return false;
  }

  len             = prach_obj.N_seq + prach_obj.N_cp;
  transmitted_tti = -1;
  cell_initiated  = true;

  logger.info("Finished setting new PRACH configuration.");

  return true;
}

bool prach::generate_buffer(uint32_t f_idx)
{
  uint32_t freq_offset = cfg.freq_offset;
  if (cell.frame_type == ISRRAN_TDD) {
    freq_offset = isrran_prach_f_ra_tdd(
        cfg.config_idx, cfg.tdd_config.sf_config, (f_idx / 6) * 10, f_idx % 6, cfg.freq_offset, cell.nof_prb);
  }
  if (isrran_prach_gen(&prach_obj, preamble_idx, freq_offset, signal_buffer)) {
    Error("Generating PRACH preamble %d", preamble_idx);
    return false;
  }

  return true;
}

bool prach::prepare_to_send(uint32_t preamble_idx_, int allowed_subframe_, float target_power_dbm_)
{
  std::lock_guard<std::mutex> lock(mutex);
  if (preamble_idx_ >= max_preambles) {
    Error("PRACH: Invalid preamble %d", preamble_idx_);
    return false;
  }

  preamble_idx     = preamble_idx_;
  target_power_dbm = target_power_dbm_;
  allowed_subframe = allowed_subframe_;
  transmitted_tti  = -1;
  Debug("PRACH: prepare to send preamble %d", preamble_idx);
  return true;
}

bool prach::is_pending() const
{
  std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
  if (lock.owns_lock()) {
    return cell_initiated && preamble_idx >= 0 && unsigned(preamble_idx) < max_preambles;
  }
  return false;
}

bool prach::is_ready_to_send(uint32_t current_tti_, uint32_t current_pci)
{
  // Make sure the curernt PCI is the one we configured the PRACH for
  if (is_pending() && current_pci == cell.id) {
    std::lock_guard<std::mutex> lock(mutex);
    // consider the number of subframes the transmission must be anticipated
    uint32_t tti_tx = TTI_TX(current_tti_);
    if (isrran_prach_tti_opportunity(&prach_obj, tti_tx, allowed_subframe)) {
      Debug("PRACH Buffer: Ready to send at tti: %d (now is %d)", tti_tx, current_tti_);
      transmitted_tti = tti_tx;
      return true;
    }
  }
  return false;
}

phy_interface_mac_lte::prach_info_t prach::get_info() const
{
  std::lock_guard<std::mutex>         lock(mutex);
  phy_interface_mac_lte::prach_info_t info = {};

  info.preamble_format = prach_obj.config_idx / 16;
  if (transmitted_tti >= 0) {
    info.tti_ra = (uint32_t)transmitted_tti;
    if (cell.frame_type == ISRRAN_TDD) {
      info.f_id =
          isrran_prach_f_id_tdd(prach_obj.config_idx, prach_obj.tdd_config.sf_config, prach_obj.current_prach_idx);
    }
    info.is_transmitted = true;
  } else {
    info.is_transmitted = false;
  }
  return info;
}

cf_t* prach::generate(float cfo, uint32_t* nof_sf, float* target_power)
{
  std::lock_guard<std::mutex> lock(mutex);
  if (!cell_initiated || preamble_idx < 0 || !nof_sf || unsigned(preamble_idx) >= max_preambles ||
      !isrran_cell_isvalid(&cell) || len >= MAX_LEN_SF * 30720 || len == 0) {
    Error("PRACH: Invalid parameters: cell_initiated=%d, preamble_idx=%d, cell.nof_prb=%d, len=%d",
          cell_initiated,
          preamble_idx,
          cell.nof_prb,
          len);
    return nullptr;
  }

  uint32_t f_idx = 0;
  if (cell.frame_type == ISRRAN_TDD) {
    f_idx = prach_obj.current_prach_idx;
    // For format4, choose odd or even position
    if (prach_obj.config_idx >= 48) {
      f_idx += 6;
    }
    if (f_idx >= max_fs) {
      Error("PRACH Buffer: Invalid f_idx=%d", f_idx);
      f_idx = 0;
    }
  }

  if (!generate_buffer(f_idx)) {
    return nullptr;
  }

  // Correct CFO before transmission
  isrran_cfo_correct(&cfo_h, signal_buffer, signal_buffer, cfo / isrran_symbol_sz(cell.nof_prb));

  // pad guard symbols with zeros
  uint32_t nsf = ISRRAN_CEIL(len, ISRRAN_SF_LEN_PRB(cell.nof_prb));
  isrran_vec_cf_zero(&signal_buffer[len], (nsf * ISRRAN_SF_LEN_PRB(cell.nof_prb) - len));

  *nof_sf = nsf;

  if (target_power) {
    *target_power = target_power_dbm;
  }

  Info("PRACH: Transmitted preamble=%d, tti_tx=%d, CFO=%.2f KHz, nof_sf=%d, target_power=%.1f dBm",
       preamble_idx,
       transmitted_tti,
       cfo * 15,
       nsf,
       target_power_dbm);
  preamble_idx = -1;

  return signal_buffer;
}
