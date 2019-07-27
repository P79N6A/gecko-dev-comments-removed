














#include <stdio.h>

#include "hasht.h"
#include "ScopedNSSTypes.h"
#include "ssl.h"
#include "TLSServer.h"

using namespace mozilla;
using namespace mozilla::test;

struct ClientAuthHost
{
  const char *mHostName;
  bool mRequestClientAuth;
  bool mRequireClientAuth;
};


static const ClientAuthHost sClientAuthHosts[] =
{
  { "noclientauth.example.com", false, false },
  { "requestclientauth.example.com", true, false },
  { "requireclientauth.example.com", true, true },
  { nullptr, nullptr }
};

static const unsigned char sClientCertFingerprint[] =
{
  0xD2, 0x2F, 0x00, 0x9A, 0x9E, 0xED, 0x79, 0xDC,
  0x8D, 0x17, 0x98, 0x8E, 0xEC, 0x76, 0x05, 0x91,
  0xA5, 0xF6, 0xC9, 0xFA, 0x16, 0x8B, 0xD2, 0x5F,
  0xE1, 0x52, 0x04, 0x7C, 0xF4, 0x76, 0x42, 0x9D
};

SECStatus
AuthCertificateHook(void* arg, PRFileDesc* fd, PRBool checkSig,
                    PRBool isServer)
{
  ScopedCERTCertificate clientCert(SSL_PeerCertificate(fd));

  unsigned char certFingerprint[SHA256_LENGTH];
  SECStatus rv = PK11_HashBuf(SEC_OID_SHA256, certFingerprint,
                              clientCert->derCert.data,
                              clientCert->derCert.len);
  if (rv != SECSuccess) {
    return rv;
  }

  static_assert(sizeof(sClientCertFingerprint) == SHA256_LENGTH,
                "Ensure fingerprint has corrent length");
  bool match = !memcmp(certFingerprint, sClientCertFingerprint,
                       sizeof(certFingerprint));
  return match ? SECSuccess : SECFailure;
}

int32_t
DoSNISocketConfig(PRFileDesc* aFd, const SECItem* aSrvNameArr,
                  uint32_t aSrvNameArrSize, void* aArg)
{
  const ClientAuthHost *host = GetHostForSNI(aSrvNameArr, aSrvNameArrSize,
                                             sClientAuthHosts);
  if (!host) {
    return SSL_SNI_SEND_ALERT;
  }

  if (gDebugLevel >= DEBUG_VERBOSE) {
    fprintf(stderr, "found pre-defined host '%s'\n", host->mHostName);
  }

  SECStatus srv = ConfigSecureServerWithNamedCert(aFd, DEFAULT_CERT_NICKNAME,
                                                  nullptr, nullptr);
  if (srv != SECSuccess) {
    return SSL_SNI_SEND_ALERT;
  }

  SSL_OptionSet(aFd, SSL_REQUEST_CERTIFICATE, host->mRequestClientAuth);
  if (host->mRequireClientAuth) {
    SSL_OptionSet(aFd, SSL_REQUIRE_CERTIFICATE, SSL_REQUIRE_ALWAYS);
  } else {
    SSL_OptionSet(aFd, SSL_REQUIRE_CERTIFICATE, SSL_REQUIRE_NEVER);
  }

  
  srv = SSL_AuthCertificateHook(aFd, AuthCertificateHook, nullptr);
  if (srv != SECSuccess) {
    return SSL_SNI_SEND_ALERT;
  }

  return 0;
}

int
main(int argc, char* argv[])
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <NSS DB directory>\n", argv[0]);
    return 1;
  }

  return StartServer(argv[1], DoSNISocketConfig, nullptr);
}
