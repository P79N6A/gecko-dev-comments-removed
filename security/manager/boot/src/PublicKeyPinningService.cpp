



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





static nsresult
GetBase64HashSPKI(const CERTCertificate* cert, SECOidTag hashType,
                  nsACString& hashSPKIDigest)
{
  hashSPKIDigest.Truncate();
  Digest digest;
  nsresult rv = digest.DigestBuf(hashType, cert->derPublicKey.data,
                                 cert->derPublicKey.len);
  if (NS_FAILED(rv)) {
    return rv;
  }
  return Base64Encode(nsDependentCSubstring(
                        reinterpret_cast<const char*>(digest.get().data),
                        digest.get().len),
                      hashSPKIDigest);
}





static nsresult
EvalCertWithHashType(const CERTCertificate* cert, SECOidTag hashType,
                     const StaticFingerprints* fingerprints,
                     const nsTArray<nsCString>* dynamicFingerprints,
              bool& certMatchesPinset)
{
  certMatchesPinset = false;
  if (!fingerprints && !dynamicFingerprints) {
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: No hashes found for hash type: %d\n", hashType));
    return NS_ERROR_INVALID_ARG;
  }

  nsAutoCString base64Out;
  nsresult rv = GetBase64HashSPKI(cert, hashType, base64Out);
  if (NS_FAILED(rv)) {
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: GetBase64HashSPKI failed!\n"));
    return rv;
  }

  if (fingerprints) {
    for (size_t i = 0; i < fingerprints->size; i++) {
      if (base64Out.Equals(fingerprints->data[i])) {
        PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
               ("pkpin: found pin base_64 ='%s'\n", base64Out.get()));
        certMatchesPinset = true;
        return NS_OK;
      }
    }
  }
  if (dynamicFingerprints) {
    for (size_t i = 0; i < dynamicFingerprints->Length(); i++) {
      if (base64Out.Equals((*dynamicFingerprints)[i])) {
        PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
               ("pkpin: found pin base_64 ='%s'\n", base64Out.get()));
        certMatchesPinset = true;
        return NS_OK;
      }
    }
  }
  return NS_OK;
}





static nsresult
EvalChainWithHashType(const CERTCertList* certList, SECOidTag hashType,
                      const StaticPinset* pinset,
                      const nsTArray<nsCString>* dynamicFingerprints,
               bool& certListIntersectsPinset)
{
  certListIntersectsPinset = false;
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
    return NS_OK;
  }

  CERTCertListNode* node;
  for (node = CERT_LIST_HEAD(certList); !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    currentCert = node->cert;
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: certArray subject: '%s'\n", currentCert->subjectName));
    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: certArray issuer: '%s'\n", currentCert->issuerName));
    nsresult rv = EvalCertWithHashType(currentCert, hashType, fingerprints,
                                       dynamicFingerprints,
                                       certListIntersectsPinset);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (certListIntersectsPinset) {
      return NS_OK;
    }
  }
  PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG, ("pkpin: no matches found\n"));
  return NS_OK;
}





static nsresult
EvalChainWithPinset(const CERTCertList* certList,
                    const StaticPinset* pinset,
             bool& certListIntersectsPinset)
{
  certListIntersectsPinset = false;
  
  nsresult rv = EvalChainWithHashType(certList, SEC_OID_SHA256, pinset,
                                      nullptr, certListIntersectsPinset);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (certListIntersectsPinset) {
    return NS_OK;
  }
  return EvalChainWithHashType(certList, SEC_OID_SHA1, pinset, nullptr,
                               certListIntersectsPinset);
}




static int
TransportSecurityPreloadCompare(const void *key, const void *entry) {
  const char *keyStr = reinterpret_cast<const char *>(key);
  const TransportSecurityPreload *preloadEntry =
    reinterpret_cast<const TransportSecurityPreload *>(entry);

  return strcmp(keyStr, preloadEntry->mHost);
}

nsresult
PublicKeyPinningService::ChainMatchesPinset(const CERTCertList* certList,
                                            const nsTArray<nsCString>& aSHA256keys,
                                     bool& chainMatchesPinset)
{
  return EvalChainWithHashType(certList, SEC_OID_SHA256, nullptr, &aSHA256keys,
                               chainMatchesPinset);
}




static nsresult
FindPinningInformation(const char* hostname, mozilla::pkix::Time time,
                nsTArray<nsCString>& dynamicFingerprints,
                TransportSecurityPreload*& staticFingerprints)
{
  if (!hostname || hostname[0] == 0) {
    return NS_ERROR_INVALID_ARG;
  }
  staticFingerprints = nullptr;
  dynamicFingerprints.Clear();
  nsCOMPtr<nsISiteSecurityService> sssService =
    do_GetService(NS_SSSERVICE_CONTRACTID);
  if (!sssService) {
    return NS_ERROR_FAILURE;
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
      return rv;
    }
    if (found && (evalHost == hostname || includeSubdomains)) {
      PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
             ("pkpin: Found dyn match for host: '%s'\n", evalHost));
      dynamicFingerprints = pinArray;
      return NS_OK;
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
      return NS_OK;
    }
    staticFingerprints = foundEntry;
  }
  return NS_OK;
}






static nsresult
CheckPinsForHostname(const CERTCertList* certList, const char* hostname,
                     bool enforceTestMode, mozilla::pkix::Time time,
              bool& chainHasValidPins)
{
  chainHasValidPins = false;
  if (!certList) {
    return NS_ERROR_INVALID_ARG;
  }
  if (!hostname || hostname[0] == 0) {
    return NS_ERROR_INVALID_ARG;
  }

  nsTArray<nsCString> dynamicFingerprints;
  TransportSecurityPreload* staticFingerprints = nullptr;
  nsresult rv = FindPinningInformation(hostname, time, dynamicFingerprints,
                                       staticFingerprints);
  
  
  if (dynamicFingerprints.Length() == 0 && !staticFingerprints) {
    chainHasValidPins = true;
    return NS_OK;
  }
  if (dynamicFingerprints.Length() > 0) {
    return EvalChainWithHashType(certList, SEC_OID_SHA256, nullptr,
                                 &dynamicFingerprints, chainHasValidPins);
  }
  if (staticFingerprints) {
    bool enforceTestModeResult;
    rv = EvalChainWithPinset(certList, staticFingerprints->pinset,
                             enforceTestModeResult);
    if (NS_FAILED(rv)) {
      return rv;
    }
    chainHasValidPins = enforceTestModeResult;
    Telemetry::ID histogram = staticFingerprints->mIsMoz
      ? Telemetry::CERT_PINNING_MOZ_RESULTS
      : Telemetry::CERT_PINNING_RESULTS;
    if (staticFingerprints->mTestMode) {
      histogram = staticFingerprints->mIsMoz
        ? Telemetry::CERT_PINNING_MOZ_TEST_RESULTS
        : Telemetry::CERT_PINNING_TEST_RESULTS;
      if (!enforceTestMode) {
        chainHasValidPins = true;
      }
    }
    
    
    if (staticFingerprints->mId != kUnknownId) {
      int32_t bucket = staticFingerprints->mId * 2 + (enforceTestModeResult ? 1 : 0);
      histogram = staticFingerprints->mTestMode
        ? Telemetry::CERT_PINNING_MOZ_TEST_RESULTS_BY_HOST
        : Telemetry::CERT_PINNING_MOZ_RESULTS_BY_HOST;
      Telemetry::Accumulate(histogram, bucket);
    } else {
      Telemetry::Accumulate(histogram, enforceTestModeResult ? 1 : 0);
    }

    
    CERTCertListNode* rootNode = CERT_LIST_TAIL(certList);
    
    if (!CERT_LIST_END(rootNode, certList)) {
      if (!enforceTestModeResult) {
        AccumulateTelemetryForRootCA(Telemetry::CERT_PINNING_FAILURES_BY_CA, rootNode->cert);
      }
    }

    PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
           ("pkpin: Pin check %s for %s host '%s' (mode=%s)\n",
            enforceTestModeResult ? "passed" : "failed",
            staticFingerprints->mIsMoz ? "mozilla" : "non-mozilla",
            hostname, staticFingerprints->mTestMode ? "test" : "production"));
  }

  return NS_OK;
}






static nsresult
CheckChainAgainstAllNames(const CERTCertList* certList, bool enforceTestMode,
                          mozilla::pkix::Time time,
                   bool& chainHasValidPins)
{
  chainHasValidPins = false;
  PR_LOG(gPublicKeyPinningLog, PR_LOG_DEBUG,
         ("pkpin: top of checkChainAgainstAllNames"));
  CERTCertListNode* node = CERT_LIST_HEAD(certList);
  if (!node) {
    return NS_ERROR_INVALID_ARG;
  }
  CERTCertificate* cert = node->cert;
  if (!cert) {
    return NS_ERROR_INVALID_ARG;
  }

  ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return NS_ERROR_FAILURE;
  }

  CERTGeneralName* nameList;
  CERTGeneralName* currentName;
  nameList = CERT_GetConstrainedCertificateNames(cert, arena.get(), PR_TRUE);
  if (!nameList) {
    return NS_ERROR_FAILURE;
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
      nsAutoCString canonicalizedHostname(
        PublicKeyPinningService::CanonicalizeHostname(hostName));
      nsresult rv = CheckPinsForHostname(certList, canonicalizedHostname.get(),
                                         enforceTestMode, time,
                                         chainHasValidPins);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (chainHasValidPins) {
        return NS_OK;
      }
    }
    currentName = CERT_GetNextGeneralName(currentName);
  } while (currentName != nameList);

  return NS_OK;
}

nsresult
PublicKeyPinningService::ChainHasValidPins(const CERTCertList* certList,
                                           const char* hostname,
                                           mozilla::pkix::Time time,
                                           bool enforceTestMode,
                                    bool& chainHasValidPins)
{
  chainHasValidPins = false;
  if (!certList) {
    return NS_ERROR_INVALID_ARG;
  }
  if (!hostname || hostname[0] == 0) {
    return CheckChainAgainstAllNames(certList, enforceTestMode, time,
                                     chainHasValidPins);
  }
  nsAutoCString canonicalizedHostname(CanonicalizeHostname(hostname));
  return CheckPinsForHostname(certList, canonicalizedHostname.get(),
                              enforceTestMode, time, chainHasValidPins);
}

nsresult
PublicKeyPinningService::HostHasPins(const char* hostname,
                                     mozilla::pkix::Time time,
                                     bool enforceTestMode,
                                      bool& hostHasPins)
{
  hostHasPins = false;
  nsAutoCString canonicalizedHostname(CanonicalizeHostname(hostname));
  nsTArray<nsCString> dynamicFingerprints;
  TransportSecurityPreload* staticFingerprints = nullptr;
  nsresult rv = FindPinningInformation(canonicalizedHostname.get(), time,
                                       dynamicFingerprints, staticFingerprints);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (dynamicFingerprints.Length() > 0) {
    hostHasPins = true;
  } else if (staticFingerprints) {
    hostHasPins = !staticFingerprints->mTestMode || enforceTestMode;
  }
  return NS_OK;
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
