



#include "base/memory_debug.h"

#ifdef PURIFY


#define PURIFY_PRIVATE_INCLUDE
#include "base/third_party/purify/pure.h"
#endif

namespace base {

bool MemoryDebug::memory_in_use_ = false;

void MemoryDebug::SetMemoryInUseEnabled(bool enabled) {
  memory_in_use_ = enabled;
}

void MemoryDebug::DumpAllMemoryInUse() {
#ifdef PURIFY
  if (memory_in_use_)
    PurifyAllInuse();
#endif
}

void MemoryDebug::DumpNewMemoryInUse() {
#ifdef PURIFY
  if (memory_in_use_)
    PurifyNewInuse();
#endif
}

void MemoryDebug::DumpAllLeaks() {
#ifdef PURIFY
  PurifyAllLeaks();
#endif
}

void MemoryDebug::DumpNewLeaks() {
#ifdef PURIFY
  PurifyNewLeaks();
#endif
}

void MemoryDebug::MarkAsInitialized(void* addr, size_t size) {
#ifdef PURIFY
  PurifyMarkAsInitialized(addr, size);
#endif
}

}  
