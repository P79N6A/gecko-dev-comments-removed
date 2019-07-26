






















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
#include "mozilla/Util.h"
#include "mozilla/FileUtils.h"
#include "nsThreadUtils.h"
#include "nsIThread.h"
#include "nsXULAppAPI.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"

#undef LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GonkDBus", args);
#else
#define BTDEBUG true
#define LOG(args...) if (BTDEBUG) printf(args);
#endif

#define DEFAULT_INITIAL_POLLFD_COUNT 8




enum DBusEventTypes {
  DBUS_EVENT_LOOP_EXIT = 1,
  DBUS_EVENT_LOOP_ADD = 2,
  DBUS_EVENT_LOOP_REMOVE = 3,
  DBUS_EVENT_LOOP_WAKEUP = 4
};

static unsigned int UnixEventsToDBusFlags(short events)
{
  return (events & DBUS_WATCH_READABLE ? POLLIN : 0) |
    (events & DBUS_WATCH_WRITABLE ? POLLOUT : 0) |
    (events & DBUS_WATCH_ERROR ? POLLERR : 0) |
    (events & DBUS_WATCH_HANGUP ? POLLHUP : 0);
}

static short DBusFlagsToUnixEvents(unsigned int flags)
{
  return (flags & POLLIN ? DBUS_WATCH_READABLE : 0) |
    (flags & POLLOUT ? DBUS_WATCH_WRITABLE : 0) |
    (flags & POLLERR ? DBUS_WATCH_ERROR : 0) |
    (flags & POLLHUP ? DBUS_WATCH_HANGUP : 0);
}

namespace mozilla {
namespace ipc {

struct PollFdComparator {
  bool Equals(const pollfd& a, const pollfd& b) const {
    return ((a.fd == b.fd) &&
            (a.events == b.events));
  }
  bool LessThan(const pollfd& a, const pollfd&b) const {
    return false;
  }
};



struct DBusThread : public RawDBusConnection
{
  DBusThread();
  ~DBusThread();

  bool Initialize();
  void CleanUp();

  void WakeUp();

  
  
  
  nsTArray<pollfd> mPollData;
  nsTArray<DBusWatch*> mWatchData;

  
  
  ScopedClose mControlFdR;
  ScopedClose mControlFdW;

protected:
  bool SetUp();
};




static dbus_bool_t
AddWatch(DBusWatch *aWatch, void *aData)
{
  DBusThread *dbt = (DBusThread *)aData;

  if (dbus_watch_get_enabled(aWatch)) {
    
    
    
    
    char control = DBUS_EVENT_LOOP_ADD;
    if (write(dbt->mControlFdW.get(), &control, sizeof(char)) < 0) {
      LOG("Cannot write DBus add watch control data to socket!\n");
      return false;
    }

    int fd = dbus_watch_get_unix_fd(aWatch);
    if (write(dbt->mControlFdW.get(), &fd, sizeof(int)) < 0) {
      LOG("Cannot write DBus add watch descriptor data to socket!\n");
      return false;
    }

    unsigned int flags = dbus_watch_get_flags(aWatch);
    if (write(dbt->mControlFdW.get(), &flags, sizeof(unsigned int)) < 0) {
      LOG("Cannot write DBus add watch flag data to socket!\n");
      return false;
    }

    if (write(dbt->mControlFdW.get(), &aWatch, sizeof(DBusWatch*)) < 0) {
      LOG("Cannot write DBus add watch struct data to socket!\n");
      return false;
    }
  }
  return true;
}

static void
RemoveWatch(DBusWatch *aWatch, void *aData)
{
  DBusThread *dbt = (DBusThread *)aData;

  char control = DBUS_EVENT_LOOP_REMOVE;
  if (write(dbt->mControlFdW.get(), &control, sizeof(char)) < 0) {
    LOG("Cannot write DBus remove watch control data to socket!\n");
    return;
  }

  int fd = dbus_watch_get_unix_fd(aWatch);
  if (write(dbt->mControlFdW.get(), &fd, sizeof(int)) < 0) {
    LOG("Cannot write DBus remove watch descriptor data to socket!\n");
    return;
  }

  unsigned int flags = dbus_watch_get_flags(aWatch);
  if (write(dbt->mControlFdW.get(), &flags, sizeof(unsigned int)) < 0) {
    LOG("Cannot write DBus remove watch flag data to socket!\n");
    return;
  }
}

static void
ToggleWatch(DBusWatch *aWatch, void *aData)
{
  if (dbus_watch_get_enabled(aWatch)) {
    AddWatch(aWatch, aData);
  } else {
    RemoveWatch(aWatch, aData);
  }
}

static void
HandleWatchAdd(DBusThread* aDbt)
{
  DBusWatch *watch;
  int newFD;
  unsigned int flags;
  if (read(aDbt->mControlFdR.get(), &newFD, sizeof(int)) < 0) {
    LOG("Cannot read DBus watch add descriptor data from socket!\n");
    return;
  }
  if (read(aDbt->mControlFdR.get(), &flags, sizeof(unsigned int)) < 0) {
    LOG("Cannot read DBus watch add flag data from socket!\n");
    return;
  }
  if (read(aDbt->mControlFdR.get(), &watch, sizeof(DBusWatch *)) < 0) {
    LOG("Cannot read DBus watch add watch data from socket!\n");
    return;
  }
  short events = DBusFlagsToUnixEvents(flags);

  pollfd p;
  p.fd = newFD;
  p.revents = 0;
  p.events = events;
  if (aDbt->mPollData.Contains(p, PollFdComparator())) return;
  aDbt->mPollData.AppendElement(p);
  aDbt->mWatchData.AppendElement(watch);
}

static void
HandleWatchRemove(DBusThread* aDbt)
{
  int removeFD;
  unsigned int flags;

  if (read(aDbt->mControlFdR.get(), &removeFD, sizeof(int)) < 0) {
    LOG("Cannot read DBus watch remove descriptor data from socket!\n");
    return;
  }
  if (read(aDbt->mControlFdR.get(), &flags, sizeof(unsigned int)) < 0) {
    LOG("Cannot read DBus watch remove flag data from socket!\n");
    return;
  }
  short events = DBusFlagsToUnixEvents(flags);
  pollfd p;
  p.fd = removeFD;
  p.events = events;
  int index = aDbt->mPollData.IndexOf(p, 0, PollFdComparator());
  
  
  
  
  if (index < 0) {
    LOG("DBus requested watch removal of non-existant socket, ignoring...");
    return;
  }
  aDbt->mPollData.RemoveElementAt(index);

  
  
  aDbt->mWatchData.RemoveElementAt(index);
}

static
void DBusWakeup(void* aData)
{
  MOZ_ASSERT(aData);
  DBusThread* dbusThread = static_cast<DBusThread*>(aData);
  dbusThread->WakeUp();
}



DBusThread::DBusThread()
{
}

DBusThread::~DBusThread()
{

}

bool
DBusThread::SetUp()
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  if (mConnection) {
    return false;
  }

  
  
  
  
  
  

  int sockets[2];
  if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets) < 0) {
    return false;
  }

  mControlFdR.rwget() = sockets[0];
  mControlFdW.rwget() = sockets[1];

  pollfd *p = mPollData.AppendElement();

  p->fd = mControlFdR.get();
  p->events = POLLIN;
  p->revents = 0;

  
  
  

  mWatchData.AppendElement(static_cast<DBusWatch*>(nullptr));

  
  nsresult rv = EstablishDBusConnection();
  if (NS_FAILED(rv)) {
    NS_WARNING("Cannot create DBus Connection for DBus Thread!");
    return false;
  }

  dbus_bool_t success =
    dbus_connection_set_watch_functions(mConnection, AddWatch, RemoveWatch,
                                        ToggleWatch, this, nullptr);
  NS_ENSURE_TRUE(success == TRUE, false);

  dbus_connection_set_wakeup_main_function(mConnection, DBusWakeup, this, nullptr);

  return true;
}

bool
DBusThread::Initialize()
{
  if (!SetUp()) {
    CleanUp();
    return false;
  }

  return true;
}

void
DBusThread::CleanUp()
{
  MOZ_ASSERT(!NS_IsMainThread());

  dbus_connection_set_wakeup_main_function(mConnection, nullptr, nullptr, nullptr);

  dbus_bool_t success = dbus_connection_set_watch_functions(mConnection, nullptr,
                                                            nullptr, nullptr,
                                                            nullptr, nullptr);
  if (success != TRUE) {
    NS_WARNING("dbus_connection_set_watch_functions failed");
  }

#ifdef DEBUG
  LOG("Removing DBus Sockets\n");
#endif
  if (mControlFdW.get()) {
    mControlFdW.dispose();
  }
  if (mControlFdR.get()) {
    mControlFdR.dispose();
  }
  mPollData.Clear();

  
  
  mWatchData.Clear();
}

void
DBusThread::WakeUp()
{
  static const char control = DBUS_EVENT_LOOP_WAKEUP;

  struct pollfd fds = {
    mControlFdW.get(),
    POLLOUT,
    0
  };

  int nfds = TEMP_FAILURE_RETRY(poll(&fds, 1, 0));
  NS_ENSURE_TRUE_VOID(nfds == 1);
  NS_ENSURE_TRUE_VOID(fds.revents == POLLOUT);

  ssize_t rv = TEMP_FAILURE_RETRY(write(mControlFdW.get(), &control, sizeof(control)));

  if (rv < 0) {
    NS_WARNING("Cannot write wakeup bit to DBus controller!");
  }
}



class DBusPollTask : public nsRunnable
{
public:
  DBusPollTask(DBusThread* aConnection)
  : mConnection(aConnection)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    bool exitThread = false;

    while (!exitThread) {

      int res = TEMP_FAILURE_RETRY(poll(mConnection->mPollData.Elements(),
                                        mConnection->mPollData.Length(),
                                        -1));
      NS_ENSURE_TRUE(res > 0, NS_OK);

      nsTArray<pollfd>::size_type i = 0;

      while (i < mConnection->mPollData.Length()) {
        if (mConnection->mPollData[i].revents == POLLIN) {

          if (mConnection->mPollData[i].fd == mConnection->mControlFdR.get()) {
            char data;
            res = TEMP_FAILURE_RETRY(read(mConnection->mControlFdR.get(), &data, sizeof(data)));
            NS_ENSURE_TRUE(res > 0, NS_OK);

            switch (data) {
            case DBUS_EVENT_LOOP_EXIT:
              exitThread = true;
              break;
            case DBUS_EVENT_LOOP_ADD:
              HandleWatchAdd(mConnection);
              break;
            case DBUS_EVENT_LOOP_REMOVE:
              HandleWatchRemove(mConnection);
              
              continue;
            case DBUS_EVENT_LOOP_WAKEUP:
              NS_ProcessPendingEvents(NS_GetCurrentThread(),
                                      PR_INTERVAL_NO_TIMEOUT);
              break;
            default:
#if DEBUG
              nsCString warning("unknown command ");
              warning.AppendInt(data);
              NS_WARNING(warning.get());
#endif
              break;
            }
          } else {
            short events = mConnection->mPollData[i].revents;
            unsigned int flags = UnixEventsToDBusFlags(events);
            dbus_watch_handle(mConnection->mWatchData[i], flags);
            mConnection->mPollData[i].revents = 0;
            
            
            break;
          }
          while (dbus_connection_dispatch(mConnection->GetConnection()) ==
                 DBUS_DISPATCH_DATA_REMAINS)
          {}
        }
        ++i;
      }
    }

    mConnection->CleanUp();

    return NS_OK;
  }

private:
  nsRefPtr<DBusThread> mConnection;
};

static StaticRefPtr<DBusThread> gDBusThread;
static StaticRefPtr<nsIThread>  gDBusServiceThread;



bool
StartDBus()
{
  MOZ_ASSERT(!NS_IsMainThread());
  NS_ENSURE_TRUE(!gDBusThread, true);

  nsRefPtr<DBusThread> dbusThread(new DBusThread());

  bool eventLoopStarted = dbusThread->Initialize();
  NS_ENSURE_TRUE(eventLoopStarted, false);

  nsresult rv;

  if (!gDBusServiceThread) {
    nsIThread* dbusServiceThread;
    rv = NS_NewNamedThread("DBus Thread", &dbusServiceThread);
    NS_ENSURE_SUCCESS(rv, false);
    gDBusServiceThread = dbusServiceThread;
  }

#ifdef DEBUG
  LOG("DBus Thread Starting\n");
#endif

  nsRefPtr<nsIRunnable> pollTask(new DBusPollTask(dbusThread));
  NS_ENSURE_TRUE(pollTask, false);

  rv = gDBusServiceThread->Dispatch(pollTask, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, false);

  gDBusThread = dbusThread;

  return true;
}

bool
StopDBus()
{
  MOZ_ASSERT(!NS_IsMainThread());
  NS_ENSURE_TRUE(gDBusServiceThread, true);

  nsRefPtr<DBusThread> dbusThread(gDBusThread);
  gDBusThread = nullptr;

  if (dbusThread) {
    static const char data = DBUS_EVENT_LOOP_EXIT;
    ssize_t wret = TEMP_FAILURE_RETRY(write(dbusThread->mControlFdW.get(),
                                            &data, sizeof(data)));
    NS_ENSURE_TRUE(wret == 1, false);
  }

  nsRefPtr<nsIThread> dbusServiceThread(gDBusServiceThread);
  gDBusServiceThread = nullptr;

  nsRefPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(dbusServiceThread, &nsIThread::Shutdown);
  nsresult rv = NS_DispatchToMainThread(runnable);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

nsresult
DispatchToDBusThread(nsIRunnable* event)
{
  nsRefPtr<nsIThread> dbusServiceThread(gDBusServiceThread);
  nsRefPtr<DBusThread> dbusThread(gDBusThread);

  NS_ENSURE_TRUE(dbusServiceThread.get() && dbusThread.get(),
                 NS_ERROR_NOT_INITIALIZED);

  nsresult rv = dbusServiceThread->Dispatch(event, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  dbusThread->WakeUp();

  return NS_OK;
}

}
}
