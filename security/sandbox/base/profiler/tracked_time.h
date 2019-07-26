



#ifndef BASE_PROFILER_TRACKED_TIME_H_
#define BASE_PROFILER_TRACKED_TIME_H_


#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/time/time.h"

namespace tracked_objects {












class BASE_EXPORT Duration {  
 public:
  Duration();

  Duration& operator+=(const Duration& other);
  Duration operator+(const Duration& other) const;

  bool operator==(const Duration& other) const;
  bool operator!=(const Duration& other) const;
  bool operator>(const Duration& other) const;

  static Duration FromMilliseconds(int ms);

  int32 InMilliseconds() const;

 private:
  friend class TrackedTime;
  explicit Duration(int32 duration);

  
  int32 ms_;
};

class BASE_EXPORT TrackedTime {  
 public:
  TrackedTime();
  explicit TrackedTime(const base::TimeTicks& time);

  static TrackedTime Now();
  Duration operator-(const TrackedTime& other) const;
  TrackedTime operator+(const Duration& other) const;
  bool is_null() const;

  static TrackedTime FromMilliseconds(int32 ms) { return TrackedTime(ms); }

 private:
  friend class Duration;
  explicit TrackedTime(int32 ms);

  
  uint32 ms_;
};

}  

#endif  
