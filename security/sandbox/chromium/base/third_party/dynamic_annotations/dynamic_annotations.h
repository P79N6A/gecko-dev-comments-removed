
















































#ifndef __DYNAMIC_ANNOTATIONS_H__
#define __DYNAMIC_ANNOTATIONS_H__

#ifndef DYNAMIC_ANNOTATIONS_PREFIX
# define DYNAMIC_ANNOTATIONS_PREFIX
#endif

#ifndef DYNAMIC_ANNOTATIONS_PROVIDE_RUNNING_ON_VALGRIND
# define DYNAMIC_ANNOTATIONS_PROVIDE_RUNNING_ON_VALGRIND 1
#endif

#ifdef DYNAMIC_ANNOTATIONS_WANT_ATTRIBUTE_WEAK
# ifdef __GNUC__
#  define DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK __attribute__((weak))
# else


#  error weak annotations are not supported for your compiler
# endif
#else
# define DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK
#endif



#define DYNAMIC_ANNOTATIONS_GLUE0(A, B) A##B
#define DYNAMIC_ANNOTATIONS_GLUE(A, B) DYNAMIC_ANNOTATIONS_GLUE0(A, B)
#define DYNAMIC_ANNOTATIONS_NAME(name) \
  DYNAMIC_ANNOTATIONS_GLUE(DYNAMIC_ANNOTATIONS_PREFIX, name)

#ifndef DYNAMIC_ANNOTATIONS_ENABLED
# define DYNAMIC_ANNOTATIONS_ENABLED 0
#endif

#if DYNAMIC_ANNOTATIONS_ENABLED != 0

  





































  

  #define ANNOTATE_CONDVAR_LOCK_WAIT(cv, lock) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateCondVarWait)(__FILE__, __LINE__, cv, lock)

  

  #define ANNOTATE_CONDVAR_WAIT(cv) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateCondVarWait)(__FILE__, __LINE__, cv, NULL)

  

  #define ANNOTATE_CONDVAR_SIGNAL(cv) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateCondVarSignal)(__FILE__, __LINE__, cv)

  

  #define ANNOTATE_CONDVAR_SIGNAL_ALL(cv) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateCondVarSignalAll)(__FILE__, __LINE__, cv)

  
  #define ANNOTATE_HAPPENS_BEFORE(obj) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateHappensBefore)(__FILE__, __LINE__, obj)
  #define ANNOTATE_HAPPENS_AFTER(obj) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateHappensAfter)(__FILE__, __LINE__, obj)

  
  #define ANNOTATE_PUBLISH_MEMORY_RANGE(pointer, size) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotatePublishMemoryRange)(__FILE__, __LINE__, \
        pointer, size)

  
  #define ANNOTATE_UNPUBLISH_MEMORY_RANGE(pointer, size) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateUnpublishMemoryRange)(__FILE__, __LINE__, \
        pointer, size)

  
  #define ANNOTATE_SWAP_MEMORY_RANGE(pointer, size)   \
    do {                                              \
      ANNOTATE_UNPUBLISH_MEMORY_RANGE(pointer, size); \
      ANNOTATE_PUBLISH_MEMORY_RANGE(pointer, size);   \
    } while (0)

  






  #define ANNOTATE_PURE_HAPPENS_BEFORE_MUTEX(mu) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateMutexIsUsedAsCondVar)(__FILE__, __LINE__, \
        mu)

  


  #define ANNOTATE_NOT_HAPPENS_BEFORE_MUTEX(mu) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateMutexIsNotPHB)(__FILE__, __LINE__, mu)

  
  #define ANNOTATE_MUTEX_IS_USED_AS_CONDVAR(mu) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateMutexIsUsedAsCondVar)(__FILE__, __LINE__, \
        mu)

  



  



  #define ANNOTATE_NEW_MEMORY(address, size) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateNewMemory)(__FILE__, __LINE__, address, \
        size)

  



  



  #define ANNOTATE_PCQ_CREATE(pcq) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQCreate)(__FILE__, __LINE__, pcq)

  
  #define ANNOTATE_PCQ_DESTROY(pcq) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQDestroy)(__FILE__, __LINE__, pcq)

  

  #define ANNOTATE_PCQ_PUT(pcq) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQPut)(__FILE__, __LINE__, pcq)

  

  #define ANNOTATE_PCQ_GET(pcq) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQGet)(__FILE__, __LINE__, pcq)

  




  



  #define ANNOTATE_BENIGN_RACE(pointer, description) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateBenignRaceSized)(__FILE__, __LINE__, \
        pointer, sizeof(*(pointer)), description)

  

  #define ANNOTATE_BENIGN_RACE_SIZED(address, size, description) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateBenignRaceSized)(__FILE__, __LINE__, \
        address, size, description)

  




  #define ANNOTATE_IGNORE_READS_BEGIN() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreReadsBegin)(__FILE__, __LINE__)

  
  #define ANNOTATE_IGNORE_READS_END() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreReadsEnd)(__FILE__, __LINE__)

  
  #define ANNOTATE_IGNORE_WRITES_BEGIN() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreWritesBegin)(__FILE__, __LINE__)

  
  #define ANNOTATE_IGNORE_WRITES_END() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreWritesEnd)(__FILE__, __LINE__)

  
  #define ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN() \
    do {\
      ANNOTATE_IGNORE_READS_BEGIN();\
      ANNOTATE_IGNORE_WRITES_BEGIN();\
    }while(0)\

  
  #define ANNOTATE_IGNORE_READS_AND_WRITES_END() \
    do {\
      ANNOTATE_IGNORE_WRITES_END();\
      ANNOTATE_IGNORE_READS_END();\
    }while(0)\

  

  #define ANNOTATE_IGNORE_SYNC_BEGIN() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreSyncBegin)(__FILE__, __LINE__)

  
  #define ANNOTATE_IGNORE_SYNC_END() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreSyncEnd)(__FILE__, __LINE__)


  


  #define ANNOTATE_ENABLE_RACE_DETECTION(enable) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateEnableRaceDetection)(__FILE__, __LINE__, \
        enable)

  


  
  #define ANNOTATE_TRACE_MEMORY(address) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateTraceMemory)(__FILE__, __LINE__, address)

  
  #define ANNOTATE_THREAD_NAME(name) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateThreadName)(__FILE__, __LINE__, name)

  




  
  #define ANNOTATE_RWLOCK_CREATE(lock) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockCreate)(__FILE__, __LINE__, lock)

  
  #define ANNOTATE_RWLOCK_DESTROY(lock) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockDestroy)(__FILE__, __LINE__, lock)

  

  #define ANNOTATE_RWLOCK_ACQUIRED(lock, is_w) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockAcquired)(__FILE__, __LINE__, lock, \
        is_w)

  
  #define ANNOTATE_RWLOCK_RELEASED(lock, is_w) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockReleased)(__FILE__, __LINE__, lock, \
        is_w)

  




  


  #define ANNOTATE_BARRIER_INIT(barrier, count, reinitialization_allowed) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierInit)(__FILE__, __LINE__, barrier, \
        count, reinitialization_allowed)

  
  #define ANNOTATE_BARRIER_WAIT_BEFORE(barrier) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierWaitBefore)(__FILE__, __LINE__, \
        barrier)

  
  #define ANNOTATE_BARRIER_WAIT_AFTER(barrier) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierWaitAfter)(__FILE__, __LINE__, \
        barrier)

  
  #define ANNOTATE_BARRIER_DESTROY(barrier) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierDestroy)(__FILE__, __LINE__, \
        barrier)

  


  

  #define ANNOTATE_EXPECT_RACE(address, description) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateExpectRace)(__FILE__, __LINE__, address, \
        description)

  #define ANNOTATE_FLUSH_EXPECTED_RACES() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateFlushExpectedRaces)(__FILE__, __LINE__)

  
  #define ANNOTATE_NO_OP(arg) \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateNoOp)(__FILE__, __LINE__, arg)

  

  #define ANNOTATE_FLUSH_STATE() \
    DYNAMIC_ANNOTATIONS_NAME(AnnotateFlushState)(__FILE__, __LINE__)


#else  

  #define ANNOTATE_RWLOCK_CREATE(lock)
  #define ANNOTATE_RWLOCK_DESTROY(lock) /* empty */
  #define ANNOTATE_RWLOCK_ACQUIRED(lock, is_w)
  #define ANNOTATE_RWLOCK_RELEASED(lock, is_w) /* empty */
  #define ANNOTATE_BARRIER_INIT(barrier, count, reinitialization_allowed)
  #define ANNOTATE_BARRIER_WAIT_BEFORE(barrier) /* empty */
  #define ANNOTATE_BARRIER_WAIT_AFTER(barrier)
  #define ANNOTATE_BARRIER_DESTROY(barrier) /* empty */
  #define ANNOTATE_CONDVAR_LOCK_WAIT(cv, lock)
  #define ANNOTATE_CONDVAR_WAIT(cv) /* empty */
  #define ANNOTATE_CONDVAR_SIGNAL(cv)
  #define ANNOTATE_CONDVAR_SIGNAL_ALL(cv) /* empty */
  #define ANNOTATE_HAPPENS_BEFORE(obj)
  #define ANNOTATE_HAPPENS_AFTER(obj) /* empty */
  #define ANNOTATE_PUBLISH_MEMORY_RANGE(address, size)
  #define ANNOTATE_UNPUBLISH_MEMORY_RANGE(address, size)  /* empty */
  #define ANNOTATE_SWAP_MEMORY_RANGE(address, size)
  #define ANNOTATE_PCQ_CREATE(pcq) /* empty */
  #define ANNOTATE_PCQ_DESTROY(pcq)
  #define ANNOTATE_PCQ_PUT(pcq) /* empty */
  #define ANNOTATE_PCQ_GET(pcq)
  #define ANNOTATE_NEW_MEMORY(address, size) /* empty */
  #define ANNOTATE_EXPECT_RACE(address, description)
  #define ANNOTATE_FLUSH_EXPECTED_RACES(address, description) /* empty */
  #define ANNOTATE_BENIGN_RACE(address, description)
  #define ANNOTATE_BENIGN_RACE_SIZED(address, size, description) /* empty */
  #define ANNOTATE_PURE_HAPPENS_BEFORE_MUTEX(mu)
  #define ANNOTATE_MUTEX_IS_USED_AS_CONDVAR(mu) /* empty */
  #define ANNOTATE_TRACE_MEMORY(arg)
  #define ANNOTATE_THREAD_NAME(name) /* empty */
  #define ANNOTATE_IGNORE_READS_BEGIN()
  #define ANNOTATE_IGNORE_READS_END() /* empty */
  #define ANNOTATE_IGNORE_WRITES_BEGIN()
  #define ANNOTATE_IGNORE_WRITES_END() /* empty */
  #define ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN()
  #define ANNOTATE_IGNORE_READS_AND_WRITES_END() /* empty */
  #define ANNOTATE_IGNORE_SYNC_BEGIN()
  #define ANNOTATE_IGNORE_SYNC_END() /* empty */
  #define ANNOTATE_ENABLE_RACE_DETECTION(enable)
  #define ANNOTATE_NO_OP(arg) /* empty */
  #define ANNOTATE_FLUSH_STATE()

#endif  


#ifdef __cplusplus
extern "C" {
#endif


void DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockCreate)(
    const char *file, int line,
    const volatile void *lock) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockDestroy)(
    const char *file, int line,
    const volatile void *lock) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockAcquired)(
    const char *file, int line,
    const volatile void *lock, long is_w) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateRWLockReleased)(
    const char *file, int line,
    const volatile void *lock, long is_w) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierInit)(
    const char *file, int line, const volatile void *barrier, long count,
    long reinitialization_allowed) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierWaitBefore)(
    const char *file, int line,
    const volatile void *barrier) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierWaitAfter)(
    const char *file, int line,
    const volatile void *barrier) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateBarrierDestroy)(
    const char *file, int line,
    const volatile void *barrier) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateCondVarWait)(
    const char *file, int line, const volatile void *cv,
    const volatile void *lock) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateCondVarSignal)(
    const char *file, int line,
    const volatile void *cv) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateCondVarSignalAll)(
    const char *file, int line,
    const volatile void *cv) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateHappensBefore)(
    const char *file, int line,
    const volatile void *obj) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateHappensAfter)(
    const char *file, int line,
    const volatile void *obj) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotatePublishMemoryRange)(
    const char *file, int line,
    const volatile void *address, long size) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateUnpublishMemoryRange)(
    const char *file, int line,
    const volatile void *address, long size) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQCreate)(
    const char *file, int line,
    const volatile void *pcq) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQDestroy)(
    const char *file, int line,
    const volatile void *pcq) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQPut)(
    const char *file, int line,
    const volatile void *pcq) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotatePCQGet)(
    const char *file, int line,
    const volatile void *pcq) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateNewMemory)(
    const char *file, int line,
    const volatile void *mem, long size) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateExpectRace)(
    const char *file, int line, const volatile void *mem,
    const char *description) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateFlushExpectedRaces)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateBenignRace)(
    const char *file, int line, const volatile void *mem,
    const char *description) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateBenignRaceSized)(
    const char *file, int line, const volatile void *mem, long size,
    const char *description) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateMutexIsUsedAsCondVar)(
    const char *file, int line,
    const volatile void *mu) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateMutexIsNotPHB)(
    const char *file, int line,
    const volatile void *mu) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateTraceMemory)(
    const char *file, int line,
    const volatile void *arg) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateThreadName)(
    const char *file, int line,
    const char *name) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreReadsBegin)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreReadsEnd)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreWritesBegin)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreWritesEnd)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreSyncBegin)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateIgnoreSyncEnd)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateEnableRaceDetection)(
    const char *file, int line, int enable) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateNoOp)(
    const char *file, int line,
    const volatile void *arg) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
void DYNAMIC_ANNOTATIONS_NAME(AnnotateFlushState)(
    const char *file, int line) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;

#if DYNAMIC_ANNOTATIONS_PROVIDE_RUNNING_ON_VALGRIND == 1















int RunningOnValgrind(void) DYNAMIC_ANNOTATIONS_ATTRIBUTE_WEAK;
#endif 

#ifdef __cplusplus
}
#endif

#if DYNAMIC_ANNOTATIONS_ENABLED != 0 && defined(__cplusplus)

  







  template <class T>
  inline T ANNOTATE_UNPROTECTED_READ(const volatile T &x) {
    ANNOTATE_IGNORE_READS_BEGIN();
    T res = x;
    ANNOTATE_IGNORE_READS_END();
    return res;
  }
  
  #define ANNOTATE_BENIGN_RACE_STATIC(static_var, description)        \
    namespace {                                                       \
      class static_var ## _annotator {                                \
       public:                                                        \
        static_var ## _annotator() {                                  \
          ANNOTATE_BENIGN_RACE_SIZED(&static_var,                     \
                                      sizeof(static_var),             \
            # static_var ": " description);                           \
        }                                                             \
      };                                                              \
      static static_var ## _annotator the ## static_var ## _annotator;\
    }
#else 

  #define ANNOTATE_UNPROTECTED_READ(x) (x)
  #define ANNOTATE_BENIGN_RACE_STATIC(static_var, description)

#endif 

#endif
