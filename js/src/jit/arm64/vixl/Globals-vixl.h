

























#ifndef VIXL_GLOBALS_H
#define VIXL_GLOBALS_H


#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "mozilla/Assertions.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "js-config.h"

#include "jit/arm64/vixl/Platform-vixl.h"
#include "js/Utility.h"


typedef uint8_t byte;

const int KBytes = 1024;
const int MBytes = 1024 * KBytes;

#define VIXL_ABORT() printf("in %s, line %i", __FILE__, __LINE__); abort()
#ifdef DEBUG
  #define VIXL_ASSERT(condition) MOZ_ASSERT(condition)
  #define VIXL_CHECK(condition) VIXL_ASSERT(condition)
  #define VIXL_UNIMPLEMENTED() printf("UNIMPLEMENTED\t"); VIXL_ABORT()
  #define VIXL_UNREACHABLE() printf("UNREACHABLE\t"); VIXL_ABORT()
#else
  #define VIXL_ASSERT(condition) ((void) 0)
  #define VIXL_CHECK(condition) ((void) 0)
  #define VIXL_UNIMPLEMENTED() ((void) 0)
  #define VIXL_UNREACHABLE() ((void) 0)
#endif



#define VIXL_CONCAT(a, b) a##b
#define VIXL_STATIC_ASSERT_LINE(line, condition) \
  typedef char VIXL_CONCAT(STATIC_ASSERT_LINE_, line)[(condition) ? 1 : -1] \
  __attribute__((unused))
#define VIXL_STATIC_ASSERT(condition) VIXL_STATIC_ASSERT_LINE(__LINE__, condition) //NOLINT

template <typename T> inline void USE(T) {}

#define VIXL_ALIGNMENT_EXCEPTION() printf("ALIGNMENT EXCEPTION\t"); VIXL_ABORT()

#endif  
