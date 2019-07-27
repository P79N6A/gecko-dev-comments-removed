





#ifndef _NSSSLSTATUS_H
#define _NSSSLSTATUS_H

#include "nsISSLStatus.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIX509Cert.h"
#include "nsISerializable.h"
#include "nsIClassInfo.h"

class nsSSLStatus MOZ_FINAL
  : public nsISSLStatus
  , public nsISerializable
  , public nsIClassInfo
{
protected:
  virtual ~nsSSLStatus();
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISSLSTATUS
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  nsSSLStatus();

  
  nsCOMPtr<nsIX509Cert> mServerCert;

  uint16_t mCipherSuite;
  uint16_t mProtocolVersion;

  bool mIsDomainMismatch;
  bool mIsNotValidAtThisTime;
  bool mIsUntrusted;

  bool mHaveCipherSuiteAndProtocol;

  

  bool mHaveCertErrorBits;
};

#define NS_SSLSTATUS_CID \
{ 0x61f69c85, 0x0fed, 0x44fb, \
  { 0x89, 0x8f, 0xa4, 0xb1, 0x3c, 0x33, 0x3c, 0x8d } }

#endif
