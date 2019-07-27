























#include "pkixder.h"
#include "pkixgtest.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

namespace mozilla { namespace pkix {

extern Result CheckSignatureAlgorithm(
                TrustDomain& trustDomain, EndEntityOrCA endEntityOrCA,
                const der::SignedDataWithSignature& signedData,
                Input signatureValue);

} } 

struct CheckSignatureAlgorithmTestParams
{
  ByteString signatureAlgorithmValue;
  ByteString signatureValue;
  unsigned int signatureLengthInBytes;
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
    2048 / 8,
    Result::ERROR_BAD_DER,
  },
  { 
    ByteString(),
    BS(tlv_sha256WithRSAEncryption),
    2048 / 8,
    Result::ERROR_BAD_DER,
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    ByteString(),
    2048 / 8,
    Result::ERROR_BAD_DER,
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption),
    2048 / 8,
    Success
  },
  { 
    BS(tlv_sha256WithRSAEncryption_truncated),
    BS(tlv_sha256WithRSAEncryption),
    2048 / 8,
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption_truncated),
    2048 / 8,
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha_1WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption),
    2048 / 8,
    Result::ERROR_SIGNATURE_ALGORITHM_MISMATCH,
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha_1WithRSAEncryption),
    2048 / 8,
    Result::ERROR_SIGNATURE_ALGORITHM_MISMATCH,
  },
  { 
    BS(tlv_md5WithRSAEncryption),
    BS(tlv_md5WithRSAEncryption),
    2048 / 8,
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_md5WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption),
    2048 / 8,
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_md5WithRSAEncryption),
    2048 / 8,
    Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED
  },
  { 
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    2048 / 8,
    Success
  },
  { 
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    BS(tlv_sha256WithRSAEncryption),
    2048 / 8,
    Success
  },
  { 
    
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption) + TLV(der::NULLTag, ByteString()),
    2048 / 8,
    Success
  },
  { 
    
    BS(tlv_sha1WithRSASignature),
    BS(tlv_sha_1WithRSAEncryption),
    2048 / 8,
    Success,
  },
  { 
    
    BS(tlv_sha_1WithRSAEncryption),
    BS(tlv_sha1WithRSASignature),
    2048 / 8,
    Success,
  },
  { 
    
    
    BS(tlv_sha256WithRSAEncryption),
    BS(tlv_sha256WithRSAEncryption),
    (2048 / 8) - 1,
    Success
  },
};

class pkixcheck_CheckSignatureAlgorithm
  : public ::testing::Test
  , public ::testing::WithParamInterface<CheckSignatureAlgorithmTestParams>
{
};

class pkixcheck_CheckSignatureAlgorithm_TrustDomain final
  : public EverythingFailsByDefaultTrustDomain
{
public:
  explicit pkixcheck_CheckSignatureAlgorithm_TrustDomain(
             unsigned int publicKeySizeInBits)
    : publicKeySizeInBits(publicKeySizeInBits)
    , checkedDigestAlgorithm(false)
    , checkedModulusSizeInBits(false)
  {
  }

  Result CheckSignatureDigestAlgorithm(DigestAlgorithm) override
  {
    checkedDigestAlgorithm = true;
    return Success;
  }

  Result CheckRSAPublicKeyModulusSizeInBits(EndEntityOrCA endEntityOrCA,
                                            unsigned int modulusSizeInBits)
    override
  {
    EXPECT_EQ(EndEntityOrCA::MustBeEndEntity, endEntityOrCA);
    EXPECT_EQ(publicKeySizeInBits, modulusSizeInBits);
    checkedModulusSizeInBits = true;
    return Success;
  }

  const unsigned int publicKeySizeInBits;
  bool checkedDigestAlgorithm;
  bool checkedModulusSizeInBits;
};

TEST_P(pkixcheck_CheckSignatureAlgorithm, CheckSignatureAlgorithm)
{
  const CheckSignatureAlgorithmTestParams& params(GetParam());

  Input signatureValueInput;
  ASSERT_EQ(Success,
            signatureValueInput.Init(params.signatureValue.data(),
                                     params.signatureValue.length()));

  pkixcheck_CheckSignatureAlgorithm_TrustDomain
    trustDomain(params.signatureLengthInBytes * 8);

  der::SignedDataWithSignature signedData;
  ASSERT_EQ(Success,
            signedData.algorithm.Init(params.signatureAlgorithmValue.data(),
                                      params.signatureAlgorithmValue.length()));

  ByteString dummySignature(params.signatureLengthInBytes, 0xDE);
  ASSERT_EQ(Success,
            signedData.signature.Init(dummySignature.data(),
                                      dummySignature.length()));

  ASSERT_EQ(params.expectedResult,
            CheckSignatureAlgorithm(trustDomain, EndEntityOrCA::MustBeEndEntity,
                                    signedData, signatureValueInput));
  ASSERT_EQ(params.expectedResult == Success,
            trustDomain.checkedDigestAlgorithm);
  ASSERT_EQ(params.expectedResult == Success,
            trustDomain.checkedModulusSizeInBits);
}

INSTANTIATE_TEST_CASE_P(
  pkixcheck_CheckSignatureAlgorithm, pkixcheck_CheckSignatureAlgorithm,
  testing::ValuesIn(CHECKSIGNATUREALGORITHM_TEST_PARAMS));

class pkixcheck_CheckSignatureAlgorithm_BuildCertChain_TrustDomain
  : public DefaultCryptoTrustDomain
{
public:
  explicit pkixcheck_CheckSignatureAlgorithm_BuildCertChain_TrustDomain(
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
  ASSERT_TRUE(keyPair.get());

  ByteString issuerExtensions[2];
  issuerExtensions[0] = CreateEncodedBasicConstraints(true, nullptr,
                                                      Critical::No);
  ASSERT_FALSE(ENCODING_FAILED(issuerExtensions[0]));

  ByteString issuer(CreateEncodedCertificate(3,
                                             sha256WithRSAEncryption(),
                                             CreateEncodedSerialNumber(1),
                                             CNToDERName("issuer"),
                                             oneDayBeforeNow, oneDayAfterNow,
                                             CNToDERName("issuer"),
                                             *keyPair,
                                             issuerExtensions,
                                             *keyPair,
                                             sha256WithRSAEncryption()));
  ASSERT_FALSE(ENCODING_FAILED(issuer));

  ByteString subject(CreateEncodedCertificate(3,
                                              sha1WithRSAEncryption(),
                                              CreateEncodedSerialNumber(2),
                                              CNToDERName("issuer"),
                                              oneDayBeforeNow, oneDayAfterNow,
                                              CNToDERName("subject"),
                                              *keyPair,
                                              nullptr,
                                              *keyPair,
                                              sha256WithRSAEncryption()));
  ASSERT_FALSE(ENCODING_FAILED(subject));

  Input subjectInput;
  ASSERT_EQ(Success, subjectInput.Init(subject.data(), subject.length()));
  pkixcheck_CheckSignatureAlgorithm_BuildCertChain_TrustDomain
    trustDomain(issuer);
  Result rv = BuildCertChain(trustDomain, subjectInput, Now(),
                             EndEntityOrCA::MustBeEndEntity,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::anyExtendedKeyUsage,
                             CertPolicyId::anyPolicy,
                             nullptr);
  ASSERT_EQ(Result::ERROR_SIGNATURE_ALGORITHM_MISMATCH, rv);
}
