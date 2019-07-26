




#include "nsUnicodeToGB2312V2.h"
#include "gbku.h"



nsUnicodeToGB2312V2::nsUnicodeToGB2312V2() :
  nsEncoderSupport(2)
{
}

NS_IMETHODIMP nsUnicodeToGB2312V2::ConvertNoBuff(const char16_t * aSrc, 
                                                 int32_t * aSrcLength, 
                                                 char * aDest, 
                                                 int32_t * aDestLength)
{
  int32_t iSrcLength = 0;
  int32_t iDestLength = 0;
  nsresult res = NS_OK;
  
  while (iSrcLength < *aSrcLength)
  {
    
    if(IS_ASCII(*aSrc))
    {
      
      *aDest = CAST_UNICHAR_TO_CHAR(*aSrc);
      aDest++; 
      iDestLength +=1;
    } else {
      char byte1, byte2;
      if(mUtil.UnicodeToGBKChar(*aSrc, false, &byte1, &byte2))
      {
        if(iDestLength+2 > *aDestLength) 
        {
          res = NS_OK_UENC_MOREOUTPUT;
          break;
        }
        aDest[0]=byte1;
        aDest[1]=byte2;
        aDest += 2;  
        iDestLength +=2; 
      } else {
        
        res= NS_ERROR_UENC_NOMAPPING;
        iSrcLength++;   
        break;
      }
    }
    iSrcLength++ ;   
    aSrc++;  
    if ( iDestLength >= (*aDestLength) && (iSrcLength < *aSrcLength ))
    {
      res = NS_OK_UENC_MOREOUTPUT;
      break;
    }
  }
  *aDestLength = iDestLength;
  *aSrcLength = iSrcLength;
  return res;
}
