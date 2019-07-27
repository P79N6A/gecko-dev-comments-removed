

























#ifndef VIXL_CPU_A64_H
#define VIXL_CPU_A64_H

#include "jit/arm64/vixl/Globals-vixl.h"
#include "jit/arm64/vixl/Instructions-vixl.h"

namespace vixl {

class CPU {
 public:
  
  static void SetUp();

  
  
  
  
  static void EnsureIAndDCacheCoherency(void *address, size_t length);

  
  template <typename T>
  static T SetPointerTag(T pointer, uint64_t tag) {
    VIXL_ASSERT(is_uintn(kAddressTagWidth, tag));

    
    

    uint64_t raw = (uint64_t)pointer;
    VIXL_STATIC_ASSERT(sizeof(pointer) == sizeof(raw));

    raw = (raw & ~kAddressTagMask) | (tag << kAddressTagOffset);
    return (T)raw;
  }

  template <typename T>
  static uint64_t GetPointerTag(T pointer) {
    
    

    uint64_t raw = (uint64_t)pointer;
    VIXL_STATIC_ASSERT(sizeof(pointer) == sizeof(raw));

    return (raw & kAddressTagMask) >> kAddressTagOffset;
  }

 private:
  
  static uint32_t GetCacheType();

  
  static unsigned icache_line_size_;
  static unsigned dcache_line_size_;
};

}  

#endif  
