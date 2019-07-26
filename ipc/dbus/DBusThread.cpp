






















#include "DBusThread.h"
#include "RawDBusConnection.h"
#include "DBusUtils.h"

#include <dbus/dbus.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <list>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"
#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "mozilla/SyncRunnable.h"
#include "mozilla/NullPtr.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Monitor.h"
#include "mozilla/FileUtils.h"
#include "nsThreadUtils.h"
#include "nsIThread.h"
#include "nsXULAppAPI.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"

#undef CHROMIUM_LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GonkDBus", args);
#else
#define BTDEBUG true
#define CHROMIUM_LOG(args...) if (BTDEBUG) printf(args);
#endif

namespace mozilla {
namespace ipc {

class DBusWatcher : public MessageLoopForIO::Watcher
{
public:
  DBusWatcher(RawDBusConnection* aConnection, DBusWatch* aWatch)
  : mConnection(aConnection),
    mWatch(aWatch)
  {
    MOZ_ASSERT(mConnection);
    MOZ_ASSERT(mWatch);
  }

  ~DBusWatcher()
  { }

  void StartWatching();
  void StopWatching();

  static void        FreeFunction(void* aData);
  static dbus_bool_t AddWatchFunction(DBusWatch* aWatch, void* aData);
  static void        RemoveWatchFunction(DBusWatch* aWatch, void* aData);
  static void        ToggleWatchFunction(DBusWatch* aWatch, void* aData);

  RawDBusConnection* GetConnection();

private:
  void OnFileCanReadWithoutBlocking(int aFd);
  void OnFileCanWriteWithoutBlocking(int aFd);

  
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;

  
  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;

  
  RawDBusConnection* mConnection;
  DBusWatch* mWatch;
};

RawDBusConnection*
DBusWatcher::GetConnection()
{
  return mConnection;
}

void DBusWatcher::StartWatching()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(mWatch);

  int fd = dbus_watch_get_unix_fd(mWatch);

  MessageLoopForIO* ioLoop = MessageLoopForIO::current();
  ioLoop->WatchFileDescriptor(fd, true, MessageLoopForIO::WATCH_READ,
                              &mReadWatcher, this);
  ioLoop->WatchFileDescriptor(fd, true, MessageLoopForIO::WATCH_WRITE,
                              &mWriteWatcher, this);
}

void DBusWatcher::StopWatching()
{
  MOZ_ASSERT(!NS_IsMainThread());

  mReadWatcher.StopWatchingFileDescriptor();
  mWriteWatcher.StopWatchingFileDescriptor();
}



void
DBusWatcher::FreeFunction(void* aData)
{
  delete static_cast<DBusWatcher*>(aData);
}

dbus_bool_t
DBusWatcher::AddWatchFunction(DBusWatch* aWatch, void* aData)
{
  MOZ_ASSERT(!NS_IsMainThread());

  RawDBusConnection* connection = static_cast<RawDBusConnection*>(aData);

  DBusWatcher* dbusWatcher = new DBusWatcher(connection, aWatch);
  dbus_watch_set_data(aWatch, dbusWatcher, DBusWatcher::FreeFunction);

  if (dbus_watch_get_enabled(aWatch)) {
    dbusWatcher->StartWatching();
  }

  return TRUE;
}

void
DBusWatcher::RemoveWatchFunction(DBusWatch* aWatch, void* aData)
{
  MOZ_ASSERT(!NS_IsMainThread());

  DBusWatcher* dbusWatcher =
    static_cast<DBusWatcher*>(dbus_watch_get_data(aWatch));
  dbusWatcher->StopWatching();
}

void
DBusWatcher::ToggleWatchFunction(DBusWatch* aWatch, void* aData)
{
  MOZ_ASSERT(!NS_IsMainThread());

  DBusWatcher* dbusWatcher =
    static_cast<DBusWatcher*>(dbus_watch_get_data(aWatch));

  if (dbus_watch_get_enabled(aWatch)) {
    dbusWatcher->StartWatching();
  } else {
    dbusWatcher->StopWatching();
  }
}

void
DBusWatcher::OnFileCanReadWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());

  dbus_watch_handle(mWatch, DBUS_WATCH_READABLE);

  DBusDispatchStatus dbusDispatchStatus;
  do {
    dbusDispatchStatus =
      dbus_connection_dispatch(mConnection->GetConnection());
  } while (dbusDispatchStatus == DBUS_DISPATCH_DATA_REMAINS);
}

void
DBusWatcher::OnFileCanWriteWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());

  dbus_watch_handle(mWatch, DBUS_WATCH_WRITABLE);
}

class WatchDBusConnectionTask : public Task
{
public:
  WatchDBusConnectionTask(RawDBusConnection* aConnection)
  : mConnection(aConnection)
  {
    MOZ_ASSERT(mConnection);
  }

  void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    dbus_bool_t success =
      dbus_connection_set_watch_functions(mConnection->GetConnection(),
                                          DBusWatcher::AddWatchFunction,
                                          DBusWatcher::RemoveWatchFunction,
                                          DBusWatcher::ToggleWatchFunction,
                                          mConnection, nullptr);
    NS_ENSURE_TRUE_VOID(success == TRUE);
  }

private:
  RawDBusConnection* mConnection;
};

class DeleteDBusConnectionTask : public Task
{
public:
  DeleteDBusConnectionTask(RawDBusConnection* aConnection)
  : mConnection(aConnection)
  {
    MOZ_ASSERT(mConnection);
  }

  void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    
    
    delete mConnection;
  }

private:
  RawDBusConnection* mConnection;
};

 

static RawDBusConnection* gDBusConnection;

bool
StartDBus()
{
  MOZ_ASSERT(!NS_IsMainThread());
  NS_ENSURE_TRUE(!gDBusConnection, true);

  RawDBusConnection* connection = new RawDBusConnection();
  nsresult rv = connection->EstablishDBusConnection();
  NS_ENSURE_SUCCESS(rv, false);

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
    new WatchDBusConnectionTask(connection));

  gDBusConnection = connection;

  return true;
}

bool
StopDBus()
{
  MOZ_ASSERT(!NS_IsMainThread());
  NS_ENSURE_TRUE(gDBusConnection, true);

  RawDBusConnection* connection = gDBusConnection;
  gDBusConnection = nullptr;

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
    new DeleteDBusConnectionTask(connection));

  return true;
}

nsresult
DispatchToDBusThread(Task* task)
{
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, task);

  return NS_OK;
}

RawDBusConnection*
GetDBusConnection()
{
  NS_ENSURE_TRUE(gDBusConnection, nullptr);

  return gDBusConnection;
}

}
}
