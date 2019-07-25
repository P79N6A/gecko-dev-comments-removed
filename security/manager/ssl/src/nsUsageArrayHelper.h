




































#ifndef _NSUSAGEARRAYHELPER_H_
#define _NSUSAGEARRAYHELPER_H_

#include "certt.h"

#include "nsNSSComponent.h"

class nsUsageArrayHelper
{
public:
  nsUsageArrayHelper(CERTCertificate *aCert);

  nsresult GetUsagesArray(const char *suffix,
               PRBool localOnly,
               PRUint32 outArraySize,
               PRUint32 *_verified,
               PRUint32 *_count,
               PRUnichar **tmpUsages);

  enum { max_returned_out_array_size = 12 };

private:
  CERTCertificate *mCert;
  nsresult m_rv;
  CERTCertDBHandle *defaultcertdb;
  nsCOMPtr<nsINSSComponent> nssComponent;

  void check(const char *suffix,
             SECCertificateUsage aCertUsage,
             PRUint32 &aCounter,
             PRUnichar **outUsages);

  void verifyFailed(PRUint32 *_verified, int err);
};

#endif
