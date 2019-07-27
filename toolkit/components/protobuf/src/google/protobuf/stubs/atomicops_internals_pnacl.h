































#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_PNACL_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_PNACL_H_

namespace google {
namespace protobuf {
namespace internal {

inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  return __sync_val_compare_and_swap(ptr, old_value, new_value);
}

inline void MemoryBarrier() {
  __sync_synchronize();
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  Atomic32 ret = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  MemoryBarrier();
  return ret;
}

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  MemoryBarrier();
  *ptr = value;
}

inline Atomic32 Acquire_Load(volatile const Atomic32* ptr) {
  Atomic32 value = *ptr;
  MemoryBarrier();
  return value;
}

}  
}  
}  

#endif  
