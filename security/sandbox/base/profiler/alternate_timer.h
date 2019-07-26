






#ifndef BASE_PROFILER_ALTERNATE_TIMER_H_
#define BASE_PROFILER_ALTERNATE_TIMER_H_

#include "base/base_export.h"

namespace tracked_objects {

enum TimeSourceType {
  TIME_SOURCE_TYPE_WALL_TIME,
  TIME_SOURCE_TYPE_TCMALLOC
};


typedef unsigned int NowFunction();




BASE_EXPORT extern const char kAlternateProfilerTime[];





BASE_EXPORT void SetAlternateTimeSource(NowFunction* now_function,
                                        TimeSourceType type);



NowFunction* GetAlternateTimeSource();


BASE_EXPORT TimeSourceType GetTimeSourceType();

}  

#endif  
