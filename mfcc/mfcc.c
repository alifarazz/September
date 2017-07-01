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
*                                                                        *
*    MFCC Analysis                                                       *
*                                                                        *
*                                       2011.9 T.Sawada                  *
*                                                                        *
*       usage:                                                           *
*               mfcc [ options ] [ infile ] > stdout                     *
*       options:                                                         *
*               -a  a    :  pre-emphasis coefficient             [0.97]  *
*               -c  c    :  liftering coefficient                [0]     *
*               -e  e    :  flooring value for culculating log(x)[1.0]   *
*                           in filterbank analysis                       *
*                           if x < e then x = e                          *
*               -l  l    :  frame length of input                [256]   *
*               -L  L    :  frame length of window               [256]   *
*               -m  m    :  order of cepstrum                    [13]    *
*               -n  n    :  order of channel for mel-filter bank [26]    *
*               -s  s    :  sampling frequency (kHz)             [16.0]  *
*               -d       :  without using fft algorithm          [FALSE] *
*               -w       :  use hamming window                   [FALSE] *
*               -E       :  use power                            [FALSE] *
*               -0       :  use 0'th static coefficient          [FALSE] *
*       infile:                                                          *
*               data sequence                                            *
*                   , x(0), x(1), ..., x(l-1),                           *
*       stdout:                                                          *
*               mel-frequency cepstral coefficients                      *
*                   , mc(0), mc(1), ..., mc(m-1),                        *
*               if -E or -0 option is given, Energy E and 0'th static    *
*               coefficient C0 is output as follows,                     *
*                   , mc(0), mc(1), ..., mc(m-1), E (C0)                 *
*               if Both -E and -0 option is given, C0 is output before E *
*                                                                        *
*       require:                                                         *
*               mfcc()                                                   *
*                                                                        *
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SPTK/SPTK.h>


/*  Default Values  */
#define ORDER 20
#define WLNG 400
#define EPS 1.0
#define CHANNEL 20
#define USEHAMMING FA
#define DFTMODE FA
#define CZERO FA
#define ENERGY  FA
#define SAMPLEFREQ 44.1
#define ALPHA 0.97
#define LIFT 22
#define WTYPE 0

static char *BOOL[] = { "FALSE", "TRUE" };

FILE* MFCCFromFrame(FILE* fp)
{
  int m = ORDER, l = WLNG, L = -1, n = CHANNEL, lift = LIFT, wtype =
    WTYPE, num = 0;
  double eps = EPS, fs = SAMPLEFREQ, alpha = ALPHA, *x, *mc;
  /* FILE *fp = stdin; */
  Boolean dftmode = DFTMODE, czero = CZERO, usehamming = USEHAMMING;

  FILE *outfp = tmpfile();

  fs *= 1000;                  /* kHz -> Hz */

  if (L < 0)
    for (L = 2; L <= l; L *= 2) {
    }
  if (wtype == 0)
    usehamming = 1 - usehamming;

  x = dgetmem(l + m + 2);
  mc = x + l;

  while (freadf(x, sizeof(*x), l, fp) == l) {

    mfcc(x, mc, fs, alpha, eps, l, L, m + 1, n, lift, dftmode, usehamming);
    if (!czero)
      mc[m] = mc[m + 1];
    fwritef(mc, sizeof(*mc), m + num, outfp);
  }

  
	return outfp;
}
