

















#ifndef __UCOL_SWP_H__
#define __UCOL_SWP_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "udataswp.h"





U_INTERNAL UBool U_EXPORT2
ucol_looksLikeCollationBinary(const UDataSwapper *ds,
                              const void *inData, int32_t length);





U_CAPI int32_t U_EXPORT2
ucol_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode);





U_CAPI int32_t U_EXPORT2
ucol_swapInverseUCA(const UDataSwapper *ds,
                    const void *inData, int32_t length, void *outData,
                    UErrorCode *pErrorCode);

#endif 

#endif
