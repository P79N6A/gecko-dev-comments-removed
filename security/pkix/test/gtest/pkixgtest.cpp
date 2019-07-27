























#include "pkixgtest.h"

#include <ctime>

#include "pkix/nullptr.h"
#include "pkix/Time.h"

namespace mozilla { namespace pkix { namespace test {



const std::time_t now(time(nullptr));
const std::time_t oneDayBeforeNow(time(nullptr) - Time::ONE_DAY_IN_SECONDS);
const std::time_t oneDayAfterNow(time(nullptr) + Time::ONE_DAY_IN_SECONDS);

} } } 
