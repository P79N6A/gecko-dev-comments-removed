





#include "CertVerifier.h"
#include "OCSPCache.h"
#include "gtest/gtest.h"
#include "nss.h"
#include "pkix/pkixtypes.h"
#include "pkixtestutil.h"
#include "prerr.h"
#include "prprf.h"
#include "secerr.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;
using namespace mozilla::psm;

const int MaxCacheEntries = 1024;

class OCSPCacheTest : public ::testing::Test
{
  protected:
    static void SetUpTestCase()
    {
      NSS_NoDB_Init(nullptr);
      mozilla::psm::InitCertVerifierLog();
    }

    mozilla::psm::OCSPCache cache;
};

static void
PutAndGet(OCSPCache& cache, const CertID& certID, Result result,
          PRTime time)
{
  
  
  
  
  ASSERT_TRUE(time >= 10);
  Result rv = cache.Put(certID, result, time - 10, time);
  ASSERT_TRUE(rv == Success);
  Result resultOut;
  PRTime timeOut;
  ASSERT_TRUE(cache.Get(certID, resultOut, timeOut));
  ASSERT_EQ(result, resultOut);
  ASSERT_EQ(time, timeOut);
}

TestInputBuffer fakeIssuer1("CN=issuer1");
TestInputBuffer fakeKey000("key000");
TestInputBuffer fakeKey001("key001");
TestInputBuffer fakeSerial0000("0000");

TEST_F(OCSPCacheTest, TestPutAndGet)
{
  TestInputBuffer fakeSerial000("000");
  TestInputBuffer fakeSerial001("001");

  SCOPED_TRACE("");
  PutAndGet(cache, CertID(fakeIssuer1, fakeKey000, fakeSerial001),
            Success, PR_Now());
  Result resultOut;
  PRTime timeOut;
  ASSERT_FALSE(cache.Get(CertID(fakeIssuer1, fakeKey001, fakeSerial000),
                         resultOut, timeOut));
}

TEST_F(OCSPCacheTest, TestVariousGets)
{
  SCOPED_TRACE("");
  PRTime timeIn = PR_Now();
  for (int i = 0; i < MaxCacheEntries; i++) {
    uint8_t serialBuf[8];
    PR_snprintf(reinterpret_cast<char*>(serialBuf), sizeof(serialBuf), "%04d", i);
    InputBuffer fakeSerial;
    ASSERT_EQ(Success, fakeSerial.Init(serialBuf, 4));
    PutAndGet(cache, CertID(fakeIssuer1, fakeKey000, fakeSerial),
              Success, timeIn + i);
  }

  Result resultOut;
  PRTime timeOut;

  
  CertID cert0000(fakeIssuer1, fakeKey000, fakeSerial0000);
  ASSERT_TRUE(cache.Get(cert0000, resultOut, timeOut));
  ASSERT_EQ(Success, resultOut);
  ASSERT_EQ(timeIn, timeOut);
  
  ASSERT_TRUE(cache.Get(cert0000, resultOut, timeOut));
  ASSERT_EQ(Success, resultOut);
  ASSERT_EQ(timeIn, timeOut);

  
  static const TestInputBuffer fakeSerial0512("0512");
  CertID cert0512(fakeIssuer1, fakeKey000, fakeSerial0512);
  ASSERT_TRUE(cache.Get(cert0512, resultOut, timeOut));
  ASSERT_EQ(Success, resultOut);
  ASSERT_EQ(timeIn + 512, timeOut);
  ASSERT_TRUE(cache.Get(cert0512, resultOut, timeOut));
  ASSERT_EQ(Success, resultOut);
  ASSERT_EQ(timeIn + 512, timeOut);

  
  static const TestInputBuffer fakeSerial1111("1111");
  ASSERT_FALSE(cache.Get(CertID(fakeIssuer1, fakeKey000, fakeSerial1111),
                         resultOut, timeOut));
}

TEST_F(OCSPCacheTest, TestEviction)
{
  SCOPED_TRACE("");
  PRTime timeIn = PR_Now();

  
  
  for (int i = 0; i < MaxCacheEntries + 1; i++) {
    uint8_t serialBuf[8];
    PR_snprintf(reinterpret_cast<char*>(serialBuf), sizeof(serialBuf), "%04d", i);
    InputBuffer fakeSerial;
    ASSERT_EQ(Success, fakeSerial.Init(serialBuf, 4));
    PutAndGet(cache, CertID(fakeIssuer1, fakeKey000, fakeSerial),
              Success, timeIn + i);
  }

  Result resultOut;
  PRTime timeOut;
  ASSERT_FALSE(cache.Get(CertID(fakeIssuer1, fakeKey001, fakeSerial0000),
                         resultOut, timeOut));
}

TEST_F(OCSPCacheTest, TestNoEvictionForRevokedResponses)
{
  SCOPED_TRACE("");
  PRTime timeIn = PR_Now();
  CertID notEvicted(fakeIssuer1, fakeKey000, fakeSerial0000);
  PutAndGet(cache, notEvicted, Result::ERROR_REVOKED_CERTIFICATE, timeIn);
  
  
  for (int i = 1; i < MaxCacheEntries + 1; i++) {
    uint8_t serialBuf[8];
    PR_snprintf(reinterpret_cast<char*>(serialBuf), sizeof(serialBuf), "%04d", i);
    InputBuffer fakeSerial;
    ASSERT_EQ(Success, fakeSerial.Init(serialBuf, 4));
    PutAndGet(cache, CertID(fakeIssuer1, fakeKey000, fakeSerial),
              Success, timeIn + i);
  }
  Result resultOut;
  PRTime timeOut;
  ASSERT_TRUE(cache.Get(notEvicted, resultOut, timeOut));
  ASSERT_EQ(Result::ERROR_REVOKED_CERTIFICATE, resultOut);
  ASSERT_EQ(timeIn, timeOut);

  TestInputBuffer fakeSerial0001("0001");
  CertID evicted(fakeIssuer1, fakeKey000, fakeSerial0001);
  ASSERT_FALSE(cache.Get(evicted, resultOut, timeOut));
}

TEST_F(OCSPCacheTest, TestEverythingIsRevoked)
{
  SCOPED_TRACE("");
  PRTime timeIn = PR_Now();
  
  for (int i = 0; i < MaxCacheEntries; i++) {
    uint8_t serialBuf[8];
    PR_snprintf(reinterpret_cast<char*>(serialBuf), sizeof(serialBuf), "%04d", i);
    InputBuffer fakeSerial;
    ASSERT_EQ(Success, fakeSerial.Init(serialBuf, 4));
    PutAndGet(cache, CertID(fakeIssuer1, fakeKey000, fakeSerial),
              Result::ERROR_REVOKED_CERTIFICATE, timeIn + i);
  }
  static const TestInputBuffer fakeSerial1025("1025");
  CertID good(fakeIssuer1, fakeKey000, fakeSerial1025);
  
  
  Result result = cache.Put(good, Success, timeIn + 1025 - 50, timeIn + 1025);
  ASSERT_EQ(Success, result);
  Result resultOut;
  PRTime timeOut;
  ASSERT_FALSE(cache.Get(good, resultOut, timeOut));

  static const TestInputBuffer fakeSerial1026("1026");
  CertID revoked(fakeIssuer1, fakeKey000, fakeSerial1026);
  
  result = cache.Put(revoked, Result::ERROR_REVOKED_CERTIFICATE,
                     timeIn + 1026 - 50, timeIn + 1026);
  ASSERT_EQ(Result::ERROR_REVOKED_CERTIFICATE, result);
}

TEST_F(OCSPCacheTest, VariousIssuers)
{
  SCOPED_TRACE("");
  static const TestInputBuffer fakeIssuer2("CN=issuer2");
  static const TestInputBuffer fakeSerial001("001");
  PRTime timeIn = PR_Now();
  CertID subject(fakeIssuer1, fakeKey000, fakeSerial001);
  PutAndGet(cache, subject, Success, timeIn);
  Result resultOut;
  PRTime timeOut;
  ASSERT_TRUE(cache.Get(subject, resultOut, timeOut));
  ASSERT_EQ(Success, resultOut);
  ASSERT_EQ(timeIn, timeOut);
  
  ASSERT_FALSE(cache.Get(CertID(fakeIssuer2, fakeKey000, fakeSerial001),
                         resultOut, timeOut));
  
  ASSERT_FALSE(cache.Get(CertID(fakeIssuer1, fakeKey001, fakeSerial001),
                         resultOut, timeOut));
}

TEST_F(OCSPCacheTest, Times)
{
  SCOPED_TRACE("");
  CertID certID(fakeIssuer1, fakeKey000, fakeSerial0000);
  PutAndGet(cache, certID, Result::ERROR_OCSP_UNKNOWN_CERT, 100);
  PutAndGet(cache, certID, Success, 200);
  
  ASSERT_EQ(Success,
            cache.Put(certID, Result::ERROR_OCSP_UNKNOWN_CERT, 100, 100));
  Result resultOut;
  PRTime timeOut;
  ASSERT_TRUE(cache.Get(certID, resultOut, timeOut));
  
  ASSERT_EQ(Success, resultOut);
  ASSERT_EQ(200, timeOut);

  
  PutAndGet(cache, certID, Result::ERROR_REVOKED_CERTIFICATE, 50);
}

TEST_F(OCSPCacheTest, NetworkFailure)
{
  SCOPED_TRACE("");
  CertID certID(fakeIssuer1, fakeKey000, fakeSerial0000);
  PutAndGet(cache, certID, Result::ERROR_CONNECT_REFUSED, 100);
  PutAndGet(cache, certID, Success, 200);
  
  ASSERT_EQ(Success,
            cache.Put(certID, Result::ERROR_CONNECT_REFUSED, 300, 350));
  Result resultOut;
  PRTime timeOut;
  ASSERT_TRUE(cache.Get(certID, resultOut, timeOut));
  ASSERT_EQ(Success, resultOut);
  ASSERT_EQ(200, timeOut);

  PutAndGet(cache, certID, Result::ERROR_OCSP_UNKNOWN_CERT, 400);
  
  ASSERT_EQ(Success,
            cache.Put(certID, Result::ERROR_CONNECT_REFUSED, 500, 550));
  ASSERT_TRUE(cache.Get(certID, resultOut, timeOut));
  ASSERT_EQ(Result::ERROR_OCSP_UNKNOWN_CERT, resultOut);
  ASSERT_EQ(400, timeOut);

  PutAndGet(cache, certID, Result::ERROR_REVOKED_CERTIFICATE, 600);
  
  ASSERT_EQ(Success,
            cache.Put(certID, Result::ERROR_CONNECT_REFUSED, 700, 750));
  ASSERT_TRUE(cache.Get(certID, resultOut, timeOut));
  ASSERT_EQ(Result::ERROR_REVOKED_CERTIFICATE, resultOut);
  ASSERT_EQ(600, timeOut);
}
