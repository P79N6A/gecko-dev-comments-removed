























#include "pkixgtest.h"
#include "pkixtestutil.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;

namespace mozilla { namespace pkix {

extern Result CheckTimes(const CERTValidity& validity, PRTime time);

} } 

static const SECItem empty_null = { siBuffer, nullptr, 0 };

static const PRTime PAST_TIME(YMDHMS(1998, 12, 31, 12, 23, 56));

static const uint8_t OLDER_GENERALIZEDTIME_DATA[] = {
  '1', '9', '9', '9', '0', '1', '0', '1', 
  '0', '0', '0', '0', '0', '0', 'Z'       
};
static const SECItem OLDER_GENERALIZEDTIME = {
  siGeneralizedTime,
  const_cast<uint8_t*>(OLDER_GENERALIZEDTIME_DATA),
  sizeof(OLDER_GENERALIZEDTIME_DATA)
};

static const uint8_t OLDER_UTCTIME_DATA[] = {
  '9', '9', '0', '1', '0', '1',           
  '0', '0', '0', '0', '0', '0', 'Z'       
};
static const SECItem OLDER_UTCTIME = {
  siUTCTime,
  const_cast<uint8_t*>(OLDER_UTCTIME_DATA),
  sizeof(OLDER_UTCTIME_DATA)
};

static const PRTime NOW(YMDHMS(2016, 12, 31, 12, 23, 56));

static const uint8_t NEWER_GENERALIZEDTIME_DATA[] = {
  '2', '0', '2', '1', '0', '1', '0', '1', 
  '0', '0', '0', '0', '0', '0', 'Z'       
};
static const SECItem NEWER_GENERALIZEDTIME = {
  siGeneralizedTime,
  const_cast<uint8_t*>(NEWER_GENERALIZEDTIME_DATA),
  sizeof(NEWER_GENERALIZEDTIME_DATA)
};

static const uint8_t NEWER_UTCTIME_DATA[] = {
  '2', '1', '0', '1', '0', '1',           
  '0', '0', '0', '0', '0', '0', 'Z'       
};
static const SECItem NEWER_UTCTIME = {
  siUTCTime,
  const_cast<uint8_t*>(NEWER_UTCTIME_DATA),
  sizeof(NEWER_UTCTIME_DATA)
};

static const PRTime FUTURE_TIME(YMDHMS(2025, 12, 31, 12, 23, 56));



class pkixcheck_CheckTimes : public ::testing::Test
{
public:
  virtual void SetUp()
  {
    PR_SetError(0, 0);
  }
};

TEST_F(pkixcheck_CheckTimes, BothEmptyNull)
{
  static const CERTValidity validity = { nullptr, empty_null, empty_null };
  ASSERT_RecoverableError(SEC_ERROR_EXPIRED_CERTIFICATE,
                          CheckTimes(validity, NOW));
}

TEST_F(pkixcheck_CheckTimes, NotBeforeEmptyNull)
{
  static const CERTValidity validity = { nullptr, empty_null, NEWER_UTCTIME };
  ASSERT_RecoverableError(SEC_ERROR_EXPIRED_CERTIFICATE,
                          CheckTimes(validity, NOW));
}

TEST_F(pkixcheck_CheckTimes, NotAfterEmptyNull)
{
  static const CERTValidity validity = { nullptr, OLDER_UTCTIME, empty_null };
  ASSERT_RecoverableError(SEC_ERROR_EXPIRED_CERTIFICATE,
                          CheckTimes(validity, NOW));
}

TEST_F(pkixcheck_CheckTimes, Valid_UTCTIME_UTCTIME)
{
  static const CERTValidity validity = {
    nullptr, OLDER_UTCTIME, NEWER_UTCTIME
  };
  ASSERT_Success(CheckTimes(validity, NOW));
}

TEST_F(pkixcheck_CheckTimes, Valid_GENERALIZEDTIME_GENERALIZEDTIME)
{
  static const CERTValidity validity = {
    nullptr, OLDER_GENERALIZEDTIME, NEWER_GENERALIZEDTIME
  };
  ASSERT_Success(CheckTimes(validity, NOW));
}

TEST_F(pkixcheck_CheckTimes, Valid_GENERALIZEDTIME_UTCTIME)
{
  static const CERTValidity validity = {
    nullptr, OLDER_GENERALIZEDTIME, NEWER_UTCTIME
  };
  ASSERT_Success(CheckTimes(validity, NOW));
}

TEST_F(pkixcheck_CheckTimes, Valid_UTCTIME_GENERALIZEDTIME)
{
  static const CERTValidity validity = {
    nullptr, OLDER_UTCTIME, NEWER_GENERALIZEDTIME
  };
  ASSERT_Success(CheckTimes(validity, NOW));
}

TEST_F(pkixcheck_CheckTimes, InvalidBeforeNotBefore)
{
  static const CERTValidity validity = {
    nullptr, OLDER_UTCTIME, NEWER_UTCTIME
  };
  ASSERT_RecoverableError(SEC_ERROR_EXPIRED_CERTIFICATE,
                          CheckTimes(validity, PAST_TIME));
}

TEST_F(pkixcheck_CheckTimes, InvalidAfterNotAfter)
{
  static const CERTValidity validity = {
    nullptr, OLDER_UTCTIME, NEWER_UTCTIME
  };
  ASSERT_RecoverableError(SEC_ERROR_EXPIRED_CERTIFICATE,
                          CheckTimes(validity, FUTURE_TIME));
}

TEST_F(pkixcheck_CheckTimes, InvalidNotAfterBeforeNotBefore)
{
  static const CERTValidity validity = {
    nullptr, NEWER_UTCTIME, OLDER_UTCTIME
  };
  ASSERT_RecoverableError(SEC_ERROR_EXPIRED_CERTIFICATE,
                          CheckTimes(validity, NOW));
}
