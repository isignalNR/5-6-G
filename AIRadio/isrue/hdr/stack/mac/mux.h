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

#ifndef ISRUE_MUX_H
#define ISRUE_MUX_H

#include <pthread.h>

#include <vector>

#include "proc_bsr.h"
#include "proc_phr.h"
#include "isrran/common/common.h"
#include "isrran/interfaces/mac_interface_types.h"
#include "isrran/mac/pdu.h"
#include "isrran/isrlog/isrlog.h"
#include "isrue/hdr/stack/mac_common/mux_base.h"
#include <mutex>

namespace isrue {

class mux : private mux_base
{
public:
  explicit mux(isrlog::basic_logger& logger);
  ~mux(){};
  void reset();
  void init(rlc_interface_mac* rlc, bsr_interface_mux* bsr_procedure, phr_proc* phr_procedure_);

  void step();

  bool is_pending_any_sdu();
  bool is_pending_sdu(uint32_t lcid);

  uint8_t* pdu_get(isrran::byte_buffer_t* payload, uint32_t pdu_sz);
  uint8_t* msg3_get(isrran::byte_buffer_t* payload, uint32_t pdu_sz);

  void msg3_flush();
  bool msg3_is_transmitted();
  void msg3_prepare();
  bool msg3_is_pending();
  bool msg3_is_empty();

  void append_crnti_ce_next_tx(uint16_t crnti);

  void setup_lcid(const isrran::logical_channel_config_t& config);

  void print_logical_channel_state(const std::string& info);

private:
  uint8_t* pdu_get_nolock(isrran::byte_buffer_t* payload, uint32_t pdu_sz);
  bool     pdu_move_to_msg3(uint32_t pdu_sz);
  uint32_t allocate_sdu(uint32_t lcid, isrran::sch_pdu* pdu, int max_sdu_sz);
  bool     sched_sdu(isrran::logical_channel_config_t* ch, int* sdu_space, int max_sdu_sz);

  const static int MAX_NOF_SUBHEADERS = 20;

  // Mutex for exclusive access
  std::mutex mutex;

  isrlog::basic_logger& logger;
  rlc_interface_mac*    rlc              = nullptr;
  bsr_interface_mux*    bsr_procedure    = nullptr;
  phr_proc*             phr_procedure    = nullptr;
  uint16_t              pending_crnti_ce = 0;

  /* Msg3 Buffer */
  isrran::byte_buffer_t msg_buff;

  /* PDU Buffer */
  isrran::sch_pdu pdu_msg;

  isrran::byte_buffer_t msg3_buff;
  bool                  msg3_has_been_transmitted = false;
  bool                  msg3_pending              = false;
};

} // namespace isrue

#endif // ISRUE_MUX_H
