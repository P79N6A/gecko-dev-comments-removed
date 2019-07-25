







#ifndef SkUserTrace_DEFINED
#define SkUserTrace_DEFINED






#define SK_TRACE_EVENT0(event) \
  SkDebugf("Trace: %s\n", event)
#define SK_TRACE_EVENT1(event, name1, value1) \
  SkDebugf("Trace: %s (%s=%s)\n", event, name1, value1)
#define SK_TRACE_EVENT2(event, name1, value1, name2, value2) \
  SkDebugf("Trace: %s (%s=%s, %s=%s)\n", event, name1, value1, name2, value2)

#endif


