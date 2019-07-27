























#include "gtest/gtest.h"
#include "pkix/pkixtypes.h"

using namespace mozilla::pkix;

namespace mozilla { namespace pkix {

extern Result CheckKeyUsage(EndEntityOrCA endEntityOrCA,
                            const SECItem* encodedKeyUsage,
                            KeyUsage requiredKeyUsageIfPresent);

} } 

class pkixcheck_CheckKeyUsage : public ::testing::Test { };

#define ASSERT_BAD(x) ASSERT_EQ(Result::ERROR_INADEQUATE_KEY_USAGE, x)


#define NAMED_SIMPLE_KU(name, unusedBits, bits) \
  const uint8_t name##_bytes[4] = { \
    0x03/*BIT STRING*/, 0x02/*LENGTH=2*/, unusedBits, bits \
  }; \
  const SECItem name = { \
    siBuffer, \
    const_cast<uint8_t*>(name##_bytes), \
    4 \
  }

static uint8_t dummy;
static const SECItem empty_null    = { siBuffer, nullptr, 0 };
static const SECItem empty_nonnull = { siBuffer, &dummy, 0 };




TEST_F(pkixcheck_CheckKeyUsage, EE_none)
{
  
  
  

  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, nullptr,
                                   KeyUsage::noParticularKeyUsageRequired));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, nullptr,
                                   KeyUsage::digitalSignature));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, nullptr,
                                   KeyUsage::nonRepudiation));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, nullptr,
                                   KeyUsage::keyEncipherment));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, nullptr,
                                   KeyUsage::dataEncipherment));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, nullptr,
                                   KeyUsage::keyAgreement));
}

TEST_F(pkixcheck_CheckKeyUsage, EE_empty)
{
  
  
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &empty_null,
                           KeyUsage::digitalSignature));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &empty_nonnull,
                           KeyUsage::digitalSignature));
}

TEST_F(pkixcheck_CheckKeyUsage, CA_none)
{
  
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeCA, nullptr,
                                   KeyUsage::keyCertSign));
}

TEST_F(pkixcheck_CheckKeyUsage, CA_empty)
{
  
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &empty_null,
                           KeyUsage::keyCertSign));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &empty_nonnull,
                           KeyUsage::keyCertSign));
}

TEST_F(pkixcheck_CheckKeyUsage, maxUnusedBits)
{
  NAMED_SIMPLE_KU(encoded, 7, 0x80);
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &encoded,
                                   KeyUsage::digitalSignature));
}

TEST_F(pkixcheck_CheckKeyUsage, tooManyUnusedBits)
{
  static uint8_t oneValueByteData[] = {
    0x03, 0x02, 8, 0x80
  };
  const SECItem oneValueByte = {
    siBuffer,
    oneValueByteData,
    sizeof(oneValueByteData)
  };
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &oneValueByte,
                           KeyUsage::digitalSignature));

  static uint8_t twoValueBytesData[] = {
    0x03, 0x03, 8, 0x01, 0x00
  };
  const SECItem twoValueBytes = {
    siBuffer,
    twoValueBytesData,
    sizeof(twoValueBytesData)
  };
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &twoValueBytes,
                           KeyUsage::digitalSignature));
}

TEST_F(pkixcheck_CheckKeyUsage, NoValueBytes_NoPaddingBits)
{
  static const uint8_t DER_BYTES[] = {
    0x03, 0x01, 0
  };
  static const SECItem DER = {
    siBuffer,
    const_cast<uint8_t*>(DER_BYTES),
    sizeof(DER_BYTES)
  };

  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &DER,
                           KeyUsage::digitalSignature));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &DER,
                           KeyUsage::keyCertSign));
}

TEST_F(pkixcheck_CheckKeyUsage, NoValueBytes_7PaddingBits)
{
  static const uint8_t DER_BYTES[] = {
    0x03, 0x01, 7
  };
  static const SECItem DER = {
    siBuffer,
    const_cast<uint8_t*>(DER_BYTES),
    sizeof(DER_BYTES)
  };

  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &DER,
                           KeyUsage::digitalSignature));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &DER,
                           KeyUsage::keyCertSign));
}

void ASSERT_SimpleCase(uint8_t unusedBits, uint8_t bits, KeyUsage usage)
{
  
  
  NAMED_SIMPLE_KU(good, unusedBits, bits);
  ASSERT_EQ(Success,
            CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &good, usage));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeCA, &good, usage));

  
  
  

  
  
  NAMED_SIMPLE_KU(notGood, unusedBits,
                  static_cast<uint8_t>((~bits >> unusedBits) << unusedBits));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &notGood, usage));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &notGood, usage));

  
  
  uint8_t twoByteNotGoodData[] = {
    0x03, 0x03, unusedBits,
    static_cast<uint8_t>(~bits),
    static_cast<uint8_t>((0xFFu >> unusedBits) << unusedBits)
  };
  const SECItem twoByteNotGood = {
    siBuffer, twoByteNotGoodData, sizeof(twoByteNotGoodData)
  };
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &twoByteNotGood,
                           usage));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &twoByteNotGood, usage));
}

TEST_F(pkixcheck_CheckKeyUsage, simpleCases)
{
  ASSERT_SimpleCase(7, 0x80, KeyUsage::digitalSignature);
  ASSERT_SimpleCase(6, 0x40, KeyUsage::nonRepudiation);
  ASSERT_SimpleCase(5, 0x20, KeyUsage::keyEncipherment);
  ASSERT_SimpleCase(4, 0x10, KeyUsage::dataEncipherment);
  ASSERT_SimpleCase(3, 0x08, KeyUsage::keyAgreement);
}


TEST_F(pkixcheck_CheckKeyUsage, keyCertSign)
{
  NAMED_SIMPLE_KU(good, 2, 0x04);
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &good,
                           KeyUsage::keyCertSign));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeCA, &good,
                                   KeyUsage::keyCertSign));

  
  
  NAMED_SIMPLE_KU(notGood, 2, 0xFB);
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &notGood,
                           KeyUsage::keyCertSign));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &notGood,
                           KeyUsage::keyCertSign));

  
  
  static uint8_t twoByteNotGoodData[] = {
    0x03, 0x03, 2, 0xFBu, 0xFCu
  };
  static const SECItem twoByteNotGood = {
    siBuffer, twoByteNotGoodData, sizeof(twoByteNotGoodData)
  };
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &twoByteNotGood,
                           KeyUsage::keyCertSign));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &twoByteNotGood,
                           KeyUsage::keyCertSign));
}

TEST_F(pkixcheck_CheckKeyUsage, unusedBitNotZero)
{
  
  static uint8_t controlOneValueByteData[] = {
    0x03, 0x02, 7, 0x80
  };
  const SECItem controlOneValueByte = {
    siBuffer,
    controlOneValueByteData,
    sizeof(controlOneValueByteData)
  };
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity,
                                   &controlOneValueByte,
                                   KeyUsage::digitalSignature));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeCA,
                                   &controlOneValueByte,
                                   KeyUsage::digitalSignature));

  
  static uint8_t oneValueByteData[] = {
    0x03, 0x02, 7, 0x80 | 0x01
  };
  const SECItem oneValueByte = {
    siBuffer,
    oneValueByteData,
    sizeof(oneValueByteData)
  };
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &oneValueByte,
                           KeyUsage::digitalSignature));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &oneValueByte,
                           KeyUsage::digitalSignature));

  
  static uint8_t controlTwoValueBytesData[] = {
    0x03, 0x03, 7,
    0x80 | 0x01, 0x80
  };
  const SECItem controlTwoValueBytes = {
    siBuffer,
    controlTwoValueBytesData,
    sizeof(controlTwoValueBytesData)
  };
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeEndEntity,
                                   &controlTwoValueBytes,
                                   KeyUsage::digitalSignature));
  ASSERT_EQ(Success, CheckKeyUsage(EndEntityOrCA::MustBeCA,
                                   &controlTwoValueBytes,
                                   KeyUsage::digitalSignature));

  
  static uint8_t twoValueBytesData[] = {
    0x03, 0x03, 7,
    0x80 | 0x01, 0x80 | 0x01
  };
  const SECItem twoValueBytes = {
    siBuffer,
    twoValueBytesData,
    sizeof(twoValueBytesData)
  };
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeEndEntity, &twoValueBytes,
                           KeyUsage::digitalSignature));
  ASSERT_BAD(CheckKeyUsage(EndEntityOrCA::MustBeCA, &twoValueBytes,
                           KeyUsage::digitalSignature));
}
