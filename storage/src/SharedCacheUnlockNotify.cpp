











































#include "SharedCacheUnlockNotify.h"

#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "nsThreadUtils.h"

namespace {

class UnlockNotification
{
public:
  UnlockNotification()
  : mMutex("UnlockNotification mMutex"),
    mCondVar(mMutex, "UnlockNotification condVar"),
    mSignaled(false)
  { }

  void Wait()
  {
    mozilla::MutexAutoLock lock(mMutex);
    while (!mSignaled) {
      mCondVar.Wait();
    }
  }

  void Signal()
  {
    mozilla::MutexAutoLock lock(mMutex);
    mSignaled = true;
    mCondVar.Notify();
  }

private:
  mozilla::Mutex mMutex;
  mozilla::CondVar mCondVar;
  bool mSignaled;
};

void
UnlockNotifyCallback(void** apArg,
                     int nArg)
{
  for (int i = 0; i < nArg; i++) {
    UnlockNotification* notification =
      static_cast<UnlockNotification*>(apArg[i]);
    notification->Signal();
  }
}

int
WaitForUnlockNotify(sqlite3* db)
{
  UnlockNotification notification;
  int srv = ::sqlite3_unlock_notify(db, UnlockNotifyCallback, &notification);
  NS_ASSERTION(srv == SQLITE_LOCKED || srv == SQLITE_OK, "Bad result!");

  if (srv == SQLITE_OK) {
    notification.Wait();
  }

  return srv;
}

} 

namespace mozilla {
namespace storage {

int
moz_sqlite3_step(sqlite3_stmt* pStmt)
{
  bool checkedMainThread = false;

  int srv;
  while ((srv = ::sqlite3_step(pStmt)) == SQLITE_LOCKED) {
    if (!checkedMainThread) {
      checkedMainThread = true;
      if (NS_IsMainThread()) {
        NS_WARNING("We won't allow blocking on the main thread!");
        break;
      }
    }

    srv = WaitForUnlockNotify(sqlite3_db_handle(pStmt));
    if (srv != SQLITE_OK)
      break;

    sqlite3_reset(pStmt);
  }

  return srv;
}

int
moz_sqlite3_prepare_v2(sqlite3* db,
                       const char* zSql,
                       int nByte,
                       sqlite3_stmt** ppStmt,
                       const char** pzTail)
{
  bool checkedMainThread = false;

  int srv;
  while((srv = ::sqlite3_prepare_v2(db, zSql, nByte, ppStmt, pzTail)) ==
        SQLITE_LOCKED) {
    if (!checkedMainThread) {
      checkedMainThread = true;
      if (NS_IsMainThread()) {
        NS_WARNING("We won't allow blocking on the main thread!");
        break;
      }
    }

    srv = WaitForUnlockNotify(db);
    if (srv != SQLITE_OK)
      break;
  }

  return srv;
}

} 
} 
