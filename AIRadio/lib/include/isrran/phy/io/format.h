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

#ifndef ISRRAN_FORMAT_H
#define ISRRAN_FORMAT_H

typedef enum {
  ISRRAN_TEXT,
  ISRRAN_FLOAT,
  ISRRAN_COMPLEX_FLOAT,
  ISRRAN_COMPLEX_SHORT,
  ISRRAN_FLOAT_BIN,
  ISRRAN_COMPLEX_FLOAT_BIN,
  ISRRAN_COMPLEX_SHORT_BIN
} isrran_datatype_t;

#endif // ISRRAN_FORMAT_H
