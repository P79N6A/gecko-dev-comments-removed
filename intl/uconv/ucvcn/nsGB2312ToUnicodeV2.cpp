
















































#if 0
#include "nsGB2312ToUnicodeV2.h"
#include "nsUCvCnDll.h"
#include "gbku.h"




NS_IMETHODIMP nsGB2312ToUnicodeV2::GetMaxLength(const char * aSrc, 
                                              PRInt32 aSrcLength, 
                                              PRInt32 * aDestLength)
{
  *aDestLength = aSrcLength;
  return NS_OK;
}
NS_IMETHODIMP nsGB2312ToUnicodeV2::ConvertNoBuff(const char* aSrc,
                                                 PRInt32 * aSrcLength,
                                                 PRUnichar *aDest,
                                                 PRInt32 * aDestLength)
{
  PRInt32 i=0;
  PRInt32 iSrcLength = (*aSrcLength);
  PRInt32 iDestlen = 0;
  nsresult rv = NS_OK;
  for (i=0;i<iSrcLength;i++)
  {
    if ( iDestlen >= (*aDestLength) )
    {
      rv = NS_OK_UDEC_MOREOUTPUT;
      break;
    }
    if(UINT8_IN_RANGE(0xa1, *aSrc, 0xfe))
    {
      if(i+1 >= iSrcLength)
      {
        rv = NS_OK_UDEC_MOREINPUT;
        break;
      }
      
      
      if(UINT8_IN_RANGE(0xa1, aSrc[1], 0xfe))
      {
        
        *aDest = mUtil.GBKCharToUnicode(aSrc[0], aSrc[1]);
        aSrc += 2;
        i++;
      } else {
        
        *aDest =  UCS2_NO_MAPPING;
        aSrc++;
      }
    } else {
      if(IS_ASCII(*aSrc))
      {
        
        *aDest = CAST_CHAR_TO_UNICHAR(*aSrc);
      } else {
        *aDest =  UCS2_NO_MAPPING;
      }
      aSrc++;
    }
    iDestlen++;
    aDest++;
    *aSrcLength = i+1;
  }
  *aDestLength = iDestlen;
  return rv;
}
#endif
