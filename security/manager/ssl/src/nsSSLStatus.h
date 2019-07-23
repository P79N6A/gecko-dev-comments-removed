






































#include "nsISSLStatus.h"

#include "nsAutoPtr.h"
#include "nsXPIDLString.h"
#include "nsIX509Cert.h"

class nsSSLStatus
  : public nsISSLStatus
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISSLSTATUS

  nsSSLStatus();
  virtual ~nsSSLStatus();

  
  nsCOMPtr<nsIX509Cert> mServerCert;

  PRUint32 mKeyLength;
  PRUint32 mSecretKeyLength;
  nsXPIDLCString mCipherName;

  PRBool mIsDomainMismatch;
  PRBool mIsNotValidAtThisTime;
  PRBool mIsUntrusted;

  PRBool mHaveKeyLengthAndCipher;
  PRBool mHaveCertStatus;
};
