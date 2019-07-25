




































#ifndef _NS_NSSCERTIFICATECHILD_H_
#define _NS_NSSCERTIFICATECHILD_H_

#include "nsIX509Cert.h"
#include "nsNSSShutDown.h"
#include "nsISerializable.h"
#include "nsIClassInfo.h"

#include "nsNSSCertHeader.h"

class nsINSSComponent;
class nsIASN1Sequence;


class nsNSSCertificateFakeTransport : public nsIX509Cert,
                              public nsISerializable,
                              public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIX509CERT
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  nsNSSCertificateFakeTransport();
  virtual ~nsNSSCertificateFakeTransport();

private:
  SECItem *mCertSerialization;
};

#endif 
