
























#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_EVENT_TRACER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_EVENT_TRACER_H_

#include "webrtc/common_types.h"

namespace webrtc {

typedef const unsigned char* (*GetCategoryEnabledPtr)(const char* name);
typedef void (*AddTraceEventPtr)(char phase,
                                 const unsigned char* category_enabled,
                                 const char* name,
                                 unsigned long long id,
                                 int num_args,
                                 const char** arg_names,
                                 const unsigned char* arg_types,
                                 const unsigned long long* arg_values,
                                 unsigned char flags);





WEBRTC_DLLEXPORT void SetupEventTracer(
    GetCategoryEnabledPtr get_category_enabled_ptr,
    AddTraceEventPtr add_trace_event_ptr);



class EventTracer {
 public:
  static const unsigned char* GetCategoryEnabled(
      const char* name);

  static void AddTraceEvent(
      char phase,
      const unsigned char* category_enabled,
      const char* name,
      unsigned long long id,
      int num_args,
      const char** arg_names,
      const unsigned char* arg_types,
      const unsigned long long* arg_values,
      unsigned char flags);
};

}  

#endif  
