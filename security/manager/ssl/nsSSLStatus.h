





#ifndef _NSSSLSTATUS_H
#define _NSSSLSTATUS_H

#include "nsISSLStatus.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIX509Cert.h"
#include "nsISerializable.h"
#include "nsIClassInfo.h"
#include "nsNSSCertificate.h" 

class nsSSLStatus final
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

  void SetServerCert(nsNSSCertificate* aServerCert,
                     nsNSSCertificate::EVStatus aEVStatus);

  bool HasServerCert() {
    return mServerCert != nullptr;
  }

  
  uint16_t mCipherSuite;
  uint16_t mProtocolVersion;

  bool mIsDomainMismatch;
  bool mIsNotValidAtThisTime;
  bool mIsUntrusted;
  bool mIsEV;

  bool mHasIsEVStatus;
  bool mHaveCipherSuiteAndProtocol;

  

  bool mHaveCertErrorBits;

private:
  nsCOMPtr<nsIX509Cert> mServerCert;
};

#define NS_SSLSTATUS_CID \
{ 0xe2f14826, 0x9e70, 0x4647, \
  { 0xb2, 0x3f, 0x10, 0x10, 0xf5, 0x12, 0x46, 0x28 } }

#endif
