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

#ifndef ISRRAN_SSS_NR_H
#define ISRRAN_SSS_NR_H

#include "isrran/config.h"
#include "isrran/phy/common/phy_common_nr.h"
#include <inttypes.h>

/**
 * @brief NR SSS sequence length in frequency domain
 */
#define ISRRAN_SSS_NR_LEN 127

/**
 * @brief NR SSS Symbol number
 */
#define ISRRAN_SSS_NR_SYMBOL_IDX 2

/**
 * @brief Put NR SSS sequence in the SSB grid
 * @remark Described in TS 38.211 section 7.4.2.3 Secondary synchronization signal
 * @param ssb_grid SSB resource grid
 * @param N_id_1 Physical cell ID 1
 * @param N_id_2 Physical cell ID 2
 * @param beta SSS power allocation
 * @return ISRRAN_SUCCESS if the parameters are valid, ISRRAN_ERROR code otherwise
 */
ISRRAN_API int isrran_sss_nr_put(cf_t ssb_grid[ISRRAN_SSB_NOF_RE], uint32_t N_id_1, uint32_t N_id_2, float beta);

/**
 * @brief Extracts the NR SSS Least Square Estimates (LSE) from the SSB grid
 * @param ssb_grid received SSB resource grid
 * @param N_id_1 Physical cell ID 1
 * @param N_id_2 Physical cell ID 2
 * @param lse Provides LSE pointer
 * @return ISRRAN_SUCCESS if the parameters are valid, ISRRAN_ERROR code otherwise
 */
ISRRAN_API int
isrran_sss_nr_extract_lse(const cf_t* ssb_grid, uint32_t N_id_1, uint32_t N_id_2, cf_t lse[ISRRAN_SSS_NR_LEN]);

/**
 * @brief Find the best SSS sequence given the N_id_2 and the SSB resource grid
 * @attention Assumes the SSB is synchronized and the average delay is pre-compensated
 * @param ssb_grid The SSB resource grid to search
 * @param N_id_2 Fix N_id_2 to search, it reduces the search space 1/3
 * @param norm_corr Normalised correlation of the best found sequence
 * @param found_N_id_1 The N_id_1 of the best sequence
 * @return ISRRAN_SUCCESS if the parameters are valid, ISRRAN_ERROR code otherwise
 */
ISRRAN_API int
isrran_sss_nr_find(const cf_t ssb_grid[ISRRAN_SSB_NOF_RE], uint32_t N_id_2, float* norm_corr, uint32_t* found_N_id_1);

#endif // ISRRAN_SSS_NR_H
