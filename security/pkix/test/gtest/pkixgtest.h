






















#ifndef mozilla_pkix_pkixgtest_h
#define mozilla_pkix_pkixgtest_h

#include <ostream>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma clang diagnostic ignored "-Wshift-sign-overflow"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wundef"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#elif defined(_MSC_VER)
#pragma warning(push, 3)


#pragma warning(disable: 4224)


#pragma warning(disable: 4826)
#endif

#include "gtest/gtest.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

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

extern const std::time_t ONE_DAY_IN_SECONDS_AS_TIME_T;

extern const std::time_t now;
extern const std::time_t oneDayBeforeNow;
extern const std::time_t oneDayAfterNow;

} } } 

#endif 
