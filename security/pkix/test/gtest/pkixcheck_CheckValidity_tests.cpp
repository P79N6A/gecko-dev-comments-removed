























#include "gtest/gtest.h"
#include "pkix/pkixtypes.h"
#include "pkixtestutil.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

namespace mozilla { namespace pkix {

Result CheckValidity(const Input encodedValidity, Time time);

} } 

static const Time PAST_TIME(YMDHMS(1998, 12, 31, 12, 23, 56));

#define OLDER_GENERALIZEDTIME \
  0x18, 15,                               /* tag, length */ \
  '1', '9', '9', '9', '0', '1', '0', '1', /* 1999-01-01 */ \
  '0', '0', '0', '0', '0', '0', 'Z'       /* 00:00:00Z */

#define OLDER_UTCTIME \
  0x17, 13,                               /* tag, length */ \
  '9', '9', '0', '1', '0', '1',           /* (19)99-01-01 */ \
  '0', '0', '0', '0', '0', '0', 'Z'       /* 00:00:00Z */

static const Time NOW(YMDHMS(2016, 12, 31, 12, 23, 56));

#define NEWER_GENERALIZEDTIME \
  0x18, 15,                               /* tag, length */ \
  '2', '0', '2', '1', '0', '1', '0', '1', /* 2021-01-01 */ \
  '0', '0', '0', '0', '0', '0', 'Z'       /* 00:00:00Z */

#define NEWER_UTCTIME \
  0x17, 13,                               /* tag, length */ \
  '2', '1', '0', '1', '0', '1',           /* 2021-01-01 */ \
  '0', '0', '0', '0', '0', '0', 'Z'       /* 00:00:00Z */

static const Time FUTURE_TIME(YMDHMS(2025, 12, 31, 12, 23, 56));

class pkixcheck_CheckValidity : public ::testing::Test { };

TEST_F(pkixcheck_CheckValidity, BothEmptyNull)
{
  static const uint8_t DER[] = {
    0x17, 0,
    0x17, 0,
  };
  static const Input validity(DER);
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE, CheckValidity(validity, NOW));
}

TEST_F(pkixcheck_CheckValidity, NotBeforeEmptyNull)
{
  static const uint8_t DER[] = {
    0x17, 0x00,
    NEWER_UTCTIME
  };
  static const Input validity(DER);
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE, CheckValidity(validity, NOW));
}

TEST_F(pkixcheck_CheckValidity, NotAfterEmptyNull)
{
  static const uint8_t DER[] = {
    NEWER_UTCTIME,
    0x17, 0x00,
  };
  static const Input validity(DER);
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE, CheckValidity(validity, NOW));
}

static const uint8_t OLDER_UTCTIME_NEWER_UTCTIME_DATA[] = {
  OLDER_UTCTIME,
  NEWER_UTCTIME,
};
static const Input
OLDER_UTCTIME_NEWER_UTCTIME(OLDER_UTCTIME_NEWER_UTCTIME_DATA);

TEST_F(pkixcheck_CheckValidity, Valid_UTCTIME_UTCTIME)
{
  ASSERT_EQ(Success, CheckValidity(OLDER_UTCTIME_NEWER_UTCTIME, NOW));
}

TEST_F(pkixcheck_CheckValidity, Valid_GENERALIZEDTIME_GENERALIZEDTIME)
{
  static const uint8_t DER[] = {
    OLDER_GENERALIZEDTIME,
    NEWER_GENERALIZEDTIME,
  };
  static const Input validity(DER);
  ASSERT_EQ(Success, CheckValidity(validity, NOW));
}

TEST_F(pkixcheck_CheckValidity, Valid_GENERALIZEDTIME_UTCTIME)
{
  static const uint8_t DER[] = {
    OLDER_GENERALIZEDTIME,
    NEWER_UTCTIME,
  };
  static const Input validity(DER);
  ASSERT_EQ(Success, CheckValidity(validity, NOW));
}

TEST_F(pkixcheck_CheckValidity, Valid_UTCTIME_GENERALIZEDTIME)
{
  static const uint8_t DER[] = {
    OLDER_UTCTIME,
    NEWER_GENERALIZEDTIME,
  };
  static const Input validity(DER);
  ASSERT_EQ(Success, CheckValidity(validity, NOW));
}

TEST_F(pkixcheck_CheckValidity, InvalidBeforeNotBefore)
{
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE,
            CheckValidity(OLDER_UTCTIME_NEWER_UTCTIME, PAST_TIME));
}

TEST_F(pkixcheck_CheckValidity, InvalidAfterNotAfter)
{
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE,
            CheckValidity(OLDER_UTCTIME_NEWER_UTCTIME, FUTURE_TIME));
}

TEST_F(pkixcheck_CheckValidity, InvalidNotAfterBeforeNotBefore)
{
  static const uint8_t DER[] = {
    NEWER_UTCTIME,
    OLDER_UTCTIME,
  };
  static const Input validity(DER);
  ASSERT_EQ(Result::ERROR_EXPIRED_CERTIFICATE, CheckValidity(validity, NOW));
}
