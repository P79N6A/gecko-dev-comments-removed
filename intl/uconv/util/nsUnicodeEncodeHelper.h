



#ifndef nsUnicodeEncodeHelper_h__
#define nsUnicodeEncodeHelper_h__

#include "nsIUnicodeEncoder.h"
#include "uconvutil.h"








class nsUnicodeEncodeHelper
{

public:
  

  


  static nsresult ConvertByTable(const PRUnichar * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength, uScanClassID aScanClass,
      uShiftOutTable * aShiftOutTable, uMappingTable  * aMappingTable);

  


  static nsresult ConvertByMultiTable(const PRUnichar * aSrc, int32_t * aSrcLength,
      char * aDest, int32_t * aDestLength, int32_t aTableCount, 
      uScanClassID * aScanClassArray, 
      uShiftOutTable ** aShiftOutTable, uMappingTable  ** aMappingTable);
};

#endif 


