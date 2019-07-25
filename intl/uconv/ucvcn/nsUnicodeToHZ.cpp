



 










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
  int32_t * aSrcLength, 
  char * aDest, 
  int32_t * aDestLength)
{
  int32_t i=0;
  int32_t iSrcLength = *aSrcLength;
  int32_t iDestLength = 0;

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
      if(mUtil.UnicodeToGBKChar(*aSrc, true, &aDest[0], &aDest[1])) {
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

NS_IMETHODIMP nsUnicodeToHZ::FinishNoBuff(char * aDest, int32_t * aDestLength)
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
