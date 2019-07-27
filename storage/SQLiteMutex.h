





#ifndef mozilla_storage_SQLiteMutex_h_
#define mozilla_storage_SQLiteMutex_h_

#include "mozilla/BlockingResourceBase.h"
#include "sqlite3.h"

namespace mozilla {
namespace storage {








class SQLiteMutex : private BlockingResourceBase
{
public:
  





  explicit SQLiteMutex(const char *aName)
  : BlockingResourceBase(aName, eMutex)
  , mMutex(nullptr)
  {
  }

  







  void initWithMutex(sqlite3_mutex *aMutex)
  {
    NS_ASSERTION(aMutex, "You must pass in a valid mutex!");
    NS_ASSERTION(!mMutex, "A mutex has already been set for this!");
    mMutex = aMutex;
  }

#if !defined(DEBUG) || defined(MOZ_NATIVE_SQLITE)
  


  void lock()
  {
    sqlite3_mutex_enter(mMutex);
  }

  


  void unlock()
  {
    sqlite3_mutex_leave(mMutex);
  }

  


  void assertCurrentThreadOwns()
  {
  }

  


  void assertNotCurrentThreadOwns()
  {
  }

#else
  void lock()
  {
    NS_ASSERTION(mMutex, "No mutex associated with this wrapper!");

    
    

    CheckAcquire();
    sqlite3_mutex_enter(mMutex);
    Acquire(); 
  }

  void unlock()
  {
    NS_ASSERTION(mMutex, "No mutex associated with this wrapper!");

    
    
    Release(); 
    sqlite3_mutex_leave(mMutex);
  }

  void assertCurrentThreadOwns()
  {
    NS_ASSERTION(mMutex, "No mutex associated with this wrapper!");
    NS_ASSERTION(sqlite3_mutex_held(mMutex),
                 "Mutex is not held, but we expect it to be!");
  }

  void assertNotCurrentThreadOwns()
  {
    NS_ASSERTION(mMutex, "No mutex associated with this wrapper!");
    NS_ASSERTION(sqlite3_mutex_notheld(mMutex),
                 "Mutex is held, but we expect it to not be!");
  }
#endif 

private:
  sqlite3_mutex *mMutex;
};





class MOZ_STACK_CLASS SQLiteMutexAutoLock
{
public:
  explicit SQLiteMutexAutoLock(SQLiteMutex &aMutex)
  : mMutex(aMutex)
  {
    mMutex.lock();
  }

  ~SQLiteMutexAutoLock()
  {
    mMutex.unlock();
  }

private:
  SQLiteMutex &mMutex;
};





class MOZ_STACK_CLASS SQLiteMutexAutoUnlock
{
public:
  explicit SQLiteMutexAutoUnlock(SQLiteMutex &aMutex)
  : mMutex(aMutex)
  {
    mMutex.unlock();
  }

  ~SQLiteMutexAutoUnlock()
  {
    mMutex.lock();
  }

private:
  SQLiteMutex &mMutex;
};

} 
} 

#endif 
