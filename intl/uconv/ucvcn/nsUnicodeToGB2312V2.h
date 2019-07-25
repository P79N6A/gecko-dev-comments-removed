




#ifndef nsUnicodeToGB2312V2_h___
#define nsUnicodeToGB2312V2_h___

#include "nsUCSupport.h"
#include "gbku.h"










class nsUnicodeToGB2312V2 : public nsEncoderSupport
{
public:

  


  nsUnicodeToGB2312V2();

protected:

  NS_IMETHOD ConvertNoBuff(const PRUnichar * aSrc, 
                            int32_t * aSrcLength, 
                            char * aDest, 
                            int32_t * aDestLength);

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, int32_t * aSrcLength, 
                                char * aDest, int32_t * aDestLength)
  {
    return NS_OK;
  }   

protected:
  nsGBKConvUtil mUtil;
};

#endif 
