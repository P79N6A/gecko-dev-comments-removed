





#include "nsNSSIOLayer.h"
#include "sslproto.h"
#include "sslerr.h"

#include "gtest/gtest.h"

NS_NAMED_LITERAL_CSTRING(HOST, "example.org");
const int16_t PORT = 443;

class TLSIntoleranceTest : public ::testing::Test
{
protected:
  nsSSLIOLayerHelpers helpers;
};

TEST_F(TLSIntoleranceTest, Test_1_2_through_3_0)
{
  helpers.mVersionFallbackLimit = SSL_LIBRARY_VERSION_3_0;

  
  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);

    ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                    range.min, range.max, 0));
  }

  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_1, range.max);

    ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                    range.min, range.max, 0));
  }

  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_0, range.max);

    ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                    range.min, range.max, 0));
  }

  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };

    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.max);

    
    ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                     range.min, range.max, 0));
  }

  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    
    
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);
  }
}

TEST_F(TLSIntoleranceTest, Test_Disable_Fallback_With_High_Limit)
{
  
  
  helpers.mVersionFallbackLimit = SSL_LIBRARY_VERSION_TLS_1_2;
  ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                   SSL_LIBRARY_VERSION_3_0,
                                                   SSL_LIBRARY_VERSION_TLS_1_2,
                                                   0));
  ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                   SSL_LIBRARY_VERSION_3_0,
                                                   SSL_LIBRARY_VERSION_TLS_1_1,
                                                   0));
  ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                   SSL_LIBRARY_VERSION_3_0,
                                                   SSL_LIBRARY_VERSION_TLS_1_0,
                                                   0));
}

TEST_F(TLSIntoleranceTest, Test_Fallback_Limit_Default)
{
  
  ASSERT_EQ(helpers.mVersionFallbackLimit, SSL_LIBRARY_VERSION_TLS_1_0);
  ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                  SSL_LIBRARY_VERSION_3_0,
                                                  SSL_LIBRARY_VERSION_TLS_1_1,
                                                  0));
  ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                   SSL_LIBRARY_VERSION_3_0,
                                                   SSL_LIBRARY_VERSION_TLS_1_0,
                                                   0));
}

TEST_F(TLSIntoleranceTest, Test_Fallback_Limit_Below_Min)
{
  
  
  ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                  SSL_LIBRARY_VERSION_TLS_1_1,
                                                  SSL_LIBRARY_VERSION_TLS_1_2,
                                                  0));
  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_1, range.max);
  }

  ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                   SSL_LIBRARY_VERSION_TLS_1_1,
                                                   SSL_LIBRARY_VERSION_TLS_1_1,
                                                   0));
}

TEST_F(TLSIntoleranceTest, Test_Tolerant_Overrides_Intolerant_1)
{
  ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                  SSL_LIBRARY_VERSION_3_0,
                                                  SSL_LIBRARY_VERSION_TLS_1_1,
                                                  0));
  helpers.rememberTolerantAtVersion(HOST, PORT, SSL_LIBRARY_VERSION_TLS_1_1);
  SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                            SSL_LIBRARY_VERSION_TLS_1_2 };
  helpers.adjustForTLSIntolerance(HOST, PORT, range);
  ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
  ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_1, range.max);
}

TEST_F(TLSIntoleranceTest, Test_Tolerant_Overrides_Intolerant_2)
{
  ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                  SSL_LIBRARY_VERSION_3_0,
                                                  SSL_LIBRARY_VERSION_TLS_1_1,
                                                  0));
  helpers.rememberTolerantAtVersion(HOST, PORT, SSL_LIBRARY_VERSION_TLS_1_2);
  SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                            SSL_LIBRARY_VERSION_TLS_1_2 };
  helpers.adjustForTLSIntolerance(HOST, PORT, range);
  ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
  ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);
}

TEST_F(TLSIntoleranceTest, Test_Intolerant_Does_Not_Override_Tolerant)
{
  
  helpers.rememberTolerantAtVersion(HOST, PORT, SSL_LIBRARY_VERSION_TLS_1_1);
  
  ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                   SSL_LIBRARY_VERSION_3_0,
                                                   SSL_LIBRARY_VERSION_TLS_1_1,
                                                   0));
  SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                            SSL_LIBRARY_VERSION_TLS_1_2 };
  helpers.adjustForTLSIntolerance(HOST, PORT, range);
  ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
  ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);
}

TEST_F(TLSIntoleranceTest, Test_Port_Is_Relevant)
{
  helpers.rememberTolerantAtVersion(HOST, 1, SSL_LIBRARY_VERSION_TLS_1_2);
  ASSERT_FALSE(helpers.rememberIntolerantAtVersion(HOST, 1,
                                                   SSL_LIBRARY_VERSION_3_0,
                                                   SSL_LIBRARY_VERSION_TLS_1_2,
                                                   0));
  ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, 2,
                                                  SSL_LIBRARY_VERSION_3_0,
                                                  SSL_LIBRARY_VERSION_TLS_1_2,
                                                  0));

  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, 1, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);
  }

  {
    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, 2, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_1, range.max);
  }
}

TEST_F(TLSIntoleranceTest, Test_Intolerance_Reason_Initial)
{
  ASSERT_EQ(0, helpers.getIntoleranceReason(HOST, 1));

  helpers.rememberTolerantAtVersion(HOST, 2, SSL_LIBRARY_VERSION_TLS_1_2);
  ASSERT_EQ(0, helpers.getIntoleranceReason(HOST, 2));
}

TEST_F(TLSIntoleranceTest, Test_Intolerance_Reason_Stored)
{
  helpers.rememberIntolerantAtVersion(HOST, 1, SSL_LIBRARY_VERSION_3_0,
                                      SSL_LIBRARY_VERSION_TLS_1_2,
                                      SSL_ERROR_BAD_SERVER);
  ASSERT_EQ(SSL_ERROR_BAD_SERVER, helpers.getIntoleranceReason(HOST, 1));

  helpers.rememberIntolerantAtVersion(HOST, 1, SSL_LIBRARY_VERSION_3_0,
                                      SSL_LIBRARY_VERSION_TLS_1_1,
                                      SSL_ERROR_BAD_MAC_READ);
  ASSERT_EQ(SSL_ERROR_BAD_MAC_READ, helpers.getIntoleranceReason(HOST, 1));
}

TEST_F(TLSIntoleranceTest, Test_Intolerance_Reason_Cleared)
{
  ASSERT_EQ(0, helpers.getIntoleranceReason(HOST, 1));

  helpers.rememberIntolerantAtVersion(HOST, 1, SSL_LIBRARY_VERSION_3_0,
                                      SSL_LIBRARY_VERSION_TLS_1_2,
                                      SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT);
  ASSERT_EQ(SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT,
            helpers.getIntoleranceReason(HOST, 1));

  helpers.rememberTolerantAtVersion(HOST, 1, SSL_LIBRARY_VERSION_TLS_1_2);
  ASSERT_EQ(0, helpers.getIntoleranceReason(HOST, 1));
}

TEST_F(TLSIntoleranceTest, TLS_Forget_Intolerance)
{
  {
    ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                    SSL_LIBRARY_VERSION_3_0,
                                                    SSL_LIBRARY_VERSION_TLS_1_2,
                                                    0));

    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_1, range.max);
  }

  {
    helpers.forgetIntolerance(HOST, PORT);

    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);
  }
}

TEST_F(TLSIntoleranceTest, TLS_Dont_Forget_Tolerance)
{
  {
    helpers.rememberTolerantAtVersion(HOST, 1, SSL_LIBRARY_VERSION_TLS_1_1);

    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);
  }

  {
    ASSERT_TRUE(helpers.rememberIntolerantAtVersion(HOST, PORT,
                                                    SSL_LIBRARY_VERSION_3_0,
                                                    SSL_LIBRARY_VERSION_TLS_1_2,
                                                    0));

    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_1, range.max);
  }

  {
    helpers.forgetIntolerance(HOST, PORT);

    SSLVersionRange range = { SSL_LIBRARY_VERSION_3_0,
                              SSL_LIBRARY_VERSION_TLS_1_2 };
    helpers.adjustForTLSIntolerance(HOST, PORT, range);
    ASSERT_EQ(SSL_LIBRARY_VERSION_3_0, range.min);
    ASSERT_EQ(SSL_LIBRARY_VERSION_TLS_1_2, range.max);
  }
}
