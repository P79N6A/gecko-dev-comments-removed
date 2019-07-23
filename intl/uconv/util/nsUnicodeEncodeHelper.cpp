




































#include "pratom.h"
#include "unicpriv.h"
#include "nsIUnicodeEncoder.h"
#include "nsUnicodeEncodeHelper.h"



nsresult nsUnicodeEncodeHelper::ConvertByTable(
                                     const PRUnichar * aSrc, 
                                     PRInt32 * aSrcLength, 
                                     char * aDest, 
                                     PRInt32 * aDestLength, 
                                     uScanClassID aScanClass,
                                     uShiftOutTable * aShiftOutTable,
                                     uMappingTable  * aMappingTable)
{
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  PRInt32 destLen = *aDestLength;

  PRUnichar med;
  PRInt32 bcw; 
  nsresult res = NS_OK;

  while (src < srcEnd) {
    if (!uMapCode((uTable*) aMappingTable, NS_STATIC_CAST(PRUnichar, *(src++)), NS_REINTERPRET_CAST(PRUint16*, &med))) {
      res = NS_ERROR_UENC_NOMAPPING;
      break;
    }

    PRBool charFound;
    if (aScanClass == uMultibytesCharset) {
      NS_ASSERTION(aShiftOutTable, "shift table missing");
      charFound = uGenerateShift(aShiftOutTable, 0, med,
                                 (PRUint8 *)dest, destLen, 
                                 (PRUint32 *)&bcw);
    } else {
      charFound = uGenerate(aScanClass, 0, med,
                            (PRUint8 *)dest, destLen, 
                            (PRUint32 *)&bcw);
    }
    if (!charFound) {
      src--;
      res = NS_OK_UENC_MOREOUTPUT;
      break;
    }

    dest += bcw;
    destLen -= bcw;
  }

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

nsresult nsUnicodeEncodeHelper::ConvertByMultiTable(
                                     const PRUnichar * aSrc, 
                                     PRInt32 * aSrcLength, 
                                     char * aDest, 
                                     PRInt32 * aDestLength, 
                                     PRInt32 aTableCount, 
                                     uScanClassID * aScanClassArray,
                                     uShiftOutTable ** aShiftOutTable, 
                                     uMappingTable  ** aMappingTable)
{
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  PRInt32 destLen = *aDestLength;

  PRUnichar med;
  PRInt32 bcw; 
  nsresult res = NS_OK;
  PRInt32 i;

  while (src < srcEnd) {
    for (i=0; i<aTableCount; i++) 
      if (uMapCode((uTable*) aMappingTable[i], NS_STATIC_CAST(PRUint16, *src), NS_REINTERPRET_CAST(PRUint16*, &med))) break;

    src++;
    if (i == aTableCount) {
      res = NS_ERROR_UENC_NOMAPPING;
      break;
    }

    PRBool charFound;
    if (aScanClassArray[i] == uMultibytesCharset) {
      NS_ASSERTION(aShiftOutTable[i], "shift table missing");
      charFound = uGenerateShift(aShiftOutTable[i], 0, med,
                                 (PRUint8 *)dest, destLen,
                                 (PRUint32 *)&bcw);
    }
    else
      charFound = uGenerate(aScanClassArray[i], 0, med,
                            (PRUint8 *)dest, destLen, 
                            (PRUint32 *)&bcw);
    if (!charFound) { 
      src--;
      res = NS_OK_UENC_MOREOUTPUT;
      break;
    }

    dest += bcw;
    destLen -= bcw;
  }

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

nsresult nsUnicodeEncodeHelper::FillInfo(PRUint32 *aInfo, uMappingTable  * aMappingTable)
{
   uFillInfo((uTable*) aMappingTable, aInfo);
   return NS_OK;
}

nsresult nsUnicodeEncodeHelper::FillInfo(PRUint32 *aInfo, PRInt32 aTableCount, uMappingTable  ** aMappingTable)
{
   for (PRInt32 i=0; i<aTableCount; i++) 
      uFillInfo((uTable*) aMappingTable[i], aInfo);
   return NS_OK;
}
