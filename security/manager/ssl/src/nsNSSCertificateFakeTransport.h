




#ifndef nsNSSCertificateFakeTransport_h
#define nsNSSCertificateFakeTransport_h

#include "nsIClassInfo.h"
#include "nsISerializable.h"
#include "nsIX509Cert.h"
#include "secitem.h"

class nsNSSCertificateFakeTransport : public nsIX509Cert,
                                      public nsISerializable,
                                      public nsIClassInfo
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIX509CERT
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  nsNSSCertificateFakeTransport();

protected:
  virtual ~nsNSSCertificateFakeTransport();

private:
  SECItem* mCertSerialization;
};

#endif 
