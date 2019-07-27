























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"

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
    rootKeyPair = GenerateKeyPair();
    return rootKeyPair.get() != nullptr;
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
    if (rootSPKIDER.Init(rootKeyPair->subjectPublicKeyInfo.data(),
                         rootKeyPair->subjectPublicKeyInfo.length())
          != Success) {
      abort();
    }
    endEntityCertID = new (std::nothrow) CertID(rootNameDERInput, rootSPKIDER,
                                                serialNumberDERInput);
    if (!endEntityCertID) {
      abort();
    }
  }

  static ScopedTestKeyPair rootKeyPair;
  static long rootIssuedCount;
  OCSPTestTrustDomain trustDomain;

  
  ByteString rootNameDER;
  ByteString serialNumberDER;
  
  ScopedPtr<CertID, deleteCertID> endEntityCertID;
};

 ScopedTestKeyPair pkixocsp_VerifyEncodedResponse::rootKeyPair;
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
  ByteString CreateEncodedOCSPErrorResponse(uint8_t status)
  {
    static const Input EMPTY;
    OCSPResponseContext context(CertID(EMPTY, EMPTY, EMPTY),
                                oneDayBeforeNow);
    context.responseStatus = status;
    context.skipResponseBytes = true;
    return CreateEncodedOCSPResponse(context);
  }
};

TEST_P(pkixocsp_VerifyEncodedResponse_WithoutResponseBytes, CorrectErrorCode)
{
  ByteString
    responseString(CreateEncodedOCSPErrorResponse(GetParam().responseStatus));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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

  ByteString CreateEncodedOCSPSuccessfulResponse(
                    OCSPResponseContext::CertStatus certStatus,
                    const CertID& certID,
                     const char* signerName,
                    const TestKeyPair& signerKeyPair,
                    time_t producedAt, time_t thisUpdate,
                     const time_t* nextUpdate,
                     const ByteString* certs = nullptr)
  {
    OCSPResponseContext context(certID, producedAt);
    if (signerName) {
      context.signerNameDER = CNToDERName(signerName);
      EXPECT_NE(ENCODING_FAILED, context.signerNameDER);
    }
    context.signerKeyPair = signerKeyPair.Clone();
    EXPECT_TRUE(context.signerKeyPair);
    context.responseStatus = OCSPResponseContext::successful;
    context.producedAt = producedAt;
    context.certs = certs;

    context.certStatus = certStatus;
    context.thisUpdate = thisUpdate;
    context.nextUpdate = nextUpdate ? *nextUpdate : 0;
    context.includeNextUpdate = nextUpdate != nullptr;

    return CreateEncodedOCSPResponse(context);
  }
};

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byKey)
{
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         *rootKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID,
                                      Now(), END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byName)
{
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, rootName,
                         *rootKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byKey_without_nextUpdate)
{
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         *rootKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, nullptr));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, revoked)
{
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::revoked, *endEntityCertID, byKey,
                         *rootKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_REVOKED_CERTIFICATE,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, unknown)
{
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::unknown, *endEntityCertID, byKey,
                         *rootKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ByteString CreateEncodedIndirectOCSPSuccessfulResponse(
               const char* certSubjectName,
               OCSPResponseContext::CertStatus certStatus,
               const char* signerName,
                const Input* signerEKUDER = &OCSPSigningEKUDER,
                ByteString* signerDEROut = nullptr)
  {
    assert(certSubjectName);

    const ByteString extensions[] = {
      signerEKUDER
        ? CreateEncodedEKUExtension(*signerEKUDER,
                                    ExtensionCriticality::NotCritical)
        : ByteString(),
      ByteString()
    };
    ScopedTestKeyPair signerKeyPair;
    ByteString signerDER(CreateEncodedCertificate(
                           ++rootIssuedCount, rootName,
                           oneDayBeforeNow, oneDayAfterNow, certSubjectName,
                           signerEKUDER ? extensions : nullptr,
                           rootKeyPair.get(), signerKeyPair));
    EXPECT_NE(ENCODING_FAILED, signerDER);
    if (signerDEROut) {
      *signerDEROut = signerDER;
    }

    ByteString signerNameDER;
    if (signerName) {
      signerNameDER = CNToDERName(signerName);
      EXPECT_NE(ENCODING_FAILED, signerNameDER);
    }
    ByteString certs[] = { signerDER, ByteString() };
    return CreateEncodedOCSPSuccessfulResponse(certStatus, *endEntityCertID,
                                               signerName, *signerKeyPair,
                                               oneDayBeforeNow,
                                               oneDayBeforeNow,
                                               &oneDayAfterNow, certs);
  }

  static ByteString CreateEncodedCertificate(uint32_t serialNumber,
                                             const char* issuer,
                                             time_t notBefore,
                                             time_t notAfter,
                                             const char* subject,
                                 const ByteString* extensions,
                                 TestKeyPair* signerKeyPair,
                                      ScopedTestKeyPair& keyPair)
  {
    ByteString serialNumberDER(CreateEncodedSerialNumber(serialNumber));
    if (serialNumberDER == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    ByteString issuerDER(CNToDERName(issuer));
    if (issuerDER == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    ByteString subjectDER(CNToDERName(subject));
    if (subjectDER == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    return ::mozilla::pkix::test::CreateEncodedCertificate(
                                    v3,
                                    sha256WithRSAEncryption,
                                    serialNumberDER, issuerDER, notBefore,
                                    notAfter, subjectDER, extensions,
                                    signerKeyPair,
                                    SignatureAlgorithm::rsa_pkcs1_with_sha256,
                                    keyPair);
  }

  static const Input OCSPSigningEKUDER;
};

 const Input pkixocsp_VerifyEncodedResponse_DelegatedResponder::
  OCSPSigningEKUDER(tlv_id_kp_OCSPSigning);

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_byKey)
{
  ByteString responseString(
               CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_byKey", OCSPResponseContext::good,
                         byKey));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Success,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_byName)
{
  ByteString responseString(
               CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_byName", OCSPResponseContext::good,
                         "good_indirect_byName"));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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
  ScopedTestKeyPair missingSignerKeyPair(GenerateKeyPair());
  ASSERT_TRUE(missingSignerKeyPair);

  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         *missingSignerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, nullptr));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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
  ScopedTestKeyPair missingSignerKeyPair(GenerateKeyPair());
  ASSERT_TRUE(missingSignerKeyPair);
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         "missing", *missingSignerKeyPair,
                         oneDayBeforeNow, oneDayBeforeNow, nullptr));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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

  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    ByteString()
  };

  ScopedTestKeyPair signerKeyPair;
  ByteString signerDER(CreateEncodedCertificate(
                          ++rootIssuedCount, rootName,
                          now - (10 * Time::ONE_DAY_IN_SECONDS),
                          now - (2 * Time::ONE_DAY_IN_SECONDS),
                          signerName, extensions, rootKeyPair.get(),
                          signerKeyPair));
  ASSERT_NE(ENCODING_FAILED, signerDER);

  ByteString certs[] = { signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_future)
{
  static const char* signerName = "good_indirect_future";

  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    ByteString()
  };

  ScopedTestKeyPair signerKeyPair;
  ByteString signerDER(CreateEncodedCertificate(
                         ++rootIssuedCount, rootName,
                         now + (2 * Time::ONE_DAY_IN_SECONDS),
                         now + (10 * Time::ONE_DAY_IN_SECONDS),
                         signerName, extensions, rootKeyPair.get(),
                         signerKeyPair));
  ASSERT_NE(ENCODING_FAILED, signerDER);

  ByteString certs[] = { signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_no_eku)
{
  ByteString responseString(
               CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_wrong_eku",
                         OCSPResponseContext::good, byKey, nullptr));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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
  ByteString responseString(
               CreateEncodedIndirectOCSPSuccessfulResponse(
                        "good_indirect_wrong_eku",
                        OCSPResponseContext::good, byKey, &serverAuthEKUDER));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}


TEST_F(pkixocsp_VerifyEncodedResponse_DelegatedResponder, good_tampered_eku)
{
  ByteString tamperedResponse(
               CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_tampered_eku",
                         OCSPResponseContext::good, byKey, &serverAuthEKUDER));
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

  
  ScopedTestKeyPair unknownKeyPair(GenerateKeyPair());
  ASSERT_TRUE(unknownKeyPair);

  
  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    ByteString()
  };
  ScopedTestKeyPair signerKeyPair;
  ByteString signerDER(CreateEncodedCertificate(
                         1, subCAName, oneDayBeforeNow, oneDayAfterNow,
                         signerName, extensions, unknownKeyPair.get(),
                         signerKeyPair));
  ASSERT_NE(ENCODING_FAILED, signerDER);

  
  ByteString certs[] = { signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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

  
  const ByteString subCAExtensions[] = {
    CreateEncodedBasicConstraints(true, 0, ExtensionCriticality::NotCritical),
    ByteString()
  };
  ScopedTestKeyPair subCAKeyPair;
  ByteString subCADER(CreateEncodedCertificate(
                        ++rootIssuedCount, rootName,
                        oneDayBeforeNow, oneDayAfterNow,
                        subCAName, subCAExtensions, rootKeyPair.get(),
                        subCAKeyPair));
  ASSERT_NE(ENCODING_FAILED, subCADER);

  
  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    ByteString(),
  };
  ScopedTestKeyPair signerKeyPair;
  ByteString signerDER(CreateEncodedCertificate(
                         1, subCAName, oneDayBeforeNow, oneDayAfterNow,
                         signerName, extensions, subCAKeyPair.get(),
                         signerKeyPair));
  ASSERT_NE(ENCODING_FAILED, signerDER);

  
  
  ByteString certs[] = { subCADER, signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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

  
  const ByteString subCAExtensions[] = {
    CreateEncodedBasicConstraints(true, 0, ExtensionCriticality::NotCritical),
    ByteString()
  };
  ScopedTestKeyPair subCAKeyPair;
  ByteString subCADER(CreateEncodedCertificate(++rootIssuedCount, rootName,
                                               oneDayBeforeNow, oneDayAfterNow,
                                               subCAName, subCAExtensions,
                                               rootKeyPair.get(),
                                               subCAKeyPair));
  ASSERT_NE(ENCODING_FAILED, subCADER);

  
  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER,
                              ExtensionCriticality::NotCritical),
    ByteString()
  };
  ScopedTestKeyPair signerKeyPair;
  ByteString signerDER(CreateEncodedCertificate(
                         1, subCAName, oneDayBeforeNow, oneDayAfterNow,
                         signerName, extensions, subCAKeyPair.get(),
                         signerKeyPair));
  ASSERT_NE(ENCODING_FAILED, signerDER);

  
  
  ByteString certs[] = { signerDER, subCADER, ByteString() };
  ByteString responseString(
                 CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow, certs));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
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

    responseString =
        CreateEncodedIndirectOCSPSuccessfulResponse(
          "OCSPGetCertTrustTest Signer", OCSPResponseContext::good,
          byKey, &OCSPSigningEKUDER, &signerCertDER);
    if (responseString == ENCODING_FAILED) {
      abort();
    }
    if (response.Init(responseString.data(), responseString.length())
          != Success) {
      abort();
    }
    if (signerCertDER.length() == 0) {
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

    bool SetCertTrust(const ByteString& certDER, TrustLevel certTrustLevel)
    {
      this->certDER = certDER;
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
      EXPECT_FALSE(certDER.empty());
      Input certDERInput;
      EXPECT_EQ(Success, certDERInput.Init(certDER.data(), certDER.length()));
      EXPECT_TRUE(InputsAreEqual(certDERInput, candidateCert));
      trustLevel = certTrustLevel;
      return Success;
    }

    ByteString certDER;
    TrustLevel certTrustLevel;
  };

  TrustDomain trustDomain;
  ByteString signerCertDER;
  ByteString responseString;
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
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}
