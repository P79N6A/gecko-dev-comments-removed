



#ifndef mozilla_test__TLSServer_h
#define mozilla_test__TLSServer_h









#include <stdint.h>
#include "prio.h"
#include "ScopedNSSTypes.h"
#include "secerr.h"
#include "ssl.h"

namespace mozilla { namespace test {

enum DebugLevel
{
  DEBUG_ERRORS = 1,
  DEBUG_WARNINGS  = 2,
  DEBUG_VERBOSE = 3
};

extern DebugLevel gDebugLevel;

void PrintPRError(const char *aPrefix);


extern const char DEFAULT_CERT_NICKNAME[];



SECStatus
ConfigSecureServerWithNamedCert(PRFileDesc *fd, const char *certName,
                                 ScopedCERTCertificate *cert,
                                 SSLKEAType *kea);

int
StartServer(const char *nssCertDBDir, SSLSNISocketConfig sniSocketConfig,
            void *sniSocketConfigArg);

template <typename Host>
inline const Host *
GetHostForSNI(const SECItem *aSrvNameArr, uint32_t aSrvNameArrSize,
              const Host *hosts)
{
  for (uint32_t i = 0; i < aSrvNameArrSize; i++) {
    for (const Host *host = hosts; host->mHostName; ++host) {
      SECItem hostName;
      hostName.data = reinterpret_cast<uint8_t*>(const_cast<char*>(host->mHostName));
      hostName.len = strlen(host->mHostName);
      if (SECITEM_ItemsAreEqual(&hostName, &aSrvNameArr[i])) {
        if (gDebugLevel >= DEBUG_VERBOSE) {
          fprintf(stderr, "found pre-defined host '%s'\n", host->mHostName);
        }
        return host;
      }
    }
  }

  if (gDebugLevel >= DEBUG_VERBOSE) {
    fprintf(stderr, "could not find host info from SNI\n");
  }

  PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
  return nullptr;
}

} } 

#endif 
