























#include "pkix/pkix.h"

#include <limits>

#include "pkixcheck.h"
#include "pkixder.h"

namespace mozilla { namespace pkix {







Result
BackCert::Init(const SECItem& certDER)
{
  
  
  
  
  
  nssCert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                    const_cast<SECItem*>(&certDER),
                                    nullptr, false, true);
  if (!nssCert) {
    return MapSECStatus(SECFailure);
  }

  if (nssCert->version.len == 1 &&
      nssCert->version.data[0] == static_cast<uint8_t>(der::Version::v3)) {
    version = der::Version::v3;
  } else if (nssCert->version.len == 1 &&
             nssCert->version.data[0] == static_cast<uint8_t>(der::Version::v2)) {
    version = der::Version::v2;
  } else if (nssCert->version.len == 1 &&
             nssCert->version.data[0] == static_cast<uint8_t>(der::Version::v2)) {
    
    
    version = der::Version::v1;
  } else if (nssCert->version.len == 0) {
    version = der::Version::v1;
  } else {
    
    
    return Fail(RecoverableError, SEC_ERROR_BAD_DER);
  }

  const CERTCertExtension* const* exts = nssCert->extensions;
  if (!exts) {
    return Success;
  }

  
  
  
  
  
  if (version != der::Version::v3) {
    return Fail(RecoverableError, SEC_ERROR_EXTENSION_VALUE_INVALID);
  }

  const SECItem* dummyEncodedSubjectKeyIdentifier = nullptr;
  const SECItem* dummyEncodedAuthorityKeyIdentifier = nullptr;
  const SECItem* dummyEncodedSubjectAltName = nullptr;

  for (const CERTCertExtension* ext = *exts; ext; ext = *++exts) {
    const SECItem** out = nullptr;

    
    static const uint8_t id_ce[] = {
      0x55, 0x1d
    };

    
    static const uint8_t id_pe_authorityInfoAccess[] = {
      0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01
    };

    if (ext->id.len == PR_ARRAY_SIZE(id_ce) + 1 &&
        !memcmp(ext->id.data, id_ce, PR_ARRAY_SIZE(id_ce))) {
      switch (ext->id.data[ext->id.len - 1]) {
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
    } else if (ext->id.len == PR_ARRAY_SIZE(id_pe_authorityInfoAccess) &&
               !memcmp(ext->id.data, id_pe_authorityInfoAccess,
                       PR_ARRAY_SIZE(id_pe_authorityInfoAccess))) {
      
      
      
      out = &encodedAuthorityInfoAccess;
    }

    
    
    
    
    if (!out && ext->critical.data && ext->critical.len > 0) {
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

Result
BackCert::VerifyOwnSignatureWithKey(TrustDomain& trustDomain,
                                    const SECItem& subjectPublicKeyInfo) const
{
  return MapSECStatus(trustDomain.VerifySignedData(&nssCert->signatureWrap,
                                                   subjectPublicKeyInfo));
}

static Result BuildForward(TrustDomain& trustDomain,
                           BackCert& subject,
                           PRTime time,
                           EndEntityOrCA endEntityOrCA,
                           KeyUsage requiredKeyUsageIfPresent,
                           KeyPurposeId requiredEKUIfPresent,
                           const CertPolicyId& requiredPolicy,
                            const SECItem* stapledOCSPResponse,
                           unsigned int subCACount,
                            ScopedCERTCertList& results);


static Result
BuildForwardInner(TrustDomain& trustDomain,
                  BackCert& subject,
                  PRTime time,
                  KeyPurposeId requiredEKUIfPresent,
                  const CertPolicyId& requiredPolicy,
                  const SECItem& potentialIssuerDER,
                  unsigned int subCACount,
                   ScopedCERTCertList& results)
{
  BackCert potentialIssuer(&subject, BackCert::IncludeCN::No);
  Result rv = potentialIssuer.Init(potentialIssuerDER);
  if (rv != Success) {
    return rv;
  }

  
  

  
  
  
  bool loopDetected = false;
  for (BackCert* prev = potentialIssuer.childCert;
       !loopDetected && prev != nullptr; prev = prev->childCert) {
    if (SECITEM_ItemsAreEqual(&potentialIssuer.GetSubjectPublicKeyInfo(),
                              &prev->GetSubjectPublicKeyInfo()) &&
        SECITEM_ItemsAreEqual(&potentialIssuer.GetSubject(),
                              &prev->GetSubject())) {
      return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER); 
    }
  }

  rv = CheckNameConstraints(potentialIssuer);
  if (rv != Success) {
    return rv;
  }

  
  
  
  rv = BuildForward(trustDomain, potentialIssuer, time, EndEntityOrCA::MustBeCA,
                    KeyUsage::keyCertSign, requiredEKUIfPresent,
                    requiredPolicy, nullptr, subCACount, results);
  if (rv != Success) {
    return rv;
  }

  return subject.VerifyOwnSignatureWithKey(
                   trustDomain, potentialIssuer.GetSubjectPublicKeyInfo());
}







static Result
BuildForward(TrustDomain& trustDomain,
             BackCert& subject,
             PRTime time,
             EndEntityOrCA endEntityOrCA,
             KeyUsage requiredKeyUsageIfPresent,
             KeyPurposeId requiredEKUIfPresent,
             const CertPolicyId& requiredPolicy,
              const SECItem* stapledOCSPResponse,
             unsigned int subCACount,
              ScopedCERTCertList& results)
{
  Result rv;

  TrustLevel trustLevel;
  
  
  
  rv = CheckIssuerIndependentProperties(trustDomain, subject, time,
                                        endEntityOrCA,
                                        requiredKeyUsageIfPresent,
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
    

    
    results = CERT_NewCertList();
    if (!results) {
      return MapSECStatus(SECFailure);
    }
    for (BackCert* cert = &subject; cert; cert = cert->childCert) {
      CERTCertificate* dup = CERT_DupCertificate(cert->GetNSSCert());
      if (CERT_AddCertToListHead(results.get(), dup) != SECSuccess) {
        CERT_DestroyCertificate(dup);
        return MapSECStatus(SECFailure);
      }
      
    }

    
    
    SECStatus srv = trustDomain.IsChainValid(results.get());
    if (srv != SECSuccess) {
      return MapSECStatus(srv);
    }

    return Success;
  }

  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    
    
    static const unsigned int MAX_SUBCA_COUNT = 6;
    if (subCACount >= MAX_SUBCA_COUNT) {
      return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER);
    }
    ++subCACount;
  } else {
    PR_ASSERT(subCACount == 0);
  }

  
  
  ScopedCERTCertList candidates;
  if (trustDomain.FindPotentialIssuers(&subject.GetIssuer(), time,
                                       candidates) != SECSuccess) {
    return MapSECStatus(SECFailure);
  }
  if (!candidates) {
    return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER);
  }

  PRErrorCode errorToReturn = 0;

  for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
       !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
    rv = BuildForwardInner(trustDomain, subject, time, requiredEKUIfPresent,
                           requiredPolicy, n->cert->derCert, subCACount,
                           results);
    if (rv == Success) {
      
      
      if (deferredEndEntityError != 0) {
        return Fail(FatalError, deferredEndEntityError);
      }

      CertID certID(subject.GetIssuer(), n->cert->derPublicKey,
                    subject.GetSerialNumber());
      SECStatus srv = trustDomain.CheckRevocation(
                                    endEntityOrCA, certID, time,
                                    stapledOCSPResponse,
                                    subject.encodedAuthorityInfoAccess);
      if (srv != SECSuccess) {
        return MapSECStatus(SECFailure);
      }

      
      
      return Success;
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
               const CERTCertificate* nssCert,
               PRTime time,
               EndEntityOrCA endEntityOrCA,
               KeyUsage requiredKeyUsageIfPresent,
               KeyPurposeId requiredEKUIfPresent,
               const CertPolicyId& requiredPolicy,
                const SECItem* stapledOCSPResponse,
                ScopedCERTCertList& results)
{
  if (!nssCert) {
    PR_NOT_REACHED("null cert passed to BuildCertChain");
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  
  
  BackCert::IncludeCN includeCN
    = endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
      requiredEKUIfPresent == KeyPurposeId::id_kp_serverAuth
    ? BackCert::IncludeCN::Yes
    : BackCert::IncludeCN::No;

  BackCert cert(nullptr, includeCN);
  Result rv = cert.Init(nssCert->derCert);
  if (rv != Success) {
    return SECFailure;
  }

  rv = BuildForward(trustDomain, cert, time, endEntityOrCA,
                    requiredKeyUsageIfPresent, requiredEKUIfPresent,
                    requiredPolicy, stapledOCSPResponse, 0, results);
  if (rv != Success) {
    results = nullptr;
    return SECFailure;
  }

  return SECSuccess;
}

} } 
