




































#include "nsUnicodeToUCS2BE.h"
#include <string.h>

inline static void SwapBytes(char *aDest, const PRUnichar* aSrc, PRInt32 aLen);

NS_IMETHODIMP nsUnicodeToUTF16BE::Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength)
{
  PRInt32 srcInLen = *aSrcLength;
  PRInt32 destInLen = *aDestLength;
  PRInt32 srcOutLen = 0;
  PRInt32 destOutLen = 0;
  PRInt32 copyCharLen;
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

NS_IMETHODIMP nsUnicodeToUTF16BE::GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength)
{
  if(0 != mBOM)
    *aDestLength = 2*(aSrcLength+1);
  else 
    *aDestLength = 2*aSrcLength;
  return NS_OK_UENC_EXACTLENGTH;
}

NS_IMETHODIMP nsUnicodeToUTF16BE::Finish(char * aDest, PRInt32 * aDestLength)
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

NS_IMETHODIMP nsUnicodeToUTF16BE::SetOutputErrorBehavior(PRInt32 aBehavior, 
      nsIUnicharEncoder * aEncoder, PRUnichar aChar)
{
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF16BE::FillInfo(PRUint32 *aInfo)
{
  ::memset(aInfo, 0xFF, (0x10000L >> 3));
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF16BE::CopyData(char* aDest, const PRUnichar* aSrc, PRInt32 aLen  )
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

NS_IMETHODIMP nsUnicodeToUTF16LE::CopyData(char* aDest, const PRUnichar* aSrc, PRInt32 aLen  )
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

inline void SwapBytes(char *aDest, const PRUnichar* aSrc, PRInt32 aLen)
{
  PRUnichar *p = (PRUnichar*) aDest;
  
  for(PRInt32 i = 0; i < aLen; i++)
  {
    PRUnichar aChar = *aSrc++;
    *p++ = (0x00FF & (aChar >> 8)) | (0xFF00 & (aChar << 8));
  }
}

