


































#ifndef SFMT_PARAMS_H
#define SFMT_PARAMS_H

#if !defined(MEXP)
#ifdef __GNUC__
  #warning "MEXP is not defined. I assume MEXP is 19937."
#endif
  #define MEXP 19937
#endif








#define N (MEXP / 128 + 1)


#define N32 (N * 4)


#define N64 (N * 2)










































#if MEXP == 607
  #include "test/SFMT-params607.h"
#elif MEXP == 1279
  #include "test/SFMT-params1279.h"
#elif MEXP == 2281
  #include "test/SFMT-params2281.h"
#elif MEXP == 4253
  #include "test/SFMT-params4253.h"
#elif MEXP == 11213
  #include "test/SFMT-params11213.h"
#elif MEXP == 19937
  #include "test/SFMT-params19937.h"
#elif MEXP == 44497
  #include "test/SFMT-params44497.h"
#elif MEXP == 86243
  #include "test/SFMT-params86243.h"
#elif MEXP == 132049
  #include "test/SFMT-params132049.h"
#elif MEXP == 216091
  #include "test/SFMT-params216091.h"
#else
#ifdef __GNUC__
  #error "MEXP is not valid."
  #undef MEXP
#else
  #undef MEXP
#endif

#endif

#endif 
