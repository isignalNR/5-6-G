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

#ifndef ISRRAN_SCHED_COMMON_TEST_SUITE_H
#define ISRRAN_SCHED_COMMON_TEST_SUITE_H

#include "isrenb/hdr/stack/mac/sched_interface.h"
#include "isrenb/hdr/stack/mac/sched_lte_common.h"
#include "isrenb/hdr/stack/mac/sched_phy_ch/sched_phy_resource.h"
#include "isrran/adt/span.h"
#include "isrran/common/tti_point.h"

namespace isrenb {

struct sf_output_res_t {
  isrran::span<const sched_cell_params_t>             cc_params;
  isrran::tti_point                                   tti_rx;
  isrran::span<const sched_interface::ul_sched_res_t> ul_cc_result;
  isrran::span<const sched_interface::dl_sched_res_t> dl_cc_result;
};

/**
 * verifies correctness of the output of PUSCH allocations for a subframe. Current checks:
 * - individual ul allocs are within (0, n_prb) bounds
 * - individual ul allocs occupy at least one prb
 * - no collisions between individual ul allocs
 * - space is left for PRACH and PUCCH
 * - if expected_ul_mask arg is not null, assert it matches the cumulative mask of all PUSCH allocations
 * @param sf_out result of subframe allocation and associated metadata
 * @param expected_ul_mask optional arg for expected cumulative UL PRB mask
 * @return error code
 */
int test_pusch_collisions(const sf_output_res_t& sf_out, uint32_t enb_cc_idx, const prbmask_t* expected_ul_mask);

/**
 * verifies correctness of the output of PDSCH allocations for a subframe. Current checks:
 * - individual DL allocs are within (0, n_prb) bounds
 * - individual DL allocs occupy at least one RBG
 * - no collisions between individual DL allocs
 * - DL data alloc ACKs do not collide with PRACH in UL
 * - if expected_rbgmask arg is not null, assert it matches the cumulative mask of all PDSCH allocations
 * @param sf_out result of subframe allocation and associated metadata
 * @param expected_rbgmask optional arg for expected cumulative DL RBG mask
 * @return error code
 */
int test_pdsch_collisions(const sf_output_res_t& sf_out, uint32_t enb_cc_idx, const rbgmask_t* expected_rbgmask);

/**
 * verifies correctness of SIB allocations. Current checks:
 * - SIB1 is allocated in correct TTIs
 * - TB size is adequate for SIB allocation
 * - The SIBs with index>1 are allocated in expected TTI windows
 * @param sf_out result of subframe allocation and associated metadata
 * @return error code
 */
int test_sib_scheduling(const sf_output_res_t& sf_out, uint32_t enb_cc_idx);

/**
 * verifies correctness of PDCCH allocations for DL, RAR, UL, Broadcast. Current checks:
 * - DCI CCE positions fit in available PDCCH resources
 * - no collisions between individual DCI CCE positions
 * @param sf_out
 * @param used_cce
 * @return error code
 */
int test_pdcch_collisions(const sf_output_res_t&                   sf_out,
                          uint32_t                                 enb_cc_idx,
                          const isrran::bounded_bitset<128, true>* expected_cce_mask);

/**
 * verifies correctness of DCI content for params that are independent of the UE configuration.
 * - TB size is large enough
 * - No repeated rntis in PDSCH and PUSCH
 * @param sf_out
 * @return error code
 */
int test_dci_content_common(const sf_output_res_t& sf_out, uint32_t enb_cc_idx);

/**
 * Call all available eNB common tests
 * @return error code
 */
int test_all_common(const sf_output_res_t& sf_out);

/// Helper function to get DL PRBs from DCI
int extract_dl_prbmask(const isrran_cell_t&               cell,
                       const isrran_dci_dl_t&             dci,
                       isrran::bounded_bitset<100, true>& alloc_mask);

} // namespace isrenb

#endif // ISRRAN_SCHED_COMMON_TEST_SUITE_H
