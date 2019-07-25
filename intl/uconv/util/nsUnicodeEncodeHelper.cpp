




#include "pratom.h"
#include "unicpriv.h"
#include "nsIUnicodeEncoder.h"
#include "nsUnicodeEncodeHelper.h"



nsresult nsUnicodeEncodeHelper::ConvertByTable(
                                     const PRUnichar * aSrc, 
                                     int32_t * aSrcLength, 
                                     char * aDest, 
                                     int32_t * aDestLength, 
                                     uScanClassID aScanClass,
                                     uShiftOutTable * aShiftOutTable,
                                     uMappingTable  * aMappingTable)
{
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  int32_t destLen = *aDestLength;

  PRUnichar med;
  int32_t bcw; 
  nsresult res = NS_OK;

  while (src < srcEnd) {
    if (!uMapCode((uTable*) aMappingTable, static_cast<PRUnichar>(*(src++)), reinterpret_cast<uint16_t*>(&med))) {
      if (aScanClass == u1ByteCharset && *(src - 1) < 0x20) {
        
        med = *(src - 1);
      } else {
        res = NS_ERROR_UENC_NOMAPPING;
        break;
      }
    }

    bool charFound;
    if (aScanClass == uMultibytesCharset) {
      NS_ASSERTION(aShiftOutTable, "shift table missing");
      charFound = uGenerateShift(aShiftOutTable, 0, med,
                                 (uint8_t *)dest, destLen, 
                                 (uint32_t *)&bcw);
    } else {
      charFound = uGenerate(aScanClass, 0, med,
                            (uint8_t *)dest, destLen, 
                            (uint32_t *)&bcw);
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
                                     int32_t * aSrcLength, 
                                     char * aDest, 
                                     int32_t * aDestLength, 
                                     int32_t aTableCount, 
                                     uScanClassID * aScanClassArray,
                                     uShiftOutTable ** aShiftOutTable, 
                                     uMappingTable  ** aMappingTable)
{
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  int32_t destLen = *aDestLength;

  PRUnichar med;
  int32_t bcw; 
  nsresult res = NS_OK;
  int32_t i;

  while (src < srcEnd) {
    for (i=0; i<aTableCount; i++) 
      if (uMapCode((uTable*) aMappingTable[i], static_cast<uint16_t>(*src), reinterpret_cast<uint16_t*>(&med))) break;

    src++;
    if (i == aTableCount) {
      res = NS_ERROR_UENC_NOMAPPING;
      break;
    }

    bool charFound;
    if (aScanClassArray[i] == uMultibytesCharset) {
      NS_ASSERTION(aShiftOutTable[i], "shift table missing");
      charFound = uGenerateShift(aShiftOutTable[i], 0, med,
                                 (uint8_t *)dest, destLen,
                                 (uint32_t *)&bcw);
    }
    else
      charFound = uGenerate(aScanClassArray[i], 0, med,
                            (uint8_t *)dest, destLen, 
                            (uint32_t *)&bcw);
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
