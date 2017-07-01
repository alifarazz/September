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

/************************************************************************
*                                                                       *
*    Extract Frame from Data Sequence                                   *
*                                                                       *
*                                       1985.11 K.Tokuda                *
*                                       1996.4  K.Koishida              *
*                                                                       *
*       usage:                                                          *
*               frame [ options ] [ infile ] > stdout                   *
*       options:                                                        *
*               -l l     :  frame length                [256]           *
*               -p p     :  frame period                [100]           *
*               -n       :  no center start point       [FALSE]         *
*       infile:                                                         *
*               data sequence                                           *
*                   , x(0), x(1), ...,                                  *
*       stdout:                                                         *
*               frame sequence                                          *
*                   0, 0, ..., 0, x(0), x(1), ..., x(l/2),              *
*                   , x(t), x(t+1),       ...,       x(t+l-1),          *
*                   , x(2t), x(2t+1),     ....                          *
*              if -n specified                                          *
*                   x(0), x(1),           ...,       x(l),              *
*                   , x(t), x(t+1),       ...,       x(t+l-1),          *
*                   , x(2t), x(2t+1),     ....                          *
*                                                                       *
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SPTK/SPTK.h>
/* #ifdef HAVE_STRING_H */
/* #include <string.h> */
/* #else */
/* #include <strings.h> */
/* #ifndef HAVE_STRRCHR */
/* #define strrchr rindex */
/* #endif */
/* #endif */

/* #if defined(WIN32) */
/* #include "SPTK.h" */
/* #else */
/* #include <SPTK/SPTK.h> */
/* #endif */


/*  Default Values  */
#define LENG 400
#define FPERIOD 160
#define NOCTR FA

static char *BOOL[] = { "FALSE", "TRUE" };


FILE* frameFromX2x(FILE *fp)
{
  int l = LENG, fprd = FPERIOD, ns, i, rnum, ts, cs;
  Boolean noctr = NOCTR;
  double *x, *xx, *p1, *p2, *p;
  char *s, c;
  /* FILE *fp = fopen("model.float", "rb"); */
  FILE *outfp = tmpfile();

  x = dgetmem(l);
  if (!noctr) {
    i = (int) ((l + 1) / 2);
    rnum = freadf(&x[(int) (l / 2)], sizeof(*x), i, fp);
  } else
    rnum = freadf(x, sizeof(*x), l, fp);
  if (rnum == 0)
    return 0;
  /* break; */
  cs = rnum;
  fwritef(x, sizeof(*x), l, outfp);

  if ((ns = (l - fprd)) > 0) {
    p = &x[fprd];
    for (;;) {
      p1 = x;
      p2 = p;
      i = ns;
      while (i--) {
        *p1++ = *p2++;
      }
      rnum = freadf(p1, sizeof(*p1), fprd, fp);
      if (rnum < fprd) {
        ts = fprd - rnum;
        cs -= ts;
        while (rnum--)
          p1++;
        while (ts--)
          *p1++ = 0.0;
      }
      if (cs <= 0)
        break;
      fwritef(x, sizeof(*x), l, outfp);
    }
  } else {
    i = -ns;
    xx = dgetmem(i);
    for (;;) {
      if (freadf(xx, sizeof(*xx), i, fp) != i)
        break;
      rnum = freadf(x, sizeof(*x), l, fp);
      if (rnum < l) {
        if (rnum == 0)
          break;
        ts = l - rnum;
        p1 = x;
        while (rnum--)
          p1++;
        while (ts--)
          *p1++ = 0.0;
      }
      fwritef(x, sizeof(*x), l, outfp);
    }
  }

  
  return outfp;
}
