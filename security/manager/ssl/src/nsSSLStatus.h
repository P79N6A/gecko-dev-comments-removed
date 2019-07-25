






































#ifndef _NSSSLSTATUS_H
#define _NSSSLSTATUS_H

#include "nsISSLStatus.h"

#include "nsAutoPtr.h"
#include "nsXPIDLString.h"
#include "nsIX509Cert.h"
#include "nsISerializable.h"
#include "nsIClassInfo.h"

class nsSSLStatus
  : public nsISSLStatus
  , public nsISerializable
  , public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISSLSTATUS
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  nsSSLStatus();
  virtual ~nsSSLStatus();

  
  nsCOMPtr<nsIX509Cert> mServerCert;

  PRUint32 mKeyLength;
  PRUint32 mSecretKeyLength;
  nsXPIDLCString mCipherName;

  bool mIsDomainMismatch;
  bool mIsNotValidAtThisTime;
  bool mIsUntrusted;

  bool mHaveKeyLengthAndCipher;
  bool mHaveCertErrorBits;
};


#define NS_SSLSTATUS_CID \
{ 0x2c3837af, 0x8b85, 0x4a68, \
  { 0xb0, 0xd8, 0x0a, 0xed, 0x88, 0x98, 0x5b, 0x32 } }

#endif
