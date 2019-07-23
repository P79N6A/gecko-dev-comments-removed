







#ifndef BASE_MEMORY_DEBUG_H_
#define BASE_MEMORY_DEBUG_H_

#include "base/basictypes.h"

namespace base {

class MemoryDebug {
 public:
  
  
  static void SetMemoryInUseEnabled(bool enabled);

  
  static void DumpAllMemoryInUse();
  
  
  static void DumpNewMemoryInUse();

  
  static void DumpAllLeaks();
  
  
  static void DumpNewLeaks();

  
  
  static void MarkAsInitialized(void* addr, size_t size);

 private:
  static bool memory_in_use_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(MemoryDebug);
};

}  

#endif  
