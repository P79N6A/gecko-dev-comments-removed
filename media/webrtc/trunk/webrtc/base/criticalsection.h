









#ifndef WEBRTC_BASE_CRITICALSECTION_H__
#define WEBRTC_BASE_CRITICALSECTION_H__

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/thread_annotations.h"

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#endif

#if defined(WEBRTC_POSIX)
#include <pthread.h>
#endif

#ifdef _DEBUG
#define CS_TRACK_OWNER 1
#endif  

#if CS_TRACK_OWNER
#define TRACK_OWNER(x) x
#else  
#define TRACK_OWNER(x)
#endif  

namespace rtc {

#if defined(WEBRTC_WIN)
class LOCKABLE CriticalSection {
 public:
  CriticalSection() {
    InitializeCriticalSection(&crit_);
    
    TRACK_OWNER(thread_ = 0);
  }
  ~CriticalSection() {
    DeleteCriticalSection(&crit_);
  }
  void Enter() EXCLUSIVE_LOCK_FUNCTION() {
    EnterCriticalSection(&crit_);
    TRACK_OWNER(thread_ = GetCurrentThreadId());
  }
  bool TryEnter() EXCLUSIVE_TRYLOCK_FUNCTION(true) {
    if (TryEnterCriticalSection(&crit_) != FALSE) {
      TRACK_OWNER(thread_ = GetCurrentThreadId());
      return true;
    }
    return false;
  }
  void Leave() UNLOCK_FUNCTION() {
    TRACK_OWNER(thread_ = 0);
    LeaveCriticalSection(&crit_);
  }

#if CS_TRACK_OWNER
  bool CurrentThreadIsOwner() const { return thread_ == GetCurrentThreadId(); }
#endif  

 private:
  CRITICAL_SECTION crit_;
  TRACK_OWNER(DWORD thread_);  
};
#endif 

#if defined(WEBRTC_POSIX)
class LOCKABLE CriticalSection {
 public:
  CriticalSection() {
    pthread_mutexattr_t mutex_attribute;
    pthread_mutexattr_init(&mutex_attribute);
    pthread_mutexattr_settype(&mutex_attribute, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_, &mutex_attribute);
    pthread_mutexattr_destroy(&mutex_attribute);
    TRACK_OWNER(thread_ = 0);
  }
  ~CriticalSection() {
    pthread_mutex_destroy(&mutex_);
  }
  void Enter() EXCLUSIVE_LOCK_FUNCTION() {
    pthread_mutex_lock(&mutex_);
    TRACK_OWNER(thread_ = pthread_self());
  }
  bool TryEnter() EXCLUSIVE_TRYLOCK_FUNCTION(true) {
    if (pthread_mutex_trylock(&mutex_) == 0) {
      TRACK_OWNER(thread_ = pthread_self());
      return true;
    }
    return false;
  }
  void Leave() UNLOCK_FUNCTION() {
    TRACK_OWNER(thread_ = 0);
    pthread_mutex_unlock(&mutex_);
  }

#if CS_TRACK_OWNER
  bool CurrentThreadIsOwner() const { return pthread_equal(thread_, pthread_self()); }
#endif  

 private:
  pthread_mutex_t mutex_;
  TRACK_OWNER(pthread_t thread_);
};
#endif 


class SCOPED_LOCKABLE CritScope {
 public:
  explicit CritScope(CriticalSection *pcrit) EXCLUSIVE_LOCK_FUNCTION(pcrit) {
    pcrit_ = pcrit;
    pcrit_->Enter();
  }
  ~CritScope() UNLOCK_FUNCTION() {
    pcrit_->Leave();
  }
 private:
  CriticalSection *pcrit_;
  DISALLOW_COPY_AND_ASSIGN(CritScope);
};








class TryCritScope {
 public:
  explicit TryCritScope(CriticalSection *pcrit) {
    pcrit_ = pcrit;
    locked_ = pcrit_->TryEnter();
  }
  ~TryCritScope() {
    if (locked_) {
      pcrit_->Leave();
    }
  }
  bool locked() const {
    return locked_;
  }
 private:
  CriticalSection *pcrit_;
  bool locked_;
  DISALLOW_COPY_AND_ASSIGN(TryCritScope);
};



class AtomicOps {
 public:
#if defined(WEBRTC_WIN)
  
  static int Increment(int* i) {
    return ::InterlockedIncrement(reinterpret_cast<LONG*>(i));
  }
  static int Decrement(int* i) {
    return ::InterlockedDecrement(reinterpret_cast<LONG*>(i));
  }
#else
  static int Increment(int* i) {
    return __sync_add_and_fetch(i, 1);
  }
  static int Decrement(int* i) {
    return __sync_sub_and_fetch(i, 1);
  }
#endif
};

} 

#endif 
