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

#ifndef ISRUE_DEMUX_H
#define ISRUE_DEMUX_H

#include "isrran/common/timers.h"
#include "isrran/interfaces/ue_mac_interfaces.h"
#include "isrran/interfaces/ue_rlc_interfaces.h"
#include "isrran/mac/pdu.h"
#include "isrran/mac/pdu_queue.h"
#include "isrran/isrlog/isrlog.h"

/* Logical Channel Demultiplexing and MAC CE dissassemble */

namespace isrue {

class rlc_interface_mac;
class phy_interface_mac_common;

class mac_interface_demux
{
public:
  virtual void reset_harq(uint32_t cc_idx)               = 0;
  virtual bool contention_resolution_id_rcv(uint64_t id) = 0;
};

class demux : public isrran::pdu_queue::process_callback
{
public:
  explicit demux(isrlog::basic_logger& logger);
  void init(phy_interface_mac_common*            phy_h_,
            rlc_interface_mac*                   rlc,
            mac_interface_demux*                 mac,
            isrran::timer_handler::unique_timer* time_alignment_timer);
  void reset();

  bool     process_pdus();
  uint8_t* request_buffer(uint32_t len);
  uint8_t* request_buffer_bcch(uint32_t len);
  void     deallocate(uint8_t* payload_buffer_ptr);

  void push_pdu(uint8_t* buff, uint32_t nof_bytes, uint32_t tti);
  void push_pdu_bcch(uint8_t* buff, uint32_t nof_bytes);
  void push_pdu_mch(uint8_t* buff, uint32_t nof_bytes);
  void push_pdu_temp_crnti(uint8_t* buff, uint32_t nof_bytes);

  bool get_uecrid_successful();

  void process_pdu(uint8_t* pdu, uint32_t nof_bytes, isrran::pdu_queue::channel_t channel, int ul_nof_prbs);
  void mch_start_rx(uint32_t lcid);

private:
  const static int MAX_PDU_LEN      = 150 * 1024 / 8; // ~ 150 Mbps
  const static int MAX_BCCH_PDU_LEN = 1024;
  uint8_t          bcch_buffer[MAX_BCCH_PDU_LEN]; // BCCH PID has a dedicated buffer

  // args
  isrlog::basic_logger&     logger;
  phy_interface_mac_common* phy_h = nullptr;
  rlc_interface_mac*        rlc   = nullptr;
  mac_interface_demux*      mac   = nullptr;

  isrran::sch_pdu mac_msg;
  isrran::mch_pdu mch_mac_msg;
  isrran::sch_pdu pending_mac_msg;
  uint8_t         mch_lcids[ISRRAN_N_MCH_LCIDS] = {};
  void            process_sch_pdu_rt(uint8_t* buff, uint32_t nof_bytes, uint32_t tti);
  void            process_sch_pdu(isrran::sch_pdu* pdu);
  void            process_mch_pdu(isrran::mch_pdu* pdu);
  bool            process_ce(isrran::sch_subh* subheader, uint32_t tti);
  void            parse_ta_cmd(isrran::sch_subh* subh, uint32_t tti);

  bool is_uecrid_successful = false;

  isrran::timer_handler::unique_timer* time_alignment_timer = nullptr;

  // Buffer of PDUs
  isrran::pdu_queue pdus;
};

} // namespace isrue

#endif // ISRUE_DEMUX_H
