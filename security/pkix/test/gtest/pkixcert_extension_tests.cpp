























#include "pkix/pkix.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;


static ByteString
CreateCert(const char* subjectCN,
           const ByteString* extensions, 
            ScopedTestKeyPair& subjectKey)
{
  static long serialNumberValue = 0;
  ++serialNumberValue;
  ByteString serialNumber(CreateEncodedSerialNumber(serialNumberValue));
  EXPECT_NE(ENCODING_FAILED, serialNumber);
  ByteString issuerDER(CNToDERName(subjectCN));
  EXPECT_NE(ENCODING_FAILED, issuerDER);
  ByteString subjectDER(CNToDERName(subjectCN));
  EXPECT_NE(ENCODING_FAILED, subjectDER);
  return CreateEncodedCertificate(v3, sha256WithRSAEncryption,
                                  serialNumber, issuerDER,
                                  oneDayBeforeNow, oneDayAfterNow,
                                  subjectDER, extensions,
                                  nullptr,
                                  SignatureAlgorithm::rsa_pkcs1_with_sha256,
                                  subjectKey);
}


static ByteString
CreateCert(const char* subjectStr,
           const ByteString& extension,
            ScopedTestKeyPair& subjectKey)
{
  const ByteString extensions[] = { extension, ByteString() };
  return CreateCert(subjectStr, extensions, subjectKey);
}

class TrustEverythingTrustDomain : public TrustDomain
{
private:
  virtual Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                              Input candidateCert,
                               TrustLevel& trustLevel)
  {
    trustLevel = TrustLevel::TrustAnchor;
    return Success;
  }

  virtual Result FindIssuer(Input ,
                            IssuerChecker& , Time )
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                                  const Input*,
                                  const Input*)
  {
    return Success;
  }

  virtual Result IsChainValid(const DERArray&)
  {
    return Success;
  }

  virtual Result VerifySignedData(const SignedDataWithSignature& signedData,
                                  Input subjectPublicKeyInfo)
  {
    return TestVerifySignedData(signedData, subjectPublicKeyInfo);
  }

  virtual Result DigestBuf(Input,  uint8_t*, size_t)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckPublicKey(Input subjectPublicKeyInfo)
  {
    return TestCheckPublicKey(subjectPublicKeyInfo);
  }
};

class pkixcert_extension : public ::testing::Test
{
protected:
  static TrustEverythingTrustDomain trustDomain;
};

 TrustEverythingTrustDomain pkixcert_extension::trustDomain;




TEST_F(pkixcert_extension, UnknownCriticalExtension)
{
  static const uint8_t unknownCriticalExtensionBytes[] = {
    0x30, 0x19, 
      0x06, 0x12, 
        
        0x2b, 0x06, 0x01, 0x04, 0x01, 0xeb, 0x49, 0x85, 0x1a,
        0x85, 0x1a, 0x85, 0x1a, 0x01, 0x83, 0x74, 0x09, 0x03,
      0x01, 0x01, 0xff, 
      0x04, 0x00 
  };
  static const ByteString
    unknownCriticalExtension(unknownCriticalExtensionBytes,
                             sizeof(unknownCriticalExtensionBytes));
  const char* certCN = "Cert With Unknown Critical Extension";
  ScopedTestKeyPair key;
  ByteString cert(CreateCert(certCN, unknownCriticalExtension, key));
  ASSERT_NE(ENCODING_FAILED, cert);
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Result::ERROR_UNKNOWN_CRITICAL_EXTENSION,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}



TEST_F(pkixcert_extension, UnknownNonCriticalExtension)
{
  static const uint8_t unknownNonCriticalExtensionBytes[] = {
    0x30, 0x16, 
      0x06, 0x12, 
        
        0x2b, 0x06, 0x01, 0x04, 0x01, 0xeb, 0x49, 0x85, 0x1a,
        0x85, 0x1a, 0x85, 0x1a, 0x01, 0x83, 0x74, 0x09, 0x03,
      0x04, 0x00 
  };
  static const ByteString
    unknownNonCriticalExtension(unknownNonCriticalExtensionBytes,
                                sizeof(unknownNonCriticalExtensionBytes));
  const char* certCN = "Cert With Unknown NonCritical Extension";
  ScopedTestKeyPair key;
  ByteString cert(CreateCert(certCN, unknownNonCriticalExtension, key));
  ASSERT_NE(ENCODING_FAILED, cert);
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Success,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}




TEST_F(pkixcert_extension, WrongOIDCriticalExtension)
{
  static const uint8_t wrongOIDCriticalExtensionBytes[] = {
    0x30, 0x10, 
      0x06, 0x09, 
        
        0x2b, 0x06, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01,
      0x01, 0x01, 0xff, 
      0x04, 0x00 
  };
  static const ByteString
    wrongOIDCriticalExtension(wrongOIDCriticalExtensionBytes,
                              sizeof(wrongOIDCriticalExtensionBytes));
  const char* certCN = "Cert With Critical Wrong OID Extension";
  ScopedTestKeyPair key;
  ByteString cert(CreateCert(certCN, wrongOIDCriticalExtension, key));
  ASSERT_NE(ENCODING_FAILED, cert);
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Result::ERROR_UNKNOWN_CRITICAL_EXTENSION,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}



TEST_F(pkixcert_extension, CriticalAIAExtension)
{
  
  
  static const uint8_t criticalAIAExtensionBytes[] = {
    0x30, 0x11, 
      0x06, 0x08, 
        
        0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01,
      0x01, 0x01, 0xff, 
      0x04, 0x02, 
        0x30, 0x00, 
  };
  static const ByteString
    criticalAIAExtension(criticalAIAExtensionBytes,
                         sizeof(criticalAIAExtensionBytes));
  const char* certCN = "Cert With Critical AIA Extension";
  ScopedTestKeyPair key;
  ByteString cert(CreateCert(certCN, criticalAIAExtension, key));
  ASSERT_NE(ENCODING_FAILED, cert);
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Success,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}




TEST_F(pkixcert_extension, UnknownCriticalCEExtension)
{
  static const uint8_t unknownCriticalCEExtensionBytes[] = {
    0x30, 0x0a, 
      0x06, 0x03, 
        0x55, 0x1d, 0x37, 
      0x01, 0x01, 0xff, 
      0x04, 0x00 
  };
  static const ByteString
    unknownCriticalCEExtension(unknownCriticalCEExtensionBytes,
                               sizeof(unknownCriticalCEExtensionBytes));
  const char* certCN = "Cert With Unknown Critical id-ce Extension";
  ScopedTestKeyPair key;
  ByteString cert(CreateCert(certCN, unknownCriticalCEExtension, key));
  ASSERT_NE(ENCODING_FAILED, cert);
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Result::ERROR_UNKNOWN_CRITICAL_EXTENSION,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}



TEST_F(pkixcert_extension, KnownCriticalCEExtension)
{
  static const uint8_t criticalCEExtensionBytes[] = {
    0x30, 0x0d, 
      0x06, 0x03, 
        0x55, 0x1d, 0x36, 
      0x01, 0x01, 0xff, 
      0x04, 0x03, 
        0x02, 0x01, 0x00, 
  };
  static const ByteString
    criticalCEExtension(criticalCEExtensionBytes,
                        sizeof(criticalCEExtensionBytes));
  const char* certCN = "Cert With Known Critical id-ce Extension";
  ScopedTestKeyPair key;
  ByteString cert(CreateCert(certCN, criticalCEExtension, key));
  ASSERT_NE(ENCODING_FAILED, cert);
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Success,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}


TEST_F(pkixcert_extension, DuplicateSubjectAltName)
{
  static const uint8_t DER_BYTES[] = {
    0x30, 22, 
      0x06, 3, 
        0x55, 0x1d, 0x11, 
      0x04, 15, 
        0x30, 13, 
          0x82, 11, 
            'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'
  };
  static const ByteString DER(DER_BYTES, sizeof(DER_BYTES));
  static const ByteString extensions[] = { DER, DER, ByteString() };
  static const char* certCN = "Cert With Duplicate subjectAltName";
  ScopedTestKeyPair key;
  ByteString cert(CreateCert(certCN, extensions, key));
  ASSERT_NE(ENCODING_FAILED, cert);
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Result::ERROR_EXTENSION_VALUE_INVALID,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}
