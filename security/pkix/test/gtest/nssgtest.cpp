























#include "nssgtest.h"

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

} } } 
