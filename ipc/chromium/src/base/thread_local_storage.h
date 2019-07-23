



#ifndef BASE_THREAD_LOCAL_STORAGE_H_
#define BASE_THREAD_LOCAL_STORAGE_H_

#include "base/basictypes.h"

#if defined(OS_POSIX)
#include <pthread.h>
#endif



class ThreadLocalStorage {
 public:

  
  
  
  typedef void (*TLSDestructorFunc)(void* value);

  
  class Slot {
   public:
    Slot(TLSDestructorFunc destructor = NULL);

    
    
    explicit Slot(base::LinkerInitialized x) {}

    
    
    
    
    bool Initialize(TLSDestructorFunc destructor);

    
    
    
    
    void Free();

    
    
    void* Get() const;

    
    
    void Set(void* value);

    bool initialized() const { return initialized_; }

   private:
    
    bool initialized_;
#if defined(OS_WIN)
    int slot_;
#elif defined(OS_POSIX)
    pthread_key_t key_;
#endif

    DISALLOW_COPY_AND_ASSIGN(Slot);
  };

#if defined(OS_WIN)
  
  
  static void ThreadExit();

 private:
  
  static void **Initialize();

 private:
  
  
  
  static const int kThreadLocalStorageSize = 64;

  static long tls_key_;
  static long tls_max_;
  static TLSDestructorFunc tls_destructors_[kThreadLocalStorageSize];
#endif  

  DISALLOW_COPY_AND_ASSIGN(ThreadLocalStorage);
};



typedef ThreadLocalStorage::Slot TLSSlot;

#endif  
