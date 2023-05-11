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

#include "phy_controller.h"
#include "isrran/interfaces/ue_nas_interfaces.h"
#include "isrran/isrlog/isrlog.h"
#include "isrue/hdr/stack/rrc/rrc.h"
#include <map>
#include <string>

#ifndef ISRRAN_RRC_PROCEDURES_H
#define ISRRAN_RRC_PROCEDURES_H

namespace isrue {

/********************************
 *         Procedures
 *******************************/

class rrc::cell_search_proc
{
public:
  enum class state_t { phy_cell_search, si_acquire, wait_measurement, phy_cell_select };

  explicit cell_search_proc(rrc* parent_);
  isrran::proc_outcome_t init();
  isrran::proc_outcome_t step();
  isrran::proc_outcome_t step_si_acquire();
  isrran::proc_outcome_t react(const phy_controller::cell_srch_res& event);
  isrran::proc_outcome_t react(const bool& event);
  isrran::proc_outcome_t step_wait_measurement();

  rrc_interface_phy_lte::cell_search_ret_t get_result() const { return search_result.cs_ret; }
  static const char*                       name() { return "Cell Search"; }

private:
  isrran::proc_outcome_t handle_cell_found(const phy_cell_t& new_cell);

  // conts
  rrc* rrc_ptr;

  // state vars
  phy_controller::cell_srch_res search_result;
  isrran::proc_future_t<void>   si_acquire_fut;
  state_t                       state;
};

/****************************************************************
 * TS 36.331 Sec 5.2.3 - Acquisition of an SI message procedure
 ***************************************************************/
class rrc::si_acquire_proc
{
public:
  const static int SIB_SEARCH_TIMEOUT_MS = 1000;
  struct si_acq_timer_expired {
    uint32_t timer_id;
  };
  struct sib_received_ev {};

  explicit si_acquire_proc(rrc* parent_);
  isrran::proc_outcome_t init(uint32_t sib_index_);
  isrran::proc_outcome_t step() { return isrran::proc_outcome_t::yield; }
  static const char*     name() { return "SI Acquire"; }
  isrran::proc_outcome_t react(si_acq_timer_expired ev);
  isrran::proc_outcome_t react(sib_received_ev ev);
  void                   then(const isrran::proc_state_t& result);

private:
  void start_si_acquire();

  // conts
  rrc*                  rrc_ptr;
  isrlog::basic_logger& logger;

  // state
  isrran::timer_handler::unique_timer si_acq_timeout, si_acq_retry_timer;
  uint32_t                            period = 0, sched_index = 0;
  uint32_t                            sib_index = 0;
};

class rrc::serving_cell_config_proc
{
public:
  explicit serving_cell_config_proc(rrc* parent_);
  isrran::proc_outcome_t init(const std::vector<uint32_t>& required_sibs_);
  isrran::proc_outcome_t step();
  static const char*     name() { return "Serving Cell Configuration"; }

private:
  rrc*                  rrc_ptr;
  isrlog::basic_logger& logger;

  isrran::proc_outcome_t launch_sib_acquire();

  // proc args
  std::vector<uint32_t> required_sibs;

  // state variables
  uint32_t                    req_idx = 0;
  isrran::proc_future_t<void> si_acquire_fut;
};

class rrc::cell_selection_proc
{
public:
  using cell_selection_complete_ev = isrran::proc_result_t<cs_result_t>;

  explicit cell_selection_proc(rrc* parent_);
  isrran::proc_outcome_t init(std::vector<uint32_t> required_sibs_ = {});
  isrran::proc_outcome_t step();
  cs_result_t            get_result() const { return cs_result; }
  static const char*     name() { return "Cell Selection"; }
  isrran::proc_outcome_t react(const bool& event);
  void                   then(const isrran::proc_result_t<cs_result_t>& proc_result) const;

private:
  isrran::proc_outcome_t start_next_cell_selection();
  isrran::proc_outcome_t step_cell_search();
  isrran::proc_outcome_t step_cell_config();
  bool                   is_serv_cell_suitable() const;
  bool                   is_sib_acq_required() const;
  isrran::proc_outcome_t set_proc_complete();
  isrran::proc_outcome_t start_phy_cell_selection(const meas_cell_eutra& cell);
  isrran::proc_outcome_t start_sib_acquisition();

  // consts
  rrc*                             rrc_ptr;
  meas_cell_list<meas_cell_eutra>* meas_cells;

  // state variables
  enum class search_state_t { cell_selection, serv_cell_camp, cell_config, cell_search };
  cs_result_t                                                     cs_result;
  search_state_t                                                  state;
  uint32_t                                                        neigh_index;
  bool                                                            serv_cell_select_attempted = false;
  isrran::proc_future_t<rrc_interface_phy_lte::cell_search_ret_t> cell_search_fut;
  isrran::proc_future_t<void>                                     serv_cell_cfg_fut;
  bool                                                            discard_serving = false, cell_search_called = false;
  std::vector<uint32_t>                                           required_sibs = {};
  phy_cell_t                                                      init_serv_cell;
};

class rrc::plmn_search_proc
{
public:
  explicit plmn_search_proc(rrc* parent_);
  isrran::proc_outcome_t init();
  isrran::proc_outcome_t step();
  void                   then(const isrran::proc_state_t& result) const;
  static const char*     name() { return "PLMN Search"; }

private:
  // consts
  rrc*                  rrc_ptr;
  isrlog::basic_logger& logger;

  // state variables
  nas_interface_rrc::found_plmn_t                                 found_plmns[nas_interface_rrc::MAX_FOUND_PLMNS];
  int                                                             nof_plmns = 0;
  isrran::proc_future_t<rrc_interface_phy_lte::cell_search_ret_t> cell_search_fut;
};

class rrc::connection_request_proc
{
public:
  explicit connection_request_proc(rrc* parent_);
  isrran::proc_outcome_t init(isrran::establishment_cause_t cause_, isrran::unique_byte_buffer_t dedicated_info_nas_);
  isrran::proc_outcome_t step();
  void                   then(const isrran::proc_state_t& result);
  isrran::proc_outcome_t react(const cell_selection_proc::cell_selection_complete_ev& e);
  static const char*     name() { return "Connection Request"; }

private:
  // const
  rrc*                  rrc_ptr;
  isrlog::basic_logger& logger;
  // args
  isrran::establishment_cause_t cause;
  isrran::unique_byte_buffer_t  dedicated_info_nas;

  // state variables
  enum class state_t { cell_selection, config_serving_cell, wait_t300 } state;
  cs_result_t                 cs_ret;
  isrran::proc_future_t<void> serv_cfg_fut;
};

class rrc::connection_setup_proc
{
public:
  explicit connection_setup_proc(rrc* parent_);
  isrran::proc_outcome_t init(const asn1::rrc::rr_cfg_ded_s* cnfg_, isrran::unique_byte_buffer_t dedicated_info_nas_);
  isrran::proc_outcome_t step() { return isrran::proc_outcome_t::yield; }
  void                   then(const isrran::proc_state_t& result);
  isrran::proc_outcome_t react(const bool& config_complete);
  static const char*     name() { return "Connection Setup"; }

private:
  // const
  rrc*                  rrc_ptr;
  isrlog::basic_logger& logger;
  // args
  isrran::unique_byte_buffer_t dedicated_info_nas;
};

class rrc::connection_reconf_no_ho_proc
{
public:
  explicit connection_reconf_no_ho_proc(rrc* parent_);
  isrran::proc_outcome_t init(const asn1::rrc::rrc_conn_recfg_s& recfg_);
  isrran::proc_outcome_t step() { return isrran::proc_outcome_t::yield; }
  static const char*     name() { return "Connection Reconfiguration"; }
  isrran::proc_outcome_t react(const bool& config_complete);
  void                   then(const isrran::proc_state_t& result);

private:
  // const
  rrc* rrc_ptr;
  // args
  bool                               has_5g_nr_reconfig = false;
  asn1::rrc::rrc_conn_recfg_r8_ies_s rx_recfg;
};

class rrc::process_pcch_proc
{
public:
  struct paging_complete {
    bool outcome;
  };

  explicit process_pcch_proc(rrc* parent_);
  isrran::proc_outcome_t init(const asn1::rrc::paging_s& paging_);
  isrran::proc_outcome_t step();
  isrran::proc_outcome_t react(paging_complete e);
  static const char*     name() { return "Process PCCH"; }

private:
  // args
  rrc*                  rrc_ptr;
  isrlog::basic_logger& logger;
  asn1::rrc::paging_s   paging;

  // vars
  uint32_t paging_idx = 0;
  enum class state_t { next_record, nas_paging, serv_cell_cfg } state;
  isrran::proc_future_t<void> serv_cfg_fut;
};

class rrc::go_idle_proc
{
public:
  explicit go_idle_proc(rrc* rrc_);
  isrran::proc_outcome_t init();
  isrran::proc_outcome_t step();
  isrran::proc_outcome_t react(bool timeout);
  static const char*     name() { return "Go Idle"; }
  void                   then(const isrran::proc_state_t& result);

private:
  static const uint32_t rlc_flush_timeout_ms = 60; // TS 36.331 Sec 5.3.8.3

  rrc*                                rrc_ptr;
  isrran::timer_handler::unique_timer rlc_flush_timer;
};

class rrc::cell_reselection_proc
{
public:
  /// Timer duration to restart Cell Reselection Procedure
  const static uint32_t cell_reselection_periodicity_ms = 20, cell_reselection_periodicity_long_ms = 1000;

  cell_reselection_proc(rrc* rrc_);
  isrran::proc_outcome_t init();
  isrran::proc_outcome_t step();
  static const char*     name() { return "Cell Reselection"; }
  void                   then(const isrran::proc_state_t& result);

private:
  rrc* rrc_ptr;

  isrran::timer_handler::unique_timer reselection_timer;
  isrran::proc_future_t<cs_result_t>  cell_selection_fut;
  cs_result_t                         cell_sel_result = cs_result_t::no_cell;
};

class rrc::connection_reest_proc
{
public:
  struct t311_expiry {};
  struct serv_cell_cfg_completed {};
  struct t301_expiry {};

  explicit connection_reest_proc(rrc* rrc_);
  // 5.3.7.2 - Initiation
  isrran::proc_outcome_t init(asn1::rrc::reest_cause_e cause);
  // 5.3.7.3 Actions following cell selection while T311 is running
  // && 5.3.7.4 - Actions related to transmission of RRCConnectionReestablishmentRequest message
  isrran::proc_outcome_t react(const cell_selection_proc::cell_selection_complete_ev& e);
  // 5.3.7.5 - Reception of the RRCConnectionReestablishment by the UE
  isrran::proc_outcome_t react(const asn1::rrc::rrc_conn_reest_s& reest_msg);
  // 5.3.7.6 - T311 expiry
  isrran::proc_outcome_t react(const t311_expiry& ev);
  // 5.3.7.7 - T301 expiry or selected cell no longer suitable
  isrran::proc_outcome_t react(const t301_expiry& ev);
  // detects if cell is no longer suitable (part of 5.3.7.7)
  isrran::proc_outcome_t step();
  // 5.3.7.8 - Reception of RRCConnectionReestablishmentReject by the UE
  isrran::proc_outcome_t react(const asn1::rrc::rrc_conn_reest_reject_s& reest_msg);
  static const char*     name() { return "Connection re-establishment"; }
  uint32_t               get_source_earfcn() const { return reest_source_freq; }

private:
  enum class state_t { wait_cell_selection, wait_reest_msg } state;

  rrc*                     rrc_ptr     = nullptr;
  asn1::rrc::reest_cause_e reest_cause = asn1::rrc::reest_cause_e::nulltype;
  uint16_t                 reest_rnti = 0, reest_source_pci = 0;
  uint32_t                 reest_source_freq = 0, reest_cellid = 0;

  bool                   passes_cell_criteria() const;
  isrran::proc_outcome_t cell_criteria();
  isrran::proc_outcome_t start_cell_selection();
};

class rrc::ho_proc
{
public:
  struct t304_expiry {};
  struct ra_completed_ev {
    bool success;
  };

  explicit ho_proc(rrc* rrc_);
  isrran::proc_outcome_t init(const asn1::rrc::rrc_conn_recfg_s& rrc_reconf);
  isrran::proc_outcome_t react(const bool& ev);
  isrran::proc_outcome_t react(t304_expiry ev);
  isrran::proc_outcome_t react(ra_completed_ev ev);
  isrran::proc_outcome_t step() { return isrran::proc_outcome_t::yield; }
  void                   then(const isrran::proc_state_t& result);
  static const char*     name() { return "Handover"; }

  phy_cell_t ho_src_cell;
  uint16_t   ho_src_rnti = 0;

private:
  rrc* rrc_ptr = nullptr;

  // args
  asn1::rrc::rrc_conn_recfg_r8_ies_s recfg_r8;

  // state
  phy_cell_t target_cell;

  // helper to revert security config of source cell
  void reset_security_config();
};

} // namespace isrue

#endif // ISRRAN_RRC_PROCEDURES_H
