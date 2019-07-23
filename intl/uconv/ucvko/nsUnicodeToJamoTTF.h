







































#ifndef nsUnicodeToJamoTTF_h___
#define nsUnicodeToJamoTTF_h___

#include "nsUCSupport.h"




class nsUnicodeToJamoTTF : public nsIUnicodeEncoder, public nsICharRepresentable
{

  NS_DECL_ISUPPORTS

public:

  


  nsUnicodeToJamoTTF();
  virtual ~nsUnicodeToJamoTTF();

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

protected:
  PRUnichar *mJamos;
  PRUnichar mJamosStatic[9];
  PRInt32  mJamoCount;
  PRInt32  mJamosMaxLength;
  PRInt32  mByteOff;

  PRInt32 mErrBehavior;
  PRUnichar mErrChar;
  nsCOMPtr<nsIUnicharEncoder> mErrEncoder;

private:
  NS_IMETHOD  composeHangul(char* output);
  int RenderAsPrecompSyllable (PRUnichar* aSrc, PRInt32* aSrcLength, 
                               char* aResult);
};

#endif 

