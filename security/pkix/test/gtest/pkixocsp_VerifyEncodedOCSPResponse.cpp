























#include "pkixgtest.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

const uint16_t END_ENTITY_MAX_LIFETIME_IN_DAYS = 10;


class OCSPTestTrustDomain : public DefaultCryptoTrustDomain
{
public:
  OCSPTestTrustDomain() { }

  Result GetCertTrust(EndEntityOrCA endEntityOrCA, const CertPolicyId&,
                      Input,  TrustLevel& trustLevel)
                       override
  {
    EXPECT_EQ(endEntityOrCA, EndEntityOrCA::MustBeEndEntity);
    trustLevel = TrustLevel::InheritsTrust;
    return Success;
  }
};

namespace {
char const* const rootName = "Test CA 1";
void deleteCertID(CertID* certID) { delete certID; }
} 

class pkixocsp_VerifyEncodedResponse : public ::testing::Test
{
public:
  static void SetUpTestCase()
  {
    rootKeyPair.reset(GenerateKeyPair());
    if (!rootKeyPair) {
      abort();
    }
  }

  void SetUp()
  {
    rootNameDER = CNToDERName(rootName);
    if (ENCODING_FAILED(rootNameDER)) {
      abort();
    }
    Input rootNameDERInput;
    if (rootNameDERInput.Init(rootNameDER.data(), rootNameDER.length())
          != Success) {
      abort();
    }

    serialNumberDER =
      CreateEncodedSerialNumber(static_cast<long>(++rootIssuedCount));
    if (ENCODING_FAILED(serialNumberDER)) {
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
    endEntityCertID.reset(new (std::nothrow) CertID(rootNameDERInput, rootSPKIDER,
                                                    serialNumberDERInput));
    if (!endEntityCertID) {
      abort();
    }
  }

  static ScopedTestKeyPair rootKeyPair;
  static uint32_t rootIssuedCount;
  OCSPTestTrustDomain trustDomain;

  
  ByteString rootNameDER;
  ByteString serialNumberDER;
  
  ScopedPtr<CertID, deleteCertID> endEntityCertID;
};

 ScopedTestKeyPair pkixocsp_VerifyEncodedResponse::rootKeyPair;
 uint32_t pkixocsp_VerifyEncodedResponse::rootIssuedCount = 0;




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
                    const TestSignatureAlgorithm& signatureAlgorithm,
        const ByteString* certs = nullptr)
  {
    OCSPResponseContext context(certID, producedAt);
    if (signerName) {
      context.signerNameDER = CNToDERName(signerName);
      EXPECT_FALSE(ENCODING_FAILED(context.signerNameDER));
    }
    context.signerKeyPair.reset(signerKeyPair.Clone());
    EXPECT_TRUE(context.signerKeyPair.get());
    context.responseStatus = OCSPResponseContext::successful;
    context.producedAt = producedAt;
    context.signatureAlgorithm = signatureAlgorithm;
    context.certs = certs;

    context.certStatus = static_cast<uint8_t>(certStatus);
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
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption()));
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
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption()));
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
                         oneDayBeforeNow, nullptr,
                         sha256WithRSAEncryption()));
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
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption()));
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
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption()));
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

TEST_F(pkixocsp_VerifyEncodedResponse_successful,
       good_unsupportedSignatureAlgorithm)
{
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         *rootKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow,
                         md5WithRSAEncryption()));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID,
                                      Now(), END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
  ASSERT_FALSE(expired);
}





TEST_F(pkixocsp_VerifyEncodedResponse_successful, check_validThrough)
{
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         *rootKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption()));
  Time validThrough(Time::uninitialized);
  {
    Input response;
    ASSERT_EQ(Success,
              response.Init(responseString.data(), responseString.length()));
    bool expired;
    ASSERT_EQ(Success,
              VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID,
                                        Now(), END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                        response, expired, nullptr,
                                        &validThrough));
    ASSERT_FALSE(expired);
    
    
    Time oneDayAfterNowAsPKIXTime(
          TimeFromEpochInSeconds(static_cast<uint64_t>(oneDayAfterNow)));
    ASSERT_TRUE(validThrough > oneDayAfterNowAsPKIXTime);
  }
  {
    Input response;
    ASSERT_EQ(Success,
              response.Init(responseString.data(), responseString.length()));
    bool expired;
    
    
    ASSERT_EQ(Success,
              VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID,
                                        validThrough, END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                        response, expired));
    ASSERT_FALSE(expired);
  }
  {
    Time noLongerValid(validThrough);
    ASSERT_EQ(Success, noLongerValid.AddSeconds(1));
    Input response;
    ASSERT_EQ(Success,
              response.Init(responseString.data(), responseString.length()));
    bool expired;
    
    
    ASSERT_EQ(Result::ERROR_OCSP_OLD_RESPONSE,
              VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID,
                                        noLongerValid, END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                        response, expired));
    ASSERT_TRUE(expired);
  }
}




class pkixocsp_VerifyEncodedResponse_DelegatedResponder
  : public pkixocsp_VerifyEncodedResponse_successful
{
protected:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ByteString CreateEncodedIndirectOCSPSuccessfulResponse(
               const char* certSubjectName,
               OCSPResponseContext::CertStatus certStatus,
               const char* signerName,
               const TestSignatureAlgorithm& certSignatureAlgorithm,
                const Input* signerEKUDER = &OCSPSigningEKUDER,
                ByteString* signerDEROut = nullptr)
  {
    assert(certSubjectName);

    const ByteString extensions[] = {
      signerEKUDER
        ? CreateEncodedEKUExtension(*signerEKUDER, Critical::No)
        : ByteString(),
      ByteString()
    };
    ScopedTestKeyPair signerKeyPair(GenerateKeyPair());
    ByteString signerDER(CreateEncodedCertificate(
                           ++rootIssuedCount, certSignatureAlgorithm,
                           rootName, oneDayBeforeNow, oneDayAfterNow,
                           certSubjectName, *signerKeyPair,
                           signerEKUDER ? extensions : nullptr,
                           *rootKeyPair));
    EXPECT_FALSE(ENCODING_FAILED(signerDER));
    if (signerDEROut) {
      *signerDEROut = signerDER;
    }

    ByteString signerNameDER;
    if (signerName) {
      signerNameDER = CNToDERName(signerName);
      EXPECT_FALSE(ENCODING_FAILED(signerNameDER));
    }
    ByteString certs[] = { signerDER, ByteString() };
    return CreateEncodedOCSPSuccessfulResponse(certStatus, *endEntityCertID,
                                               signerName, *signerKeyPair,
                                               oneDayBeforeNow,
                                               oneDayBeforeNow,
                                               &oneDayAfterNow,
                                               sha256WithRSAEncryption(),
                                               certs);
  }

  static ByteString CreateEncodedCertificate(uint32_t serialNumber,
                                             const TestSignatureAlgorithm& signatureAlg,
                                             const char* issuer,
                                             time_t notBefore,
                                             time_t notAfter,
                                             const char* subject,
                                             const TestKeyPair& subjectKeyPair,
                                 const ByteString* extensions,
                                             const TestKeyPair& signerKeyPair)
  {
    ByteString serialNumberDER(CreateEncodedSerialNumber(
                                 static_cast<long>(serialNumber)));
    if (ENCODING_FAILED(serialNumberDER)) {
      return ByteString();
    }
    ByteString issuerDER(CNToDERName(issuer));
    if (ENCODING_FAILED(issuerDER)) {
      return ByteString();
    }
    ByteString subjectDER(CNToDERName(subject));
    if (ENCODING_FAILED(subjectDER)) {
      return ByteString();
    }
    return ::mozilla::pkix::test::CreateEncodedCertificate(
                                    v3, signatureAlg, serialNumberDER,
                                    issuerDER, notBefore, notAfter,
                                    subjectDER, subjectKeyPair, extensions,
                                    signerKeyPair, signatureAlg);
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
                         byKey, sha256WithRSAEncryption()));
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
                         "good_indirect_byName", sha256WithRSAEncryption()));
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
  ASSERT_TRUE(missingSignerKeyPair.get());

  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID, byKey,
                         *missingSignerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, nullptr,
                         sha256WithRSAEncryption()));
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
  ASSERT_TRUE(missingSignerKeyPair.get());
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         "missing", *missingSignerKeyPair,
                         oneDayBeforeNow, oneDayBeforeNow, nullptr,
                         sha256WithRSAEncryption()));
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
    CreateEncodedEKUExtension(OCSPSigningEKUDER, Critical::No),
    ByteString()
  };

  ScopedTestKeyPair signerKeyPair(GenerateKeyPair());
  ByteString signerDER(CreateEncodedCertificate(
                          ++rootIssuedCount, sha256WithRSAEncryption(),
                          rootName,
                          now - (10 * ONE_DAY_IN_SECONDS_AS_TIME_T),
                          now - (2 * ONE_DAY_IN_SECONDS_AS_TIME_T),
                          signerName, *signerKeyPair, extensions,
                          *rootKeyPair));
  ASSERT_FALSE(ENCODING_FAILED(signerDER));

  ByteString certs[] = { signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption(), certs));
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
    CreateEncodedEKUExtension(OCSPSigningEKUDER, Critical::No),
    ByteString()
  };

  ScopedTestKeyPair signerKeyPair(GenerateKeyPair());
  ByteString signerDER(CreateEncodedCertificate(
                         ++rootIssuedCount, sha256WithRSAEncryption(),
                         rootName,
                         now + (2 * ONE_DAY_IN_SECONDS_AS_TIME_T),
                         now + (10 * ONE_DAY_IN_SECONDS_AS_TIME_T),
                         signerName, *signerKeyPair, extensions,
                         *rootKeyPair));
  ASSERT_FALSE(ENCODING_FAILED(signerDER));

  ByteString certs[] = { signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption(), certs));
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
                         OCSPResponseContext::good, byKey,
                         sha256WithRSAEncryption(), nullptr));
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
                        OCSPResponseContext::good, byKey,
                        sha256WithRSAEncryption(), &serverAuthEKUDER));
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
                         OCSPResponseContext::good, byKey,
                         sha256WithRSAEncryption(), &serverAuthEKUDER));
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
  ASSERT_TRUE(unknownKeyPair.get());

  
  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER, Critical::No),
    ByteString()
  };
  ScopedTestKeyPair signerKeyPair(GenerateKeyPair());
  ByteString signerDER(CreateEncodedCertificate(
                         1, sha256WithRSAEncryption(), subCAName,
                         oneDayBeforeNow, oneDayAfterNow, signerName,
                         *signerKeyPair, extensions, *unknownKeyPair));
  ASSERT_FALSE(ENCODING_FAILED(signerDER));

  
  ByteString certs[] = { signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption(), certs));
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
    CreateEncodedBasicConstraints(true, 0, Critical::No),
    ByteString()
  };
  ScopedTestKeyPair subCAKeyPair(GenerateKeyPair());
  ByteString subCADER(CreateEncodedCertificate(
                        ++rootIssuedCount, sha256WithRSAEncryption(), rootName,
                        oneDayBeforeNow, oneDayAfterNow, subCAName,
                        *subCAKeyPair, subCAExtensions, *rootKeyPair));
  ASSERT_FALSE(ENCODING_FAILED(subCADER));

  
  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER, Critical::No),
    ByteString(),
  };
  ScopedTestKeyPair signerKeyPair(GenerateKeyPair());
  ByteString signerDER(CreateEncodedCertificate(
                         1, sha256WithRSAEncryption(), subCAName,
                         oneDayBeforeNow, oneDayAfterNow, signerName,
                         *signerKeyPair, extensions, *subCAKeyPair));
  ASSERT_FALSE(ENCODING_FAILED(signerDER));

  
  
  ByteString certs[] = { subCADER, signerDER, ByteString() };
  ByteString responseString(
               CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption(), certs));
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
    CreateEncodedBasicConstraints(true, 0, Critical::No),
    ByteString()
  };
  ScopedTestKeyPair subCAKeyPair(GenerateKeyPair());
  ByteString subCADER(CreateEncodedCertificate(++rootIssuedCount,
                                               sha256WithRSAEncryption(),
                                               rootName,
                                               oneDayBeforeNow, oneDayAfterNow,
                                               subCAName, *subCAKeyPair,
                                               subCAExtensions, *rootKeyPair));
  ASSERT_FALSE(ENCODING_FAILED(subCADER));

  
  const ByteString extensions[] = {
    CreateEncodedEKUExtension(OCSPSigningEKUDER, Critical::No),
    ByteString()
  };
  ScopedTestKeyPair signerKeyPair(GenerateKeyPair());
  ByteString signerDER(CreateEncodedCertificate(
                         1, sha256WithRSAEncryption(), subCAName,
                         oneDayBeforeNow, oneDayAfterNow, signerName,
                         *signerKeyPair, extensions, *subCAKeyPair));
  ASSERT_FALSE(ENCODING_FAILED(signerDER));

  
  
  ByteString certs[] = { signerDER, subCADER, ByteString() };
  ByteString responseString(
                 CreateEncodedOCSPSuccessfulResponse(
                         OCSPResponseContext::good, *endEntityCertID,
                         signerName, *signerKeyPair, oneDayBeforeNow,
                         oneDayBeforeNow, &oneDayAfterNow,
                         sha256WithRSAEncryption(), certs));
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
       good_unsupportedSignatureAlgorithmOnResponder)
{
  
  
  
  ByteString responseString(
               CreateEncodedIndirectOCSPSuccessfulResponse(
                         "good_indirect_unsupportedSignatureAlgorithm",
                         OCSPResponseContext::good, byKey,
                         md5WithRSAEncryption()));
  Input response;
  ASSERT_EQ(Success,
            response.Init(responseString.data(), responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      response, expired));
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
          byKey, sha256WithRSAEncryption(), &OCSPSigningEKUDER,
          &signerCertDER);
    if (ENCODING_FAILED(responseString)) {
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

  class TrustDomain final : public OCSPTestTrustDomain
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
    Result GetCertTrust(EndEntityOrCA endEntityOrCA, const CertPolicyId&,
                        Input candidateCert,  TrustLevel& trustLevel)
                        override
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
  Input responseInput;
  ASSERT_EQ(Success,
            responseInput.Init(responseString.data(),
                               responseString.length()));
  bool expired;
  ASSERT_EQ(Result::ERROR_OCSP_INVALID_SIGNING_CERT,
            VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, Now(),
                                      END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                      responseInput, expired));
  ASSERT_FALSE(expired);
}
