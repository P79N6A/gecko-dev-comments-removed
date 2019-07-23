




































#ifndef nsHZToUnicode_h___
#define nsHZToUnicode_h___

#include "nsUCSupport.h"
#include "gbku.h"











class nsHZToUnicode : public nsBufferDecoderSupport
{
public:
		
  


  nsHZToUnicode();

protected:

  
  
  NS_IMETHOD ConvertNoBuff(const char* aSrc, PRInt32 * aSrcLength, 
                           PRUnichar *aDest, PRInt32 * aDestLength); 
  nsGBKConvUtil mUtil;

private:
  PRInt16 mHZState;

};

#endif 
