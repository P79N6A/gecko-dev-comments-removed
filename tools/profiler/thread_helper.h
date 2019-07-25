







































#if defined(XP_WIN)
  
  
  
  
  
  
  
  
extern "C" {
__declspec(dllimport) void * __stdcall TlsGetValue(unsigned long);
__declspec(dllimport) int __stdcall TlsSetValue(unsigned long, void *);
__declspec(dllimport) unsigned long __stdcall TlsAlloc();
};
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

typedef unsigned long key;

template <typename T>
inline T* get(key mykey) {
  return (T*) TlsGetValue(mykey);
}

template <typename T>
inline bool set(key mykey, const T* value) {
  return TlsSetValue(mykey, const_cast<T*>(value));
}

inline bool create(key* mykey) {
  key newkey = TlsAlloc();
  if (newkey == (unsigned long)0xFFFFFFFF ) {
    return false;
  }
  *mykey = newkey;
  return true;
}

#else

typedef pthread_key_t key;

template <typename T>
inline T* get(key mykey) {
  return (T*) pthread_getspecific(mykey);
}

template <typename T>
inline bool set(key mykey, const T* value) {
  return !pthread_setspecific(mykey, value);
}

inline bool create(key* mykey) {
  return !pthread_key_create(mykey, NULL);
}

#endif

}

}
 
