























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "prinit.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

static bool
CreateCert(PLArenaPool* arena, const char* issuerStr,
           const char* subjectStr, EndEntityOrCA endEntityOrCA,
            SECKEYPrivateKey* issuerKey,
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
  const SECItem* issuerDER(ASCIIToDERName(arena, issuerStr));
  if (!issuerDER) {
    return false;
  }
  const SECItem* subjectDER(ASCIIToDERName(arena, subjectStr));
  if (!subjectDER) {
    return false;
  }

  const SECItem* extensions[2] = { nullptr, nullptr };
  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
    extensions[0] =
      CreateEncodedBasicConstraints(arena, true, nullptr,
                                    ExtensionCriticality::Critical);
    if (!extensions[0]) {
      return false;
    }
  }

  SECItem* certDER(CreateEncodedCertificate(arena, v3, SEC_OID_SHA256,
                                            serialNumber, issuerDER,
                                            PR_Now() - ONE_DAY,
                                            PR_Now() + ONE_DAY,
                                            subjectDER, extensions,
                                            issuerKey, SEC_OID_SHA256,
                                            subjectKey));
  if (!certDER) {
    return false;
  }
  subjectCert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), certDER,
                                        nullptr, false, true);
  return subjectCert.get() != nullptr;
}

class TestTrustDomain : public TrustDomain
{
public:
  
  
  
  bool SetUpCertChainTail()
  {
    static char const* const names[] = {
        "CN=CA1 (Root)", "CN=CA2", "CN=CA3", "CN=CA4", "CN=CA5", "CN=CA6",
        "CN=CA7"
    };

    static_assert(PR_ARRAY_SIZE(names) == PR_ARRAY_SIZE(certChainTail),
                  "mismatch in sizes of names and certChainTail arrays");

    ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
    if (!arena) {
      return false;
    }

    for (size_t i = 0; i < PR_ARRAY_SIZE(names); ++i) {
      const char* issuerName = i == 0 ? names[0]
                                      : certChainTail[i - 1]->subjectName;
      if (!CreateCert(arena.get(), issuerName, names[i],
                      EndEntityOrCA::MustBeCA, leafCAKey.get(), leafCAKey,
                      certChainTail[i])) {
        return false;
      }
    }

    return true;
  }

private:
  SECStatus GetCertTrust(EndEntityOrCA,
                         const CertPolicyId&,
                         const CERTCertificate* candidateCert,
                          TrustLevel* trustLevel)
  {
    if (candidateCert == certChainTail[0].get()) {
      *trustLevel = TrustLevel::TrustAnchor;
    } else {
      *trustLevel = TrustLevel::InheritsTrust;
    }
    return SECSuccess;
  }

  SECStatus FindPotentialIssuers(const SECItem* encodedIssuerName,
                                 PRTime time,
                                  ScopedCERTCertList& results)
  {
    results = CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                         encodedIssuerName, time, true);
    return SECSuccess;
  }

  SECStatus VerifySignedData(const CERTSignedData* signedData,
                             const CERTCertificate* cert)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, cert, nullptr);
  }

  SECStatus CheckRevocation(EndEntityOrCA, const CERTCertificate*,
                             CERTCertificate*, PRTime,
                             const SECItem*)
  {
    return SECSuccess;
  }

  virtual SECStatus IsChainValid(const CERTCertList*)
  {
    return SECSuccess;
  }

  
  
  ScopedCERTCertificate certChainTail[7];

public:
  ScopedSECKEYPrivateKey leafCAKey;
  CERTCertificate* GetLeafeCACert() const
  {
    return certChainTail[PR_ARRAY_SIZE(certChainTail) - 1].get();
  }
};

class pkix_cert_chain_length : public NSSTest
{
public:
  static void SetUpTestCase()
  {
    NSSTest::SetUpTestCase();
    
    
    if (!trustDomain.SetUpCertChainTail()) {
      PR_Abort();
    }
  }

protected:
  static TestTrustDomain trustDomain;
};

 TestTrustDomain pkix_cert_chain_length::trustDomain;

TEST_F(pkix_cert_chain_length, MaxAcceptableCertChainLength)
{
  ScopedCERTCertList results;
  ASSERT_SECSuccess(BuildCertChain(trustDomain, trustDomain.GetLeafeCACert(),
                                   now, EndEntityOrCA::MustBeCA,
                                   0, 
                                   KeyPurposeId::id_kp_serverAuth,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));

  ScopedSECKEYPrivateKey privateKey;
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(),
                         trustDomain.GetLeafeCACert()->subjectName,
                         "CN=Direct End-Entity",
                         EndEntityOrCA::MustBeEndEntity,
                         trustDomain.leafCAKey.get(), privateKey, cert));
  ASSERT_SECSuccess(BuildCertChain(trustDomain, cert.get(), now,
                                   EndEntityOrCA::MustBeEndEntity,
                                   0, 
                                   KeyPurposeId::id_kp_serverAuth,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}

TEST_F(pkix_cert_chain_length, BeyondMaxAcceptableCertChainLength)
{
  ScopedCERTCertList results;

  ScopedSECKEYPrivateKey caPrivateKey;
  ScopedCERTCertificate caCert;
  ASSERT_TRUE(CreateCert(arena.get(),
                         trustDomain.GetLeafeCACert()->subjectName,
                         "CN=CA Too Far", EndEntityOrCA::MustBeCA,
                         trustDomain.leafCAKey.get(),
                         caPrivateKey, caCert));
  PR_SetError(0, 0);
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_ISSUER,
                    BuildCertChain(trustDomain, caCert.get(), now,
                                   EndEntityOrCA::MustBeCA,
                                   0, 
                                   KeyPurposeId::id_kp_serverAuth,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));

  ScopedSECKEYPrivateKey privateKey;
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), caCert->subjectName,
                         "CN=End-Entity Too Far",
                         EndEntityOrCA::MustBeEndEntity,
                         caPrivateKey.get(), privateKey, cert));
  PR_SetError(0, 0);
  ASSERT_SECFailure(SEC_ERROR_UNKNOWN_ISSUER,
                    BuildCertChain(trustDomain, cert.get(), now,
                                   EndEntityOrCA::MustBeEndEntity,
                                   0, 
                                   KeyPurposeId::id_kp_serverAuth,
                                   CertPolicyId::anyPolicy,
                                   nullptr, 
                                   results));
}
