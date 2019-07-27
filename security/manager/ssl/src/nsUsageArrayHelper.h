



#ifndef _NSUSAGEARRAYHELPER_H_
#define _NSUSAGEARRAYHELPER_H_

#include "CertVerifier.h"
#include "nsNSSComponent.h"
#include "certt.h"
#include "pkix/Time.h"

class nsUsageArrayHelper
{
public:
  explicit nsUsageArrayHelper(CERTCertificate *aCert);

  nsresult GetUsagesArray(const char *suffix,
               bool localOnly,
               uint32_t outArraySize,
               uint32_t *_verified,
               uint32_t *_count,
               char16_t **tmpUsages);

  enum { max_returned_out_array_size = 12 };

private:
  CERTCertificate *mCert;
  nsresult m_rv;
  CERTCertDBHandle *defaultcertdb;
  nsCOMPtr<nsINSSComponent> nssComponent;

  uint32_t check(uint32_t previousCheckResult,
                 const char *suffix,
                 mozilla::psm::CertVerifier * certVerifier,
                 SECCertificateUsage aCertUsage,
                 mozilla::pkix::Time time,
                 mozilla::psm::CertVerifier::Flags flags,
                 uint32_t &aCounter,
                 char16_t **outUsages);

  void verifyFailed(uint32_t *_verified, int err);
};

#endif
