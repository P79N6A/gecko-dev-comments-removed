























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"
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

  SECItem* certDER(CreateEncodedCertificate(
                     arena, v3, SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION,
                     serialNumber, issuerDER,
                     PR_Now() - ONE_DAY, PR_Now() + ONE_DAY,
                     subjectDER, extensions, issuerKey, SEC_OID_SHA256,
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
  virtual Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                              const SECItem& candidateCert,
                               TrustLevel* trustLevel)
  {
    if (SECITEM_ItemsAreEqual(&candidateCert, &certChainTail[0]->derCert)) {
      *trustLevel = TrustLevel::TrustAnchor;
    } else {
      *trustLevel = TrustLevel::InheritsTrust;
    }
    return Success;
  }

  virtual Result FindIssuer(const SECItem& encodedIssuerName,
                            IssuerChecker& checker, PRTime time)
  {
    ScopedCERTCertList
      candidates(CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                            &encodedIssuerName, time, true));
    if (candidates) {
      for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
           !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
        bool keepGoing;
        Result rv = checker.Check(n->cert->derCert,
                                  nullptr,
                                  keepGoing);
        if (rv != Success) {
          return rv;
        }
        if (!keepGoing) {
          break;
        }
      }
    }

    return Success;
  }

  virtual Result CheckRevocation(EndEntityOrCA, const CertID&, PRTime,
                                  const SECItem*,
                                  const SECItem*)
  {
    return Success;
  }

  virtual Result IsChainValid(const DERArray&)
  {
    return Success;
  }

  virtual Result VerifySignedData(const SignedDataWithSignature& signedData,
                                  const SECItem& subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                             nullptr);
  }

  virtual Result DigestBuf(const SECItem& item,  uint8_t* digestBuf,
                           size_t digestBufLen)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckPublicKey(const SECItem& subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::CheckPublicKey(subjectPublicKeyInfo);
  }

  
  
  ScopedCERTCertificate certChainTail[7];

public:
  ScopedSECKEYPrivateKey leafCAKey;
  CERTCertificate* GetLeafCACert() const
  {
    return certChainTail[PR_ARRAY_SIZE(certChainTail) - 1].get();
  }
};

class pkixbuild : public NSSTest
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

 TestTrustDomain pkixbuild::trustDomain;

TEST_F(pkixbuild, MaxAcceptableCertChainLength)
{
  ASSERT_EQ(Success,
            BuildCertChain(trustDomain, trustDomain.GetLeafCACert()->derCert,
                           now, EndEntityOrCA::MustBeCA,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));

  ScopedSECKEYPrivateKey privateKey;
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(),
                         trustDomain.GetLeafCACert()->subjectName,
                         "CN=Direct End-Entity",
                         EndEntityOrCA::MustBeEndEntity,
                         trustDomain.leafCAKey.get(), privateKey, cert));
  ASSERT_EQ(Success,
            BuildCertChain(trustDomain, cert->derCert, now,
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));
}

TEST_F(pkixbuild, BeyondMaxAcceptableCertChainLength)
{
  ScopedSECKEYPrivateKey caPrivateKey;
  ScopedCERTCertificate caCert;
  ASSERT_TRUE(CreateCert(arena.get(),
                         trustDomain.GetLeafCACert()->subjectName,
                         "CN=CA Too Far", EndEntityOrCA::MustBeCA,
                         trustDomain.leafCAKey.get(),
                         caPrivateKey, caCert));
  ASSERT_EQ(Result::ERROR_UNKNOWN_ISSUER,
            BuildCertChain(trustDomain, caCert->derCert, now,
                           EndEntityOrCA::MustBeCA,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));

  ScopedSECKEYPrivateKey privateKey;
  ScopedCERTCertificate cert;
  ASSERT_TRUE(CreateCert(arena.get(), caCert->subjectName,
                         "CN=End-Entity Too Far",
                         EndEntityOrCA::MustBeEndEntity,
                         caPrivateKey.get(), privateKey, cert));
  ASSERT_EQ(Result::ERROR_UNKNOWN_ISSUER,
            BuildCertChain(trustDomain, cert->derCert, now,
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::id_kp_serverAuth,
                           CertPolicyId::anyPolicy,
                           nullptr));
}
