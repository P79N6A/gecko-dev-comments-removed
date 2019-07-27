



















#ifndef AVUTIL_INTFLOAT_H
#define AVUTIL_INTFLOAT_H

#include <stdint.h>
#include "attributes.h"

union av_intfloat32 {
    uint32_t i;
    float    f;
};

union av_intfloat64 {
    uint64_t i;
    double   f;
};




static av_always_inline float av_int2float(uint32_t i)
{
    union av_intfloat32 v;
    v.i = i;
    return v.f;
}




static av_always_inline uint32_t av_float2int(float f)
{
    union av_intfloat32 v;
    v.f = f;
    return v.i;
}




static av_always_inline double av_int2double(uint64_t i)
{
    union av_intfloat64 v;
    v.i = i;
    return v.f;
}




static av_always_inline uint64_t av_double2int(double f)
{
    union av_intfloat64 v;
    v.f = f;
    return v.i;
}

#endif 
