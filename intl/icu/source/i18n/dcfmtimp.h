





#ifndef DCFMTIMP_H
#define DCFMTIMP_H

#include "unicode/utypes.h"


#if UCONFIG_FORMAT_FASTPATHS_49

U_NAMESPACE_BEGIN

enum EDecimalFormatFastpathStatus {
  kFastpathNO = 0,
  kFastpathYES = 1,
  kFastpathUNKNOWN = 2 
};




struct DecimalFormatInternal {
  uint8_t    fFastFormatStatus;
  uint8_t    fFastParseStatus;
  
#ifdef FMT_DEBUG
  void dump() const {
    printf("DecimalFormatInternal: fFastFormatStatus=%c, fFastParseStatus=%c\n",
           "NY?"[(int)fFastFormatStatus&3],
           "NY?"[(int)fFastParseStatus&3]
           );
  }
#endif  
};



U_NAMESPACE_END

#endif

#endif
