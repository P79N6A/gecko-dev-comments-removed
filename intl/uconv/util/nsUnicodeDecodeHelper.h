



































#ifndef nsUnicodeDecodeHelper_h__
#define nsUnicodeDecodeHelper_h__

#include "nsIUnicodeDecoder.h"
#include "uconvutil.h"








class nsUnicodeDecodeHelper
{
public:
  


  static nsresult ConvertByTable(const char * aSrc, PRInt32 * aSrcLength, 
                                 PRUnichar * aDest, PRInt32 * aDestLength,
                                 uScanClassID aScanClass,
                                 uShiftInTable * aShiftInTable,
                                 uMappingTable  * aMappingTable);

  


  static nsresult ConvertByMultiTable(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength, PRInt32 aTableCount, 
      const uRange * aRangeArray, uScanClassID * aScanClassArray,
      uMappingTable ** aMappingTable);

  


  static nsresult ConvertByFastTable(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength, const PRUnichar * aFastTable, 
      PRInt32 aTableSize);

  


  static nsresult CreateFastTable(uMappingTable * aMappingTable,
      PRUnichar * aFastTable,  PRInt32 aTableSize);
};

#endif 


