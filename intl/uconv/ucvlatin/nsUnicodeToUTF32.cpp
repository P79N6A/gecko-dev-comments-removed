







































#include <string.h>
#include "nsUCSupport.h"
#include "nsUnicodeToUTF32.h"

#ifdef IS_BIG_ENDIAN
#define UCS4_TO_LE_STRING(u, s)           \
  PR_BEGIN_MACRO                          \
    s[3] = PRUint8(((u) >> 24) & 0xffL);  \
    s[2] = PRUint8(((u) >> 16) & 0xffL);  \
    s[1] = PRUint8(((u) >> 8) & 0xffL);   \
    s[0] = PRUint8((u) & 0xffL);          \
  PR_END_MACRO
#else 
#define UCS4_TO_LE_STRING(u, s)           \
  PR_BEGIN_MACRO                          \
    *((PRUint32*)(s)) = (u);              \
  PR_END_MACRO
#endif

#ifdef IS_BIG_ENDIAN
#define UCS4_TO_BE_STRING(u, s)           \
  PR_BEGIN_MACRO                          \
    *((PRUint32*)(s)) = (u);              \
  PR_END_MACRO
#else
#define UCS4_TO_BE_STRING(u, s)           \
  PR_BEGIN_MACRO                          \
    s[0] = PRUint8(((u) >> 24) & 0xffL);  \
    s[1] = PRUint8(((u) >> 16) & 0xffL);  \
    s[2] = PRUint8(((u) >> 8) & 0xffL);   \
    s[3] = PRUint8((u) & 0xffL);          \
  PR_END_MACRO
#endif



 
static nsresult ConvertCommon(const PRUnichar * aSrc, 
                              PRInt32 * aSrcLength, 
                              char * aDest, 
                              PRInt32 * aDestLength,
                              PRUnichar * aHighSurrogate,
                              PRBool aIsLE)
{
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  const char * destEnd = aDest + *aDestLength; 
  PRUint32 ucs4;


  
  if (*aHighSurrogate) 
  {
    if (! *aSrcLength)
    {
      *aDestLength = 0;
      return NS_OK_UENC_MOREINPUT;
    }
    if (*aDestLength < 4) 
    {
      *aSrcLength = 0;
      *aDestLength = 0;
      return NS_OK_UENC_MOREOUTPUT;
    }
    if ((*src & 0xfc00) != 0xdc00) 
      ucs4 = PRUint32(*aHighSurrogate);
    else 
      ucs4 = (((*aHighSurrogate & 0x3ffL) << 10) | (*src & 0x3ffL)) + 0x10000;

    ++src;
    if (aIsLE)
      UCS4_TO_LE_STRING(ucs4, dest);
    else
      UCS4_TO_BE_STRING(ucs4, dest);
    dest += 4;
    *aHighSurrogate = 0;
  }

  while (src < srcEnd) {
    
    if ((src[0] & 0xfc00) != 0xd800) 
    {
      if (destEnd - dest < 4)
        goto error_more_output;
      ucs4 = PRUint32(src[0]);  
    }
    else  
    {
      if ((src+1) >= srcEnd) {
        
        *aHighSurrogate = src[0];
        *aDestLength = dest - aDest;
        return NS_OK_UENC_MOREINPUT;
      }
      
      if (destEnd - dest < 4)
        goto error_more_output;
      if ((src[1] & 0xfc00) != 0xdc00)  
        ucs4 = PRUint32(src[0]);  
      else 
      {  
        ucs4 = (((src[0] & 0x3ffL) << 10) | (src[1] & 0x3ffL)) + 0x10000;
        *aHighSurrogate = 0;
        ++src;
      }
    }
    if (aIsLE)
      UCS4_TO_LE_STRING(ucs4, dest);
    else
      UCS4_TO_BE_STRING(ucs4, dest);
    dest += 4;
    ++src;
  }

  *aDestLength = dest - aDest;
  return NS_OK;

error_more_output:
  *aSrcLength = src - aSrc;
  *aDestLength = dest - aDest;
  return NS_OK_UENC_MOREOUTPUT;

}

static nsresult FinishCommon(char * aDest, 
                             PRInt32 * aDestLength, 
                             PRUnichar * aHighSurrogate,
                             PRBool aIsLE)
{
  char * dest = aDest;

  if (*aHighSurrogate) {
    if (*aDestLength < 4) {
      *aDestLength = 0;
      return NS_OK_UENC_MOREOUTPUT;
    }
    PRUint32 high = PRUint32(*aHighSurrogate);
    if (aIsLE)
      UCS4_TO_LE_STRING(high, dest);
    else
      UCS4_TO_BE_STRING(high, dest);
    *aHighSurrogate = 0;
    *aDestLength = 4;
    return NS_OK;
  } 

  *aDestLength  = 0;
  return NS_OK;
}






NS_IMPL_ISUPPORTS1(nsUnicodeToUTF32, nsIUnicodeEncoder)





NS_IMETHODIMP nsUnicodeToUTF32::GetMaxLength(const PRUnichar * aSrc, 
                                            PRInt32 aSrcLength, 
                                            PRInt32 * aDestLength)
{
  *aDestLength = aSrcLength * 4;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToUTF32::FillInfo(PRUint32 *aInfo)
{
  memset(aInfo, 0xFF, (0x10000L >> 3));
  return NS_OK;
}







  

NS_IMETHODIMP nsUnicodeToUTF32BE::Convert(const PRUnichar * aSrc, 
                                          PRInt32 * aSrcLength, 
                                          char * aDest, 
                                          PRInt32 * aDestLength)
{
  return ConvertCommon(aSrc, aSrcLength, aDest, aDestLength, 
                       &mHighSurrogate, PR_FALSE);
}

NS_IMETHODIMP nsUnicodeToUTF32BE::Finish(char * aDest, 
                                         PRInt32 * aDestLength)
{
  return FinishCommon(aDest, aDestLength, &mHighSurrogate, PR_FALSE);
}




  




NS_IMETHODIMP nsUnicodeToUTF32LE::Convert(const PRUnichar * aSrc, 
                                          PRInt32 * aSrcLength, 
                                          char * aDest, 
                                          PRInt32 * aDestLength)
{
  return ConvertCommon(aSrc, aSrcLength, aDest, aDestLength, 
                       &mHighSurrogate, PR_TRUE);
}

NS_IMETHODIMP nsUnicodeToUTF32LE::Finish(char * aDest, 
                                         PRInt32 * aDestLength)
{
  return FinishCommon(aDest, aDestLength, &mHighSurrogate, PR_TRUE);
}

