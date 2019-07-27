























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;


static const SECItem*
CreateCert(PLArenaPool* arena, const char* subjectStr,
           SECItem const* const* extensions, 
            ScopedSECKEYPrivateKey& subjectKey)
{
  static long serialNumberValue = 0;
  ++serialNumberValue;
  const SECItem* serialNumber(CreateEncodedSerialNumber(arena,
                                                        serialNumberValue));
  if (!serialNumber) {
    return nullptr;
  }
  const SECItem* issuerDER(ASCIIToDERName(arena, subjectStr));
  if (!issuerDER) {
    return nullptr;
  }
  const SECItem* subjectDER(ASCIIToDERName(arena, subjectStr));
  if (!subjectDER) {
    return nullptr;
  }

  return CreateEncodedCertificate(arena, v3,
                                  SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION,
                                  serialNumber, issuerDER,
                                  PR_Now() - ONE_DAY,
                                  PR_Now() + ONE_DAY,
                                  subjectDER, extensions,
                                  nullptr, SEC_OID_SHA256, subjectKey);
}


static const SECItem*
CreateCert(PLArenaPool* arena, const char* subjectStr,
           const SECItem* extension,
            ScopedSECKEYPrivateKey& subjectKey)
{
  const SECItem * extensions[] = { extension, nullptr };
  return CreateCert(arena, subjectStr, extensions, subjectKey);
}

class TrustEverythingTrustDomain : public TrustDomain
{
private:
  SECStatus GetCertTrust(EndEntityOrCA,
                         const CertPolicyId&,
                         const SECItem& candidateCert,
                          TrustLevel* trustLevel)
  {
    *trustLevel = TrustLevel::TrustAnchor;
    return SECSuccess;
  }

  SECStatus FindIssuer(const SECItem& ,
                       IssuerChecker& , PRTime )
  {
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  SECStatus CheckRevocation(EndEntityOrCA, const CertID&, PRTime,
                             const SECItem*,
                             const SECItem*)
  {
    return SECSuccess;
  }

  virtual SECStatus IsChainValid(const DERArray&)
  {
    return SECSuccess;
  }

  SECStatus VerifySignedData(const SignedDataWithSignature& signedData,
                             const SECItem& subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                             nullptr);
  }

  SECStatus DigestBuf(const SECItem&,  uint8_t *, size_t)
  {
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }
};

class pkixcert_extension: public NSSTest
{
public:
  static void SetUpTestCase()
  {
    NSSTest::SetUpTestCase();
  }

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
  static const SECItem unknownCriticalExtension = {
    siBuffer,
    const_cast<unsigned char*>(unknownCriticalExtensionBytes),
    sizeof(unknownCriticalExtensionBytes)
  };
  const char* certCN = "CN=Cert With Unknown Critical Extension";
  ScopedSECKEYPrivateKey key;
  
  const SECItem* cert(CreateCert(arena.get(), certCN,
                                 &unknownCriticalExtension, key));
  ASSERT_TRUE(cert);
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION,
                    BuildCertChain(trustDomain, *cert, now,
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
  static const SECItem unknownNonCriticalExtension = {
    siBuffer,
    const_cast<unsigned char*>(unknownNonCriticalExtensionBytes),
    sizeof(unknownNonCriticalExtensionBytes)
  };
  const char* certCN = "CN=Cert With Unknown NonCritical Extension";
  ScopedSECKEYPrivateKey key;
  
  const SECItem* cert(CreateCert(arena.get(), certCN,
                                 &unknownNonCriticalExtension, key));
  ASSERT_TRUE(cert);
  ASSERT_SECSuccess(BuildCertChain(trustDomain, *cert, now,
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
  static const SECItem wrongOIDCriticalExtension = {
    siBuffer,
    const_cast<unsigned char*>(wrongOIDCriticalExtensionBytes),
    sizeof(wrongOIDCriticalExtensionBytes)
  };
  const char* certCN = "CN=Cert With Critical Wrong OID Extension";
  ScopedSECKEYPrivateKey key;
  
  const SECItem* cert(CreateCert(arena.get(), certCN,
                                 &wrongOIDCriticalExtension, key));
  ASSERT_TRUE(cert);
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION,
                    BuildCertChain(trustDomain, *cert, now,
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
  static const SECItem criticalAIAExtension = {
    siBuffer,
    const_cast<unsigned char*>(criticalAIAExtensionBytes),
    sizeof(criticalAIAExtensionBytes)
  };
  const char* certCN = "CN=Cert With Critical AIA Extension";
  ScopedSECKEYPrivateKey key;
  
  const SECItem* cert(CreateCert(arena.get(), certCN, &criticalAIAExtension,
                                 key));
  ASSERT_TRUE(cert);
  ASSERT_SECSuccess(BuildCertChain(trustDomain, *cert, now,
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
  static const SECItem unknownCriticalCEExtension = {
    siBuffer,
    const_cast<unsigned char*>(unknownCriticalCEExtensionBytes),
    sizeof(unknownCriticalCEExtensionBytes)
  };
  const char* certCN = "CN=Cert With Unknown Critical id-ce Extension";
  ScopedSECKEYPrivateKey key;
  
  const SECItem* cert(CreateCert(arena.get(), certCN,
                                 &unknownCriticalCEExtension, key));
  ASSERT_TRUE(cert);
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION,
                    BuildCertChain(trustDomain, *cert, now,
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
  static const SECItem criticalCEExtension = {
    siBuffer,
    const_cast<unsigned char*>(criticalCEExtensionBytes),
    sizeof(criticalCEExtensionBytes)
  };
  const char* certCN = "CN=Cert With Known Critical id-ce Extension";
  ScopedSECKEYPrivateKey key;
  
  const SECItem* cert(CreateCert(arena.get(), certCN, &criticalCEExtension,
                                 key));
  ASSERT_TRUE(cert);
  ASSERT_SECSuccess(BuildCertChain(trustDomain, *cert,
                                   now, EndEntityOrCA::MustBeEndEntity,
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
  static const SECItem DER = {
    siBuffer,
    const_cast<unsigned char*>(DER_BYTES),
    sizeof(DER_BYTES)
  };
  static SECItem const* const extensions[] = { &DER, &DER, nullptr };
  static const char* certCN = "CN=Cert With Duplicate subjectAltName";
  ScopedSECKEYPrivateKey key;
  
  const SECItem* cert(CreateCert(arena.get(), certCN, extensions, key));
  ASSERT_TRUE(cert);
  ASSERT_SECFailure(SEC_ERROR_EXTENSION_VALUE_INVALID,
                    BuildCertChain(trustDomain, *cert,
                                   now, EndEntityOrCA::MustBeEndEntity,
                                   KeyUsage::noParticularKeyUsageRequired,
                                   KeyPurposeId::anyExtendedKeyUsage,
                                   CertPolicyId::anyPolicy,
                                   nullptr));
}
