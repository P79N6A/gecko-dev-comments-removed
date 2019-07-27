























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkixder.h"
#include "pkixtestutil.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

class CreateEncodedOCSPRequestTrustDomain : public TrustDomain
{
private:
  virtual Result GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                              Input,  TrustLevel&)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result FindIssuer(Input, IssuerChecker&, Time)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                                 const Input*, const Input*)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result IsChainValid(const DERArray&)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result VerifySignedData(const SignedDataWithSignature&,
                                  Input)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result DigestBuf(Input item,  uint8_t *digestBuf,
                           size_t digestBufLen)
  {
    return TestDigestBuf(item, digestBuf, digestBufLen);
  }

  virtual Result CheckPublicKey(Input subjectPublicKeyInfo)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }
};

class pkixocsp_CreateEncodedOCSPRequest : public NSSTest
{
protected:
  void MakeIssuerCertIDComponents(const char* issuerASCII,
                                   ByteString& issuerDER,
                                   ByteString& issuerSPKI)
  {
    issuerDER = CNToDERName(issuerASCII);
    ASSERT_NE(ENCODING_FAILED, issuerDER);

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
