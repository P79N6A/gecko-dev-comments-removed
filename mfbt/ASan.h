









#ifndef mozilla_ASan_h_
#define mozilla_ASan_h_

#ifdef MOZ_ASAN
#include <stddef.h>

extern "C" {
  void __asan_poison_memory_region(void const volatile *addr, size_t size)
    __attribute__((visibility("default")));
  void __asan_unpoison_memory_region(void const volatile *addr, size_t size)
    __attribute__((visibility("default")));
#define ASAN_POISON_MEMORY_REGION(addr, size)   \
  __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
}
#endif

#endif  
