













#ifndef UMISC_H
#define UMISC_H

#include "unicode/utypes.h"








U_CDECL_BEGIN




typedef struct UFieldPosition {
  



  int32_t field;
  



  int32_t beginIndex;
  



  int32_t endIndex;
} UFieldPosition;

#if !UCONFIG_NO_SERVICE




typedef const void* URegistryKey;
#endif

U_CDECL_END

#endif
