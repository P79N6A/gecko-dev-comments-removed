























#include "nss.h"
#include "nssgtest.h"
#include "pkix/pkix.h"
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

  virtual SECStatus GetCertTrust(EndEntityOrCA endEntityOrCA,
                                 const CertPolicyId&,
                                 const SECItem& candidateCert,
                          TrustLevel* trustLevel)
  {
    EXPECT_EQ(endEntityOrCA, EndEntityOrCA::MustBeEndEntity);
    EXPECT_TRUE(trustLevel);
    *trustLevel = TrustLevel::InheritsTrust;
    return SECSuccess;
  }

  virtual SECStatus FindIssuer(const SECItem&, IssuerChecker&, PRTime)
  {
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  virtual SECStatus CheckRevocation(EndEntityOrCA endEntityOrCA, const CertID&,
                                    PRTime time,
                                     const SECItem*,
                                     const SECItem*)
  {
    
    
    
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  virtual SECStatus IsChainValid(const DERArray&)
  {
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  virtual SECStatus VerifySignedData(const SignedDataWithSignature& signedData,
                                     const SECItem& subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::VerifySignedData(signedData, subjectPublicKeyInfo,
                                             nullptr);
  }

  virtual SECStatus DigestBuf(const SECItem& item,  uint8_t* digestBuf,
                              size_t digestBufLen)
  {
    return ::mozilla::pkix::DigestBuf(item, digestBuf, digestBufLen);
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

    const SECItem* rootNameDER = ASCIIToDERName(arena.get(), rootName);
    if (!rootNameDER) {
      PR_Abort();
    }
    const SECItem*
      endEntitySerialNumber(CreateEncodedSerialNumber(arena.get(),
                                                      ++rootIssuedCount));
    if (!endEntitySerialNumber) {
      PR_Abort();
    }
    endEntityCertID = new (std::nothrow) CertID(*rootNameDER, *rootSPKI,
                                                *endEntitySerialNumber);
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
  PRErrorCode expectedError;
};

static const WithoutResponseBytes WITHOUT_RESPONSEBYTES[] = {
  { OCSPResponseContext::successful, SEC_ERROR_OCSP_MALFORMED_RESPONSE },
  { OCSPResponseContext::malformedRequest, SEC_ERROR_OCSP_MALFORMED_REQUEST },
  { OCSPResponseContext::internalError, SEC_ERROR_OCSP_SERVER_ERROR },
  { OCSPResponseContext::tryLater, SEC_ERROR_OCSP_TRY_SERVER_LATER },
  { 4, SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS },
  { OCSPResponseContext::sigRequired, SEC_ERROR_OCSP_REQUEST_NEEDS_SIG },
  { OCSPResponseContext::unauthorized, SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST },
  { OCSPResponseContext::unauthorized + 1,
    SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS
  },
};

class pkixocsp_VerifyEncodedResponse_WithoutResponseBytes
  : public pkixocsp_VerifyEncodedResponse
  , public ::testing::WithParamInterface<WithoutResponseBytes>
{
protected:
  SECItem* CreateEncodedOCSPErrorResponse(uint8_t status)
  {
    static const SECItem EMPTY = { siBuffer, nullptr, 0 };
    OCSPResponseContext context(arena.get(),
                                CertID(EMPTY, EMPTY, EMPTY),
                                oneDayBeforeNow);
    context.responseStatus = status;
    context.skipResponseBytes = true;
    return CreateEncodedOCSPResponse(context);
  }
};

TEST_P(pkixocsp_VerifyEncodedResponse_WithoutResponseBytes, CorrectErrorCode)
{
  SECItem* response(CreateEncodedOCSPErrorResponse(
                      GetParam().responseStatus));
  ASSERT_TRUE(response);
  bool expired;
  ASSERT_SECFailure(GetParam().expectedError,
                    VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                              END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                              *response, expired));
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

  SECItem* CreateEncodedOCSPSuccessfulResponse(
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
      if (!context.signerNameDER) {
        return nullptr;
      }
    }
    context.signerPrivateKey = SECKEY_CopyPrivateKey(signerPrivateKey.get());
    if (!context.signerPrivateKey) {
      return nullptr;
    }
    context.responseStatus = OCSPResponseContext::successful;
    context.producedAt = producedAt;
    context.certs = certs;

    context.certIDHashAlg = SEC_OID_SHA1;
    context.certStatus = certStatus;
    context.thisUpdate = thisUpdate;
    context.nextUpdate = nextUpdate ? *nextUpdate : 0;
    context.includeNextUpdate = nextUpdate != nullptr;

    return CreateEncodedOCSPResponse(context);
  }
};

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byKey)
{
  SECItem* response(CreateEncodedOCSPSuccessfulResponse(
                      OCSPResponseContext::good, *endEntityCertID, byKey,
                      rootPrivateKey, oneDayBeforeNow, oneDayBeforeNow,
                      &oneDayAfterNow));
  ASSERT_TRUE(response);
  bool expired;
  ASSERT_SECSuccess(VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                              END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                              *response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byName)
{
  SECItem* response(CreateEncodedOCSPSuccessfulResponse(
                      OCSPResponseContext::good, *endEntityCertID, rootName,
                      rootPrivateKey, oneDayBeforeNow, oneDayBeforeNow,
                      &oneDayAfterNow));
  ASSERT_TRUE(response);
  bool expired;
  ASSERT_SECSuccess(VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                              END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                              *response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, good_byKey_without_nextUpdate)
{
  SECItem* response(CreateEncodedOCSPSuccessfulResponse(
                      OCSPResponseContext::good, *endEntityCertID, byKey,
                      rootPrivateKey, oneDayBeforeNow, oneDayBeforeNow,
                      nullptr));
  ASSERT_TRUE(response);
  bool expired;
  ASSERT_SECSuccess(VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                              END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                              *response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, revoked)
{
  SECItem* response(CreateEncodedOCSPSuccessfulResponse(
                      OCSPResponseContext::revoked, *endEntityCertID, byKey,
                      rootPrivateKey, oneDayBeforeNow, oneDayBeforeNow,
                      &oneDayAfterNow));
  ASSERT_TRUE(response);
  bool expired;
  ASSERT_SECFailure(SEC_ERROR_REVOKED_CERTIFICATE,
                    VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                              END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                              *response, expired));
  ASSERT_FALSE(expired);
}

TEST_F(pkixocsp_VerifyEncodedResponse_successful, unknown)
{
  SECItem* response(CreateEncodedOCSPSuccessfulResponse(
                      OCSPResponseContext::unknown, *endEntityCertID, byKey,
                      rootPrivateKey, oneDayBeforeNow, oneDayBeforeNow,
                      &oneDayAfterNow));
  ASSERT_TRUE(response);
  bool expired;
  ASSERT_SECFailure(SEC_ERROR_OCSP_UNKNOWN_CERT,
                    VerifyEncodedOCSPResponse(trustDomain, *endEntityCertID, now,
                                              END_ENTITY_MAX_LIFETIME_IN_DAYS,
                                              *response, expired));
  ASSERT_FALSE(expired);
}
