























#include <functional>
#include <vector>
#include <gtest/gtest.h>

#include "pkixder.h"

using namespace mozilla::pkix::der;

namespace {

class pkixder_pki_types_tests : public ::testing::Test
{
protected:
  virtual void SetUp()
  {
    PR_SetError(0, 0);
  }
};

TEST_F(pkixder_pki_types_tests, CertificateSerialNumber)
{
  const uint8_t DER_CERT_SERIAL[] = {
    0x02,                       
    8,                          
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_CERT_SERIAL, sizeof DER_CERT_SERIAL));

  SECItem item;
  ASSERT_EQ(Success, CertificateSerialNumber(input, item));

  ASSERT_EQ(sizeof DER_CERT_SERIAL - 2, item.len);
  ASSERT_TRUE(memcmp(item.data, DER_CERT_SERIAL + 2,
                     sizeof DER_CERT_SERIAL - 2) == 0);
}

TEST_F(pkixder_pki_types_tests, CertificateSerialNumberLongest)
{
  const uint8_t DER_CERT_SERIAL_LONGEST[] = {
    0x02,                       
    20,                         
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_CERT_SERIAL_LONGEST,
                                sizeof DER_CERT_SERIAL_LONGEST));

  SECItem item;
  ASSERT_EQ(Success, CertificateSerialNumber(input, item));

  ASSERT_EQ(sizeof DER_CERT_SERIAL_LONGEST - 2, item.len);
  ASSERT_TRUE(memcmp(item.data, DER_CERT_SERIAL_LONGEST + 2,
                     sizeof DER_CERT_SERIAL_LONGEST - 2) == 0);
}

TEST_F(pkixder_pki_types_tests, CertificateSerialNumberCrazyLong)
{
  const uint8_t DER_CERT_SERIAL_CRAZY_LONG[] = {
    0x02,                       
    32,                         
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_CERT_SERIAL_CRAZY_LONG,
                                sizeof DER_CERT_SERIAL_CRAZY_LONG));

  SECItem item;
  ASSERT_EQ(Success, CertificateSerialNumber(input, item));
}

TEST_F(pkixder_pki_types_tests, CertificateSerialNumberZeroLength)
{
  const uint8_t DER_CERT_SERIAL_ZERO_LENGTH[] = {
    0x02,                       
    0x00                        
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_CERT_SERIAL_ZERO_LENGTH,
                                sizeof DER_CERT_SERIAL_ZERO_LENGTH));

  SECItem item;
  ASSERT_EQ(Failure, CertificateSerialNumber(input, item));
  ASSERT_EQ(SEC_ERROR_BAD_DER, PR_GetError());
}

TEST_F(pkixder_pki_types_tests, OptionalVersionV1ExplicitEncodingAllowed)
{
  const uint8_t DER_OPTIONAL_VERSION_V1[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x00              
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_OPTIONAL_VERSION_V1,
                                sizeof DER_OPTIONAL_VERSION_V1));

  
  
  
  
  
  Version version = Version::v3;
  ASSERT_EQ(Success, OptionalVersion(input, version));
  ASSERT_EQ(Version::v1, version);
}

TEST_F(pkixder_pki_types_tests, OptionalVersionV2)
{
  const uint8_t DER_OPTIONAL_VERSION_V2[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x01              
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_OPTIONAL_VERSION_V2,
                                sizeof DER_OPTIONAL_VERSION_V2));

  Version version = Version::v1;
  ASSERT_EQ(Success, OptionalVersion(input, version));
  ASSERT_EQ(Version::v2, version);
}

TEST_F(pkixder_pki_types_tests, OptionalVersionV3)
{
  const uint8_t DER_OPTIONAL_VERSION_V3[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x02              
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_OPTIONAL_VERSION_V3,
                                sizeof DER_OPTIONAL_VERSION_V3));

  Version version = Version::v1;
  ASSERT_EQ(Success, OptionalVersion(input, version));
  ASSERT_EQ(Version::v3, version);
}

TEST_F(pkixder_pki_types_tests, OptionalVersionUnknown)
{
  const uint8_t DER_OPTIONAL_VERSION_INVALID[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x42              
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_OPTIONAL_VERSION_INVALID,
                                sizeof DER_OPTIONAL_VERSION_INVALID));

  Version version = Version::v1;
  ASSERT_EQ(Failure, OptionalVersion(input, version));
  ASSERT_EQ(SEC_ERROR_BAD_DER, PR_GetError());
}

TEST_F(pkixder_pki_types_tests, OptionalVersionInvalidTooLong)
{
  const uint8_t DER_OPTIONAL_VERSION_INVALID_TOO_LONG[] = {
    0xa0, 0x03,                   
    0x02, 0x02, 0x12, 0x34        
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_OPTIONAL_VERSION_INVALID_TOO_LONG,
                                sizeof DER_OPTIONAL_VERSION_INVALID_TOO_LONG));

  Version version;
  ASSERT_EQ(Failure, OptionalVersion(input, version));
  ASSERT_EQ(SEC_ERROR_BAD_DER, PR_GetError());
}

TEST_F(pkixder_pki_types_tests, OptionalVersionMissing)
{
  const uint8_t DER_OPTIONAL_VERSION_MISSING[] = {
    0x02, 0x11, 0x22              
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_OPTIONAL_VERSION_MISSING,
                                sizeof DER_OPTIONAL_VERSION_MISSING));

  Version version = Version::v3;
  ASSERT_EQ(Success, OptionalVersion(input, version));
  ASSERT_EQ(Version::v1, version);
}

} 
