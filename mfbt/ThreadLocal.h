






#ifndef mozilla_ThreadLocal_h_
#define mozilla_ThreadLocal_h_

#if defined(XP_WIN)







extern "C" {
__declspec(dllimport) void* __stdcall TlsGetValue(unsigned long);
__declspec(dllimport) int __stdcall TlsSetValue(unsigned long, void*);
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

























template<typename T>
class ThreadLocal
{
#if defined(XP_WIN)
    typedef unsigned long key_t;
#else
    typedef pthread_key_t key_t;
#endif

    union Helper {
      void* ptr;
      T value;
    };

  public:
    MOZ_WARN_UNUSED_RESULT inline bool init();

    inline T get() const;

    inline void set(const T value);

    bool initialized() const {
      return inited;
    }

  private:
    key_t key;
    bool inited;
};

template<typename T>
inline bool
ThreadLocal<T>::init()
{
  MOZ_STATIC_ASSERT(sizeof(T) <= sizeof(void*),
                    "mozilla::ThreadLocal can't be used for types larger than "
                    "a pointer");
  MOZ_ASSERT(!initialized());
#ifdef XP_WIN
  key = TlsAlloc();
  inited = key != 0xFFFFFFFFUL; 
#else
  inited = !pthread_key_create(&key, NULL);
#endif
  return inited;
}

template<typename T>
inline T
ThreadLocal<T>::get() const
{
  MOZ_ASSERT(initialized());
  Helper h;
#ifdef XP_WIN
  h.ptr = TlsGetValue(key);
#else
  h.ptr = pthread_getspecific(key);
#endif
  return h.value;
}

template<typename T>
inline void
ThreadLocal<T>::set(const T value)
{
  MOZ_ASSERT(initialized());
  Helper h;
  h.value = value;
  bool succeeded;
#ifdef XP_WIN
  succeeded = TlsSetValue(key, h.ptr);
#else
  succeeded = !pthread_setspecific(key, h.ptr);
#endif
  if (!succeeded)
    MOZ_CRASH();
}

} 

#endif 
