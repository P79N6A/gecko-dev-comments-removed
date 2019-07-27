























#ifndef mozilla_pkix__nssgtest_h
#define mozilla_pkix__nssgtest_h

#include "stdint.h"
#include "gtest/gtest.h"
#include "pkix/pkixtypes.h"
#include "pkixtestutil.h"
#include "prerror.h"
#include "prtime.h"
#include "seccomon.h"

namespace mozilla { namespace pkix { namespace test {

inline void
PORT_FreeArena_false(PLArenaPool* arena)
{
  
  
  return PORT_FreeArena(arena, PR_FALSE);
}

typedef ScopedPtr<PLArenaPool, PORT_FreeArena_false> ScopedPLArenaPool;

class SECStatusWithPRErrorCode
{
public:
  SECStatusWithPRErrorCode(SECStatus rv, PRErrorCode errorCode)
    : mRv(rv)
    , mErrorCode(errorCode)
  {
  }

  explicit SECStatusWithPRErrorCode(SECStatus rv)
    : mRv(rv)
    , mErrorCode(rv == SECSuccess ? 0 : PR_GetError())
  {
  }

  bool operator==(const SECStatusWithPRErrorCode& other) const
  {
    return mRv == other.mRv && mErrorCode == other.mErrorCode;
  }

private:
  const SECStatus mRv;
  const PRErrorCode mErrorCode;

  friend std::ostream& operator<<(std::ostream& os,
                                  SECStatusWithPRErrorCode const& value);

  void operator=(const SECStatusWithPRErrorCode&) ;
};

::std::ostream& operator<<(::std::ostream&,
                           SECStatusWithPRErrorCode const&);

#define ASSERT_SECSuccess(rv) \
  ASSERT_EQ(::mozilla::pkix::test::SECStatusWithPRErrorCode(SECSuccess, 0), \
            ::mozilla::pkix::test::SECStatusWithPRErrorCode(rv))
#define EXPECT_SECSuccess(rv) \
  EXPECT_EQ(::mozilla::pkix::test::SECStatusWithPRErrorCode(SECSuccess, 0), \
            ::mozilla::pkix::test::SECStatusWithPRErrorCode(rv))

#define ASSERT_SECFailure(expectedError, rv) \
  ASSERT_EQ(::mozilla::pkix::test::SECStatusWithPRErrorCode(SECFailure, \
                                                            expectedError), \
            ::mozilla::pkix::test::SECStatusWithPRErrorCode(rv))
#define EXPECT_SECFailure(expectedError, rv) \
  EXPECT_EQ(::mozilla::pkix::test::SECStatusWithPRErrorCode(SECFailure, \
                                                            expectedError), \
            ::mozilla::pkix::test::SECStatusWithPRErrorCode(rv))

class NSSTest : public ::testing::Test
{
public:
  static void SetUpTestCase();

protected:
  NSSTest();

  ScopedPLArenaPool arena;
  static mozilla::pkix::Time now;
  static PRTime pr_now;
  static PRTime pr_oneDayBeforeNow;
  static PRTime pr_oneDayAfterNow;
};

} } } 

#endif 
