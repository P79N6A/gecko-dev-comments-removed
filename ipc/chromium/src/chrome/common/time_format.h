



#ifndef CHROME_COMMON_TIME_FORMAT_H__
#define CHROME_COMMON_TIME_FORMAT_H__



#include <string>

#include "unicode/smpdtfmt.h"

namespace base {
class Time;
class TimeDelta;
}

class TimeFormat {
 public:
  
  
  
  

  
  static std::wstring TimeElapsed(const base::TimeDelta& delta);

  
  static std::wstring TimeRemaining(const base::TimeDelta& delta);

  
  static std::wstring TimeRemainingShort(const base::TimeDelta& delta);

  
  
  
  
  
  
  
  
  
  
  
  
  
  static std::wstring RelativeDate(const base::Time& time,
                                   const base::Time* optional_midnight_today);
};

#endif  
