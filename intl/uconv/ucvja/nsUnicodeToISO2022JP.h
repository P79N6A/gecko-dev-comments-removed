




































#ifndef nsUnicodeToISO2022JP_h___
#define nsUnicodeToISO2022JP_h___

#include "nsUCSupport.h"










class nsUnicodeToISO2022JP : public nsEncoderSupport
{
public:

  


  nsUnicodeToISO2022JP();

  


  virtual ~nsUnicodeToISO2022JP();

protected:

  PRInt32                   mCharset;       

  nsresult ChangeCharset(PRInt32 aCharset, char * aDest, 
      PRInt32 * aDestLength);

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD FinishNoBuff(char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Reset();
  NS_IMETHOD FillInfo(PRUint32 *aInfo);
};

#endif 
