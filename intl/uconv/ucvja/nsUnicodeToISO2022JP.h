




#ifndef nsUnicodeToISO2022JP_h___
#define nsUnicodeToISO2022JP_h___

#include "nsUCSupport.h"










class nsUnicodeToISO2022JP : public nsEncoderSupport
{
public:

  


  nsUnicodeToISO2022JP();

  


  virtual ~nsUnicodeToISO2022JP();

protected:

  int32_t                   mCharset;       

  nsresult ChangeCharset(int32_t aCharset, char * aDest, 
      int32_t * aDestLength);
  nsresult ConvertHankaku(const PRUnichar *aSrc, int32_t * aSrcLength,
                          char *aDest, int32_t * aDestLength);

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
  NS_IMETHOD FinishNoBuff(char * aDest, int32_t * aDestLength);
  NS_IMETHOD Reset();
};

#endif 
