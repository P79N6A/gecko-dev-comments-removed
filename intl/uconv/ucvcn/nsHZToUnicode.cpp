



























































#include "nsUCvCnDll.h"
#include "nsHZToUnicode.h"
#include "gbku.h"







#define HZ_STATE_GB		1
#define HZ_STATE_ASCII	2
#define HZ_STATE_TILD	3
#define HZLEAD1 '~'
#define HZLEAD2 '{'
#define HZLEAD3 '}'
#define HZLEAD4 '\n'

nsHZToUnicode::nsHZToUnicode() : nsBufferDecoderSupport(1)
{
  mHZState = HZ_STATE_ASCII;	
  mRunLength = 0;
}

NS_IMETHODIMP nsHZToUnicode::ConvertNoBuff(
  const char* aSrc, 
  PRInt32 * aSrcLength, 
  PRUnichar *aDest, 
  PRInt32 * aDestLength)
{
  PRInt32 i=0;
  PRInt32 iSrcLength = *aSrcLength;
  PRInt32 iDestlen = 0;
  PRUint8 ch1, ch2;
  nsresult res = NS_OK;
  *aSrcLength=0;
  for (i=0;i<iSrcLength;i++)
  {
    if ( iDestlen >= (*aDestLength) )
    {
      res = NS_OK_UDEC_MOREOUTPUT;
      break;
    }
    if ( *aSrc & 0x80 ) 
    {
      if (UINT8_IN_RANGE(0x81, aSrc[0], 0xFE) &&
          UINT8_IN_RANGE(0x40, aSrc[1], 0xFE)) {
        
        *aDest = mUtil.GBKCharToUnicode(aSrc[0], aSrc[1]);
      } else {
        *aDest = UCS2_NO_MAPPING;
      }
      aSrc += 2;
      i++;
      iDestlen++;
      aDest++;
      *aSrcLength = i+1;
      continue;
    }
    
    
    ch1 = *aSrc;
    ch2	= *(aSrc+1);
    if (ch1 == HZLEAD1 )  
    {
      switch (ch2)
      {
        case HZLEAD2: 
          
          
          mHZState = HZ_STATE_GB;
          mRunLength = 0;
          aSrc += 2;
          i++;
          break;
        case HZLEAD3: 
          
          
          mHZState = HZ_STATE_ASCII;
          aSrc += 2;
          i++;
          if (mRunLength == 0) {
            *aDest = UCS2_NO_MAPPING;
            iDestlen++;
            aDest++;
          }
          mRunLength = 0;
          break;
        case HZLEAD1: 
          
          aSrc++;
          *aDest = CAST_CHAR_TO_UNICHAR(*aSrc);
          aSrc++;
          i++;
          iDestlen++;
          aDest++;
          mRunLength++;
          break;
        case HZLEAD4:	
          
          
          
          
          aSrc++;
          break;
        default:
          
          aSrc += 2;
          *aDest = UCS2_NO_MAPPING;
          iDestlen++;
          aDest++;
          break;
      };
      continue;
    }
    
    switch (mHZState)
    {
      case HZ_STATE_GB:
        
        *aDest = mUtil.GBKCharToUnicode(aSrc[0]|0x80, aSrc[1]|0x80);
        aSrc += 2;
        i++;
        iDestlen++;
        aDest++;
        mRunLength++;
        break;
      case HZ_STATE_ASCII:
      default:
        
        
        *aDest = CAST_CHAR_TO_UNICHAR(*aSrc);
        aSrc++;
        iDestlen++;
        aDest++;
        break;
    }
    *aSrcLength = i+1;
  }
  *aDestLength = iDestlen;
  return NS_OK;
}


