























#if defined(_MSC_VER) && _MSC_VER < 1900



#pragma warning(push)
#pragma warning(disable: 4702)
#endif

#include <map>

#if defined(_MSC_VER) && _MSC_VER < 1900
#pragma warning(pop)
#endif

#include "pkixgtest.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

static ByteString
CreateCert(const char* issuerCN, 
           const char* subjectCN, 
           EndEntityOrCA endEntityOrCA,
            std::map<ByteString, ByteString>*
             subjectDERToCertDER = nullptr)
{
  static long serialNumberValue = 0;
  ++serialNumberValue;
  ByteString serialNumber(CreateEncodedSerialNumber(serialNumberValue));
  EXPECT_FALSE(ENCODING_FAILED(serialNumber));

  ByteString issuerDER(issuerCN ? CNToDERName(issuerCN) : Name(ByteString()));
  ByteString subjectDER(subjectCN ? CNToDERName(subjectCN) : Name(ByteString()));

  ByteString extensions[2];
  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    extensions[0] =
      CreateEncodedBasicConstraints(true, nullptr, Critical::Yes);
    EXPECT_FALSE(ENCODING_FAILED(extensions[0]));
  }

  ScopedTestKeyPair reusedKey(CloneReusedKeyPair());
  ByteString certDER(CreateEncodedCertificate(
                       v3, sha256WithRSAEncryption(), serialNumber, issuerDER,
                       oneDayBeforeNow, oneDayAfterNow, subjectDER,
                       *reusedKey, extensions, *reusedKey,
                       sha256WithRSAEncryption()));
  EXPECT_FALSE(ENCODING_FAILED(certDER));

  if (subjectDERToCertDER) {
    (*subjectDERToCertDER)[subjectDER] = certDER;
  }

  return certDER;
}

class TestTrustDomain final : public DefaultCryptoTrustDomain
{
public:
  
  
  
  bool SetUpCertChainTail()
  {
    static char const* const names[] = {
        "CA1 (Root)", "CA2", "CA3", "CA4", "CA5", "CA6", "CA7"
    };

    for (size_t i = 0; i < MOZILLA_PKIX_ARRAY_LENGTH(names); ++i) {
      const char* issuerName = i == 0 ? names[0] : names[i-1];
      CreateCACert(issuerName, names[i]);
      if (i == 0) {
        rootCACertDER = leafCACertDER;
      }
    }

    return true;
  }

  void CreateCACert(const char* issuerName, const char* subjectName)
  {
    leafCACertDER = CreateCert(issuerName, subjectName,
                               EndEntityOrCA::MustBeCA, &subjectDERToCertDER);
    assert(!ENCODING_FAILED(leafCACertDER));
  }

  ByteString GetLeafCACertDER() const { return leafCACertDER; }

private:
  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&, Input candidateCert,
                       TrustLevel& trustLevel) override
  {
    trustLevel = InputEqualsByteString(candidateCert, rootCACertDER)
               ? TrustLevel::TrustAnchor
               : TrustLevel::InheritsTrust;
    return Success;
  }

  Result FindIssuer(Input encodedIssuerName, IssuerChecker& checker, Time)
                    override
  {
    ByteString subjectDER(InputToByteString(encodedIssuerName));
    ByteString certDER(subjectDERToCertDER[subjectDER]);
    Input derCert;
    Result rv = derCert.Init(certDER.data(), certDER.length());
    if (rv != Success) {
      return rv;
    }
    bool keepGoing;
    rv = checker.Check(derCert, nullptr,
                       keepGoing);
    if (rv != Success) {
      return rv;
    }
    return Success;
  }

  Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                          const Input*,  const Input*)
                         override
  {
    return Success;
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    return Success;
  }

  std::map<ByteString, ByteString> subjectDERToCertDER;
  ByteString leafCACertDER;
  ByteString rootCACertDER;
};

class pkixbuild : public ::testing::Test
{
public:
  static void SetUpTestCase()
  {
    if (!trustDomain.SetUpCertChainTail()) {
      abort();
    }
  }

protected:

  static TestTrustDomain trustDomain;
};

 TestTrustDomain pkixbuild::trustDomain;

TEST_F(pkixbuild, MaxAcceptableCertChainLength)
{
  {
    ByteString leafCACert(trustDomain.GetLeafCACertDER());
    Input certDER;
    ASSERT_EQ(Success, certDER.Init(leafCACert.data(), leafCACert.length()));
    ASSERT_EQ(Success,
              BuildCertChain(trustDomain, certDER, Now(),
                             EndEntityOrCA::MustBeCA,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr));
  }

  {
    ByteString certDER(CreateCert("CA7", "Direct End-Entity",
                                  EndEntityOrCA::MustBeEndEntity));
    ASSERT_FALSE(ENCODING_FAILED(certDER));
    Input certDERInput;
    ASSERT_EQ(Success, certDERInput.Init(certDER.data(), certDER.length()));
    ASSERT_EQ(Success,
              BuildCertChain(trustDomain, certDERInput, Now(),
                             EndEntityOrCA::MustBeEndEntity,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr));
  }
}

TEST_F(pkixbuild, BeyondMaxAcceptableCertChainLength)
{
  static char const* const caCertName = "CA Too Far";

  trustDomain.CreateCACert("CA7", caCertName);

  {
    ByteString certDER(trustDomain.GetLeafCACertDER());
    Input certDERInput;
    ASSERT_EQ(Success, certDERInput.Init(certDER.data(), certDER.length()));
    ASSERT_EQ(Result::ERROR_UNKNOWN_ISSUER,
              BuildCertChain(trustDomain, certDERInput, Now(),
                             EndEntityOrCA::MustBeCA,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr));
  }

  {
    ByteString certDER(CreateCert(caCertName, "End-Entity Too Far",
                                  EndEntityOrCA::MustBeEndEntity));
    ASSERT_FALSE(ENCODING_FAILED(certDER));
    Input certDERInput;
    ASSERT_EQ(Success, certDERInput.Init(certDER.data(), certDER.length()));
    ASSERT_EQ(Result::ERROR_UNKNOWN_ISSUER,
              BuildCertChain(trustDomain, certDERInput, Now(),
                             EndEntityOrCA::MustBeEndEntity,
                             KeyUsage::noParticularKeyUsageRequired,
                             KeyPurposeId::id_kp_serverAuth,
                             CertPolicyId::anyPolicy,
                             nullptr));
  }
}






class ExpiredCertTrustDomain final : public DefaultCryptoTrustDomain
{
public:
  explicit ExpiredCertTrustDomain(ByteString rootDER)
    : rootDER(rootDER)
  {
  }

  
  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&, Input candidateCert,
                       TrustLevel& trustLevel) override
  {
    Input rootCert;
    Result rv = rootCert.Init(rootDER.data(), rootDER.length());
    if (rv != Success) {
      return rv;
    }
    if (InputsAreEqual(candidateCert, rootCert)) {
      trustLevel = TrustLevel::TrustAnchor;
    } else {
      trustLevel = TrustLevel::InheritsTrust;
    }
    return Success;
  }

  Result FindIssuer(Input, IssuerChecker& checker, Time) override
  {
    
    
    
    bool keepGoing;
    Input rootCert;
    Result rv = rootCert.Init(rootDER.data(), rootDER.length());
    if (rv != Success) {
      return rv;
    }
    return checker.Check(rootCert, nullptr, keepGoing);
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    return Success;
  }

private:
  ByteString rootDER;
};

TEST_F(pkixbuild, NoRevocationCheckingForExpiredCert)
{
  const char* rootCN = "Root CA";
  ByteString rootDER(CreateCert(rootCN, rootCN, EndEntityOrCA::MustBeCA,
                                nullptr));
  EXPECT_FALSE(ENCODING_FAILED(rootDER));
  ExpiredCertTrustDomain expiredCertTrustDomain(rootDER);

  ByteString serialNumber(CreateEncodedSerialNumber(100));
  EXPECT_FALSE(ENCODING_FAILED(serialNumber));
  ByteString issuerDER(CNToDERName(rootCN));
  ByteString subjectDER(CNToDERName("Expired End-Entity Cert"));
  ScopedTestKeyPair reusedKey(CloneReusedKeyPair());
  ByteString certDER(CreateEncodedCertificate(
                       v3, sha256WithRSAEncryption(),
                       serialNumber, issuerDER,
                       oneDayBeforeNow - ONE_DAY_IN_SECONDS_AS_TIME_T,
                       oneDayBeforeNow,
                       subjectDER, *reusedKey, nullptr, *reusedKey,
                       sha256WithRSAEncryption()));
  EXPECT_FALSE(ENCODING_FAILED(certDER));

  Input cert;
  ASSERT_EQ(Success, cert.Init(certDER.data(), certDER.length()));
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE,
            BuildCertChain(expiredCertTrustDomain, cert, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));
}

class DSSTrustDomain final : public EverythingFailsByDefaultTrustDomain
{
public:
  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                      Input,  TrustLevel& trustLevel) override
  {
    trustLevel = TrustLevel::TrustAnchor;
    return Success;
  }
};

class pkixbuild_DSS : public ::testing::Test { };

TEST_F(pkixbuild_DSS, DSSEndEntityKeyNotAccepted)
{
  DSSTrustDomain trustDomain;

  ByteString serialNumber(CreateEncodedSerialNumber(1));
  ASSERT_FALSE(ENCODING_FAILED(serialNumber));

  ByteString subjectDER(CNToDERName("DSS"));
  ASSERT_FALSE(ENCODING_FAILED(subjectDER));
  ScopedTestKeyPair subjectKey(GenerateDSSKeyPair());
  ASSERT_TRUE(subjectKey.get());

  ByteString issuerDER(CNToDERName("RSA"));
  ASSERT_FALSE(ENCODING_FAILED(issuerDER));
  ScopedTestKeyPair issuerKey(CloneReusedKeyPair());
  ASSERT_TRUE(issuerKey.get());

  ByteString cert(CreateEncodedCertificate(v3, sha256WithRSAEncryption(),
                                           serialNumber, issuerDER,
                                           oneDayBeforeNow, oneDayAfterNow,
                                           subjectDER, *subjectKey, nullptr,
                                           *issuerKey, sha256WithRSAEncryption()));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certDER;
  ASSERT_EQ(Success, certDER.Init(cert.data(), cert.length()));

  ASSERT_EQ(Result::ERROR_UNSUPPORTED_KEYALG,
            BuildCertChain(trustDomain, certDER, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));
}

class IssuerNameCheckTrustDomain final : public DefaultCryptoTrustDomain
{
public:
  IssuerNameCheckTrustDomain(const ByteString& issuer, bool expectedKeepGoing)
    : issuer(issuer)
    , expectedKeepGoing(expectedKeepGoing)
  {
  }

  Result GetCertTrust(EndEntityOrCA endEntityOrCA, const CertPolicyId&, Input,
                       TrustLevel& trustLevel) override
  {
    trustLevel = endEntityOrCA == EndEntityOrCA::MustBeCA
               ? TrustLevel::TrustAnchor
               : TrustLevel::InheritsTrust;
    return Success;
  }

  Result FindIssuer(Input, IssuerChecker& checker, Time) override
  {
    Input issuerInput;
    EXPECT_EQ(Success, issuerInput.Init(issuer.data(), issuer.length()));
    bool keepGoing;
    EXPECT_EQ(Success,
              checker.Check(issuerInput, nullptr ,
                            keepGoing));
    EXPECT_EQ(expectedKeepGoing, keepGoing);
    return Success;
  }

  Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                          const Input*,  const Input*)
                         override
  {
    return Success;
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    return Success;
  }

private:
  const ByteString issuer;
  const bool expectedKeepGoing;
};

struct IssuerNameCheckParams
{
  const char* subjectIssuerCN; 
  const char* issuerSubjectCN; 
  bool matches;
};

static const IssuerNameCheckParams ISSUER_NAME_CHECK_PARAMS[] =
{
  { "foo", "foo", true },
  { "foo", "bar", false },
  { "f", "foo", false }, 
  { "foo", "f", false }, 
  { "foo", "Foo", false }, 
  { "", "", true },
  { nullptr, nullptr, true }, 
};

class pkixbuild_IssuerNameCheck
  : public ::testing::Test
  , public ::testing::WithParamInterface<IssuerNameCheckParams>
{
};

TEST_P(pkixbuild_IssuerNameCheck, MatchingName)
{
  const IssuerNameCheckParams& params(GetParam());

  ByteString issuerCertDER(CreateCert(params.issuerSubjectCN,
                                      params.issuerSubjectCN,
                                      EndEntityOrCA::MustBeCA, nullptr));
  ASSERT_FALSE(ENCODING_FAILED(issuerCertDER));

  ByteString subjectCertDER(CreateCert(params.subjectIssuerCN, "end-entity",
                                       EndEntityOrCA::MustBeEndEntity,
                                       nullptr));
  ASSERT_FALSE(ENCODING_FAILED(subjectCertDER));

  Input subjectCertDERInput;
  ASSERT_EQ(Success, subjectCertDERInput.Init(subjectCertDER.data(),
                                              subjectCertDER.length()));

  IssuerNameCheckTrustDomain trustDomain(issuerCertDER, !params.matches);
  ASSERT_EQ(params.matches ? Success : Result::ERROR_UNKNOWN_ISSUER,
            BuildCertChain(trustDomain, subjectCertDERInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));
}

INSTANTIATE_TEST_CASE_P(pkixbuild_IssuerNameCheck, pkixbuild_IssuerNameCheck,
                        testing::ValuesIn(ISSUER_NAME_CHECK_PARAMS));
