



#ifndef BASE_THREADING_THREAD_LOCAL_STORAGE_H_
#define BASE_THREADING_THREAD_LOCAL_STORAGE_H_

#include "base/base_export.h"
#include "base/basictypes.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <pthread.h>
#endif

namespace base {

namespace internal {




class BASE_EXPORT PlatformThreadLocalStorage {
 public:

#if defined(OS_WIN)
  typedef unsigned long TLSKey;
  enum { TLS_KEY_OUT_OF_INDEXES = TLS_OUT_OF_INDEXES };
#elif defined(OS_POSIX)
  typedef pthread_key_t TLSKey;
  
  
  
  
  enum { TLS_KEY_OUT_OF_INDEXES = 0x7FFFFFFF };
#endif

  
  
  
  
  
  
  
  static bool AllocTLS(TLSKey* key);
  
  
  
  static void FreeTLS(TLSKey key);
  static void SetTLSValue(TLSKey key, void* value);
  static void* GetTLSValue(TLSKey key);

  
  
  
  
  
  
  
  
#if defined(OS_WIN)
  
  
  static void OnThreadExit();
#elif defined(OS_POSIX)
  
  
  
  static void OnThreadExit(void* value);
#endif
};

}  



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
    int slot_;
  };

  
  
  class BASE_EXPORT Slot : public StaticSlot {
   public:
    
    explicit Slot(TLSDestructorFunc destructor = NULL);

   private:
    using StaticSlot::initialized_;
    using StaticSlot::slot_;

    DISALLOW_COPY_AND_ASSIGN(Slot);
  };

  DISALLOW_COPY_AND_ASSIGN(ThreadLocalStorage);
};

}  

#endif  
