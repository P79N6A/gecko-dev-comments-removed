














































#ifndef BASE_THREAD_LOCAL_H_
#define BASE_THREAD_LOCAL_H_

#include "base/basictypes.h"

#if defined(OS_POSIX)
#include <pthread.h>
#endif

namespace base {


struct ThreadLocalPlatform {
#if defined(OS_WIN)
  typedef int SlotType;
#elif defined(OS_POSIX)
  typedef pthread_key_t SlotType;
#endif

  static void AllocateSlot(SlotType& slot);
  static void FreeSlot(SlotType& slot);
  static void* GetValueFromSlot(SlotType& slot);
  static void SetValueInSlot(SlotType& slot, void* value);
};

template <typename Type>
class ThreadLocalPointer {
 public:
  ThreadLocalPointer() : slot_() {
    ThreadLocalPlatform::AllocateSlot(slot_);
  }

  ~ThreadLocalPointer() {
    ThreadLocalPlatform::FreeSlot(slot_);
  }

  Type* Get() {
    return static_cast<Type*>(ThreadLocalPlatform::GetValueFromSlot(slot_));
  }

  void Set(Type* ptr) {
    ThreadLocalPlatform::SetValueInSlot(slot_, ptr);
  }

 private:
  typedef ThreadLocalPlatform::SlotType SlotType;

  SlotType slot_;

  DISALLOW_COPY_AND_ASSIGN(ThreadLocalPointer<Type>);
};

class ThreadLocalBoolean {
 public:
  ThreadLocalBoolean() { }
  ~ThreadLocalBoolean() { }

  bool Get() {
    return tlp_.Get() != NULL;
  }

  void Set(bool val) {
    tlp_.Set(reinterpret_cast<void*>(val ? 1 : 0));
  }

 private:
  ThreadLocalPointer<void> tlp_;

  DISALLOW_COPY_AND_ASSIGN(ThreadLocalBoolean);
};

}  

#endif  
