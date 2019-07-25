






















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
#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"
#include "mozilla/Util.h"
#include "mozilla/FileUtils.h"
#include "nsAutoPtr.h"
#include "nsIThread.h"
#include "nsXULAppAPI.h"

#undef LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args);
#else
#define LOG(args...)  printf(args);
#endif

#define DEFAULT_INITIAL_POLLFD_COUNT 8






enum {
  DBUS_EVENT_LOOP_EXIT = 1,
  DBUS_EVENT_LOOP_ADD = 2,
  DBUS_EVENT_LOOP_REMOVE = 3,
} DBusEventTypes;




static const char* DBUS_SIGNALS[] =
{
  "type='signal',interface='org.freedesktop.DBus'",
  "type='signal',interface='org.bluez.Adapter'",
  "type='signal',interface='org.bluez.Device'",
  "type='signal',interface='org.bluez.Input'",
  "type='signal',interface='org.bluez.Network'",
  "type='signal',interface='org.bluez.NetworkServer'",
  "type='signal',interface='org.bluez.HealthDevice'",
  "type='signal',interface='org.bluez.AudioSink'"
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

  bool StartEventLoop();
  void StopEventLoop();
  bool IsEventLoopRunning();
  static void* EventLoop(void* aPtr);
  
  
  pthread_t mThread;
  Mutex mMutex;
  bool mIsRunning;

  
  
  
  nsTArray<pollfd> mPollData;
  nsTArray<DBusWatch*> mWatchData;

  
  
  ScopedClose mControlFdR;
  ScopedClose mControlFdW;

protected:  
  bool SetUpEventLoop();
  bool TearDownData();
  bool TearDownEventLoop();
};

static nsAutoPtr<DBusThread> sDBusThread;




static dbus_bool_t
AddWatch(DBusWatch *aWatch, void *aData)
{
  DBusThread *dbt = (DBusThread *)aData;

  if (dbus_watch_get_enabled(aWatch)) {
    
    
    
    
    char control = DBUS_EVENT_LOOP_ADD;
    write(dbt->mControlFdW.mFd, &control, sizeof(char));

    
    int fd = dbus_watch_get_fd(aWatch);
    write(dbt->mControlFdW.mFd, &fd, sizeof(int));

    unsigned int flags = dbus_watch_get_flags(aWatch);
    write(dbt->mControlFdW.mFd, &flags, sizeof(unsigned int));

    write(dbt->mControlFdW.mFd, &aWatch, sizeof(DBusWatch*));
  }
  return true;
}

static void
RemoveWatch(DBusWatch *aWatch, void *aData)
{
  DBusThread *dbt = (DBusThread *)aData;

  char control = DBUS_EVENT_LOOP_REMOVE;
  write(dbt->mControlFdW.mFd, &control, sizeof(char));

  
  int fd = dbus_watch_get_fd(aWatch);
  write(dbt->mControlFdW.mFd, &fd, sizeof(int));

  unsigned int flags = dbus_watch_get_flags(aWatch);
  write(dbt->mControlFdW.mFd, &flags, sizeof(unsigned int));
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
  read(aDbt->mControlFdR.mFd, &newFD, sizeof(int));
  read(aDbt->mControlFdR.mFd, &flags, sizeof(unsigned int));
  read(aDbt->mControlFdR.mFd, &watch, sizeof(DBusWatch *));
  short events = DBusFlagsToUnixEvents(flags);

  pollfd p;
  p.fd = newFD;
  p.revents = 0;
  p.events = events;
  if(aDbt->mPollData.Contains(p, PollFdComparator())) return;
  aDbt->mPollData.AppendElement(p);
  aDbt->mWatchData.AppendElement(watch);
}

static void HandleWatchRemove(DBusThread* aDbt) {
  int removeFD;
  unsigned int flags;

  read(aDbt->mControlFdR.mFd, &removeFD, sizeof(int));
  read(aDbt->mControlFdR.mFd, &flags, sizeof(unsigned int));
  short events = DBusFlagsToUnixEvents(flags);
  pollfd p;
  p.fd = removeFD;
  p.events = events;
  int index = aDbt->mPollData.IndexOf(p, 0, PollFdComparator());
  aDbt->mPollData.RemoveElementAt(index);

  
  
  aDbt->mWatchData.RemoveElementAt(index);
}


static DBusHandlerResult
EventFilter(DBusConnection *aConn, DBusMessage *aMsg,
            void *aData)
{
  DBusError err;

  dbus_error_init(&err);

  if (dbus_message_get_type(aMsg) != DBUS_MESSAGE_TYPE_SIGNAL) {
    LOG("%s: not interested (not a signal).\n", __FUNCTION__);
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  LOG("%s: Received signal %s:%s from %s\n", __FUNCTION__,
      dbus_message_get_interface(aMsg), dbus_message_get_member(aMsg),
      dbus_message_get_path(aMsg));

  return DBUS_HANDLER_RESULT_HANDLED;
}



DBusThread::DBusThread() : mMutex("DBusGonk.mMutex")
                         , mIsRunning(false)
                         , mControlFdR(-1)
                         , mControlFdW(-1)
{
}

DBusThread::~DBusThread()
{

}

bool
DBusThread::SetUpEventLoop()
{
  
  if(mConnection) {
    return false;
  }
  
  if(!Create()) {
    return false;
  }

  dbus_threads_init_default();
  DBusError err;
  dbus_error_init(&err);

  
  if (!dbus_connection_add_filter(mConnection, EventFilter, this, NULL)){
    return false;
  }

  
  
  
  
  for(uint32_t i = 0; i < ArrayLength(DBUS_SIGNALS); ++i) {
    dbus_bus_add_match(mConnection,
                       DBUS_SIGNALS[i],
                       &err);
    if (dbus_error_is_set(&err)) {
      LOG_AND_FREE_DBUS_ERROR(&err);
      return false;
    }
  }
  return true;
}

bool
DBusThread::TearDownData()
{
  if (mControlFdW.mFd) {
    close(mControlFdW.mFd);
    mControlFdW.mFd = 0;
  }
  if (mControlFdR.mFd) {
    close(mControlFdR.mFd);
    mControlFdR.mFd = 0;
  }    
  mPollData.Clear();

  
  
  mWatchData.Clear();
  return true;
}

bool
DBusThread::TearDownEventLoop()
{
  MOZ_ASSERT(mConnection);

  DBusError err;
  dbus_error_init(&err);

  for(uint32_t i = 0; i < ArrayLength(DBUS_SIGNALS); ++i) {
    dbus_bus_remove_match(mConnection,
                          DBUS_SIGNALS[i],
                          &err);
    if (dbus_error_is_set(&err)) {
      LOG_AND_FREE_DBUS_ERROR(&err);
    }
  }

  dbus_connection_remove_filter(mConnection, EventFilter, this);
  return true;
}

void*
DBusThread::EventLoop(void *aPtr)
{
  DBusThread* dbt = static_cast<DBusThread*>(aPtr);
  MOZ_ASSERT(dbt);

  dbus_connection_set_watch_functions(dbt->mConnection, AddWatch,
                                      RemoveWatch, ToggleWatch, aPtr, NULL);

  dbt->mIsRunning = true;

  while (1) {
    poll(dbt->mPollData.Elements(), dbt->mPollData.Length(), -1);
    
    for (uint32_t i = 0; i < dbt->mPollData.Length(); i++) {
      if (!dbt->mPollData[i].revents) {
        continue;
      }

      if (dbt->mPollData[i].fd == dbt->mControlFdR.mFd) {
        char data;
        while (recv(dbt->mControlFdR.mFd, &data, sizeof(char), MSG_DONTWAIT)
               != -1) {
          switch (data) {
          case DBUS_EVENT_LOOP_EXIT:
          {
            dbus_connection_set_watch_functions(dbt->mConnection,
                                                NULL, NULL, NULL, NULL, NULL);
            dbt->TearDownEventLoop();
            return NULL;
          }
          case DBUS_EVENT_LOOP_ADD:
          {
            HandleWatchAdd(dbt);
            break;
          }
          case DBUS_EVENT_LOOP_REMOVE:
          {
            HandleWatchRemove(dbt);
            break;
          }
          }
        }
      } else {
        short events = dbt->mPollData[i].revents;
        unsigned int flags = UnixEventsToDBusFlags(events);
        dbus_watch_handle(dbt->mWatchData[i], flags);
        dbt->mPollData[i].revents = 0;
        
        
        break;
      }
    }
    while (dbus_connection_dispatch(dbt->mConnection) ==
           DBUS_DISPATCH_DATA_REMAINS)
    {}
  }
}

bool
DBusThread::StartEventLoop()
{
  MutexAutoLock lock(mMutex);
  mIsRunning = false;
  if (socketpair(AF_LOCAL, SOCK_STREAM, 0, &(mControlFdR.mFd))) {
    TearDownData();
    return false;
  }
  pollfd p;
  p.fd = mControlFdR.mFd;
  p.events = POLLIN;
  mPollData.AppendElement(p);
  
  
  
  mWatchData.AppendElement((DBusWatch*)NULL);
  if (SetUpEventLoop() != true) {
    TearDownData();
    return false;
  }
  pthread_create(&(mThread), NULL, DBusThread::EventLoop, this);
  return true;
}

void
DBusThread::StopEventLoop()
{
  MutexAutoLock lock(mMutex);
  if (mIsRunning) {
    char data = DBUS_EVENT_LOOP_EXIT;
    write(mControlFdW.mFd, &data, sizeof(char));
    void *ret;
    pthread_join(mThread, &ret);
    TearDownData();
  }
  mIsRunning = false;
}

bool
DBusThread::IsEventLoopRunning()
{
  MutexAutoLock lock(mMutex);
  return mIsRunning;
}



static void
ConnectDBus(Monitor* aMonitor, bool* aSuccess)
{
  MOZ_ASSERT(!sDBusThread);

  sDBusThread = new DBusThread();
  *aSuccess = true;
  if(!sDBusThread->StartEventLoop())
  {
    *aSuccess = false;
  }
  {
    MonitorAutoLock lock(*aMonitor);
    lock.Notify();
  }
}

static void
DisconnectDBus(Monitor* aMonitor, bool* aSuccess)
{
  MOZ_ASSERT(sDBusThread);

  *aSuccess = true;
  sDBusThread->StopEventLoop();
  sDBusThread = NULL;
  {
    MonitorAutoLock lock(*aMonitor);
    lock.Notify();
  }
}

bool
StartDBus()
{
  Monitor monitor("StartDBus.monitor");
  bool success;
  {
    MonitorAutoLock lock(monitor);

    XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(ConnectDBus, &monitor, &success));
    lock.Wait();
  }
  return success;
}

bool
StopDBus()
{
  Monitor monitor("StopDBus.monitor");
  bool success;
  {
    MonitorAutoLock lock(monitor);

    XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(DisconnectDBus, &monitor, &success));
    lock.Wait();
  }
  return success;
}

}
}
