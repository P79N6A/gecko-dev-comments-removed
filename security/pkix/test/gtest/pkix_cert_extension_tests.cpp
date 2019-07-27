























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;


static bool
CreateCert(PLArenaPool* arena, const char* subjectStr,
           const SECItem* extension,
            ScopedSECKEYPrivateKey& subjectKey,
            ScopedCERTCertificate& subjectCert)
{
  static long serialNumberValue = 0;
  ++serialNumberValue;
  const SECItem* serialNumber(CreateEncodedSerialNumber(arena,
                                                        serialNumberValue));
  if (!serialNumber) {
    return false;
  }
  const SECItem* issuerDER(ASCIIToDERName(arena, subjectStr));
  if (!issuerDER) {
    return false;
  }
  const SECItem* subjectDER(ASCIIToDERName(arena, subjectStr));
  if (!subjectDER) {
    return false;
  }

  const SECItem* extensions[2] = { extension, nullptr };

  SECItem* certDER(CreateEncodedCertificate(arena, v3, SEC_OID_SHA256,
                                            serialNumber, issuerDER,
                                            PR_Now() - ONE_DAY,
                                            PR_Now() + ONE_DAY,
                                            subjectDER, extensions,
                                            nullptr, SEC_OID_SHA256,
                                            subjectKey));
  if (!certDER) {
    return false;
  }
  subjectCert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), certDER,
                                        nullptr, false, true);
  return subjectCert.get() != nullptr;
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

  SECStatus FindPotentialIssuers(const SECItem* encodedIssuerName,
                                 PRTime time,
                                  ScopedCERTCertList& results)
  {
    return SECSuccess;
  }

  SECStatus VerifySignedData(const CERTSignedData* signedData,
                             const SECItem& subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                             nullptr);
  }

  SECStatus CheckRevocation(EndEntityOrCA, const CertID&, PRTime,
                             const SECItem*,
                             const SECItem*)
  {
    return SECSuccess;
  }

  virtual SECStatus IsChainValid(const CERTCertList*)
  {
    return SECSuccess;
  }
};

class pkix_cert_extensions: public NSSTest
{
public:
  static void SetUpTestCase()
  {
    NSSTest::SetUpTestCase();
  }

protected:
  static TrustEverythingTrustDomain trustDomain;
};

 TrustEverythingTrustDomain pkix_cert_extensions::trustDomain;




TEST_F(pkix_cert_extensions, UnknownCriticalExtension)
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
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), certCN, &unknownCriticalExtension, key,
                         cert));
  ScopedCERTCertList results;
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION,
                    BuildCertChain(trustDomain, cert.get(),
                                   now, EndEntityOrCA::MustBeEndEntity,
                                   KeyUsage::noParticularKeyUsageRequired,
                                   KeyPurposeId::anyExtendedKeyUsage,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}



TEST_F(pkix_cert_extensions, UnknownNonCriticalExtension)
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
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), certCN, &unknownNonCriticalExtension, key,
                         cert));
  ScopedCERTCertList results;
  ASSERT_SECSuccess(BuildCertChain(trustDomain, cert.get(),
                                   now, EndEntityOrCA::MustBeEndEntity,
                                   KeyUsage::noParticularKeyUsageRequired,
                                   KeyPurposeId::anyExtendedKeyUsage,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}




TEST_F(pkix_cert_extensions, WrongOIDCriticalExtension)
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
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), certCN, &wrongOIDCriticalExtension, key,
                         cert));
  ScopedCERTCertList results;
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION,
                    BuildCertChain(trustDomain, cert.get(),
                                   now, EndEntityOrCA::MustBeEndEntity,
                                   KeyUsage::noParticularKeyUsageRequired,
                                   KeyPurposeId::anyExtendedKeyUsage,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}



TEST_F(pkix_cert_extensions, CriticalAIAExtension)
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
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), certCN, &criticalAIAExtension, key,
                         cert));
  ScopedCERTCertList results;
  ASSERT_SECSuccess(BuildCertChain(trustDomain, cert.get(),
                                   now, EndEntityOrCA::MustBeEndEntity,
                                   KeyUsage::noParticularKeyUsageRequired,
                                   KeyPurposeId::anyExtendedKeyUsage,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}




TEST_F(pkix_cert_extensions, UnknownCriticalCEExtension)
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
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), certCN, &unknownCriticalCEExtension, key,
                         cert));
  ScopedCERTCertList results;
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION,
                    BuildCertChain(trustDomain, cert.get(),
                                   now, EndEntityOrCA::MustBeEndEntity,
                                   KeyUsage::noParticularKeyUsageRequired,
                                   KeyPurposeId::anyExtendedKeyUsage,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}



TEST_F(pkix_cert_extensions, KnownCriticalCEExtension)
{
  static const uint8_t criticalCEExtensionBytes[] = {
    0x30, 0x0a, 
      0x06, 0x03, 
        0x55, 0x1d, 0x36, 
      0x01, 0x01, 0xff, 
      0x04, 0x00 
  };
  static const SECItem criticalCEExtension = {
    siBuffer,
    const_cast<unsigned char*>(criticalCEExtensionBytes),
    sizeof(criticalCEExtensionBytes)
  };
  const char* certCN = "CN=Cert With Known Critical id-ce Extension";
  ScopedSECKEYPrivateKey key;
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), certCN, &criticalCEExtension, key, cert));
  ScopedCERTCertList results;
  ASSERT_SECSuccess(BuildCertChain(trustDomain, cert.get(),
                                   now, EndEntityOrCA::MustBeEndEntity,
                                   KeyUsage::noParticularKeyUsageRequired,
                                   KeyPurposeId::anyExtendedKeyUsage,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}
