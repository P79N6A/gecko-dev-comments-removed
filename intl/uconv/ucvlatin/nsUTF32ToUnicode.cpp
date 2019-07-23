







































#include "nsUCSupport.h"
#include "nsUTF32ToUnicode.h"
#include "nsCharTraits.h"
#include <string.h>




#ifdef IS_BIG_ENDIAN
#define LE_STRING_TO_UCS4(s)                                       \
        (PRUint8(*(s)) | (PRUint8(*((s) + 1)) << 8) |              \
         (PRUint8(*((s) + 2)) << 16) | (PRUint8(*((s) + 3)) << 24))
#else
#define LE_STRING_TO_UCS4(s) (*(PRUint32*) (s))
#endif

#ifdef IS_BIG_ENDIAN
#define BE_STRING_TO_UCS4(s) (*(PRUint32*) (s))
#else
#define BE_STRING_TO_UCS4(s)                                       \
        (PRUint8(*((s) + 3)) | (PRUint8(*((s) + 2)) << 8) |         \
         (PRUint8(*((s) + 1)) << 16) | (PRUint8(*(s)) << 24))
#endif
 
static nsresult ConvertCommon(const char * aSrc, 
                              PRInt32 * aSrcLength, 
                              PRUnichar * aDest, 
                              PRInt32 * aDestLength,
                              PRUint16 * aState,
                              PRUint8  * aBuffer,
                              PRBool aIsLE)
{
   
  NS_ENSURE_TRUE(*aState < 4, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(*aDestLength > 0, NS_ERROR_INVALID_ARG);

  const char *src = aSrc;
  const char *srcEnd = aSrc + *aSrcLength;
   
  PRUnichar *dest = aDest;
  PRUnichar *destEnd = aDest + *aDestLength;

  if (*aState > *aSrcLength) 
  {
    memcpy(aBuffer + 4 - *aState, src, *aSrcLength);
    *aDestLength = 0;
    *aState -= *aSrcLength;
    return NS_OK_UDEC_MOREINPUT;
  }

  PRUint32 ucs4;

  
  if (*aState > 0)
  {
    memcpy(aBuffer + 4 - *aState, src, *aState);
    ucs4 =  aIsLE ? LE_STRING_TO_UCS4(aBuffer) : BE_STRING_TO_UCS4(aBuffer); 
    if (ucs4 < 0x10000L)  
    {
      *dest++= IS_SURROGATE(ucs4) ? UCS2_REPLACEMENT_CHAR : PRUnichar(ucs4);
    }
    else if (ucs4 < 0x110000L)  
    {
      if (destEnd - dest < 2) 
      {
        *aSrcLength = 0;
        *aDestLength = 0;
        return NS_OK_UDEC_MOREOUTPUT;
      }
      *dest++= H_SURROGATE(ucs4);
      *dest++= L_SURROGATE(ucs4);
    }       
    
    
    
    
    
    
    else                   
      *dest++ = UCS2_REPLACEMENT_CHAR;
    src += *aState;
    *aState = 0;
  }

  nsresult rv = NS_OK;  

  for ( ; src < srcEnd && dest < destEnd; src += 4)
  {
    if (srcEnd - src < 4) 
    {
      
      memcpy(aBuffer, src, srcEnd - src);
      *aState = 4 - (srcEnd - src); 
      src = srcEnd;
      rv = NS_OK_UDEC_MOREINPUT;
      break;
    }

    ucs4 =  aIsLE ? LE_STRING_TO_UCS4(src) : BE_STRING_TO_UCS4(src); 
    if (ucs4 < 0x10000L)  
    {
      *dest++= IS_SURROGATE(ucs4) ? UCS2_REPLACEMENT_CHAR : PRUnichar(ucs4);
    }
    else if (ucs4 < 0x110000L)  
    {
      if (destEnd - dest < 2) 
        break;
      
      *dest++= H_SURROGATE(ucs4);
      *dest++= L_SURROGATE(ucs4);
    }       
    else                       
      *dest++ = UCS2_REPLACEMENT_CHAR;
  }

  
  if((NS_OK == rv) && (src < srcEnd) && (dest >= destEnd)) 
    rv = NS_OK_UDEC_MOREOUTPUT;

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;

  return rv;
}





nsUTF32ToUnicode::nsUTF32ToUnicode() : nsBasicDecoderSupport()
{
  Reset();
}




NS_IMETHODIMP nsUTF32ToUnicode::GetMaxLength(const char * aSrc, 
                                            PRInt32 aSrcLength, 
                                            PRInt32 * aDestLength)
{
  
  
  *aDestLength = aSrcLength / 2;
  return NS_OK;
}





NS_IMETHODIMP nsUTF32ToUnicode::Reset()
{
  
  mState = 0;  
  memset(mBufferInc, 0, 4);
  return NS_OK;

}








NS_IMETHODIMP nsUTF32BEToUnicode::Convert(const char * aSrc, 
                                          PRInt32 * aSrcLength, 
                                          PRUnichar * aDest, 
                                          PRInt32 * aDestLength)
{
  return ConvertCommon(aSrc, aSrcLength, aDest, aDestLength, &mState, 
                       mBufferInc, PR_FALSE);
}



  



NS_IMETHODIMP nsUTF32LEToUnicode::Convert(const char * aSrc, 
                                          PRInt32 * aSrcLength, 
                                          PRUnichar * aDest, 
                                          PRInt32 * aDestLength)
{
  return ConvertCommon(aSrc, aSrcLength, aDest, aDestLength, &mState, 
                       mBufferInc, PR_TRUE);
}


  
