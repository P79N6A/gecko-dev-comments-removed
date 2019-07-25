





































#ifndef jsprf_h___
#define jsprf_h___



















#include "jstypes.h"
#include <stdio.h>
#include <stdarg.h>

JS_BEGIN_EXTERN_C






extern JS_PUBLIC_API(uint32_t) JS_snprintf(char *out, uint32_t outlen, const char *fmt, ...);






extern JS_PUBLIC_API(char*) JS_smprintf(const char *fmt, ...);




extern JS_PUBLIC_API(void) JS_smprintf_free(char *mem);








extern JS_PUBLIC_API(char*) JS_sprintf_append(char *last, const char *fmt, ...);








typedef JSIntn (*JSStuffFunc)(void *arg, const char *s, uint32_t slen);

extern JS_PUBLIC_API(uint32_t) JS_sxprintf(JSStuffFunc f, void *arg, const char *fmt, ...);




extern JS_PUBLIC_API(uint32_t) JS_vsnprintf(char *out, uint32_t outlen, const char *fmt, va_list ap);
extern JS_PUBLIC_API(char*) JS_vsmprintf(const char *fmt, va_list ap);
extern JS_PUBLIC_API(char*) JS_vsprintf_append(char *last, const char *fmt, va_list ap);
extern JS_PUBLIC_API(uint32_t) JS_vsxprintf(JSStuffFunc f, void *arg, const char *fmt, va_list ap);

JS_END_EXTERN_C

#endif 
