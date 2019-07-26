























#ifndef mozilla_pkix__nssgtest_h
#define mozilla_pkix__nssgtest_h

#include "stdint.h"
#include "gtest/gtest.h"
#include "prerror.h"
#include "seccomon.h"

namespace mozilla { namespace pkix { namespace test {

class SECStatusWithPRErrorCode
{
public:
  SECStatusWithPRErrorCode(SECStatus rv, PRErrorCode errorCode)
    : mRv(rv)
    , mErrorCode(errorCode)
  {
  }

  SECStatusWithPRErrorCode(SECStatus rv)
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

} } } 

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

#endif 
