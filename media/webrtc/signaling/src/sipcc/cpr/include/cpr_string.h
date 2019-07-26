






































#ifndef _CPR_STRING_H_
#define _CPR_STRING_H_

#include "cpr_types.h"
#include "cpr_strings.h"

__BEGIN_DECLS

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_string.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_string.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_string.h"
#endif

__END_DECLS

#endif
