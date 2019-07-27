












#include <stdio.h>

#include "TLSServer.h"

using namespace mozilla;
using namespace mozilla::test;

struct BadCertHost
{
  const char *mHostName;
  const char *mCertName;
};


const BadCertHost sBadCertHosts[] =
{
  { "expired.example.com", "expired" },
  { "notyetvalid.example.com", "notYetValid" },
  { "before-epoch.example.com", "beforeEpoch" },
  { "selfsigned.example.com", "selfsigned" },
  { "unknownissuer.example.com", "unknownissuer" },
  { "mismatch.example.com", "mismatch" },
  { "expiredissuer.example.com", "expiredissuer" },
  { "notyetvalidissuer.example.com", "notYetValidIssuer" },
  { "before-epoch-issuer.example.com", "beforeEpochIssuer" },
  { "md5signature.example.com", "md5signature" },
  { "untrusted.example.com", "localhostAndExampleCom" },
  { "untrustedissuer.example.com", "untrustedissuer" },
  { "mismatch-expired.example.com", "mismatch-expired" },
  { "mismatch-notYetValid.example.com", "mismatch-notYetValid" },
  { "mismatch-untrusted.example.com", "mismatch-untrusted" },
  { "untrusted-expired.example.com", "untrusted-expired" },
  { "md5signature-expired.example.com", "md5signature-expired" },
  { "mismatch-untrusted-expired.example.com", "mismatch-untrusted-expired" },
  { "inadequatekeyusage.example.com", "inadequatekeyusage" },
  { "selfsigned-inadequateEKU.example.com", "selfsigned-inadequateEKU" },
  { "self-signed-end-entity-with-cA-true.example.com", "self-signed-EE-with-cA-true" },
  { "ca-used-as-end-entity.example.com", "ca-used-as-end-entity" },
  { "ca-used-as-end-entity-name-mismatch.example.com", "ca-used-as-end-entity" },
  
  
  
  
  { "include-subdomains.pinning.example.com", "localhostAndExampleCom" },
  { "good.include-subdomains.pinning.example.com", "localhostAndExampleCom" },
  { "bad.include-subdomains.pinning.example.com", "otherIssuerEE" },
  { "bad.include-subdomains.pinning.example.com.", "otherIssuerEE" },
  { "bad.include-subdomains.pinning.example.com..", "otherIssuerEE" },
  { "exclude-subdomains.pinning.example.com", "localhostAndExampleCom" },
  { "sub.exclude-subdomains.pinning.example.com", "otherIssuerEE" },
  { "test-mode.pinning.example.com", "otherIssuerEE" },
  { "unknownissuer.include-subdomains.pinning.example.com", "unknownissuer" },
  { "unknownissuer.test-mode.pinning.example.com", "unknownissuer" },
  { "nsCertTypeNotCritical.example.com", "nsCertTypeNotCritical" },
  { "nsCertTypeCriticalWithExtKeyUsage.example.com", "nsCertTypeCriticalWithExtKeyUsage" },
  { "nsCertTypeCritical.example.com", "nsCertTypeCritical" },
  { "end-entity-issued-by-v1-cert.example.com", "eeIssuedByV1Cert" },
  { "end-entity-issued-by-non-CA.example.com", "eeIssuedByNonCA" },
  { "inadequate-key-size-ee.example.com", "inadequateKeySizeEE" },
  { "badSubjectAltNames.example.com", "badSubjectAltNames" },
  { nullptr, nullptr }
};

int32_t
DoSNISocketConfig(PRFileDesc *aFd, const SECItem *aSrvNameArr,
                  uint32_t aSrvNameArrSize, void *aArg)
{
  const BadCertHost *host = GetHostForSNI(aSrvNameArr, aSrvNameArrSize,
                                          sBadCertHosts);
  if (!host) {
    return SSL_SNI_SEND_ALERT;
  }

  if (gDebugLevel >= DEBUG_VERBOSE) {
    fprintf(stderr, "found pre-defined host '%s'\n", host->mHostName);
  }

  ScopedCERTCertificate cert;
  SSLKEAType certKEA;
  if (SECSuccess != ConfigSecureServerWithNamedCert(aFd, host->mCertName,
                                                    &cert, &certKEA)) {
    return SSL_SNI_SEND_ALERT;
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <NSS DB directory>\n", argv[0]);
    return 1;
  }

  return StartServer(argv[1], DoSNISocketConfig, nullptr);
}
