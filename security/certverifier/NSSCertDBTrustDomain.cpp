





#include "NSSCertDBTrustDomain.h"

#include <stdint.h>

#include "ExtendedValidation.h"
#include "OCSPRequestor.h"
#include "certdb.h"
#include "cert.h"
#include "mozilla/UniquePtr.h"
#include "nsNSSCertificate.h"
#include "nss.h"
#include "NSSErrorsService.h"
#include "nsServiceManagerUtils.h"
#include "pk11pub.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "prerror.h"
#include "prmem.h"
#include "prprf.h"
#include "ScopedNSSTypes.h"
#include "secerr.h"

#include "CNNICHashWhitelist.inc"

using namespace mozilla;
using namespace mozilla::pkix;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gCertVerifierLog;
#endif

static const uint64_t ServerFailureDelaySeconds = 5 * 60;

namespace mozilla { namespace psm {

const char BUILTIN_ROOTS_MODULE_DEFAULT_NAME[] = "Builtin Roots Module";

NSSCertDBTrustDomain::NSSCertDBTrustDomain(SECTrustType certDBTrustType,
                                           OCSPFetching ocspFetching,
                                           OCSPCache& ocspCache,
              void* pinArg,
                                           CertVerifier::OcspGetConfig ocspGETConfig,
                                           CertVerifier::PinningMode pinningMode,
                                           unsigned int minRSABits,
                               const char* hostname,
                               ScopedCERTCertList* builtChain)
  : mCertDBTrustType(certDBTrustType)
  , mOCSPFetching(ocspFetching)
  , mOCSPCache(ocspCache)
  , mPinArg(pinArg)
  , mOCSPGetConfig(ocspGETConfig)
  , mPinningMode(pinningMode)
  , mMinRSABits(minRSABits)
  , mHostname(hostname)
  , mBuiltChain(builtChain)
  , mCertBlocklist(do_GetService(NS_CERTBLOCKLIST_CONTRACTID))
  , mOCSPStaplingStatus(CertVerifier::OCSP_STAPLING_NEVER_CHECKED)
{
}



static Result
FindIssuerInner(ScopedCERTCertList& candidates, bool useRoots,
                Input encodedIssuerName, TrustDomain::IssuerChecker& checker,
                 bool& keepGoing)
{
  keepGoing = true;
  for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
       !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
    bool candidateIsRoot = !!n->cert->isRoot;
    if (candidateIsRoot != useRoots) {
      continue;
    }
    Input certDER;
    Result rv = certDER.Init(n->cert->derCert.data, n->cert->derCert.len);
    if (rv != Success) {
      continue; 
    }

    const SECItem encodedIssuerNameItem = {
      siBuffer,
      const_cast<unsigned char*>(encodedIssuerName.UnsafeGetData()),
      encodedIssuerName.GetLength()
    };
    ScopedSECItem nameConstraints(::SECITEM_AllocItem(nullptr, nullptr, 0));
    SECStatus srv = CERT_GetImposedNameConstraints(&encodedIssuerNameItem,
                                                   nameConstraints.get());
    if (srv != SECSuccess) {
      if (PR_GetError() != SEC_ERROR_EXTENSION_NOT_FOUND) {
        return Result::FATAL_ERROR_LIBRARY_FAILURE;
      }

      
      rv = checker.Check(certDER, nullptr, keepGoing);
    } else {
      
      Input nameConstraintsInput;
      if (nameConstraintsInput.Init(
              nameConstraints->data,
              nameConstraints->len) != Success) {
        return Result::FATAL_ERROR_LIBRARY_FAILURE;
      }
      rv = checker.Check(certDER, &nameConstraintsInput, keepGoing);
    }
    if (rv != Success) {
      return rv;
    }
    if (!keepGoing) {
      break;
    }
  }

  return Success;
}

Result
NSSCertDBTrustDomain::FindIssuer(Input encodedIssuerName,
                                 IssuerChecker& checker, Time)
{
  
  
  SECItem encodedIssuerNameItem = UnsafeMapInputToSECItem(encodedIssuerName);
  ScopedCERTCertList
    candidates(CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                          &encodedIssuerNameItem, 0,
                                          false));
  if (candidates) {
    
    bool keepGoing;
    Result rv = FindIssuerInner(candidates, true, encodedIssuerName, checker,
                                keepGoing);
    if (rv != Success) {
      return rv;
    }
    if (keepGoing) {
      rv = FindIssuerInner(candidates, false, encodedIssuerName, checker,
                           keepGoing);
      if (rv != Success) {
        return rv;
      }
    }
  }

  return Success;
}

Result
NSSCertDBTrustDomain::GetCertTrust(EndEntityOrCA endEntityOrCA,
                                   const CertPolicyId& policy,
                                   Input candidateCertDER,
                                    TrustLevel& trustLevel)
{
#ifdef MOZ_NO_EV_CERTS
  if (!policy.IsAnyPolicy()) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }
#endif

  
  
  
  
  
  
  SECItem candidateCertDERSECItem = UnsafeMapInputToSECItem(candidateCertDER);
  ScopedCERTCertificate candidateCert(
    CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &candidateCertDERSECItem,
                            nullptr, false, true));
  if (!candidateCert) {
    return MapPRErrorCodeToResult(PR_GetError());
  }

  
  if (!mCertBlocklist) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  bool isCertRevoked;
  nsresult nsrv = mCertBlocklist->IsCertRevoked(
                    candidateCert->derIssuer.data,
                    candidateCert->derIssuer.len,
                    candidateCert->serialNumber.data,
                    candidateCert->serialNumber.len,
                    candidateCert->derSubject.data,
                    candidateCert->derSubject.len,
                    candidateCert->derPublicKey.data,
                    candidateCert->derPublicKey.len,
                    &isCertRevoked);
  if (NS_FAILED(nsrv)) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  if (isCertRevoked) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: certificate is in blocklist"));
    return Result::ERROR_REVOKED_CERTIFICATE;
  }

  
  
  
  
  
  CERTCertTrust trust;
  if (CERT_GetCertTrust(candidateCert.get(), &trust) == SECSuccess) {
    uint32_t flags = SEC_GET_TRUST_FLAGS(&trust, mCertDBTrustType);

    
    
    
    
    
    uint32_t relevantTrustBit =
      endEntityOrCA == EndEntityOrCA::MustBeCA ? CERTDB_TRUSTED_CA
                                               : CERTDB_TRUSTED;
    if (((flags & (relevantTrustBit|CERTDB_TERMINAL_RECORD)))
            == CERTDB_TERMINAL_RECORD) {
      trustLevel = TrustLevel::ActivelyDistrusted;
      return Success;
    }

    
    
    
    if (flags & CERTDB_TRUSTED_CA) {
      if (policy.IsAnyPolicy()) {
        trustLevel = TrustLevel::TrustAnchor;
        return Success;
      }
#ifndef MOZ_NO_EV_CERTS
      if (CertIsAuthoritativeForEVPolicy(candidateCert.get(), policy)) {
        trustLevel = TrustLevel::TrustAnchor;
        return Success;
      }
#endif
    }
  }

  trustLevel = TrustLevel::InheritsTrust;
  return Success;
}

Result
NSSCertDBTrustDomain::DigestBuf(Input item, DigestAlgorithm digestAlg,
                                 uint8_t* digestBuf, size_t digestBufLen)
{
  return DigestBufNSS(item, digestAlg, digestBuf, digestBufLen);
}


static PRIntervalTime
OCSPFetchingTypeToTimeoutTime(NSSCertDBTrustDomain::OCSPFetching ocspFetching)
{
  switch (ocspFetching) {
    case NSSCertDBTrustDomain::FetchOCSPForDVSoftFail:
      return PR_SecondsToInterval(2);
    case NSSCertDBTrustDomain::FetchOCSPForEV:
    case NSSCertDBTrustDomain::FetchOCSPForDVHardFail:
      return PR_SecondsToInterval(10);
    
    
    case NSSCertDBTrustDomain::NeverFetchOCSP:
    case NSSCertDBTrustDomain::LocalOnlyOCSPForEV:
      PR_NOT_REACHED("we should never see this OCSPFetching type here");
    default:
      PR_NOT_REACHED("we're not handling every OCSPFetching type");
  }
  return PR_SecondsToInterval(2);
}






static Result
GetOCSPAuthorityInfoAccessLocation(PLArenaPool* arena,
                                   Input aiaExtension,
                                    char const*& url)
{
  url = nullptr;
  SECItem aiaExtensionSECItem = UnsafeMapInputToSECItem(aiaExtension);
  CERTAuthInfoAccess** aia =
    CERT_DecodeAuthInfoAccessExtension(arena, &aiaExtensionSECItem);
  if (!aia) {
    return Result::ERROR_CERT_BAD_ACCESS_LOCATION;
  }
  for (size_t i = 0; aia[i]; ++i) {
    if (SECOID_FindOIDTag(&aia[i]->method) == SEC_OID_PKIX_OCSP) {
      
      CERTGeneralName* current = aia[i]->location;
      if (!current) {
        continue;
      }
      do {
        if (current->type == certURI) {
          const SECItem& location = current->name.other;
          
          
          if (location.len > 1024 || memchr(location.data, 0, location.len)) {
            
            return Result::ERROR_CERT_BAD_ACCESS_LOCATION;
          }
          
          char* nullTerminatedURL(static_cast<char*>(
                                    PORT_ArenaAlloc(arena, location.len + 1)));
          if (!nullTerminatedURL) {
            return Result::FATAL_ERROR_NO_MEMORY;
          }
          memcpy(nullTerminatedURL, location.data, location.len);
          nullTerminatedURL[location.len] = 0;
          url = nullTerminatedURL;
          return Success;
        }
        current = CERT_GetNextGeneralName(current);
      } while (current != aia[i]->location);
    }
  }

  return Success;
}

Result
NSSCertDBTrustDomain::CheckRevocation(EndEntityOrCA endEntityOrCA,
                                      const CertID& certID, Time time,
                          const Input* stapledOCSPResponse,
                          const Input* aiaExtension)
{
  
  

  
  

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("NSSCertDBTrustDomain: Top of CheckRevocation\n"));

  
  
  
  
  uint16_t maxOCSPLifetimeInDays = 10;
  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    maxOCSPLifetimeInDays = 365;
  }

  
  
  
  
  
  
  
  
  
  
  Result stapledOCSPResponseResult = Success;
  if (stapledOCSPResponse) {
    PR_ASSERT(endEntityOrCA == EndEntityOrCA::MustBeEndEntity);
    bool expired;
    stapledOCSPResponseResult =
      VerifyAndMaybeCacheEncodedOCSPResponse(certID, time,
                                             maxOCSPLifetimeInDays,
                                             *stapledOCSPResponse,
                                             ResponseWasStapled, expired);
    if (stapledOCSPResponseResult == Success) {
      
      mOCSPStaplingStatus = CertVerifier::OCSP_STAPLING_GOOD;
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: stapled OCSP response: good"));
      return Success;
    }
    if (stapledOCSPResponseResult == Result::ERROR_OCSP_OLD_RESPONSE ||
        expired) {
      
      mOCSPStaplingStatus = CertVerifier::OCSP_STAPLING_EXPIRED;
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: expired stapled OCSP response"));
    } else {
      
      mOCSPStaplingStatus = CertVerifier::OCSP_STAPLING_INVALID;
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: stapled OCSP response: failure"));
      return stapledOCSPResponseResult;
    }
  } else if (endEntityOrCA == EndEntityOrCA::MustBeEndEntity) {
    
    mOCSPStaplingStatus = CertVerifier::OCSP_STAPLING_NONE;
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: no stapled OCSP response"));
  }

  Result cachedResponseResult = Success;
  Time cachedResponseValidThrough(Time::uninitialized);
  bool cachedResponsePresent = mOCSPCache.Get(certID,
                                              cachedResponseResult,
                                              cachedResponseValidThrough);
  if (cachedResponsePresent) {
    if (cachedResponseResult == Success && cachedResponseValidThrough >= time) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: cached OCSP response: good"));
      return Success;
    }
    
    if (cachedResponseResult == Result::ERROR_REVOKED_CERTIFICATE) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: cached OCSP response: revoked"));
      return Result::ERROR_REVOKED_CERTIFICATE;
    }
    
    
    
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: cached OCSP response: error %ld valid "
           "until %lld", cachedResponseResult, cachedResponseValidThrough));
    
    
    
    if (cachedResponseResult == Success && cachedResponseValidThrough < time) {
      cachedResponseResult = Result::ERROR_OCSP_OLD_RESPONSE;
    }
    
    
    if (cachedResponseResult != Success &&
        cachedResponseResult != Result::ERROR_OCSP_UNKNOWN_CERT &&
        cachedResponseResult != Result::ERROR_OCSP_OLD_RESPONSE &&
        cachedResponseValidThrough < time) {
      cachedResponseResult = Success;
      cachedResponsePresent = false;
    }
  } else {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: no cached OCSP response"));
  }
  
  
  PR_ASSERT((!cachedResponsePresent && cachedResponseResult == Success) ||
            (cachedResponsePresent && cachedResponseResult != Success));

  
  bool blocklistIsFresh;
  nsresult nsrv = mCertBlocklist->IsBlocklistFresh(&blocklistIsFresh);
  if (NS_FAILED(nsrv)) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  
  
  
  

  if ((mOCSPFetching == NeverFetchOCSP) ||
      (endEntityOrCA == EndEntityOrCA::MustBeCA &&
       (mOCSPFetching == FetchOCSPForDVHardFail ||
        mOCSPFetching == FetchOCSPForDVSoftFail ||
        blocklistIsFresh))) {
    
    
    if (cachedResponseResult == Result::ERROR_OCSP_UNKNOWN_CERT) {
      return Result::ERROR_OCSP_UNKNOWN_CERT;
    }
    
    
    if (mOCSPFetching == FetchOCSPForDVHardFail &&
        cachedResponseResult == Result::ERROR_OCSP_OLD_RESPONSE) {
      return Result::ERROR_OCSP_OLD_RESPONSE;
    }

    return Success;
  }

  if (mOCSPFetching == LocalOnlyOCSPForEV) {
    if (cachedResponseResult != Success) {
      return cachedResponseResult;
    }
    return Result::ERROR_OCSP_UNKNOWN_CERT;
  }

  ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return Result::FATAL_ERROR_NO_MEMORY;
  }

  Result rv;
  const char* url = nullptr; 

  if (aiaExtension) {
    rv = GetOCSPAuthorityInfoAccessLocation(arena.get(), *aiaExtension, url);
    if (rv != Success) {
      return rv;
    }
  }

  if (!url) {
    if (mOCSPFetching == FetchOCSPForEV ||
        cachedResponseResult == Result::ERROR_OCSP_UNKNOWN_CERT) {
      return Result::ERROR_OCSP_UNKNOWN_CERT;
    }
    if (cachedResponseResult == Result::ERROR_OCSP_OLD_RESPONSE) {
      return Result::ERROR_OCSP_OLD_RESPONSE;
    }
    if (stapledOCSPResponseResult != Success) {
      return stapledOCSPResponseResult;
    }

    
    
    
    
    return Success;
  }

  
  
  Input response;
  bool attemptedRequest;
  if (cachedResponseResult == Success ||
      cachedResponseResult == Result::ERROR_OCSP_UNKNOWN_CERT ||
      cachedResponseResult == Result::ERROR_OCSP_OLD_RESPONSE) {
    uint8_t ocspRequest[OCSP_REQUEST_MAX_LENGTH];
    size_t ocspRequestLength;
    rv = CreateEncodedOCSPRequest(*this, certID, ocspRequest,
                                  ocspRequestLength);
    if (rv != Success) {
      return rv;
    }
    SECItem ocspRequestItem = {
      siBuffer,
      ocspRequest,
      static_cast<unsigned int>(ocspRequestLength)
    };
    
    const SECItem* responseSECItem =
      DoOCSPRequest(arena.get(), url, &ocspRequestItem,
                    OCSPFetchingTypeToTimeoutTime(mOCSPFetching),
                    mOCSPGetConfig == CertVerifier::ocspGetEnabled);
    if (!responseSECItem) {
      rv = MapPRErrorCodeToResult(PR_GetError());
    } else if (response.Init(responseSECItem->data, responseSECItem->len)
                 != Success) {
      rv = Result::ERROR_OCSP_MALFORMED_RESPONSE; 
    }
    attemptedRequest = true;
  } else {
    rv = cachedResponseResult;
    attemptedRequest = false;
  }

  if (response.GetLength() == 0) {
    Result error = rv;
    if (attemptedRequest) {
      Time timeout(time);
      if (timeout.AddSeconds(ServerFailureDelaySeconds) != Success) {
        return Result::FATAL_ERROR_LIBRARY_FAILURE; 
      }
      rv = mOCSPCache.Put(certID, error, time, timeout);
      if (rv != Success) {
        return rv;
      }
    }
    if (mOCSPFetching != FetchOCSPForDVSoftFail) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: returning SECFailure after "
              "OCSP request failure"));
      return error;
    }
    if (cachedResponseResult == Result::ERROR_OCSP_UNKNOWN_CERT) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: returning SECFailure from cached "
              "response after OCSP request failure"));
      return cachedResponseResult;
    }
    if (stapledOCSPResponseResult != Success) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: returning SECFailure from expired "
              "stapled response after OCSP request failure"));
      return stapledOCSPResponseResult;
    }

    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: returning SECSuccess after "
            "OCSP request failure"));
    return Success; 
  }

  
  
  
  bool expired;
  rv = VerifyAndMaybeCacheEncodedOCSPResponse(certID, time,
                                              maxOCSPLifetimeInDays,
                                              response, ResponseIsFromNetwork,
                                              expired);
  if (rv == Success || mOCSPFetching != FetchOCSPForDVSoftFail) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
      ("NSSCertDBTrustDomain: returning after VerifyEncodedOCSPResponse"));
    return rv;
  }

  if (rv == Result::ERROR_OCSP_UNKNOWN_CERT ||
      rv == Result::ERROR_REVOKED_CERTIFICATE) {
    return rv;
  }
  if (stapledOCSPResponseResult != Success) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: returning SECFailure from expired stapled "
            "response after OCSP request verification failure"));
    return stapledOCSPResponseResult;
  }

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("NSSCertDBTrustDomain: end of CheckRevocation"));

  return Success; 
}

Result
NSSCertDBTrustDomain::VerifyAndMaybeCacheEncodedOCSPResponse(
  const CertID& certID, Time time, uint16_t maxLifetimeInDays,
  Input encodedResponse, EncodedResponseSource responseSource,
   bool& expired)
{
  Time thisUpdate(Time::uninitialized);
  Time validThrough(Time::uninitialized);
  Result rv = VerifyEncodedOCSPResponse(*this, certID, time,
                                        maxLifetimeInDays, encodedResponse,
                                        expired, &thisUpdate, &validThrough);
  
  
  if (responseSource == ResponseWasStapled && expired) {
    PR_ASSERT(rv != Success);
    return rv;
  }
  
  
  
  
  if (rv != Success && rv != Result::ERROR_REVOKED_CERTIFICATE &&
      rv != Result::ERROR_OCSP_UNKNOWN_CERT) {
    validThrough = time;
    if (validThrough.AddSeconds(ServerFailureDelaySeconds) != Success) {
      return Result::FATAL_ERROR_LIBRARY_FAILURE; 
    }
  }
  if (responseSource == ResponseIsFromNetwork ||
      rv == Success ||
      rv == Result::ERROR_REVOKED_CERTIFICATE ||
      rv == Result::ERROR_OCSP_UNKNOWN_CERT) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: caching OCSP response"));
    Result putRV = mOCSPCache.Put(certID, rv, thisUpdate, validThrough);
    if (putRV != Success) {
      return putRV;
    }
  }

  return rv;
}

static const uint8_t CNNIC_ROOT_CA_SUBJECT_DATA[] =
  "\x30\x32\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x43\x4E\x31\x0E\x30"
  "\x0C\x06\x03\x55\x04\x0A\x13\x05\x43\x4E\x4E\x49\x43\x31\x13\x30\x11\x06"
  "\x03\x55\x04\x03\x13\x0A\x43\x4E\x4E\x49\x43\x20\x52\x4F\x4F\x54";

static const uint8_t CNNIC_EV_ROOT_CA_SUBJECT_DATA[] =
  "\x30\x81\x8A\x31\x0B\x30\x09\x06\x03\x55\x04\x06\x13\x02\x43\x4E\x31\x32"
  "\x30\x30\x06\x03\x55\x04\x0A\x0C\x29\x43\x68\x69\x6E\x61\x20\x49\x6E\x74"
  "\x65\x72\x6E\x65\x74\x20\x4E\x65\x74\x77\x6F\x72\x6B\x20\x49\x6E\x66\x6F"
  "\x72\x6D\x61\x74\x69\x6F\x6E\x20\x43\x65\x6E\x74\x65\x72\x31\x47\x30\x45"
  "\x06\x03\x55\x04\x03\x0C\x3E\x43\x68\x69\x6E\x61\x20\x49\x6E\x74\x65\x72"
  "\x6E\x65\x74\x20\x4E\x65\x74\x77\x6F\x72\x6B\x20\x49\x6E\x66\x6F\x72\x6D"
  "\x61\x74\x69\x6F\x6E\x20\x43\x65\x6E\x74\x65\x72\x20\x45\x56\x20\x43\x65"
  "\x72\x74\x69\x66\x69\x63\x61\x74\x65\x73\x20\x52\x6F\x6F\x74";

class WhitelistedCNNICHashBinarySearchComparator
{
public:
  explicit WhitelistedCNNICHashBinarySearchComparator(const uint8_t* aTarget,
                                                      size_t aTargetLength)
    : mTarget(aTarget)
  {
    MOZ_ASSERT(aTargetLength == CNNIC_WHITELIST_HASH_LEN,
               "Hashes should be of the same length.");
  }

  int operator()(const WhitelistedCNNICHash val) const {
    return memcmp(mTarget, val.hash, CNNIC_WHITELIST_HASH_LEN);
  }

private:
  const uint8_t* mTarget;
};

Result
NSSCertDBTrustDomain::IsChainValid(const DERArray& certArray, Time time)
{
  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("NSSCertDBTrustDomain: IsChainValid"));

  ScopedCERTCertList certList;
  SECStatus srv = ConstructCERTCertListFromReversedDERArray(certArray,
                                                            certList);
  if (srv != SECSuccess) {
    return MapPRErrorCodeToResult(PR_GetError());
  }

  
  
  CERTCertListNode* rootNode = CERT_LIST_TAIL(certList);
  if (!rootNode) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }
  CERTCertificate* root = rootNode->cert;
  if (!root) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }
  if ((root->derSubject.len == sizeof(CNNIC_ROOT_CA_SUBJECT_DATA) - 1 &&
       memcmp(root->derSubject.data, CNNIC_ROOT_CA_SUBJECT_DATA,
              root->derSubject.len) == 0) ||
      (root->derSubject.len == sizeof(CNNIC_EV_ROOT_CA_SUBJECT_DATA) - 1 &&
       memcmp(root->derSubject.data, CNNIC_EV_ROOT_CA_SUBJECT_DATA,
              root->derSubject.len) == 0)) {
    CERTCertListNode* certNode = CERT_LIST_HEAD(certList);
    if (!certNode) {
      return Result::FATAL_ERROR_LIBRARY_FAILURE;
    }
    CERTCertificate* cert = certNode->cert;
    if (!cert) {
      return Result::FATAL_ERROR_LIBRARY_FAILURE;
    }
    Digest digest;
    nsresult nsrv = digest.DigestBuf(SEC_OID_SHA256, cert->derCert.data,
                                     cert->derCert.len);
    if (NS_FAILED(nsrv)) {
      return Result::FATAL_ERROR_LIBRARY_FAILURE;
    }
    const uint8_t* certHash(
      reinterpret_cast<const uint8_t*>(digest.get().data));
    size_t certHashLen = digest.get().len;
    size_t unused;
    if (!mozilla::BinarySearchIf(WhitelistedCNNICHashes, 0,
                                 ArrayLength(WhitelistedCNNICHashes),
                                 WhitelistedCNNICHashBinarySearchComparator(
                                   certHash, certHashLen),
                                 &unused)) {
      return Result::ERROR_REVOKED_CERTIFICATE;
    }
  }

  Result result = CertListContainsExpectedKeys(certList, mHostname, time,
                                               mPinningMode);
  if (result != Success) {
    return result;
  }

  if (mBuiltChain) {
    *mBuiltChain = certList.forget();
  }

  return Success;
}

Result
NSSCertDBTrustDomain::CheckSignatureDigestAlgorithm(DigestAlgorithm)
{
  return Success;
}

Result
NSSCertDBTrustDomain::CheckRSAPublicKeyModulusSizeInBits(
  EndEntityOrCA , unsigned int modulusSizeInBits)
{
  if (modulusSizeInBits < mMinRSABits) {
    return Result::ERROR_INADEQUATE_KEY_SIZE;
  }
  return Success;
}

Result
NSSCertDBTrustDomain::VerifyRSAPKCS1SignedDigest(
  const SignedDigest& signedDigest,
  Input subjectPublicKeyInfo)
{
  return VerifyRSAPKCS1SignedDigestNSS(signedDigest, subjectPublicKeyInfo,
                                       mPinArg);
}

Result
NSSCertDBTrustDomain::CheckECDSACurveIsAcceptable(
  EndEntityOrCA , NamedCurve curve)
{
  switch (curve) {
    case NamedCurve::secp256r1: 
    case NamedCurve::secp384r1: 
    case NamedCurve::secp521r1:
      return Success;
  }

  return Result::ERROR_UNSUPPORTED_ELLIPTIC_CURVE;
}

Result
NSSCertDBTrustDomain::VerifyECDSASignedDigest(const SignedDigest& signedDigest,
                                              Input subjectPublicKeyInfo)
{
  return VerifyECDSASignedDigestNSS(signedDigest, subjectPublicKeyInfo,
                                    mPinArg);
}

namespace {

static char*
nss_addEscape(const char* string, char quote)
{
  char* newString = 0;
  size_t escapes = 0, size = 0;
  const char* src;
  char* dest;

  for (src = string; *src; src++) {
  if ((*src == quote) || (*src == '\\')) {
    escapes++;
  }
  size++;
  }

  newString = (char*) PORT_ZAlloc(escapes + size + 1u);
  if (!newString) {
    return nullptr;
  }

  for (src = string, dest = newString; *src; src++, dest++) {
    if ((*src == quote) || (*src == '\\')) {
      *dest++ = '\\';
    }
    *dest = *src;
  }

  return newString;
}

} 

SECStatus
InitializeNSS(const char* dir, bool readOnly)
{
  
  
  
  
  
  uint32_t flags = NSS_INIT_NOROOTINIT | NSS_INIT_OPTIMIZESPACE;
  if (readOnly) {
    flags |= NSS_INIT_READONLY;
  }
  return ::NSS_Initialize(dir, "", "", SECMOD_DB, flags);
}

void
DisableMD5()
{
  NSS_SetAlgorithmPolicy(SEC_OID_MD5,
    0, NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE);
  NSS_SetAlgorithmPolicy(SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION,
    0, NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE);
  NSS_SetAlgorithmPolicy(SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC,
    0, NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE);
}

SECStatus
LoadLoadableRoots( const char* dir, const char* modNameUTF8)
{
  PR_ASSERT(modNameUTF8);

  if (!modNameUTF8) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  UniquePtr<char, void(&)(char*)>
    fullLibraryPath(PR_GetLibraryName(dir, "nssckbi"), PR_FreeLibraryName);
  if (!fullLibraryPath) {
    return SECFailure;
  }

  UniquePtr<char, void(&)(void*)>
    escaped_fullLibraryPath(nss_addEscape(fullLibraryPath.get(), '\"'),
                            PORT_Free);
  if (!escaped_fullLibraryPath) {
    return SECFailure;
  }

  
  int modType;
  SECMOD_DeleteModule(modNameUTF8, &modType);

  UniquePtr<char, void(&)(char*)>
    pkcs11ModuleSpec(PR_smprintf("name=\"%s\" library=\"%s\"", modNameUTF8,
                                 escaped_fullLibraryPath.get()),
                     PR_smprintf_free);
  if (!pkcs11ModuleSpec) {
    return SECFailure;
  }

  ScopedSECMODModule rootsModule(SECMOD_LoadUserModule(pkcs11ModuleSpec.get(),
                                                       nullptr, false));
  if (!rootsModule) {
    return SECFailure;
  }

  if (!rootsModule->loaded) {
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  return SECSuccess;
}

void
UnloadLoadableRoots(const char* modNameUTF8)
{
  PR_ASSERT(modNameUTF8);
  ScopedSECMODModule rootsModule(SECMOD_FindModule(modNameUTF8));

  if (rootsModule) {
    SECMOD_UnloadUserModule(rootsModule.get());
  }
}

char*
DefaultServerNicknameForCert(CERTCertificate* cert)
{
  char* nickname = nullptr;
  int count;
  bool conflict;
  char* servername = nullptr;

  servername = CERT_GetCommonName(&cert->subject);
  if (!servername) {
    
    
    servername = CERT_GetOrgUnitName(&cert->subject);
    if (!servername) {
      servername = CERT_GetOrgName(&cert->subject);
      if (!servername) {
        servername = CERT_GetLocalityName(&cert->subject);
        if (!servername) {
          servername = CERT_GetStateName(&cert->subject);
          if (!servername) {
            servername = CERT_GetCountryName(&cert->subject);
            if (!servername) {
              
              
              return nullptr;
            }
          }
        }
      }
    }
  }

  count = 1;
  while (1) {
    if (count == 1) {
      nickname = PR_smprintf("%s", servername);
    }
    else {
      nickname = PR_smprintf("%s #%d", servername, count);
    }
    if (!nickname) {
      break;
    }

    conflict = SEC_CertNicknameConflict(nickname, &cert->derSubject,
                                        cert->dbhandle);
    if (!conflict) {
      break;
    }
    PR_Free(nickname);
    count++;
  }
  PR_FREEIF(servername);
  return nickname;
}

void
SaveIntermediateCerts(const ScopedCERTCertList& certList)
{
  if (!certList) {
    return;
  }

  bool isEndEntity = true;
  for (CERTCertListNode* node = CERT_LIST_HEAD(certList);
        !CERT_LIST_END(node, certList);
        node = CERT_LIST_NEXT(node)) {
    if (isEndEntity) {
      
      isEndEntity = false;
      continue;
    }

    if (node->cert->slot) {
      
      continue;
    }

    if (node->cert->isperm) {
      
      continue;
    }

    
    char* nickname = DefaultServerNicknameForCert(node->cert);
    if (nickname && *nickname) {
      ScopedPK11SlotInfo slot(PK11_GetInternalKeySlot());
      if (slot) {
        PK11_ImportCert(slot.get(), node->cert, CK_INVALID_HANDLE,
                        nickname, false);
      }
    }
    PR_FREEIF(nickname);
  }
}

} } 
