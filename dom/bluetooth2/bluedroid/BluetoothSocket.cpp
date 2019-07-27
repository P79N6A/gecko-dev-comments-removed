





#include "BluetoothSocket.h"

#include <fcntl.h>
#include <sys/socket.h>

#include "base/message_loop.h"
#include "BluetoothInterface.h"
#include "BluetoothSocketObserver.h"
#include "BluetoothUtils.h"
#include "mozilla/FileUtils.h"
#include "mozilla/RefPtr.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#define FIRST_SOCKET_INFO_MSG_LENGTH 4
#define TOTAL_SOCKET_INFO_LENGTH 20

using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

static const size_t MAX_READ_SIZE = 1 << 16;
static const uint8_t UUID_OBEX_OBJECT_PUSH[] = {
  0x00, 0x00, 0x11, 0x05, 0x00, 0x00, 0x10, 0x00,
  0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB
};
static BluetoothSocketInterface* sBluetoothSocketInterface;


static bool
EnsureBluetoothSocketHalLoad()
{
  if (sBluetoothSocketInterface) {
    return true;
  }

  BluetoothInterface* btInf = BluetoothInterface::GetInstance();
  NS_ENSURE_TRUE(btInf, false);

  sBluetoothSocketInterface = btInf->GetBluetoothSocketInterface();
  NS_ENSURE_TRUE(sBluetoothSocketInterface, false);

  return true;
}

static int16_t
ReadInt16(const uint8_t* aData, size_t* aOffset)
{
  int16_t value = (aData[*aOffset + 1] << 8) | aData[*aOffset];

  *aOffset += 2;
  return value;
}

static int32_t
ReadInt32(const uint8_t* aData, size_t* aOffset)
{
  int32_t value = (aData[*aOffset + 3] << 24) |
                  (aData[*aOffset + 2] << 16) |
                  (aData[*aOffset + 1] << 8) |
                  aData[*aOffset];
  *aOffset += 4;
  return value;
}

static void
ReadBdAddress(const uint8_t* aData, size_t* aOffset, nsAString& aDeviceAddress)
{
  char bdstr[18];
  sprintf(bdstr, "%02x:%02x:%02x:%02x:%02x:%02x",
          aData[*aOffset], aData[*aOffset + 1], aData[*aOffset + 2],
          aData[*aOffset + 3], aData[*aOffset + 4], aData[*aOffset + 5]);

  aDeviceAddress.AssignLiteral(bdstr);
  *aOffset += 6;
}

class mozilla::dom::bluetooth::DroidSocketImpl : public ipc::UnixFdWatcher
{
public:
  enum ConnectionStatus {
    SOCKET_IS_DISCONNECTED = 0,
    SOCKET_IS_LISTENING,
    SOCKET_IS_CONNECTING,
    SOCKET_IS_CONNECTED
  };

  DroidSocketImpl(MessageLoop* aIOLoop, BluetoothSocket* aConsumer, int aFd)
    : ipc::UnixFdWatcher(aIOLoop, aFd)
    , mConsumer(aConsumer)
    , mReadMsgForClientFd(false)
    , mShuttingDownOnIOThread(false)
    , mChannel(0)
    , mAuth(false)
    , mEncrypt(false)
    , mConnectionStatus(SOCKET_IS_DISCONNECTED)
  { }

  DroidSocketImpl(MessageLoop* aIOLoop, BluetoothSocket* aConsumer,
                  int aChannel, bool aAuth, bool aEncrypt)
    : ipc::UnixFdWatcher(aIOLoop)
    , mConsumer(aConsumer)
    , mReadMsgForClientFd(false)
    , mShuttingDownOnIOThread(false)
    , mChannel(aChannel)
    , mAuth(aAuth)
    , mEncrypt(aEncrypt)
    , mConnectionStatus(SOCKET_IS_DISCONNECTED)
  { }

  DroidSocketImpl(MessageLoop* aIOLoop, BluetoothSocket* aConsumer,
                  const nsAString& aDeviceAddress,
                  int aChannel, bool aAuth, bool aEncrypt)
    : ipc::UnixFdWatcher(aIOLoop)
    , mConsumer(aConsumer)
    , mReadMsgForClientFd(false)
    , mShuttingDownOnIOThread(false)
    , mDeviceAddress(aDeviceAddress)
    , mChannel(aChannel)
    , mAuth(aAuth)
    , mEncrypt(aEncrypt)
    , mConnectionStatus(SOCKET_IS_DISCONNECTED)
  {
    MOZ_ASSERT(!mDeviceAddress.IsEmpty());
  }

  ~DroidSocketImpl()
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  void QueueWriteData(UnixSocketRawData* aData)
  {
    mOutgoingQ.AppendElement(aData);
    OnFileCanWriteWithoutBlocking(GetFd());
  }

  bool IsShutdownOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mConsumer == nullptr;
  }

  bool IsShutdownOnIOThread()
  {
    return mShuttingDownOnIOThread;
  }

  void ShutdownOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!IsShutdownOnMainThread());
    mConsumer = nullptr;
  }

  void ShutdownOnIOThread()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!mShuttingDownOnIOThread);

    RemoveWatchers(READ_WATCHER | WRITE_WATCHER);

    mShuttingDownOnIOThread = true;
  }

  void Connect(int aFd);
  void Listen(int aFd);

  void ConnectClientFd()
  {
    
    RemoveWatchers(READ_WATCHER);

    mConnectionStatus = SOCKET_IS_CONNECTED;

    
    AddWatchers(READ_WATCHER, true);
    AddWatchers(WRITE_WATCHER, false);
  }

  




  RefPtr<BluetoothSocket> mConsumer;

  


  bool mReadMsgForClientFd;

private:
  





  virtual void OnFileCanReadWithoutBlocking(int aFd);

  





  virtual void OnFileCanWriteWithoutBlocking(int aFd);

  void OnSocketCanReceiveWithoutBlocking(int aFd);
  void OnSocketCanAcceptWithoutBlocking(int aFd);
  void OnSocketCanSendWithoutBlocking(int aFd);
  void OnSocketCanConnectWithoutBlocking(int aFd);

  






  ssize_t ReadMsg(int aFd, void *aBuffer, size_t aLength);

  


  typedef nsTArray<UnixSocketRawData* > UnixSocketRawDataQueue;
  UnixSocketRawDataQueue mOutgoingQ;

  


  bool mShuttingDownOnIOThread;

  nsString mDeviceAddress;
  int mChannel;
  bool mAuth;
  bool mEncrypt;
  ConnectionStatus mConnectionStatus;
};

template<class T>
class DeleteInstanceRunnable : public nsRunnable
{
public:
  DeleteInstanceRunnable(T* aInstance)
  : mInstance(aInstance)
  { }

  NS_IMETHOD Run()
  {
    delete mInstance;

    return NS_OK;
  }

private:
  T* mInstance;
};

class RequestClosingSocketTask : public nsRunnable
{
public:
  RequestClosingSocketTask(DroidSocketImpl* aImpl) : mImpl(aImpl)
  {
    MOZ_ASSERT(aImpl);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mImpl->IsShutdownOnMainThread()) {
      NS_WARNING("CloseSocket has already been called!");
      
      
      return NS_OK;
    }

    
    
    mImpl->mConsumer->CloseDroidSocket();
    return NS_OK;
  }
private:
  DroidSocketImpl* mImpl;
};

class ShutdownSocketTask : public Task {
  virtual void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    
    
    
    
    
    mImpl->ShutdownOnIOThread();

    nsRefPtr<nsIRunnable> t(new DeleteInstanceRunnable<
                                  mozilla::dom::bluetooth::DroidSocketImpl>(mImpl));
    nsresult rv = NS_DispatchToMainThread(t);
    NS_ENSURE_SUCCESS_VOID(rv);
  }

  DroidSocketImpl* mImpl;

public:
  ShutdownSocketTask(DroidSocketImpl* aImpl) : mImpl(aImpl) { }
};

class SocketReceiveTask : public nsRunnable
{
public:
  SocketReceiveTask(DroidSocketImpl* aImpl, UnixSocketRawData* aData) :
    mImpl(aImpl),
    mRawData(aData)
  {
    MOZ_ASSERT(aImpl);
    MOZ_ASSERT(aData);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (mImpl->IsShutdownOnMainThread()) {
      NS_WARNING("mConsumer is null, aborting receive!");
      
      
      return NS_OK;
    }

    MOZ_ASSERT(mImpl->mConsumer);
    mImpl->mConsumer->ReceiveSocketData(mRawData);
    return NS_OK;
  }
private:
  DroidSocketImpl* mImpl;
  nsAutoPtr<UnixSocketRawData> mRawData;
};

class SocketSendTask : public Task
{
public:
  SocketSendTask(BluetoothSocket* aConsumer, DroidSocketImpl* aImpl,
                 UnixSocketRawData* aData)
    : mConsumer(aConsumer),
      mImpl(aImpl),
      mData(aData)
  {
    MOZ_ASSERT(aConsumer);
    MOZ_ASSERT(aImpl);
    MOZ_ASSERT(aData);
  }

  void
  Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!mImpl->IsShutdownOnIOThread());

    mImpl->QueueWriteData(mData);
  }

private:
  nsRefPtr<BluetoothSocket> mConsumer;
  DroidSocketImpl* mImpl;
  UnixSocketRawData* mData;
};

class DroidSocketImplTask : public CancelableTask
{
public:
  DroidSocketImpl* GetDroidSocketImpl() const
  {
    return mDroidSocketImpl;
  }
  void Cancel() MOZ_OVERRIDE
  {
    mDroidSocketImpl = nullptr;
  }
  bool IsCanceled() const
  {
    return !mDroidSocketImpl;
  }
protected:
  DroidSocketImplTask(DroidSocketImpl* aDroidSocketImpl)
  : mDroidSocketImpl(aDroidSocketImpl)
  {
    MOZ_ASSERT(mDroidSocketImpl);
  }
private:
  DroidSocketImpl* mDroidSocketImpl;
};

class SocketConnectTask : public DroidSocketImplTask
{
public:
  SocketConnectTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : DroidSocketImplTask(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!IsCanceled());
    GetDroidSocketImpl()->Connect(mFd);
  }

private:
  int mFd;
};

class SocketListenTask : public DroidSocketImplTask
{
public:
  SocketListenTask(DroidSocketImpl* aDroidSocketImpl, int aFd)
  : DroidSocketImplTask(aDroidSocketImpl)
  , mFd(aFd)
  { }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread());
    if (!IsCanceled()) {
      GetDroidSocketImpl()->Listen(mFd);
    }
  }

private:
  int mFd;
};

class SocketConnectClientFdTask : public Task
{
  virtual void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    mImpl->ConnectClientFd();
  }

  DroidSocketImpl* mImpl;
public:
  SocketConnectClientFdTask(DroidSocketImpl* aImpl) : mImpl(aImpl) { }
};

void
DroidSocketImpl::Connect(int aFd)
{
  MOZ_ASSERT(aFd >= 0);

  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  NS_ENSURE_TRUE_VOID(flags >= 0);

  if (!(flags & O_NONBLOCK)) {
    int res = TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFL, flags | O_NONBLOCK));
    NS_ENSURE_TRUE_VOID(!res);
  }

  SetFd(aFd);
  mConnectionStatus = SOCKET_IS_CONNECTING;

  AddWatchers(READ_WATCHER, true);
}

void
DroidSocketImpl::Listen(int aFd)
{
  MOZ_ASSERT(aFd >= 0);

  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  NS_ENSURE_TRUE_VOID(flags >= 0);

  if (!(flags & O_NONBLOCK)) {
    int res = TEMP_FAILURE_RETRY(fcntl(aFd, F_SETFL, flags | O_NONBLOCK));
    NS_ENSURE_TRUE_VOID(!res);
  }

  SetFd(aFd);
  mConnectionStatus = SOCKET_IS_LISTENING;

  AddWatchers(READ_WATCHER, true);
}

ssize_t
DroidSocketImpl::ReadMsg(int aFd, void *aBuffer, size_t aLength)
{
  ssize_t ret;
  struct msghdr msg;
  struct iovec iv;
  struct cmsghdr cmsgbuf[2 * sizeof(cmsghdr) + 0x100];

  memset(&msg, 0, sizeof(msg));
  memset(&iv, 0, sizeof(iv));

  iv.iov_base = (unsigned char *)aBuffer;
  iv.iov_len = aLength;

  msg.msg_iov = &iv;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  ret = recvmsg(GetFd(), &msg, MSG_NOSIGNAL);
  if (ret < 0 && errno == EPIPE) {
    
    return 0;
  }

  NS_ENSURE_FALSE(ret < 0, -1);
  NS_ENSURE_FALSE(msg.msg_flags & (MSG_CTRUNC | MSG_OOB | MSG_ERRQUEUE), -1);

  
  for (struct cmsghdr *cmsgptr = CMSG_FIRSTHDR(&msg);
       cmsgptr != nullptr; cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {
    if (cmsgptr->cmsg_level != SOL_SOCKET) {
      continue;
    }
    if (cmsgptr->cmsg_type == SCM_RIGHTS) {
      int *pDescriptors = (int *)CMSG_DATA(cmsgptr);

      
      int fd = pDescriptors[0];
      int flags = TEMP_FAILURE_RETRY(fcntl(fd, F_GETFL));
      NS_ENSURE_TRUE(flags >= 0, 0);
      if (!(flags & O_NONBLOCK)) {
        int res = TEMP_FAILURE_RETRY(fcntl(fd, F_SETFL, flags | O_NONBLOCK));
        NS_ENSURE_TRUE(!res, 0);
      }
      Close();
      SetFd(fd);
      break;
    }
  }

  return ret;
}

void
DroidSocketImpl::OnFileCanReadWithoutBlocking(int aFd)
{
  if (mConnectionStatus == SOCKET_IS_CONNECTED) {
    OnSocketCanReceiveWithoutBlocking(aFd);
  } else if (mConnectionStatus == SOCKET_IS_LISTENING) {
    OnSocketCanAcceptWithoutBlocking(aFd);
  } else if (mConnectionStatus == SOCKET_IS_CONNECTING) {
    
    OnSocketCanConnectWithoutBlocking(aFd);
  } else {
    NS_NOTREACHED("invalid connection state for reading");
  }
}

void
DroidSocketImpl::OnSocketCanReceiveWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  
  while (true) {
    nsAutoPtr<UnixSocketRawData> incoming(new UnixSocketRawData(MAX_READ_SIZE));

    ssize_t ret = read(aFd, incoming->mData, incoming->mSize);

    if (ret <= 0) {
      if (ret == -1) {
        if (errno == EINTR) {
          continue; 
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return; 
        }

        BT_WARNING("Cannot read from network");
        
      }

      
      
      RemoveWatchers(READ_WATCHER | WRITE_WATCHER);
      nsRefPtr<RequestClosingSocketTask> t = new RequestClosingSocketTask(this);
      NS_DispatchToMainThread(t);
      return;
    }

    incoming->mSize = ret;
    nsRefPtr<SocketReceiveTask> t =
      new SocketReceiveTask(this, incoming.forget());
    NS_DispatchToMainThread(t);

    
    
    if (ret < ssize_t(MAX_READ_SIZE)) {
      return;
    }
  }

  MOZ_CRASH("We returned early");
}

void
DroidSocketImpl::OnSocketCanAcceptWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  
  while (true) {
    nsAutoPtr<UnixSocketRawData> incoming(new UnixSocketRawData(MAX_READ_SIZE));

    ssize_t ret;
    if (!mReadMsgForClientFd) {
      ret = read(aFd, incoming->mData, incoming->mSize);
    } else {
      ret = ReadMsg(aFd, incoming->mData, incoming->mSize);
    }

    if (ret <= 0) {
      if (ret == -1) {
        if (errno == EINTR) {
          continue; 
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return; 
        }

        BT_WARNING("Cannot read from network");
        
      }

      
      
      RemoveWatchers(READ_WATCHER | WRITE_WATCHER);
      nsRefPtr<RequestClosingSocketTask> t = new RequestClosingSocketTask(this);
      NS_DispatchToMainThread(t);
      return;
    }

    incoming->mSize = ret;
    nsRefPtr<SocketReceiveTask> t =
      new SocketReceiveTask(this, incoming.forget());
    NS_DispatchToMainThread(t);

    
    
    if (ret < ssize_t(MAX_READ_SIZE)) {
      return;
    }
  }

  MOZ_CRASH("We returned early");
}

void
DroidSocketImpl::OnFileCanWriteWithoutBlocking(int aFd)
{
  if (mConnectionStatus == SOCKET_IS_CONNECTED) {
    OnSocketCanSendWithoutBlocking(aFd);
  } else if (mConnectionStatus == SOCKET_IS_CONNECTING) {
    OnSocketCanConnectWithoutBlocking(aFd);
  } else {
    NS_NOTREACHED("invalid connection state for writing");
  }
}

void
DroidSocketImpl::OnSocketCanSendWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);
  MOZ_ASSERT(aFd >= 0);

  
  
  
  
  
  
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
      AddWatchers(WRITE_WATCHER, false);
    }
    mOutgoingQ.RemoveElementAt(0);
    delete data;
  }
}

void
DroidSocketImpl::OnSocketCanConnectWithoutBlocking(int aFd)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDownOnIOThread);

  
  while (true) {
    nsAutoPtr<UnixSocketRawData> incoming(new UnixSocketRawData(MAX_READ_SIZE));

    ssize_t ret;
    if (!mReadMsgForClientFd) {
      ret = read(aFd, incoming->mData, incoming->mSize);
    } else {
      ret = ReadMsg(aFd, incoming->mData, incoming->mSize);
    }

    if (ret <= 0) {
      if (ret == -1) {
        if (errno == EINTR) {
          continue; 
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return; 
        }

        BT_WARNING("Cannot read from network");
        
      }

      
      
      RemoveWatchers(READ_WATCHER | WRITE_WATCHER);
      nsRefPtr<RequestClosingSocketTask> t = new RequestClosingSocketTask(this);
      NS_DispatchToMainThread(t);
      return;
    }

    incoming->mSize = ret;
    nsRefPtr<SocketReceiveTask> t =
      new SocketReceiveTask(this, incoming.forget());
    NS_DispatchToMainThread(t);

    
    
    if (ret < ssize_t(MAX_READ_SIZE)) {
      return;
    }
  }

  MOZ_CRASH("We returned early");
}

BluetoothSocket::BluetoothSocket(BluetoothSocketObserver* aObserver,
                                 BluetoothSocketType aType,
                                 bool aAuth,
                                 bool aEncrypt)
  : mObserver(aObserver)
  , mImpl(nullptr)
  , mAuth(aAuth)
  , mEncrypt(aEncrypt)
  , mReceivedSocketInfoLength(0)
{
  MOZ_ASSERT(aObserver);

  EnsureBluetoothSocketHalLoad();
  mDeviceAddress.AssignLiteral(BLUETOOTH_ADDRESS_NONE);
}

void
BluetoothSocket::CloseDroidSocket()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mImpl) {
    return;
  }

  
  
  
  mImpl->ShutdownOnMainThread();
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new ShutdownSocketTask(mImpl));
  mImpl = nullptr;

  NotifyDisconnect();
}

class ConnectResultHandler MOZ_FINAL : public BluetoothSocketResultHandler
{
public:
  ConnectResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Connect(int aFd) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new SocketConnectTask(mImpl, aFd));
  }

  void OnError(bt_status_t aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    BT_WARNING("Connect failed: %d", (int)aStatus);
  }

private:
  DroidSocketImpl* mImpl;
};

bool
BluetoothSocket::Connect(const nsAString& aDeviceAddress, int aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_FALSE(mImpl, false);

  mIsServer = false;
  mImpl = new DroidSocketImpl(XRE_GetIOMessageLoop(), this, aDeviceAddress,
                              aChannel, mAuth, mEncrypt);

  bt_bdaddr_t remoteBdAddress;
  StringToBdAddressType(aDeviceAddress, &remoteBdAddress);

  
  sBluetoothSocketInterface->Connect(&remoteBdAddress,
                                     BTSOCK_RFCOMM,
                                     UUID_OBEX_OBJECT_PUSH,
                                     aChannel,
                                     (BTSOCK_FLAG_ENCRYPT * mEncrypt) |
                                     (BTSOCK_FLAG_AUTH * mAuth),
                                     new ConnectResultHandler(mImpl));
  return true;
}

class ListenResultHandler MOZ_FINAL : public BluetoothSocketResultHandler
{
public:
  ListenResultHandler(DroidSocketImpl* aImpl)
  : mImpl(aImpl)
  {
    MOZ_ASSERT(mImpl);
  }

  void Listen(int aFd) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new SocketListenTask(mImpl, aFd));
  }

  void OnError(bt_status_t aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    BT_WARNING("Listen failed: %d", (int)aStatus);
  }

private:
  DroidSocketImpl* mImpl;
};

bool
BluetoothSocket::Listen(int aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_FALSE(mImpl, false);

  mIsServer = true;
  mImpl = new DroidSocketImpl(XRE_GetIOMessageLoop(), this, aChannel, mAuth,
                              mEncrypt);

  sBluetoothSocketInterface->Listen(BTSOCK_RFCOMM,
                                    "OBEX Object Push",
                                    UUID_OBEX_OBJECT_PUSH,
                                    aChannel,
                                    (BTSOCK_FLAG_ENCRYPT * mEncrypt) |
                                    (BTSOCK_FLAG_AUTH * mAuth),
                                    new ListenResultHandler(mImpl));
  return true;
}

bool
BluetoothSocket::SendDroidSocketData(UnixSocketRawData* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE(mImpl, false);

  MOZ_ASSERT(!mImpl->IsShutdownOnMainThread());
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new SocketSendTask(this, mImpl, aData));
  return true;
}

bool
BluetoothSocket::ReceiveSocketInfo(nsAutoPtr<UnixSocketRawData>& aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());

  




  if (mReceivedSocketInfoLength >= TOTAL_SOCKET_INFO_LENGTH) {
    
    return false;
  }
  mReceivedSocketInfoLength += aMessage->mSize;

  size_t offset = 0;
  if (mReceivedSocketInfoLength == FIRST_SOCKET_INFO_MSG_LENGTH) {
    
    int32_t channel = ReadInt32(aMessage->mData, &offset);
    BT_LOGR("channel %d", channel);

    
    mImpl->mReadMsgForClientFd = mIsServer;
  } else if (mReceivedSocketInfoLength == TOTAL_SOCKET_INFO_LENGTH) {
    
    int16_t size = ReadInt16(aMessage->mData, &offset);
    ReadBdAddress(aMessage->mData, &offset, mDeviceAddress);
    int32_t channel = ReadInt32(aMessage->mData, &offset);
    int32_t connectionStatus = ReadInt32(aMessage->mData, &offset);

    BT_LOGR("size %d channel %d remote addr %s status %d",
      size, channel, NS_ConvertUTF16toUTF8(mDeviceAddress).get(), connectionStatus);

    if (connectionStatus != 0) {
      NotifyError();
      return true;
    }

    mImpl->mReadMsgForClientFd = false;
    
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new SocketConnectClientFdTask(mImpl));
    NotifySuccess();
  }

  return true;
}

void
BluetoothSocket::ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage)
{
  if (ReceiveSocketInfo(aMessage)) {
    return;
  }

  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->ReceiveSocketData(this, aMessage);
}

void
BluetoothSocket::OnConnectSuccess()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectSuccess(this);
}

void
BluetoothSocket::OnConnectError()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectError(this);
}

void
BluetoothSocket::OnDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketDisconnect(this);
}

