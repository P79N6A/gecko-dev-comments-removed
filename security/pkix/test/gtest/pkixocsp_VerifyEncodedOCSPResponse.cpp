























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
char const* const rootName = "Test CA 1";
void deleteCertID(CertID* certID) { delete certID; }
} 

class pkixocsp_VerifyEncodedResponse : public NSSTest
{
public:
  static bool SetUpTestCaseInner()
  {
    ScopedSECKEYPublicKey rootPublicKey;
    if (GenerateKeyPair(rootPublicKey, rootPrivateKey) != Success) {
      return false;
    }
    ScopedSECItem rootSPKIItem(
      SECKEY_EncodeDERSubjectPublicKeyInfo(rootPublicKey.get()));
    if (!rootSPKIItem) {
      return false;
    }
    rootSPKI.assign(rootSPKIItem->data, rootSPKIItem->len);
    return true;
  }

  static void SetUpTestCase()
  {
    NSSTest::SetUpTestCase();
    if (!SetUpTestCaseInner()) {
      abort();
    }
  }

  void SetUp()
  {
    NSSTest::SetUp();

    rootNameDER = CNToDERName(rootName);
    if (rootNameDER == ENCODING_FAILED) {
      abort();
    }
    Input rootNameDERInput;
    if (rootNameDERInput.Init(rootNameDER.data(), rootNameDER.length())
          != Success) {
      abort();
    }

    serialNumberDER = CreateEncodedSerialNumber(++rootIssuedCount);
    if (serialNumberDER == ENCODING_FAILED) {
      abort();
    }
    Input serialNumberDERInput;
    if (serialNumberDERInput.Init(serialNumberDER.data(),
                                  serialNumberDER.length()) != Success) {
      abort();
    }

    Input rootSPKIDER;
    if (rootSPKIDER.Init(rootSPKI.data(), rootSPKI.length()) != Success) {
      abort();
    }
    endEntityCertID = new (std::nothrow) CertID(rootNameDERInput, rootSPKIDER,
                                                serialNumberDERInput);
    if (!endEntityCertID) {
      abort();
    }
  }

  static ScopedSECKEYPrivateKey rootPrivateKey;
  static ByteString rootSPKI;
  static long rootIssuedCount;
  OCSPTestTrustDomain trustDomain;

  ByteString rootNameDER;
  ByteString serialNumberDER;
  
  ScopedPtr<CertID, deleteCertID> endEntityCertID;
};

 ScopedSECKEYPrivateKey
              pkixocsp_VerifyEncodedResponse::rootPrivateKey;
 ByteString pkixocsp_VerifyEncodedResponse::rootSPKI;
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
                                oneDayBeforeNow);
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
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
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
                    time_t producedAt, time_t thisUpdate,
                     const time_t* nextUpdate,
                     SECItem const* const* certs = nullptr)
  {
    OCSPResponseContext context(arena.get(), certID, producedAt);
    if (signerName) {
      context.signerNameDER = CNToDERName(signerName);
      EXPECT_NE(ENCODING_FAILED, context.signerNameDER);
    }
    context.signerPrivateKey = SECKEY_CopyPrivateKey(signerPrivateKey.get());
    EXPECT_TRUE(context.signerPrivateKey);
    context.responseStatus = OCSPResponseContext::successful;
    context.producedAt = producedAt;
    context.certs = certs;

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
                         rootPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID,
                                      Now(), END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byName)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, rootName,
                         rootPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byKey_without_nextUpdate)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         rootPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, nullptr));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, revoked)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::revoked, *endEntityCertID, byKey,
                         rootPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Result::ERROR_REVOKED_CERTIFICATE,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, unknown)
{
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::unknown, *endEntityCertID, byKey,
                         rootPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_UNKNOWN_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
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
               const Input* signerEKUDER = &OCSPSigningEKUDER,
               Input* signerDEROut = nullptr)
  {
    assert(certSubjectName);

    const SECItem* extensions[] = {
      signerEKUDER
        ? CreateEncodedEKUExtension(arena.get(), *signerEKUDER,
                                    ExtensionCriticality::NotCritical)
        : nullptr,
      nullptr
    };
    ScopedSECKEYPrivateKey signerPrivateKey;
    SECItem* signerDER(CreateEncodedCertificate(
                          arena.get(), ++rootIssuedCount, rootName,
                          oneDayBeforeNow, oneDayAfterNow, certSubjectName,
                          signerEKUDER ? extensions : nullptr,
                          rootPrivateKey.get(), signerPrivateKey));
    EXPECT_TRUE(signerDER);
    if (signerDEROut) {
      EXPECT_EQ(Success,
                signerDEROut->Init(signerDER->data, signerDER->len));
    }

    ByteString signerNameDER;
    if (signerName) {
      signerNameDER = CNToDERName(signerName);
      EXPECT_NE(ENCODING_FAILED, signerNameDER);
    }
    SECItem const* const certs[] = { signerDER, nullptr };
    return CreateEncodedOCSPSuccessfulResponse(certStatus, *endEntityCertID,
                                               signerName, signerPrivateKey,
                                               oneDayBeforeNow,
                                               oneDayBeforeNow,
                                               &oneDayAfterNow, certs);
  }

  static SECItem* CreateEncodedCertificate(PLArenaPool* arena,
                                           uint32_t serialNumber,
                                           const char* issuer,
                                           time_t notBefore,
                                           time_t notAfter,
                                           const char* subject,
                               SECItem const* const* extensions,
                               SECKEYPrivateKey* signerKey,
                                    ScopedSECKEYPrivateKey& privateKey)
  {
    ByteString serialNumberDER(CreateEncodedSerialNumber(serialNumber));
    if (serialNumberDER == ENCODING_FAILED) {
      return nullptr;
    }
    ByteString issuerDER(CNToDERName(issuer));
    if (issuerDER == ENCODING_FAILED) {
      return nullptr;
    }
    ByteString subjectDER(CNToDERName(subject));
    if (subjectDER == ENCODING_FAILED) {
      return nullptr;
    }
    return ::mozilla::pkix::test::CreateEncodedCertificate(
                                    arena, v3,
                                    sha256WithRSAEncryption,
                                    serialNumberDER, issuerDER, notBefore,
                                    notAfter, subjectDER, extensions,
                                    signerKey,
                                    SignatureAlgorithm::rsa_pkcs1_with_sha256,
                                    privateKey);
  }

  static const Input OCSPSigningEKUDER;
};

 const Input pkixocsp_VerifyEncodedResponse_DelegatedResponder::
  OCSPSigningEKUDER(tlv_id_kp_OCSPSigning);

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_byKey)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_byKey", OCSPResponseContext::good,
                         byKey));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_byName)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_byName", OCSPResponseContext::good,
                         "good_indirect_byName"));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_byKey_missing_signer)
{
  ScopedSECKEYPublicKey missingSignerPublicKey;
  ScopedSECKEYPrivateKey missingSignerPrivateKey;
  ASSERT_EQ(Success, GenerateKeyPair(missingSignerPublicKey,
                                     missingSignerPrivateKey));
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         missingSignerPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, nullptr));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_byName_missing_signer)
{
  ScopedSECKEYPublicKey missingSignerPublicKey;
  ScopedSECKEYPrivateKey missingSignerPrivateKey;
  ASSERT_EQ(Success, GenerateKeyPair(missingSignerPublicKey,
                                     missingSignerPrivateKey));
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         "missing", missingSignerPrivateKey,
                         oneDayBeforeNow, oneDayBeforeNow, nullptr));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_expired)
{
  static const char* signerName = "good_indirect_expired";

  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    nullptr
  };

  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                              rootName,
                                              now - (10 * Time::ONE_DAY_IN_SECONDS),
                                              now - (2 * Time::ONE_DAY_IN_SECONDS),
                                              signerName, extensions,
                                              rootPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  SECItem const* const certs[] = { signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_future)
{
  static const char* signerName = "good_indirect_future";

  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    nullptr
  };

  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                              rootName,
                                              now + (2 * Time::ONE_DAY_IN_SECONDS),
                                              now + (10 * Time::ONE_DAY_IN_SECONDS),
                                              signerName, extensions,
                                              rootPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  SECItem const* const certs[] = { signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_no_eku)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_wrong_eku",
                         OCSPResponseContext::good, byKey, nullptr));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

static const Input serverAuthEKUDER(tlv_id_kp_serverAuth);

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_indirect_wrong_eku)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                        "good_indirect_wrong_eku",
                        OCSPResponseContext::good, byKey, &serverAuthEKUDER));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}


TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_tampered_eku)
{
  Input response(CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_tampered_eku",
                         OCSPResponseContext::good, byKey,
                         &serverAuthEKUDER));
  ByteString tamperedResponse(response.UnsafeGetData(),
                              response.GetLength());
  ASSERT_EQ(Success,
            TamperOnce(tamperedResponse,
                       ByteString(tlv_id_kp_serverAuth,
                                  sizeof(tlv_id_kp_serverAuth)),
                       ByteString(tlv_id_kp_OCSPSigning,
                                  sizeof(tlv_id_kp_OCSPSigning))));
  Input tamperedResponseInput;
  ASSERT_EQ(Success, tamperedResponseInput.Init(tamperedResponse.data(),
                                                tamperedResponse.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      tamperedResponseInput, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_unknown_issuer)
{
  static const char* subCAName = "good_indirect_unknown_issuer sub-CA";
  static const char* signerName = "good_indirect_unknown_issuer OCSP signer";

  
  ScopedSECKEYPublicKey unknownPublicKey;
  ScopedSECKEYPrivateKey unknownPrivateKey;
  ASSERT_EQ(Success, GenerateKeyPair(unknownPublicKey, unknownPrivateKey));

  
  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), 1,
                        subCAName, oneDayBeforeNow, oneDayAfterNow,
                        signerName, extensions, unknownPrivateKey.get(),
                        signerPrivateKey));
  ASSERT_TRUE(signerDER);

  
  SECItem const* const certs[] = { signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}




TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_indirect_subca_1_first)
{
  static const char* subCAName = "good_indirect_subca_1_first sub-CA";
  static const char* signerName = "good_indirect_subca_1_first OCSP signer";

  
  const SECItem* subCAExtensions[] = {
    CreateEncodedBasicConstraints(arena.get(), true, 0,
                                  ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey subCAPrivateKey;
  SECItem* subCADER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                             rootName,
                                             oneDayBeforeNow, oneDayAfterNow,
                                             subCAName, subCAExtensions,
                                             rootPrivateKey.get(),
                                             subCAPrivateKey));
  ASSERT_TRUE(subCADER);

  
  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), 1, subCAName,
                                              oneDayBeforeNow, oneDayAfterNow,
                                              signerName, extensions,
                                              subCAPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  
  
  SECItem const* const certs[] = { subCADER, signerDER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}




TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder,
       good_indirect_subca_1_second)
{
  static const char* subCAName = "good_indirect_subca_1_second sub-CA";
  static const char* signerName = "good_indirect_subca_1_second OCSP signer";

  
  const SECItem* subCAExtensions[] = {
    CreateEncodedBasicConstraints(arena.get(), true, 0,
                                  ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey subCAPrivateKey;
  SECItem* subCADER(CreateEncodedCertificate(arena.get(), ++rootIssuedCount,
                                             rootName,
                                             oneDayBeforeNow, oneDayAfterNow,
                                             subCAName, subCAExtensions,
                                             rootPrivateKey.get(),
                                             subCAPrivateKey));
  ASSERT_TRUE(subCADER);

  
  const SECItem* extensions[] = {
    CreateEncodedEKUExtension(arena.get(), OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    nullptr
  };
  ScopedSECKEYPrivateKey signerPrivateKey;
  SECItem* signerDER(CreateEncodedCertificate(arena.get(), 1, subCAName,
                                              oneDayBeforeNow, oneDayAfterNow,
                                              signerName, extensions,
                                              subCAPrivateKey.get(),
                                              signerPrivateKey));
  ASSERT_TRUE(signerDER);

  
  
  SECItem const* const certs[] = { signerDER, subCADER, nullptr };
  Input response(CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, signerPrivateKey, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
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
          "OCSPGetCertTrustTest Signer", OCSPResponseContext::good,
          byKey, &OCSPSigningEKUDER, &signerCertDER));
    if (response.Init(createdResponse) != Success) {
      abort();
    }

    if (response.GetLength() == 0 || signerCertDER.GetLength() == 0) {
      abort();
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
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
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
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
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
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}
