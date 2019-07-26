




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
#define WRAP(x) x

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
#define WRAP(x) __wrap_##x
#endif

#if ANDROID_VERSION < 17 || defined(MOZ_WIDGET_ANDROID)
extern "C" NS_EXPORT int
WRAP(pthread_atfork)(void (*prepare)(void), void (*parent)(void), void (*child)(void))
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

extern "C" pid_t __fork(void);

extern "C" NS_EXPORT pid_t
WRAP(fork)(void)
{
  pid_t pid;
  for (auto it = atfork.rbegin();
       it < atfork.rend(); ++it)
    if (it->prepare)
      it->prepare();

  switch ((pid = __fork())) {
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
WRAP(raise)(int sig)
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
    result = putenv(string);
    pthread_mutex_unlock(&_pr_envLock);
    return (result)? -1 : 0;
}

extern "C" NS_EXPORT pthread_mutex_t *
PR_GetEnvLock(void)
{
  return &_pr_envLock;
}







#ifndef MOZ_WIDGET_GONK
#include <string.h>
extern "C" NS_EXPORT void* __real_memccpy(void * a0, const void * a1, int a2, size_t a3);
extern "C" NS_EXPORT void* __real_memchr(const void * a0, int a1, size_t a2);
extern "C" NS_EXPORT void* __real_memrchr(const void * a0, int a1, size_t a2);
extern "C" NS_EXPORT int __real_memcmp(const void * a0, const void * a1, size_t a2);
extern "C" NS_EXPORT void* __real_memcpy(void * a0, const void * a1, size_t a2);
extern "C" NS_EXPORT void* __real_memmove(void * a0, const void * a1, size_t a2);
extern "C" NS_EXPORT void* __real_memset(void * a0, int a1, size_t a2);
extern "C" NS_EXPORT void* __real_memmem(const void * a0, size_t a1, const void * a2, size_t a3);
extern "C" NS_EXPORT void __real_memswap(void * a0, void * a1, size_t a2);
extern "C" NS_EXPORT char* __real_index(const char * a0, int a1);
extern "C" NS_EXPORT char* __real_strchr(const char * a0, int a1);
extern "C" NS_EXPORT char* __real_strrchr(const char * a0, int a1);
extern "C" NS_EXPORT size_t __real_strlen(const char * a0);
extern "C" NS_EXPORT int __real_strcmp(const char * a0, const char * a1);
extern "C" NS_EXPORT char* __real_strcpy(char * a0, const char * a1);
extern "C" NS_EXPORT char* __real_strcat(char * a0, const char * a1);
extern "C" NS_EXPORT int __real_strcasecmp(const char * a0, const char * a1);
extern "C" NS_EXPORT int __real_strncasecmp(const char * a0, const char * a1, size_t a2);
extern "C" NS_EXPORT char* __real_strstr(const char * a0, const char * a1);
extern "C" NS_EXPORT char* __real_strcasestr(const char * a0, const char * a1);
extern "C" NS_EXPORT char* __real_strtok(char * a0, const char * a1);
extern "C" NS_EXPORT char* __real_strtok_r(char * a0, const char * a1, char** a2);
extern "C" NS_EXPORT char* __real_strerror(int a0);
extern "C" NS_EXPORT int __real_strerror_r(int a0, char * a1, size_t a2);
extern "C" NS_EXPORT size_t __real_strnlen(const char * a0, size_t a1);
extern "C" NS_EXPORT char* __real_strncat(char * a0, const char * a1, size_t a2);
extern "C" NS_EXPORT int __real_strncmp(const char * a0, const char * a1, size_t a2);
extern "C" NS_EXPORT char* __real_strncpy(char * a0, const char * a1, size_t a2);
extern "C" NS_EXPORT size_t __real_strlcat(char * a0, const char * a1, size_t a2);
extern "C" NS_EXPORT size_t __real_strlcpy(char * a0, const char * a1, size_t a2);
extern "C" NS_EXPORT size_t __real_strcspn(const char * a0, const char * a1);
extern "C" NS_EXPORT char* __real_strpbrk(const char * a0, const char * a1);
extern "C" NS_EXPORT char* __real_strsep(char ** a0, const char * a1);
extern "C" NS_EXPORT size_t __real_strspn(const char * a0, const char * a1);
extern "C" NS_EXPORT int __real_strcoll(const char * a0, const char * a1);
extern "C" NS_EXPORT size_t __real_strxfrm(char * a0, const char * a1, size_t a2);

extern "C" NS_EXPORT void* __wrap_memccpy(void * a0, const void * a1, int a2, size_t a3) { return __real_memccpy(a0, a1, a2, a3); }
extern "C" NS_EXPORT void* __wrap_memchr(const void * a0, int a1, size_t a2) { return __real_memchr(a0, a1, a2); }
extern "C" NS_EXPORT void* __wrap_memrchr(const void * a0, int a1, size_t a2) { return __real_memrchr(a0, a1, a2); }
extern "C" NS_EXPORT int __wrap_memcmp(const void * a0, const void * a1, size_t a2) { return __real_memcmp(a0, a1, a2); }
extern "C" NS_EXPORT void* __wrap_memcpy(void * a0, const void * a1, size_t a2) { return __real_memcpy(a0, a1, a2); }
extern "C" NS_EXPORT void* __wrap_memmove(void * a0, const void * a1, size_t a2) { return __real_memmove(a0, a1, a2); }
extern "C" NS_EXPORT void* __wrap_memset(void * a0, int a1, size_t a2) { return __real_memset(a0, a1, a2); }
extern "C" NS_EXPORT void* __wrap_memmem(const void * a0, size_t a1, const void * a2, size_t a3) { return __real_memmem(a0, a1, a2, a3); }
extern "C" NS_EXPORT void __wrap_memswap(void * a0, void * a1, size_t a2) { __real_memswap(a0, a1, a2); }
extern "C" NS_EXPORT char* __wrap_index(const char * a0, int a1) { return __real_index(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strchr(const char * a0, int a1) { return __real_strchr(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strrchr(const char * a0, int a1) { return __real_strrchr(a0, a1); }
extern "C" NS_EXPORT size_t __wrap_strlen(const char * a0) { return __real_strlen(a0); }
extern "C" NS_EXPORT int __wrap_strcmp(const char * a0, const char * a1) { return __real_strcmp(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strcpy(char * a0, const char * a1) { return __real_strcpy(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strcat(char * a0, const char * a1) { return __real_strcat(a0, a1); }
extern "C" NS_EXPORT int __wrap_strcasecmp(const char * a0, const char * a1) { return __real_strcasecmp(a0, a1); }
extern "C" NS_EXPORT int __wrap_strncasecmp(const char * a0, const char * a1, size_t a2) { return __real_strncasecmp(a0, a1, a2); }
extern "C" NS_EXPORT char* __wrap_strstr(const char * a0, const char * a1) { return __real_strstr(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strcasestr(const char * a0, const char * a1) { return __real_strcasestr(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strtok(char * a0, const char * a1) { return __real_strtok(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strtok_r(char * a0, const char * a1, char** a2) { return __real_strtok_r(a0, a1, a2); }
extern "C" NS_EXPORT char* __wrap_strerror(int a0) { return __real_strerror(a0); }
extern "C" NS_EXPORT int __wrap_strerror_r(int a0, char * a1, size_t a2) { return __real_strerror_r(a0, a1, a2); }
extern "C" NS_EXPORT size_t __wrap_strnlen(const char * a0, size_t a1) { return __real_strnlen(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strncat(char * a0, const char * a1, size_t a2) { return __real_strncat(a0, a1, a2); }
extern "C" NS_EXPORT int __wrap_strncmp(const char * a0, const char * a1, size_t a2) { return __real_strncmp(a0, a1, a2); }
extern "C" NS_EXPORT char* __wrap_strncpy(char * a0, const char * a1, size_t a2) { return __real_strncpy(a0, a1, a2); }
extern "C" NS_EXPORT size_t __wrap_strlcat(char * a0, const char * a1, size_t a2) { return __real_strlcat(a0, a1, a2); }
extern "C" NS_EXPORT size_t __wrap_strlcpy(char * a0, const char * a1, size_t a2) { return __real_strlcpy(a0, a1, a2); }
extern "C" NS_EXPORT size_t __wrap_strcspn(const char * a0, const char * a1) { return __real_strcspn(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strpbrk(const char * a0, const char * a1) { return __real_strpbrk(a0, a1); }
extern "C" NS_EXPORT char* __wrap_strsep(char ** a0, const char * a1) { return __real_strsep(a0, a1); }
extern "C" NS_EXPORT size_t __wrap_strspn(const char * a0, const char * a1) { return __real_strspn(a0, a1); }
extern "C" NS_EXPORT int __wrap_strcoll(const char * a0, const char * a1) { return __real_strcoll(a0, a1); }
extern "C" NS_EXPORT size_t __wrap_strxfrm(char * a0, const char * a1, size_t a2) { return __real_strxfrm(a0, a1, a2); }
#endif
