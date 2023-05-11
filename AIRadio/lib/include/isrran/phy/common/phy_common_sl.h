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

/**********************************************************************************************
 *  File:         phy_common_sl.h
 *
 *  Description:  Common parameters and lookup functions for Sidelink PHY
 *
 *  Reference:    3GPP TS 36.211 version 15.3.0 Release 15
 *********************************************************************************************/

#ifndef ISRRAN_PHY_COMMON_SL_H
#define ISRRAN_PHY_COMMON_SL_H

/**
 *  \brief Common parameters and lookup functions for Sidelink PHY
 */

#include "isrran/phy/common/phy_common.h"

typedef enum ISRRAN_API {
  ISRRAN_SIDELINK_TM1 = 0,
  ISRRAN_SIDELINK_TM2,
  ISRRAN_SIDELINK_TM3,
  ISRRAN_SIDELINK_TM4
} isrran_sl_tm_t;

typedef enum ISRRAN_API {
  ISRRAN_SIDELINK_PSBCH = 0, // Physical Sidelink Broadcast Channel
  ISRRAN_SIDELINK_PSCCH,     // Physical Sidelink Control Channel
  ISRRAN_SIDELINK_PSSCH,     // Physical Sidelink Shared Channel
  ISRRAN_SIDELINK_PSDCH      // Physical Sidelink Discovery Channel
} isrran_sl_channels_t;

typedef struct ISRRAN_API {
  isrran_sl_tm_t tm;
  uint32_t       N_sl_id;
  uint32_t       nof_prb;
  isrran_cp_t    cp;
} isrran_cell_sl_t;

#define ISRRAN_SL_MAX_PERIOD_LENGTH 320 // SL-PeriodComm-r12 3GPP TS 36.331 Section 6.3.8
// SL-CommResourcePool: 3GPP TS 36.331 version 15.6.0 Release 15 Section 6.3.8
typedef struct ISRRAN_API {
  uint32_t period_length;

  uint32_t prb_num;
  uint32_t prb_start;
  uint32_t prb_end;

  uint8_t pscch_sf_bitmap[ISRRAN_SL_MAX_PERIOD_LENGTH];
  uint8_t pssch_sf_bitmap[ISRRAN_SL_MAX_PERIOD_LENGTH];

  uint32_t size_sub_channel;      // sizeSubchannel-r14
  uint32_t num_sub_channel;       // numSubchannel-r14
  uint32_t start_prb_sub_channel; // startRB-Subchannel-r14 offset
  bool     adjacency_pscch_pssch; // adjacencyPSCCH-PSSCH-r14

  uint32_t sf_bitmap_tm34_len;
  uint8_t  sf_bitmap_tm34[ISRRAN_SL_MAX_PERIOD_LENGTH]; // sl_Subframe_r14: 3GPP 36.331 Section 6.3.8
} isrran_sl_comm_resource_pool_t;

typedef enum ISRRAN_API {
  ISRRAN_SIDELINK_DATA_SYMBOL = 0,
  ISRRAN_SIDELINK_SYNC_SYMBOL,
  ISRRAN_SIDELINK_DMRS_SYMBOL,
  ISRRAN_SIDELINK_GUARD_SYMBOL
} isrran_sl_symbol_t;

#define ISRRAN_SL_DUPLEX_MODE_FDD (1)
#define ISRRAN_SL_DUPLEX_MODE_TDD (2)

#define ISRRAN_SLSS_SIDE_PEAK_OFFSET (0.005f)
#define ISRRAN_SLSS_SIDE_PEAK_THRESHOLD_HIGH (0.49f) // square(0.7), max 70% of main peak
#define ISRRAN_SLSS_SIDE_PEAK_THRESHOLD_LOW (0.09f)  // square(0.3), min 30% of main peak

#define ISRRAN_PSBCH_NOF_PRB (6)
#define ISRRAN_PSCCH_TM34_NOF_PRB (2)

#define ISRRAN_MIB_SL_LEN (40)     // TM1/2: 40 bits
#define ISRRAN_MIB_SL_V2X_LEN (48) // TM3/4: 48 bits
#define ISRRAN_MIB_SL_MAX_LEN (ISRRAN_MIB_SL_V2X_LEN)

#define ISRRAN_SL_TM12_DEFAULT_NUM_DMRS_SYMBOLS (2)
#define ISRRAN_SL_TM34_DEFAULT_NUM_DMRS_SYMBOLS (4) ///< In TM3/4, all channels have 4 DMRS by default

#define ISRRAN_PSBCH_TM12_NUM_DATA_SYMBOLS (8) // PSBCH is in 8 OFDM symbols (but only 7 are tx'ed)
#define ISRRAN_PSBCH_TM12_NUM_DATA_SYMBOLS_EXT                                                                         \
  (6) // PSBCH is in 7 OFDM symbols for extended cyclic prefix (but only 6 are tx'ed)
#define ISRRAN_PSBCH_TM12_NUM_DMRS_SYMBOLS (2) ///< PSBCH has 2 DMRS symbols
#define ISRRAN_PSBCH_TM12_NUM_SYNC_SYMBOLS (4) ///< Two symbols PSSS and two SSSS

#define ISRRAN_PSBCH_TM34_NUM_DATA_SYMBOLS (7) ///< SL-BCH is in 7 OFDM symbols (but only 6 are tx'ed)
#define ISRRAN_PSBCH_TM34_NUM_DMRS_SYMBOLS (3) ///< PSBCH has 3 DMRS symbols in TM3 and TM4
#define ISRRAN_PSBCH_TM34_NUM_SYNC_SYMBOLS (4) ///< Two symbols PSSS and two SSSS

#define ISRRAN_SCI_CRC_LEN (16)
#define ISRRAN_SCI_MAX_LEN (45)
#define ISRRAN_SCI_TM34_LEN (32)

#define ISRRAN_PSCCH_QM 2
#define ISRRAN_PSCCH_TM12_NOF_PRB (1)
#define ISRRAN_PSCCH_TM34_NOF_PRB (2)
#define ISRRAN_PSCCH_MAX_NOF_PRB (ISRRAN_PSCCH_TM34_NOF_PRB)
#define ISRRAN_PSCCH_SCRAMBLING_SEED (510) ///< Scrambling seed for PSCCH is 510

#define ISRRAN_PSCCH_TM12_NUM_DATA_SYMBOLS (12)
#define ISRRAN_PSCCH_TM12_NUM_DMRS_SYMBOLS (2)
#define ISRRAN_PSCCH_TM12_NUM_DATA_SYMBOLS_EXT (10)

#define ISRRAN_PSCCH_TM34_NUM_DATA_SYMBOLS (10)
#define ISRRAN_PSCCH_TM34_NUM_DMRS_SYMBOLS (4)

#define ISRRAN_PSCCH_TM12_NOF_CODED_BITS                                                                               \
  (ISRRAN_NRE * ISRRAN_PSCCH_TM12_NUM_DATA_SYMBOLS * ISRRAN_PSCCH_TM12_NOF_PRB * ISRRAN_PSCCH_QM)
#define ISRRAN_PSCCH_TM34_NOF_CODED_BITS                                                                               \
  (ISRRAN_NRE * ISRRAN_PSCCH_TM34_NUM_DATA_SYMBOLS * ISRRAN_PSCCH_TM34_NOF_PRB * ISRRAN_PSCCH_QM)
#define ISRRAN_PSCCH_MAX_CODED_BITS ISRRAN_MAX(ISRRAN_PSCCH_TM12_NOF_CODED_BITS, ISRRAN_PSCCH_TM34_NOF_CODED_BITS)

#define ISRRAN_PSSCH_MAX_QM 6
#define ISRRAN_PSSCH_CRC_LEN 24
#define ISRRAN_MAX_CODEWORD_LEN 168000   // 12 subcarriers * 100 PRB * 14 symbols * 10 bits, assuming 1024QAM
#define ISRRAN_SL_SCH_MAX_TB_LEN (48936) // 3GPP 36.306 v15.4.0 Table 4.1B-1
#define ISRRAN_PSSCH_MAX_CODED_BITS (3 * ISRRAN_TCOD_MAX_LEN_CB + ISRRAN_TCOD_TOTALTAIL)

#define ISRRAN_PSSCH_TM12_NUM_DATA_SYMBOLS (12) // PSSCH is in 12 OFDM symbols (but only 11 are tx'ed)
#define ISRRAN_PSSCH_TM12_NUM_DMRS_SYMBOLS (2)  // PSSCH has 2 DMRS symbols in TM1 and TM2

#define ISRRAN_PSSCH_TM12_NUM_DATA_SYMBOLS_CP_EXT                                                                      \
  (10) // PSSCH is in 10 OFDM symbols for extended cyclic prefix (but only 9 are tx'ed)
#define ISRRAN_PSSCH_TM12_NUM_DMRS_SYMBOLS_CP_EXT                                                                      \
  (2) // PSSCH has 2 DMRS symbols for extended cyclic prefix in TM1 and TM2

#define ISRRAN_PSSCH_TM34_NUM_DATA_SYMBOLS (10) // PSSCH is in 10 OFDM symbols (but only 9 are tx'ed)
#define ISRRAN_PSSCH_TM34_NUM_DMRS_SYMBOLS (4)  // PSSCH has 4 DMRS symbols in TM3 and TM4

ISRRAN_API int isrran_sl_group_hopping_f_gh(uint32_t f_gh[ISRRAN_NSLOTS_X_FRAME * 2], uint32_t N_x_id);
#define ISRRAN_PSCCH_MAX_NUM_DATA_SYMBOLS (ISRRAN_PSCCH_TM12_NUM_DATA_SYMBOLS)

ISRRAN_API bool isrran_slss_side_peak_pos_is_valid(uint32_t side_peak_pos,
                                                   uint32_t main_peak_pos,
                                                   uint32_t side_peak_delta_a,
                                                   uint32_t side_peak_delta_b);
ISRRAN_API bool isrran_slss_side_peak_value_is_valid(float side_peak_value, float threshold_low, float threshold_high);

ISRRAN_API int isrran_sl_tm_to_cell_sl_tm_t(isrran_cell_sl_t* q, uint32_t tm);

ISRRAN_API uint32_t isrran_sl_get_num_symbols(isrran_sl_tm_t tm, isrran_cp_t cp);
ISRRAN_API bool     isrran_psbch_is_symbol(isrran_sl_symbol_t type, isrran_sl_tm_t tm, uint32_t i, isrran_cp_t cp);
ISRRAN_API bool     isrran_pscch_is_symbol(isrran_sl_symbol_t type, isrran_sl_tm_t tm, uint32_t i, isrran_cp_t cp);
ISRRAN_API bool     isrran_pssch_is_symbol(isrran_sl_symbol_t type, isrran_sl_tm_t tm, uint32_t i, isrran_cp_t cp);

ISRRAN_API uint32_t isrran_sci_format0_sizeof(uint32_t nof_prb);

ISRRAN_API int isrran_sl_comm_resource_pool_get_default_config(isrran_sl_comm_resource_pool_t* q,
                                                               isrran_cell_sl_t                cell);

#endif // ISRRAN_PHY_COMMON_SL_H