

































#ifndef BASE_LAZY_INSTANCE_H_
#define BASE_LAZY_INSTANCE_H_

#include "base/atomicops.h"
#include "base/basictypes.h"

namespace base {

template <typename Type>
struct DefaultLazyInstanceTraits {
  static void New(void* instance) {
    
    
    new (instance) Type();
  }
  static void Delete(void* instance) {
    
    reinterpret_cast<Type*>(instance)->~Type();
  }
};



class LazyInstanceHelper {
 protected:
  enum {
    STATE_EMPTY    = 0,
    STATE_CREATING = 1,
    STATE_CREATED  = 2
  };

  explicit LazyInstanceHelper(LinkerInitialized x) {  }
  
  

  
  
  
  void EnsureInstance(void* instance, void (*ctor)(void*), void (*dtor)(void*));

  base::subtle::Atomic32 state_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LazyInstanceHelper);
};

template <typename Type, typename Traits = DefaultLazyInstanceTraits<Type> >
class LazyInstance : public LazyInstanceHelper {
 public:
  explicit LazyInstance(LinkerInitialized x) : LazyInstanceHelper(x) { }
  
  

  Type& Get() {
    return *Pointer();
  }

  Type* Pointer() {
    Type* instance = reinterpret_cast<Type*>(&buf_);

    
    if (base::subtle::NoBarrier_Load(&state_) != STATE_CREATED)
      EnsureInstance(instance, Traits::New, Traits::Delete);

    return instance;
  }

 private:
  int8 buf_[sizeof(Type)];  

  DISALLOW_COPY_AND_ASSIGN(LazyInstance);
};

}  

#endif
