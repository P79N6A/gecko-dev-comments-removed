






















#ifndef mozilla_pkix__pkixgtest_h
#define mozilla_pkix__pkixgtest_h

#include <ostream>

#include "gtest/gtest.h"
#include "pkixutil.h"
#include "prerror.h"
#include "stdint.h"

namespace mozilla { namespace pkix { namespace test {

class ResultWithPRErrorCode
{
public:
  ResultWithPRErrorCode(Result rv, PRErrorCode errorCode)
    : mRv(rv)
    , mErrorCode(errorCode)
  {
  }

  explicit ResultWithPRErrorCode(Result rv)
    : mRv(rv)
    , mErrorCode(rv == Success ? 0 : PR_GetError())
  {
  }

  bool operator==(const ResultWithPRErrorCode& other) const
  {
    return mRv == other.mRv && mErrorCode == other.mErrorCode;
  }

private:
  const Result mRv;
  const PRErrorCode mErrorCode;

  friend std::ostream& operator<<(std::ostream& os,
                                  const ResultWithPRErrorCode & value);

  void operator=(const ResultWithPRErrorCode&) ;
};

::std::ostream& operator<<(::std::ostream&, const ResultWithPRErrorCode &);

#define ASSERT_Success(rv) \
  ASSERT_EQ(::mozilla::pkix::test::ResultWithPRErrorCode( \
                ::mozilla::pkix::Success, 0), \
            ::mozilla::pkix::test::ResultWithPRErrorCode(rv))
#define EXPECT_Success(rv) \
  EXPECT_EQ(::mozilla::pkix::test::ResultWithPRErrorCode( \
                ::mozilla::pkix::Success, 0), \
            ::mozilla::pkix::test::ResultWithPRErrorCode(rv))

#define ASSERT_RecoverableError(expectedError, rv) \
  ASSERT_EQ(::mozilla::pkix::test::ResultWithPRErrorCode( \
                 ::mozilla::pkix::RecoverableError, expectedError), \
            ::mozilla::pkix::test::ResultWithPRErrorCode(rv))
#define EXPECT_RecoverableError(expectedError, rv) \
  EXPECT_EQ(::mozilla::pkix::test::ResultWithPRErrorCode( \
                 ::mozilla::pkix::RecoverableError, expectedError), \
            ::mozilla::pkix::test::ResultWithPRErrorCode(rv))

#define ASSERT_FatalError(expectedError, rv) \
  ASSERT_EQ(::mozilla::pkix::test::ResultWithPRErrorCode( \
                 ::mozilla::pkix::FatalError, expectedError), \
            ::mozilla::pkix::test::ResultWithPRErrorCode(rv))
#define EXPECT_FatalError(expectedError, rv) \
  EXPECT_EQ(::mozilla::pkix::test::ResultWithPRErrorCode( \
                 ::mozilla::pkix::FatalError, expectedError), \
            ::mozilla::pkix::test::ResultWithPRErrorCode(rv))

} } } 

#endif 
