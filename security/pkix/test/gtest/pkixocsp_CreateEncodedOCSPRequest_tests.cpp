























#include "pkixgtest.h"
#include "pkix/pkix.h"
#include "pkixder.h"
#include "pkixtestutil.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

class CreateEncodedOCSPRequestTrustDomain final : public TrustDomain
{
private:
  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&, Input,
                       TrustLevel&) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  Result FindIssuer(Input, IssuerChecker&, Time) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  Result CheckRevocation(EndEntityOrCA, const CertID&, Time, const Input*,
                         const Input*) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  Result VerifySignedData(const SignedDataWithSignature&, Input) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  Result DigestBuf(Input item,  uint8_t *digestBuf, size_t digestBufLen)
                   override
  {
    return TestDigestBuf(item, digestBuf, digestBufLen);
  }

  Result CheckPublicKey(Input) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }
};

class pkixocsp_CreateEncodedOCSPRequest : public ::testing::Test
{
protected:
  void MakeIssuerCertIDComponents(const char* issuerASCII,
                                   ByteString& issuerDER,
                                   ByteString& issuerSPKI)
  {
    issuerDER = CNToDERName(issuerASCII);
    ASSERT_FALSE(ENCODING_FAILED(issuerDER));

    ScopedTestKeyPair keyPair(GenerateKeyPair());
    ASSERT_TRUE(keyPair);
    issuerSPKI = keyPair->subjectPublicKeyInfo;
  }

  CreateEncodedOCSPRequestTrustDomain trustDomain;
};



TEST_F(pkixocsp_CreateEncodedOCSPRequest, ChildCertLongSerialNumberTest)
{
  static const uint8_t UNSUPPORTED_LEN = 128; 

  ByteString serialNumberString;
  
  
  
  serialNumberString.push_back(0x80 + 1);
  serialNumberString.push_back(UNSUPPORTED_LEN);
  
  serialNumberString.push_back(0x01);
  for (size_t i = 1; i < UNSUPPORTED_LEN; ++i) {
    serialNumberString.push_back(0x00);
  }

  ByteString issuerDER;
  ByteString issuerSPKI;
  ASSERT_NO_FATAL_FAILURE(MakeIssuerCertIDComponents("CA", issuerDER,
                                                     issuerSPKI));

  Input issuer;
  ASSERT_EQ(Success, issuer.Init(issuerDER.data(), issuerDER.length()));

  Input spki;
  ASSERT_EQ(Success, spki.Init(issuerSPKI.data(), issuerSPKI.length()));

  Input serialNumber;
  ASSERT_EQ(Success, serialNumber.Init(serialNumberString.data(),
                                       serialNumberString.length()));

  uint8_t ocspRequest[OCSP_REQUEST_MAX_LENGTH];
  size_t ocspRequestLength;
  ASSERT_EQ(Result::ERROR_BAD_DER,
            CreateEncodedOCSPRequest(trustDomain,
                                     CertID(issuer, spki, serialNumber),
                                     ocspRequest, ocspRequestLength));
}



TEST_F(pkixocsp_CreateEncodedOCSPRequest, LongestSupportedSerialNumberTest)
{
  static const uint8_t LONGEST_REQUIRED_LEN = 20;

  ByteString serialNumberString;
  
  serialNumberString.push_back(der::INTEGER);
  serialNumberString.push_back(LONGEST_REQUIRED_LEN);
  serialNumberString.push_back(0x01);
  
  for (size_t i = 1; i < LONGEST_REQUIRED_LEN; ++i) {
    serialNumberString.push_back(0x00);
  }

  ByteString issuerDER;
  ByteString issuerSPKI;
  ASSERT_NO_FATAL_FAILURE(MakeIssuerCertIDComponents("CA", issuerDER,
                                                     issuerSPKI));

  Input issuer;
  ASSERT_EQ(Success, issuer.Init(issuerDER.data(), issuerDER.length()));

  Input spki;
  ASSERT_EQ(Success, spki.Init(issuerSPKI.data(), issuerSPKI.length()));

  Input serialNumber;
  ASSERT_EQ(Success, serialNumber.Init(serialNumberString.data(),
                                       serialNumberString.length()));

  uint8_t ocspRequest[OCSP_REQUEST_MAX_LENGTH];
  size_t ocspRequestLength;
  ASSERT_EQ(Success,
            CreateEncodedOCSPRequest(trustDomain,
                                     CertID(issuer, spki, serialNumber),
                                     ocspRequest, ocspRequestLength));
}
