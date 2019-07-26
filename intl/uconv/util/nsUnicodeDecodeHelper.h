



#ifndef nsUnicodeDecodeHelper_h__
#define nsUnicodeDecodeHelper_h__

#include "nsIUnicodeDecoder.h"
#include "uconvutil.h"








class nsUnicodeDecodeHelper
{
public:
  


  static nsresult ConvertByTable(const char * aSrc, int32_t * aSrcLength, 
                                 PRUnichar * aDest, int32_t * aDestLength,
                                 uScanClassID aScanClass,
                                 uShiftInTable * aShiftInTable,
                                 uMappingTable  * aMappingTable,
                                 bool aErrorSignal = false);

  


  static nsresult ConvertByMultiTable(const char * aSrc, int32_t * aSrcLength,
      PRUnichar * aDest, int32_t * aDestLength, int32_t aTableCount, 
      const uRange * aRangeArray, uScanClassID * aScanClassArray,
      uMappingTable ** aMappingTable, bool aErrorSignal = false);

  


  static nsresult ConvertByFastTable(const char * aSrc, int32_t * aSrcLength, 
      PRUnichar * aDest, int32_t * aDestLength, const PRUnichar * aFastTable, 
      int32_t aTableSize, bool aErrorSignal);

  


  static nsresult CreateFastTable(uMappingTable * aMappingTable,
      PRUnichar * aFastTable,  int32_t aTableSize);
};

#endif 


