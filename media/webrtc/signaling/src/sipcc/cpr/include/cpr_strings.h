



#ifndef _CPR_STRINGS_H_
#define _CPR_STRINGS_H_

#include "cpr_types.h"

__BEGIN_DECLS

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_strings.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_strings.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_strings.h"
#endif

#ifdef CPR_USE_OS_STRCASECMP

#ifndef cpr_strcasecmp
#define cpr_strcasecmp  strcasecmp
#endif
#ifndef cpr_strncasecmp
#define cpr_strncasecmp strncasecmp
#endif
#else














int cpr_strcasecmp(const char *s1, const char *s2);















int cpr_strncasecmp(const char *s1, const char *s2, size_t len);
#endif

__END_DECLS

#endif
