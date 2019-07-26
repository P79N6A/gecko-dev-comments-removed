





#ifndef trace_malloc_nsTypeInfo_h_
#define trace_malloc_nsTypeInfo_h_

#ifdef __cplusplus
extern "C" {
#endif

extern const char* nsGetTypeName(void* ptr);

extern void RegisterTraceMallocShutdown();

#ifdef __cplusplus
}
#endif

#endif
