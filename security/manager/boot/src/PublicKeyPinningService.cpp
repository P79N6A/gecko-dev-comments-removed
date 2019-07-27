



#include "PublicKeyPinningService.h"
#include "pkix/nullptr.h"
#include "StaticHPKPins.h" 

#include "cert.h"
#include "mozilla/Base64.h"
#include "mozilla/Telemetry.h"
#include "nsString.h"
#include "nssb64.h"
#include "pkix/pkixtypes.h"
#include "prlog.h"
#include "ScopedNSSTypes.h"
#include "seccomon.h"
#include "sechash.h"

using namespace mozilla;
using namespace mozilla::pkix;
using namespace mozilla::psm;

#if defined(PR_LOGGING)
PRLogModuleInfo* gPublicKeyPinningLog =
  PR_NewLogModule("PublicKeyPinningService");
#endif





static SECStatus
GetBase64HashSPKI(const CERTCertificate* cert, SECOidTag hashType,
                  nsACString& hashSPKIDigest)
{
  hashSPKIDigest.Truncate();
  Digest digest;
  nsresult rv = digest.DigestBuf(hashType, cert->derPublicKey.data,
                                 cert->derPublicKey.len);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return SECFailure;
  }
  rv = Base64Encode(nsDependentCSubstring(
                      reinterpret_cast<const char*>(digest.get().data),
                      digest.get().len),
                      hashSPKIDigest);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return SECFailure;
  }
  return SECSuccess;
}





static bool
EvalCertWithHashType(const CERTCertificate* cert, SECOidTag hashType,
                     const StaticFingerprints* fingerprints)
{
  if (!fingerprints) {
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: No hashes found for hash type: %d\n", hashType));
    return false;
  }

  nsAutoCString base64Out;
  SECStatus srv = GetBase64HashSPKI(cert, hashType, base64Out);
  if (srv != SECSuccess) {
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: GetBase64HashSPKI failed!\n"));
    return false;
  }

  for (size_t i = 0; i < fingerprints->size; i++) {
    if (base64Out.Equals(fingerprints->data[i])) {
      PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
             ("pkpin: found pin base_64 ='%s'\n", base64Out.get()));
      return true;
    }
  }
  return false;
}





static bool
EvalChainWithHashType(const CERTCertList* certList, SECOidTag hashType,
                      const StaticPinset* pinset)
{
  CERTCertificate* currentCert;

  const StaticFingerprints* fingerprints = nullptr;
  if (hashType == SEC_OID_SHA256) {
    fingerprints = pinset->sha256;
  } else if (hashType == SEC_OID_SHA1) {
    fingerprints = pinset->sha1;
  }
  if (!fingerprints) {
    return false;
  }

  CERTCertListNode* node;
  for (node = CERT_LIST_HEAD(certList); !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    currentCert = node->cert;
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: certArray subject: '%s'\n",
            currentCert->subjectName));
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: certArray common_name: '%s'\n",
            CERT_GetCommonName(&(currentCert->issuer))));
    if (EvalCertWithHashType(currentCert, hashType, fingerprints)) {
      return true;
    }
  }
  PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG, ("pkpin: no matches found\n"));
  return false;
}





static bool
EvalChainWithPinset(const CERTCertList* certList,
                    const StaticPinset* pinset) {
  
  if (EvalChainWithHashType(certList, SEC_OID_SHA256, pinset)) {
    return true;
  }
  return EvalChainWithHashType(certList, SEC_OID_SHA1, pinset);
}




static int
TransportSecurityPreloadCompare(const void *key, const void *entry) {
  const char *keyStr = reinterpret_cast<const char *>(key);
  const TransportSecurityPreload *preloadEntry =
    reinterpret_cast<const TransportSecurityPreload *>(entry);

  return strcmp(keyStr, preloadEntry->mHost);
}




static bool
CheckPinsForHostname(const CERTCertList *certList, const char *hostname,
                     bool enforceTestMode)
{
  if (!certList) {
    return false;
  }
  if (!hostname || hostname[0] == 0) {
    return false;
  }

  TransportSecurityPreload *foundEntry = nullptr;
  char *evalHost = const_cast<char*>(hostname);
  char *evalPart;
  
  while (!foundEntry && (evalPart = strchr(evalHost, '.'))) {
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: Querying pinsets for host: '%s'\n", evalHost));
    foundEntry = (TransportSecurityPreload *)bsearch(evalHost,
      kPublicKeyPinningPreloadList,
      sizeof(kPublicKeyPinningPreloadList) / sizeof(TransportSecurityPreload),
      sizeof(TransportSecurityPreload),
      TransportSecurityPreloadCompare);
    if (foundEntry) {
      PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
             ("pkpin: Found pinset for host: '%s'\n", evalHost));
      if (evalHost != hostname) {
        if (!foundEntry->mIncludeSubdomains) {
          
          foundEntry = nullptr;
        }
      }
    } else {
      PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
             ("pkpin: Didn't find pinset for host: '%s'\n", evalHost));
    }
    
    evalHost = evalPart + 1;
  }

  if (foundEntry && foundEntry->pinset) {
    bool result = EvalChainWithPinset(certList, foundEntry->pinset);
    bool retval = result;
    Telemetry::ID histogram = foundEntry->mIsMoz
      ? Telemetry::CERT_PINNING_MOZ_RESULTS
      : Telemetry::CERT_PINNING_RESULTS;
    if (foundEntry->mTestMode) {
      histogram = foundEntry->mIsMoz
        ? Telemetry::CERT_PINNING_MOZ_TEST_RESULTS
        : Telemetry::CERT_PINNING_TEST_RESULTS;
      if (!enforceTestMode) {
        retval = true;
      }
    }
    
    
    if (foundEntry->mId != kUnknownId) {
      int32_t bucket = foundEntry->mId * 2 + (result ? 1 : 0);
      histogram = foundEntry->mTestMode
        ? Telemetry::CERT_PINNING_MOZ_TEST_RESULTS_BY_HOST
        : Telemetry::CERT_PINNING_MOZ_RESULTS_BY_HOST;
      Telemetry::Accumulate(histogram, bucket);
    } else {
      Telemetry::Accumulate(histogram, result ? 1 : 0);
    }
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: Pin check %s for %s host '%s' (mode=%s)\n",
            result ? "passed" : "failed",
            foundEntry->mIsMoz ? "mozilla" : "non-mozilla",
            hostname, foundEntry->mTestMode ? "test" : "production"));
    return retval;
  }
  return true; 
}






static bool
CheckChainAgainstAllNames(const CERTCertList* certList, bool enforceTestMode)
{
  PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
         ("pkpin: top of checkChainAgainstAllNames"));
  CERTCertListNode* node = CERT_LIST_HEAD(certList);
  if (!node) {
    return false;
  }
  CERTCertificate* cert = node->cert;
  if (!cert) {
    return false;
  }

  ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return false;
  }

  bool hasValidPins = false;
  CERTGeneralName* nameList;
  CERTGeneralName* currentName;
  nameList = CERT_GetConstrainedCertificateNames(cert, arena.get(), PR_TRUE);
  if (!nameList) {
    return false;
  }

  currentName = nameList;
  do {
    if (currentName->type == certDNSName
        && currentName->name.other.data[0] != 0) {
      
      char *hostName = (char *)PORT_ArenaAlloc(arena.get(),
                                               currentName->name.other.len + 1);
      if (!hostName) {
        break;
      }
      
      
      hostName[currentName->name.other.len] = 0;
      memcpy(hostName, currentName->name.other.data,
             currentName->name.other.len);
      if (!hostName[0]) {
        
        break;
      }
      if (CheckPinsForHostname(certList, hostName, enforceTestMode)) {
        hasValidPins = true;
        break;
      }
    }
    currentName = CERT_GetNextGeneralName(currentName);
  } while (currentName != nameList);

  return hasValidPins;
}

bool
PublicKeyPinningService::ChainHasValidPins(const CERTCertList* certList,
                                           const char* hostname,
                                           mozilla::pkix::Time time,
                                           bool enforceTestMode)
{
  if (!certList) {
    return false;
  }
  if (time > TimeFromEpochInSeconds(kPreloadPKPinsExpirationTime /
                                    PR_USEC_PER_SEC)) {
    return true;
  }
  if (!hostname || hostname[0] == 0) {
    return CheckChainAgainstAllNames(certList, enforceTestMode);
  }
  return CheckPinsForHostname(certList, hostname, enforceTestMode);
}
