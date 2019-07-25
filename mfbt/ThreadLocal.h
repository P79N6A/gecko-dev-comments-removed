





#ifndef mozilla_TLS_h_
#define mozilla_TLS_h_

#if defined(XP_WIN)







extern "C" {
__declspec(dllimport) void * __stdcall TlsGetValue(unsigned long);
__declspec(dllimport) int __stdcall TlsSetValue(unsigned long, void *);
__declspec(dllimport) unsigned long __stdcall TlsAlloc();
}
#else
#  include <pthread.h>
#  include <signal.h>
#endif

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

namespace mozilla {




#if defined(XP_WIN)
typedef unsigned long sig_safe_t;
#else
typedef sig_atomic_t sig_safe_t;
#endif

























template <typename T>
class ThreadLocal
{
#if defined(XP_WIN)
    typedef unsigned long key_t;
#else
    typedef pthread_key_t key_t;
#endif

  public:
    MOZ_WARN_UNUSED_RESULT inline bool init();

    inline T* get() const;

    inline bool set(const T* value);

    bool initialized() const {
      return inited;
    }

  private:
    key_t key;
    bool inited;
};

template <typename T>
inline bool
ThreadLocal<T>::init() {
  MOZ_ASSERT(!initialized());
#ifdef XP_WIN
  key = TlsAlloc();
  inited = key != 0xFFFFFFFFUL; 
#else
  inited = !pthread_key_create(&key, NULL);
#endif
  return inited;
}

template <typename T>
inline T*
ThreadLocal<T>::get() const {
  MOZ_ASSERT(initialized());
#ifdef XP_WIN
  return reinterpret_cast<T*>(TlsGetValue(key));
#else
  return reinterpret_cast<T*>(pthread_getspecific(key));
#endif
}

template <typename T>
inline bool
ThreadLocal<T>::set(const T* value) {
  MOZ_ASSERT(initialized());
#ifdef XP_WIN
  return TlsSetValue(key, const_cast<T*>(value));
#else
  return !pthread_setspecific(key, value);
#endif
}

} 

#endif 
