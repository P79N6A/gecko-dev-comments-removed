

































#ifndef BASE_LAZY_INSTANCE_H_
#define BASE_LAZY_INSTANCE_H_

#include <new>  

#include "base/atomicops.h"
#include "base/basictypes.h"
#include "base/logging.h"

#include "mozilla/Alignment.h"





#define LAZY_INSTANCE_INITIALIZER {0}

namespace base {

template <typename Type>
struct DefaultLazyInstanceTraits {
  static const bool kRegisterOnExit = true;

  static Type* New(void* instance) {
    DCHECK_EQ(reinterpret_cast<uintptr_t>(instance) & (MOZ_ALIGNOF(Type) - 1), 0u)
        << ": Bad boy, the buffer passed to placement new is not aligned!\n"
        "This may break some stuff like SSE-based optimizations assuming the "
        "<Type> objects are word aligned.";
    
    
    return new (instance) Type();
  }
  static void Delete(Type* instance) {
    
    instance->~Type();
  }
};



namespace internal {









template <typename Type>
struct LeakyLazyInstanceTraits {
  static const bool kRegisterOnExit = false;

  static Type* New(void* instance) {
    return DefaultLazyInstanceTraits<Type>::New(instance);
  }
  static void Delete(Type* instance) {
  }
};



static const subtle::AtomicWord kLazyInstanceStateCreating = 1;




bool NeedsLazyInstance(subtle::AtomicWord* state);



void CompleteLazyInstance(subtle::AtomicWord* state,
                                      subtle::AtomicWord new_instance,
                                      void* lazy_instance,
                                      void (*dtor)(void*));

}  

template <typename Type, typename Traits = DefaultLazyInstanceTraits<Type> >
class LazyInstance {
 public:
  
  
  
  
  
  

  
  
  typedef LazyInstance<Type, internal::LeakyLazyInstanceTraits<Type> > Leaky;

  Type& Get() {
    return *Pointer();
  }

  Type* Pointer() {
    
    
    static const subtle::AtomicWord kLazyInstanceCreatedMask =
        ~internal::kLazyInstanceStateCreating;

    
    
    
    
    
    
    
    subtle::AtomicWord value = subtle::Acquire_Load(&private_instance_);
    if (!(value & kLazyInstanceCreatedMask) &&
        internal::NeedsLazyInstance(&private_instance_)) {
      
      value = reinterpret_cast<subtle::AtomicWord>(
          Traits::New(private_buf_.addr()));
      internal::CompleteLazyInstance(&private_instance_, value, this,
                                     Traits::kRegisterOnExit ? OnExit : NULL);
    }

    return instance();
  }

  bool operator==(Type* p) {
    switch (subtle::NoBarrier_Load(&private_instance_)) {
      case 0:
        return p == NULL;
      case internal::kLazyInstanceStateCreating:
        return static_cast<void*>(p) == private_buf_.addr();
      default:
        return p == instance();
    }
  }

  
  
  

  subtle::AtomicWord private_instance_;
  
  mozilla::AlignedStorage2<Type> private_buf_;

 private:
  Type* instance() {
    return reinterpret_cast<Type*>(subtle::NoBarrier_Load(&private_instance_));
  }

  
  
  
  static void OnExit(void* lazy_instance) {
    LazyInstance<Type, Traits>* me =
        reinterpret_cast<LazyInstance<Type, Traits>*>(lazy_instance);
    Traits::Delete(me->instance());
    subtle::NoBarrier_Store(&me->private_instance_, 0);
  }
};

}  

#endif  
