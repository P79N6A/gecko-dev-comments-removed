






































#ifndef nsUnicodeToSunIndic_h___
#define nsUnicodeToSunIndic_h___

#include "nspr.h"
#include "nsIFactory.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIModule.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharRepresentable.h"

#include "nsILE.h"




class nsUnicodeToSunIndic : public nsIUnicodeEncoder, public nsICharRepresentable
{
NS_DECL_ISUPPORTS

public:
  nsUnicodeToSunIndic();
  virtual ~nsUnicodeToSunIndic();

  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength,
                     char * aDest, PRInt32 * aDestLength);

  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);

  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength,
                          PRInt32 * aDestLength);

  NS_IMETHOD Reset();

  NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior,
                                    nsIUnicharEncoder * aEncoder,
                                    PRUnichar aChar);

  NS_IMETHOD FillInfo(PRUint32* aInfo);

private:
  PRUint8 mState;
  PRInt32 mByteOff;
  PRInt32 mCharOff;

  nsCOMPtr<nsILE> mCtlObj;

  PRInt32 mErrBehavior;
  PRUnichar mErrChar;
  nsCOMPtr<nsIUnicharEncoder> mErrEncoder;
};
#endif 
