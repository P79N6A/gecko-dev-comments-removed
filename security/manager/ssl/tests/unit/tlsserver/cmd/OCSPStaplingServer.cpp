












#include <stdio.h>

#include "OCSPCommon.h"
#include "TLSServer.h"

using namespace mozilla;
using namespace mozilla::test;

const OCSPHost sOCSPHosts[] =
{
  { "ocsp-stapling-good.example.com", ORTGood, nullptr },
  { "ocsp-stapling-revoked.example.com", ORTRevoked, nullptr },
  { "ocsp-stapling-revoked-old.example.com", ORTRevokedOld, nullptr },
  { "ocsp-stapling-unknown.example.com", ORTUnknown, nullptr },
  { "ocsp-stapling-unknown-old.example.com", ORTUnknownOld, nullptr },
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
  { "ocsp-stapling-with-intermediate.example.com", ORTGood, "ocspEEWithIntermediate" },
  { "ocsp-stapling-bad-signature.example.com", ORTBadSignature, nullptr },
  { "ocsp-stapling-skip-responseBytes.example.com", ORTSkipResponseBytes, nullptr },
  { "ocsp-stapling-critical-extension.example.com", ORTCriticalExtension, nullptr },
  { "ocsp-stapling-noncritical-extension.example.com", ORTNoncriticalExtension, nullptr },
  { "ocsp-stapling-empty-extensions.example.com", ORTEmptyExtensions, nullptr },
  { "ocsp-stapling-delegated-included.example.com", ORTDelegatedIncluded, "delegatedSigner" },
  { "ocsp-stapling-delegated-included-last.example.com", ORTDelegatedIncludedLast, "delegatedSigner" },
  { "ocsp-stapling-delegated-missing.example.com", ORTDelegatedMissing, "delegatedSigner" },
  { "ocsp-stapling-delegated-missing-multiple.example.com", ORTDelegatedMissingMultiple, "delegatedSigner" },
  { "ocsp-stapling-delegated-no-extKeyUsage.example.com", ORTDelegatedIncluded, "invalidDelegatedSignerNoExtKeyUsage" },
  { "ocsp-stapling-delegated-from-intermediate.example.com", ORTDelegatedIncluded, "invalidDelegatedSignerFromIntermediate" },
  { "ocsp-stapling-delegated-keyUsage-crlSigning.example.com", ORTDelegatedIncluded, "invalidDelegatedSignerKeyUsageCrlSigning" },
  { "ocsp-stapling-delegated-wrong-extKeyUsage.example.com", ORTDelegatedIncluded, "invalidDelegatedSignerWrongExtKeyUsage" },
  { "ocsp-stapling-ancient-valid.example.com", ORTAncientAlmostExpired, nullptr},
  { "keysize-ocsp-delegated.example.com", ORTDelegatedIncluded, "rsa-1008-keysizeDelegatedSigner" },
  { "revoked-ca-cert-used-as-end-entity.example.com", ORTRevoked, "ca-used-as-end-entity" },
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

  const char *certNickname;
  if (strcmp(host->mHostName,
             "ocsp-stapling-with-intermediate.example.com") == 0) {
    certNickname = host->mAdditionalCertName;
  } else {
    certNickname = DEFAULT_CERT_NICKNAME;
  }

  ScopedCERTCertificate cert;
  SSLKEAType certKEA;
  if (SECSuccess != ConfigSecureServerWithNamedCert(aFd, certNickname,
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
