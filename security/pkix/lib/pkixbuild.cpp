























#include "pkix/pkix.h"

#include <limits>

#include "pkixcheck.h"
#include "pkixder.h"

namespace mozilla { namespace pkix {







Result
BackCert::Init()
{
  const CERTCertExtension* const* exts = nssCert->extensions;
  if (!exts) {
    return Success;
  }
  
  
  
  
  
  
  if (! (nssCert->version.len == 1 &&
      nssCert->version.data[0] == mozilla::pkix::der::Version::v3)) {
    return Fail(RecoverableError, SEC_ERROR_EXTENSION_VALUE_INVALID);
  }

  const SECItem* dummyEncodedSubjectKeyIdentifier = nullptr;
  const SECItem* dummyEncodedAuthorityKeyIdentifier = nullptr;
  const SECItem* dummyEncodedAuthorityInfoAccess = nullptr;
  const SECItem* dummyEncodedSubjectAltName = nullptr;

  for (const CERTCertExtension* ext = *exts; ext; ext = *++exts) {
    const SECItem** out = nullptr;

    if (ext->id.len == 3 &&
        ext->id.data[0] == 0x55 && ext->id.data[1] == 0x1d) {
      
      switch (ext->id.data[2]) {
        case 14: out = &dummyEncodedSubjectKeyIdentifier; break; 
        case 15: out = &encodedKeyUsage; break;
        case 17: out = &dummyEncodedSubjectAltName; break; 
        case 19: out = &encodedBasicConstraints; break;
        case 30: out = &encodedNameConstraints; break;
        case 32: out = &encodedCertificatePolicies; break;
        case 35: out = &dummyEncodedAuthorityKeyIdentifier; break; 
        case 37: out = &encodedExtendedKeyUsage; break;
        case 54: out = &encodedInhibitAnyPolicy; break; 
      }
    } else if (ext->id.len == 9 &&
               ext->id.data[0] == 0x2b && ext->id.data[1] == 0x06 &&
               ext->id.data[2] == 0x06 && ext->id.data[3] == 0x01 &&
               ext->id.data[4] == 0x05 && ext->id.data[5] == 0x05 &&
               ext->id.data[6] == 0x07 && ext->id.data[7] == 0x01) {
      
      switch (ext->id.data[8]) {
        
        
        
        case 1: out = &dummyEncodedAuthorityInfoAccess; break;
      }
    } else if (ext->critical.data && ext->critical.len > 0) {
      
      
      return Fail(RecoverableError, SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION);
    }

    if (out) {
      
      
      if (*out) {
        
        return Fail(RecoverableError, SEC_ERROR_EXTENSION_VALUE_INVALID);
      }
      *out = &ext->value;
    }
  }

  return Success;
}

static Result BuildForward(TrustDomain& trustDomain,
                           BackCert& subject,
                           PRTime time,
                           EndEntityOrCA endEntityOrCA,
                           KeyUsages requiredKeyUsagesIfPresent,
                           KeyPurposeId requiredEKUIfPresent,
                           const CertPolicyId& requiredPolicy,
                            const SECItem* stapledOCSPResponse,
                           unsigned int subCACount,
                            ScopedCERTCertList& results);


static Result
BuildForwardInner(TrustDomain& trustDomain,
                  BackCert& subject,
                  PRTime time,
                  EndEntityOrCA endEntityOrCA,
                  KeyPurposeId requiredEKUIfPresent,
                  const CertPolicyId& requiredPolicy,
                  CERTCertificate* potentialIssuerCertToDup,
                  unsigned int subCACount,
                  ScopedCERTCertList& results)
{
  PORT_Assert(potentialIssuerCertToDup);

  BackCert potentialIssuer(potentialIssuerCertToDup, &subject,
                           BackCert::IncludeCN::No);
  Result rv = potentialIssuer.Init();
  if (rv != Success) {
    return rv;
  }

  
  

  
  
  
  bool loopDetected = false;
  for (BackCert* prev = potentialIssuer.childCert;
       !loopDetected && prev != nullptr; prev = prev->childCert) {
    if (SECITEM_ItemsAreEqual(&potentialIssuer.GetNSSCert()->derPublicKey,
                              &prev->GetNSSCert()->derPublicKey) &&
        SECITEM_ItemsAreEqual(&potentialIssuer.GetNSSCert()->derSubject,
                              &prev->GetNSSCert()->derSubject)) {
      return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER); 
    }
  }

  rv = CheckNameConstraints(potentialIssuer);
  if (rv != Success) {
    return rv;
  }

  unsigned int newSubCACount = subCACount;
  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    newSubCACount = subCACount + 1;
  } else {
    PR_ASSERT(newSubCACount == 0);
  }
  rv = BuildForward(trustDomain, potentialIssuer, time, EndEntityOrCA::MustBeCA,
                    KU_KEY_CERT_SIGN, requiredEKUIfPresent, requiredPolicy,
                    nullptr, newSubCACount, results);
  if (rv != Success) {
    return rv;
  }

  if (trustDomain.VerifySignedData(&subject.GetNSSCert()->signatureWrap,
                                   potentialIssuer.GetNSSCert()) != SECSuccess) {
    return MapSECStatus(SECFailure);
  }

  return Success;
}







static Result
BuildForward(TrustDomain& trustDomain,
             BackCert& subject,
             PRTime time,
             EndEntityOrCA endEntityOrCA,
             KeyUsages requiredKeyUsagesIfPresent,
             KeyPurposeId requiredEKUIfPresent,
             const CertPolicyId& requiredPolicy,
              const SECItem* stapledOCSPResponse,
             unsigned int subCACount,
              ScopedCERTCertList& results)
{
  
  
  static const size_t MAX_DEPTH = 8;
  if (subCACount >= MAX_DEPTH - 1) {
    return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER);
  }

  Result rv;

  TrustLevel trustLevel;
  
  
  
  rv = CheckIssuerIndependentProperties(trustDomain, subject, time,
                                        endEntityOrCA,
                                        requiredKeyUsagesIfPresent,
                                        requiredEKUIfPresent, requiredPolicy,
                                        subCACount, &trustLevel);
  PRErrorCode deferredEndEntityError = 0;
  if (rv != Success) {
    if (endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
        trustLevel != TrustLevel::TrustAnchor) {
      deferredEndEntityError = PR_GetError();
    } else {
      return rv;
    }
  }

  if (trustLevel == TrustLevel::TrustAnchor) {
    ScopedCERTCertList certChain(CERT_NewCertList());
    if (!certChain) {
      PR_SetError(SEC_ERROR_NO_MEMORY, 0);
      return MapSECStatus(SECFailure);
    }

    rv = subject.PrependNSSCertToList(certChain.get());
    if (rv != Success) {
      return rv;
    }
    BackCert* child = subject.childCert;
    while (child) {
      rv = child->PrependNSSCertToList(certChain.get());
      if (rv != Success) {
        return rv;
      }
      child = child->childCert;
    }

    SECStatus srv = trustDomain.IsChainValid(certChain.get());
    if (srv != SECSuccess) {
      return MapSECStatus(srv);
    }

    
    
    results = CERT_NewCertList();
    if (!results) {
      return FatalError;
    }
    rv = subject.PrependNSSCertToList(results.get());
    return rv;
  }

  
  
  ScopedCERTCertList candidates;
  if (trustDomain.FindPotentialIssuers(&subject.GetNSSCert()->derIssuer, time,
                                       candidates) != SECSuccess) {
    return MapSECStatus(SECFailure);
  }
  if (!candidates) {
    return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER);
  }

  PRErrorCode errorToReturn = 0;

  for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
       !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
    rv = BuildForwardInner(trustDomain, subject, time, endEntityOrCA,
                           requiredEKUIfPresent, requiredPolicy,
                           n->cert, subCACount, results);
    if (rv == Success) {
      
      
      if (deferredEndEntityError != 0) {
        return Fail(FatalError, deferredEndEntityError);
      }

      SECStatus srv = trustDomain.CheckRevocation(endEntityOrCA,
                                                  subject.GetNSSCert(),
                                                  n->cert, time,
                                                  stapledOCSPResponse);
      if (srv != SECSuccess) {
        return MapSECStatus(SECFailure);
      }

      
      return subject.PrependNSSCertToList(results.get());
    }
    if (rv != RecoverableError) {
      return rv;
    }

    PRErrorCode currentError = PR_GetError();
    switch (currentError) {
      case 0:
        PR_NOT_REACHED("Error code not set!");
        return Fail(FatalError, PR_INVALID_STATE_ERROR);
      case SEC_ERROR_UNTRUSTED_CERT:
        currentError = SEC_ERROR_UNTRUSTED_ISSUER;
        break;
      default:
        break;
    }
    if (errorToReturn == 0) {
      errorToReturn = currentError;
    } else if (errorToReturn != currentError) {
      errorToReturn = SEC_ERROR_UNKNOWN_ISSUER;
    }
  }

  if (errorToReturn == 0) {
    errorToReturn = SEC_ERROR_UNKNOWN_ISSUER;
  }

  return Fail(RecoverableError, errorToReturn);
}

SECStatus
BuildCertChain(TrustDomain& trustDomain,
               CERTCertificate* certToDup,
               PRTime time,
               EndEntityOrCA endEntityOrCA,
                KeyUsages requiredKeyUsagesIfPresent,
                KeyPurposeId requiredEKUIfPresent,
               const CertPolicyId& requiredPolicy,
                const SECItem* stapledOCSPResponse,
                ScopedCERTCertList& results)
{
  PORT_Assert(certToDup);

  if (!certToDup) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  
  

  
  
  BackCert::IncludeCN includeCN
    = endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
      requiredEKUIfPresent == KeyPurposeId::id_kp_serverAuth
    ? BackCert::IncludeCN::Yes
    : BackCert::IncludeCN::No;

  BackCert cert(certToDup, nullptr, includeCN);
  Result rv = cert.Init();
  if (rv != Success) {
    return SECFailure;
  }

  rv = BuildForward(trustDomain, cert, time, endEntityOrCA,
                    requiredKeyUsagesIfPresent, requiredEKUIfPresent,
                    requiredPolicy, stapledOCSPResponse, 0, results);
  if (rv != Success) {
    results = nullptr;
    return SECFailure;
  }

  return SECSuccess;
}

PLArenaPool*
BackCert::GetArena()
{
  if (!arena) {
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  }
  return arena.get();
}

Result
BackCert::PrependNSSCertToList(CERTCertList* results)
{
  PORT_Assert(results);

  CERTCertificate* dup = CERT_DupCertificate(nssCert);
  if (CERT_AddCertToListHead(results, dup) != SECSuccess) { 
    CERT_DestroyCertificate(dup);
    return FatalError;
  }

  return Success;
}

} } 
