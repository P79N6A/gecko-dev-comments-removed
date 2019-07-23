




































#ifndef prdtoa_h___
#define prdtoa_h___

#include "prtypes.h"

PR_BEGIN_EXTERN_C












#if defined(HAVE_WATCOM_BUG_1)



PRFloat64 __pascal __loadds __export
#else
NSPR_API(PRFloat64)
#endif
PR_strtod(const char *s00, char **se);







NSPR_API(void) PR_cnvtf(char *buf, PRIntn bufsz, PRIntn prcsn, PRFloat64 fval);












NSPR_API(PRStatus) PR_dtoa(PRFloat64 d, PRIntn mode, PRIntn ndigits,
	PRIntn *decpt, PRIntn *sign, char **rve, char *buf, PRSize bufsize);

PR_END_EXTERN_C

#endif 
