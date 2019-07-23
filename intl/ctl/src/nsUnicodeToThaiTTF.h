







































#ifndef nsUnicodeToThaiTTF_h___
#define nsUnicodeToThaiTTF_h___

#include "nsISupports.h"
#include "nsUnicodeToTIS620.h"

#define CHAR_BUFFER_SIZE 2048




class nsUnicodeToThaiTTF : public nsUnicodeToTIS620
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  nsUnicodeToThaiTTF() : nsUnicodeToTIS620() {};
  virtual ~nsUnicodeToThaiTTF() {};

  NS_IMETHOD Convert      (const PRUnichar * aSrc, PRInt32 * aSrcLength,
                           char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD GetMaxLength (const PRUnichar * aSrc, PRInt32  aSrcLength,
                           PRInt32 * aDestLength);

  NS_IMETHOD SetOutputErrorBehavior (PRInt32 aBehavior, 
                                     nsIUnicharEncoder *aEncoder, 
                                     PRUnichar aChar);

private:
  char mStaticBuffer[CHAR_BUFFER_SIZE];

  PRInt32 mErrBehavior;
  PRUnichar mErrChar;
  nsCOMPtr<nsIUnicharEncoder> mErrEncoder;
};

#endif 

