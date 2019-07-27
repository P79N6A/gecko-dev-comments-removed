























#include "nss.h"
#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"
#include "prinit.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

const uint16_t END_ENTITY_MAX_LIFETIME_IN_DAYS = 10;

class OCSPTestTrustDomain : public TrustDomain
{
public:
  OCSPTestTrustDomain()
  {
  }

  Result GetCertTrust(EndEntityOrCA endEntityOrCA, const CertPolicyId&,
                      Input,  TrustLevel& trustLevel)
  {
    EXPECT_EQ(endEntityOrCA, EndEntityOrCA::MustBeEndEntity);
    trustLevel = TrustLevel::InheritsTrust;
    return Success;
  }

  Result FindIssuer(Input, IssuerChecker&, Time)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckRevocation(EndEntityOrCA endEntityOrCA, const CertID&,
                                 Time time,  const Input*,
                                  const Input*)
  {
    
    
    
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result IsChainValid(const DERArray&)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result VerifySignedData(const SignedDataWithSignature& signedData,
                                  Input subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                             nullptr);
  }

  virtual Result DigestBuf(Input item,  uint8_t *digestBuf,
                           size_t digestBufLen)
  {
    return ::mozilla::pkix::DigestBuf(item, digestBuf, digestBufLen);
  }

  virtual Result CheckPublicKey(Input subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::CheckPublicKey(subjectPublicKeyInfo);
  }

private:
  OCSPTestTrustDomain(const OCSPTestTrustDomain&) ;
  void operator=(const OCSPTestTrustDomain&) ;
};

namespace {
char const* const rootName = "CN=Test CA 1";
void deleteCertID(CertID* certID) { delete certID; }
} 

class pkixocsp_VerifyEncodedResponse : public NSSTest
{
public:
  static bool SetUpTestCaseInner()
  {
    ScopedSECKEYPublicKey rootPublicKey;
    if (GenerateKeyPair(rootPublicKey, rootPrivateKey) != SECSuccess) {
      return false;
    }
    rootSPKI = SECKEY_EncodeDERSubjectPublicKeyInfo(rootPublicKey.get());
    if (!rootSPKI) {
      return false;
    }

    return true;
  }

  static void SetUpTestCase()
  {
    NSSTest::SetUpTestCase();
    if (!SetUpTestCaseInner()) {
      PR_Abort();
    }
  }

  void SetUp()
  {
    NSSTest::SetUp();

    Input rootNameDER;
    
    if (InitInputFromSECItem(ASCIIToDERName(arena.get(), rootName),
                             rootNameDER) != Success) {
      PR_Abort();
    }

    Input serialNumberDER;
    
    if (InitInputFromSECItem(
          CreateEncodedSerialNumber(arena.get(), ++rootIssuedCount),
          serialNumberDER) != Success) {
      PR_Abort();
    }

    Input rootSPKIDER;
    if (InitInputFromSECItem(rootSPKI.get(), rootSPKIDER) != Success) {
      PR_Abort();
    }
    endEntityCertID = new (std::nothrow) CertID(rootNameDER, rootSPKIDER,
                                                serialNumberDER);
    if (!endEntityCertID) {
      PR_Abort();
    }
  }

  static ScopedSECKEYPrivateKey rootPrivateKey;
  static ScopedSECItem rootSPKI;
  static long rootIssuedCount;

  OCSPTestTrustDomain trustDomain;
  
  ScopedPtr<CertID, deleteCertID> endEntityCertID;
};

 ScopedSECKEYPrivateKey
              pkixocsp_VerifyEncodedResponse::rootPrivateKey;
 ScopedSECItem pkixocsp_VerifyEncodedResponse::rootSPKI;
 long pkixocsp_VerifyEncodedResponse::rootIssuedCount = 0;




struct WithoutResponseBytes
{
  uint8_t responseStatus;
  Result expectedError;
};

static const WithoutResponseBytes WITHOUT_RESPONSEBYTES[] = {
  { OCSPResponseContext::successful, Result::ERROR_OCSP_MALFORMED_RESPONSE },
  { OCSPResponseContext::malformedRequest, Result::ERROR_OCSP_MALFORMED_REQUEST },
  { OCSPResponseContext::internalError, Result::ERROR_OCSP_SERVER_ERROR },
  { OCSPResponseContext::tryLater, Result::ERROR_OCSP_TRY_SERVER_LATER },
  { 4, Result::ERROR_OCSP_UNKNOWN_RESPONSE_STATUS },
  { OCSPResponseContext::sigRequired, Result::ERROR_OCSP_REQUEST_NEEDS_SIG },
  { OCSPResponseContext::unauthorized, Result::ERROR_OCSP_UNAUTHORIZED_REQUEST },
  { OCSPResponseContext::unauthorized + 1,
    Result::ERROR_OCSP_UNKNOWN_RESPONSE_STATUS
  },
};

class pkixocsp_VerifyEncodedResponse_WithoutResponseBytes
  : public pkixocsp_VerifyEncodedResponse
  , public ::testing::WithParamInterface<WithoutResponseBytes>
{
protected:
  
  Input CreateEncodedOCSPErrorResponse(uint8_t status)
  {
    static const Input EMPTY;
    OCSPResponseContext context(arena.get(),
                                CertID(EMPTY, EMPTY, EMPTY),
                                pr_oneDayBeforeNow);
    context.responseStatus = status;
    context.skipResponseBytes = true;
    SECItem* response = CreateEncodedOCSPResponse(context);
    EXPECT_TRUE(response);
    
    
    Input result;
    EXPECT_EQ(Success, result.Init(response->data, response->len));
    return result;
  }
};

TEST_P(pkixocsp_VerifyEncodedResponse_WithoutResponseBytes, CorrectErrorCode)
{
  Input
    response(CreateEncodedOCSPErrorResponse(GetParam().responseStatus));

  bool expired;
  ASSERT_EQ(GetParam().expectedError,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
}

INSTANTIATE_TEST_CASE_P(pkixocsp_VerifyEncodedResponse_WithoutResponseBytes,
                        pkixocsp_VerifyEncodedResponse_WithoutResponseBytes,
                        testing::ValuesIn(WITHOUT_RESPONSEBYTES));




namespace {


static const char* byKey = nullptr;

} 

class pkixocsp_VerifyEncodedResponse_successful
  : public pkixocsp_VerifyEncodedResponse
{
public:
  void SetUp()
  {
    pkixocsp_VerifyEncodedResponse::SetUp();
  }

  static void SetUpTestCase()
  {
    pkixocsp_VerifyEncodedResponse::SetUpTestCase();
  }

  
  Input CreateEncodedOCSPSuccessfulResponse(
                    OCSPResponseContext::CertStatus certStatus,
                    const CertID& certID,
                     const char* signerName,
                    const ScopedSECKEYPrivateKey& signerPrivateKey,
                    PRTime producedAt, PRTime thisUpdate,
                     const PRTime* nextUpdate,
                     SECItem const* const* certs = nullptr)
  {
    OCSPResponseContext context(arena.get(), certID, producedAt);
    if (signerName) {
      context.signerNameDER = ASCIIToDERName(arena.get(), signerName);
      EXPECT_TRUE(context.signerNameDER);
    }
    context.signerPrivateKey = SECKEY_CopyPrivateKey(signerPrivateKey.get());
    EXPECT_TRUE(context.signerPrivateKey);
    context.responseStatus = OCSPResponseContext::successful;
    context.producedAt = producedAt;
    context.certs = certs;

    context.certIDHashAlg = SEC_OID_SHA1;
    context.certStatus = certStatus;
    context.thisUpdate = thisUpdate;
    context.nextUpdate = nextUpdate ? *nextUpdate : 0;
    context.includeNextUpdate = nextUpdate != nullptr;

    SECItem* response = CreateEncodedOCSPResponse(context);
    EXPECT_TRUE(response);
    Input result;
    EXPECT_EQ(Success, result.Init(response->data, response->len));
    return result;
  }
};

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byKey)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         rootPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID,
                                      now, END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byName)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, rootName,
                         rootPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byKey_without_nextUpdate)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         rootPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, nullptr));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, revoked)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::revoked, *endEntityCertID, byKey,
                         rootPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Result::ERROR_REVOKED_CERTIFICATE,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, unknown)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::unknown, *endEntityCertID, byKey,
                         rootPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_UNKNOWN_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}




class pkixocsp_VerifyEncodedResponse_DelegatedResponder
  : public pkixocsp_VerifyEncodedResponse_successful
{
protected:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  Input CreateEncodedIndirectOCSPSuccessfulResponse(
              const char* certSubjectName,
              OCSPResponseContext::CertStatus certStatus,
              const char* signerName,
              SECOidTag signerEKU = SEC_OID_OCSP_RESPONDER,
               Input* signerDEROut = nullptr)
  {
    PR_ASSERT(certSubjectName);

    const SECItem* extensions[] = {
      signerEKU != SEC_OID_UNKNOWN
        ? CreateEncodedEKUExtension(arena.get(), &signerEKU, 1,
                                    ExtensionCriticality::NotCritical)
        : nullptr,
      nullptr
    };
    ScopedSECKEYPrivateKey signerPrivateKey;
    SECItem* signerDER(CreateEncodedCertificate(
                          arena.get(), ++rootIssuedCount, rootName,
                          pr_oneDayBeforeNow, pr_oneDayAfterNow,
                          certSubjectName,
                          signerEKU != SEC_OID_UNKNOWN ? extensions : nullptr,
                          rootPrivateKey.get(), signerPrivateKey));
    EXPECT_TRUE(signerDER);
    if (signerDEROut) {
      EXPECT_EQ(Success,
                signerDEROut->Init(signerDER->data, signerDER->len));
    }

    const SECItem* signerNameDER = nullptr;
    if (signerName) {
      signerNameDER = ASCIIToDERName(arena.get(), signerName);
      EXPECT_TRUE(signerNameDER);
    }
    SECItem const* const certs[] = { signerDER, nullptr };
    return CreateEncodedOCSPSuccessfulResponse(certStatus, *endEntityCertID,
                                               signerName, signerPrivateKey,
                                               pr_oneDayBeforeNow,
                                               pr_oneDayBeforeNow,
                                               &pr_oneDayAfterNow, certs);
  }

  static SECItem* CreateEncodedCertificate(PLArenaPool* arena,
                                           uint32_t serialNumber,
                                           const char* issuer,
                                           PRTime notBefore,
                                           PRTime notAfter,
                                           const char* subject,
                               SECItem const* const* extensions,
                               SECKEYPrivateKey* signerKey,
                                    ScopedSECKEYPrivateKey& privateKey)
  {
    const SECItem* serialNumberDER(CreateEncodedSerialNumber(arena,
                                                             serialNumber));
    if (!serialNumberDER) {
      return nullptr;
    }
    const SECItem* issuerDER(ASCIIToDERName(arena, issuer));
    if (!issuerDER) {
      return nullptr;
    }
    const SECItem* subjectDER(ASCIIToDERName(arena, subject));
    if (!subjectDER) {
      return nullptr;
    }
    return ::mozilla::pkix::test::CreateEncodedCertificate(
                                    arena, v3,
                                    SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION,
                                    serialNumberDER, issuerDER, notBefore,
                                    notAfter, subjectDER, extensions,
                                    signerKey, SEC_OID_SHA256, privateKey);
  }
};

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_byKey)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "CN=good_indirect_byKey", OCSPResponseContext::good,
                         byKey));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_byName)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "CN=good_indirect_byName", OCSPResponseContext::good,
                         "CN=good_indirect_byName"));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_byKey_missing_signer)
{
  ScopedSECKEYPublicKey missingSignerPublicKey;
  ScopedSECKEYPrivateKey missingSignerPrivateKey;
  ASSERT_SECSuccess(GenerateKeyPair(missingSignerPublicKey,
                                    missingSignerPrivateKey));
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         missingSignerPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, nullptr));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_byName_missing_signer)
{
  ScopedSECKEYPublicKey missingSignerPublicKey;
  ScopedSECKEYPrivateKey missingSignerPrivateKey;
  ASSERT_SECSuccess(GenerateKeyPair(missingSignerPublicKey,
                                    missingSignerPrivateKey));
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         "CN=missing", missingSignerPrivateKey,
                         pr_oneDayBeforeNow, pr_oneDayBeforeNow, nullptr));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_expired)
{
  static const SECOidTag signerEKU = SEC_OID_OCSP_RESPONDER;
  static const char* signerName = "CN=good_indirect_expired";

  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), &signerEKU, 1,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                              rootName,
                                              pr_now - (10 * ONE_DAY),
                                              pr_now - (2 * ONE_DAY),
                                              signerName, extensions,
                                              rootPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  SECItem const* const certs[] = { signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_future)
{
  static const SECOidTag signerEKU = SEC_OID_OCSP_RESPONDER;
  static const char* signerName = "CN=good_indirect_future";

  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), &signerEKU, 1,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                              rootName,
                                              pr_now + (2 * ONE_DAY),
                                              pr_now + (10 * ONE_DAY),
                                              signerName, extensions,
                                              rootPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  SECItem const* const certs[] = { signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_no_eku)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "CN=good_indirect_wrong_eku",
                         OCSPResponseContext::good, byKey, SEC_OID_UNKNOWN));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_indirect_wrong_eku)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                        "CN=good_indirect_wrong_eku",
                        OCSPResponseContext::good, byKey,
                        SEC_OID_EXT_KEY_USAGE_SERVER_AUTH));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}


TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_tampered_eku)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "CN=good_indirect_tampered_eku",
                         OCSPResponseContext::good, byKey,
                         SEC_OID_EXT_KEY_USAGE_SERVER_AUTH));

#define EKU_PREFIX \
  0x06, 8, /* OBJECT IDENTIFIER, 8 bytes */ \
  0x2B, 6, 1, 5, 5, 7, /* id-pkix */ \
  0x03 /* id-kp */
  static const uint8_t EKU_SERVER_AUTH[] = { EKU_PREFIX, 0x01 }; 
  static const uint8_t EKU_OCSP_SIGNER[] = { EKU_PREFIX, 0x09 }; 
#undef EKU_PREFIX
  SECItem responseSECItem = UnsafeMapInputToSECItem(response);
  ASSERT_SECSuccess(TamperOnce(responseSECItem,
                               EKU_SERVER_AUTH, PR_ARRAY_SIZE(EKU_SERVER_AUTH),
                               EKU_OCSP_SIGNER, PR_ARRAY_SIZE(EKU_OCSP_SIGNER)));

  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_unknown_issuer)
{
  static const char* subCAName = "CN=good_indirect_unknown_issuer sub-CA";
  static const char* signerName = "CN=good_indirect_unknown_issuer OCSP signer";

  
  ScopedSECKEYPublicKey unknownPublicKey;
  ScopedSECKEYPrivateKey unknownPrivateKey;
  ASSERT_SECSuccess(GenerateKeyPair(unknownPublicKey, unknownPrivateKey));

  
  static const SECOidTag signerEKU = SEC_OID_OCSP_RESPONDER;
  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), &signerEKU, 1,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), 1,
                        subCAName, pr_oneDayBeforeNow, pr_oneDayAfterNow,
                        signerName, extensions, unknownPrivateKey.get(),
                        signerPrivateKey));
  ASSERT_TRUE(signerDER);

  
  SECItem const* const certs[] = { signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}




TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_indirect_subca_1_first)
{
  static const char* subCAName = "CN=good_indirect_subca_1_first sub-CA";
  static const char* signerName = "CN=good_indirect_subca_1_first OCSP signer";

  
  const SECItem* subCAExtensions[] = {
    CreateEncodedBasicConstraints(arena.get(), true, 0,
                                  ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey subCAPrivateKey;
  SECItem* subCADER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                             rootName,
                                             pr_oneDayBeforeNow, pr_oneDayAfterNow,
                                             subCAName, subCAExtensions,
                                             rootPrivateKey.get(),
                                             subCAPrivateKey));
  ASSERT_TRUE(subCADER);

  
  static const SECOidTag signerEKU = SEC_OID_OCSP_RESPONDER;
  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), &signerEKU, 1,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), 1, subCAName,
                                              pr_oneDayBeforeNow,
                                              pr_oneDayAfterNow,
                                              signerName, extensions,
                                              subCAPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  
  
  SECItem const* const certs[] = { subCADER, signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}




TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_indirect_subca_1_second)
{
  static const char* subCAName = "CN=good_indirect_subca_1_second sub-CA";
  static const char* signerName = "CN=good_indirect_subca_1_second OCSP signer";

  
  const SECItem* subCAExtensions[] = {
    CreateEncodedBasicConstraints(arena.get(), true, 0,
                                  ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey subCAPrivateKey;
  SECItem* subCADER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                             rootName,
                                             pr_oneDayBeforeNow,
                                             pr_oneDayAfterNow,
                                             subCAName, subCAExtensions,
                                             rootPrivateKey.get(),
                                             subCAPrivateKey));
  ASSERT_TRUE(subCADER);

  
  static const SECOidTag signerEKU = SEC_OID_OCSP_RESPONDER;
  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), &signerEKU, 1,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), 1, subCAName,
                                              pr_oneDayBeforeNow,
                                              pr_oneDayAfterNow,
                                              signerName, extensions,
                                              subCAPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  
  
  SECItem const* const certs[] = { signerDER, subCADER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, pr_oneDayBeforeNow,
                         pr_oneDayBeforeNow, &pr_oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

class pkixocsp_VerifyEncodedResponse_GetCertTrust
  : public pkixocsp_VerifyEncodedResponse_DelegatedResponder {
public:
  void SetUp()
  {
    pkixocsp_VerifyEncodedResponse_DelegatedResponder::SetUp();

    Input
      createdResponse(
        CreateEncodedIndirectOCSPSuccessfulResponse(
          "CN=OCSPGetCertTrustTest Signer", OCSPResponseContext::good,
          byKey, SEC_OID_OCSP_RESPONDER, &signerCertDER));
    if (response.Init(createdResponse) != Success) {
      PR_Abort();
    }

    if (response.GetLength() == 0 || signerCertDER.GetLength() == 0) {
      PR_Abort();
    }
  }

  class TrustDomain : public OCSPTestTrustDomain
  {
  public:
    TrustDomain()
      : certTrustLevel(TrustLevel::InheritsTrust)
    {
    }

    bool SetCertTrust(Input certDER, TrustLevel certTrustLevel)
    {
      EXPECT_EQ(Success, this->certDER.Init(certDER));
      this->certTrustLevel = certTrustLevel;
      return true;
    }
  private:
    virtual Result GetCertTrust(EndEntityOrCA endEntityOrCA,
                                const CertPolicyId&,
                                Input candidateCert,
                                 TrustLevel& trustLevel)
    {
      EXPECT_EQ(endEntityOrCA, EndEntityOrCA::MustBeEndEntity);
      EXPECT_NE(0, certDER.GetLength());
      EXPECT_TRUE(InputsAreEqual(certDER, candidateCert));
      trustLevel = certTrustLevel;
      return Success;
    }

    Input certDER;
    TrustLevel certTrustLevel;
  };

  TrustDomain trustDomain;
  Input signerCertDER; 
  Input response; 
};

TEST_F(pkixocsp_VerifyEncodedResponse_GetCertTrust, InheritTrust)
{
  ASSERT_TRUE(trustDomain.SetCertTrust(signerCertDER,
                                       TrustLevel::InheritsTrust));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_GetCertTrust, TrustAnchor)
{
  ASSERT_TRUE(trustDomain.SetCertTrust(signerCertDER,
                                       TrustLevel::TrustAnchor));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_GetCertTrust, ActivelyDistrusted)
{
  ASSERT_TRUE(trustDomain.SetCertTrust(signerCertDER,
                                       TrustLevel::ActivelyDistrusted));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}
