












#include <stdio.h>

#include "OCSPCommon.h"
#include "TLSServer.h"

using namespace mozilla;
using namespace mozilla::test;

const OCSPHost sOCSPHosts[] =
{
  { "ocsp-stapling-good.example.com", ORTGood, nullptr },
  { "ocsp-stapling-revoked.example.com", ORTRevoked, nullptr },
  { "ocsp-stapling-unknown.example.com", ORTUnknown, nullptr },
  { "ocsp-stapling-good-other.example.com", ORTGoodOtherCert, "ocspOtherEndEntity" },
  { "ocsp-stapling-good-other-ca.example.com", ORTGoodOtherCA, "otherCA" },
  { "ocsp-stapling-expired.example.com", ORTExpired, nullptr },
  { "ocsp-stapling-expired-fresh-ca.example.com", ORTExpiredFreshCA, nullptr },
  { "ocsp-stapling-none.example.com", ORTNone, nullptr },
  { "ocsp-stapling-empty.example.com", ORTEmpty, nullptr },
  { "ocsp-stapling-malformed.example.com", ORTMalformed, nullptr },
  { "ocsp-stapling-srverr.example.com", ORTSrverr, nullptr },
  { "ocsp-stapling-trylater.example.com", ORTTryLater, nullptr },
  { "ocsp-stapling-needssig.example.com", ORTNeedsSig, nullptr },
  { "ocsp-stapling-unauthorized.example.com", ORTUnauthorized, nullptr },
  { nullptr, ORTNull, nullptr }
};

int32_t
DoSNISocketConfig(PRFileDesc *aFd, const SECItem *aSrvNameArr,
                  uint32_t aSrvNameArrSize, void *aArg)
{
  const OCSPHost *host = GetHostForSNI(aSrvNameArr, aSrvNameArrSize,
                                       sOCSPHosts);
  if (!host) {
    return SSL_SNI_SEND_ALERT;
  }

  if (gDebugLevel >= DEBUG_VERBOSE) {
    fprintf(stderr, "found pre-defined host '%s'\n", host->mHostName);
  }

  ScopedCERTCertificate cert;
  SSLKEAType certKEA;
  if (SECSuccess != ConfigSecureServerWithNamedCert(aFd, DEFAULT_CERT_NICKNAME,
                                                    &cert, &certKEA)) {
    return SSL_SNI_SEND_ALERT;
  }

  
  if (host->mORT == ORTNone) {
    return 0;
  }

  PLArenaPool *arena = PORT_NewArena(1024);
  if (!arena) {
    PrintPRError("PORT_NewArena failed");
    return SSL_SNI_SEND_ALERT;
  }

  
  SECItemArray *response = GetOCSPResponseForType(host->mORT, cert, arena,
                                                  host->mAdditionalCertName);
  if (!response) {
    PORT_FreeArena(arena, PR_FALSE);
    return SSL_SNI_SEND_ALERT;
  }

  
  SECStatus st = SSL_SetStapledOCSPResponses(aFd, response, certKEA);
  PORT_FreeArena(arena, PR_FALSE);
  if (st != SECSuccess) {
    PrintPRError("SSL_SetStapledOCSPResponses failed");
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
