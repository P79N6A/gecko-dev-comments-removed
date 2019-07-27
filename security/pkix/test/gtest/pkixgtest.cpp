























#include "pkixgtest.h"

#include <ctime>

#include "pkix/Time.h"

namespace mozilla { namespace pkix { namespace test {

const std::time_t ONE_DAY_IN_SECONDS_AS_TIME_T =
  static_cast<std::time_t>(Time::ONE_DAY_IN_SECONDS);



const std::time_t now(time(nullptr));
const std::time_t oneDayBeforeNow(time(nullptr) -
                                  ONE_DAY_IN_SECONDS_AS_TIME_T);
const std::time_t oneDayAfterNow(time(nullptr) +
                                 ONE_DAY_IN_SECONDS_AS_TIME_T);

} } } 
