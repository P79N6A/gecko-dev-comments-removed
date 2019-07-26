









#ifndef BASE_TRACKING_INFO_H_
#define BASE_TRACKING_INFO_H_

#include "base/base_export.h"
#include "base/profiler/tracked_time.h"
#include "base/time/time.h"

namespace tracked_objects {
class Location;
class Births;
}

namespace base {


struct BASE_EXPORT TrackingInfo {
  TrackingInfo();
  TrackingInfo(const tracked_objects::Location& posted_from,
               base::TimeTicks delayed_run_time);
  ~TrackingInfo();

  
  
  
  
  
  
  tracked_objects::TrackedTime EffectiveTimePosted() const {
    return tracked_objects::TrackedTime(
        delayed_run_time.is_null() ? time_posted : delayed_run_time);
  }

  
  tracked_objects::Births* birth_tally;

  
  base::TimeTicks time_posted;

  
  base::TimeTicks delayed_run_time;
};

}  

#endif  
