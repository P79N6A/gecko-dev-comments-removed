






































#ifndef nsUnicodeToX11Johab_h___
#define nsUnicodeToX11Johab_h___

#include "nsUCSupport.h"




class nsUnicodeToX11Johab : public nsIUnicodeEncoder, public nsICharRepresentable
{

NS_DECL_ISUPPORTS

public:

  


  nsUnicodeToX11Johab();
  virtual ~nsUnicodeToX11Johab();

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
private:
  PRUint8 state;
  PRUint8 l;
  PRUint8 v;
  PRUint8 t;
  PRInt32 byteOff;
  PRInt32 charOff;
  void composeHangul(char* output);
};

#endif 
