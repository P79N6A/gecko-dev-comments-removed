



































#ifndef nsUnicodeEncodeHelper_h__
#define nsUnicodeEncodeHelper_h__

#include "nsIUnicodeEncoder.h"
#include "uconvutil.h"








class nsUnicodeEncodeHelper
{

public:
  

  


  static nsresult ConvertByTable(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength, uScanClassID aScanClass,
      uShiftOutTable * aShiftOutTable, uMappingTable  * aMappingTable);

  


  static nsresult ConvertByMultiTable(const PRUnichar * aSrc, PRInt32 * aSrcLength,
      char * aDest, PRInt32 * aDestLength, PRInt32 aTableCount, 
      uScanClassID * aScanClassArray, 
      uShiftOutTable ** aShiftOutTable, uMappingTable  ** aMappingTable);

  


  static nsresult FillInfo(PRUint32* aInfo, uMappingTable  * aMappingTable);
  static nsresult FillInfo(PRUint32* aInfo, PRInt32 aTableCount, uMappingTable  ** aMappingTable);
};

#endif 


