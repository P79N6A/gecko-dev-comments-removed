






































#ifndef _CPR_ASSERT_H_
#define _CPR_ASSERT_H_

#include "cpr_types.h"


__BEGIN_DECLS


#define __CPRSTRING(x) #x

extern void __cprAssert(const char *str, const char *file, int line);


__END_DECLS


#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_assert.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_assert.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_assert.h"
#endif

#endif 






#undef cprAssert
#define cprAssert(expr, rc)

