






































#include "storage_test_harness.h"

#include "SQLiteMutex.h"

using namespace mozilla;
using namespace mozilla::storage;





void
test_AutoLock()
{
  int lockTypes[] = {
    SQLITE_MUTEX_FAST,
    SQLITE_MUTEX_RECURSIVE,
  };
  for (size_t i = 0; i < ArrayLength(lockTypes); i++) {
    
    
    SQLiteMutex mutex("TestMutex");
    sqlite3_mutex *inner = sqlite3_mutex_alloc(lockTypes[i]);
    do_check_true(inner);
    mutex.initWithMutex(inner);

    
    mutex.assertNotCurrentThreadOwns();
    {
      SQLiteMutexAutoLock lockedScope(mutex);
      mutex.assertCurrentThreadOwns();
    }
    mutex.assertNotCurrentThreadOwns();

    
    sqlite3_mutex_free(inner);
  }
}

void
test_AutoUnlock()
{
  int lockTypes[] = {
    SQLITE_MUTEX_FAST,
    SQLITE_MUTEX_RECURSIVE,
  };
  for (size_t i = 0; i < ArrayLength(lockTypes); i++) {
    
    
    SQLiteMutex mutex("TestMutex");
    sqlite3_mutex *inner = sqlite3_mutex_alloc(lockTypes[i]);
    do_check_true(inner);
    mutex.initWithMutex(inner);

    
    {
      SQLiteMutexAutoLock lockedScope(mutex);

      {
        SQLiteMutexAutoUnlock unlockedScope(mutex);
        mutex.assertNotCurrentThreadOwns();
      }
      mutex.assertCurrentThreadOwns();
    }

    
    sqlite3_mutex_free(inner);
  }
}

void (*gTests[])(void) = {
  test_AutoLock,
  test_AutoUnlock,
};

const char *file = __FILE__;
#define TEST_NAME "SQLiteMutex"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
