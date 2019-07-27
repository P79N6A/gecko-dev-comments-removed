























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "pkixder.h"
#include "pkixtestutil.h"
#include "secerr.h"

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

  virtual Result FindIssuer(Input, IssuerChecker&, PRTime)
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  virtual Result CheckRevocation(EndEntityOrCA, const CertID&, PRTime,
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
    return ::mozilla::pkix::DigestBuf(item, digestBuf, digestBufLen);
  }

  virtual Result CheckPublicKey(Input subjectPublicKeyInfo)
  {
    return ::mozilla::pkix::CheckPublicKey(subjectPublicKeyInfo);
  }
};

class pkixocsp_CreateEncodedOCSPRequest : public NSSTest
{
protected:
  
  SECItem* unsupportedLongSerialNumber;
  SECItem* longestRequiredSerialNumber;

  void SetUp()
  {
    static const uint8_t UNSUPPORTED_LEN = 128; 
    
    unsupportedLongSerialNumber = SECITEM_AllocItem(arena.get(), nullptr,
                                                    1 + 2 + UNSUPPORTED_LEN);
    memset(unsupportedLongSerialNumber->data, 0,
           unsupportedLongSerialNumber->len);
    unsupportedLongSerialNumber->data[0] = der::INTEGER;
    
    
    unsupportedLongSerialNumber->data[1] = 0x80 + 1;
    unsupportedLongSerialNumber->data[2] = UNSUPPORTED_LEN;
    unsupportedLongSerialNumber->data[3] = 0x01; 

    static const uint8_t LONGEST_REQUIRED_LEN = 20;
    
    longestRequiredSerialNumber = SECITEM_AllocItem(arena.get(), nullptr,
                                    1 + 1 + LONGEST_REQUIRED_LEN);
    memset(longestRequiredSerialNumber->data, 0,
           longestRequiredSerialNumber->len);
    longestRequiredSerialNumber->data[0] = der::INTEGER;
    longestRequiredSerialNumber->data[1] = LONGEST_REQUIRED_LEN;
    longestRequiredSerialNumber->data[2] = 0x01; 
  }

  
  SECStatus MakeIssuerCertIDComponents(const char* issuerASCII,
                                        Input& issuerDER,
                                        Input& issuerSPKI)
  {
    const SECItem* issuerDERSECItem = ASCIIToDERName(arena.get(), issuerASCII);
    if (!issuerDERSECItem) {
      return SECFailure;
    }
    if (issuerDER.Init(issuerDERSECItem->data, issuerDERSECItem->len)
          != Success) {
      return SECFailure;
    }

    ScopedSECKEYPublicKey issuerPublicKey;
    ScopedSECKEYPrivateKey issuerPrivateKey;
    if (GenerateKeyPair(issuerPublicKey, issuerPrivateKey) != SECSuccess) {
      return SECFailure;
    }
    ScopedSECItem issuerSPKIOriginal(
      SECKEY_EncodeDERSubjectPublicKeyInfo(issuerPublicKey.get()));
    if (!issuerSPKIOriginal) {
      return SECFailure;
    }
    SECItem issuerSPKICopy;
    if (SECITEM_CopyItem(arena.get(), &issuerSPKICopy,
                         issuerSPKIOriginal.get()) != SECSuccess) {
      return SECFailure;
    }
    if (issuerSPKI.Init(issuerSPKICopy.data, issuerSPKICopy.len) != Success) {
      return SECFailure;
    }

    return SECSuccess;
  }

  CreateEncodedOCSPRequestTrustDomain trustDomain;
};



TEST_F(pkixocsp_CreateEncodedOCSPRequest, ChildCertLongSerialNumberTest)
{
  Input issuerDER;
  Input issuerSPKI;
  ASSERT_EQ(SECSuccess,
            MakeIssuerCertIDComponents("CN=CA", issuerDER, issuerSPKI));
  Input serialNumber;
  ASSERT_EQ(Success, serialNumber.Init(unsupportedLongSerialNumber->data,
                                       unsupportedLongSerialNumber->len));
  uint8_t ocspRequest[OCSP_REQUEST_MAX_LENGTH];
  size_t ocspRequestLength;
  ASSERT_EQ(Result::ERROR_BAD_DER,
            CreateEncodedOCSPRequest(trustDomain,
                                     CertID(issuerDER, issuerSPKI,
                                            serialNumber),
                                     ocspRequest, ocspRequestLength));
}



TEST_F(pkixocsp_CreateEncodedOCSPRequest, LongestSupportedSerialNumberTest)
{
  Input issuerDER;
  Input issuerSPKI;
  ASSERT_EQ(SECSuccess,
            MakeIssuerCertIDComponents("CN=CA", issuerDER, issuerSPKI));
  Input serialNumber;
  ASSERT_EQ(Success, serialNumber.Init(longestRequiredSerialNumber->data,
                                       longestRequiredSerialNumber->len));
  uint8_t ocspRequest[OCSP_REQUEST_MAX_LENGTH];
  size_t ocspRequestLength;
  ASSERT_EQ(Success,
            CreateEncodedOCSPRequest(trustDomain,
                                     CertID(issuerDER, issuerSPKI,
                                            serialNumber),
                                     ocspRequest, ocspRequestLength));
}
