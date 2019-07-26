









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ALIGNED_MALLOC_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ALIGNED_MALLOC_H_







#include <stddef.h>

#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {





void* GetRightAlign(const void* ptr, size_t alignment);




void* AlignedMalloc(size_t size, size_t alignment);

void AlignedFree(void* mem_block);



template<typename T>
T* GetRightAlign(const T* ptr, size_t alignment) {
  return reinterpret_cast<T*>(GetRightAlign(reinterpret_cast<const void*>(ptr),
                                            alignment));
}
template<typename T>
T* AlignedMalloc(size_t size, size_t alignment) {
  return reinterpret_cast<T*>(AlignedMalloc(size, alignment));
}


template<typename T>
struct Allocator {
  typedef scoped_ptr_malloc<T, AlignedFree> scoped_ptr_aligned;
};

}  

#endif 
