









































#if !defined __mozce_internal_h
#define __mozce_internal_h

#include <windows.h>
#include <winsock2.h>

#include "mozce_defs.h"






















int a2w_buffer(const char* inACPString, int inACPChars, unsigned short* outWideString, int inWideChars);

























unsigned short* a2w_malloc(const char* inACPString, int inACPChars, int* outWideChars);





















int w2a_buffer(const unsigned short* inWideString, int inWideChars, char* outACPString, int inACPChars);
























char* w2a_malloc(unsigned short* inWideString, int inWideChars, int* outACPChars);


void dumpMemoryInfo();

#define charcount(array) (sizeof(array) / sizeof(array[0]))



#ifdef __cplusplus
extern "C" {
#endif

	MOZCE_SHUNT_API int mozce_printf(const char *, ...);

#ifdef __cplusplus
};
#endif

int nclog (const char *fmt, ...);
void nclograw(const char* data, long length);














#define MOZCE_PRECHECK                 \
{                                      \
    SetLastError(0);                   \
}


#endif
