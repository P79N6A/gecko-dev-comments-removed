



#include "PublicKeyPinningService.h"

#include "cert.h"
#include "mozilla/Base64.h"
#include "mozilla/Telemetry.h"
#include "nsISiteSecurityService.h"
#include "nssb64.h"
#include "nsServiceManagerUtils.h"
#include "nsSiteSecurityService.h"
#include "nsString.h"
#include "nsTArray.h"
#include "pkix/pkixtypes.h"
#include "prlog.h"
#include "RootCertificateTelemetryUtils.h"
#include "ScopedNSSTypes.h"
#include "seccomon.h"
#include "sechash.h"
#include "StaticHPKPins.h" 

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
                     const StaticFingerprints* fingerprints,
                     const nsTArray<nsCString>* dynamicFingerprints)
{
  if (!fingerprints && !dynamicFingerprints) {
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

  if (fingerprints) {
    for (size_t i = 0; i < fingerprints->size; i++) {
      if (base64Out.Equals(fingerprints->data[i])) {
        PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
               ("pkpin: found pin base_64 ='%s'\n", base64Out.get()));
       return true;
      }
    }
  }
  if (dynamicFingerprints) {
    for (size_t i = 0; i < dynamicFingerprints->Length(); i++) {
      if (base64Out.Equals((*dynamicFingerprints)[i])) {
        PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
               ("pkpin: found pin base_64 ='%s'\n", base64Out.get()));
        return true;
      }
    }
  }
  return false;
}





static bool
EvalChainWithHashType(const CERTCertList* certList, SECOidTag hashType,
                      const StaticPinset* pinset,
                      const nsTArray<nsCString>* dynamicFingerprints)
{
  CERTCertificate* currentCert;

  const StaticFingerprints* fingerprints = nullptr;
  if (pinset) {
    if (hashType == SEC_OID_SHA256) {
      fingerprints = pinset->sha256;
    } else if (hashType == SEC_OID_SHA1) {
      fingerprints = pinset->sha1;
    }
  }
  if (!fingerprints && !dynamicFingerprints) {
    return false;
  }

  CERTCertListNode* node;
  for (node = CERT_LIST_HEAD(certList); !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    currentCert = node->cert;
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: certArray subject: '%s'\n", currentCert->subjectName));
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: certArray issuer: '%s'\n", currentCert->issuerName));
    if (EvalCertWithHashType(currentCert, hashType, fingerprints,
                             dynamicFingerprints)) {
      return true;
    }
  }
  PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG, ("pkpin: no matches found\n"));
  return false;
}





static bool
EvalChainWithPinset(const CERTCertList* certList,
                    const StaticPinset* pinset) {
  
  if (EvalChainWithHashType(certList, SEC_OID_SHA256, pinset, nullptr)) {
    return true;
  }
  return EvalChainWithHashType(certList, SEC_OID_SHA1, pinset, nullptr);
}




static int
TransportSecurityPreloadCompare(const void *key, const void *entry) {
  const char *keyStr = reinterpret_cast<const char *>(key);
  const TransportSecurityPreload *preloadEntry =
    reinterpret_cast<const TransportSecurityPreload *>(entry);

  return strcmp(keyStr, preloadEntry->mHost);
}

bool
PublicKeyPinningService::ChainMatchesPinset(const CERTCertList* certList,
                                            const nsTArray<nsCString>& aSHA256keys)
{
  return EvalChainWithHashType(certList, SEC_OID_SHA256, nullptr, &aSHA256keys);
}




static bool
CheckPinsForHostname(const CERTCertList *certList, const char *hostname,
                     bool enforceTestMode, mozilla::pkix::Time time)
{
  if (!certList) {
    return false;
  }
  if (!hostname || hostname[0] == 0) {
    return false;
  }

  nsCOMPtr<nsISiteSecurityService> sssService =
    do_GetService(NS_SSSERVICE_CONTRACTID);
  if (!sssService) {
    return false;
  }
  SiteHPKPState dynamicEntry;
  TransportSecurityPreload *foundEntry = nullptr;
  char *evalHost = const_cast<char*>(hostname);
  char *evalPart;
  
  while (!foundEntry && (evalPart = strchr(evalHost, '.'))) {
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: Querying pinsets for host: '%s'\n", evalHost));
    
    nsresult rv;
    bool found;
    bool includeSubdomains;
    nsTArray<nsCString> pinArray;
    rv = sssService->GetKeyPinsForHostname(evalHost, time, pinArray,
                                           &includeSubdomains, &found);
    if (NS_FAILED(rv)) {
      return false;
    }
    if (found && (evalHost == hostname || includeSubdomains)) {
      PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
             ("pkpin: Found dyn match for host: '%s'\n", evalHost));
      return EvalChainWithHashType(certList, SEC_OID_SHA256, nullptr,
                                   &pinArray);
    }

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
    if (time > TimeFromEpochInSeconds(kPreloadPKPinsExpirationTime /
                                      PR_USEC_PER_SEC)) {
      return true;
    }
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

    
    CERTCertListNode* rootNode = CERT_LIST_TAIL(certList);
    
    if (!CERT_LIST_END(rootNode, certList)) {
      if (!result) {
        AccumulateTelemetryForRootCA(Telemetry::CERT_PINNING_FAILURES_BY_CA, rootNode->cert);
      }
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
CheckChainAgainstAllNames(const CERTCertList* certList, bool enforceTestMode,
                          mozilla::pkix::Time time)
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
      if (CheckPinsForHostname(certList, hostName, enforceTestMode, time)) {
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
  if (!hostname || hostname[0] == 0) {
    return CheckChainAgainstAllNames(certList, enforceTestMode, time);
  }
  nsAutoCString canonicalizedHostname(CanonicalizeHostname(hostname));
  return CheckPinsForHostname(certList, canonicalizedHostname.get(),
                              enforceTestMode, time);
}

nsAutoCString
PublicKeyPinningService::CanonicalizeHostname(const char* hostname)
{
  nsAutoCString canonicalizedHostname(hostname);
  ToLowerCase(canonicalizedHostname);
  while (canonicalizedHostname.Length() > 0 &&
         canonicalizedHostname.Last() == '.') {
    canonicalizedHostname.Truncate(canonicalizedHostname.Length() - 1);
  }
  return canonicalizedHostname;
}
