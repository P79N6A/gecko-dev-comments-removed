






































#include "nsUCSupport.h"
#include "nsUnicodeToUTF8.h"
#include <string.h>

NS_IMPL_ISUPPORTS1(nsUnicodeToUTF8, nsIUnicodeEncoder)




NS_IMETHODIMP nsUnicodeToUTF8::GetMaxLength(const PRUnichar * aSrc, 
                                              PRInt32 aSrcLength,
                                              PRInt32 * aDestLength)
{
  
  
  
  
  
  *aDestLength = 3*aSrcLength + 3;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF8::FillInfo(PRUint32 *aInfo)
{
  memset(aInfo, 0xFF, (0x10000L >> 3));
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF8::Convert(const PRUnichar * aSrc, 
                                PRInt32 * aSrcLength, 
                                char * aDest, 
                                PRInt32 * aDestLength)
{
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  PRInt32 destLen = *aDestLength;
  PRUint32 n;

  
  if (mHighSurrogate) {
    if (src < srcEnd) {
      *aDestLength = 0;
      return NS_OK_UENC_MOREINPUT;
    }
    if (*aDestLength < 4) {
      *aSrcLength = 0;
      *aDestLength = 0;
      return NS_OK_UENC_MOREOUTPUT;
    }
    if (*src < (PRUnichar)0xdc00 || *src > (PRUnichar)0xdfff) { 
      *dest++ = (char)0xe0 | (mHighSurrogate >> 12);
      *dest++ = (char)0x80 | ((mHighSurrogate >> 6) & 0x003f);
      *dest++ = (char)0x80 | (mHighSurrogate & 0x003f);
      destLen -= 3;
    } else { 
      n = ((mHighSurrogate - (PRUnichar)0xd800) << 10) + 
              (*src - (PRUnichar)0xdc00) + 0x10000;
      *dest++ = (char)0xf0 | (n >> 18);
      *dest++ = (char)0x80 | ((n >> 12) & 0x3f);
      *dest++ = (char)0x80 | ((n >> 6) & 0x3f);
      *dest++ = (char)0x80 | (n & 0x3f);
      ++src;
      destLen -= 4;
    }
    mHighSurrogate = 0;
  }

  while (src < srcEnd) {
    if ( *src <= 0x007f) {
      if (destLen < 1)
        goto error_more_output;
      *dest++ = (char)*src;
      --destLen;
    } else if (*src <= 0x07ff) {
      if (destLen < 2)
        goto error_more_output;
      *dest++ = (char)0xc0 | (*src >> 6);
      *dest++ = (char)0x80 | (*src & 0x003f);
      destLen -= 2;
    } else if (*src >= (PRUnichar)0xD800 && *src < (PRUnichar)0xDC00) {
      if ((src+1) >= srcEnd) {
        
        mHighSurrogate = *src;
        *aDestLength = dest - aDest;
        return NS_OK_UENC_MOREINPUT;
      }
      
      if (destLen < 4)
        goto error_more_output;
      if (*(src+1) < (PRUnichar)0xdc00 || *(src+1) > 0xdfff) { 
        *dest++ = (char)0xe0 | (*src >> 12);
        *dest++ = (char)0x80 | ((*src >> 6) & 0x003f);
        *dest++ = (char)0x80 | (*src & 0x003f);
        destLen -= 3;
      } else {
        n = ((*src - (PRUnichar)0xd800) << 10) + (*(src+1) - (PRUnichar)0xdc00) + (PRUint32)0x10000;
        *dest++ = (char)0xf0 | (n >> 18);
        *dest++ = (char)0x80 | ((n >> 12) & 0x3f);
        *dest++ = (char)0x80 | ((n >> 6) & 0x3f);
        *dest++ = (char)0x80 | (n & 0x3f);
        destLen -= 4;
        ++src;
      }
    } else { 
      if (destLen < 3)
        goto error_more_output;
      
      *dest++ = (char)0xe0 | (*src >> 12);
      *dest++ = (char)0x80 | ((*src >> 6) & 0x003f);
      *dest++ = (char)0x80 | (*src & 0x003f);
      destLen -= 3;
    }
    ++src;
  }

  *aDestLength = dest - aDest;
  return NS_OK;

error_more_output:
  *aSrcLength = src - aSrc;
  *aDestLength = dest - aDest;
  return NS_OK_UENC_MOREOUTPUT;
}

NS_IMETHODIMP nsUnicodeToUTF8::Finish(char * aDest, PRInt32 * aDestLength)
{
  char * dest = aDest;

  if (mHighSurrogate) {
    if (*aDestLength < 3) {
      *aDestLength = 0;
      return NS_OK_UENC_MOREOUTPUT;
    }
    *dest++ = (char)0xe0 | (mHighSurrogate >> 12);
    *dest++ = (char)0x80 | ((mHighSurrogate >> 6) & 0x003f);
    *dest++ = (char)0x80 | (mHighSurrogate & 0x003f);
    mHighSurrogate = 0;
    *aDestLength = 3;
    return NS_OK;
  } 

  *aDestLength  = 0;
  return NS_OK;
}
