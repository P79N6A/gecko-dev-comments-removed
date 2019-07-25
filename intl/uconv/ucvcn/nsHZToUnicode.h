




#ifndef nsHZToUnicode_h___
#define nsHZToUnicode_h___

#include "nsUCSupport.h"
#include "gbku.h"











class nsHZToUnicode : public nsBufferDecoderSupport
{
public:
		
  


  nsHZToUnicode();

protected:

  
  
  NS_IMETHOD ConvertNoBuff(const char* aSrc, int32_t * aSrcLength, 
                           PRUnichar *aDest, int32_t * aDestLength); 
  nsGBKConvUtil mUtil;

private:
  int16_t mHZState;
  uint32_t mRunLength; 
  char mOddByte; 

};

#endif 
