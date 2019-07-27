



#ifndef BASE_STRINGS_SAFE_SPRINTF_H_
#define BASE_STRINGS_SAFE_SPRINTF_H_

#include "build/build_config.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(OS_POSIX)

#include <unistd.h>
#endif

#include "base/base_export.h"
#include "base/basictypes.h"

namespace base {
namespace strings {

#if defined(_MSC_VER)

#if defined(_WIN64)
typedef __int64 ssize_t;
#else
typedef long ssize_t;
#endif
#endif




































































































namespace internal {




struct Arg {
  enum Type { INT, UINT, STRING, POINTER };

  
  Arg(signed char c) : type(INT) {
    integer.i = c;
    integer.width = sizeof(char);
  }
  Arg(unsigned char c) : type(UINT) {
    integer.i = c;
    integer.width = sizeof(char);
  }
  Arg(signed short j) : type(INT) {
    integer.i = j;
    integer.width = sizeof(short);
  }
  Arg(unsigned short j) : type(UINT) {
    integer.i = j;
    integer.width = sizeof(short);
  }
  Arg(signed int j) : type(INT) {
    integer.i = j;
    integer.width = sizeof(int);
  }
  Arg(unsigned int j) : type(UINT) {
    integer.i = j;
    integer.width = sizeof(int);
  }
  Arg(signed long j) : type(INT) {
    integer.i = j;
    integer.width = sizeof(long);
  }
  Arg(unsigned long j) : type(UINT) {
    integer.i = j;
    integer.width = sizeof(long);
  }
  Arg(signed long long j) : type(INT) {
    integer.i = j;
    integer.width = sizeof(long long);
  }
  Arg(unsigned long long j) : type(UINT) {
    integer.i = j;
    integer.width = sizeof(long long);
  }

  
  Arg(const char* s) : str(s), type(STRING) { }
  Arg(char* s)       : str(s), type(STRING) { }

  
  template<class T> Arg(T* p) : ptr((void*)p), type(POINTER) { }

  union {
    
    struct {
      int64_t       i;
      unsigned char width;
    } integer;

    
    const char* str;

    
    const void* ptr;
  };
  const enum Type type;
};



BASE_EXPORT ssize_t SafeSNPrintf(char* buf, size_t sz, const char* fmt,
                                 const Arg* args, size_t max_args);

#if !defined(NDEBUG)



BASE_EXPORT void SetSafeSPrintfSSizeMaxForTest(size_t max);
BASE_EXPORT size_t GetSafeSPrintfSSizeMaxForTest();
#endif

}  

template<typename... Args>
ssize_t SafeSNPrintf(char* buf, size_t N, const char* fmt, Args... args) {
  
  
  const internal::Arg arg_array[] = { args... };
  return internal::SafeSNPrintf(buf, N, fmt, arg_array, sizeof...(args));
}

template<size_t N, typename... Args>
ssize_t SafeSPrintf(char (&buf)[N], const char* fmt, Args... args) {
  
  
  const internal::Arg arg_array[] = { args... };
  return internal::SafeSNPrintf(buf, N, fmt, arg_array, sizeof...(args));
}


BASE_EXPORT ssize_t SafeSNPrintf(char* buf, size_t N, const char* fmt);
template<size_t N>
inline ssize_t SafeSPrintf(char (&buf)[N], const char* fmt) {
  return SafeSNPrintf(buf, N, fmt);
}

}  
}  

#endif  
