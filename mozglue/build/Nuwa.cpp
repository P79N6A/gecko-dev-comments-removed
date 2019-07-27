




#include <map>
#include <memory>

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <poll.h>
#include <pthread.h>
#include <alloca.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <vector>

#include "mozilla/LinkedList.h"
#include "mozilla/TaggedAnonymousMemory.h"
#include "Nuwa.h"

using namespace mozilla;

extern "C" MFBT_API int tgkill(pid_t tgid, pid_t tid, int signalno) {
  return syscall(__NR_tgkill, tgid, tid, signalno);
}









extern "C" {
#pragma GCC visibility push(default)
int __real_pthread_create(pthread_t *thread,
                          const pthread_attr_t *attr,
                          void *(*start_routine) (void *),
                          void *arg);
int __real_pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
int __real_pthread_key_delete(pthread_key_t key);
pthread_t __real_pthread_self();
int __real_pthread_join(pthread_t thread, void **retval);
int __real_epoll_wait(int epfd,
                      struct epoll_event *events,
                      int maxevents,
                      int timeout);
int __real_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx);
int __real_pthread_cond_timedwait(pthread_cond_t *cond,
                                  pthread_mutex_t *mtx,
                                  const struct timespec *abstime);
int __real___pthread_cond_timedwait(pthread_cond_t *cond,
                                    pthread_mutex_t *mtx,
                                    const struct timespec *abstime,
                                    clockid_t clock);
int __real_pthread_mutex_lock(pthread_mutex_t *mtx);
int __real_poll(struct pollfd *fds, nfds_t nfds, int timeout);
int __real_epoll_create(int size);
int __real_socketpair(int domain, int type, int protocol, int sv[2]);
int __real_pipe2(int __pipedes[2], int flags);
int __real_pipe(int __pipedes[2]);
int __real_epoll_ctl(int aEpollFd, int aOp, int aFd, struct epoll_event *aEvent);
int __real_close(int aFd);
#pragma GCC visibility pop
}

#define REAL(s) __real_##s






static bool sIsNuwaProcess = false; 
static bool sIsFreezing = false; 
static bool sNuwaReady = false;  
static bool sNuwaPendingSpawn = false; 
static bool sNuwaForking = false;


static NuwaProtoFdInfo sProtoFdInfos[NUWA_TOPLEVEL_MAX];
static int sProtoFdInfosSize = 0;

typedef std::vector<std::pair<pthread_key_t, void *> >
TLSInfoList;




static size_t getPageSize(void) {
#ifdef HAVE_GETPAGESIZE
  return getpagesize();
#elif defined(_SC_PAGESIZE)
  return sysconf(_SC_PAGESIZE);
#elif defined(PAGE_SIZE)
  return PAGE_SIZE;
#else
  #warning "Hard-coding page size to 4096 bytes"
  return 4096
#endif
}






#ifndef NUWA_STACK_SIZE
#define NUWA_STACK_SIZE (1024 * 128)
#endif

#define NATIVE_THREAD_NAME_LENGTH 16

struct thread_info : public mozilla::LinkedListElement<thread_info> {
  pthread_t origThreadID;
  pthread_t recreatedThreadID;
  pthread_attr_t threadAttr;
  jmp_buf jmpEnv;
  jmp_buf retEnv;

  int flags;

  void *(*startupFunc)(void *arg);
  void *startupArg;

  
  
  void (*recrFunc)(void *arg);
  void *recrArg;

  TLSInfoList tlsInfo;

  











  pthread_mutex_t *condMutex;
  bool condMutexNeedsBalancing;

  void *stk;

  pid_t origNativeThreadID;
  pid_t recreatedNativeThreadID;
  char nativeThreadName[NATIVE_THREAD_NAME_LENGTH];
};

typedef struct thread_info thread_info_t;

static thread_info_t *sCurrentRecreatingThread = nullptr;





static void
RunCustomRecreation() {
  thread_info_t *tinfo = sCurrentRecreatingThread;
  if (tinfo->recrFunc != nullptr) {
    tinfo->recrFunc(tinfo->recrArg);
  }
}












#define TINFO_FLAG_NUWA_SUPPORT 0x1
#define TINFO_FLAG_NUWA_SKIP 0x2
#define TINFO_FLAG_NUWA_EXPLICIT_CHECKPOINT 0x4

typedef struct nuwa_construct {
  void (*construct)(void *);
  void *arg;
} nuwa_construct_t;

static std::vector<nuwa_construct_t> sConstructors;
static std::vector<nuwa_construct_t> sFinalConstructors;

typedef std::map<pthread_key_t, void (*)(void *)> TLSKeySet;
static TLSKeySet sTLSKeys;







static pthread_mutex_t sThreadFreezeLock = PTHREAD_MUTEX_INITIALIZER;

static thread_info_t sMainThread;
static int sThreadCount = 0;
static int sThreadFreezeCount = 0;









struct AllThreadsListType : public LinkedList<thread_info_t>
{
  ~AllThreadsListType()
  {
    if (!isEmpty()) {
      __android_log_print(ANDROID_LOG_WARN, "Nuwa",
                          "Threads remaining at exit:\n");
      int n = 0;
      for (const thread_info_t* t = getFirst(); t; t = t->getNext()) {
        __android_log_print(ANDROID_LOG_WARN, "Nuwa",
                            "  %.*s (origNativeThreadID=%d recreatedNativeThreadID=%d)\n",
                            NATIVE_THREAD_NAME_LENGTH,
                            t->nativeThreadName,
                            t->origNativeThreadID,
                            t->recreatedNativeThreadID);
        n++;
      }
      __android_log_print(ANDROID_LOG_WARN, "Nuwa",
                          "total: %d outstanding threads. "
                          "Please fix them so they're destroyed before this point!\n", n);
      __android_log_print(ANDROID_LOG_WARN, "Nuwa",
                          "note: sThreadCount=%d, sThreadFreezeCount=%d\n",
                          sThreadCount,
                          sThreadFreezeCount);
    }
    clear();
  }
};
static AllThreadsListType sAllThreads;





static pthread_mutex_t sThreadCountLock = PTHREAD_MUTEX_INITIALIZER;




static pthread_cond_t sThreadChangeCond = PTHREAD_COND_INITIALIZER;





static pthread_mutex_t sForkLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sForkWaitCond = PTHREAD_COND_INITIALIZER;






static bool sForkWaitCondChanged = false;





static pthread_mutex_t sTLSKeyLock = PTHREAD_MUTEX_INITIALIZER;
static int sThreadSkipCount = 0;

static thread_info_t *
GetThreadInfoInner(pthread_t threadID) {
  for (thread_info_t *tinfo = sAllThreads.getFirst();
       tinfo;
       tinfo = tinfo->getNext()) {
    if (pthread_equal(tinfo->origThreadID, threadID)) {
      return tinfo;
    }
  }

  return nullptr;
}






static thread_info_t *
GetThreadInfo(pthread_t threadID) {
  if (sIsNuwaProcess) {
    REAL(pthread_mutex_lock)(&sThreadCountLock);
  }
  thread_info_t *tinfo = GetThreadInfoInner(threadID);
  if (sIsNuwaProcess) {
    pthread_mutex_unlock(&sThreadCountLock);
  }
  return tinfo;
}






static thread_info_t*
GetThreadInfo(pid_t threadID) {
  if (sIsNuwaProcess) {
    REAL(pthread_mutex_lock)(&sThreadCountLock);
  }
  thread_info_t *thrinfo = nullptr;
  for (thread_info_t *tinfo = sAllThreads.getFirst();
       tinfo;
       tinfo = tinfo->getNext()) {
    if (tinfo->origNativeThreadID == threadID) {
      thrinfo = tinfo;
      break;
    }
  }
  if (sIsNuwaProcess) {
    pthread_mutex_unlock(&sThreadCountLock);
  }

  return thrinfo;
}

#if !defined(HAVE_THREAD_TLS_KEYWORD)





static thread_info_t *
GetCurThreadInfo() {
  pthread_t threadID = REAL(pthread_self)();
  pthread_t thread_info_t::*threadIDptr =
      (sIsNuwaProcess ?
         &thread_info_t::origThreadID :
         &thread_info_t::recreatedThreadID);

  REAL(pthread_mutex_lock)(&sThreadCountLock);
  thread_info_t *tinfo;
  for (tinfo = sAllThreads.getFirst();
       tinfo;
       tinfo = tinfo->getNext()) {
    if (pthread_equal(tinfo->*threadIDptr, threadID)) {
      break;
    }
  }
  pthread_mutex_unlock(&sThreadCountLock);
  return tinfo;
}
#define CUR_THREAD_INFO GetCurThreadInfo()
#define SET_THREAD_INFO(x)
#else


static __thread thread_info_t *sCurThreadInfo = nullptr;
#define CUR_THREAD_INFO sCurThreadInfo
#define SET_THREAD_INFO(x) do { sCurThreadInfo = (x); } while(0)
#endif  




class EpollManager {
public:
  class EpollInfo {
  public:
    typedef struct epoll_event Events;
    typedef std::map<int, Events> EpollEventsMap;
    typedef EpollEventsMap::iterator iterator;
    typedef EpollEventsMap::const_iterator const_iterator;

    EpollInfo(): mBackSize(0) {}
    EpollInfo(int aBackSize): mBackSize(aBackSize) {}
    EpollInfo(const EpollInfo &aOther): mEvents(aOther.mEvents)
                                      , mBackSize(aOther.mBackSize) {
    }
    ~EpollInfo() {
      mEvents.clear();
    }

    void AddEvents(int aFd, Events &aEvents) {
      std::pair<iterator, bool> pair =
        mEvents.insert(std::make_pair(aFd, aEvents));
      if (!pair.second) {
        abort();
      }
    }

    void RemoveEvents(int aFd) {
      if (!mEvents.erase(aFd)) {
        abort();
      }
    }

    void ModifyEvents(int aFd, Events &aEvents) {
      iterator it = mEvents.find(aFd);
      if (it == mEvents.end()) {
        abort();
      }
      it->second = aEvents;
    }

    const Events &FindEvents(int aFd) const {
      const_iterator it = mEvents.find(aFd);
      if (it == mEvents.end()) {
        abort();
      }
      return it->second;
    }

    int Size() const { return mEvents.size(); }

    
    const_iterator begin() const { return mEvents.begin(); }
    const_iterator end() const { return mEvents.end(); }

    int BackSize() const { return mBackSize; }

  private:
    EpollEventsMap mEvents;
    int mBackSize;

    friend class EpollManager;
  };

  typedef std::map<int, EpollInfo> EpollInfoMap;
  typedef EpollInfoMap::iterator iterator;
  typedef EpollInfoMap::const_iterator const_iterator;

public:
  void AddEpollInfo(int aEpollFd, int aBackSize) {
    EpollInfo *oldinfo = FindEpollInfo(aEpollFd);
    if (oldinfo != nullptr) {
      abort();
    }
    mEpollFdsInfo[aEpollFd] = EpollInfo(aBackSize);
  }

  EpollInfo *FindEpollInfo(int aEpollFd) {
    iterator it = mEpollFdsInfo.find(aEpollFd);
    if (it == mEpollFdsInfo.end()) {
      return nullptr;
    }
    return &it->second;
  }

  void RemoveEpollInfo(int aEpollFd) {
    if (!mEpollFdsInfo.erase(aEpollFd)) {
      abort();
    }
  }

  int Size() const { return mEpollFdsInfo.size(); }

  
  const_iterator begin() const { return mEpollFdsInfo.begin(); }
  const_iterator end() const { return mEpollFdsInfo.end(); }

  static EpollManager *Singleton() {
    if (!sInstance) {
      sInstance = new EpollManager();
    }
    return sInstance;
  }

  static void Shutdown() {
    if (!sInstance) {
      abort();
    }

    delete sInstance;
    sInstance = nullptr;
  }

private:
  static EpollManager *sInstance;
  ~EpollManager() {
    mEpollFdsInfo.clear();
  }

  EpollInfoMap mEpollFdsInfo;

  EpollManager() {}
};

EpollManager* EpollManager::sInstance;

static thread_info_t *
thread_info_new(void) {
  
  thread_info_t *tinfo = new thread_info_t();
  tinfo->flags = 0;
  tinfo->recrFunc = nullptr;
  tinfo->recrArg = nullptr;
  tinfo->recreatedThreadID = 0;
  tinfo->recreatedNativeThreadID = 0;
  tinfo->condMutex = nullptr;
  tinfo->condMutexNeedsBalancing = false;
  tinfo->stk = MozTaggedAnonymousMmap(nullptr,
                                      NUWA_STACK_SIZE + getPageSize(),
                                      PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS,
                                       -1,
                                       0,
                                      "nuwa-thread-stack");

  
  
  
  mprotect(tinfo->stk, getPageSize(), PROT_NONE);

  pthread_attr_init(&tinfo->threadAttr);

  REAL(pthread_mutex_lock)(&sThreadCountLock);
  
  sAllThreads.insertBack(tinfo);

  sThreadCount++;
  pthread_cond_signal(&sThreadChangeCond);
  pthread_mutex_unlock(&sThreadCountLock);

  return tinfo;
}

static void
thread_info_cleanup(void *arg) {
  if (sNuwaForking) {
    
    abort();
  }

  thread_info_t *tinfo = (thread_info_t *)arg;
  pthread_attr_destroy(&tinfo->threadAttr);

  munmap(tinfo->stk, NUWA_STACK_SIZE + getPageSize());

  REAL(pthread_mutex_lock)(&sThreadCountLock);
  
  tinfo->remove();
  pthread_mutex_unlock(&sThreadCountLock);

  
  
  
  
  delete tinfo;

  REAL(pthread_mutex_lock)(&sThreadCountLock);
  sThreadCount--;
  pthread_cond_signal(&sThreadChangeCond);
  pthread_mutex_unlock(&sThreadCountLock);
}

static void*
cleaner_thread(void *arg) {
  thread_info_t *tinfo = (thread_info_t *)arg;
  pthread_t *thread = sIsNuwaProcess ? &tinfo->origThreadID
                                     : &tinfo->recreatedThreadID;
  
  while (!pthread_kill(*thread, 0)) {
    sched_yield();
  }
  thread_info_cleanup(tinfo);
  return nullptr;
}

static void
thread_cleanup(void *arg) {
  pthread_t thread;
  REAL(pthread_create)(&thread, nullptr, &cleaner_thread, arg);
  pthread_detach(thread);
}

static void *
_thread_create_startup(void *arg) {
  thread_info_t *tinfo = (thread_info_t *)arg;
  void *r;

  
  
  pthread_getattr_np(REAL(pthread_self)(), &tinfo->threadAttr);

  SET_THREAD_INFO(tinfo);
  tinfo->origThreadID = REAL(pthread_self)();
  tinfo->origNativeThreadID = gettid();

  pthread_cleanup_push(thread_cleanup, tinfo);

  r = tinfo->startupFunc(tinfo->startupArg);

  if (!sIsNuwaProcess) {
    return r;
  }

  pthread_cleanup_pop(1);

  return r;
}


#define STACK_RESERVED_SZ 64
#define STACK_SENTINEL(v) ((v)[0])
#define STACK_SENTINEL_VALUE(v) ((uint32_t)(v) ^ 0xdeadbeef)

static void *
thread_create_startup(void *arg) {
  






  void *r;
  volatile uint32_t reserved[STACK_RESERVED_SZ];

  
  STACK_SENTINEL(reserved) = STACK_SENTINEL_VALUE(reserved);

  r = _thread_create_startup(arg);

  
  if (STACK_SENTINEL(reserved) != STACK_SENTINEL_VALUE(reserved)) {
    abort();                    
  }

  thread_info_t *tinfo = CUR_THREAD_INFO;
  if (!sIsNuwaProcess) {
    longjmp(tinfo->retEnv, 1);

    
    abort();
  }

  return r;
}

extern "C" MFBT_API int
__wrap_pthread_create(pthread_t *thread,
                      const pthread_attr_t *attr,
                      void *(*start_routine) (void *),
                      void *arg) {
  if (!sIsNuwaProcess) {
    return REAL(pthread_create)(thread, attr, start_routine, arg);
  }

  thread_info_t *tinfo = thread_info_new();
  tinfo->startupFunc = start_routine;
  tinfo->startupArg = arg;
  pthread_attr_setstack(&tinfo->threadAttr,
                        (char*)tinfo->stk + getPageSize(),
                        NUWA_STACK_SIZE);

  int rv = REAL(pthread_create)(thread,
                                &tinfo->threadAttr,
                                thread_create_startup,
                                tinfo);
  if (rv) {
    thread_info_cleanup(tinfo);
  } else {
    tinfo->origThreadID = *thread;
  }

  return rv;
}







static void
SaveTLSInfo(thread_info_t *tinfo) {
  REAL(pthread_mutex_lock)(&sTLSKeyLock);
  tinfo->tlsInfo.clear();
  for (TLSKeySet::const_iterator it = sTLSKeys.begin();
       it != sTLSKeys.end();
       it++) {
    void *value = pthread_getspecific(it->first);
    if (value == nullptr) {
      continue;
    }

    pthread_key_t key = it->first;
    tinfo->tlsInfo.push_back(TLSInfoList::value_type(key, value));
  }
  pthread_mutex_unlock(&sTLSKeyLock);
}




static void
RestoreTLSInfo(thread_info_t *tinfo) {
  for (TLSInfoList::const_iterator it = tinfo->tlsInfo.begin();
       it != tinfo->tlsInfo.end();
       it++) {
    pthread_key_t key = it->first;
    const void *value = it->second;
    if (pthread_setspecific(key, value)) {
      abort();
    }
  }

  SET_THREAD_INFO(tinfo);
  tinfo->recreatedThreadID = REAL(pthread_self)();
  tinfo->recreatedNativeThreadID = gettid();
}

extern "C" MFBT_API int
__wrap_pthread_key_create(pthread_key_t *key, void (*destructor)(void*)) {
  int rv = REAL(pthread_key_create)(key, destructor);
  if (rv != 0) {
    return rv;
  }
  REAL(pthread_mutex_lock)(&sTLSKeyLock);
  sTLSKeys.insert(TLSKeySet::value_type(*key, destructor));
  pthread_mutex_unlock(&sTLSKeyLock);
  return 0;
}

extern "C" MFBT_API int
__wrap_pthread_key_delete(pthread_key_t key) {
  if (!sIsNuwaProcess) {
    return REAL(pthread_key_delete)(key);
  }
  int rv = REAL(pthread_key_delete)(key);
  if (rv != 0) {
    return rv;
  }
  REAL(pthread_mutex_lock)(&sTLSKeyLock);
  sTLSKeys.erase(key);
  pthread_mutex_unlock(&sTLSKeyLock);
  return 0;
}

extern "C" MFBT_API pthread_t
__wrap_pthread_self() {
  thread_info_t *tinfo = CUR_THREAD_INFO;
  if (tinfo) {
    
    
    return tinfo->origThreadID;
  }
  return REAL(pthread_self)();
}

extern "C" MFBT_API int
__wrap_pthread_join(pthread_t thread, void **retval) {
  thread_info_t *tinfo = GetThreadInfo(thread);
  if (tinfo == nullptr) {
    return REAL(pthread_join)(thread, retval);
  }
  
  return REAL(pthread_join)(tinfo->recreatedThreadID, retval);
}




















static pthread_mutex_t sRecreateWaitLock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t sRecreateGateLock = PTHREAD_MUTEX_INITIALIZER;


static pthread_mutex_t sRecreateVIPGateLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sRecreateVIPCond = PTHREAD_COND_INITIALIZER;
static int sRecreateVIPCount = 0;
static int sRecreateGatePassed = 0;

























#define RECREATE_START()                          \
  do {                                            \
    REAL(pthread_mutex_lock)(&sRecreateWaitLock); \
    REAL(pthread_mutex_lock)(&sRecreateGateLock); \
  } while(0)
#define RECREATE_BEFORE(info) do { sCurrentRecreatingThread = info; } while(0)
#define RECREATE_WAIT() REAL(pthread_mutex_lock)(&sRecreateWaitLock)
#define RECREATE_CONTINUE() do {                \
    RunCustomRecreation();                      \
    pthread_mutex_unlock(&sRecreateWaitLock);   \
  } while(0)
#define RECREATE_FINISH() pthread_mutex_unlock(&sRecreateWaitLock)
#define RECREATE_GATE()                           \
  do {                                            \
    REAL(pthread_mutex_lock)(&sRecreateGateLock); \
    sRecreateGatePassed++;                        \
    pthread_mutex_unlock(&sRecreateGateLock);     \
  } while(0)
#define RECREATE_OPEN_GATE() pthread_mutex_unlock(&sRecreateGateLock)
#define RECREATE_GATE_VIP()                       \
  do {                                            \
    REAL(pthread_mutex_lock)(&sRecreateGateLock); \
    pthread_mutex_unlock(&sRecreateGateLock);     \
  } while(0)
#define RECREATE_PASS_VIP()                          \
  do {                                               \
    REAL(pthread_mutex_lock)(&sRecreateVIPGateLock); \
    sRecreateGatePassed++;                           \
    pthread_cond_signal(&sRecreateVIPCond);          \
    pthread_mutex_unlock(&sRecreateVIPGateLock);     \
  } while(0)
#define RECREATE_WAIT_ALL_VIP()                        \
  do {                                                 \
    REAL(pthread_mutex_lock)(&sRecreateVIPGateLock);   \
    while(sRecreateGatePassed < sRecreateVIPCount) {   \
      REAL(pthread_cond_wait)(&sRecreateVIPCond,       \
                        &sRecreateVIPGateLock);        \
    }                                                  \
    pthread_mutex_unlock(&sRecreateVIPGateLock);       \
  } while(0)























#define THREAD_FREEZE_POINT1()                                 \
  bool freezeCountChg = false;                                 \
  bool recreated = false;                                      \
  volatile bool freezePoint2 = false;                          \
  thread_info_t *tinfo;                                        \
  if (sIsNuwaProcess &&                                        \
      (tinfo = CUR_THREAD_INFO) &&                             \
      (tinfo->flags & TINFO_FLAG_NUWA_SUPPORT) &&              \
      !(tinfo->flags & TINFO_FLAG_NUWA_EXPLICIT_CHECKPOINT)) { \
    if (!setjmp(tinfo->jmpEnv)) {                              \
      REAL(pthread_mutex_lock)(&sThreadCountLock);             \
      SaveTLSInfo(tinfo);                                      \
      sThreadFreezeCount++;                                    \
      freezeCountChg = true;                                   \
      pthread_cond_signal(&sThreadChangeCond);                 \
      pthread_mutex_unlock(&sThreadCountLock);                 \
                                                               \
      if (sIsFreezing) {                                       \
        REAL(pthread_mutex_lock)(&sThreadFreezeLock);          \
        /* Never return from the pthread_mutex_lock() call. */ \
        abort();                                               \
      }                                                        \
    } else {                                                   \
      RECREATE_CONTINUE();                                     \
      RECREATE_GATE();                                         \
      freezeCountChg = false;                                  \
      recreated = true;                                        \
    }                                                          \
  }

#define THREAD_FREEZE_POINT1_VIP()                             \
  bool freezeCountChg = false;                                 \
  bool recreated = false;                                      \
  volatile bool freezePoint1 = false;                          \
  volatile bool freezePoint2 = false;                          \
  thread_info_t *tinfo;                                        \
  if (sIsNuwaProcess &&                                        \
      (tinfo = CUR_THREAD_INFO) &&                             \
      (tinfo->flags & TINFO_FLAG_NUWA_SUPPORT) &&              \
      !(tinfo->flags & TINFO_FLAG_NUWA_EXPLICIT_CHECKPOINT)) { \
    if (!setjmp(tinfo->jmpEnv)) {                              \
      REAL(pthread_mutex_lock)(&sThreadCountLock);             \
      SaveTLSInfo(tinfo);                                      \
      sThreadFreezeCount++;                                    \
      sRecreateVIPCount++;                                     \
      freezeCountChg = true;                                   \
      pthread_cond_signal(&sThreadChangeCond);                 \
      pthread_mutex_unlock(&sThreadCountLock);                 \
                                                               \
      if (sIsFreezing) {                                       \
        freezePoint1 = true;                                   \
        REAL(pthread_mutex_lock)(&sThreadFreezeLock);          \
        /* Never return from the pthread_mutex_lock() call. */ \
        abort();                                               \
      }                                                        \
    } else {                                                   \
      freezeCountChg = false;                                  \
      recreated = true;                                        \
    }                                                          \
  }

#define THREAD_FREEZE_POINT2()                               \
  if (freezeCountChg) {                                      \
    REAL(pthread_mutex_lock)(&sThreadCountLock);             \
    if (sNuwaReady && sIsNuwaProcess) {                      \
      pthread_mutex_unlock(&sThreadCountLock);               \
      freezePoint2 = true;                                   \
      REAL(pthread_mutex_lock)(&sThreadFreezeLock);          \
      /* Never return from the pthread_mutex_lock() call. */ \
      abort();                                               \
    }                                                        \
    sThreadFreezeCount--;                                    \
    pthread_cond_signal(&sThreadChangeCond);                 \
    pthread_mutex_unlock(&sThreadCountLock);                 \
  }

#define THREAD_FREEZE_POINT2_VIP()                           \
  if (freezeCountChg) {                                      \
    REAL(pthread_mutex_lock)(&sThreadCountLock);             \
    if (sNuwaReady && sIsNuwaProcess) {                      \
      pthread_mutex_unlock(&sThreadCountLock);               \
      freezePoint2 = true;                                   \
      REAL(pthread_mutex_lock)(&sThreadFreezeLock);          \
      /* Never return from the pthread_mutex_lock() call. */ \
      abort();                                               \
    }                                                        \
    sThreadFreezeCount--;                                    \
    sRecreateVIPCount--;                                     \
    pthread_cond_signal(&sThreadChangeCond);                 \
    pthread_mutex_unlock(&sThreadCountLock);                 \
  }




















extern "C" MFBT_API int
__wrap_epoll_wait(int epfd,
                  struct epoll_event *events,
                  int maxevents,
                  int timeout) {
  int rv;

  THREAD_FREEZE_POINT1();
  rv = REAL(epoll_wait)(epfd, events, maxevents, timeout);
  THREAD_FREEZE_POINT2();

  return rv;
}

extern "C" MFBT_API int
__wrap_pthread_cond_wait(pthread_cond_t *cond,
                         pthread_mutex_t *mtx) {
  int rv = 0;

  THREAD_FREEZE_POINT1_VIP();
  if (freezePoint2) {
    RECREATE_CONTINUE();
    RECREATE_PASS_VIP();
    RECREATE_GATE_VIP();
    return rv;
  }
  if (recreated && mtx) {
    if (!freezePoint1) {
      tinfo->condMutex = mtx;
      
      
      
      
      
      if (!pthread_mutex_trylock(mtx)) {
        tinfo->condMutexNeedsBalancing = true;
      }
    }
    RECREATE_CONTINUE();
    RECREATE_PASS_VIP();
  }
  rv = REAL(pthread_cond_wait)(cond, mtx);
  if (recreated && mtx) {
    
    
    RECREATE_GATE_VIP();
  }
  THREAD_FREEZE_POINT2_VIP();

  return rv;
}

extern "C" MFBT_API int
__wrap_pthread_cond_timedwait(pthread_cond_t *cond,
                              pthread_mutex_t *mtx,
                              const struct timespec *abstime) {
  int rv = 0;

  THREAD_FREEZE_POINT1_VIP();
  if (freezePoint2) {
    RECREATE_CONTINUE();
    RECREATE_PASS_VIP();
    RECREATE_GATE_VIP();
    return rv;
  }
  if (recreated && mtx) {
    if (!freezePoint1) {
      tinfo->condMutex = mtx;
      if (!pthread_mutex_trylock(mtx)) {
        tinfo->condMutexNeedsBalancing = true;
      }
    }
    RECREATE_CONTINUE();
    RECREATE_PASS_VIP();
  }
  rv = REAL(pthread_cond_timedwait)(cond, mtx, abstime);
  if (recreated && mtx) {
    RECREATE_GATE_VIP();
  }
  THREAD_FREEZE_POINT2_VIP();

  return rv;
}

extern "C" int __pthread_cond_timedwait(pthread_cond_t *cond,
                                        pthread_mutex_t *mtx,
                                        const struct timespec *abstime,
                                        clockid_t clock);

extern "C" MFBT_API int
__wrap___pthread_cond_timedwait(pthread_cond_t *cond,
                                pthread_mutex_t *mtx,
                                const struct timespec *abstime,
                                clockid_t clock) {
  int rv = 0;

  THREAD_FREEZE_POINT1_VIP();
  if (freezePoint2) {
    RECREATE_CONTINUE();
    RECREATE_PASS_VIP();
    RECREATE_GATE_VIP();
    return rv;
  }
  if (recreated && mtx) {
    if (!freezePoint1) {
      tinfo->condMutex = mtx;
      if (!pthread_mutex_trylock(mtx)) {
        tinfo->condMutexNeedsBalancing = true;
      }
    }
    RECREATE_CONTINUE();
    RECREATE_PASS_VIP();
  }
  rv = REAL(__pthread_cond_timedwait)(cond, mtx, abstime, clock);
  if (recreated && mtx) {
    RECREATE_GATE_VIP();
  }
  THREAD_FREEZE_POINT2_VIP();

  return rv;
}

extern "C" MFBT_API int
__wrap_pthread_mutex_lock(pthread_mutex_t *mtx) {
  int rv = 0;

  THREAD_FREEZE_POINT1();
  if (freezePoint2) {
    return rv;
  }
  rv = REAL(pthread_mutex_lock)(mtx);
  THREAD_FREEZE_POINT2();

  return rv;
}

extern "C" MFBT_API int
__wrap_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  int rv;

  THREAD_FREEZE_POINT1();
  rv = REAL(poll)(fds, nfds, timeout);
  THREAD_FREEZE_POINT2();

  return rv;
}

extern "C" MFBT_API int
__wrap_epoll_create(int size) {
  int epollfd = REAL(epoll_create)(size);

  if (!sIsNuwaProcess) {
    return epollfd;
  }

  if (epollfd >= 0) {
    EpollManager::Singleton()->AddEpollInfo(epollfd, size);
  }

  return epollfd;
}







struct FdPairInfo {
  enum {
    kPipe,
    kSocketpair
  } call;

  int FDs[2];
  int flags;
  int domain;
  int type;
  int protocol;
};




static pthread_mutex_t  sSignalFdLock = PTHREAD_MUTEX_INITIALIZER;
static std::vector<FdPairInfo> sSignalFds;

extern "C" MFBT_API int
__wrap_socketpair(int domain, int type, int protocol, int sv[2])
{
  int rv = REAL(socketpair)(domain, type, protocol, sv);

  if (!sIsNuwaProcess || rv < 0) {
    return rv;
  }

  REAL(pthread_mutex_lock)(&sSignalFdLock);
  FdPairInfo signalFd;
  signalFd.call = FdPairInfo::kSocketpair;
  signalFd.FDs[0] = sv[0];
  signalFd.FDs[1] = sv[1];
  signalFd.domain = domain;
  signalFd.type = type;
  signalFd.protocol = protocol;

  sSignalFds.push_back(signalFd);
  pthread_mutex_unlock(&sSignalFdLock);

  return rv;
}

extern "C" MFBT_API int
__wrap_pipe2(int __pipedes[2], int flags)
{
  int rv = REAL(pipe2)(__pipedes, flags);
  if (!sIsNuwaProcess || rv < 0) {
    return rv;
  }

  REAL(pthread_mutex_lock)(&sSignalFdLock);
  FdPairInfo signalFd;
  signalFd.call = FdPairInfo::kPipe;
  signalFd.FDs[0] = __pipedes[0];
  signalFd.FDs[1] = __pipedes[1];
  signalFd.flags = flags;
  sSignalFds.push_back(signalFd);
  pthread_mutex_unlock(&sSignalFdLock);
  return rv;
}

extern "C" MFBT_API int
__wrap_pipe(int __pipedes[2])
{
  return __wrap_pipe2(__pipedes, 0);
}

static void
DupeSingleFd(int newFd, int origFd)
{
  struct stat sb;
  if (fstat(origFd, &sb)) {
    
    return;
  }
  int fd = fcntl(origFd, F_GETFD);
  int fl = fcntl(origFd, F_GETFL);
  dup2(newFd, origFd);
  fcntl(origFd, F_SETFD, fd);
  fcntl(origFd, F_SETFL, fl);
  REAL(close)(newFd);
}

extern "C" MFBT_API void
ReplaceSignalFds()
{
  for (std::vector<FdPairInfo>::iterator it = sSignalFds.begin();
       it < sSignalFds.end(); ++it) {
    int fds[2];
    int rc = 0;
    switch (it->call) {
    case FdPairInfo::kPipe:
      rc = REAL(pipe2)(fds, it->flags);
      break;
    case FdPairInfo::kSocketpair:
      rc = REAL(socketpair)(it->domain, it->type, it->protocol, fds);
      break;
    default:
      continue;
    }

    if (rc == 0) {
      DupeSingleFd(fds[0], it->FDs[0]);
      DupeSingleFd(fds[1], it->FDs[1]);
    }
  }
}

extern "C" MFBT_API int
__wrap_epoll_ctl(int aEpollFd, int aOp, int aFd, struct epoll_event *aEvent) {
  int rv = REAL(epoll_ctl)(aEpollFd, aOp, aFd, aEvent);

  if (!sIsNuwaProcess || rv == -1) {
    return rv;
  }

  EpollManager::EpollInfo *info =
    EpollManager::Singleton()->FindEpollInfo(aEpollFd);
  if (info == nullptr) {
    abort();
  }

  switch(aOp) {
  case EPOLL_CTL_ADD:
    info->AddEvents(aFd, *aEvent);
    break;

  case EPOLL_CTL_MOD:
    info->ModifyEvents(aFd, *aEvent);
    break;

  case EPOLL_CTL_DEL:
    info->RemoveEvents(aFd);
    break;

  default:
    abort();
  }

  return rv;
}


extern "C" MFBT_API int
__wrap_close(int aFd) {
  int rv = REAL(close)(aFd);
  if (!sIsNuwaProcess || rv == -1) {
    return rv;
  }

  EpollManager::EpollInfo *info =
    EpollManager::Singleton()->FindEpollInfo(aFd);
  if (info) {
    EpollManager::Singleton()->RemoveEpollInfo(aFd);
  }

  return rv;
}

extern "C" MFBT_API int
__wrap_tgkill(pid_t tgid, pid_t tid, int signalno)
{
  if (sIsNuwaProcess) {
    return tgkill(tgid, tid, signalno);
  }

  if (tid == sMainThread.origNativeThreadID) {
    return tgkill(tgid, sMainThread.recreatedNativeThreadID, signalno);
  }

  thread_info_t *tinfo = (tid == sMainThread.origNativeThreadID ?
      &sMainThread :
      GetThreadInfo(tid));
  if (!tinfo) {
    return tgkill(tgid, tid, signalno);
  }

  return tgkill(tgid, tinfo->recreatedNativeThreadID, signalno);
}

static void *
thread_recreate_startup(void *arg) {
  












  thread_info_t *tinfo = (thread_info_t *)arg;

  prctl(PR_SET_NAME, (unsigned long)&tinfo->nativeThreadName, 0, 0, 0);
  RestoreTLSInfo(tinfo);

  if (setjmp(tinfo->retEnv) != 0) {
    return nullptr;
  }

  
  longjmp(tinfo->jmpEnv, 1);

  
  abort();

  return nullptr;
}




static void
thread_recreate(thread_info_t *tinfo) {
  pthread_t thread;

  
  
  pthread_create(&thread, &tinfo->threadAttr, thread_recreate_startup, tinfo);
}




static void
RecreateThreads() {
  sIsNuwaProcess = false;
  sIsFreezing = false;

  sMainThread.recreatedThreadID = pthread_self();
  sMainThread.recreatedNativeThreadID = gettid();

  
  for (std::vector<nuwa_construct_t>::iterator ctr = sConstructors.begin();
       ctr != sConstructors.end();
       ctr++) {
    (*ctr).construct((*ctr).arg);
  }
  sConstructors.clear();

  REAL(pthread_mutex_lock)(&sThreadCountLock);
  thread_info_t *tinfo = sAllThreads.getFirst();
  pthread_mutex_unlock(&sThreadCountLock);

  RECREATE_START();
  while (tinfo != nullptr) {
    if (tinfo->flags & TINFO_FLAG_NUWA_SUPPORT) {
      RECREATE_BEFORE(tinfo);
      thread_recreate(tinfo);
      RECREATE_WAIT();
      if (tinfo->condMutex) {
        
        REAL(pthread_mutex_lock)(tinfo->condMutex);
        if (tinfo->condMutexNeedsBalancing) {
          pthread_mutex_unlock(tinfo->condMutex);
        }
      }
    } else if(!(tinfo->flags & TINFO_FLAG_NUWA_SKIP)) {
      

      
      
      
      abort();
    }

    tinfo = tinfo->getNext();
  }
  RECREATE_WAIT_ALL_VIP();
  RECREATE_OPEN_GATE();

  RECREATE_FINISH();

  
  for (std::vector<nuwa_construct_t>::iterator ctr = sFinalConstructors.begin();
       ctr != sFinalConstructors.end();
       ctr++) {
    (*ctr).construct((*ctr).arg);
  }
  sFinalConstructors.clear();
}

extern "C" {




static void
RecreateEpollFds() {
  EpollManager *man = EpollManager::Singleton();

  for (EpollManager::const_iterator info_it = man->begin();
       info_it != man->end();
       info_it++) {
    int epollfd = info_it->first;
    const EpollManager::EpollInfo *info = &info_it->second;

    int fdflags = fcntl(epollfd, F_GETFD);
    if (fdflags == -1) {
      abort();
    }
    int fl = fcntl(epollfd, F_GETFL);
    if (fl == -1) {
      abort();
    }

    int newepollfd = REAL(epoll_create)(info->BackSize());
    if (newepollfd == -1) {
      abort();
    }
    int rv = REAL(close)(epollfd);
    if (rv == -1) {
      abort();
    }
    rv = dup2(newepollfd, epollfd);
    if (rv == -1) {
      abort();
    }
    rv = REAL(close)(newepollfd);
    if (rv == -1) {
      abort();
    }

    rv = fcntl(epollfd, F_SETFD, fdflags);
    if (rv == -1) {
      abort();
    }
    rv = fcntl(epollfd, F_SETFL, fl);
    if (rv == -1) {
      abort();
    }

    for (EpollManager::EpollInfo::const_iterator events_it = info->begin();
         events_it != info->end();
         events_it++) {
      int fd = events_it->first;
      epoll_event events;
      events = events_it->second;
      rv = REAL(epoll_ctl)(epollfd, EPOLL_CTL_ADD, fd, &events);
      if (rv == -1) {
        abort();
      }
    }
  }

  
  EpollManager::Shutdown();
}






static void
ReplaceIPC(NuwaProtoFdInfo *aInfoList, int aInfoSize) {
  int i;
  int rv;

  for (i = 0; i < aInfoSize; i++) {
    int fd = fcntl(aInfoList[i].originFd, F_GETFD);
    if (fd == -1) {
      abort();
    }

    int fl = fcntl(aInfoList[i].originFd, F_GETFL);
    if (fl == -1) {
      abort();
    }

    rv = dup2(aInfoList[i].newFds[NUWA_NEWFD_CHILD], aInfoList[i].originFd);
    if (rv == -1) {
      abort();
    }

    rv = fcntl(aInfoList[i].originFd, F_SETFD, fd);
    if (rv == -1) {
      abort();
    }

    rv = fcntl(aInfoList[i].originFd, F_SETFL, fl);
    if (rv == -1) {
      abort();
    }
  }
}




static void
AddNewProcess(pid_t pid, NuwaProtoFdInfo *aInfoList, int aInfoSize) {
  static bool (*AddNewIPCProcess)(pid_t, NuwaProtoFdInfo *, int) = nullptr;

  if (AddNewIPCProcess == nullptr) {
    AddNewIPCProcess = (bool (*)(pid_t, NuwaProtoFdInfo *, int))
      dlsym(RTLD_DEFAULT, "AddNewIPCProcess");
  }
  AddNewIPCProcess(pid, aInfoList, aInfoSize);
}

static void
PrepareProtoSockets(NuwaProtoFdInfo *aInfoList, int aInfoSize) {
  int i;
  int rv;

  for (i = 0; i < aInfoSize; i++) {
    rv = REAL(socketpair)(PF_UNIX, SOCK_STREAM, 0, aInfoList[i].newFds);
    if (rv == -1) {
      abort();
    }
  }
}

static void
CloseAllProtoSockets(NuwaProtoFdInfo *aInfoList, int aInfoSize) {
  int i;

  for (i = 0; i < aInfoSize; i++) {
    REAL(close)(aInfoList[i].newFds[0]);
    REAL(close)(aInfoList[i].newFds[1]);
  }
}

static void
AfterForkHook()
{
  void (*AfterNuwaFork)();

  
  AfterNuwaFork = (void (*)())
    dlsym(RTLD_DEFAULT, "AfterNuwaFork");
  AfterNuwaFork();
}






static int
ForkIPCProcess() {
  int pid;

  REAL(pthread_mutex_lock)(&sForkLock);

  PrepareProtoSockets(sProtoFdInfos, sProtoFdInfosSize);

  sNuwaForking = true;
  pid = fork();
  sNuwaForking = false;
  if (pid == -1) {
    abort();
  }

  if (pid > 0) {
    
    AddNewProcess(pid, sProtoFdInfos, sProtoFdInfosSize);
    CloseAllProtoSockets(sProtoFdInfos, sProtoFdInfosSize);
  } else {
    
    if (getenv("MOZ_DEBUG_CHILD_PROCESS")) {
      printf("\n\nNUWA CHILDCHILDCHILDCHILD\n  debug me @ %d\n\n", getpid());
      sleep(30);
    }
    AfterForkHook();
    ReplaceSignalFds();
    ReplaceIPC(sProtoFdInfos, sProtoFdInfosSize);
    RecreateEpollFds();
    RecreateThreads();
    CloseAllProtoSockets(sProtoFdInfos, sProtoFdInfosSize);
  }

  sForkWaitCondChanged = true;
  pthread_cond_signal(&sForkWaitCond);
  pthread_mutex_unlock(&sForkLock);

  return pid;
}




MFBT_API void
NuwaSpawnPrepare() {
  REAL(pthread_mutex_lock)(&sForkLock);

  sForkWaitCondChanged = false; 
}




MFBT_API void
NuwaSpawnWait() {
  while (!sForkWaitCondChanged) {
    REAL(pthread_cond_wait)(&sForkWaitCond, &sForkLock);
  }
  pthread_mutex_unlock(&sForkLock);
}







MFBT_API pid_t
NuwaSpawn() {
  if (gettid() != getpid()) {
    
    abort();
  }

  pid_t pid = 0;

  if (sNuwaReady) {
    pid = ForkIPCProcess();
  } else {
    sNuwaPendingSpawn = true;
  }

  return pid;
}




MFBT_API void
PrepareNuwaProcess() {
  sIsNuwaProcess = true;
  
  
  signal(SIGCHLD, SIG_IGN);

  
  REAL(pthread_mutex_lock)(&sThreadFreezeLock);

  
  sMainThread.origThreadID = pthread_self();
  sMainThread.origNativeThreadID = gettid();
}


MFBT_API void
MakeNuwaProcess() {
  void (*GetProtoFdInfos)(NuwaProtoFdInfo *, int, int *) = nullptr;
  void (*OnNuwaProcessReady)() = nullptr;
  sIsFreezing = true;

  REAL(pthread_mutex_lock)(&sThreadCountLock);

  
  while ((sThreadFreezeCount + sThreadSkipCount) != sThreadCount) {
    REAL(pthread_cond_wait)(&sThreadChangeCond, &sThreadCountLock);
  }

  GetProtoFdInfos = (void (*)(NuwaProtoFdInfo *, int, int *))
    dlsym(RTLD_DEFAULT, "GetProtoFdInfos");
  GetProtoFdInfos(sProtoFdInfos, NUWA_TOPLEVEL_MAX, &sProtoFdInfosSize);

  sNuwaReady = true;

  pthread_mutex_unlock(&sThreadCountLock);

  OnNuwaProcessReady = (void (*)())dlsym(RTLD_DEFAULT, "OnNuwaProcessReady");
  OnNuwaProcessReady();

  if (sNuwaPendingSpawn) {
    sNuwaPendingSpawn = false;
    NuwaSpawn();
  }
}





MFBT_API void
NuwaMarkCurrentThread(void (*recreate)(void *), void *arg) {
  if (!sIsNuwaProcess) {
    return;
  }

  thread_info_t *tinfo = CUR_THREAD_INFO;
  if (tinfo == nullptr) {
    abort();
  }

  tinfo->flags |= TINFO_FLAG_NUWA_SUPPORT;
  tinfo->recrFunc = recreate;
  tinfo->recrArg = arg;

  
  
  prctl(PR_GET_NAME, (unsigned long)&tinfo->nativeThreadName, 0, 0, 0);
}





MFBT_API void
NuwaSkipCurrentThread() {
  if (!sIsNuwaProcess) return;

  thread_info_t *tinfo = CUR_THREAD_INFO;
  if (tinfo == nullptr) {
    abort();
  }

  if (!(tinfo->flags & TINFO_FLAG_NUWA_SKIP)) {
    sThreadSkipCount++;
  }
  tinfo->flags |= TINFO_FLAG_NUWA_SKIP;
}







MFBT_API void
NuwaFreezeCurrentThread() {
  thread_info_t *tinfo = CUR_THREAD_INFO;
  if (sIsNuwaProcess &&
      (tinfo = CUR_THREAD_INFO) &&
      (tinfo->flags & TINFO_FLAG_NUWA_SUPPORT)) {
    if (!setjmp(tinfo->jmpEnv)) {
      REAL(pthread_mutex_lock)(&sThreadCountLock);
      SaveTLSInfo(tinfo);
      sThreadFreezeCount++;
      pthread_cond_signal(&sThreadChangeCond);
      pthread_mutex_unlock(&sThreadCountLock);

      REAL(pthread_mutex_lock)(&sThreadFreezeLock);
    } else {
      RECREATE_CONTINUE();
      RECREATE_GATE();
    }
  }
}


















MFBT_API jmp_buf*
NuwaCheckpointCurrentThread1() {
  thread_info_t *tinfo = CUR_THREAD_INFO;
  if (sIsNuwaProcess &&
      (tinfo = CUR_THREAD_INFO) &&
      (tinfo->flags & TINFO_FLAG_NUWA_SUPPORT)) {
    return &tinfo->jmpEnv;
  }
  abort();
  return nullptr;
}

MFBT_API bool
NuwaCheckpointCurrentThread2(int setjmpCond) {
  thread_info_t *tinfo = CUR_THREAD_INFO;
  if (setjmpCond == 0) {
    REAL(pthread_mutex_lock)(&sThreadCountLock);
    if (!(tinfo->flags & TINFO_FLAG_NUWA_EXPLICIT_CHECKPOINT)) {
      tinfo->flags |= TINFO_FLAG_NUWA_EXPLICIT_CHECKPOINT;
      SaveTLSInfo(tinfo);
      sThreadFreezeCount++;
    }
    pthread_cond_signal(&sThreadChangeCond);
    pthread_mutex_unlock(&sThreadCountLock);
    return true;
  }
  RECREATE_CONTINUE();
  RECREATE_GATE();
  return false;               
}





MFBT_API void
NuwaAddConstructor(void (*construct)(void *), void *arg) {
  nuwa_construct_t ctr;
  ctr.construct = construct;
  ctr.arg = arg;
  sConstructors.push_back(ctr);
}





MFBT_API void
NuwaAddFinalConstructor(void (*construct)(void *), void *arg) {
  nuwa_construct_t ctr;
  ctr.construct = construct;
  ctr.arg = arg;
  sFinalConstructors.push_back(ctr);
}




MFBT_API bool
IsNuwaProcess() {
  return sIsNuwaProcess;
}




MFBT_API bool
IsNuwaReady() {
  return sNuwaReady;
}

}      
