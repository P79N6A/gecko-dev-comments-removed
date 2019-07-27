























#include "pkixder.h"
#include "pkixgtest.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

namespace mozilla { namespace pkix {

extern Result CheckSignatureAlgorithm(Input signatureAlgorithmValue,
                                      Input signatureValue);

} } 

struct CheckSignatureAlgorithmTestParams
{
  ByteString signatureAlgorithmValue;
  ByteString signatureValue;
  Result expectedResult;
};

#define BS(s) ByteString(s, MOZILLA_PKIX_ARRAY_LENGTH(s))


static const uint8_t tlv_sha256WithRSAEncryption[] = {
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b
};



static const uint8_t tlv_sha256WithRSAEncryption_truncated[] = {
  0x06, 0x08, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01
};


static const uint8_t tlv_sha_1WithRSAEncryption[] = {
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05
};


static const uint8_t tlv_sha1WithRSASignature[] = {
  0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1d
};


static const uint8_t tlv_md5WithRSAEncryption[] = {
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04
};

static const CheckSignatureAlgorithmTestParams
  CHECKSIGNATUREALGORITHM_TEST_PARAMS[] =
{
  { 
    ByteString(),
    ByteString(),
    Result::ERROR_BAD_DER,
  },
  { 
    ByteString(),
    BS(tlv_sha256WithRSAEncryption),
    Result::ERROR_BAD_DER,
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    ByteString(),
    Result::ERROR_BAD_DER,
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption),
    Success
  },
  { 
    BS(tlv_sha256WithRSAEncryption_truncated),
    BS(tlv_sha256WithRSAEncryption),
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption_truncated),
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha_1WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption),
    Result::ERROR_SIGNATURE_ALGORITHM_MISMATCH,
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha_1WithRSAEncryption),
    Result::ERROR_SIGNATURE_ALGORITHM_MISMATCH,
  },
  { 
    BS(tlv_md5WithRSAEncryption),
    BS(tlv_md5WithRSAEncryption),
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_md5WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption),
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_md5WithRSAEncryption),
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    Success
  },
  { 
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    BS(tlv_sha256WithRSAEncryption),
    Success
  },
  { 
    
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    Success
  },
  { 
    
    BS(tlv_sha1WithRSASignature),
    BS(tlv_sha_1WithRSAEncryption),
    Success,
  },
  { 
    
    BS(tlv_sha_1WithRSAEncryption),
    BS(tlv_sha1WithRSASignature),
    Success,
  },
};

class pkixcheck_CheckSignatureAlgorithm
  : public ::testing::Test
  , public ::testing::WithParamInterface<CheckSignatureAlgorithmTestParams>
{
};

TEST_P(pkixcheck_CheckSignatureAlgorithm, CheckSignatureAlgorithm)
{
  const CheckSignatureAlgorithmTestParams& params(GetParam());

  Input signatureValueInput;
  ASSERT_EQ(Success,
            signatureValueInput.Init(params.signatureValue.data(),
                                     params.signatureValue.length()));

  Input signatureAlgorithmValueInput;
  ASSERT_EQ(Success,
            signatureAlgorithmValueInput.Init(
              params.signatureAlgorithmValue.data(),
              params.signatureAlgorithmValue.length()));

  ASSERT_EQ(params.expectedResult,
            CheckSignatureAlgorithm(signatureAlgorithmValueInput,
                                    signatureValueInput));
}

INSTANTIATE_TEST_CASE_P(
  pkixcheck_CheckSignatureAlgorithm, pkixcheck_CheckSignatureAlgorithm,
  testing::ValuesIn(CHECKSIGNATUREALGORITHM_TEST_PARAMS));

class pkixcheck_CheckSignatureAlgorithmTrustDomain
  : public DefaultCryptoTrustDomain
{
public:
  explicit pkixcheck_CheckSignatureAlgorithmTrustDomain(
             const ByteString& issuer)
    : issuer(issuer)
  {
  }

  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                      Input cert,  TrustLevel& trustLevel) override
  {
    trustLevel = InputEqualsByteString(cert, issuer)
               ? TrustLevel::TrustAnchor
               : TrustLevel::InheritsTrust;
    return Success;
  }

  Result FindIssuer(Input, IssuerChecker& checker, Time) override
  {
    EXPECT_FALSE(ENCODING_FAILED(issuer));

    Input issuerInput;
    EXPECT_EQ(Success, issuerInput.Init(issuer.data(), issuer.length()));

    bool keepGoing;
    EXPECT_EQ(Success, checker.Check(issuerInput, nullptr, keepGoing));
    EXPECT_FALSE(keepGoing);

    return Success;
  }

  Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                          const Input*,
                          const Input*) override
  {
    return Success;
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    return Success;
  }

  ByteString issuer;
};



TEST_F(pkixcheck_CheckSignatureAlgorithm, BuildCertChain)
{
  ScopedTestKeyPair keyPair(CloneReusedKeyPair());
  ASSERT_TRUE(keyPair);

  ByteString issuerExtensions[2];
  issuerExtensions[0] = CreateEncodedBasicConstraints(true, nullptr,
                                                      Critical::No);
  ASSERT_FALSE(ENCODING_FAILED(issuerExtensions[0]));

  ByteString issuer(CreateEncodedCertificate(3,
                                             sha256WithRSAEncryption,
                                             CreateEncodedSerialNumber(1),
                                             CNToDERName("issuer"),
                                             oneDayBeforeNow, oneDayAfterNow,
                                             CNToDERName("issuer"),
                                             *keyPair,
                                             issuerExtensions,
                                             *keyPair,
                                             sha256WithRSAEncryption));
  ASSERT_FALSE(ENCODING_FAILED(issuer));

  ByteString subject(CreateEncodedCertificate(3,
                                              TLV(der::SEQUENCE,
                                                  BS(tlv_sha_1WithRSAEncryption)),
                                              CreateEncodedSerialNumber(2),
                                              CNToDERName("issuer"),
                                              oneDayBeforeNow, oneDayAfterNow,
                                              CNToDERName("subject"),
                                              *keyPair,
                                              nullptr,
                                              *keyPair,
                                              sha256WithRSAEncryption));
  ASSERT_FALSE(ENCODING_FAILED(subject));

  Input subjectInput;
  ASSERT_EQ(Success, subjectInput.Init(subject.data(), subject.length()));
  pkixcheck_CheckSignatureAlgorithmTrustDomain trustDomain(issuer);
  Result rv = BuildCertChain(trustDomain, subjectInput, Now(),
                             EndEntityOrCA::MustBeEndEntity,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::anyExtendedKeyUsage,
                             CertPolicyId::anyPolicy,
                             nullptr);
  ASSERT_EQ(Result::ERROR_SIGNATURE_ALGORITHM_MISMATCH, rv);
}
