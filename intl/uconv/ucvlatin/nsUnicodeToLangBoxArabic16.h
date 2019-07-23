




































#ifndef nsUnicodeToLangBoxArabic16_h__
#define nsUnicodeToLangBoxArabic16_h__
#include "nsUCSupport.h"



class nsUnicodeToLangBoxArabic16 : public nsBasicEncoder
{

public:

  


  nsUnicodeToLangBoxArabic16() {};
  virtual ~nsUnicodeToLangBoxArabic16() {};

  NS_IMETHOD Convert(
      const PRUnichar * aSrc, PRInt32 * aSrcLength,
      char * aDest, PRInt32 * aDestLength);

  NS_IMETHOD Finish(
      char * aDest, PRInt32 * aDestLength);

  NS_IMETHOD GetMaxLength(
      const PRUnichar * aSrc, PRInt32 aSrcLength,
      PRInt32 * aDestLength);

  NS_IMETHOD Reset();
 
  NS_IMETHOD SetOutputErrorBehavior(
      PRInt32 aBehavior,
      nsIUnicharEncoder * aEncoder, PRUnichar aChar);
 
  NS_IMETHOD FillInfo(PRUint32* aInfo);
};

#endif 
