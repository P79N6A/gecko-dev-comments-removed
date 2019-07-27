























#include "nssgtest.h"
#include "nss.h"
#include "pkixtestutil.h"
#include "prinit.h"

using namespace std;
using namespace testing;

namespace mozilla { namespace pkix { namespace test {

ostream&
operator<<(ostream& os, SECStatusWithPRErrorCode const& value)
{
  switch (value.mRv)
  {
    case SECSuccess:
      os << "SECSuccess";
      break;
    case SECWouldBlock:
      os << "SECWouldBlock";
      break;
    case SECFailure:
      os << "SECFailure";
      break;
    default:
      os << "[Invalid SECStatus: " << static_cast<int64_t>(value.mRv) << ']';
      break;
  }

  if (value.mRv != SECSuccess) {
    os << '(';
    const char* name = PR_ErrorToName(value.mErrorCode);
    if (name) {
      os << name;
    } else {
      os << value.mErrorCode;
    }
    os << ')';
  }

  return os;
}

AssertionResult
Pred_SECFailure(const char* expectedExpr, const char* actualExpr,
                PRErrorCode expectedErrorCode, SECStatus actual)
{
  if (SECFailure == actual && expectedErrorCode == PR_GetError()) {
    return AssertionSuccess();
  }

  return AssertionFailure()
      << "Expected: (" << expectedExpr << ") == (" << actualExpr
      << "), actual: " << SECFailure << " != " << actual;
}

 void
NSSTest::SetUpTestCase()
{
  if (NSS_NoDB_Init(nullptr) != SECSuccess) {
    PR_Abort();
  }

  now = Now();
  pr_now = PR_Now();
  pr_oneDayBeforeNow = pr_now - ONE_DAY;
  pr_oneDayAfterNow = pr_now + ONE_DAY;
}

NSSTest::NSSTest()
  : arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE))
{
  if (!arena) {
    PR_Abort();
  }
}

 mozilla::pkix::Time NSSTest::now(Now());
 PRTime NSSTest::pr_now;
 PRTime NSSTest::pr_oneDayBeforeNow;
 PRTime NSSTest::pr_oneDayAfterNow;

} } } 
