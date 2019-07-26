























#include "pkixgtest.h"

namespace mozilla { namespace pkix { namespace test {

std::ostream&
operator<<(std::ostream& os, const ResultWithPRErrorCode& value)
{
  switch (value.mRv)
  {
    case Success:
      os << "Success";
      break;
    case RecoverableError:
      os << "RecoverableError";
      break;
    case SECFailure:
      os << "FatalError";
      break;
    default:
      os << "[Invalid Result: " << static_cast<int64_t>(value.mRv) << ']';
      break;
  }

  if (value.mRv != Success) {
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

} } } 
