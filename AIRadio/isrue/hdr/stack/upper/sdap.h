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

#ifndef ISRUE_SDAP_H
#define ISRUE_SDAP_H

#include "isrran/common/buffer_pool.h"
#include "isrran/common/common.h"
#include "isrran/common/common_nr.h"
#include "isrran/interfaces/ue_gw_interfaces.h"
#include "isrran/interfaces/ue_pdcp_interfaces.h"
#include "isrran/interfaces/ue_sdap_interfaces.h"

namespace isrue {

class sdap final : public sdap_interface_pdcp_nr, public sdap_interface_gw_nr, public sdap_interface_rrc
{
public:
  explicit sdap(const char* logname);
  bool init(pdcp_interface_sdap_nr* pdcp_, isrue::gw_interface_pdcp* gw_);
  void stop();

  // Interface for GW
  void write_pdu(uint32_t lcid, isrran::unique_byte_buffer_t pdu) final;

  // Interface for PDCP
  void write_sdu(uint32_t lcid, isrran::unique_byte_buffer_t pdu) final;

  // Interface for RRC
  bool set_bearer_cfg(uint32_t lcid, const sdap_interface_rrc::bearer_cfg_t& cfg) final;

private:
  pdcp_interface_sdap_nr* m_pdcp = nullptr;
  gw_interface_pdcp*      m_gw   = nullptr;

  // state
  bool running = false;

  // configuration
  std::array<sdap_interface_rrc::bearer_cfg_t, isrran::MAX_NR_NOF_BEARERS> bearers = {};

  isrlog::basic_logger& logger;
};

} // namespace isrue

#endif // ISRUE_SDAP_H
