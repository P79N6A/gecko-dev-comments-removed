





#ifndef CHROME_COMMON_ENV_VARS_H__
#define CHROME_COMMON_ENV_VARS_H__

#if defined(COMPILER_MSVC)
#include <string.h>
#endif

namespace env_vars {

extern const wchar_t kHeadless[];
extern const wchar_t kLogFileName[];
extern const wchar_t kShowRestart[];
extern const wchar_t kRestartInfo[];
extern const wchar_t kRtlLocale[];
extern const wchar_t kLtrLocale[];
extern const wchar_t kNoOOBreakpad[];

}  

#endif  
