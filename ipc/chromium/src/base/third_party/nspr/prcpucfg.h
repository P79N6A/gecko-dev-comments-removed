




























#ifndef BASE_THIRD_PARTY_NSPR_PRCPUCFG_H__
#define BASE_THIRD_PARTY_NSPR_PRCPUCFG_H__

#if defined(WIN32)
#include "base/third_party/nspr/prcpucfg_win.h"
#elif defined(__APPLE__)
#include "base/third_party/nspr/prcpucfg_mac.h"
#elif defined(__linux__) || defined(ANDROID)
#include "base/third_party/nspr/prcpucfg_linux.h"
#elif defined(__OpenBSD__)
#include "base/third_party/nspr/prcpucfg_openbsd.h"
#else
#error Provide a prcpucfg.h appropriate for your platform
#endif

#endif  
