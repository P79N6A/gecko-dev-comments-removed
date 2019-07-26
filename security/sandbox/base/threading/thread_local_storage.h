



#ifndef BASE_THREADING_THREAD_LOCAL_STORAGE_H_
#define BASE_THREADING_THREAD_LOCAL_STORAGE_H_

#include "base/base_export.h"
#include "base/basictypes.h"

#if defined(OS_POSIX)
#include <pthread.h>
#endif

namespace base {



class BASE_EXPORT ThreadLocalStorage {
 public:

  
  
  
  typedef void (*TLSDestructorFunc)(void* value);

  
  
  
  
  #define TLS_INITIALIZER {0}

  
  
  
  
  
  struct BASE_EXPORT StaticSlot {
    
    
    
    
    bool Initialize(TLSDestructorFunc destructor);

    
    
    
    
    void Free();

    
    
    void* Get() const;

    
    
    void Set(void* value);

    bool initialized() const { return initialized_; }

    
    bool initialized_;
#if defined(OS_WIN)
    int slot_;
#elif defined(OS_POSIX)
    pthread_key_t key_;
#endif

  };

  
  
  class BASE_EXPORT Slot : public StaticSlot {
   public:
    
    explicit Slot(TLSDestructorFunc destructor = NULL);

   private:
    using StaticSlot::initialized_;
#if defined(OS_WIN)
    using StaticSlot::slot_;
#elif defined(OS_POSIX)
    using StaticSlot::key_;
#endif
    DISALLOW_COPY_AND_ASSIGN(Slot);
  };

  DISALLOW_COPY_AND_ASSIGN(ThreadLocalStorage);
};

}  

#endif  
