























#include "pkix/pkix.h"

#include <limits>

#include "pkixcheck.h"

namespace mozilla { namespace pkix {

static Result BuildForward(TrustDomain& trustDomain,
                           const BackCert& subject,
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
                  const BackCert& subject,
                  PRTime time,
                  KeyPurposeId requiredEKUIfPresent,
                  const CertPolicyId& requiredPolicy,
                  const SECItem& potentialIssuerDER,
                  unsigned int subCACount,
                   ScopedCERTCertList& results)
{
  BackCert potentialIssuer(potentialIssuerDER, &subject,
                           BackCert::IncludeCN::No);
  Result rv = potentialIssuer.Init();
  if (rv != Success) {
    return rv;
  }

  
  

  
  
  
  bool loopDetected = false;
  for (const BackCert* prev = potentialIssuer.childCert;
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

  SECStatus srv = trustDomain.VerifySignedData(
                                subject.GetSignedData(),
                                potentialIssuer.GetSubjectPublicKeyInfo());
  if (srv != SECSuccess) {
    return MapSECStatus(srv);
  }

  return Success;
}







static Result
BuildForward(TrustDomain& trustDomain,
             const BackCert& subject,
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
    for (const BackCert* cert = &subject; cert; cert = cert->childCert) {
      ScopedCERTCertificate
        nssCert(CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                        const_cast<SECItem*>(&cert->GetDER()),
                                        nullptr, false, true));
      if (CERT_AddCertToListHead(results.get(), nssCert.get()) != SECSuccess) {
        return MapSECStatus(SECFailure);
      }
      nssCert.release(); 
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
                                    subject.GetAuthorityInfoAccess());
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
BuildCertChain(TrustDomain& trustDomain, const SECItem& certDER,
               PRTime time, EndEntityOrCA endEntityOrCA,
               KeyUsage requiredKeyUsageIfPresent,
               KeyPurposeId requiredEKUIfPresent,
               const CertPolicyId& requiredPolicy,
                const SECItem* stapledOCSPResponse,
                ScopedCERTCertList& results)
{
  
  
  BackCert::IncludeCN includeCN
    = endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
      requiredEKUIfPresent == KeyPurposeId::id_kp_serverAuth
    ? BackCert::IncludeCN::Yes
    : BackCert::IncludeCN::No;

  BackCert cert(certDER, nullptr, includeCN);
  Result rv = cert.Init();
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
