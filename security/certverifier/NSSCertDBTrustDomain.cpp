





#include "NSSCertDBTrustDomain.h"

#include <stdint.h>

#include "ExtendedValidation.h"
#include "nsNSSCertificate.h"
#include "NSSErrorsService.h"
#include "OCSPRequestor.h"
#include "certdb.h"
#include "mozilla/Telemetry.h"
#include "nss.h"
#include "pk11pub.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "pkix/ScopedPtr.h"
#include "prerror.h"
#include "prmem.h"
#include "prprf.h"
#include "ScopedNSSTypes.h"
#include "secerr.h"
#include "secmod.h"

using namespace mozilla;
using namespace mozilla::pkix;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gCertVerifierLog;
#endif

namespace mozilla { namespace psm {

const char BUILTIN_ROOTS_MODULE_DEFAULT_NAME[] = "Builtin Roots Module";

void PORT_Free_string(char* str) { PORT_Free(str); }

namespace {

typedef ScopedPtr<SECMODModule, SECMOD_DestroyModule> ScopedSECMODModule;

} 

NSSCertDBTrustDomain::NSSCertDBTrustDomain(SECTrustType certDBTrustType,
                                           OCSPFetching ocspFetching,
                                           OCSPCache& ocspCache,
              void* pinArg,
                                           CertVerifier::ocsp_get_config ocspGETConfig,
                               CERTChainVerifyCallback* checkChainCallback,
                               ScopedCERTCertList* builtChain)
  : mCertDBTrustType(certDBTrustType)
  , mOCSPFetching(ocspFetching)
  , mOCSPCache(ocspCache)
  , mPinArg(pinArg)
  , mOCSPGetConfig(ocspGETConfig)
  , mCheckChainCallback(checkChainCallback)
  , mBuiltChain(builtChain)
{
}


static const uint8_t ANSSI_SUBJECT_DATA[] =
                       "\x30\x81\x85\x31\x0B\x30\x09\x06\x03\x55\x04"
                       "\x06\x13\x02\x46\x52\x31\x0F\x30\x0D\x06\x03"
                       "\x55\x04\x08\x13\x06\x46\x72\x61\x6E\x63\x65"
                       "\x31\x0E\x30\x0C\x06\x03\x55\x04\x07\x13\x05"
                       "\x50\x61\x72\x69\x73\x31\x10\x30\x0E\x06\x03"
                       "\x55\x04\x0A\x13\x07\x50\x4D\x2F\x53\x47\x44"
                       "\x4E\x31\x0E\x30\x0C\x06\x03\x55\x04\x0B\x13"
                       "\x05\x44\x43\x53\x53\x49\x31\x0E\x30\x0C\x06"
                       "\x03\x55\x04\x03\x13\x05\x49\x47\x43\x2F\x41"
                       "\x31\x23\x30\x21\x06\x09\x2A\x86\x48\x86\xF7"
                       "\x0D\x01\x09\x01\x16\x14\x69\x67\x63\x61\x40"
                       "\x73\x67\x64\x6E\x2E\x70\x6D\x2E\x67\x6F\x75"
                       "\x76\x2E\x66\x72";

static const uint8_t PERMIT_FRANCE_GOV_NAME_CONSTRAINTS_DATA[] =
                       "\x30\x5D" 
                       "\xA0\x5B" 
                       "\x30\x05\x82\x03" ".fr"
                       "\x30\x05\x82\x03" ".gp"
                       "\x30\x05\x82\x03" ".gf"
                       "\x30\x05\x82\x03" ".mq"
                       "\x30\x05\x82\x03" ".re"
                       "\x30\x05\x82\x03" ".yt"
                       "\x30\x05\x82\x03" ".pm"
                       "\x30\x05\x82\x03" ".bl"
                       "\x30\x05\x82\x03" ".mf"
                       "\x30\x05\x82\x03" ".wf"
                       "\x30\x05\x82\x03" ".pf"
                       "\x30\x05\x82\x03" ".nc"
                       "\x30\x05\x82\x03" ".tf";

Result
NSSCertDBTrustDomain::FindIssuer(Input encodedIssuerName,
                                 IssuerChecker& checker, PRTime time)
{
  
  
  SECItem encodedIssuerNameSECItem = UnsafeMapInputToSECItem(encodedIssuerName);
  ScopedCERTCertList
    candidates(CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                          &encodedIssuerNameSECItem, time,
                                          true));
  if (candidates) {
    for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
         !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
      Input certDER;
      Result rv = certDER.Init(n->cert->derCert.data, n->cert->derCert.len);
      if (rv != Success) {
        continue; 
      }

      bool keepGoing;
      Input anssiSubject;
      rv = anssiSubject.Init(ANSSI_SUBJECT_DATA,
                             sizeof(ANSSI_SUBJECT_DATA) - 1);
      if (rv != Success) {
        return Result::FATAL_ERROR_LIBRARY_FAILURE;
      }
      
      if (InputsAreEqual(encodedIssuerName, anssiSubject)) {
        Input anssiNameConstraints;
        if (anssiNameConstraints.Init(
                PERMIT_FRANCE_GOV_NAME_CONSTRAINTS_DATA,
                sizeof(PERMIT_FRANCE_GOV_NAME_CONSTRAINTS_DATA) - 1)
              != Success) {
          return Result::FATAL_ERROR_LIBRARY_FAILURE;
        }
        rv = checker.Check(certDER, &anssiNameConstraints, keepGoing);
      } else {
        rv = checker.Check(certDER, nullptr, keepGoing);
      }
      if (rv != Success) {
        return rv;
      }
      if (!keepGoing) {
        break;
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

  
  
  
  
  
  CERTCertTrust trust;
  if (CERT_GetCertTrust(candidateCert.get(), &trust) == SECSuccess) {
    PRUint32 flags = SEC_GET_TRUST_FLAGS(&trust, mCertDBTrustType);

    
    
    
    
    
    PRUint32 relevantTrustBit =
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
NSSCertDBTrustDomain::VerifySignedData(const SignedDataWithSignature& signedData,
                                       Input subjectPublicKeyInfo)
{
  return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                           mPinArg);
}

Result
NSSCertDBTrustDomain::DigestBuf(Input item,
                                 uint8_t* digestBuf, size_t digestBufLen)
{
  return ::mozilla::pkix::DigestBuf(item, digestBuf, digestBufLen);
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
                                      const CertID& certID, PRTime time,
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
      
      Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 1);
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: stapled OCSP response: good"));
      return Success;
    }
    if (stapledOCSPResponseResult == Result::ERROR_OCSP_OLD_RESPONSE ||
        expired) {
      
      Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 3);
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: expired stapled OCSP response"));
    } else {
      
      Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 4);
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: stapled OCSP response: failure"));
      return stapledOCSPResponseResult;
    }
  } else {
    
    Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 2);
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: no stapled OCSP response"));
  }

  Result cachedResponseResult = Success;
  PRTime cachedResponseValidThrough = 0;
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

  
  
  
  

  if ((mOCSPFetching == NeverFetchOCSP) ||
      (endEntityOrCA == EndEntityOrCA::MustBeCA &&
       (mOCSPFetching == FetchOCSPForDVHardFail ||
        mOCSPFetching == FetchOCSPForDVSoftFail))) {
    
    
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
                    mOCSPGetConfig == CertVerifier::ocsp_get_enabled);
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
      PRTime timeout = time + ServerFailureDelay;
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
  const CertID& certID, PRTime time, uint16_t maxLifetimeInDays,
  Input encodedResponse, EncodedResponseSource responseSource,
   bool& expired)
{
  PRTime thisUpdate = 0;
  PRTime validThrough = 0;
  Result rv = VerifyEncodedOCSPResponse(*this, certID, time,
                                        maxLifetimeInDays, encodedResponse,
                                        expired, &thisUpdate, &validThrough);
  
  
  if (responseSource == ResponseWasStapled && expired) {
    PR_ASSERT(rv != Success);
    return rv;
  }
  
  
  
  
  if (rv != Success && rv != Result::ERROR_REVOKED_CERTIFICATE &&
      rv != Result::ERROR_OCSP_UNKNOWN_CERT) {
    validThrough = time + ServerFailureDelay;
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

Result
NSSCertDBTrustDomain::IsChainValid(const DERArray& certArray)
{
  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
      ("NSSCertDBTrustDomain: Top of IsChainValid mCheckChainCallback=%p",
       mCheckChainCallback));

  if (!mBuiltChain && !mCheckChainCallback) {
    
    return Success;
  }

  ScopedCERTCertList certList;
  SECStatus srv = ConstructCERTCertListFromReversedDERArray(certArray,
                                                            certList);
  if (srv != SECSuccess) {
    return MapPRErrorCodeToResult(PR_GetError());
  }

  if (mCheckChainCallback) {
    if (!mCheckChainCallback->isChainValid) {
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    PRBool chainOK;
    srv = (mCheckChainCallback->isChainValid)(
            mCheckChainCallback->isChainValidArg, certList.get(), &chainOK);
    if (srv != SECSuccess) {
      return MapPRErrorCodeToResult(PR_GetError());
    }
    if (!chainOK) {
      return Result::ERROR_KEY_PINNING_FAILURE;
    }
  }

  if (mBuiltChain) {
    *mBuiltChain = certList.forget();
  }

  return Success;
}

Result
NSSCertDBTrustDomain::CheckPublicKey(Input subjectPublicKeyInfo)
{
  return ::mozilla::pkix::CheckPublicKey(subjectPublicKeyInfo);
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

  ScopedPtr<char, PR_FreeLibraryName> fullLibraryPath(
    PR_GetLibraryName(dir, "nssckbi"));
  if (!fullLibraryPath) {
    return SECFailure;
  }

  ScopedPtr<char, PORT_Free_string> escaped_fullLibraryPath(
    nss_addEscape(fullLibraryPath.get(), '\"'));
  if (!escaped_fullLibraryPath) {
    return SECFailure;
  }

  
  int modType;
  SECMOD_DeleteModule(modNameUTF8, &modType);

  ScopedPtr<char, PR_smprintf_free> pkcs11ModuleSpec(
    PR_smprintf("name=\"%s\" library=\"%s\"", modNameUTF8,
                escaped_fullLibraryPath.get()));
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
      ScopedPtr<PK11SlotInfo, PK11_FreeSlot> slot(PK11_GetInternalKeySlot());
      if (slot) {
        PK11_ImportCert(slot.get(), node->cert, CK_INVALID_HANDLE,
                        nickname, false);
      }
    }
    PR_FREEIF(nickname);
  }
}

} } 
