




































































































#ifndef GOOGLE_MUTEX_H_
#define GOOGLE_MUTEX_H_

#include "config.h"           

#if defined(NO_THREADS)
  typedef int MutexType;      
#elif defined(_WIN32) || defined(__CYGWIN32__) || defined(__CYGWIN64__)
# define WIN32_LEAN_AND_MEAN
# ifdef GMUTEX_TRYLOCK
  
  
  
#   ifndef _WIN32_WINNT
#     define _WIN32_WINNT 0x0400
#   endif
# endif

# define NOGDI

# define NOMINMAX
# include <windows.h>
  typedef CRITICAL_SECTION MutexType;
#elif defined(HAVE_PTHREAD) && defined(HAVE_RWLOCK)
  
  
  
  
# ifdef __linux__
#   define _XOPEN_SOURCE 500  // may be needed to get the rwlock calls
# endif
# include <pthread.h>
  typedef pthread_rwlock_t MutexType;
#elif defined(HAVE_PTHREAD)
# include <pthread.h>
  typedef pthread_mutex_t MutexType;
#else
# error Need to implement mutex.h for your architecture, or #define NO_THREADS
#endif



#include <assert.h>
#include <stdlib.h>      

#define MUTEX_NAMESPACE glog_internal_namespace_

namespace MUTEX_NAMESPACE {

class Mutex {
 public:
  
  
  
  
  inline Mutex();

  
  inline ~Mutex();

  inline void Lock();    
  inline void Unlock();  
#ifdef GMUTEX_TRYLOCK
  inline bool TryLock(); 
#endif
  
  
  
  
  inline void ReaderLock();   
  inline void ReaderUnlock(); 
  inline void WriterLock() { Lock(); }     
  inline void WriterUnlock() { Unlock(); } 

  
  inline void AssertHeld() {}

 private:
  MutexType mutex_;
  
  
  
  volatile bool is_safe_;

  inline void SetIsSafe() { is_safe_ = true; }

  
  Mutex(Mutex* ) {}
  
  Mutex(const Mutex&);
  void operator=(const Mutex&);
};


#if defined(NO_THREADS)











Mutex::Mutex() : mutex_(0) { }
Mutex::~Mutex()            { assert(mutex_ == 0); }
void Mutex::Lock()         { assert(--mutex_ == -1); }
void Mutex::Unlock()       { assert(mutex_++ == -1); }
#ifdef GMUTEX_TRYLOCK
bool Mutex::TryLock()      { if (mutex_) return false; Lock(); return true; }
#endif
void Mutex::ReaderLock()   { assert(++mutex_ > 0); }
void Mutex::ReaderUnlock() { assert(mutex_-- > 0); }

#elif defined(_WIN32) || defined(__CYGWIN32__) || defined(__CYGWIN64__)

Mutex::Mutex()             { InitializeCriticalSection(&mutex_); SetIsSafe(); }
Mutex::~Mutex()            { DeleteCriticalSection(&mutex_); }
void Mutex::Lock()         { if (is_safe_) EnterCriticalSection(&mutex_); }
void Mutex::Unlock()       { if (is_safe_) LeaveCriticalSection(&mutex_); }
#ifdef GMUTEX_TRYLOCK
bool Mutex::TryLock()      { return is_safe_ ?
                                 TryEnterCriticalSection(&mutex_) != 0 : true; }
#endif
void Mutex::ReaderLock()   { Lock(); }      
void Mutex::ReaderUnlock() { Unlock(); }

#elif defined(HAVE_PTHREAD) && defined(HAVE_RWLOCK)

#define SAFE_PTHREAD(fncall)  do {   /* run fncall if is_safe_ is true */  \
  if (is_safe_ && fncall(&mutex_) != 0) abort();                           \
} while (0)

Mutex::Mutex() {
  SetIsSafe();
  if (is_safe_ && pthread_rwlock_init(&mutex_, NULL) != 0) abort();
}
Mutex::~Mutex()            { SAFE_PTHREAD(pthread_rwlock_destroy); }
void Mutex::Lock()         { SAFE_PTHREAD(pthread_rwlock_wrlock); }
void Mutex::Unlock()       { SAFE_PTHREAD(pthread_rwlock_unlock); }
#ifdef GMUTEX_TRYLOCK
bool Mutex::TryLock()      { return is_safe_ ?
                                    pthread_rwlock_trywrlock(&mutex_) == 0 :
                                    true; }
#endif
void Mutex::ReaderLock()   { SAFE_PTHREAD(pthread_rwlock_rdlock); }
void Mutex::ReaderUnlock() { SAFE_PTHREAD(pthread_rwlock_unlock); }
#undef SAFE_PTHREAD

#elif defined(HAVE_PTHREAD)

#define SAFE_PTHREAD(fncall)  do {   /* run fncall if is_safe_ is true */  \
  if (is_safe_ && fncall(&mutex_) != 0) abort();                           \
} while (0)

Mutex::Mutex()             {
  SetIsSafe();
  if (is_safe_ && pthread_mutex_init(&mutex_, NULL) != 0) abort();
}
Mutex::~Mutex()            { SAFE_PTHREAD(pthread_mutex_destroy); }
void Mutex::Lock()         { SAFE_PTHREAD(pthread_mutex_lock); }
void Mutex::Unlock()       { SAFE_PTHREAD(pthread_mutex_unlock); }
#ifdef GMUTEX_TRYLOCK
bool Mutex::TryLock()      { return is_safe_ ?
                                 pthread_mutex_trylock(&mutex_) == 0 : true; }
#endif
void Mutex::ReaderLock()   { Lock(); }
void Mutex::ReaderUnlock() { Unlock(); }
#undef SAFE_PTHREAD

#endif





class MutexLock {
 public:
  explicit MutexLock(Mutex *mu) : mu_(mu) { mu_->Lock(); }
  ~MutexLock() { mu_->Unlock(); }
 private:
  Mutex * const mu_;
  
  MutexLock(const MutexLock&);
  void operator=(const MutexLock&);
};


class ReaderMutexLock {
 public:
  explicit ReaderMutexLock(Mutex *mu) : mu_(mu) { mu_->ReaderLock(); }
  ~ReaderMutexLock() { mu_->ReaderUnlock(); }
 private:
  Mutex * const mu_;
  
  ReaderMutexLock(const ReaderMutexLock&);
  void operator=(const ReaderMutexLock&);
};

class WriterMutexLock {
 public:
  explicit WriterMutexLock(Mutex *mu) : mu_(mu) { mu_->WriterLock(); }
  ~WriterMutexLock() { mu_->WriterUnlock(); }
 private:
  Mutex * const mu_;
  
  WriterMutexLock(const WriterMutexLock&);
  void operator=(const WriterMutexLock&);
};


#define MutexLock(x) COMPILE_ASSERT(0, mutex_lock_decl_missing_var_name)
#define ReaderMutexLock(x) COMPILE_ASSERT(0, rmutex_lock_decl_missing_var_name)
#define WriterMutexLock(x) COMPILE_ASSERT(0, wmutex_lock_decl_missing_var_name)

}  

using namespace MUTEX_NAMESPACE;

#undef MUTEX_NAMESPACE

#endif  
