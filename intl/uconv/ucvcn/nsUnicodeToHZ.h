




 







#ifndef nsUnicodeToHZ_h___
#define nsUnicodeToHZ_h___

#include "nsUCSupport.h"
#include "nsGBKConvUtil.h"



class nsUnicodeToHZ: public nsEncoderSupport
{
public:

  


  nsUnicodeToHZ();

protected:

  
  
  NS_IMETHOD ConvertNoBuff(const char16_t * aSrc, 
                            int32_t * aSrcLength, 
                            char * aDest, 
                            int32_t * aDestLength);

  NS_IMETHOD FinishNoBuff(char * aDest, int32_t * aDestLength);

  NS_IMETHOD ConvertNoBuffNoErr(const char16_t * aSrc, int32_t * aSrcLength, 
                                char * aDest, int32_t * aDestLength)
  {
    return NS_OK;
  }  

  uint16_t mHZState;
protected:
  nsGBKConvUtil mUtil;


};

#endif 
