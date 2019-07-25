







































#if defined(XP_WIN)
  
  
  
  
# include <windef.h>
# include <winbase.h>
#else
# include <pthread.h>
# include <signal.h>
#endif

namespace mozilla {

#if defined(XP_WIN)
typedef unsigned long sig_safe_t;
#else
typedef sig_atomic_t sig_safe_t;
#endif

namespace tls {

#if defined(XP_WIN)

typedef DWORD key;

template <typename T>
static T* get(key mykey) {
  return (T*) TlsGetValue(mykey);
}

template <typename T>
static bool set(key mykey, const T* value) {
  return TlsSetValue(mykey, const_cast<T*>(value));
}

static inline bool create(key* mykey) {
  key newkey = TlsAlloc();
  if (newkey == TLS_OUT_OF_INDEXES) {
    return false;
  }
  *mykey = newkey;
  return true;
}

#else

typedef pthread_key_t key;

template <typename T>
static T* get(key mykey) {
  return (T*) pthread_getspecific(mykey);
}

template <typename T>
static bool set(key mykey, const T* value) {
  return !pthread_setspecific(mykey, value);
}

static bool create(key* mykey) {
  return !pthread_key_create(mykey, NULL);
}

#endif

}

}
 
