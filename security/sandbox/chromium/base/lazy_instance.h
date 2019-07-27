

































#ifndef BASE_LAZY_INSTANCE_H_
#define BASE_LAZY_INSTANCE_H_

#include <new>  

#include "base/atomicops.h"
#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/debug/leak_annotations.h"
#include "base/logging.h"
#include "base/memory/aligned_memory.h"
#include "base/threading/thread_restrictions.h"





#define LAZY_INSTANCE_INITIALIZER {0}

namespace base {

template <typename Type>
struct DefaultLazyInstanceTraits {
  static const bool kRegisterOnExit = true;
#ifndef NDEBUG
  static const bool kAllowedToAccessOnNonjoinableThread = false;
#endif

  static Type* New(void* instance) {
    DCHECK_EQ(reinterpret_cast<uintptr_t>(instance) & (ALIGNOF(Type) - 1), 0u)
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
#ifndef NDEBUG
  static const bool kAllowedToAccessOnNonjoinableThread = true;
#endif

  static Type* New(void* instance) {
    ANNOTATE_SCOPED_MEMORY_LEAK;
    return DefaultLazyInstanceTraits<Type>::New(instance);
  }
  static void Delete(Type* instance) {
  }
};



static const subtle::AtomicWord kLazyInstanceStateCreating = 1;




BASE_EXPORT bool NeedsLazyInstance(subtle::AtomicWord* state);



BASE_EXPORT void CompleteLazyInstance(subtle::AtomicWord* state,
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
#ifndef NDEBUG
    
    if (!Traits::kAllowedToAccessOnNonjoinableThread)
      ThreadRestrictions::AssertSingletonAllowed();
#endif
    
    
    static const subtle::AtomicWord kLazyInstanceCreatedMask =
        ~internal::kLazyInstanceStateCreating;

    
    
    
    
    
    
    
    subtle::AtomicWord value = subtle::Acquire_Load(&private_instance_);
    if (!(value & kLazyInstanceCreatedMask) &&
        internal::NeedsLazyInstance(&private_instance_)) {
      
      value = reinterpret_cast<subtle::AtomicWord>(
          Traits::New(private_buf_.void_data()));
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
        return static_cast<void*>(p) == private_buf_.void_data();
      default:
        return p == instance();
    }
  }

  
  
  

  subtle::AtomicWord private_instance_;
  
  base::AlignedMemory<sizeof(Type), ALIGNOF(Type)> private_buf_;

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
