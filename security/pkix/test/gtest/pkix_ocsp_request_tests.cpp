























#include "nssgtest.h"
#include "pkix/pkix.h"
#include "pkixder.h"
#include "prerror.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

class pkix_ocsp_request_tests : public NSSTest
{
protected:
  
  SECItem* unsupportedLongSerialNumber;
  SECItem* shortSerialNumber;
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

    
    shortSerialNumber = SECITEM_AllocItem(arena.get(), nullptr, 3);
    shortSerialNumber->data[0] = der::INTEGER;
    shortSerialNumber->data[1] = 0x01; 
    shortSerialNumber->data[2] = 0x01; 

    static const uint8_t LONGEST_REQUIRED_LEN = 20;
    
    longestRequiredSerialNumber = SECITEM_AllocItem(arena.get(), nullptr,
                                    1 + 1 + LONGEST_REQUIRED_LEN);
    memset(longestRequiredSerialNumber->data, 0,
           longestRequiredSerialNumber->len);
    longestRequiredSerialNumber->data[0] = der::INTEGER;
    longestRequiredSerialNumber->data[1] = LONGEST_REQUIRED_LEN;
    longestRequiredSerialNumber->data[2] = 0x01; 
  }

  void MakeTwoCerts(const char* issuerCN, SECItem* issuerSerial,
                     ScopedCERTCertificate& issuer,
                    const char* childCN, SECItem* childSerial,
                     ScopedCERTCertificate& child)
  {
    const SECItem* issuerNameDer = ASCIIToDERName(arena.get(), issuerCN);
    ASSERT_TRUE(issuerNameDer);
    ScopedSECKEYPrivateKey issuerKey;
    SECItem* issuerCertDer(CreateEncodedCertificate(arena.get(), v3,
                             SEC_OID_SHA256, issuerSerial, issuerNameDer,
                             oneDayBeforeNow, oneDayAfterNow, issuerNameDer,
                             nullptr, nullptr, SEC_OID_SHA256, issuerKey));
    ASSERT_TRUE(issuerCertDer);
    const SECItem* childNameDer = ASCIIToDERName(arena.get(), childCN);
    ASSERT_TRUE(childNameDer);
    ScopedSECKEYPrivateKey childKey;
    SECItem* childDer(CreateEncodedCertificate(arena.get(), v3,
                        SEC_OID_SHA256, childSerial, issuerNameDer,
                        oneDayBeforeNow, oneDayAfterNow, childNameDer, nullptr,
                        issuerKey.get(), SEC_OID_SHA256, childKey));
    ASSERT_TRUE(childDer);
    issuer = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), issuerCertDer,
                                     nullptr, false, true);
    ASSERT_TRUE(issuer);
    child = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), childDer, nullptr,
                                    false, true);
    ASSERT_TRUE(child);
  }

};



TEST_F(pkix_ocsp_request_tests, IssuerCertLongSerialNumberTest)
{
  const char* issuerCN = "CN=Long Serial Number CA";
  const char* childCN = "CN=Short Serial Number EE";
  ScopedCERTCertificate issuer;
  ScopedCERTCertificate child;
  {
    SCOPED_TRACE("IssuerCertLongSerialNumberTest");
    MakeTwoCerts(issuerCN, unsupportedLongSerialNumber, issuer,
                 childCN, shortSerialNumber, child);
  }
  ASSERT_TRUE(issuer);
  ASSERT_TRUE(child);
  ASSERT_TRUE(CreateEncodedOCSPRequest(arena.get(), child.get(),
                                       issuer.get()));
  ASSERT_EQ(0, PR_GetError());
}



TEST_F(pkix_ocsp_request_tests, ChildCertLongSerialNumberTest)
{
  const char* issuerCN = "CN=Short Serial Number CA";
  const char* childCN = "CN=Long Serial Number EE";
  ScopedCERTCertificate issuer;
  ScopedCERTCertificate child;
  {
    SCOPED_TRACE("ChildCertLongSerialNumberTest");
    MakeTwoCerts(issuerCN, shortSerialNumber, issuer,
                 childCN, unsupportedLongSerialNumber, child);
  }
  ASSERT_TRUE(issuer);
  ASSERT_TRUE(child);
  ASSERT_FALSE(CreateEncodedOCSPRequest(arena.get(), child.get(),
                                        issuer.get()));
  ASSERT_EQ(SEC_ERROR_BAD_DATA, PR_GetError());
}



TEST_F(pkix_ocsp_request_tests, LongestSupportedSerialNumberTest)
{
  const char* issuerCN = "CN=Short Serial Number CA";
  const char* childCN = "CN=Longest Serial Number Supported EE";
  ScopedCERTCertificate issuer;
  ScopedCERTCertificate child;
  {
    SCOPED_TRACE("LongestSupportedSerialNumberTest");
    MakeTwoCerts(issuerCN, shortSerialNumber, issuer,
                 childCN, longestRequiredSerialNumber, child);
  }
  ASSERT_TRUE(issuer);
  ASSERT_TRUE(child);
  ASSERT_TRUE(CreateEncodedOCSPRequest(arena.get(), child.get(),
                                       issuer.get()));
  ASSERT_EQ(0, PR_GetError());
}
