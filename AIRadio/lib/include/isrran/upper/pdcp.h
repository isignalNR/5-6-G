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

#ifndef ISRRAN_PDCP_H
#define ISRRAN_PDCP_H

#include "isrran/common/common.h"
#include "isrran/common/task_scheduler.h"
#include "isrran/interfaces/ue_pdcp_interfaces.h"
#include "isrran/upper/pdcp_entity_lte.h"
#include <set>

namespace isrran {

class pdcp : public isrue::pdcp_interface_rlc, public isrue::pdcp_interface_rrc
{
public:
  pdcp(isrran::task_sched_handle task_sched_, const char* logname);
  virtual ~pdcp();
  void init(isrue::rlc_interface_pdcp* rlc_, isrue::rrc_interface_pdcp* rrc_, isrue::gw_interface_pdcp* gw_);
  void stop();

  // Stack interface
  bool is_lcid_enabled(uint32_t lcid);

  // RRC interface
  void reestablish() override;
  void reestablish(uint32_t lcid) override;
  void reset() override;
  void set_enabled(uint32_t lcid, bool enabled) override;
  void write_sdu(uint32_t lcid, unique_byte_buffer_t sdu, int sn = -1) override;
  void write_sdu_mch(uint32_t lcid, unique_byte_buffer_t sdu);
  int  add_bearer(uint32_t lcid, const pdcp_config_t& cnfg) override;
  void add_bearer_mrb(uint32_t lcid, const pdcp_config_t& cnfg);
  void del_bearer(uint32_t lcid) override;
  void change_lcid(uint32_t old_lcid, uint32_t new_lcid) override;
  void config_security(uint32_t lcid, const as_security_config_t& sec_cfg) override;
  void config_security_all(const as_security_config_t& sec_cfg) override;
  void enable_integrity(uint32_t lcid, isrran_direction_t direction) override;
  void enable_encryption(uint32_t lcid, isrran_direction_t direction) override;
  void enable_security_timed(uint32_t lcid, isrran_direction_t direction, uint32_t sn);
  bool get_bearer_state(uint32_t lcid, isrran::pdcp_lte_state_t* state);
  bool set_bearer_state(uint32_t lcid, const isrran::pdcp_lte_state_t& state);
  void send_status_report() override;
  void send_status_report(uint32_t lcid) override;

  // RLC interface
  void write_pdu(uint32_t lcid, unique_byte_buffer_t sdu) override;
  void write_pdu_mch(uint32_t lcid, unique_byte_buffer_t sdu) override;
  void write_pdu_bcch_bch(unique_byte_buffer_t sdu) override;
  void write_pdu_bcch_dlsch(unique_byte_buffer_t sdu) override;
  void write_pdu_pcch(unique_byte_buffer_t sdu) override;
  void notify_delivery(uint32_t lcid, const pdcp_sn_vector_t& pdcp_sns) override;
  void notify_failure(uint32_t lcid, const pdcp_sn_vector_t& pdcp_sns) override;

  // eNB-only methods
  std::map<uint32_t, isrran::unique_byte_buffer_t> get_buffered_pdus(uint32_t lcid);

  // Metrics
  void get_metrics(pdcp_metrics_t& m, const uint32_t nof_tti);
  void reset_metrics();

private:
  isrue::rlc_interface_pdcp* rlc    = nullptr;
  isrue::rrc_interface_pdcp* rrc    = nullptr;
  isrue::gw_interface_pdcp*  gw     = nullptr;
  isrran::task_sched_handle  task_sched;
  isrlog::basic_logger&      logger;

  using pdcp_map_t = std::map<uint16_t, std::unique_ptr<pdcp_entity_base> >;
  pdcp_map_t pdcp_array, pdcp_array_mrb;

  // cache valid lcids to be checked from separate thread
  std::mutex         cache_mutex;
  std::set<uint32_t> valid_lcids_cached;

  bool valid_lcid(uint32_t lcid);
  bool valid_mch_lcid(uint32_t lcid);

  // Timer needed for metrics calculation
  std::chrono::high_resolution_clock::time_point metrics_tp;
};

} // namespace isrran

#endif // ISRRAN_PDCP_H
