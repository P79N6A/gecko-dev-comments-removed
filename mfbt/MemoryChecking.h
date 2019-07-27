





















#ifndef mozilla_MemoryChecking_h
#define mozilla_MemoryChecking_h

#if defined(MOZ_VALGRIND)
#include "valgrind/memcheck.h"
#endif

#if defined(MOZ_ASAN) || defined(MOZ_VALGRIND)
#define MOZ_HAVE_MEM_CHECKS 1
#endif

#if defined(MOZ_ASAN)
#include <stddef.h>

#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#ifdef _MSC_VER


#define MOZ_ASAN_VISIBILITY
#else
#define MOZ_ASAN_VISIBILITY MOZ_EXPORT
#endif

extern "C" {



void MOZ_ASAN_VISIBILITY
__asan_poison_memory_region(void const volatile *addr, size_t size);
void MOZ_ASAN_VISIBILITY
__asan_unpoison_memory_region(void const volatile *addr, size_t size);

#define MOZ_MAKE_MEM_NOACCESS(addr, size) \
  __asan_poison_memory_region((addr), (size))

#define MOZ_MAKE_MEM_UNDEFINED(addr, size) \
  __asan_unpoison_memory_region((addr), (size))

#define MOZ_MAKE_MEM_DEFINED(addr, size) \
  __asan_unpoison_memory_region((addr), (size))





void MOZ_EXPORT
__lsan_ignore_object(const void *p);

}
#elif defined(MOZ_MSAN)
#include <stddef.h>

#include "mozilla/Types.h"

extern "C" {



void MOZ_EXPORT
__msan_poison(void const volatile *addr, size_t size);
void MOZ_EXPORT
__msan_unpoison(void const volatile *addr, size_t size);

#define MOZ_MAKE_MEM_NOACCESS(addr, size) \
  __msan_poison((addr), (size))

#define MOZ_MAKE_MEM_UNDEFINED(addr, size) \
  __msan_poison((addr), (size))

#define MOZ_MAKE_MEM_DEFINED(addr, size) \
  __msan_unpoison((addr), (size))
}
#elif defined(MOZ_VALGRIND)
#define MOZ_MAKE_MEM_NOACCESS(addr, size) \
  VALGRIND_MAKE_MEM_NOACCESS((addr), (size))

#define MOZ_MAKE_MEM_UNDEFINED(addr, size) \
  VALGRIND_MAKE_MEM_UNDEFINED((addr), (size))

#define MOZ_MAKE_MEM_DEFINED(addr, size) \
  VALGRIND_MAKE_MEM_DEFINED((addr), (size))
#else

#define MOZ_MAKE_MEM_NOACCESS(addr, size) do {} while (0)
#define MOZ_MAKE_MEM_UNDEFINED(addr, size) do {} while (0)
#define MOZ_MAKE_MEM_DEFINED(addr, size) do {} while (0)

#endif









#if defined(MOZ_ASAN)
#  define MOZ_LSAN_INTENTIONALLY_LEAK_OBJECT(X) __lsan_ignore_object(X)
#else
#  define MOZ_LSAN_INTENTIONALLY_LEAK_OBJECT(X)
#endif 


#endif 
