





































#ifndef MOZCE_YMATH_H
#define MOZCE_YMATH_H
#include "mozce_ymath_actual_incl.h"
#define _FMAX	    ((1 << (15 - _FOFF)) - 1)
#define _FOFF	    7
#define _FSIGN    0x8000
#define INIT(w0)  {w0, 0}
static const _Dconst _FNanWinCE = {INIT((_FMAX << _FOFF)
  | (1 << (_FOFF - 1)))};
#define _FNan _FNanWinCE
#endif
