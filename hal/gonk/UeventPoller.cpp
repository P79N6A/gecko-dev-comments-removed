















#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "nsDebug.h"
#include "base/message_loop.h"
#include "mozilla/FileUtils.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#include "UeventPoller.h"

namespace mozilla {
namespace hal_impl {

static void ShutdownUevent();

class NetlinkPoller : public MessageLoopForIO::Watcher
{
public:
  NetlinkPoller() : mSocket(-1),
                    mIOLoop(MessageLoopForIO::current())
  {
  }

  virtual ~NetlinkPoller() {}

  bool OpenSocket();

  virtual void OnFileCanReadWithoutBlocking(int fd);

  
  virtual void OnFileCanWriteWithoutBlocking(int fd)
  {
    MOZ_CRASH("Must not write to netlink socket");
  }

  MessageLoopForIO *GetIOLoop () const { return mIOLoop; }
  void RegisterObserver(IUeventObserver *aObserver)
  {
    mUeventObserverList.AddObserver(aObserver);
  }

  void UnregisterObserver(IUeventObserver *aObserver)
  {
    mUeventObserverList.RemoveObserver(aObserver);
    if (mUeventObserverList.Length() == 0)
      ShutdownUevent();  
  }

private:
  ScopedClose mSocket;
  MessageLoopForIO* mIOLoop;
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;

  const static int kBuffsize = 64 * 1024;
  uint8_t mBuffer [kBuffsize];

  typedef ObserverList<NetlinkEvent> UeventObserverList;
  UeventObserverList mUeventObserverList;
};

bool
NetlinkPoller::OpenSocket()
{
  mSocket.rwget() = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
  if (mSocket.get() < 0)
    return false;

  int sz = kBuffsize;

  if (setsockopt(mSocket.get(), SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0)
    return false;

  
  int flags = fcntl(mSocket.get(), F_GETFD);
  if (flags == -1) {
      return false;
  }
  flags |= FD_CLOEXEC;
  if (fcntl(mSocket.get(), F_SETFD, flags) == -1)
    return false;

  
  if (fcntl(mSocket.get(), F_SETFL, O_NONBLOCK) == -1)
    return false;

  struct sockaddr_nl saddr;
  bzero(&saddr, sizeof(saddr));
  saddr.nl_family = AF_NETLINK;
  saddr.nl_groups = 1;
  saddr.nl_pid = gettid();

  do {
    if (bind(mSocket.get(), (struct sockaddr *)&saddr, sizeof(saddr)) == 0) {
      break;
    }

    if (errno != EADDRINUSE) {
      return false;
    }

    if (saddr.nl_pid == 0) {
      return false;
    }

    
    
    printf_stderr("The netlink socket address saddr.nl_pid=%u is in use. Let the kernel re-assign.\n", saddr.nl_pid);
    saddr.nl_pid = 0;
  } while (true);

  if (!mIOLoop->WatchFileDescriptor(mSocket.get(),
                                    true,
                                    MessageLoopForIO::WATCH_READ,
                                    &mReadWatcher,
                                    this)) {
      return false;
  }

  return true;
}

static nsAutoPtr<NetlinkPoller> sPoller;

class UeventInitTask : public Task
{
  virtual void Run()
  {
    if (!sPoller) {
      return;
    }
    if (sPoller->OpenSocket()) {
      return;
    }
    sPoller->GetIOLoop()->PostDelayedTask(FROM_HERE, new UeventInitTask(), 1000);
  }
};

void
NetlinkPoller::OnFileCanReadWithoutBlocking(int fd)
{
  MOZ_ASSERT(fd == mSocket.get());
  while (true) {
    int ret = read(fd, mBuffer, kBuffsize);
    if (ret == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      if (errno == EINTR) {
        continue;
      }
    }
    if (ret <= 0) {
      
      _exit(1);
    }
    NetlinkEvent netlinkEvent;
    netlinkEvent.decode(reinterpret_cast<char*>(mBuffer), ret);
    mUeventObserverList.Broadcast(netlinkEvent);
  }
}

static void
InitializeUevent()
{
  MOZ_ASSERT(!sPoller);
  sPoller = new NetlinkPoller();
  sPoller->GetIOLoop()->PostTask(FROM_HERE, new UeventInitTask());

}

static void
ShutdownUevent()
{
  sPoller = nullptr;
}

void
RegisterUeventListener(IUeventObserver *aObserver)
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  if (!sPoller)
    InitializeUevent();
  sPoller->RegisterObserver(aObserver);
}

void
UnregisterUeventListener(IUeventObserver *aObserver)
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  sPoller->UnregisterObserver(aObserver);
}

} 
} 

