



#ifndef nsUnicodeEncodeHelper_h__
#define nsUnicodeEncodeHelper_h__

#include "nsError.h"
#include "uconvutil.h"








class nsUnicodeEncodeHelper
{

public:
  

  


  static nsresult ConvertByTable(const char16_t * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength, uScanClassID aScanClass,
      uShiftOutTable * aShiftOutTable, uMappingTable  * aMappingTable);

  


  static nsresult ConvertByMultiTable(const char16_t * aSrc, int32_t * aSrcLength,
      char * aDest, int32_t * aDestLength, int32_t aTableCount, 
      uScanClassID * aScanClassArray, 
      uShiftOutTable ** aShiftOutTable, uMappingTable  ** aMappingTable);
};

#endif 


