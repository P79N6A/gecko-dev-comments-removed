



#ifndef BASE_SINGLETON_H_
#define BASE_SINGLETON_H_

#include "base/at_exit.h"
#include "base/atomicops.h"
#include "base/platform_thread.h"




template<typename Type>
struct DefaultSingletonTraits {
  
  static Type* New() {
    
    
    return new Type();
  }

  
  static void Delete(Type* x) {
    delete x;
  }

  
  
  static const bool kRegisterAtExit = true;
};





template<typename Type>
struct LeakySingletonTraits : public DefaultSingletonTraits<Type> {
  static const bool kRegisterAtExit = false;
};






























































template <typename Type,
          typename Traits = DefaultSingletonTraits<Type>,
          typename DifferentiatingType = Type>
class Singleton {
 public:
  
  

  
  static Type* get() {
    
    
    static const base::subtle::AtomicWord kBeingCreatedMarker = 1;

    base::subtle::AtomicWord value = base::subtle::NoBarrier_Load(&instance_);
    if (value != 0 && value != kBeingCreatedMarker)
      return reinterpret_cast<Type*>(value);

    
    if (base::subtle::Acquire_CompareAndSwap(&instance_,
                                             0,
                                             kBeingCreatedMarker) == 0) {
      
      
      
      Type* newval = Traits::New();
      base::subtle::Release_Store(
          &instance_, reinterpret_cast<base::subtle::AtomicWord>(newval));

      if (Traits::kRegisterAtExit)
        base::AtExitManager::RegisterCallback(OnExit, NULL);

      return newval;
    }

    
    
    
    
    
    
    
    while (true) {
      value = base::subtle::NoBarrier_Load(&instance_);
      if (value != kBeingCreatedMarker)
        break;
      PlatformThread::YieldCurrentThread();
    }

    return reinterpret_cast<Type*>(value);
  }

  
  Type& operator*() {
    return *get();
  }

  Type* operator->() {
    return get();
  }

 private:
  
  
  static void OnExit(void* unused) {
    
    
    Traits::Delete(reinterpret_cast<Type*>(
        base::subtle::NoBarrier_AtomicExchange(&instance_, 0)));
  }
  static base::subtle::AtomicWord instance_;
};

template <typename Type, typename Traits, typename DifferentiatingType>
base::subtle::AtomicWord Singleton<Type, Traits, DifferentiatingType>::
    instance_ = 0;

#endif  
