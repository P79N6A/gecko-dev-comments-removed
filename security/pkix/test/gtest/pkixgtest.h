






















#ifndef mozilla_pkix__pkixgtest_h
#define mozilla_pkix__pkixgtest_h

#include <ostream>

#include "gtest/gtest.h"
#include "pkix/Result.h"


namespace mozilla { namespace pkix {

inline void
PrintTo(const Result& result, ::std::ostream* os)
{
  const char* stringified = MapResultToName(result);
  if (stringified) {
    *os << stringified;
  } else {
    *os << "mozilla::pkix::Result(" << static_cast<unsigned int>(result) << ")";
  }
}

} } 

namespace mozilla { namespace pkix { namespace test {

extern const std::time_t now;
extern const std::time_t oneDayBeforeNow;
extern const std::time_t oneDayAfterNow;

} } } 

#endif 
