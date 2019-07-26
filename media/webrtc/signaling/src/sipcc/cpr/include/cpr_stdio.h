






































#ifndef _CPR_STDIO_H_
#define _CPR_STDIO_H_

#include "cpr_types.h"

__BEGIN_DECLS

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_stdio.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_stdio.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_stdio.h"
#endif











extern int32_t buginf(const char *_format, ...);










extern int32_t buginf_msg(const char *str);











extern void err_msg(const char *_format, ...);











extern void notice_msg(const char *_format, ...);

__END_DECLS

#endif

