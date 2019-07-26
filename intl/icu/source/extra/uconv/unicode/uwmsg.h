









#ifndef _UWMSG
#define _UWMSG

#include <stdio.h>

#include "unicode/ures.h"




U_CFUNC UResourceBundle *u_wmsg_setPath(const char *path, UErrorCode *err);


U_CFUNC int u_wmsg(FILE *fp, const char *tag, ... );


U_CFUNC const UChar* u_wmsg_errorName(UErrorCode err);

#endif
