




















#ifndef mozilla_MemoryChecking_h_
#define mozilla_MemoryChecking_h_

#if defined(MOZ_VALGRIND)
#include "valgrind/memcheck.h"
#endif

#if defined(MOZ_ASAN) || defined(MOZ_VALGRIND)
#define MOZ_HAVE_MEM_CHECKS 1
#endif

#if defined(MOZ_ASAN)
#include <stddef.h>

extern "C" {
  


  void __asan_poison_memory_region(void const volatile *addr, size_t size)
    __attribute__((visibility("default")));
  void __asan_unpoison_memory_region(void const volatile *addr, size_t size)
    __attribute__((visibility("default")));

#define MOZ_MAKE_MEM_NOACCESS(addr, size) \
  __asan_poison_memory_region((addr), (size))

#define MOZ_MAKE_MEM_UNDEFINED(addr, size) \
  __asan_unpoison_memory_region((addr), (size))

#define MOZ_MAKE_MEM_DEFINED(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
}
#elif defined(MOZ_VALGRIND)
#define MOZ_MAKE_MEM_NOACCESS(addr, size) \
  VALGRIND_MAKE_MEM_NOACCESS((addr), (size))

#define MOZ_MAKE_MEM_UNDEFINED(addr, size) \
  VALGRIND_MAKE_MEM_UNDEFINED((addr), (size))

#define MOZ_MAKE_MEM_DEFINED(addr, size) \
  VALGRIND_MAKE_MEM_DEFINED((addr), (size))
#else

#define MOZ_MAKE_MEM_NOACCESS(addr, size) do {} while(0)
#define MOZ_MAKE_MEM_UNDEFINED(addr, size) do {} while(0)
#define MOZ_MAKE_MEM_DEFINED(addr, size) do {} while(0)

#endif

#endif  
