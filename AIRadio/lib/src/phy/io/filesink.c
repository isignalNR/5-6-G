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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "isrran/phy/io/filesink.h"

int isrran_filesink_init(isrran_filesink_t* q, const char* filename, isrran_datatype_t type)
{
  bzero(q, sizeof(isrran_filesink_t));
  q->f = fopen(filename, "w");
  if (!q->f) {
    perror("fopen");
    return -1;
  }
  q->type = type;
  return 0;
}

void isrran_filesink_free(isrran_filesink_t* q)
{
  if (q->f) {
    fclose(q->f);
  }
  bzero(q, sizeof(isrran_filesink_t));
}

int isrran_filesink_write(isrran_filesink_t* q, void* buffer, int nsamples)
{
  int             i    = 0;
  float*          fbuf = (float*)buffer;
  _Complex float* cbuf = (_Complex float*)buffer;
  _Complex short* sbuf = (_Complex short*)buffer;
  int             size = 0;

  switch (q->type) {
    case ISRRAN_TEXT:
      fprintf(q->f, "%s", (char*)buffer);
      break;
    case ISRRAN_FLOAT:
      for (i = 0; i < nsamples; i++) {
        fprintf(q->f, "%g\n", fbuf[i]);
      }
      break;
    case ISRRAN_COMPLEX_FLOAT:
      for (i = 0; i < nsamples; i++) {
        if (__imag__ cbuf[i] >= 0)
          fprintf(q->f, "%g+%gi\n", __real__ cbuf[i], __imag__ cbuf[i]);
        else
          fprintf(q->f, "%g-%gi\n", __real__ cbuf[i], fabsf(__imag__ cbuf[i]));
      }
      break;
    case ISRRAN_COMPLEX_SHORT:
      for (i = 0; i < nsamples; i++) {
        if (__imag__ sbuf[i] >= 0)
          fprintf(q->f, "%hd+%hdi\n", __real__ sbuf[i], __imag__ sbuf[i]);
        else
          fprintf(q->f, "%hd-%hdi\n", __real__ sbuf[i], (short)abs(__imag__ sbuf[i]));
      }
      break;
    case ISRRAN_FLOAT_BIN:
    case ISRRAN_COMPLEX_FLOAT_BIN:
    case ISRRAN_COMPLEX_SHORT_BIN:
      if (q->type == ISRRAN_FLOAT_BIN) {
        size = sizeof(float);
      } else if (q->type == ISRRAN_COMPLEX_FLOAT_BIN) {
        size = sizeof(_Complex float);
      } else if (q->type == ISRRAN_COMPLEX_SHORT_BIN) {
        size = sizeof(_Complex short);
      }
      return fwrite(buffer, size, nsamples, q->f);
    default:
      i = -1;
      break;
  }
  return i;
}

int isrran_filesink_write_multi(isrran_filesink_t* q, void** buffer, int nsamples, int nchannels)
{
  int              i, j;
  float**          fbuf = (float**)buffer;
  _Complex float** cbuf = (_Complex float**)buffer;
  _Complex short** sbuf = (_Complex short**)buffer;
  int              size = 0;

  switch (q->type) {
    case ISRRAN_FLOAT:
      for (i = 0; i < nsamples; i++) {
        for (j = 0; j < nchannels; j++) {
          fprintf(q->f, "%g%c", fbuf[j][i], (j != (nchannels - 1)) ? '\t' : '\n');
        }
      }
      break;
    case ISRRAN_COMPLEX_FLOAT:
      for (i = 0; i < nsamples; i++) {
        for (j = 0; j < nchannels; j++) {
          fprintf(q->f, "%g%+gi%c", __real__ cbuf[j][i], __imag__ cbuf[j][i], (j != (nchannels - 1)) ? '\t' : '\n');
        }
      }
      break;
    case ISRRAN_COMPLEX_SHORT:
      for (i = 0; i < nsamples; i++) {
        for (j = 0; j < nchannels; j++) {
          fprintf(q->f, "%hd%+hdi%c", __real__ sbuf[j][i], __imag__ sbuf[j][i], (j != (nchannels - 1)) ? '\t' : '\n');
        }
      }
      break;
    case ISRRAN_FLOAT_BIN:
    case ISRRAN_COMPLEX_FLOAT_BIN:
    case ISRRAN_COMPLEX_SHORT_BIN:
      if (q->type == ISRRAN_FLOAT_BIN) {
        size = sizeof(float);
      } else if (q->type == ISRRAN_COMPLEX_FLOAT_BIN) {
        size = sizeof(_Complex float);
      } else if (q->type == ISRRAN_COMPLEX_SHORT_BIN) {
        size = sizeof(_Complex short);
      }
      if (nchannels > 1) {
        uint32_t count = 0;
        for (i = 0; i < nsamples; i++) {
          for (j = 0; j < nchannels; j++) {
            count += fwrite(&cbuf[j][i], size, 1, q->f);
          }
        }
        return count;
      } else {
        return fwrite(buffer[0], size, nsamples, q->f);
      }
      break;
    default:
      i = -1;
      break;
  }
  return i;
}
