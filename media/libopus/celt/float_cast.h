




































#ifndef FLOAT_CAST_H
#define FLOAT_CAST_H


#include "arch.h"






























#if (HAVE_LRINTF)








#define _ISOC9X_SOURCE 1
#define _ISOC99_SOURCE 1

#define __USE_ISOC9X 1
#define __USE_ISOC99 1

#include <math.h>
#define float2int(x) lrintf(x)

#elif (defined(HAVE_LRINT))

#define _ISOC9X_SOURCE 1
#define _ISOC99_SOURCE 1

#define __USE_ISOC9X 1
#define __USE_ISOC99 1

#include <math.h>
#define float2int(x) lrint(x)

#elif (defined (WIN64) || defined (_WIN64))
        #include <xmmintrin.h>

        __inline long int float2int(float value)
        {
                return _mm_cvtss_si32(_mm_load_ss(&value));
        }
#elif (defined (WIN32) || defined (_WIN32))
        #include <math.h>

        



        __inline long int
        float2int (float flt)
        {       int intgr;

                _asm
                {       fld flt
                        fistp intgr
                } ;

                return intgr ;
        }

#else

#if (defined(__GNUC__) && defined(__STDC__) && __STDC__ && __STDC_VERSION__ >= 199901L)
        
        #warning "Don't have the functions lrint() and lrintf ()."
        #warning "Replacing these functions with a standard C cast."
#endif 
        #include <math.h>
        #define float2int(flt) ((int)(floor(.5+flt)))
#endif

static inline opus_int16 FLOAT2INT16(float x)
{
   x = x*CELT_SIG_SCALE;
   x = MAX32(x, -32768);
   x = MIN32(x, 32767);
   return (opus_int16)float2int(x);
}

#endif
