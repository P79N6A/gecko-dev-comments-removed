





#include "NSSCertDBTrustDomain.h"

#include <stdint.h>

#include "ExtendedValidation.h"
#include "insanity/pkix.h"
#include "certdb.h"
#include "nss.h"
#include "ocsp.h"
#include "pk11pub.h"
#include "prerror.h"
#include "prmem.h"
#include "prprf.h"
#include "secerr.h"
#include "secmod.h"

using namespace insanity::pkix;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gCertVerifierLog;
#endif

namespace mozilla { namespace psm {

const char BUILTIN_ROOTS_MODULE_DEFAULT_NAME[] = "Builtin Roots Module";

namespace {

inline void PORT_Free_string(char* str) { PORT_Free(str); }

typedef ScopedPtr<SECMODModule, SECMOD_DestroyModule> ScopedSECMODModule;

} 

NSSCertDBTrustDomain::NSSCertDBTrustDomain(SECTrustType certDBTrustType,
                                           OCSPFetching ocspFetching,
                                           void* pinArg)
  : mCertDBTrustType(certDBTrustType)
  , mOCSPFetching(ocspFetching)
  , mPinArg(pinArg)
{
}

SECStatus
NSSCertDBTrustDomain::FindPotentialIssuers(
  const SECItem* encodedIssuerName, PRTime time,
   insanity::pkix::ScopedCERTCertList& results)
{
  
  
  
  results = CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                       encodedIssuerName, time, true);
  if (!results) {
    
    
    if (PR_GetError() == SEC_ERROR_BAD_DATABASE) {
      PR_SetError(SEC_ERROR_UNKNOWN_ISSUER, 0);
    }
    return SECFailure;
  }

  return SECSuccess;
}

SECStatus
NSSCertDBTrustDomain::GetCertTrust(EndEntityOrCA endEntityOrCA,
                                   SECOidTag policy,
                                   const CERTCertificate* candidateCert,
                                    TrustLevel* trustLevel)
{
  PR_ASSERT(candidateCert);
  PR_ASSERT(trustLevel);

  if (!candidateCert || !trustLevel) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

#ifdef MOZ_NO_EV_CERTS
  if (policy != SEC_OID_X509_ANY_POLICY) {
    PR_SetError(SEC_ERROR_POLICY_VALIDATION_FAILED, 0);
    return SECFailure;
  }
#endif

  
  
  
  
  
  CERTCertTrust trust;
  if (CERT_GetCertTrust(candidateCert, &trust) == SECSuccess) {
    PRUint32 flags = SEC_GET_TRUST_FLAGS(&trust, mCertDBTrustType);

    
    
    
    
    
    PRUint32 relevantTrustBit = endEntityOrCA == MustBeCA ? CERTDB_TRUSTED_CA
                                                          : CERTDB_TRUSTED;
    if (((flags & (relevantTrustBit|CERTDB_TERMINAL_RECORD)))
            == CERTDB_TERMINAL_RECORD) {
      *trustLevel = ActivelyDistrusted;
      return SECSuccess;
    }

    
    
    
    if (flags & CERTDB_TRUSTED_CA) {
      if (policy == SEC_OID_X509_ANY_POLICY) {
        *trustLevel = TrustAnchor;
        return SECSuccess;
      }
#ifndef MOZ_NO_EV_CERTS
      if (CertIsAuthoritativeForEVPolicy(candidateCert, policy)) {
        *trustLevel = TrustAnchor;
        return SECSuccess;
      }
#endif
    }
  }

  *trustLevel = InheritsTrust;
  return SECSuccess;
}

SECStatus
NSSCertDBTrustDomain::VerifySignedData(const CERTSignedData* signedData,
                                       const CERTCertificate* cert)
{
  return ::insanity::pkix::VerifySignedData(signedData, cert, mPinArg);
}

SECStatus
NSSCertDBTrustDomain::CheckRevocation(
  insanity::pkix::EndEntityOrCA endEntityOrCA,
  const CERTCertificate* cert,
   CERTCertificate* issuerCert,
  PRTime time,
   const SECItem* stapledOCSPResponse)
{
  
  

  
  

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("NSSCertDBTrustDomain: Top of CheckRevocation\n"));

  PORT_Assert(cert);
  PORT_Assert(issuerCert);
  if (!cert || !issuerCert) {
    PORT_SetError(SEC_ERROR_INVALID_ARGS);
    return SECFailure;
  }

  
  
  
  
  if (stapledOCSPResponse) {
    PR_ASSERT(endEntityOrCA == MustBeEndEntity);
    SECStatus rv = VerifyEncodedOCSPResponse(*this, cert, issuerCert, time,
                                             stapledOCSPResponse);
    if (rv == SECSuccess) {
      return rv;
    }
    if (PR_GetError() != SEC_ERROR_OCSP_OLD_RESPONSE) {
      return rv;
    }
  }

  

  
  
  
  

  if ((mOCSPFetching == NeverFetchOCSP) ||
      (endEntityOrCA == MustBeCA && (mOCSPFetching == FetchOCSPForDVHardFail ||
                                     mOCSPFetching == FetchOCSPForDVSoftFail))) {
    return SECSuccess;
  }

  if (mOCSPFetching == LocalOnlyOCSPForEV) {
    PR_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT, 0);
    return SECFailure;
  }

  ScopedPtr<char, PORT_Free_string>
    url(CERT_GetOCSPAuthorityInfoAccessLocation(cert));

  if (!url) {
    if (stapledOCSPResponse) {
      PR_SetError(SEC_ERROR_OCSP_OLD_RESPONSE, 0);
      return SECFailure;
    }
    if (mOCSPFetching == FetchOCSPForEV) {
      PR_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT, 0);
      return SECFailure;
    }

    
    
    
    
    return SECSuccess;
  }

  ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return SECFailure;
  }

  const SECItem* request(CreateEncodedOCSPRequest(arena.get(), cert,
                                                  issuerCert));
  if (!request) {
    return SECFailure;
  }

  const SECItem* response(CERT_PostOCSPRequest(arena.get(), url.get(),
                                               request));
  if (!response) {
    if (mOCSPFetching != FetchOCSPForDVSoftFail) {
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("NSSCertDBTrustDomain: returning SECFailure after "
              "CERT_PostOCSPRequest failure"));
      return SECFailure;
    }

    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("NSSCertDBTrustDomain: returning SECSuccess after "
            "CERT_PostOCSPRequest failure"));
    return SECSuccess; 
  }

  SECStatus rv = VerifyEncodedOCSPResponse(*this, cert, issuerCert, time,
                                           response);
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

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("NSSCertDBTrustDomain: end of CheckRevocation"));

  return SECSuccess;
}

namespace {

static char*
nss_addEscape(const char* string, char quote)
{
  char* newString = 0;
  int escapes = 0, size = 0;
  const char* src;
  char* dest;

  for (src = string; *src; src++) {
  if ((*src == quote) || (*src == '\\')) {
    escapes++;
  }
  size++;
  }

  newString = (char*) PORT_ZAlloc(escapes + size + 1);
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

void
SetClassicOCSPBehavior(CertVerifier::ocsp_download_config enabled,
                       CertVerifier::ocsp_strict_config strict,
                       CertVerifier::ocsp_get_config get)
{
  CERT_DisableOCSPDefaultResponder(CERT_GetDefaultCertDB());
  if (enabled == CertVerifier::ocsp_off) {
    CERT_DisableOCSPChecking(CERT_GetDefaultCertDB());
  } else {
    CERT_EnableOCSPChecking(CERT_GetDefaultCertDB());
  }

  SEC_OcspFailureMode failureMode = strict == CertVerifier::ocsp_strict
                                  ? ocspMode_FailureIsVerificationFailure
                                  : ocspMode_FailureIsNotAVerificationFailure;
  (void) CERT_SetOCSPFailureMode(failureMode);

  CERT_ForcePostMethodForOCSP(get != CertVerifier::ocsp_get_enabled);

  int OCSPTimeoutSeconds = 3;
  if (strict == CertVerifier::ocsp_strict) {
    OCSPTimeoutSeconds = 10;
  }
  CERT_SetOCSPTimeout(OCSPTimeoutSeconds);
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
