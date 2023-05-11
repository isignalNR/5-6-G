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

#ifndef ISRRAN_ZC_SEQUENCE_H
#define ISRRAN_ZC_SEQUENCE_H

#include "isrran/config.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Defines the maximum number of ZC sequence groups (u)
 */
#define ISRRAN_ZC_SEQUENCE_NOF_GROUPS 30

/**
 * @brief Defines the maximum number of base sequences (v)
 */
#define ISRRAN_ZC_SEQUENCE_NOF_BASE 2

/**
 * @brief Generates ZC sequences given the required parameters used in the TS 36 series (LTE)
 *
 * @remark Implemented as defined in TS 36.211 section 5.5.1 Generation of the reference signal sequence
 *
 * @param[in] u Group number {0,1,...29}
 * @param[in] v Base sequence
 * @param[in] alpha Phase shift
 * @param[in] nof_prb Number of PRB
 * @param[out] sequence Output sequence
 * @return ISRRAN_SUCCESS if the generation is successful, ISRRAN_ERROR code otherwise
 */
ISRRAN_API int isrran_zc_sequence_generate_lte(uint32_t u, uint32_t v, float alpha, uint32_t nof_prb, cf_t* sequence);

/**
 * @brief Generates ZC sequences given the required parameters used in the TS 38 series (NR)
 *
 * @remark Implemented as defined in TS 38.211 section 5.2.2 Low-PAPR sequence generation
 *
 * @param u Group number {0,1,...29}
 * @param v base sequence
 * @param alpha Phase shift
 * @param m Number of PRB
 * @param delta Delta parameter described in specification
 * @param sequence Output sequence
 * @return ISRRAN_SUCCESS if the generation is successful, ISRRAN_ERROR code otherwise
 */
ISRRAN_API int
isrran_zc_sequence_generate_nr(uint32_t u, uint32_t v, float alpha, uint32_t m, uint32_t delta, cf_t* sequence);

/**
 * @brief Low-PAPR ZC sequence look-up-table
 */
typedef struct ISRRAN_API {
  uint32_t M_zc;
  uint32_t nof_alphas;
  cf_t*    sequence[ISRRAN_ZC_SEQUENCE_NOF_GROUPS][ISRRAN_ZC_SEQUENCE_NOF_BASE];
} isrran_zc_sequence_lut_t;

/**
 * @brief Initialises a Low-PAPR sequence look-up-table object using NR tables
 *
 * @param q Object pointer
 * @param m Number of PRB
 * @param delta Delta parameter described in specification
 * @param alphas Vector with the alpha shift parameters
 * @param nof_alphas Number alpha shifts to generate
 * @return ISRRAN_SUCCESS if the initialization is successful, ISRRAN_ERROR code otherwise
 */
ISRRAN_API int isrran_zc_sequence_lut_init_nr(isrran_zc_sequence_lut_t* q,
                                              uint32_t                  m,
                                              uint32_t                  delta,
                                              float*                    alphas,
                                              uint32_t                  nof_alphas);

/**
 * @brief Deallocates a Low-PAPR sequence look-up-table object
 * @param q Object pointer
 */
ISRRAN_API void isrran_zc_sequence_lut_free(isrran_zc_sequence_lut_t* q);

/**
 * @brief Get a Low-PAPR sequence from the LUT
 * @param q Object pointer
 * @param u Group number {0,1,...29}
 * @param v base sequence
 * @param alpha_idx Phase shift index
 * @return ISRRAN_SUCCESS if the generation is successful, ISRRAN_ERROR code otherwise
 */
ISRRAN_API const cf_t*
                 isrran_zc_sequence_lut_get(const isrran_zc_sequence_lut_t* q, uint32_t u, uint32_t v, uint32_t alpha_idx);

#endif // ISRRAN_ZC_SEQUENCE_H
