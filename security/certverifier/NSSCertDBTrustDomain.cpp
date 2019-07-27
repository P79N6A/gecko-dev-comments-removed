





#include "NSSCertDBTrustDomain.h"

#include <stdint.h>

#include "ExtendedValidation.h"
#include "NSSErrorsService.h"
#include "OCSPRequestor.h"
#include "certdb.h"
#include "mozilla/Telemetry.h"
#include "nss.h"
#include "pk11pub.h"
#include "pkix/pkix.h"
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
                                           CERTChainVerifyCallback* checkChainCallback)
  : mCertDBTrustType(certDBTrustType)
  , mOCSPFetching(ocspFetching)
  , mOCSPCache(ocspCache)
  , mPinArg(pinArg)
  , mOCSPGetConfig(ocspGETConfig)
  , mCheckChainCallback(checkChainCallback)
{
}

SECStatus
NSSCertDBTrustDomain::FindIssuer(const SECItem& encodedIssuerName,
                                 IssuerChecker& checker, PRTime time)
{
  
  
  mozilla::pkix::ScopedCERTCertList
    candidates(CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                          &encodedIssuerName, time, true));
  if (candidates) {
    for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
         !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
      bool keepGoing;
      SECStatus srv = checker.Check(n->cert->derCert, keepGoing);
      if (srv != SECSuccess) {
        return SECFailure;
      }
      if (!keepGoing) {
        break;
      }
    }
  }

  return SECSuccess;
}

SECStatus
NSSCertDBTrustDomain::GetCertTrust(EndEntityOrCA endEntityOrCA,
                                   const CertPolicyId& policy,
                                   const SECItem& candidateCertDER,
                                    TrustLevel* trustLevel)
{
  PR_ASSERT(trustLevel);

  if (!trustLevel) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

#ifdef MOZ_NO_EV_CERTS
  if (!policy.IsAnyPolicy()) {
    PR_SetError(SEC_ERROR_POLICY_VALIDATION_FAILED, 0);
    return SECFailure;
  }
#endif

  
  
  
  
  
  
  ScopedCERTCertificate candidateCert(
    CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                            const_cast<SECItem*>(&candidateCertDER), nullptr,
                            false, true));
  if (!candidateCert) {
    return SECFailure;
  }

  
  
  
  
  
  CERTCertTrust trust;
  if (CERT_GetCertTrust(candidateCert.get(), &trust) == SECSuccess) {
    PRUint32 flags = SEC_GET_TRUST_FLAGS(&trust, mCertDBTrustType);

    
    
    
    
    
    PRUint32 relevantTrustBit =
      endEntityOrCA == EndEntityOrCA::MustBeCA ? CERTDB_TRUSTED_CA
                                               : CERTDB_TRUSTED;
    if (((flags & (relevantTrustBit|CERTDB_TERMINAL_RECORD)))
            == CERTDB_TERMINAL_RECORD) {
      *trustLevel = TrustLevel::ActivelyDistrusted;
      return SECSuccess;
    }

    
    
    
    if (flags & CERTDB_TRUSTED_CA) {
      if (policy.IsAnyPolicy()) {
        *trustLevel = TrustLevel::TrustAnchor;
        return SECSuccess;
      }
#ifndef MOZ_NO_EV_CERTS
      if (CertIsAuthoritativeForEVPolicy(candidateCert.get(), policy)) {
        *trustLevel = TrustLevel::TrustAnchor;
        return SECSuccess;
      }
#endif
    }
  }

  *trustLevel = TrustLevel::InheritsTrust;
  return SECSuccess;
}

SECStatus
NSSCertDBTrustDomain::VerifySignedData(const CERTSignedData& signedData,
                                       const SECItem& subjectPublicKeyInfo)
{
  return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                           mPinArg);
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






static SECStatus
GetOCSPAuthorityInfoAccessLocation(PLArenaPool* arena,
                                   const SECItem& aiaExtension,
                                    char const*& url)
{
  url = nullptr;

  
  CERTAuthInfoAccess** aia = CERT_DecodeAuthInfoAccessExtension(
                                arena,
                                const_cast<SECItem*>(&aiaExtension));
  if (!aia) {
    PR_SetError(SEC_ERROR_CERT_BAD_ACCESS_LOCATION, 0);
    return SECFailure;
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
            
            PR_SetError(SEC_ERROR_CERT_BAD_ACCESS_LOCATION, 0);
            return SECFailure;
          }
          
          char* nullTerminatedURL(static_cast<char*>(
                                    PORT_ArenaAlloc(arena, location.len + 1)));
          if (!nullTerminatedURL) {
            return SECFailure;
          }
          memcpy(nullTerminatedURL, location.data, location.len);
          nullTerminatedURL[location.len] = 0;
          url = nullTerminatedURL;
          return SECSuccess;
        }
        current = CERT_GetNextGeneralName(current);
      } while (current != aia[i]->location);
    }
  }

  return SECSuccess;
}

SECStatus
NSSCertDBTrustDomain::CheckRevocation(EndEntityOrCA endEntityOrCA,
                                      const CertID& certID, PRTime time,
                          const SECItem* stapledOCSPResponse,
                          const SECItem* aiaExtension)
{
  
  

  
  

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("NSSCertDBTrustDomain: Top of CheckRevocation\n"));

  
  
  
  
  uint16_t maxOCSPLifetimeInDays = 10;
  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    maxOCSPLifetimeInDays = 365;
  }

  
  
  
  
  
  
  PRErrorCode stapledOCSPResponseErrorCode = 0;
  if (stapledOCSPResponse) {
    PR_ASSERT(endEntityOrCA == EndEntityOrCA::MustBeEndEntity);
    bool expired;
    SECStatus rv = VerifyAndMaybeCacheEncodedOCSPResponse(certID, time,
                                                          maxOCSPLifetimeInDays,
                                                          *stapledOCSPResponse,
                                                          ResponseWasStapled,
                                                          expired);
    if (rv == SECSuccess) {
      
      Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 1);
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: stapled OCSP response: good"));
      return rv;
    }
    stapledOCSPResponseErrorCode = PR_GetError();
    if (stapledOCSPResponseErrorCode == SEC_ERROR_OCSP_OLD_RESPONSE ||
        expired) {
      
      Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 3);
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: expired stapled OCSP response"));
    } else {
      
      Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 4);
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: stapled OCSP response: failure"));
      return rv;
    }
  } else {
    
    Telemetry::Accumulate(Telemetry::SSL_OCSP_STAPLING, 2);
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: no stapled OCSP response"));
  }

  PRErrorCode cachedResponseErrorCode = 0;
  PRTime cachedResponseValidThrough = 0;
  bool cachedResponsePresent = mOCSPCache.Get(certID,
                                              cachedResponseErrorCode,
                                              cachedResponseValidThrough);
  if (cachedResponsePresent) {
    if (cachedResponseErrorCode == 0 && cachedResponseValidThrough >= time) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: cached OCSP response: good"));
      return SECSuccess;
    }
    
    if (cachedResponseErrorCode == SEC_ERROR_REVOKED_CERTIFICATE) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: cached OCSP response: revoked"));
      PR_SetError(SEC_ERROR_REVOKED_CERTIFICATE, 0);
      return SECFailure;
    }
    
    
    
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: cached OCSP response: error %ld valid "
           "until %lld", cachedResponseErrorCode, cachedResponseValidThrough));
    
    
    
    if (cachedResponseErrorCode == 0 && cachedResponseValidThrough < time) {
      cachedResponseErrorCode = SEC_ERROR_OCSP_OLD_RESPONSE;
    }
    
    
    if (cachedResponseErrorCode != 0 &&
        cachedResponseErrorCode != SEC_ERROR_OCSP_UNKNOWN_CERT &&
        cachedResponseErrorCode != SEC_ERROR_OCSP_OLD_RESPONSE &&
        cachedResponseValidThrough < time) {
      cachedResponseErrorCode = 0;
      cachedResponsePresent = false;
    }
  } else {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: no cached OCSP response"));
  }
  
  
  PR_ASSERT((!cachedResponsePresent && cachedResponseErrorCode == 0) ||
            (cachedResponsePresent && cachedResponseErrorCode != 0));

  
  
  
  

  if ((mOCSPFetching == NeverFetchOCSP) ||
      (endEntityOrCA == EndEntityOrCA::MustBeCA &&
       (mOCSPFetching == FetchOCSPForDVHardFail ||
        mOCSPFetching == FetchOCSPForDVSoftFail))) {
    
    
    if (cachedResponseErrorCode == SEC_ERROR_OCSP_UNKNOWN_CERT) {
      PR_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT, 0);
      return SECFailure;
    }
    
    
    if (mOCSPFetching == FetchOCSPForDVHardFail &&
        cachedResponseErrorCode == SEC_ERROR_OCSP_OLD_RESPONSE) {
      PR_SetError(SEC_ERROR_OCSP_OLD_RESPONSE, 0);
      return SECFailure;
    }

    return SECSuccess;
  }

  if (mOCSPFetching == LocalOnlyOCSPForEV) {
    PR_SetError(cachedResponseErrorCode != 0 ? cachedResponseErrorCode
                                             : SEC_ERROR_OCSP_UNKNOWN_CERT, 0);
    return SECFailure;
  }

  ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return SECFailure;
  }

  const char* url = nullptr; 

  if (aiaExtension) {
    if (GetOCSPAuthorityInfoAccessLocation(arena.get(), *aiaExtension, url)
          != SECSuccess) {
      return SECFailure;
    }
  }

  if (!url) {
    if (mOCSPFetching == FetchOCSPForEV ||
        cachedResponseErrorCode == SEC_ERROR_OCSP_UNKNOWN_CERT) {
      PR_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT, 0);
      return SECFailure;
    }
    if (cachedResponseErrorCode == SEC_ERROR_OCSP_OLD_RESPONSE) {
      PR_SetError(SEC_ERROR_OCSP_OLD_RESPONSE, 0);
      return SECFailure;
    }
    if (stapledOCSPResponseErrorCode != 0) {
      PR_SetError(stapledOCSPResponseErrorCode, 0);
      return SECFailure;
    }

    
    
    
    
    return SECSuccess;
  }

  
  
  const SECItem* response = nullptr;
  if (cachedResponseErrorCode == 0 ||
      cachedResponseErrorCode == SEC_ERROR_OCSP_UNKNOWN_CERT ||
      cachedResponseErrorCode == SEC_ERROR_OCSP_OLD_RESPONSE) {
    const SECItem* request(CreateEncodedOCSPRequest(arena.get(), certID));
    if (!request) {
      return SECFailure;
    }

    response = DoOCSPRequest(arena.get(), url, request,
                             OCSPFetchingTypeToTimeoutTime(mOCSPFetching),
                             mOCSPGetConfig == CertVerifier::ocsp_get_enabled);
  }

  if (!response) {
    PRErrorCode error = PR_GetError();
    if (error == 0) {
      error = cachedResponseErrorCode;
    }
    PRTime timeout = time + ServerFailureDelay;
    if (mOCSPCache.Put(certID, error, time, timeout) != SECSuccess) {
      return SECFailure;
    }
    PR_SetError(error, 0);
    if (mOCSPFetching != FetchOCSPForDVSoftFail) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: returning SECFailure after "
              "OCSP request failure"));
      return SECFailure;
    }
    if (cachedResponseErrorCode == SEC_ERROR_OCSP_UNKNOWN_CERT) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: returning SECFailure from cached "
              "response after OCSP request failure"));
      PR_SetError(cachedResponseErrorCode, 0);
      return SECFailure;
    }
    if (stapledOCSPResponseErrorCode != 0) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: returning SECFailure from expired "
              "stapled response after OCSP request failure"));
      PR_SetError(stapledOCSPResponseErrorCode, 0);
      return SECFailure;
    }

    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: returning SECSuccess after "
            "OCSP request failure"));
    return SECSuccess; 
  }

  
  
  
  bool expired;
  SECStatus rv = VerifyAndMaybeCacheEncodedOCSPResponse(certID, time,
                                                        maxOCSPLifetimeInDays,
                                                        *response,
                                                        ResponseIsFromNetwork,
                                                        expired);
  if (rv == SECSuccess || mOCSPFetching != FetchOCSPForDVSoftFail) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
      ("NSSCertDBTrustDomain: returning after VerifyEncodedOCSPResponse"));
    return rv;
  }

  PRErrorCode error = PR_GetError();
  if (error == SEC_ERROR_OCSP_UNKNOWN_CERT ||
      error == SEC_ERROR_REVOKED_CERTIFICATE) {
    return rv;
  }

  if (stapledOCSPResponseErrorCode != 0) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: returning SECFailure from expired stapled "
            "response after OCSP request verification failure"));
    PR_SetError(stapledOCSPResponseErrorCode, 0);
    return SECFailure;
  }

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("NSSCertDBTrustDomain: end of CheckRevocation"));

  return SECSuccess; 
}

SECStatus
NSSCertDBTrustDomain::VerifyAndMaybeCacheEncodedOCSPResponse(
  const CertID& certID, PRTime time, uint16_t maxLifetimeInDays,
  const SECItem& encodedResponse, EncodedResponseSource responseSource,
   bool& expired)
{
  PRTime thisUpdate = 0;
  PRTime validThrough = 0;
  SECStatus rv = VerifyEncodedOCSPResponse(*this, certID, time,
                                           maxLifetimeInDays, encodedResponse,
                                           expired, &thisUpdate, &validThrough);
  PRErrorCode error = (rv == SECSuccess ? 0 : PR_GetError());
  
  
  if (responseSource == ResponseWasStapled && expired) {
    PR_ASSERT(rv != SECSuccess);
    return rv;
  }
  
  
  
  
  if (rv != SECSuccess && error != SEC_ERROR_REVOKED_CERTIFICATE &&
      error != SEC_ERROR_OCSP_UNKNOWN_CERT) {
    validThrough = time + ServerFailureDelay;
  }
  if (responseSource == ResponseIsFromNetwork ||
      rv == SECSuccess ||
      error == SEC_ERROR_REVOKED_CERTIFICATE ||
      error == SEC_ERROR_OCSP_UNKNOWN_CERT) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: caching OCSP response"));
    if (mOCSPCache.Put(certID, error, thisUpdate, validThrough) != SECSuccess) {
      return SECFailure;
    }
  }

  
  
  if (rv != SECSuccess) {
    PR_SetError(error, 0);
  }
  return rv;
}

SECStatus
NSSCertDBTrustDomain::IsChainValid(const CERTCertList* certChain) {
  SECStatus rv = SECFailure;

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
      ("NSSCertDBTrustDomain: Top of IsChainValid mCheckCallback=%p",
       mCheckChainCallback));

  if (!mCheckChainCallback) {
    return SECSuccess;
  }
  if (!mCheckChainCallback->isChainValid) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }
  PRBool chainOK;
  rv = (mCheckChainCallback->isChainValid)(mCheckChainCallback->isChainValidArg,
                                           certChain, &chainOK);
  if (rv != SECSuccess) {
    return rv;
  }
  
  
  
  if (chainOK) {
    return SECSuccess;
  }
  PR_SetError(PSM_ERROR_KEY_PINNING_FAILURE, 0);
  return SECFailure;
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
SaveIntermediateCerts(const mozilla::pkix::ScopedCERTCertList& certList)
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
