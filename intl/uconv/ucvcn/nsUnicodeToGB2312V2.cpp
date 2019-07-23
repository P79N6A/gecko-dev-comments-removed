





































#include "nsUnicodeToGB2312V2.h"
#include "nsICharRepresentable.h"
#include "nsUCvCnDll.h"
#include "gbku.h"



nsUnicodeToGB2312V2::nsUnicodeToGB2312V2() :
  nsEncoderSupport(2)
{
  mUtil.InitToGBKTable();
}

NS_IMETHODIMP nsUnicodeToGB2312V2::ConvertNoBuff(const PRUnichar * aSrc, 
                                                 PRInt32 * aSrcLength, 
                                                 char * aDest, 
                                                 PRInt32 * aDestLength)
{
  PRInt32 iSrcLength = 0;
  PRInt32 iDestLength = 0;
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
      if(mUtil.UnicodeToGBKChar(*aSrc, PR_FALSE, &byte1, &byte2))
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




NS_IMETHODIMP nsUnicodeToGB2312V2::FillInfo(PRUint32 *aInfo)
{
  mUtil.FillGB2312Info(aInfo);
  
  for ( PRUint16 u = 0x0000; u <= 0x007F; u++)
    SET_REPRESENTABLE(aInfo, u);
  return NS_OK;
}
