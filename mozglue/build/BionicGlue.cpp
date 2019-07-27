




#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <android/log.h>
#include <sys/syscall.h>

#include "mozilla/Alignment.h"

#include <vector>

#define NS_EXPORT __attribute__ ((visibility("default")))

#if ANDROID_VERSION < 17 || defined(MOZ_WIDGET_ANDROID)

struct AtForkFuncs {
  void (*prepare)(void);
  void (*parent)(void);
  void (*child)(void);
};







template <typename T>
struct SpecialAllocator: public std::allocator<T>
{
  SpecialAllocator(): bufUsed(false) {}

  inline typename std::allocator<T>::pointer allocate(typename std::allocator<T>::size_type n, const void * = 0) {
    if (!bufUsed && n == 1) {
      bufUsed = true;
      return buf.addr();
    }
    return reinterpret_cast<T *>(::operator new(sizeof(T) * n));
  }

  inline void deallocate(typename std::allocator<T>::pointer p, typename std::allocator<T>::size_type n) {
    if (p == buf.addr())
      bufUsed = false;
    else
      ::operator delete(p);
  }

  template<typename U>
  struct rebind {
    typedef SpecialAllocator<U> other;
  };

private:
  mozilla::AlignedStorage2<T> buf;
  bool bufUsed;
};

static std::vector<AtForkFuncs, SpecialAllocator<AtForkFuncs> > atfork;
#endif

#ifdef MOZ_WIDGET_GONK
#include "cpuacct.h"

#if ANDROID_VERSION < 17 || defined(MOZ_WIDGET_ANDROID)
extern "C" NS_EXPORT int
timer_create(clockid_t, struct sigevent*, timer_t*)
{
  __android_log_print(ANDROID_LOG_ERROR, "BionicGlue", "timer_create not supported!");
  abort();
  return -1;
}
#endif

#else
#define cpuacct_add(x)
#endif

#if ANDROID_VERSION < 17 || defined(MOZ_WIDGET_ANDROID)
extern "C" NS_EXPORT int
pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
  AtForkFuncs funcs;
  funcs.prepare = prepare;
  funcs.parent = parent;
  funcs.child = child;
  if (!atfork.capacity())
    atfork.reserve(1);
  atfork.push_back(funcs);
  return 0;
}

extern "C" NS_EXPORT pid_t __fork(void);

extern "C" NS_EXPORT pid_t
fork(void)
{
  pid_t pid;
  for (auto it = atfork.rbegin();
       it < atfork.rend(); ++it)
    if (it->prepare)
      it->prepare();

  switch ((pid = syscall(__NR_clone, SIGCHLD, NULL, NULL, NULL, NULL))) {
  case 0:
    cpuacct_add(getuid());
    for (auto it = atfork.begin();
         it < atfork.end(); ++it)
      if (it->child)
        it->child();
    break;
  default:
    for (auto it = atfork.begin();
         it < atfork.end(); ++it)
      if (it->parent)
        it->parent();
  }
  return pid;
}
#endif

extern "C" NS_EXPORT int
raise(int sig)
{
  
  
  
  
  
  
  

  extern pid_t gettid(void);
  return syscall(__NR_tgkill, getpid(), gettid(), sig);
}










static pthread_mutex_t  _pr_envLock = PTHREAD_MUTEX_INITIALIZER;

extern "C" NS_EXPORT char*
__wrap_PR_GetEnv(const char *var)
{
    char *ev;

    pthread_mutex_lock(&_pr_envLock);
    ev = getenv(var);
    pthread_mutex_unlock(&_pr_envLock);
    return ev;
}

extern "C" NS_EXPORT int
__wrap_PR_SetEnv(const char *string)
{
    int result;

    if ( !strchr(string, '=')) return(-1);

    pthread_mutex_lock(&_pr_envLock);
    result = putenv(const_cast<char*>(string));
    pthread_mutex_unlock(&_pr_envLock);
    return (result)? -1 : 0;
}

extern "C" NS_EXPORT pthread_mutex_t *
PR_GetEnvLock(void)
{
  return &_pr_envLock;
}


#ifndef MOZ_WIDGET_GONK
namespace android {
  namespace VectorImpl {
    NS_EXPORT void reservedVectorImpl1(void) { }
    NS_EXPORT void reservedVectorImpl2(void) { }
    NS_EXPORT void reservedVectorImpl3(void) { }
    NS_EXPORT void reservedVectorImpl4(void) { }
    NS_EXPORT void reservedVectorImpl5(void) { }
    NS_EXPORT void reservedVectorImpl6(void) { }
    NS_EXPORT void reservedVectorImpl7(void) { }
    NS_EXPORT void reservedVectorImpl8(void) { }
  }
}
#endif

