












#include <stdio.h>

#include "OCSPCommon.h"
#include "TLSServer.h"

using namespace mozilla;
using namespace mozilla::test;

const OCSPHost sOCSPHosts[] =
{
  { "ocsp-stapling-good.example.com", OSRTGood },
  { "ocsp-stapling-revoked.example.com", OSRTRevoked },
  { "ocsp-stapling-unknown.example.com", OSRTUnknown },
  { "ocsp-stapling-good-other.example.com", OSRTGoodOtherCert },
  { "ocsp-stapling-good-other-ca.example.com", OSRTGoodOtherCA },
  { "ocsp-stapling-expired.example.com", OSRTExpired },
  { "ocsp-stapling-expired-fresh-ca.example.com", OSRTExpiredFreshCA },
  { "ocsp-stapling-none.example.com", OSRTNone },
  { "ocsp-stapling-empty.example.com", OSRTEmpty },
  { "ocsp-stapling-malformed.example.com", OSRTMalformed },
  { "ocsp-stapling-srverr.example.com", OSRTSrverr },
  { "ocsp-stapling-trylater.example.com", OSRTTryLater },
  { "ocsp-stapling-needssig.example.com", OSRTNeedsSig },
  { "ocsp-stapling-unauthorized.example.com", OSRTUnauthorized },
  { nullptr, OSRTNull }
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

  
  if (host->mOSRT == OSRTNone) {
    return 0;
  }

  PLArenaPool *arena = PORT_NewArena(1024);
  if (!arena) {
    PrintPRError("PORT_NewArena failed");
    return SSL_SNI_SEND_ALERT;
  }

  
  SECItemArray *response = GetOCSPResponseForType(host->mOSRT, cert, arena);
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
