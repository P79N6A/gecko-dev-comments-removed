























#include "pkix/pkix.h"

#include "pkixcheck.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

static Result BuildForward(TrustDomain& trustDomain,
                           const BackCert& subject,
                           Time time,
                           KeyUsage requiredKeyUsageIfPresent,
                           KeyPurposeId requiredEKUIfPresent,
                           const CertPolicyId& requiredPolicy,
                            const Input* stapledOCSPResponse,
                           unsigned int subCACount);

TrustDomain::IssuerChecker::IssuerChecker() { }
TrustDomain::IssuerChecker::~IssuerChecker() { }



class PathBuildingStep : public TrustDomain::IssuerChecker
{
public:
  PathBuildingStep(TrustDomain& trustDomain, const BackCert& subject,
                   Time time, KeyPurposeId requiredEKUIfPresent,
                   const CertPolicyId& requiredPolicy,
                    const Input* stapledOCSPResponse,
                   unsigned int subCACount)
    : trustDomain(trustDomain)
    , subject(subject)
    , time(time)
    , requiredEKUIfPresent(requiredEKUIfPresent)
    , requiredPolicy(requiredPolicy)
    , stapledOCSPResponse(stapledOCSPResponse)
    , subCACount(subCACount)
    , result(Result::FATAL_ERROR_LIBRARY_FAILURE)
    , resultWasSet(false)
  {
  }

  Result Check(Input potentialIssuerDER,
                const Input* additionalNameConstraints,
                bool& keepGoing);

  Result CheckResult() const;

private:
  TrustDomain& trustDomain;
  const BackCert& subject;
  const Time time;
  const KeyPurposeId requiredEKUIfPresent;
  const CertPolicyId& requiredPolicy;
   Input const* const stapledOCSPResponse;
  const unsigned int subCACount;

  Result RecordResult(Result currentResult,  bool& keepGoing);
  Result result;
  bool resultWasSet;

  PathBuildingStep(const PathBuildingStep&) ;
  void operator=(const PathBuildingStep&) ;
};

Result
PathBuildingStep::RecordResult(Result newResult,  bool& keepGoing)
{
  if (newResult == Result::ERROR_UNTRUSTED_CERT) {
    newResult = Result::ERROR_UNTRUSTED_ISSUER;
  } else if (newResult == Result::ERROR_EXPIRED_CERTIFICATE) {
    newResult = Result::ERROR_EXPIRED_ISSUER_CERTIFICATE;
  }

  if (resultWasSet) {
    if (result == Success) {
      PR_NOT_REACHED("RecordResult called after finding a chain");
      return Result::FATAL_ERROR_INVALID_STATE;
    }
    
    
    
    
    if (newResult != Success && newResult != result) {
      newResult = Result::ERROR_UNKNOWN_ISSUER;
    }
  }

  result = newResult;
  resultWasSet = true;
  keepGoing = result != Success;
  return Success;
}

Result
PathBuildingStep::CheckResult() const
{
  if (!resultWasSet) {
    return Result::ERROR_UNKNOWN_ISSUER;
  }
  return result;
}


Result
PathBuildingStep::Check(Input potentialIssuerDER,
            const Input* additionalNameConstraints,
                 bool& keepGoing)
{
  BackCert potentialIssuer(potentialIssuerDER, EndEntityOrCA::MustBeCA,
                           &subject);
  Result rv = potentialIssuer.Init();
  if (rv != Success) {
    return RecordResult(rv, keepGoing);
  }

  
  

  
  
  
  bool loopDetected = false;
  for (const BackCert* prev = potentialIssuer.childCert;
       !loopDetected && prev != nullptr; prev = prev->childCert) {
    if (InputsAreEqual(potentialIssuer.GetSubjectPublicKeyInfo(),
                       prev->GetSubjectPublicKeyInfo()) &&
        InputsAreEqual(potentialIssuer.GetSubject(), prev->GetSubject())) {
      
      return RecordResult(Result::ERROR_UNKNOWN_ISSUER, keepGoing);
    }
  }

  if (potentialIssuer.GetNameConstraints()) {
    rv = CheckNameConstraints(*potentialIssuer.GetNameConstraints(),
                              subject, requiredEKUIfPresent);
    if (rv != Success) {
       return RecordResult(rv, keepGoing);
    }
  }

  if (additionalNameConstraints) {
    rv = CheckNameConstraints(*additionalNameConstraints, subject,
                              requiredEKUIfPresent);
    if (rv != Success) {
       return RecordResult(rv, keepGoing);
    }
  }

  
  
  
  rv = BuildForward(trustDomain, potentialIssuer, time, KeyUsage::keyCertSign,
                    requiredEKUIfPresent, requiredPolicy, nullptr, subCACount);
  if (rv != Success) {
    return RecordResult(rv, keepGoing);
  }

  rv = trustDomain.VerifySignedData(subject.GetSignedData(),
                                    potentialIssuer.GetSubjectPublicKeyInfo());
  if (rv != Success) {
    return RecordResult(rv, keepGoing);
  }

  CertID certID(subject.GetIssuer(), potentialIssuer.GetSubjectPublicKeyInfo(),
                subject.GetSerialNumber());
  rv = trustDomain.CheckRevocation(subject.endEntityOrCA, certID, time,
                                   stapledOCSPResponse,
                                   subject.GetAuthorityInfoAccess());
  if (rv != Success) {
    return RecordResult(rv, keepGoing);
  }

  return RecordResult(Success, keepGoing);
}







static Result
BuildForward(TrustDomain& trustDomain,
             const BackCert& subject,
             Time time,
             KeyUsage requiredKeyUsageIfPresent,
             KeyPurposeId requiredEKUIfPresent,
             const CertPolicyId& requiredPolicy,
              const Input* stapledOCSPResponse,
             unsigned int subCACount)
{
  Result rv;

  TrustLevel trustLevel;
  
  
  
  rv = CheckIssuerIndependentProperties(trustDomain, subject, time,
                                        requiredKeyUsageIfPresent,
                                        requiredEKUIfPresent, requiredPolicy,
                                        subCACount, trustLevel);
  Result deferredEndEntityError = Success;
  if (rv != Success) {
    if (subject.endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
        trustLevel != TrustLevel::TrustAnchor) {
      deferredEndEntityError = rv;
    } else {
      return rv;
    }
  }

  if (trustLevel == TrustLevel::TrustAnchor) {
    

    NonOwningDERArray chain;
    for (const BackCert* cert = &subject; cert; cert = cert->childCert) {
      rv = chain.Append(cert->GetDER());
      if (rv != Success) {
        PR_NOT_REACHED("NonOwningDERArray::SetItem failed.");
        return rv;
      }
    }

    
    
    return trustDomain.IsChainValid(chain);
  }

  if (subject.endEntityOrCA == EndEntityOrCA::MustBeCA) {
    
    
    static const unsigned int MAX_SUBCA_COUNT = 6;
    static_assert(1 + MAX_SUBCA_COUNT + 1 ==
                  NonOwningDERArray::MAX_LENGTH,
                  "MAX_SUBCA_COUNT and NonOwningDERArray::MAX_LENGTH mismatch.");
    if (subCACount >= MAX_SUBCA_COUNT) {
      return Result::ERROR_UNKNOWN_ISSUER;
    }
    ++subCACount;
  } else {
    PR_ASSERT(subCACount == 0);
  }

  

  PathBuildingStep pathBuilder(trustDomain, subject, time,
                               requiredEKUIfPresent, requiredPolicy,
                               stapledOCSPResponse, subCACount);

  
  rv = trustDomain.FindIssuer(subject.GetIssuer(), pathBuilder, time);
  if (rv != Success) {
    return rv;
  }

  rv = pathBuilder.CheckResult();
  if (rv != Success) {
    return rv;
  }

  
  
  if (deferredEndEntityError != Success) {
    return deferredEndEntityError;
  }

  
  return Success;
}

Result
BuildCertChain(TrustDomain& trustDomain, Input certDER,
               Time time, EndEntityOrCA endEntityOrCA,
               KeyUsage requiredKeyUsageIfPresent,
               KeyPurposeId requiredEKUIfPresent,
               const CertPolicyId& requiredPolicy,
                const Input* stapledOCSPResponse)
{
  
  
  BackCert cert(certDER, endEntityOrCA, nullptr);
  Result rv = cert.Init();
  if (rv != Success) {
    return rv;
  }

  
  
  
  rv = trustDomain.CheckPublicKey(cert.GetSubjectPublicKeyInfo());
  if (rv != Success) {
    return rv;
  }

  return BuildForward(trustDomain, cert, time, requiredKeyUsageIfPresent,
                      requiredEKUIfPresent, requiredPolicy, stapledOCSPResponse,
                      0);
}

} } 
