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

#ifndef ISRRAN_PUCCH_CFG_NR_H
#define ISRRAN_PUCCH_CFG_NR_H

#include "isrran/config.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * NR-PUCCH Format 0 ranges
 */
#define ISRRAN_PUCCH_NR_FORMAT0_MAX_CS 11
#define ISRRAN_PUCCH_NR_FORMAT0_MIN_NSYMB 1
#define ISRRAN_PUCCH_NR_FORMAT0_MAX_NSYMB 2
#define ISRRAN_PUCCH_NR_FORMAT0_MAX_STARTSYMB 13

/**
 * NR-PUCCH Format 1 ranges
 */
#define ISRRAN_PUCCH_NR_FORMAT1_MAX_CS 11
#define ISRRAN_PUCCH_NR_FORMAT1_MAX_TOCC 6
#define ISRRAN_PUCCH_NR_FORMAT1_MIN_NSYMB 4
#define ISRRAN_PUCCH_NR_FORMAT1_MAX_NSYMB 14
#define ISRRAN_PUCCH_NR_FORMAT1_MAX_STARTSYMB 10
#define ISRRAN_PUCCH_NR_FORMAT1_MAX_NOF_BITS 2

/**
 * NR-PUCCH Format 2 ranges
 */
#define ISRRAN_PUCCH_NR_FORMAT2_MIN_NPRB 1
#define ISRRAN_PUCCH_NR_FORMAT2_MAX_NPRB 16
#define ISRRAN_PUCCH_NR_FORMAT2_MIN_NSYMB 1
#define ISRRAN_PUCCH_NR_FORMAT2_MAX_NSYMB 2
#define ISRRAN_PUCCH_NR_FORMAT2_MAX_STARTSYMB 13
#define ISRRAN_PUCCH_NR_FORMAT2_MIN_NOF_BITS 3

/**
 * NR-PUCCH Format 3 ranges
 */
#define ISRRAN_PUCCH_NR_FORMAT3_MIN_NPRB 1
#define ISRRAN_PUCCH_NR_FORMAT3_MAX_NPRB 16
#define ISRRAN_PUCCH_NR_FORMAT3_MIN_NSYMB 4
#define ISRRAN_PUCCH_NR_FORMAT3_MAX_NSYMB 14
#define ISRRAN_PUCCH_NR_FORMAT3_MAX_STARTSYMB 10

/**
 * NR-PUCCH Format 4 ranges
 */
#define ISRRAN_PUCCH_NR_FORMAT4_NPRB 1
#define ISRRAN_PUCCH_NR_FORMAT4_MIN_NSYMB 4
#define ISRRAN_PUCCH_NR_FORMAT4_MAX_NSYMB 14
#define ISRRAN_PUCCH_NR_FORMAT4_MAX_STARTSYMB 10

/**
 * NR-PUCCH Formats 2, 3 and 4 code rate range
 */
#define ISRRAN_PUCCH_NR_MAX_CODE_RATE 7

/**
 * Maximum number of NR-PUCCH resources per set (TS 38.331 maxNrofPUCCH-ResourcesPerSet)
 */
#define ISRRAN_PUCCH_NR_MAX_NOF_RESOURCES_PER_SET 32

/**
 * Maximum number NR-PUCCH resource sets (TS 38.331 maxNrofPUCCH-ResourceSets)
 */
#define ISRRAN_PUCCH_NR_MAX_NOF_SETS 4

/**
 * Maximum number of SR resources (TS 38.331 maxNrofSR-Resources)
 */
#define ISRRAN_PUCCH_MAX_NOF_SR_RESOURCES 8

typedef enum ISRRAN_API {
  ISRRAN_PUCCH_NR_FORMAT_0 = 0,
  ISRRAN_PUCCH_NR_FORMAT_1,
  ISRRAN_PUCCH_NR_FORMAT_2,
  ISRRAN_PUCCH_NR_FORMAT_3,
  ISRRAN_PUCCH_NR_FORMAT_4,
  ISRRAN_PUCCH_NR_FORMAT_ERROR,
} isrran_pucch_nr_format_t;

typedef enum ISRRAN_API {
  ISRRAN_PUCCH_NR_GROUP_HOPPING_NEITHER = 0,
  ISRRAN_PUCCH_NR_GROUP_HOPPING_ENABLE,
  ISRRAN_PUCCH_NR_GROUP_HOPPING_DISABLE
} isrran_pucch_nr_group_hopping_t;

/**
 * @brief PUCCH Common configuration
 * @remark Defined in TS 38.331 PUCCH-ConfigCommon
 */
typedef struct ISRRAN_API {
  uint32_t                        resource_common; ///< Configures a set of cell-specific PUCCH resources/parameters
  isrran_pucch_nr_group_hopping_t group_hopping;   ///< Configuration of group and sequence hopping
  uint32_t hopping_id; ///< Cell-specific scrambling ID for group hopping and sequence hopping if enabled
  bool     hopping_id_present;
  float    p0_nominal; ///< Power control parameter P0 for PUCCH transmissions. Value in dBm. (-202..24)

  // From PUSCH-config
  bool     scrambling_id_present;
  uint32_t scambling_id; // Identifier used to initialize data scrambling (dataScramblingIdentityPUSCH, 0-1023)
} isrran_pucch_nr_common_cfg_t;

/**
 * @brief Generic PUCCH Resource configuration
 * @remark Defined in TS 38.331 PUCCH-Config
 */
typedef struct ISRRAN_API {
  // Common PUCCH-Resource parameter
  uint32_t starting_prb;
  bool     intra_slot_hopping;
  uint32_t second_hop_prb;

  // Common PUCCH-Resource parameters among all formats
  isrran_pucch_nr_format_t format;           ///< PUCCH format this configuration belongs
  uint32_t                 nof_symbols;      ///< Number of symbols
  uint32_t                 start_symbol_idx; ///< Starting symbol index

  // Specific PUCCH-Resource
  uint32_t initial_cyclic_shift; ///< Used by formats 0, 1
  uint32_t time_domain_occ;      ///< Used by format 1
  uint32_t nof_prb;              ///< Used by formats 2, 3
  uint32_t occ_lenth;            ///< Spreading factor, used by format 4 (2, 4). Also called N_PUCCH4_SF
  uint32_t occ_index;            ///< Used by format 4

  // PUCCH Format common parameters
  bool     enable_pi_bpsk;  ///< Enables PI-BPSK
  uint32_t max_code_rate;   ///< Maximum code rate r (0..7)
  bool     additional_dmrs; ///< UE enables 2 DMRS symbols per hop of a PUCCH Format 3 or 4
} isrran_pucch_nr_resource_t;

typedef struct ISRRAN_API {
  isrran_pucch_nr_resource_t resources[ISRRAN_PUCCH_NR_MAX_NOF_RESOURCES_PER_SET];
  uint32_t                   nof_resources;    ///< Set to 0 if it is NOT provided by higher layers
  uint32_t                   max_payload_size; ///< Maximum payload size, set to 0 if not present
} isrran_pucch_nr_resource_set_t;

/**
 * @brief Scheduling Request resource described in TS 38.331 SchedulingRequestResourceConfig
 * @note Every SR configuration corresponds to one or more logical channels (resources)
 */
typedef struct ISRRAN_API {
  uint32_t                   sr_id;      ///< Scheduling Request identifier
  uint32_t                   period;     ///< Period in slots
  uint32_t                   offset;     ///< Offset from beginning of the period in slots
  isrran_pucch_nr_resource_t resource;   ///< PUCCH resource
  bool                       configured; ///< Set to true if higher layers added this value, otherwise set to false
} isrran_pucch_nr_sr_resource_t;

typedef struct ISRRAN_API {
  isrran_pucch_nr_common_cfg_t   common; ///< NR-PUCCH configuration common for all formats and resources
  isrran_pucch_nr_resource_set_t sets[ISRRAN_PUCCH_NR_MAX_NOF_SETS]; ///< Resource sets, indexed by pucch-ResourceSetId
  bool                           enabled;                            ///< Set to true if any set is enabled
  isrran_pucch_nr_sr_resource_t
      sr_resources[ISRRAN_PUCCH_MAX_NOF_SR_RESOURCES]; ///< SR resources, indexed by identifier
} isrran_pucch_nr_hl_cfg_t;

/**
 * @brief Validates an NR-PUCCH resource configuration provided by upper layers
 * @param resource Resource configuration to validate
 * @return ISRRAN_SUCCESS if valid, ISRRAN_ERROR code otherwise
 */
ISRRAN_API int isrran_pucch_nr_cfg_resource_valid(const isrran_pucch_nr_resource_t* resource);

#endif // ISRRAN_PUCCH_CFG_NR_H
