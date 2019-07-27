



































#ifndef FE_TYPE_H
#define FE_TYPE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sphinxbase/fe.h"
#include "sphinxbase/fixpoint.h"

#ifdef FIXED16

typedef int16 frame_t;
typedef int16 window_t;
typedef int32 powspec_t;
typedef struct { int16 r, i; } complex;
#elif defined(FIXED_POINT)
typedef fixed32 frame_t;
typedef int32 powspec_t;
typedef fixed32 window_t;
typedef struct { fixed32 r, i; } complex;
#else 
typedef float64 frame_t;
typedef float64 powspec_t;
typedef float64 window_t;
typedef struct { float64 r, i; } complex;
#endif 

#endif 
