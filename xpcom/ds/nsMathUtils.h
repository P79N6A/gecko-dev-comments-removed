




































#ifndef nsMathUtils_h__
#define nsMathUtils_h__

#include "nscore.h"
#include <math.h>
#include <float.h>




inline NS_HIDDEN_(double) NS_round(double x)
{
    return x >= 0.0 ? floor(x + 0.5) : ceil(x - 0.5);
}
inline NS_HIDDEN_(float) NS_roundf(float x)
{
    return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}
inline NS_HIDDEN_(PRInt32) NS_lround(double x)
{
    return x >= 0.0 ? PRInt32(x + 0.5) : PRInt32(x - 0.5);
}
inline NS_HIDDEN_(PRInt32) NS_lroundf(float x)
{
    return x >= 0.0f ? PRInt32(x + 0.5f) : PRInt32(x - 0.5f);
}




inline NS_HIDDEN_(double) NS_ceil(double x)
{
    return ceil(x);
}
inline NS_HIDDEN_(float) NS_ceilf(float x)
{
    return ceilf(x);
}




inline NS_HIDDEN_(double) NS_floor(double x)
{
    return floor(x);
}
inline NS_HIDDEN_(float) NS_floorf(float x)
{
    return floorf(x);
}

#endif
