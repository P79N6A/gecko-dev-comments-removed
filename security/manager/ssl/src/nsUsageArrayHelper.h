



#ifndef _NSUSAGEARRAYHELPER_H_
#define _NSUSAGEARRAYHELPER_H_

#include "certt.h"

#include "nsNSSComponent.h"

class nsUsageArrayHelper
{
public:
  nsUsageArrayHelper(CERTCertificate *aCert);

  nsresult GetUsagesArray(const char *suffix,
               bool localOnly,
               uint32_t outArraySize,
               uint32_t *_verified,
               uint32_t *_count,
               PRUnichar **tmpUsages);

  enum { max_returned_out_array_size = 12 };

private:
  CERTCertificate *mCert;
  nsresult m_rv;
  CERTCertDBHandle *defaultcertdb;
  nsCOMPtr<nsINSSComponent> nssComponent;

  void check(const char *suffix,
             SECCertificateUsage aCertUsage,
             uint32_t &aCounter,
             PRUnichar **outUsages);

  void verifyFailed(uint32_t *_verified, int err);
};

#endif
