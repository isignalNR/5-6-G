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

#ifndef ISRENB_MAC_NR_H
#define ISRENB_MAC_NR_H

#include "isrran/common/block_queue.h"
#include "isrran/common/mac_pcap.h"

#include "isrenb/hdr/common/rnti_pool.h"
#include "isrenb/hdr/stack/enb_stack_base.h"
#include "isrgnb/hdr/stack/mac/ue_nr.h"
#include "isrran/common/task_scheduler.h"
#include "isrran/interfaces/enb_metrics_interface.h"
#include "isrran/interfaces/enb_rlc_interfaces.h"
#include "isrran/interfaces/gnb_mac_interfaces.h"

namespace isrenb {

struct mac_nr_args_t {
  isrran::phy_cfg_nr_t             phy_base_cfg = {};
  int                              fixed_dl_mcs = -1;
  int                              fixed_ul_mcs = -1;
  sched_nr_interface::sched_args_t sched_cfg    = {};
  isrenb::pcap_args_t              pcap;
};

class sched_nr;
class mac_nr_rx;

class mac_nr final : public mac_interface_phy_nr,
                     public mac_interface_rrc_nr,
                     public mac_interface_rlc_nr,
                     public mac_interface_pdu_demux_nr
{
public:
  explicit mac_nr(isrran::task_sched_handle task_sched_);
  ~mac_nr();

  int  init(const mac_nr_args_t&    args_,
            phy_interface_stack_nr* phy,
            stack_interface_mac*    stack_,
            rlc_interface_mac*      rlc_,
            rrc_interface_mac_nr*   rrc_);
  void stop();

  /// Called from metrics thread.
  void get_metrics(isrenb::mac_metrics_t& metrics);

  // MAC interface for RRC
  int      cell_cfg(const std::vector<isrenb::sched_nr_cell_cfg_t>& nr_cells) override;
  uint16_t reserve_rnti(uint32_t enb_cc_idx, const sched_nr_interface::ue_cfg_t& uecfg) override;
  int      read_pdu_bcch_bch(uint8_t* payload);
  int      ue_cfg(uint16_t rnti, const sched_nr_interface::ue_cfg_t& ue_cfg) override;
  int      remove_ue(uint16_t rnti) override;

  // MAC interface for RLC
  int rlc_buffer_state(uint16_t rnti, uint32_t lcid, uint32_t tx_queue, uint32_t retx_queue) override;

  // Interface for PHY
  int         slot_indication(const isrran_slot_cfg_t& slot_cfg) override;
  dl_sched_t* get_dl_sched(const isrran_slot_cfg_t& slot_cfg) override;
  ul_sched_t* get_ul_sched(const isrran_slot_cfg_t& slot_cfg) override;
  int         pucch_info(const isrran_slot_cfg_t& slot_cfg, const pucch_info_t& pucch_info) override;
  int         pusch_info(const isrran_slot_cfg_t& slot_cfg, pusch_info_t& pusch_info) override;
  void        rach_detected(const rach_info_t& rach_info) override;

  // MAC-internal interface
  void store_msg3(uint16_t rnti, isrran::unique_byte_buffer_t pdu) override;

  // Test interface
  void ul_bsr(uint16_t rnti, uint32_t lcid, uint32_t bsr);

private:
  uint16_t add_ue_(uint32_t enb_cc_idx);
  uint16_t alloc_ue(uint32_t enb_cc_idx);

  // internal misc helpers
  bool is_rnti_valid_nolock(uint16_t rnti);
  bool is_rnti_active_nolock(uint16_t rnti);

  // handle UCI data from either PUCCH or PUSCH
  bool handle_uci_data(uint16_t rnti, const isrran_uci_cfg_nr_t& cfg, const isrran_uci_value_nr_t& value);

  // Metrics processing
  void get_metrics_nolock(isrenb::mac_metrics_t& metrics);

  // Encoding
  isrran::byte_buffer_t* assemble_rar(isrran::const_span<sched_nr_interface::msg3_grant_t> grants);

  isrran::unique_byte_buffer_t rar_pdu_buffer;

  // Interaction with other components
  phy_interface_stack_nr* phy   = nullptr;
  stack_interface_mac*    stack = nullptr;
  rlc_interface_mac*      rlc   = nullptr;
  rrc_interface_mac_nr*   rrc   = nullptr;

  // args
  isrran::task_sched_handle task_sched;
  isrran::task_queue_handle stack_task_queue;
  mac_nr_args_t             args = {};
  isrlog::basic_logger&     logger;

  // initial UE config, before RRC setup (without UE-dedicated)
  isrran::phy_cfg_nr_t default_ue_phy_cfg;

  std::unique_ptr<isrran::mac_pcap> pcap = nullptr;

  std::atomic<bool> started = {false};

  const static uint32_t             NUMEROLOGY_IDX = 0; /// only 15kHz supported at this stage
  std::unique_ptr<isrenb::sched_nr> sched;
  std::vector<sched_nr_cell_cfg_t>  cell_config;

  // Map of active UEs
  pthread_rwlock_t                                                              rwmutex    = {};
  static const uint16_t                                                         FIRST_RNTI = 0x4601;
  isrran::static_circular_map<uint16_t, std::unique_ptr<ue_nr>, ISRENB_MAX_UES> ue_db;

  std::atomic<uint16_t> ue_counter{0};

  // BCH buffers
  struct sib_info_t {
    uint32_t                     index;
    uint32_t                     periodicity;
    isrran::unique_byte_buffer_t payload;
  };
  std::vector<sib_info_t>      bcch_dlsch_payload;
  isrran::unique_byte_buffer_t bcch_bch_payload = nullptr;

  // Number of rach preambles detected for a CC
  std::vector<uint32_t> detected_rachs;

  // Decoding of UL PDUs
  std::unique_ptr<mac_nr_rx> rx;
};

} // namespace isrenb

#endif // ISRENB_MAC_NR_H
