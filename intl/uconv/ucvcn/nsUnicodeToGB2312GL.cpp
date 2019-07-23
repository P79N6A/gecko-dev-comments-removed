





































#include "nsICharRepresentable.h"
#include "nsUnicodeToGB2312GL.h"
#include "nsUCvCnDll.h"

#include "gbku.h"




nsUnicodeToGB2312GL::nsUnicodeToGB2312GL() :
  nsEncoderSupport(2)
{
  mUtil.InitToGBKTable();
}

NS_IMETHODIMP nsUnicodeToGB2312GL::ConvertNoBuff(const PRUnichar * aSrc, 
                                                 PRInt32 * aSrcLength, 
                                                 char * aDest, 
                                                 PRInt32 * aDestLength)
{
  PRInt32 iSrcLength = 0;
  PRInt32 iDestLength = 0;
  nsresult res = NS_OK;
  while( iSrcLength < *aSrcLength)
  {
    char byte1, byte2;
    if(mUtil.UnicodeToGBKChar(*aSrc, PR_TRUE, &byte1,&byte2))
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
      
      res = NS_ERROR_UENC_NOMAPPING;
      break; 
    }
    iSrcLength++ ; 
    aSrc++; 
    
    if ( (iDestLength >= *aDestLength) && ( iSrcLength < *aSrcLength) )
    {
      res = NS_OK_UENC_MOREOUTPUT;
      break;
    }
  }
  *aDestLength = iDestLength;
  *aSrcLength = iSrcLength;
  return res;
}




NS_IMETHODIMP nsUnicodeToGB2312GL::FillInfo(PRUint32 *aInfo)
{
  mUtil.FillGB2312Info(aInfo);
  return NS_OK;
}
