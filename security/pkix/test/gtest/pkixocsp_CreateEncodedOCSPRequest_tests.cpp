























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkixder.h"
#include "prerror.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

class CreateEncodedOCSPRequestTrustDomain : public TrustDomain
{
private:
  virtual SECStatus GetCertTrust(EndEntityOrCA, const CertPolicyId&,
                                 const SECItem&,  TrustLevel*)
  {
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  virtual SECStatus FindIssuer(const SECItem&, IssuerChecker&, PRTime)
  {
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  virtual SECStatus CheckRevocation(EndEntityOrCA, const CertID&, PRTime,
                                    const SECItem*, const SECItem*)
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

  virtual SECStatus VerifySignedData(const SignedDataWithSignature&,
                                     const SECItem&)
  {
    ADD_FAILURE();
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  virtual SECStatus DigestBuf(const SECItem& item,  uint8_t *digestBuf,
                              size_t digestBufLen)
  {
    return ::mozilla::pkix::DigestBuf(item, digestBuf, digestBufLen);
  }

  virtual SECStatus CheckPublicKey(const SECItem& subjectPublicKeyInfo)
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
                                        const SECItem*& issuerDER,
                                        ScopedSECItem& issuerSPKI)
  {
    issuerDER = ASCIIToDERName(arena.get(), issuerASCII);
    if (!issuerDER) {
      return SECFailure;
    }
    ScopedSECKEYPublicKey issuerPublicKey;
    ScopedSECKEYPrivateKey issuerPrivateKey;
    if (GenerateKeyPair(issuerPublicKey, issuerPrivateKey) != SECSuccess) {
      return SECFailure;
    }
    issuerSPKI = SECKEY_EncodeDERSubjectPublicKeyInfo(issuerPublicKey.get());
    if (!issuerSPKI) {
      return SECFailure;
    }

    return SECSuccess;
  }

  CreateEncodedOCSPRequestTrustDomain trustDomain;
};



TEST_F(pkixocsp_CreateEncodedOCSPRequest, ChildCertLongSerialNumberTest)
{
  const SECItem* issuerDER;
  ScopedSECItem issuerSPKI;
  ASSERT_EQ(SECSuccess,
            MakeIssuerCertIDComponents("CN=CA", issuerDER, issuerSPKI));
  ASSERT_FALSE(CreateEncodedOCSPRequest(trustDomain, arena.get(),
                                        CertID(*issuerDER, *issuerSPKI,
                                               *unsupportedLongSerialNumber)));
  ASSERT_EQ(SEC_ERROR_BAD_DATA, PR_GetError());
}



TEST_F(pkixocsp_CreateEncodedOCSPRequest, LongestSupportedSerialNumberTest)
{
  const SECItem* issuerDER;
  ScopedSECItem issuerSPKI;
  ASSERT_EQ(SECSuccess,
            MakeIssuerCertIDComponents("CN=CA", issuerDER, issuerSPKI));
  ASSERT_TRUE(CreateEncodedOCSPRequest(trustDomain, arena.get(),
                                        CertID(*issuerDER, *issuerSPKI,
                                               *longestRequiredSerialNumber)));
}
