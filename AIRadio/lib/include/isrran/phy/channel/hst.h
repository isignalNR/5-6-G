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

#ifndef ISRRAN_HST_H_
#define ISRRAN_HST_H_

#include "isrran/config.h"
#include "isrran/phy/common/timestamp.h"
#include <stdint.h>

typedef struct {
  // System parameters
  uint32_t srate_hz; // Sampling rate

  // Model Parameters
  float fd_hz;       // Maximum Doppler Frequency
  float ds_m;        // eNb distance [m]
  float dmin_m;      // eNb Rail-track distance [m]
  float period_s;    // 2 * Ds / speed [s]
  float init_time_s; // Time offset [s]

  // State
  float fs_hz; // Last doppler dispersion [Hz]
} isrran_channel_hst_t;

#ifdef __cplusplus
extern "C" {
#endif

ISRRAN_API int isrran_channel_hst_init(isrran_channel_hst_t* q, float fd_hz, float period_d, float init_time_s);

ISRRAN_API void isrran_channel_hst_update_srate(isrran_channel_hst_t* q, uint32_t srate);

ISRRAN_API void
isrran_channel_hst_execute(isrran_channel_hst_t* q, cf_t* in, cf_t* out, uint32_t len, const isrran_timestamp_t* ts);

ISRRAN_API void isrran_channel_hst_free(isrran_channel_hst_t* q);

#ifdef __cplusplus
}
#endif

#endif // ISRRAN_HST_H_
