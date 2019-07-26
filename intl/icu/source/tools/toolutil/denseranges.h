















#ifndef __DENSERANGES_H__
#define __DENSERANGES_H__

#include "unicode/utypes.h"













U_CAPI int32_t U_EXPORT2
uprv_makeDenseRanges(const int32_t values[], int32_t length,
                     int32_t density,
                     int32_t ranges[][2], int32_t capacity);

#endif  
