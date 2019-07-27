




#include "pkix/pkix.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

static ByteString
CreateCert(const char* issuerCN,
           const char* subjectCN,
           EndEntityOrCA endEntityOrCA,
           const ByteString& signatureAlgorithm,
            ByteString& subjectDER)
{
  static long serialNumberValue = 0;
  ++serialNumberValue;
  ByteString serialNumber(CreateEncodedSerialNumber(serialNumberValue));
  EXPECT_FALSE(ENCODING_FAILED(serialNumber));

  ByteString issuerDER(CNToDERName(issuerCN));
  EXPECT_FALSE(ENCODING_FAILED(issuerDER));
  subjectDER = CNToDERName(subjectCN);
  EXPECT_FALSE(ENCODING_FAILED(subjectDER));

  ByteString extensions[2];
  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    extensions[0] =
      CreateEncodedBasicConstraints(true, nullptr, Critical::Yes);
    EXPECT_FALSE(ENCODING_FAILED(extensions[0]));
  }

  ScopedTestKeyPair reusedKey(CloneReusedKeyPair());
  ByteString certDER(CreateEncodedCertificate(v3, signatureAlgorithm,
                                              serialNumber, issuerDER,
                                              oneDayBeforeNow, oneDayAfterNow,
                                              subjectDER, *reusedKey,
                                              extensions, *reusedKey,
                                              signatureAlgorithm));
  EXPECT_FALSE(ENCODING_FAILED(certDER));
  return certDER;
}

class AlgorithmTestsTrustDomain final : public TrustDomain
{
public:
  AlgorithmTestsTrustDomain(const ByteString& rootDER,
                            const ByteString& rootSubjectDER,
                const ByteString& intDER,
                const ByteString& intSubjectDER)
    : rootDER(rootDER)
    , rootSubjectDER(rootSubjectDER)
    , intDER(intDER)
    , intSubjectDER(intSubjectDER)
  {
  }

private:
  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&, Input candidateCert,
                       TrustLevel& trustLevel) override
  {
    if (InputEqualsByteString(candidateCert, rootDER)) {
      trustLevel = TrustLevel::TrustAnchor;
    } else {
      trustLevel = TrustLevel::InheritsTrust;
    }
    return Success;
  }

  Result FindIssuer(Input encodedIssuerName, IssuerChecker& checker, Time)
                    override
  {
    ByteString* issuerDER = nullptr;
    if (InputEqualsByteString(encodedIssuerName, rootSubjectDER)) {
      issuerDER = &rootDER;
    } else if (InputEqualsByteString(encodedIssuerName, intSubjectDER)) {
      issuerDER = &intDER;
    } else {
      
      return Success;
    }
    Input issuerCert;
    Result rv = issuerCert.Init(issuerDER->data(), issuerDER->length());
    if (rv != Success) {
      return rv;
    }
    bool keepGoing;
    return checker.Check(issuerCert, nullptr, keepGoing);
  }

  Result CheckRevocation(EndEntityOrCA, const CertID&, Time, const Input*,
                         const Input*) override
  {
    return Success;
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    return Success;
  }

  Result DigestBuf(Input input, DigestAlgorithm digestAlg,
                    uint8_t* digestBuf, size_t digestLen) override
  {
    return TestDigestBuf(input, digestAlg, digestBuf, digestLen);
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

  Result CheckECDSACurveIsAcceptable(EndEntityOrCA, NamedCurve) override
  {
    return Success;
  }

  Result VerifyECDSASignedDigest(const SignedDigest& signedDigest,
                                 Input subjectPublicKeyInfo) override
  {
    return TestVerifyECDSASignedDigest(signedDigest, subjectPublicKeyInfo);
  }

  ByteString rootDER;
  ByteString rootSubjectDER;
  ByteString intDER;
  ByteString intSubjectDER;
};

static const ByteString NO_INTERMEDIATE; 

struct ChainValidity final
{
  
  
  
  
  
  ByteString endEntitySignatureAlgorithm;
  ByteString optionalIntermediateSignatureAlgorithm;
  ByteString rootSignatureAlgorithm;
  bool isValid;
};

static const ChainValidity CHAIN_VALIDITY[] =
{
  
  
  { sha256WithRSAEncryption,
    NO_INTERMEDIATE,
    md5WithRSAEncryption,
    true
  },
  { sha256WithRSAEncryption,
    NO_INTERMEDIATE,
    md2WithRSAEncryption,
    true
  },

  
  
  { md5WithRSAEncryption,
    NO_INTERMEDIATE,
    sha256WithRSAEncryption,
    false
  },
  { md2WithRSAEncryption,
    NO_INTERMEDIATE,
    sha256WithRSAEncryption,
    false
  },
  { md2WithRSAEncryption,
    NO_INTERMEDIATE,
    md5WithRSAEncryption,
    false
  },
  { sha256WithRSAEncryption,
    md5WithRSAEncryption,
    sha256WithRSAEncryption,
    false
  },
  { sha256WithRSAEncryption,
    md2WithRSAEncryption,
    sha256WithRSAEncryption,
    false
  },
  { sha256WithRSAEncryption,
    md2WithRSAEncryption,
    md5WithRSAEncryption,
    false
  },
};

class pkixcert_IsValidChainForAlgorithm
  : public ::testing::Test
  , public ::testing::WithParamInterface<ChainValidity>
{
};

TEST_P(pkixcert_IsValidChainForAlgorithm, IsValidChainForAlgorithm)
{
  const ChainValidity& chainValidity(GetParam());
  const char* rootCN = "CN=Root";
  ByteString rootSubjectDER;
  ByteString rootEncoded(
    CreateCert(rootCN, rootCN, EndEntityOrCA::MustBeCA,
               chainValidity.rootSignatureAlgorithm, rootSubjectDER));
  EXPECT_FALSE(ENCODING_FAILED(rootEncoded));
  EXPECT_FALSE(ENCODING_FAILED(rootSubjectDER));

  const char* issuerCN = rootCN;

  const char* intermediateCN = "CN=Intermediate";
  ByteString intermediateSubjectDER;
  ByteString intermediateEncoded;
  if (chainValidity.optionalIntermediateSignatureAlgorithm != NO_INTERMEDIATE) {
    intermediateEncoded =
      CreateCert(rootCN, intermediateCN, EndEntityOrCA::MustBeCA,
                 chainValidity.optionalIntermediateSignatureAlgorithm,
                 intermediateSubjectDER);
    EXPECT_FALSE(ENCODING_FAILED(intermediateEncoded));
    EXPECT_FALSE(ENCODING_FAILED(intermediateSubjectDER));
    issuerCN = intermediateCN;
  }

  AlgorithmTestsTrustDomain trustDomain(rootEncoded, rootSubjectDER,
                                        intermediateEncoded,
                                        intermediateSubjectDER);

  const char* endEntityCN = "CN=End Entity";
  ByteString endEntitySubjectDER;
  ByteString endEntityEncoded(
    CreateCert(issuerCN, endEntityCN, EndEntityOrCA::MustBeEndEntity,
               chainValidity.endEntitySignatureAlgorithm,
               endEntitySubjectDER));
  EXPECT_FALSE(ENCODING_FAILED(endEntityEncoded));
  EXPECT_FALSE(ENCODING_FAILED(endEntitySubjectDER));

  Input endEntity;
  ASSERT_EQ(Success, endEntity.Init(endEntityEncoded.data(),
                                    endEntityEncoded.length()));
  Result expectedResult = chainValidity.isValid
                        ? Success
                        : Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED;
  ASSERT_EQ(expectedResult,
            BuildCertChain(trustDomain, endEntity, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy, nullptr));
}

INSTANTIATE_TEST_CASE_P(pkixcert_IsValidChainForAlgorithm,
                        pkixcert_IsValidChainForAlgorithm,
                        testing::ValuesIn(CHAIN_VALIDITY));
