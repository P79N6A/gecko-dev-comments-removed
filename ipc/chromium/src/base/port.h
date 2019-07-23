



#ifndef BASE_PORT_H_
#define BASE_PORT_H_

#include <stdarg.h>
#include "build/build_config.h"

#ifdef COMPILER_MSVC
#define GG_LONGLONG(x) x##I64
#define GG_ULONGLONG(x) x##UI64
#else
#define GG_LONGLONG(x) x##LL
#define GG_ULONGLONG(x) x##ULL
#endif







#define GG_INT8_C(x)    (x)
#define GG_INT16_C(x)   (x)
#define GG_INT32_C(x)   (x)
#define GG_INT64_C(x)   GG_LONGLONG(x)

#define GG_UINT8_C(x)   (x ## U)
#define GG_UINT16_C(x)  (x ## U)
#define GG_UINT32_C(x)  (x ## U)
#define GG_UINT64_C(x)  GG_ULONGLONG(x)

namespace base {







#if !defined(CHROMIUM_MOZILLA_BUILD)
inline void va_copy(va_list& a, va_list& b) {
#if defined(COMPILER_GCC)
  ::va_copy(a, b);
#elif defined(COMPILER_MSVC)
  a = b;
#endif
}

#else


#  if defined(COMPILER_GCC)
#    define base_va_copy(_a, _b) ::va_copy(_a, _b)
#  elif defined(COMPILER_MSVC)
#    define base_va_copy(_a, _b) (_a = _b)
#  else
#    error No va_copy for your compiler
#  endif

#endif

}  


#if defined(OS_WIN)
#define API_CALL __stdcall
#elif defined(OS_LINUX) || defined(OS_MACOSX)
#define API_CALL
#endif

#endif  
