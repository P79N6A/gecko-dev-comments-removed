






















#ifndef mozilla_pkix_pkixgtest_h
#define mozilla_pkix_pkixgtest_h

#include <ostream>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma clang diagnostic ignored "-Wshift-sign-overflow"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wundef"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#elif defined(_MSC_VER)
#pragma warning(push, 3)


#pragma warning(disable: 4224)


#pragma warning(disable: 4826)
#endif

#include "gtest/gtest.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "pkix/pkix.h"
#include "pkixtestutil.h"


namespace mozilla { namespace pkix {

inline void
PrintTo(const Result& result, ::std::ostream* os)
{
  const char* stringified = MapResultToName(result);
  if (stringified) {
    *os << stringified;
  } else {
    *os << "mozilla::pkix::Result(" << static_cast<unsigned int>(result) << ")";
  }
}

} } 

namespace mozilla { namespace pkix { namespace test {

extern const std::time_t ONE_DAY_IN_SECONDS_AS_TIME_T;

extern const std::time_t now;
extern const std::time_t oneDayBeforeNow;
extern const std::time_t oneDayAfterNow;


class EverythingFailsByDefaultTrustDomain : public TrustDomain
{
public:
  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                      Input,  TrustLevel&) override
  {
    ADD_FAILURE();
    return NotReached("GetCertTrust should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result FindIssuer(Input, IssuerChecker&, Time) override
  {
    ADD_FAILURE();
    return NotReached("FindIssuer should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                           const Input*,
                           const Input*) override
  {
    ADD_FAILURE();
    return NotReached("CheckRevocation should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    ADD_FAILURE();
    return NotReached("IsChainValid should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result DigestBuf(Input, DigestAlgorithm,  uint8_t*, size_t) override
  {
    ADD_FAILURE();
    return NotReached("DigestBuf should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result CheckECDSACurveIsAcceptable(EndEntityOrCA, NamedCurve) override
  {
    ADD_FAILURE();
    return NotReached("CheckECDSACurveIsAcceptable should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result VerifyECDSASignedDigest(const SignedDigest&, Input) override
  {
    ADD_FAILURE();
    return NotReached("VerifyECDSASignedDigest should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result CheckRSAPublicKeyModulusSizeInBits(EndEntityOrCA, unsigned int)
                                            override
  {
    ADD_FAILURE();
    return NotReached("CheckRSAPublicKeyModulusSizeInBits should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }

  Result VerifyRSAPKCS1SignedDigest(const SignedDigest&, Input) override
  {
    ADD_FAILURE();
    return NotReached("VerifyRSAPKCS1SignedDigest should not be called",
                      Result::FATAL_ERROR_LIBRARY_FAILURE);
  }
};

class DefaultCryptoTrustDomain : public EverythingFailsByDefaultTrustDomain
{
  Result DigestBuf(Input item, DigestAlgorithm digestAlg,
                    uint8_t* digestBuf, size_t digestBufLen) override
  {
    return TestDigestBuf(item, digestAlg, digestBuf, digestBufLen);
  }

  Result CheckECDSACurveIsAcceptable(EndEntityOrCA, NamedCurve) override
  {
    return Success;
  }

  Result VerifyECDSASignedDigest(const SignedDigest& signedDigest,
                                 Input subjectPublicKeyInfo) override
  {
    return TestVerifyECDSASignedDigest(signedDigest, subjectPublicKeyInfo);
  }

  Result CheckRSAPublicKeyModulusSizeInBits(EndEntityOrCA, unsigned int)
                                            override
  {
    return Success;
  }

  Result VerifyRSAPKCS1SignedDigest(const SignedDigest& signedDigest,
                                    Input subjectPublicKeyInfo) override
  {
    return TestVerifyRSAPKCS1SignedDigest(signedDigest, subjectPublicKeyInfo);
  }
};

} } } 

#endif 
