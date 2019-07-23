





#ifndef BASE_BASE_SWITCHES_H_
#define BASE_BASE_SWITCHES_H_

#if defined(CHROMIUM_MOZILLA_BUILD) && defined(COMPILER_MSVC)
#include <string.h>
#endif

namespace switches {

extern const wchar_t kDebugOnStart[];
extern const wchar_t kWaitForDebugger[];
extern const wchar_t kDisableBreakpad[];
extern const wchar_t kFullMemoryCrashReport[];
extern const wchar_t kNoErrorDialogs[];
extern const wchar_t kProcessType[];
extern const wchar_t kEnableDCHECK[];
extern const wchar_t kForceHTTPS[];

}  

#endif  
