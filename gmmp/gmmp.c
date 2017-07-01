/* ----------------------------------------------------------------- */
/*             The Speech Signal Processing Toolkit (SPTK)           */
/*             developed by SPTK Working Group                       */
/*             http://sp-tk.sourceforge.net/                         */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 1984-2007  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/*                1996-2016  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the SPTK working group nor the names of its */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

/*************************************************************************
 *                                                                       *
 *    Calculation of GMM log-probagility                                 *
 *                                                                       *
 *                                       2000.7  C. Miyajima             *
 *                                                                       *
 *       usage:                                                          *
 *               gmmp [options] gmmfile [infile] > stdout                *
 *       options:                                                        *
 *               -l l  :  length of vector                      [26]     *
 *               -m m  :  number of Gaussian components         [16]     *
 *               -f    :  full covariance                       [FALSE]  *
 *               -a    :  output average log-probability        [FALSE]  *
 *             (level 2)                                                 *
 *               -B B1 ... Bb : block size in covariance matrix [FALSE]  *
 *                              where (B1 + B2 + ... + Bb) = l           *
 *               -c1   :  inter-block correlation               [FALSE]  *
 *               -c2   :  full covariance in each block         [FALSE]  *
 *               -D    :  print log-probability of each block   [FALSE]  *
 *       infile:                                                         *
 *               input vector sequence                          [stdin]  *
 *       stdout:                                                         *
 *               sequence of frame log-probabilities                     *
 *               average log-probability (if -a is used)                 *
 *                                                                       *
 ************************************************************************/


/*  Standard C Libraries  */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#ifndef HAVE_STRRCHR
#define strrchr rindex
#endif
#endif

#include <math.h>

#if defined(WIN32)
#include "SPTK.h"
#else
#include <SPTK/SPTK.h>
#endif

/*  Default Values  */

#define DEF_L  20
#define DEF_M  16
#define DEF_A  TR
#define DEF_D  FA
#define FULL   FA

static char *BOOL[] = { "FALSE", "TRUE" };

/*  Command Name  */
static char *cmnd = "gmmp";


union c2f {
	float result;
	char c[sizeof(float)];
};

static void usage(int status)
{
  printf("usage function called from %s with option %d\n", cmnd, status);
	exit(status);
  return ;
}

static float ParseFloat(FILE *fp)
{
	union c2f trnsfrmr;
	size_t i;

	for (i = 0; i < sizeof(float) ; i++)
		trnsfrmr.c[i] = getc(fp);

	return trnsfrmr.result;
}

float gmmpFromMFCC(char *gmmFilePath, FILE *MFCCfp)
{
  FILE *fp = stdin, *fgmm = NULL;
  GMM gmm;
  double logp, *ave_logp, *x;
  int M = DEF_M, L = DEF_L, T;
  Boolean aflag = DEF_A, Dflag = DEF_D, full = FULL;
  int cov_dim = 0, dim_list[1024], i, j, l1, l2;
  Boolean block_full = FA, block_corr = FA, multiple_dim = FA;

  FILE *outfp = tmpfile();
  float result;

  fgmm = getfp(gmmFilePath, "rb+");
  fp = MFCCfp;

  /* *********************************************************************** */

  if (multiple_dim == TR) {
    for (i = 0, j = 0; i < cov_dim; i++) {
      j += dim_list[i];
    }
    if (j != L) {
      fprintf(stderr,
              "%s: block size must be coincided with dimention of vector\n",
              cmnd);
      usage(1);
    }
    full = TR;
  } else {
    if (block_corr == TR || block_full == TR) {
      if (full != TR) {
        fprintf(stderr,
                "%s: -c1 and -c2 option must be specified with -B option!\n",
                cmnd);
        usage(1);
      }
    }
  }

  /* Read GMM parameters */
  if (fgmm == NULL) {
    fprintf(stderr, "%s: GMM file must be specified!\n", cmnd);
    usage(1);
  }

  alloc_GMM(&gmm, M, L, full);
  load_GMM(&gmm, fgmm);

  fclose(fgmm);

  if (gmm.full != TR && full == TR) {
    fprintf(stderr,
            "%s: GMM is diagonal covariance, so -f or -B option is inappropriate!\n",
            cmnd);
    usage(1);
  }

  if (full == TR && multiple_dim == TR) {
    maskCov_GMM(&gmm, dim_list, cov_dim, block_full, block_corr);
  }

  if (gmm.full == TR) {
    prepareCovInv_GMM(&gmm);
    prepareGconst_GMM(&gmm);
  }


  /* / change stdout to a tmpfile /
   * / hopefully it works/ */

  /* Calculate and output log-probability */
  T = 0;
  x = dgetmem(L);
  ave_logp = dgetmem(cov_dim);
  while (freadf(x, sizeof(*x), L, fp) == L) {
    if (Dflag == TR) {
      l1 = 0;
      l2 = 0;
      if (multiple_dim != TR) {
        fprintf(stderr,
                "%s: -D option must be specified with -B option!\n", cmnd);
        usage(1);
      }
      for (i = 0; i < cov_dim; i++) {
        l2 = l2 + dim_list[i];
        if (aflag == TR) {
          ave_logp[i] += log_outp(&gmm, l1, l2, x);
        } else {
          logp = log_outp(&gmm, l1, l2, x);
          fwritef(&logp, sizeof(double), 1, outfp);
        }
        l1 = l2;
      }
    } else {
      if (aflag == TR) {
        ave_logp[0] += log_outp(&gmm, 0, L, x);
      } else {
        logp = log_outp(&gmm, 0, L, x);
        fwritef(&logp, sizeof(double), 1, outfp);
      }
    }
    T++;
  }
  /* fclose(fp); */

  if (aflag == TR) {
    if (T == 0) {
      fprintf(stderr, "%s: No input data!\n", cmnd);
      usage(1);
    } else {
      if (Dflag == TR) {
        for (i = 0; i < cov_dim; i++) {
          ave_logp[i] /= (double) T;
          fwritef(&ave_logp[i], sizeof(double), 1, outfp);
        }
      } else {
        ave_logp[0] /= (double) T;
        fwritef(&ave_logp[0], sizeof(double), 1, outfp);
      }
    }
  }

	rewind(outfp);
  result = ParseFloat(outfp);
  fclose(outfp);

  return result;
}
