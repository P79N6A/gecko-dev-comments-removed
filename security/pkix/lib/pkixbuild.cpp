























#include "pkix/pkix.h"

#include <limits>

#include "pkixcheck.h"

namespace mozilla { namespace pkix {

static Result BuildForward(TrustDomain& trustDomain,
                           const BackCert& subject,
                           PRTime time,
                           KeyUsage requiredKeyUsageIfPresent,
                           KeyPurposeId requiredEKUIfPresent,
                           const CertPolicyId& requiredPolicy,
                            const SECItem* stapledOCSPResponse,
                           unsigned int subCACount);

TrustDomain::IssuerChecker::IssuerChecker() { }
TrustDomain::IssuerChecker::~IssuerChecker() { }



class PathBuildingStep : public TrustDomain::IssuerChecker
{
public:
  PathBuildingStep(TrustDomain& trustDomain, const BackCert& subject,
                   PRTime time, KeyPurposeId requiredEKUIfPresent,
                   const CertPolicyId& requiredPolicy,
                    const SECItem* stapledOCSPResponse,
                   unsigned int subCACount)
    : trustDomain(trustDomain)
    , subject(subject)
    , time(time)
    , requiredEKUIfPresent(requiredEKUIfPresent)
    , requiredPolicy(requiredPolicy)
    , stapledOCSPResponse(stapledOCSPResponse)
    , subCACount(subCACount)
    , result(SEC_ERROR_LIBRARY_FAILURE)
    , resultWasSet(false)
  {
  }

  SECStatus Check(const SECItem& potentialIssuerDER,
                   const SECItem* additionalNameConstraints,
                   bool& keepGoing);

  Result CheckResult() const;

private:
  TrustDomain& trustDomain;
  const BackCert& subject;
  const PRTime time;
  const KeyPurposeId requiredEKUIfPresent;
  const CertPolicyId& requiredPolicy;
   SECItem const* const stapledOCSPResponse;
  const unsigned int subCACount;

  SECStatus RecordResult(PRErrorCode currentResult,  bool& keepGoing);
  PRErrorCode result;
  bool resultWasSet;

  PathBuildingStep(const PathBuildingStep&) ;
  void operator=(const PathBuildingStep&) ;
};

SECStatus
PathBuildingStep::RecordResult(PRErrorCode newResult,
                                bool& keepGoing)
{
  if (newResult == SEC_ERROR_UNTRUSTED_CERT) {
    newResult = SEC_ERROR_UNTRUSTED_ISSUER;
  }
  if (resultWasSet) {
    if (result == 0) {
      PR_NOT_REACHED("RecordResult called after finding a chain");
      PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
      return SECFailure;
    }
    
    
    
    
    if (newResult != 0 && newResult != result) {
      newResult = SEC_ERROR_UNKNOWN_ISSUER;
    }
  }

  result = newResult;
  resultWasSet = true;
  keepGoing = result != 0;
  return SECSuccess;
}

Result
PathBuildingStep::CheckResult() const
{
  if (!resultWasSet) {
    return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER);
  }
  if (result == 0) {
    return Success;
  }
  PR_SetError(result, 0);
  return MapSECStatus(SECFailure);
}


SECStatus
PathBuildingStep::Check(const SECItem& potentialIssuerDER,
                         const SECItem* additionalNameConstraints,
                         bool& keepGoing)
{
  BackCert potentialIssuer(potentialIssuerDER, EndEntityOrCA::MustBeCA,
                           &subject);
  Result rv = potentialIssuer.Init();
  if (rv != Success) {
    return RecordResult(PR_GetError(), keepGoing);
  }

  
  

  
  
  
  bool loopDetected = false;
  for (const BackCert* prev = potentialIssuer.childCert;
       !loopDetected && prev != nullptr; prev = prev->childCert) {
    if (SECITEM_ItemsAreEqual(&potentialIssuer.GetSubjectPublicKeyInfo(),
                              &prev->GetSubjectPublicKeyInfo()) &&
        SECITEM_ItemsAreEqual(&potentialIssuer.GetSubject(),
                              &prev->GetSubject())) {
      
      return RecordResult(SEC_ERROR_UNKNOWN_ISSUER, keepGoing);
    }
  }

  if (potentialIssuer.GetNameConstraints()) {
    rv = CheckNameConstraints(*potentialIssuer.GetNameConstraints(),
                              subject, requiredEKUIfPresent);
    if (rv != Success) {
       return RecordResult(PR_GetError(), keepGoing);
    }
  }

  if (additionalNameConstraints) {
    rv = CheckNameConstraints(*additionalNameConstraints, subject,
                              requiredEKUIfPresent);
    if (rv != Success) {
       return RecordResult(PR_GetError(), keepGoing);
    }
  }

  
  
  
  rv = BuildForward(trustDomain, potentialIssuer, time, KeyUsage::keyCertSign,
                    requiredEKUIfPresent, requiredPolicy, nullptr, subCACount);
  if (rv != Success) {
    return RecordResult(PR_GetError(), keepGoing);
  }

  SECStatus srv = trustDomain.VerifySignedData(
                                subject.GetSignedData(),
                                potentialIssuer.GetSubjectPublicKeyInfo());
  if (srv != SECSuccess) {
    return RecordResult(PR_GetError(), keepGoing);
  }

  CertID certID(subject.GetIssuer(), potentialIssuer.GetSubjectPublicKeyInfo(),
                subject.GetSerialNumber());
  srv = trustDomain.CheckRevocation(subject.endEntityOrCA, certID, time,
                                    stapledOCSPResponse,
                                    subject.GetAuthorityInfoAccess());
  if (srv != SECSuccess) {
    return RecordResult(PR_GetError(), keepGoing);
  }

  return RecordResult(0, keepGoing);
}







static Result
BuildForward(TrustDomain& trustDomain,
             const BackCert& subject,
             PRTime time,
             KeyUsage requiredKeyUsageIfPresent,
             KeyPurposeId requiredEKUIfPresent,
             const CertPolicyId& requiredPolicy,
              const SECItem* stapledOCSPResponse,
             unsigned int subCACount)
{
  Result rv;

  TrustLevel trustLevel;
  
  
  
  rv = CheckIssuerIndependentProperties(trustDomain, subject, time,
                                        requiredKeyUsageIfPresent,
                                        requiredEKUIfPresent, requiredPolicy,
                                        subCACount, &trustLevel);
  PRErrorCode deferredEndEntityError = 0;
  if (rv != Success) {
    if (subject.endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
        trustLevel != TrustLevel::TrustAnchor) {
      deferredEndEntityError = PR_GetError();
    } else {
      return rv;
    }
  }

  if (trustLevel == TrustLevel::TrustAnchor) {
    

    NonOwningDERArray chain;
    for (const BackCert* cert = &subject; cert; cert = cert->childCert) {
      Result rv = chain.Append(cert->GetDER());
      if (rv != Success) {
        PR_NOT_REACHED("NonOwningDERArray::SetItem failed.");
        return rv;
      }
    }

    
    
    SECStatus srv = trustDomain.IsChainValid(chain);
    if (srv != SECSuccess) {
      return MapSECStatus(srv);
    }

    return Success;
  }

  if (subject.endEntityOrCA == EndEntityOrCA::MustBeCA) {
    
    
    static const unsigned int MAX_SUBCA_COUNT = 6;
    static_assert(1 + MAX_SUBCA_COUNT + 1 ==
                  NonOwningDERArray::MAX_LENGTH,
                  "MAX_SUBCA_COUNT and NonOwningDERArray::MAX_LENGTH mismatch.");
    if (subCACount >= MAX_SUBCA_COUNT) {
      return Fail(RecoverableError, SEC_ERROR_UNKNOWN_ISSUER);
    }
    ++subCACount;
  } else {
    PR_ASSERT(subCACount == 0);
  }

  

  PathBuildingStep pathBuilder(trustDomain, subject, time,
                               requiredEKUIfPresent, requiredPolicy,
                               stapledOCSPResponse, subCACount);

  
  if (trustDomain.FindIssuer(subject.GetIssuer(), pathBuilder, time)
        != SECSuccess) {
    return MapSECStatus(SECFailure);
  }

  rv = pathBuilder.CheckResult();
  if (rv != Success) {
    return rv;
  }

  
  
  if (deferredEndEntityError != 0) {
    return Fail(RecoverableError, deferredEndEntityError);
  }

  
  return Success;
}

SECStatus
BuildCertChain(TrustDomain& trustDomain, const SECItem& certDER,
               PRTime time, EndEntityOrCA endEntityOrCA,
               KeyUsage requiredKeyUsageIfPresent,
               KeyPurposeId requiredEKUIfPresent,
               const CertPolicyId& requiredPolicy,
                const SECItem* stapledOCSPResponse)
{
  
  
  BackCert cert(certDER, endEntityOrCA, nullptr);
  Result rv = cert.Init();
  if (rv != Success) {
    return SECFailure;
  }

  rv = BuildForward(trustDomain, cert, time, requiredKeyUsageIfPresent,
                    requiredEKUIfPresent, requiredPolicy, stapledOCSPResponse,
                    0);
  if (rv != Success) {
    return SECFailure;
  }

  return SECSuccess;
}

} } 
