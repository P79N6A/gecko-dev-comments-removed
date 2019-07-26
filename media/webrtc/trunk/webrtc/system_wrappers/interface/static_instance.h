









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_STATIC_INSTANCE_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_STATIC_INSTANCE_H_

#include <assert.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#ifdef _WIN32
#include "webrtc/system_wrappers/interface/fix_interlocked_exchange_pointer_win.h"
#endif

namespace webrtc {

enum CountOperation {
  kRelease,
  kAddRef,
  kAddRefNoCreate
};
enum CreateOperation {
  kInstanceExists,
  kCreate,
  kDestroy
};

template <class T>


static T* GetStaticInstance(CountOperation count_operation) {
  
  static volatile long instance_count = 0;
  static T* volatile instance = NULL;
  CreateOperation state = kInstanceExists;
#ifndef _WIN32
  
  
  
  
  
  static CriticalSectionWrapper* crit_sect(
    CriticalSectionWrapper::CreateCriticalSection());
  CriticalSectionScoped lock(crit_sect);

  if (count_operation ==
      kAddRefNoCreate && instance_count == 0) {
    return NULL;
  }
  if (count_operation ==
      kAddRef ||
      count_operation == kAddRefNoCreate) {
    instance_count++;
    if (instance_count == 1) {
      state = kCreate;
    }
  } else {
    instance_count--;
    if (instance_count == 0) {
      state = kDestroy;
    }
  }
  if (state == kCreate) {
    instance = T::CreateInstance();
  } else if (state == kDestroy) {
    T* old_instance = instance;
    instance = NULL;
    
    
    
    
    
    
    crit_sect->Leave();
    if (old_instance) {
      delete old_instance;
    }
    
    
    crit_sect->Enter();
    return NULL;
  }
#else  
  if (count_operation ==
      kAddRefNoCreate && instance_count == 0) {
    return NULL;
  }
  if (count_operation == kAddRefNoCreate) {
    if (1 == InterlockedIncrement(&instance_count)) {
      
      InterlockedDecrement(&instance_count);
      assert(false);
      return NULL;
    }
    
    if (instance == NULL) {
      assert(false);
      InterlockedDecrement(&instance_count);
      return NULL;
    }
  } else if (count_operation == kAddRef) {
    if (instance_count == 0) {
      state = kCreate;
    } else {
      if (1 == InterlockedIncrement(&instance_count)) {
        
        
        InterlockedDecrement(&instance_count);
        state = kCreate;
      }
    }
  } else {
    int new_value = InterlockedDecrement(&instance_count);
    if (new_value == 0) {
      state = kDestroy;
    }
  }

  if (state == kCreate) {
    
    
    
    T* new_instance = T::CreateInstance();
    if (1 == InterlockedIncrement(&instance_count)) {
      InterlockedExchangePointer(reinterpret_cast<void * volatile*>(&instance),
                                 new_instance);
    } else {
      InterlockedDecrement(&instance_count);
      if (new_instance) {
        delete static_cast<T*>(new_instance);
      }
    }
  } else if (state == kDestroy) {
    T* old_value = static_cast<T*>(InterlockedExchangePointer(
        reinterpret_cast<void * volatile*>(&instance), NULL));
    if (old_value) {
      delete static_cast<T*>(old_value);
    }
    return NULL;
  }
#endif  
  return instance;
}

}  

#endif  
