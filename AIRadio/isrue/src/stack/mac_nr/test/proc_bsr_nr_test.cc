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
#include "isrran/common/buffer_pool.h"
#include "isrran/common/common.h"
#include "isrran/common/test_common.h"
#include "isrue/hdr/stack/mac_nr/proc_bsr_nr.h"

using namespace isrue;

int sbsr_tests()
{
  auto& mac_logger = isrlog::fetch_basic_logger("MAC");
  mac_logger.set_level(isrlog::basic_levels::debug);
  mac_logger.set_hex_dump_max_size(-1);

  isrran::task_scheduler        task_sched{5, 2};
  isrran::ext_task_sched_handle ext_task_sched_h(&task_sched);

  proc_bsr_nr proc(mac_logger);
  proc.init(nullptr, nullptr, nullptr, &ext_task_sched_h);

  isrran::bsr_cfg_nr_t bsr_cfg = {};
  bsr_cfg.periodic_timer       = 20;
  bsr_cfg.retx_timer           = 320;
  TESTASSERT(proc.set_config(bsr_cfg) == ISRRAN_SUCCESS);

  uint32_t            tti = 0;
  mac_buffer_states_t buffer_state;
  buffer_state.nof_lcgs_with_data = 1;
  buffer_state.last_non_zero_lcg  = 1;
  buffer_state.lcg_buffer_size[1] = 10;
  proc.step(tti++, buffer_state);

  // Buffer size == 10 should result in index 1 (<= 10)
  isrran::mac_sch_subpdu_nr::lcg_bsr_t sbsr = proc.generate_sbsr();
  TESTASSERT(sbsr.lcg_id == 1);
  TESTASSERT(sbsr.buffer_size == 1);

  buffer_state.last_non_zero_lcg  = 1;
  buffer_state.lcg_buffer_size[1] = 11;
  proc.step(tti++, buffer_state);

  // Buffer size == 11 should result in index 1
  sbsr = proc.generate_sbsr();
  TESTASSERT(sbsr.lcg_id == 1);
  TESTASSERT(sbsr.buffer_size == 2);

  buffer_state.last_non_zero_lcg  = 1;
  buffer_state.lcg_buffer_size[1] = 77285; // 77284 + 1
  proc.step(tti++, buffer_state);

  // Buffer size 77285 should result in index 29 (first value of that index)
  sbsr = proc.generate_sbsr();
  TESTASSERT(sbsr.lcg_id == 1);
  TESTASSERT(sbsr.buffer_size == 29);

  buffer_state.last_non_zero_lcg  = 1;
  buffer_state.lcg_buffer_size[1] = 150000;
  proc.step(tti++, buffer_state);

  // Buffer size 150000 should result in index 30
  sbsr = proc.generate_sbsr();
  TESTASSERT(sbsr.lcg_id == 1);
  TESTASSERT(sbsr.buffer_size == 30);

  buffer_state.last_non_zero_lcg  = 1;
  buffer_state.lcg_buffer_size[1] = 150001;
  proc.step(tti++, buffer_state);

  // Buffer size 150001 should result in index 31
  sbsr = proc.generate_sbsr();
  TESTASSERT(sbsr.lcg_id == 1);
  TESTASSERT(sbsr.buffer_size == 31);

  return ISRRAN_SUCCESS;
}

int main()
{
  isrlog::init();

  TESTASSERT(sbsr_tests() == ISRRAN_SUCCESS);

  return ISRRAN_SUCCESS;
}
