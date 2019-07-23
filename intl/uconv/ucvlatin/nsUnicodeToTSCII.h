







































#ifndef nsUnicodeToTSCII_h___
#define nsUnicodeToTSCII_h___

#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharRepresentable.h"




class nsUnicodeToTSCII : public nsIUnicodeEncoder, public nsICharRepresentable
{

NS_DECL_ISUPPORTS

public:
  nsUnicodeToTSCII() { mBuffer = 0; };
  virtual ~nsUnicodeToTSCII() {};

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
  PRUint32 mBuffer; 
                    
                    
};

#define CHAR_BUFFER_SIZE 2048




class nsUnicodeToTamilTTF : public nsUnicodeToTSCII
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  nsUnicodeToTamilTTF() : nsUnicodeToTSCII() {};
  virtual ~nsUnicodeToTamilTTF() {};

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
