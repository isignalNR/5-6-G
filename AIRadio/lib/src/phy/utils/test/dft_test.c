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

#include "isrran/phy/utils/debug.h"
#include "isrran/phy/utils/random.h"
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "isrran/phy/dft/dft.h"
#include "isrran/phy/utils/vector.h"

uint32_t N       = 256;
bool     forward = true;
bool     mirror  = false;
bool     norm    = false;
bool     dc      = false;

void usage(char* prog)
{
  printf("Usage: %s\n", prog);
  printf("\t-N Transform size [Default 256]\n");
  printf("\t-b Backwards transform first [Default Forwards]\n");
  printf("\t-m Mirror the transform freq bins [Default false]\n");
  printf("\t-n Normalize the transform output [Default false]\n");
  printf("\t-d Handle insertion/removal of null DC carrier internally [Default false]\n");
}

void parse_args(int argc, char** argv)
{
  int opt;
  while ((opt = getopt(argc, argv, "Nbmnd")) != -1) {
    switch (opt) {
      case 'N':
        N = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'b':
        forward = false;
        break;
      case 'm':
        mirror = true;
        break;
      case 'n':
        norm = true;
        break;
      case 'd':
        dc = true;
        break;
      default:
        usage(argv[0]);
        exit(-1);
    }
  }
}

void print(cf_t* in, int len)
{
  for (int i = 0; i < len; i++) {
    float re = crealf(in[i]);
    float im = cimagf(in[i]);
    printf("%f+%fi, ", re, im);
  }
  printf("\n\n");
}

int test_dft(cf_t* in)
{
  int res = 0;

  cf_t* out1 = isrran_vec_cf_malloc(N);
  cf_t* out2 = isrran_vec_cf_malloc(N);
  isrran_vec_cf_zero(out1, N);
  isrran_vec_cf_zero(out2, N);

  isrran_dft_plan_t plan = {};
  if (forward) {
    if (isrran_dft_plan(&plan, N, ISRRAN_DFT_FORWARD, ISRRAN_DFT_COMPLEX) != ISRRAN_SUCCESS) {
      ERROR("Error in DFT plan");
      goto clean_exit;
    }
  } else {
    if (isrran_dft_plan(&plan, N, ISRRAN_DFT_BACKWARD, ISRRAN_DFT_COMPLEX) != ISRRAN_SUCCESS) {
      ERROR("Error in DFT plan");
      goto clean_exit;
    }
  }
  isrran_dft_plan_set_mirror(&plan, mirror);
  isrran_dft_plan_set_norm(&plan, norm);
  isrran_dft_plan_set_dc(&plan, dc);

  print(in, N);
  isrran_dft_run(&plan, in, out1);
  print(out1, N);

  isrran_dft_plan_t plan_rev;
  if (!forward) {
    if (isrran_dft_plan(&plan_rev, N, ISRRAN_DFT_FORWARD, ISRRAN_DFT_COMPLEX) != ISRRAN_SUCCESS) {
      ERROR("Error in DFT plan");
      goto clean_exit;
    }
  } else {
    if (isrran_dft_plan(&plan_rev, N, ISRRAN_DFT_BACKWARD, ISRRAN_DFT_COMPLEX) != ISRRAN_SUCCESS) {
      ERROR("Error in DFT plan");
      goto clean_exit;
    }
  }
  isrran_dft_plan_set_mirror(&plan_rev, mirror);
  isrran_dft_plan_set_norm(&plan_rev, norm);
  isrran_dft_plan_set_dc(&plan_rev, dc);

  isrran_dft_run(&plan_rev, out1, out2);
  print(out2, N);

  if (!norm) {
    cf_t n = N + 0 * I;
    for (int i = 0; i < N; i++)
      out2[i] /= n;
  }

  for (int i = 0; i < N; i++) {
    float diff = cabsf(in[i] - out2[i]);
    if (diff > 0.01)
      res = -1;
  }

clean_exit:
  isrran_dft_plan_free(&plan);
  isrran_dft_plan_free(&plan_rev);
  free(out1);
  free(out2);

  return res;
}

int main(int argc, char** argv)
{
  isrran_random_t random_gen = isrran_random_init(0x1234);
  parse_args(argc, argv);
  cf_t* in = isrran_vec_cf_malloc(N);
  in[0]    = 0.0f;
  isrran_random_uniform_complex_dist_vector(random_gen, &in[1], N - 1, -1, 1);

  if (test_dft(in) != 0)
    return -1;

  free(in);
  isrran_random_free(random_gen);
  printf("Done\n");
  exit(0);
}
