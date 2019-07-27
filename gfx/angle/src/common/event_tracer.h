



#ifndef COMMON_EVENT_TRACER_H_
#define COMMON_EVENT_TRACER_H_

#include "common/platform.h"

#if !defined(TRACE_ENTRY)
#   ifdef ANGLE_PLATFORM_WINDOWS
#       define TRACE_ENTRY __stdcall
#   else
#       define TRACE_ENTRY
#   endif 
#endif 

extern "C" {

typedef const unsigned char* (*GetCategoryEnabledFlagFunc)(const char* name);
typedef void (*AddTraceEventFunc)(char phase, const unsigned char* categoryGroupEnabled, const char* name,
                                  unsigned long long id, int numArgs, const char** argNames,
                                  const unsigned char* argTypes, const unsigned long long* argValues,
                                  unsigned char flags);


void TRACE_ENTRY SetTraceFunctionPointers(GetCategoryEnabledFlagFunc get_category_enabled_flag,
                                          AddTraceEventFunc add_trace_event_func);

}

namespace gl
{

const unsigned char* TraceGetTraceCategoryEnabledFlag(const char* name);

void TraceAddTraceEvent(char phase, const unsigned char* categoryGroupEnabled, const char* name, unsigned long long id,
                        int numArgs, const char** argNames, const unsigned char* argTypes,
                        const unsigned long long* argValues, unsigned char flags);

}

#endif  
