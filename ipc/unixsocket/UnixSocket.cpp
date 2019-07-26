





#include "UnixSocket.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/socket.h>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"

#include "mozilla/Monitor.h"
#include "mozilla/Util.h"
#include "mozilla/FileUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace ipc {

class UnixSocketImpl : public MessageLoopForIO::Watcher
{
public:
  UnixSocketImpl(UnixSocketConsumer* aConsumer, int aFd)
    : mConsumer(aConsumer)
    , mIOLoop(nullptr)
    , mFd(aFd)
  {
  }

  ~UnixSocketImpl()
  {
    mReadWatcher.StopWatchingFileDescriptor();
    mWriteWatcher.StopWatchingFileDescriptor();
  }

  void QueueWriteData(UnixSocketRawData* aData)
  {
    mOutgoingQ.AppendElement(aData);
    OnFileCanWriteWithoutBlocking(mFd);
  }

  bool isFdValid()
  {
    return mFd > 0;
  }

  void SetUpIO()
  {
    MOZ_ASSERT(!mIOLoop);
    mIOLoop = MessageLoopForIO::current();
    mIOLoop->WatchFileDescriptor(mFd,
                                 true,
                                 MessageLoopForIO::WATCH_READ,
                                 &mReadWatcher,
                                 this);
  }

  void PrepareRemoval()
  {
    mConsumer.forget();
  }

  




  RefPtr<UnixSocketConsumer> mConsumer;

private:
  





  virtual void OnFileCanReadWithoutBlocking(int aFd);

  





  virtual void OnFileCanWriteWithoutBlocking(int aFd);

  


  MessageLoopForIO* mIOLoop;

  


  typedef nsTArray<UnixSocketRawData* > UnixSocketRawDataQueue;
  UnixSocketRawDataQueue mOutgoingQ;

  



  ScopedClose mFd;

  


  nsAutoPtr<UnixSocketRawData> mIncoming;

  


  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;

  


  MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;
};

static void
DestroyImpl(UnixSocketImpl* impl)
{
  delete impl;
}

class SocketReceiveTask : public nsRunnable
{
public:
  SocketReceiveTask(UnixSocketImpl* aImpl, UnixSocketRawData* aData) :
    mImpl(aImpl),
    mRawData(aData)
  {
    MOZ_ASSERT(aImpl);
    MOZ_ASSERT(aData);
  }

  NS_IMETHOD Run()
  {
    if(!mImpl->mConsumer) {
      NS_WARNING("mConsumer is null, aborting receive!");
      
      
      return NS_OK;
    }
    mImpl->mConsumer->ReceiveSocketData(mRawData);
    return NS_OK;
  }
private:
  UnixSocketImpl* mImpl;
  nsAutoPtr<UnixSocketRawData> mRawData;
};

class SocketSendTask : public Task
{
public:
  SocketSendTask(UnixSocketConsumer* aConsumer, UnixSocketImpl* aImpl,
                 UnixSocketRawData* aData)
    : mConsumer(aConsumer),
      mImpl(aImpl),
      mData(aData)
  {
    MOZ_ASSERT(aConsumer);
    MOZ_ASSERT(aImpl);
    MOZ_ASSERT(aData);
  }

  void Run()
  {
    mImpl->QueueWriteData(mData);
  }

private:
  nsRefPtr<UnixSocketConsumer> mConsumer;
  UnixSocketImpl* mImpl;
  UnixSocketRawData* mData;
};

class StartImplReadingTask : public Task
{
public:
  StartImplReadingTask(UnixSocketImpl* aImpl)
    : mImpl(aImpl)
  {
  }

  void Run()
  {
    mImpl->SetUpIO();
  }
private:
  UnixSocketImpl* mImpl;
};

bool
UnixSocketConnector::Connect(int aFd, const char* aAddress)
{
  if (!ConnectInternal(aFd, aAddress))
  {
    return false;
  }

  
  int flags = fcntl(aFd, F_GETFD);
  if (-1 == flags) {
    return false;
  }

  flags |= FD_CLOEXEC;
  if (-1 == fcntl(aFd, F_SETFD, flags)) {
    return false;
  }

  
  if (-1 == fcntl(aFd, F_SETFL, O_NONBLOCK)) {
    return false;
  }

  return true;
}

UnixSocketConsumer::~UnixSocketConsumer()
{
}

bool
UnixSocketConsumer::SendSocketData(UnixSocketRawData* aData)
{
  if (!mImpl) {
    return false;
  }
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketSendTask(this, mImpl, aData));
  return true;
}

bool
UnixSocketConsumer::SendSocketData(const nsACString& aStr)
{
  if (!mImpl) {
    return false;
  }
  if (aStr.Length() > UnixSocketRawData::MAX_DATA_SIZE) {
    return false;
  }
  nsCString str(aStr);
  UnixSocketRawData* d = new UnixSocketRawData(aStr.Length());
  memcpy(d->mData, str.get(), aStr.Length());
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketSendTask(this, mImpl, d));
  return true;
}

void
UnixSocketConsumer::CloseSocket()
{
  if (!mImpl) {
    return;
  }
  
  mImpl->PrepareRemoval();
  
  
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   NewRunnableFunction(DestroyImpl,
                                                       mImpl.forget()));
}

void
UnixSocketImpl::OnFileCanReadWithoutBlocking(int aFd)
{
  
  
  
  
  
  
  
  
  while (true) {
    if (!mIncoming) {
      mIncoming = new UnixSocketRawData();
      ssize_t ret = read(aFd, mIncoming->mData, UnixSocketRawData::MAX_DATA_SIZE);
      if (ret <= 0) {
        if (ret == -1) {
          if (errno == EINTR) {
            continue; 
          }
          else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            mIncoming.forget();
            return; 
          }
          
        }
#ifdef DEBUG
        nsAutoString str;
        str.AssignLiteral("Cannot read from network, error ");
        str += (int)ret;
        NS_WARNING(NS_ConvertUTF16toUTF8(str).get());
#endif
        
        
        mIncoming.forget();
        mReadWatcher.StopWatchingFileDescriptor();
        mWriteWatcher.StopWatchingFileDescriptor();
        mConsumer->CloseSocket();
        return;
      }
      mIncoming->mData[ret] = 0;
      mIncoming->mSize = ret;
      nsRefPtr<SocketReceiveTask> t =
        new SocketReceiveTask(this, mIncoming.forget());
      NS_DispatchToMainThread(t);
      if (ret < ssize_t(UnixSocketRawData::MAX_DATA_SIZE)) {
        return;
      }
    }
  }
}

void
UnixSocketImpl::OnFileCanWriteWithoutBlocking(int aFd)
{
  
  
  
  
  
  
  while (true) {
    UnixSocketRawData* data;
    if (mOutgoingQ.IsEmpty()) {
      return;
    }
    data = mOutgoingQ.ElementAt(0);
    const uint8_t *toWrite;
    toWrite = data->mData;

    while (data->mCurrentWriteOffset < data->mSize) {
      ssize_t write_amount = data->mSize - data->mCurrentWriteOffset;
      ssize_t written;
      written = write (aFd, toWrite + data->mCurrentWriteOffset,
                       write_amount);
      if (written > 0) {
        data->mCurrentWriteOffset += written;
      }
      if (written != write_amount) {
        break;
      }
    }

    if (data->mCurrentWriteOffset != data->mSize) {
      MessageLoopForIO::current()->WatchFileDescriptor(
        aFd,
        false,
        MessageLoopForIO::WATCH_WRITE,
        &mWriteWatcher,
        this);
      return;
    }
    mOutgoingQ.RemoveElementAt(0);
    delete data;
  }
}

bool
UnixSocketConsumer::ConnectSocket(UnixSocketConnector& aConnector,
                                  const char* aAddress)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mImpl);
  ScopedClose fd(aConnector.Create());
  if (fd < 0) {
    return false;
  }
  if (!aConnector.Connect(fd, aAddress)) {
    return false;
  }
  mImpl = new UnixSocketImpl(this, fd.forget());
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new StartImplReadingTask(mImpl));
  return true;
}

} 
} 
