



































 










#include "nsUnicodeToHZ.h"
#include "nsUCvCnDll.h"
#include "gbku.h"


#define HZ_STATE_GB		1
#define HZ_STATE_ASCII	2
#define HZ_STATE_TILD	3
#define HZLEAD1 '~'
#define HZLEAD2 '{'
#define HZLEAD3 '}'
#define UNICODE_TILD	0x007E
nsUnicodeToHZ::nsUnicodeToHZ() : nsEncoderSupport(6)
{
  mUtil.InitToGBKTable();
  mHZState = HZ_STATE_ASCII;	
}
NS_IMETHODIMP nsUnicodeToHZ::ConvertNoBuff(
  const PRUnichar * aSrc, 
  PRInt32 * aSrcLength, 
  char * aDest, 
  PRInt32 * aDestLength)
{
  PRInt32 i=0;
  PRInt32 iSrcLength = *aSrcLength;
  PRInt32 iDestLength = 0;

  for (i=0;i< iSrcLength;i++)
  {
    if(! IS_ASCII(*aSrc))
    {
      
      if ( mHZState != HZ_STATE_GB )
      {
        
        mHZState = HZ_STATE_GB;
        aDest[0] = '~';
        aDest[1] = '{';
        aDest += 2;	
        iDestLength +=2;
      }
      if(mUtil.UnicodeToGBKChar(*aSrc, PR_TRUE, &aDest[0], &aDest[1])) {
        aDest += 2;	
        iDestLength +=2;
      } else {
        
        
        
      }
    } else {
      

      
      if ( mHZState == HZ_STATE_GB )
      {
        mHZState = HZ_STATE_ASCII;
        aDest[0] = '~';
        aDest[1] = '}';
        aDest += 2; 
        iDestLength +=2;
      }
          
      
      if ( *aSrc == UNICODE_TILD )
      {
        aDest[0] = '~';
        aDest[1] = '~';
        aDest += 2; 
        iDestLength +=2;
      } else {
        
	
        
        *aDest = (char) ( (PRUnichar)(*aSrc) );
        aDest++; 
        iDestLength +=1;
      }
    }
    aSrc++;	 
    if ( iDestLength >= (*aDestLength) )
    {
      break;
    }
  }
  *aDestLength = iDestLength;
  *aSrcLength = i;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToHZ::FinishNoBuff(char * aDest, PRInt32 * aDestLength)
{
  if ( mHZState == HZ_STATE_GB )
  {
    
    mHZState = HZ_STATE_ASCII;
    aDest[0] = '~';
    aDest[1] = '}';
    *aDestLength = 2;
  } else {
    *aDestLength = 0;
  }
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToHZ::FillInfo(PRUint32 *aInfo)
{
  mUtil.FillGB2312Info(aInfo);
  
  for ( PRUint16 u = 0x0000; u <= 0x007F; u++)
    SET_REPRESENTABLE(aInfo, u);
  return NS_OK;
}
