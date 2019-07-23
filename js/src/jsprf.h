





































#ifndef jsprf_h___
#define jsprf_h___



















#include "jstypes.h"
#include <stdio.h>
#include <stdarg.h>

JS_BEGIN_EXTERN_C






extern JS_PUBLIC_API(JSUint32) JS_snprintf(char *out, JSUint32 outlen, const char *fmt, ...);






extern JS_PUBLIC_API(char*) JS_smprintf(const char *fmt, ...);




extern JS_PUBLIC_API(void) JS_smprintf_free(char *mem);








extern JS_PUBLIC_API(char*) JS_sprintf_append(char *last, const char *fmt, ...);








typedef JSIntn (*JSStuffFunc)(void *arg, const char *s, JSUint32 slen);

extern JS_PUBLIC_API(JSUint32) JS_sxprintf(JSStuffFunc f, void *arg, const char *fmt, ...);




extern JS_PUBLIC_API(JSUint32) JS_vsnprintf(char *out, JSUint32 outlen, const char *fmt, va_list ap);
extern JS_PUBLIC_API(char*) JS_vsmprintf(const char *fmt, va_list ap);
extern JS_PUBLIC_API(char*) JS_vsprintf_append(char *last, const char *fmt, va_list ap);
extern JS_PUBLIC_API(JSUint32) JS_vsxprintf(JSStuffFunc f, void *arg, const char *fmt, va_list ap);


































extern JS_PUBLIC_API(JSInt32) JS_sscanf(const char *buf, const char *fmt, ...);

JS_END_EXTERN_C

#endif 
