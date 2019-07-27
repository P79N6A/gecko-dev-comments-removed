























#include <functional>
#include <vector>

#include "pkixgtest.h"
#include "pkix/pkixtypes.h"
#include "pkixder.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::der;

class pkixder_pki_types_tests : public ::testing::Test { };

TEST_F(pkixder_pki_types_tests, CertificateSerialNumber)
{
  const uint8_t DER_CERT_SERIAL[] = {
    0x02,                       
    8,                          
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
  };
  Input input(DER_CERT_SERIAL);
  Reader reader(input);

  Input item;
  ASSERT_EQ(Success, CertificateSerialNumber(reader, item));

  Input expected;
  ASSERT_EQ(Success,
            expected.Init(DER_CERT_SERIAL + 2, sizeof DER_CERT_SERIAL - 2));
  ASSERT_TRUE(InputsAreEqual(expected, item));
}

TEST_F(pkixder_pki_types_tests, CertificateSerialNumberLongest)
{
  const uint8_t DER_CERT_SERIAL_LONGEST[] = {
    0x02,                       
    20,                         
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
  };
  Input input(DER_CERT_SERIAL_LONGEST);
  Reader reader(input);

  Input item;
  ASSERT_EQ(Success, CertificateSerialNumber(reader, item));

  Input expected;
  ASSERT_EQ(Success,
            expected.Init(DER_CERT_SERIAL_LONGEST + 2,
                          sizeof DER_CERT_SERIAL_LONGEST - 2));
  ASSERT_TRUE(InputsAreEqual(expected, item));
}

TEST_F(pkixder_pki_types_tests, CertificateSerialNumberCrazyLong)
{
  const uint8_t DER_CERT_SERIAL_CRAZY_LONG[] = {
    0x02,                       
    32,                         
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
  };
  Input input(DER_CERT_SERIAL_CRAZY_LONG);
  Reader reader(input);

  Input item;
  ASSERT_EQ(Success, CertificateSerialNumber(reader, item));
}

TEST_F(pkixder_pki_types_tests, CertificateSerialNumberZeroLength)
{
  const uint8_t DER_CERT_SERIAL_ZERO_LENGTH[] = {
    0x02,                       
    0x00                        
  };
  Input input(DER_CERT_SERIAL_ZERO_LENGTH);
  Reader reader(input);

  Input item;
  ASSERT_EQ(Result::ERROR_BAD_DER, CertificateSerialNumber(reader, item));
}

TEST_F(pkixder_pki_types_tests, OptionalVersionV1ExplicitEncodingAllowed)
{
  const uint8_t DER_OPTIONAL_VERSION_V1[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x00              
  };
  Input input(DER_OPTIONAL_VERSION_V1);
  Reader reader(input);

  
  
  
  
  der::Version version = der::Version::v3;
  ASSERT_EQ(Success, OptionalVersion(reader, version));
  ASSERT_EQ(der::Version::v1, version);
}

TEST_F(pkixder_pki_types_tests, OptionalVersionV2)
{
  const uint8_t DER_OPTIONAL_VERSION_V2[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x01              
  };
  Input input(DER_OPTIONAL_VERSION_V2);
  Reader reader(input);

  der::Version version = der::Version::v1;
  ASSERT_EQ(Success, OptionalVersion(reader, version));
  ASSERT_EQ(der::Version::v2, version);
}

TEST_F(pkixder_pki_types_tests, OptionalVersionV3)
{
  const uint8_t DER_OPTIONAL_VERSION_V3[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x02              
  };
  Input input(DER_OPTIONAL_VERSION_V3);
  Reader reader(input);

  der::Version version = der::Version::v1;
  ASSERT_EQ(Success, OptionalVersion(reader, version));
  ASSERT_EQ(der::Version::v3, version);
}

TEST_F(pkixder_pki_types_tests, OptionalVersionUnknown)
{
  const uint8_t DER_OPTIONAL_VERSION_INVALID[] = {
    0xa0, 0x03,                   
    0x02, 0x01, 0x42              
  };
  Input input(DER_OPTIONAL_VERSION_INVALID);
  Reader reader(input);

  der::Version version = der::Version::v1;
  ASSERT_EQ(Result::ERROR_BAD_DER, OptionalVersion(reader, version));
}

TEST_F(pkixder_pki_types_tests, OptionalVersionInvalidTooLong)
{
  const uint8_t DER_OPTIONAL_VERSION_INVALID_TOO_LONG[] = {
    0xa0, 0x03,                   
    0x02, 0x02, 0x12, 0x34        
  };
  Input input(DER_OPTIONAL_VERSION_INVALID_TOO_LONG);
  Reader reader(input);

  der::Version version;
  ASSERT_EQ(Result::ERROR_BAD_DER, OptionalVersion(reader, version));
}

TEST_F(pkixder_pki_types_tests, OptionalVersionMissing)
{
  const uint8_t DER_OPTIONAL_VERSION_MISSING[] = {
    0x02, 0x11, 0x22              
  };
  Input input(DER_OPTIONAL_VERSION_MISSING);
  Reader reader(input);

  der::Version version = der::Version::v3;
  ASSERT_EQ(Success, OptionalVersion(reader, version));
  ASSERT_EQ(der::Version::v1, version);
}

static const size_t MAX_ALGORITHM_OID_DER_LENGTH = 13;

struct InvalidAlgorithmIdentifierTestInfo
{
  uint8_t der[MAX_ALGORITHM_OID_DER_LENGTH];
  size_t derLength;
};

struct ValidDigestAlgorithmIdentifierTestInfo
{
  DigestAlgorithm algorithm;
  uint8_t der[MAX_ALGORITHM_OID_DER_LENGTH];
  size_t derLength;
};

class pkixder_DigestAlgorithmIdentifier_Valid
  : public ::testing::Test
  , public ::testing::WithParamInterface<ValidDigestAlgorithmIdentifierTestInfo>
{
};

static const ValidDigestAlgorithmIdentifierTestInfo
  VALID_DIGEST_ALGORITHM_TEST_INFO[] =
{
  { DigestAlgorithm::sha512,
    { 0x30, 0x0b, 0x06, 0x09,
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03 },
    13
  },
  { DigestAlgorithm::sha384,
    { 0x30, 0x0b, 0x06, 0x09,
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02 },
    13
  },
  { DigestAlgorithm::sha256,
    { 0x30, 0x0b, 0x06, 0x09,
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01 },
    13
  },
  { DigestAlgorithm::sha1,
    { 0x30, 0x07, 0x06, 0x05,
      0x2b, 0x0e, 0x03, 0x02, 0x1a },
    9
  },
};

TEST_P(pkixder_DigestAlgorithmIdentifier_Valid, Valid)
{
  const ValidDigestAlgorithmIdentifierTestInfo& param(GetParam());

  {
    Input input;
    ASSERT_EQ(Success, input.Init(param.der, param.derLength));
    Reader reader(input);
    DigestAlgorithm alg;
    ASSERT_EQ(Success, DigestAlgorithmIdentifier(reader, alg));
    ASSERT_EQ(param.algorithm, alg);
    ASSERT_EQ(Success, End(reader));
  }

  {
    uint8_t derWithNullParam[MAX_ALGORITHM_OID_DER_LENGTH + 2];
    memcpy(derWithNullParam, param.der, param.derLength);
    derWithNullParam[1] += 2; 
    derWithNullParam[param.derLength] = 0x05; 
    derWithNullParam[param.derLength + 1] = 0x00; 

    Input input;
    ASSERT_EQ(Success, input.Init(derWithNullParam, param.derLength + 2));
    Reader reader(input);
    DigestAlgorithm alg;
    ASSERT_EQ(Success, DigestAlgorithmIdentifier(reader, alg));
    ASSERT_EQ(param.algorithm, alg);
    ASSERT_EQ(Success, End(reader));
  }
}

INSTANTIATE_TEST_CASE_P(pkixder_DigestAlgorithmIdentifier_Valid,
                        pkixder_DigestAlgorithmIdentifier_Valid,
                        testing::ValuesIn(VALID_DIGEST_ALGORITHM_TEST_INFO));

class pkixder_DigestAlgorithmIdentifier_Invalid
  : public ::testing::Test
  , public ::testing::WithParamInterface<InvalidAlgorithmIdentifierTestInfo>
{
};

static const InvalidAlgorithmIdentifierTestInfo
  INVALID_DIGEST_ALGORITHM_TEST_INFO[] =
{
  { 
    { 0x30, 0x0a, 0x06, 0x08,
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x05 },
    12,
  },
  { 
    { 0x30, 0x0a, 0x06, 0x08,
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02 },
    12,
  },
};

TEST_P(pkixder_DigestAlgorithmIdentifier_Invalid, Invalid)
{
  const InvalidAlgorithmIdentifierTestInfo& param(GetParam());
  Input input;
  ASSERT_EQ(Success, input.Init(param.der, param.derLength));
  Reader reader(input);
  DigestAlgorithm alg;
  ASSERT_EQ(Result::ERROR_INVALID_ALGORITHM,
            DigestAlgorithmIdentifier(reader, alg));
}

INSTANTIATE_TEST_CASE_P(pkixder_DigestAlgorithmIdentifier_Invalid,
                        pkixder_DigestAlgorithmIdentifier_Invalid,
                        testing::ValuesIn(INVALID_DIGEST_ALGORITHM_TEST_INFO));

struct ValidSignatureAlgorithmIdentifierValueTestInfo
{
  PublicKeyAlgorithm publicKeyAlg;
  DigestAlgorithm digestAlg;
  uint8_t der[MAX_ALGORITHM_OID_DER_LENGTH];
  size_t derLength;
};

static const ValidSignatureAlgorithmIdentifierValueTestInfo
  VALID_SIGNATURE_ALGORITHM_VALUE_TEST_INFO[] =
{
  
  { PublicKeyAlgorithm::ECDSA,
    DigestAlgorithm::sha512,
    { 0x06, 0x08,
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x04 },
    10,
  },
  { PublicKeyAlgorithm::ECDSA,
    DigestAlgorithm::sha384,
    { 0x06, 0x08,
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x03 },
    10,
  },
  { PublicKeyAlgorithm::ECDSA,
    DigestAlgorithm::sha256,
    { 0x06, 0x08,
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02 },
    10,
  },
  { PublicKeyAlgorithm::ECDSA,
    DigestAlgorithm::sha1,
    { 0x06, 0x07,
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x01 },
    9,
  },

  
  { PublicKeyAlgorithm::RSA_PKCS1,
    DigestAlgorithm::sha512,
    { 0x06, 0x09,
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0d },
    11,
  },
  { PublicKeyAlgorithm::RSA_PKCS1,
    DigestAlgorithm::sha384,
    { 0x06, 0x09,
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0c },
    11,
  },
  { PublicKeyAlgorithm::RSA_PKCS1,
    DigestAlgorithm::sha256,
    { 0x06, 0x09,
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b },
    11,
  },
  { PublicKeyAlgorithm::RSA_PKCS1,
    DigestAlgorithm::sha1,
    
    { 0x06, 0x09,
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05 },
    11,
  },
  { PublicKeyAlgorithm::RSA_PKCS1,
    DigestAlgorithm::sha1,
    
    { 0x06, 0x05,
      0x2b, 0x0e, 0x03, 0x02, 0x1d },
    7,
  },
};

class pkixder_SignatureAlgorithmIdentifierValue_Valid
  : public ::testing::Test
  , public ::testing::WithParamInterface<
             ValidSignatureAlgorithmIdentifierValueTestInfo>
{
};

TEST_P(pkixder_SignatureAlgorithmIdentifierValue_Valid, Valid)
{
  const ValidSignatureAlgorithmIdentifierValueTestInfo& param(GetParam());

  {
    Input input;
    ASSERT_EQ(Success, input.Init(param.der, param.derLength));
    Reader reader(input);
    PublicKeyAlgorithm publicKeyAlg;
    DigestAlgorithm digestAlg;
    ASSERT_EQ(Success,
              SignatureAlgorithmIdentifierValue(reader, publicKeyAlg,
                                                digestAlg));
    ASSERT_EQ(param.publicKeyAlg, publicKeyAlg);
    ASSERT_EQ(param.digestAlg, digestAlg);
    ASSERT_EQ(Success, End(reader));
  }

  {
    uint8_t derWithNullParam[MAX_ALGORITHM_OID_DER_LENGTH + 2];
    memcpy(derWithNullParam, param.der, param.derLength);
    derWithNullParam[param.derLength] = 0x05; 
    derWithNullParam[param.derLength + 1] = 0x00; 

    Input input;
    ASSERT_EQ(Success, input.Init(derWithNullParam, param.derLength + 2));
    Reader reader(input);
    PublicKeyAlgorithm publicKeyAlg;
    DigestAlgorithm digestAlg;
    ASSERT_EQ(Success,
              SignatureAlgorithmIdentifierValue(reader, publicKeyAlg,
                                                digestAlg));
    ASSERT_EQ(param.publicKeyAlg, publicKeyAlg);
    ASSERT_EQ(param.digestAlg, digestAlg);
    ASSERT_EQ(Success, End(reader));
  }
}

INSTANTIATE_TEST_CASE_P(
  pkixder_SignatureAlgorithmIdentifierValue_Valid,
  pkixder_SignatureAlgorithmIdentifierValue_Valid,
  testing::ValuesIn(VALID_SIGNATURE_ALGORITHM_VALUE_TEST_INFO));

static const InvalidAlgorithmIdentifierTestInfo
  INVALID_SIGNATURE_ALGORITHM_VALUE_TEST_INFO[] =
{
  
  { { 0x06, 0x09,
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x03, 0x02 },
    11,
  },

  
  { { 0x06, 0x07,
      0x2a, 0x86, 0x48, 0xce, 0x38, 0x04, 0x03 },
    9,
  },

  
  { { 0x06, 0x09,
      0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04 },
    11,
  },

  
  
  { { 0x06, 0x09,
      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01 },
    11,
  },
};

class pkixder_SignatureAlgorithmIdentifier_Invalid
  : public ::testing::Test
  , public ::testing::WithParamInterface<InvalidAlgorithmIdentifierTestInfo>
{
};

TEST_P(pkixder_SignatureAlgorithmIdentifier_Invalid, Invalid)
{
  const InvalidAlgorithmIdentifierTestInfo& param(GetParam());
  Input input;
  ASSERT_EQ(Success, input.Init(param.der, param.derLength));
  Reader reader(input);
  der::PublicKeyAlgorithm publicKeyAlg;
  DigestAlgorithm digestAlg;
  ASSERT_EQ(Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED,
            SignatureAlgorithmIdentifierValue(reader, publicKeyAlg, digestAlg));
}

INSTANTIATE_TEST_CASE_P(
  pkixder_SignatureAlgorithmIdentifier_Invalid,
  pkixder_SignatureAlgorithmIdentifier_Invalid,
  testing::ValuesIn(INVALID_SIGNATURE_ALGORITHM_VALUE_TEST_INFO));
