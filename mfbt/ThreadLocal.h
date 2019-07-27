







#ifndef mozilla_ThreadLocal_h
#define mozilla_ThreadLocal_h

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
#include "mozilla/NullPtr.h"
#include "mozilla/TypeTraits.h"

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

  
  
  
  
  template<typename S>
  struct Helper
  {
    typedef uintptr_t Type;
  };

  template<typename S>
  struct Helper<S *>
  {
    typedef S *Type;
  };

public:
  MOZ_WARN_UNUSED_RESULT inline bool init();

  inline T get() const;

  inline void set(const T aValue);

  bool initialized() const { return mInited; }

private:
  key_t mKey;
  bool mInited;
};

template<typename T>
inline bool
ThreadLocal<T>::init()
{
  static_assert(mozilla::IsPointer<T>::value || mozilla::IsIntegral<T>::value,
                "mozilla::ThreadLocal must be used with a pointer or "
                "integral type");
  static_assert(sizeof(T) <= sizeof(void*),
                "mozilla::ThreadLocal can't be used for types larger than "
                "a pointer");
  MOZ_ASSERT(!initialized());
#ifdef XP_WIN
  mKey = TlsAlloc();
  mInited = mKey != 0xFFFFFFFFUL; 
#else
  mInited = !pthread_key_create(&mKey, nullptr);
#endif
  return mInited;
}

template<typename T>
inline T
ThreadLocal<T>::get() const
{
  MOZ_ASSERT(initialized());
  void* h;
#ifdef XP_WIN
  h = TlsGetValue(mKey);
#else
  h = pthread_getspecific(mKey);
#endif
  return static_cast<T>(reinterpret_cast<typename Helper<T>::Type>(h));
}

template<typename T>
inline void
ThreadLocal<T>::set(const T aValue)
{
  MOZ_ASSERT(initialized());
  void* h = reinterpret_cast<void*>(static_cast<typename Helper<T>::Type>(aValue));
#ifdef XP_WIN
  bool succeeded = TlsSetValue(mKey, h);
#else
  bool succeeded = !pthread_setspecific(mKey, h);
#endif
  if (!succeeded) {
    MOZ_CRASH();
  }
}

} 

#endif 
