




































#ifndef prprf_h___
#define prprf_h___

















#include "prtypes.h"
#include "prio.h"
#include <stdio.h>
#include <stdarg.h>

PR_BEGIN_EXTERN_C






NSPR_API(PRUint32) PR_snprintf(char *out, PRUint32 outlen, const char *fmt, ...);






NSPR_API(char*) PR_smprintf(const char *fmt, ...);




NSPR_API(void) PR_smprintf_free(char *mem);








NSPR_API(char*) PR_sprintf_append(char *last, const char *fmt, ...);








typedef PRIntn (*PRStuffFunc)(void *arg, const char *s, PRUint32 slen);

NSPR_API(PRUint32) PR_sxprintf(PRStuffFunc f, void *arg, const char *fmt, ...);




NSPR_API(PRUint32) PR_fprintf(struct PRFileDesc* fd, const char *fmt, ...);




NSPR_API(PRUint32) PR_vsnprintf(char *out, PRUint32 outlen, const char *fmt, va_list ap);
NSPR_API(char*) PR_vsmprintf(const char *fmt, va_list ap);
NSPR_API(char*) PR_vsprintf_append(char *last, const char *fmt, va_list ap);
NSPR_API(PRUint32) PR_vsxprintf(PRStuffFunc f, void *arg, const char *fmt, va_list ap);
NSPR_API(PRUint32) PR_vfprintf(struct PRFileDesc* fd, const char *fmt, va_list ap);


































NSPR_API(PRInt32) PR_sscanf(const char *buf, const char *fmt, ...);

PR_END_EXTERN_C

#endif 
