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

#ifndef ISRRAN_CSI_RS_CFG_H
#define ISRRAN_CSI_RS_CFG_H

#include "isrran/config.h"
#include "isrran/phy/common/phy_common_nr.h"

/**
 * @brief Maximum number of ZP CSI-RS resources per set defined in
 * - TS 38.214 clause 5.1.4.2 PDSCH resource mapping with RE level granularity
 * - TS 38.331 constant maxNrofZP-CSI-RS-ResourcesPerSet
 */
#define ISRRAN_PHCH_CFG_MAX_NOF_CSI_RS_PER_SET 16

/**
 * @brief Maximum number of ZP CSI-RS Sets defined in TS 38.331 constant maxNrofZP-CSI-RS-ResourceSets
 */
#define ISRRAN_PHCH_CFG_MAX_NOF_CSI_RS_SETS 16

/**
 * @brief Maximum number of CSI-RS frequency domain allocation bits
 */
#define ISRRAN_CSI_RS_NOF_FREQ_DOMAIN_ALLOC_MAX 12

typedef enum ISRRAN_API {
  isrran_csi_rs_resource_mapping_row_1 = 0,
  isrran_csi_rs_resource_mapping_row_2,
  isrran_csi_rs_resource_mapping_row_4,
  isrran_csi_rs_resource_mapping_row_other,
} isrran_csi_rs_resource_mapping_row_t;

typedef enum ISRRAN_API {
  isrran_csi_rs_resource_mapping_density_three = 0,
  isrran_csi_rs_resource_mapping_density_dot5_even,
  isrran_csi_rs_resource_mapping_density_dot5_odd,
  isrran_csi_rs_resource_mapping_density_one,
  isrran_csi_rs_resource_mapping_density_spare
} isrran_csi_rs_density_t;

typedef enum ISRRAN_API {
  isrran_csi_rs_cdm_nocdm = 0,
  isrran_csi_rs_cdm_fd_cdm2,
  isrran_csi_rs_cdm_cdm4_fd2_td2,
  isrran_csi_rs_cdm_cdm8_fd2_td4
} isrran_csi_rs_cdm_t;

/**
 * @brief Contains CSI-FrequencyOccupation flattened configuration
 */
typedef struct ISRRAN_API {
  uint32_t start_rb; ///< PRB where this CSI resource starts in relation to common resource block #0 (CRB#0) on the
  ///< common resource block grid. Only multiples of 4 are allowed (0, 4, ..., 274)

  uint32_t nof_rb; ///< Number of PRBs across which this CSI resource spans. Only multiples of 4 are allowed. The
  ///< smallest configurable number is the minimum of 24 and the width of the associated BWP. If the
  ///< configured value is larger than the width of the corresponding BWP, the UE shall assume that the
  ///< actual CSI-RS bandwidth is equal to the width of the BWP.
} isrran_csi_rs_freq_occupation_t;

/**
 * @brief Contains CSI-ResourcePeriodicityAndOffset flattened configuration
 */
typedef struct ISRRAN_API {
  uint32_t period; // 4,5,8,10,16,20,32,40,64,80,160,320,640
  uint32_t offset; // 0..period-1
} isrran_csi_rs_period_and_offset_t;

/**
 * @brief Contains CSI-RS-ResourceMapping flattened configuration
 */
typedef struct ISRRAN_API {
  isrran_csi_rs_resource_mapping_row_t row;
  bool                                 frequency_domain_alloc[ISRRAN_CSI_RS_NOF_FREQ_DOMAIN_ALLOC_MAX];
  uint32_t                             nof_ports;         // 1, 2, 4, 8, 12, 16, 24, 32
  uint32_t                             first_symbol_idx;  // 0..13
  uint32_t                             first_symbol_idx2; // 2..12 (set to 0 for disabled)
  isrran_csi_rs_cdm_t                  cdm;
  isrran_csi_rs_density_t              density;
  isrran_csi_rs_freq_occupation_t      freq_band;
} isrran_csi_rs_resource_mapping_t;

/**
 * @brief Contains TS 38.331 NZP-CSI-RS-Resource flattened configuration
 */
typedef struct ISRRAN_API {
  uint32_t                          id;
  isrran_csi_rs_resource_mapping_t  resource_mapping;        ///< CSI-RS time/frequency mapping
  float                             power_control_offset;    ///< -8..15 dB
  float                             power_control_offset_ss; ///< -3, 0, 3, 6 dB
  uint32_t                          scrambling_id;           ///< 0..1023
  isrran_csi_rs_period_and_offset_t periodicity;             ///< Periodicity
} isrran_csi_rs_nzp_resource_t;

/**
 * @brief Non-Zero-Power CSI resource set
 */
typedef struct ISRRAN_API {
  isrran_csi_rs_nzp_resource_t data[ISRRAN_PHCH_CFG_MAX_NOF_CSI_RS_PER_SET]; ///< Resources
  uint32_t                     count;                                        ///< Set to zero for not present
  bool trs_info; ///< Indicates that the antenna port for all NZP-CSI-RS resources in the CSI-RS resource set is same.
} isrran_csi_rs_nzp_set_t;

/**
 * @brief Contains TS 38.331 ZP-CSI-RS-Resource flattened configuration
 */
typedef struct {
  uint32_t                          id;
  isrran_csi_rs_resource_mapping_t  resource_mapping; ///< CSI-RS time/frequency mapping
  isrran_csi_rs_period_and_offset_t periodicity;
} isrran_csi_rs_zp_resource_t;

/**
 * @brief Zero-Power CSI resource set
 */
typedef struct ISRRAN_API {
  isrran_csi_rs_zp_resource_t data[ISRRAN_PHCH_CFG_MAX_NOF_CSI_RS_PER_SET]; ///< Resources
  uint32_t                    count;                                        ///< Number of resources in the set
} isrran_csi_rs_zp_set_t;

ISRRAN_API bool isrran_csi_rs_resource_mapping_is_valid(const isrran_csi_rs_resource_mapping_t* res);

ISRRAN_API uint32_t isrran_csi_rs_resource_mapping_info(const isrran_csi_rs_resource_mapping_t* res,
                                                        char*                                   str,
                                                        uint32_t                                str_len);

#endif // ISRRAN_CSI_RS_CFG_H
