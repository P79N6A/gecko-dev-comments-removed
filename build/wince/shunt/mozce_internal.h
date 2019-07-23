









































#if !defined __mozce_internal_h
#define __mozce_internal_h

#include <windows.h>
#include <winsock2.h>

#include "mozce_defs.h"






















int a2w_buffer(const char* inACPString, int inACPChars, unsigned short* outWideString, int inWideChars);

























unsigned short* a2w_malloc(const char* inACPString, int inACPChars, int* outWideChars);





















int w2a_buffer( unsigned short* inWideString, int inWideChars, char* outACPString, int inACPChars);
























char* w2a_malloc(unsigned short* inWideString, int inWideChars, int* outACPChars);


#define charcount(array) (sizeof(array) / sizeof(array[0]))



#ifdef __cplusplus
extern "C" {
#endif

	MOZCE_SHUNT_API int mozce_printf(const char *, ...);

#ifdef SHUNT_LOG_ENABLED
    void mozce_DebugInit();
    void mozce_DebugDeinit();
    void mozce_DebugWriteToLog(char * str);
#endif

#ifdef API_LOGGING

#ifdef WINCE_MEMORY_CHECKPOINTING
    MOZCE_SHUNT_API void mozce_MemoryCheckpoint();

#define WINCE_LOG_API_CALL(x)          mozce_MemoryCheckpoint(); mozce_printf(x)
#define WINCE_LOG_API_CALL_1(x,y)      mozce_MemoryCheckpoint(); mozce_printf(x,y)
#define WINCE_LOG_API_CALL_2(x,y,z)    mozce_MemoryCheckpoint(); mozce_printf(x,y,z)
#else
#define WINCE_LOG_API_CALL(x)          mozce_printf(x)
#define WINCE_LOG_API_CALL_1(x,y)      mozce_printf(x,y)
#define WINCE_LOG_API_CALL_2(x,y,z)    mozce_printf(x,y,z)
#endif

#else

#define WINCE_LOG_API_CALL(x)
#define WINCE_LOG_API_CALL_1(x,y)
#define WINCE_LOG_API_CALL_2(x,y,z)

#endif          

#ifdef __cplusplus
};
#endif

int nclog (const char *fmt, ...);
void nclograw(const char* data, long length);

#endif
