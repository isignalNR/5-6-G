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
#ifndef ISREPC_S1AP_ERAB_MNGMT_PROC_H
#define ISREPC_S1AP_ERAB_MNGMT_PROC_H

#include "mme_gtpc.h"
#include "s1ap_common.h"
#include "isrran/asn1/s1ap.h"
#include "isrran/common/buffer_pool.h"
#include "isrran/common/common.h"
#include <netinet/sctp.h>

namespace isrepc {

class s1ap;

class s1ap_erab_mngmt_proc
{
public:
  static s1ap_erab_mngmt_proc* m_instance;
  static s1ap_erab_mngmt_proc* get_instance(void);
  static void                  cleanup(void);

  void init(void);

  bool send_erab_release_command(uint32_t               enb_ue_s1ap_id,
                                 uint32_t               mme_ue_s1ap_id,
                                 std::vector<uint16_t>  erabs_to_release,
                                 struct sctp_sndrcvinfo enb_sri);
  bool send_erab_modify_request(uint32_t                     enb_ue_s1ap_id,
                                uint32_t                     mme_ue_s1ap_id,
                                std::map<uint16_t, uint16_t> erabs_to_modify,
                                isrran::byte_buffer_t*       nas_msg,
                                struct sctp_sndrcvinfo       enb_sri);
  bool handle_erab_release_response(const asn1::s1ap::init_context_setup_resp_s& in_ctxt_resp);

private:
  s1ap_erab_mngmt_proc();
  virtual ~s1ap_erab_mngmt_proc();

  s1ap*                 m_s1ap   = nullptr;
  isrlog::basic_logger& m_logger = isrlog::fetch_basic_logger("S1AP");

  s1ap_args_t m_s1ap_args;

  mme_gtpc* m_mme_gtpc = nullptr;
};

} // namespace isrepc
#endif // ISREPC_S1AP_CTX_MNGMT_PROC_H
