









#include "webrtc/system_wrappers/interface/event_tracer.h"

namespace webrtc {

namespace {

GetCategoryEnabledPtr g_get_category_enabled_ptr = 0;
AddTraceEventPtr g_add_trace_event_ptr = 0;

}  

void SetupEventTracer(GetCategoryEnabledPtr get_category_enabled_ptr,
                      AddTraceEventPtr add_trace_event_ptr) {
  g_get_category_enabled_ptr = get_category_enabled_ptr;
  g_add_trace_event_ptr = add_trace_event_ptr;
}


const unsigned char* EventTracer::GetCategoryEnabled(const char* name) {
  if (g_get_category_enabled_ptr)
    return g_get_category_enabled_ptr(name);

  
  return reinterpret_cast<const unsigned char*>("\0");
}


void EventTracer::AddTraceEvent(char phase,
                                const unsigned char* category_enabled,
                                const char* name,
                                unsigned long long id,
                                int num_args,
                                const char** arg_names,
                                const unsigned char* arg_types,
                                const unsigned long long* arg_values,
                                unsigned char flags) {
  if (g_add_trace_event_ptr) {
    g_add_trace_event_ptr(phase,
                          category_enabled,
                          name,
                          id,
                          num_args,
                          arg_names,
                          arg_types,
                          arg_values,
                          flags);
  }
}

}  
