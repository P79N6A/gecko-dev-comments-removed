

















#ifndef BASE_MEMORY_SINGLETON_H_
#define BASE_MEMORY_SINGLETON_H_

#include "base/at_exit.h"
#include "base/atomicops.h"
#include "base/base_export.h"
#include "base/memory/aligned_memory.h"
#include "base/third_party/dynamic_annotations/dynamic_annotations.h"
#include "base/threading/thread_restrictions.h"

namespace base {
namespace internal {



static const subtle::AtomicWord kBeingCreatedMarker = 1;



BASE_EXPORT subtle::AtomicWord WaitForInstance(subtle::AtomicWord* instance);

}  
}  






template<typename Type>
struct DefaultSingletonTraits {
  
  static Type* New() {
    
    
    return new Type();
  }

  
  static void Delete(Type* x) {
    delete x;
  }

  
  
  static const bool kRegisterAtExit = true;

  
  
  
  static const bool kAllowedToAccessOnNonjoinableThread = false;
};





template<typename Type>
struct LeakySingletonTraits : public DefaultSingletonTraits<Type> {
  static const bool kRegisterAtExit = false;
  static const bool kAllowedToAccessOnNonjoinableThread = true;
};























template <typename Type>
struct StaticMemorySingletonTraits {
  
  
  static Type* New() {
    
    if (base::subtle::NoBarrier_AtomicExchange(&dead_, 1))
      return NULL;

    return new(buffer_.void_data()) Type();
  }

  static void Delete(Type* p) {
    if (p != NULL)
      p->Type::~Type();
  }

  static const bool kRegisterAtExit = true;
  static const bool kAllowedToAccessOnNonjoinableThread = true;

  
  static void Resurrect() {
    base::subtle::NoBarrier_Store(&dead_, 0);
  }

 private:
  static base::AlignedMemory<sizeof(Type), ALIGNOF(Type)> buffer_;
  
  static base::subtle::Atomic32 dead_;
};

template <typename Type> base::AlignedMemory<sizeof(Type), ALIGNOF(Type)>
    StaticMemorySingletonTraits<Type>::buffer_;
template <typename Type> base::subtle::Atomic32
    StaticMemorySingletonTraits<Type>::dead_ = 0;






































































template <typename Type,
          typename Traits = DefaultSingletonTraits<Type>,
          typename DifferentiatingType = Type>
class Singleton {
 private:
  
  
  friend Type* Type::GetInstance();

  
  friend class DeleteTraceLogForTesting;

  
  

  
  static Type* get() {
#ifndef NDEBUG
    
    if (!Traits::kAllowedToAccessOnNonjoinableThread)
      base::ThreadRestrictions::AssertSingletonAllowed();
#endif

    base::subtle::AtomicWord value = base::subtle::NoBarrier_Load(&instance_);
    if (value != 0 && value != base::internal::kBeingCreatedMarker) {
      
      ANNOTATE_HAPPENS_AFTER(&instance_);
      return reinterpret_cast<Type*>(value);
    }

    
    if (base::subtle::Acquire_CompareAndSwap(
          &instance_, 0, base::internal::kBeingCreatedMarker) == 0) {
      
      
      
      Type* newval = Traits::New();

      
      
      
      ANNOTATE_HAPPENS_BEFORE(&instance_);
      base::subtle::Release_Store(
          &instance_, reinterpret_cast<base::subtle::AtomicWord>(newval));

      if (newval != NULL && Traits::kRegisterAtExit)
        base::AtExitManager::RegisterCallback(OnExit, NULL);

      return newval;
    }

    
    value = base::internal::WaitForInstance(&instance_);

    
    ANNOTATE_HAPPENS_AFTER(&instance_);
    return reinterpret_cast<Type*>(value);
  }

  
  
  
  static void OnExit(void* ) {
    
    
    Traits::Delete(
        reinterpret_cast<Type*>(base::subtle::NoBarrier_Load(&instance_)));
    instance_ = 0;
  }
  static base::subtle::AtomicWord instance_;
};

template <typename Type, typename Traits, typename DifferentiatingType>
base::subtle::AtomicWord Singleton<Type, Traits, DifferentiatingType>::
    instance_ = 0;

#endif  
