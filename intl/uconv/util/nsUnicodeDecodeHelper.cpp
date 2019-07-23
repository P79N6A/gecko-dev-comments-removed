




































#include "pratom.h"
#include "unicpriv.h"
#include "nsIUnicodeDecoder.h"
#include "nsUnicodeDecodeHelper.h"



nsresult nsUnicodeDecodeHelper::ConvertByTable(
                                     const char * aSrc, 
                                     PRInt32 * aSrcLength, 
                                     PRUnichar * aDest, 
                                     PRInt32 * aDestLength, 
                                     uScanClassID aScanClass,
                                     uShiftInTable * aShiftInTable, 
                                     uMappingTable  * aMappingTable)
{
  const char * src = aSrc;
  PRInt32 srcLen = *aSrcLength;
  PRUnichar * dest = aDest;
  PRUnichar * destEnd = aDest + *aDestLength;

  PRUnichar med;
  PRInt32 bcr; 
  nsresult res = NS_OK;

  while ((srcLen > 0) && (dest < destEnd)) {
    PRBool charFound;
    if (aScanClass == uMultibytesCharset) {
      NS_ASSERTION(aShiftInTable, "shift table missing");
      charFound = uScanShift(aShiftInTable, NULL, (PRUint8 *)src,
                             reinterpret_cast<PRUint16*>(&med), srcLen, 
                             (PRUint32 *)&bcr);
    } else {
      charFound = uScan(aScanClass, NULL, (PRUint8 *)src,
                        reinterpret_cast<PRUint16*>(&med),
                        srcLen, (PRUint32 *)&bcr);
    }
    if (!charFound) {
      res = NS_OK_UDEC_MOREINPUT;
      break;
    }

    if (!uMapCode((uTable*) aMappingTable, static_cast<PRUint16>(med), reinterpret_cast<PRUint16*>(dest))) {
      if (med < 0x20) {
        
        *dest = med;
      } else {
        
        *dest = 0xfffd;
      }
    }

    src += bcr;
    srcLen -= bcr;
    dest++;
  }

  if ((srcLen > 0) && (res == NS_OK)) res = NS_OK_UDEC_MOREOUTPUT;

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

nsresult nsUnicodeDecodeHelper::ConvertByMultiTable(
                                     const char * aSrc, 
                                     PRInt32 * aSrcLength, 
                                     PRUnichar * aDest, 
                                     PRInt32 * aDestLength, 
                                     PRInt32 aTableCount, 
                                     const uRange * aRangeArray, 
                                     uScanClassID * aScanClassArray,
                                     uMappingTable ** aMappingTable)
{
  PRUint8 * src = (PRUint8 *)aSrc;
  PRInt32 srcLen = *aSrcLength;
  PRUnichar * dest = aDest;
  PRUnichar * destEnd = aDest + *aDestLength;

  PRUnichar med;
  PRInt32 bcr; 
  nsresult res = NS_OK;
  PRInt32 i;

  while ((srcLen > 0) && (dest < destEnd)) 
  {
    PRBool done= PR_FALSE;
    PRBool passRangeCheck = PR_FALSE;
    PRBool passScan = PR_FALSE;
    for (i=0; (!done) && (i<aTableCount); i++)  
    {
      if ((aRangeArray[i].min <= *src) && (*src <= aRangeArray[i].max)) 
      {
        passRangeCheck = PR_TRUE;
        if (uScan(aScanClassArray[i], NULL, src, 
                   reinterpret_cast<PRUint16*>(&med), srcLen, 
                   (PRUint32 *)&bcr)) 
        {
          passScan = PR_TRUE;
          done = uMapCode((uTable*) aMappingTable[i], 
                          static_cast<PRUint16>(med), 
                          reinterpret_cast<PRUint16*>(dest)); 
        } 
      } 
    } 

    if(passRangeCheck && (! passScan))
    {
      res = NS_OK_UDEC_MOREINPUT;
      break;
    }
    if(! done)
    {
      bcr = 1;
      if ((PRUint8)*src < 0x20) {
        
        *dest = *src;
      } else if(*src == (PRUint8) 0xa0) {
        
        *dest = 0x00a0;
      } else {
        
        for (i=0; i<aTableCount; i++)  
        {
          if ((aRangeArray[i].min <= *src) && (*src <= aRangeArray[i].max)) 
          {
            if (uScan(aScanClassArray[i], NULL, src, 
                   reinterpret_cast<PRUint16*>(&med), srcLen, 
                   (PRUint32*)&bcr)) 
            { 
               
              
               PRInt32 k; 
               for(k = 1; k < bcr; k++)
               {
                 if(0 == (src[k]  & 0x80))
                 { 
                   
                   bcr = 1;
                   break; 
                 }
               }
               break;
            }
          }
        }
        
        *dest = ((1==bcr)&&(*src == (PRUint8)0xa0 )) ? 0x00a0 : 0xfffd;
      }
    }

    src += bcr;
    srcLen -= bcr;
    dest++;
  } 

  if ((srcLen > 0) && (res == NS_OK)) res = NS_OK_UDEC_MOREOUTPUT;

  *aSrcLength = src - (PRUint8 *)aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

nsresult nsUnicodeDecodeHelper::ConvertByFastTable(
                                     const char * aSrc, 
                                     PRInt32 * aSrcLength, 
                                     PRUnichar * aDest, 
                                     PRInt32 * aDestLength, 
                                     const PRUnichar * aFastTable, 
                                     PRInt32 aTableSize)
{
  PRUint8 * src = (PRUint8 *)aSrc;
  PRUint8 * srcEnd = src;
  PRUnichar * dest = aDest;

  nsresult res;
  if (*aSrcLength > *aDestLength) {
    srcEnd += (*aDestLength);
    res = NS_PARTIAL_MORE_OUTPUT;
  } else {
    srcEnd += (*aSrcLength);
    res = NS_OK;
  }

  for (; src<srcEnd;) *dest++ = aFastTable[*src++];

  *aSrcLength = src - (PRUint8 *)aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

nsresult nsUnicodeDecodeHelper::CreateFastTable(
                                     uMappingTable  * aMappingTable,
                                     PRUnichar * aFastTable, 
                                     PRInt32 aTableSize)
{
  PRInt32 tableSize = aTableSize;
  PRInt32 buffSize = aTableSize;
  char * buff = new char [buffSize];
  if (buff == NULL) return NS_ERROR_OUT_OF_MEMORY;

  char * p = buff;
  for (PRInt32 i=0; i<aTableSize; i++) *(p++) = i;
  nsresult res = ConvertByTable(buff, &buffSize, aFastTable, &tableSize, 
      u1ByteCharset, nsnull, aMappingTable);

  delete [] buff;
  return res;
}

