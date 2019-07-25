




#include "nsUnicodeToUCS2BE.h"
#include <string.h>

inline static void SwapBytes(char *aDest, const PRUnichar* aSrc, int32_t aLen);

NS_IMETHODIMP nsUnicodeToUTF16BE::Convert(const PRUnichar * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength)
{
  int32_t srcInLen = *aSrcLength;
  int32_t destInLen = *aDestLength;
  int32_t srcOutLen = 0;
  int32_t destOutLen = 0;
  int32_t copyCharLen;
  PRUnichar *p = (PRUnichar*)aDest;
 
  
  if(0!=mBOM)
  {
     if(destInLen <2)
        goto needmoreoutput;
  
     *p++ = mBOM;
     mBOM = 0;
     destOutLen +=2;
  }
  

  copyCharLen = srcInLen;
  if(copyCharLen > (destInLen - destOutLen) / 2) {
     copyCharLen = (destInLen - destOutLen) / 2;
  }

  
  CopyData((char*)p , aSrc, copyCharLen );

  srcOutLen += copyCharLen;
  destOutLen += copyCharLen * 2;
  if(copyCharLen < srcInLen)
      goto needmoreoutput;
  
  *aSrcLength = srcOutLen;
  *aDestLength = destOutLen;
  return NS_OK;

needmoreoutput:
  *aSrcLength = srcOutLen;
  *aDestLength = destOutLen;
  return NS_OK_UENC_MOREOUTPUT;
}

NS_IMETHODIMP nsUnicodeToUTF16BE::GetMaxLength(const PRUnichar * aSrc, int32_t aSrcLength, 
      int32_t * aDestLength)
{
  if(0 != mBOM)
    *aDestLength = 2*(aSrcLength+1);
  else 
    *aDestLength = 2*aSrcLength;
  return NS_OK_UENC_EXACTLENGTH;
}

NS_IMETHODIMP nsUnicodeToUTF16BE::Finish(char * aDest, int32_t * aDestLength)
{
  if(0 != mBOM)
  {
     if(*aDestLength >= 2)
     {
        *((PRUnichar*)aDest)= mBOM;
        mBOM=0;
        *aDestLength = 2;
        return NS_OK;  
     } else {
        *aDestLength = 0;
        return NS_OK;  
     }
  } else { 
     *aDestLength = 0;
     return NS_OK;
  } 
}

NS_IMETHODIMP nsUnicodeToUTF16BE::Reset()
{
  mBOM = 0;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF16BE::SetOutputErrorBehavior(int32_t aBehavior, 
      nsIUnicharEncoder * aEncoder, PRUnichar aChar)
{
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF16BE::CopyData(char* aDest, const PRUnichar* aSrc, int32_t aLen  )
{
#ifdef IS_BIG_ENDIAN
  
  ::memcpy(aDest, (void*) aSrc, aLen * 2);
#elif defined(IS_LITTLE_ENDIAN)
  
  SwapBytes(aDest, aSrc, aLen);
#else
  #error "Unknown endianness"
#endif
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF16LE::CopyData(char* aDest, const PRUnichar* aSrc, int32_t aLen  )
{
#ifdef IS_LITTLE_ENDIAN
  
  ::memcpy(aDest, (void*) aSrc, aLen * 2);
#elif defined(IS_BIG_ENDIAN)
  
  SwapBytes(aDest, aSrc, aLen);
#else
  #error "Unknown endianness"
#endif
  return NS_OK;
}

inline void SwapBytes(char *aDest, const PRUnichar* aSrc, int32_t aLen)
{
  PRUnichar *p = (PRUnichar*) aDest;
  
  for(int32_t i = 0; i < aLen; i++)
  {
    PRUnichar aChar = *aSrc++;
    *p++ = (0x00FF & (aChar >> 8)) | (0xFF00 & (aChar << 8));
  }
}

