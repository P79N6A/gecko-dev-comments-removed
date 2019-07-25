




































#ifndef _NS_NSSCERTIFICATECHILD_H_
#define _NS_NSSCERTIFICATECHILD_H_

#include "nsIX509Cert.h"
#include "nsIX509Cert2.h"
#include "nsIX509Cert3.h"
#include "nsIX509CertDB.h"
#include "nsIX509CertList.h"
#include "nsIASN1Object.h"
#include "nsISMimeCert.h"
#include "nsIIdentityInfo.h"
#include "nsNSSShutDown.h"
#include "nsISimpleEnumerator.h"
#include "nsISerializable.h"
#include "nsIClassInfo.h"

#include "nsNSSCertHeader.h"

class nsINSSComponent;
class nsIASN1Sequence;


class nsNSSCertificateFakeTransport : public nsIX509Cert3,
                              public nsISerializable,
                              public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIX509CERT
  NS_DECL_NSIX509CERT2
  NS_DECL_NSIX509CERT3
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  nsNSSCertificateFakeTransport();
  virtual ~nsNSSCertificateFakeTransport();

private:
  SECItem *mCertSerialization;
};

#endif 
