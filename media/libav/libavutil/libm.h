






















#ifndef AVUTIL_LIBM_H
#define AVUTIL_LIBM_H

#include <math.h>
#include "config.h"
#include "attributes.h"
#include "intfloat.h"

#if !HAVE_ATANF
#undef atanf
#define atanf(x) ((float)atan(x))
#endif

#if !HAVE_ATAN2F
#undef atan2f
#define atan2f(y, x) ((float)atan2(y, x))
#endif

#if !HAVE_POWF
#undef powf
#define powf(x, y) ((float)pow(x, y))
#endif

#if !HAVE_CBRTF
static av_always_inline float cbrtf(float x)
{
    return x < 0 ? -powf(-x, 1.0 / 3.0) : powf(x, 1.0 / 3.0);
}
#endif

#if !HAVE_COSF
#undef cosf
#define cosf(x) ((float)cos(x))
#endif

#if !HAVE_EXPF
#undef expf
#define expf(x) ((float)exp(x))
#endif

#if !HAVE_EXP2
#undef exp2
#define exp2(x) exp((x) * 0.693147180559945)
#endif 

#if !HAVE_EXP2F
#undef exp2f
#define exp2f(x) ((float)exp2(x))
#endif 

#if !HAVE_ISINF
static av_always_inline av_const int isinf(float x)
{
    uint32_t v = av_float2int(x);
    if ((v & 0x7f800000) != 0x7f800000)
        return 0;
    return !(v & 0x007fffff);
}
#endif 

#if !HAVE_ISNAN
static av_always_inline av_const int isnan(float x)
{
    uint32_t v = av_float2int(x);
    if ((v & 0x7f800000) != 0x7f800000)
        return 0;
    return v & 0x007fffff;
}
#endif 

#if !HAVE_LDEXPF
#undef ldexpf
#define ldexpf(x, exp) ((float)ldexp(x, exp))
#endif

#if !HAVE_LLRINT
#undef llrint
#define llrint(x) ((long long)rint(x))
#endif 

#if !HAVE_LLRINTF
#undef llrintf
#define llrintf(x) ((long long)rint(x))
#endif 

#if !HAVE_LOG2
#undef log2
#define log2(x) (log(x) * 1.44269504088896340736)
#endif 

#if !HAVE_LOG2F
#undef log2f
#define log2f(x) ((float)log2(x))
#endif 

#if !HAVE_LOG10F
#undef log10f
#define log10f(x) ((float)log10(x))
#endif

#if !HAVE_SINF
#undef sinf
#define sinf(x) ((float)sin(x))
#endif

#if !HAVE_RINT
static inline double rint(double x)
{
    return x >= 0 ? floor(x + 0.5) : ceil(x - 0.5);
}
#endif 

#if !HAVE_LRINT
static av_always_inline av_const long int lrint(double x)
{
    return rint(x);
}
#endif 

#if !HAVE_LRINTF
static av_always_inline av_const long int lrintf(float x)
{
    return (int)(rint(x));
}
#endif 

#if !HAVE_ROUND
static av_always_inline av_const double round(double x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}
#endif 

#if !HAVE_ROUNDF
static av_always_inline av_const float roundf(float x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}
#endif 

#if !HAVE_TRUNC
static av_always_inline av_const double trunc(double x)
{
    return (x > 0) ? floor(x) : ceil(x);
}
#endif 

#if !HAVE_TRUNCF
static av_always_inline av_const float truncf(float x)
{
    return (x > 0) ? floor(x) : ceil(x);
}
#endif 

#endif 
