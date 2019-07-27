





#include "BluetoothSocketHALInterface.h"
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include "BluetoothHALHelpers.h"
#include "nsXULAppAPI.h"

BEGIN_BLUETOOTH_NAMESPACE

typedef
  BluetoothHALInterfaceRunnable1<BluetoothSocketResultHandler, void,
                                 int, int>
  BluetoothSocketHALIntResultRunnable;

typedef
  BluetoothHALInterfaceRunnable3<BluetoothSocketResultHandler, void,
                                 int, const nsString, int,
                                 int, const nsAString_internal&, int>
  BluetoothSocketHALIntStringIntResultRunnable;

typedef
  BluetoothHALInterfaceRunnable1<BluetoothSocketResultHandler, void,
                                 BluetoothStatus, BluetoothStatus>
  BluetoothSocketHALErrorRunnable;

static nsresult
DispatchBluetoothSocketHALResult(
  BluetoothSocketResultHandler* aRes,
  void (BluetoothSocketResultHandler::*aMethod)(int), int aArg,
  BluetoothStatus aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == STATUS_SUCCESS) {
    runnable = new BluetoothSocketHALIntResultRunnable(aRes, aMethod, aArg);
  } else {
    runnable = new BluetoothSocketHALErrorRunnable(aRes,
      &BluetoothSocketResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_WARNING("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}

static nsresult
DispatchBluetoothSocketHALResult(
  BluetoothSocketResultHandler* aRes,
  void (BluetoothSocketResultHandler::*aMethod)(int, const nsAString&, int),
  int aArg1, const nsAString& aArg2, int aArg3, BluetoothStatus aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == STATUS_SUCCESS) {
    runnable = new BluetoothSocketHALIntStringIntResultRunnable(
      aRes, aMethod, aArg1, aArg2, aArg3);
  } else {
    runnable = new BluetoothSocketHALErrorRunnable(aRes,
      &BluetoothSocketResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_WARNING("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}

void
BluetoothSocketHALInterface::Listen(BluetoothSocketType aType,
                                    const nsAString& aServiceName,
                                    const uint8_t aServiceUuid[16],
                                    int aChannel, bool aEncrypt,
                                    bool aAuth,
                                    BluetoothSocketResultHandler* aRes)
{
  int fd;
  bt_status_t status;
  btsock_type_t type = BTSOCK_RFCOMM; 

  if (NS_SUCCEEDED(Convert(aType, type))) {
    status = mInterface->listen(type,
                                NS_ConvertUTF16toUTF8(aServiceName).get(),
                                aServiceUuid, aChannel, &fd,
                                (BTSOCK_FLAG_ENCRYPT * aEncrypt) |
                                (BTSOCK_FLAG_AUTH * aAuth));
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothSocketHALResult(
      aRes, &BluetoothSocketResultHandler::Listen, fd,
      ConvertDefault(status, STATUS_FAIL));
  }
}

#define CMSGHDR_CONTAINS_FD(_cmsghdr) \
    ( ((_cmsghdr)->cmsg_level == SOL_SOCKET) && \
      ((_cmsghdr)->cmsg_type == SCM_RIGHTS) )















class SocketMessageWatcher : public MessageLoopForIO::Watcher
{
public:
  static const unsigned char MSG1_SIZE = 4;
  static const unsigned char MSG2_SIZE = 16;

  static const unsigned char OFF_CHANNEL1 = 0;
  static const unsigned char OFF_SIZE = 4;
  static const unsigned char OFF_BDADDRESS = 6;
  static const unsigned char OFF_CHANNEL2 = 12;
  static const unsigned char OFF_STATUS = 16;

  SocketMessageWatcher(int aFd)
  : mFd(aFd)
  , mClientFd(-1)
  , mLen(0)
  { }

  virtual ~SocketMessageWatcher()
  { }

  virtual void Proceed(BluetoothStatus aStatus) = 0;

  void OnFileCanReadWithoutBlocking(int aFd) MOZ_OVERRIDE
  {
    BluetoothStatus status;

    switch (mLen) {
      case 0:
        status = RecvMsg1();
        break;
      case MSG1_SIZE:
        status = RecvMsg2();
        break;
      default:
        
        status = STATUS_FAIL;
        break;
    }

    if (IsComplete() || status != STATUS_SUCCESS) {
      mWatcher.StopWatchingFileDescriptor();
      Proceed(status);
    }
  }

  void OnFileCanWriteWithoutBlocking(int aFd) MOZ_OVERRIDE
  { }

  void Watch()
  {
    MessageLoopForIO::current()->WatchFileDescriptor(
      mFd,
      true,
      MessageLoopForIO::WATCH_READ,
      &mWatcher,
      this);
  }

  bool IsComplete() const
  {
    return mLen == (MSG1_SIZE + MSG2_SIZE);
  }

  int GetFd() const
  {
    return mFd;
  }

  int32_t GetChannel1() const
  {
    return ReadInt32(OFF_CHANNEL1);
  }

  int32_t GetSize() const
  {
    return ReadInt16(OFF_SIZE);
  }

  nsString GetBdAddress() const
  {
    nsString bdAddress;
    ReadBdAddress(OFF_BDADDRESS, bdAddress);
    return bdAddress;
  }

  int32_t GetChannel2() const
  {
    return ReadInt32(OFF_CHANNEL2);
  }

  int32_t GetConnectionStatus() const
  {
    return ReadInt32(OFF_STATUS);
  }

  int GetClientFd() const
  {
    return mClientFd;
  }

private:
  BluetoothStatus RecvMsg1()
  {
    struct iovec iv;
    memset(&iv, 0, sizeof(iv));
    iv.iov_base = mBuf;
    iv.iov_len = MSG1_SIZE;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iv;
    msg.msg_iovlen = 1;

    ssize_t res = TEMP_FAILURE_RETRY(recvmsg(mFd, &msg, MSG_NOSIGNAL));
    if (res < 0) {
      return STATUS_FAIL;
    }

    mLen += res;

    return STATUS_SUCCESS;
  }

  BluetoothStatus RecvMsg2()
  {
    struct iovec iv;
    memset(&iv, 0, sizeof(iv));
    iv.iov_base = mBuf + MSG1_SIZE;
    iv.iov_len = MSG2_SIZE;

    struct msghdr msg;
    struct cmsghdr cmsgbuf[2 * sizeof(cmsghdr) + 0x100];
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iv;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    ssize_t res = TEMP_FAILURE_RETRY(recvmsg(mFd, &msg, MSG_NOSIGNAL));
    if (res < 0) {
      return STATUS_FAIL;
    }

    mLen += res;

    if (msg.msg_flags & (MSG_CTRUNC | MSG_OOB | MSG_ERRQUEUE)) {
      return STATUS_FAIL;
    }

    struct cmsghdr *cmsgptr = CMSG_FIRSTHDR(&msg);

    
    for (; cmsgptr; cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {
      if (CMSGHDR_CONTAINS_FD(cmsgptr)) {
        
        
        if (mClientFd != -1) {
          TEMP_FAILURE_RETRY(close(mClientFd));
        }
        
        mClientFd = *(static_cast<int*>(CMSG_DATA(cmsgptr)));
      }
    }

    return STATUS_SUCCESS;
  }

  int16_t ReadInt16(unsigned long aOffset) const
  {
    
    return (static_cast<int16_t>(mBuf[aOffset + 1]) << 8) |
            static_cast<int16_t>(mBuf[aOffset]);
  }

  int32_t ReadInt32(unsigned long aOffset) const
  {
    
    return (static_cast<int32_t>(mBuf[aOffset + 3]) << 24) |
           (static_cast<int32_t>(mBuf[aOffset + 2]) << 16) |
           (static_cast<int32_t>(mBuf[aOffset + 1]) << 8) |
            static_cast<int32_t>(mBuf[aOffset]);
  }

  void ReadBdAddress(unsigned long aOffset, nsAString& aBdAddress) const
  {
    const bt_bdaddr_t* bdAddress =
      reinterpret_cast<const bt_bdaddr_t*>(mBuf+aOffset);

    if (NS_FAILED(Convert(*bdAddress, aBdAddress))) {
      aBdAddress.AssignLiteral(BLUETOOTH_ADDRESS_NONE);
    }
  }

  MessageLoopForIO::FileDescriptorWatcher mWatcher;
  int mFd;
  int mClientFd;
  unsigned char mLen;
  uint8_t mBuf[MSG1_SIZE + MSG2_SIZE];
};




class SocketMessageWatcherTask MOZ_FINAL : public Task
{
public:
  SocketMessageWatcherTask(SocketMessageWatcher* aWatcher)
  : mWatcher(aWatcher)
  {
    MOZ_ASSERT(mWatcher);
  }

  void Run() MOZ_OVERRIDE
  {
    mWatcher->Watch();
  }

private:
  SocketMessageWatcher* mWatcher;
};



template <typename T>
class DeleteTask MOZ_FINAL : public Task
{
public:
  DeleteTask(T* aPtr)
  : mPtr(aPtr)
  { }

  void Run() MOZ_OVERRIDE
  {
    mPtr = nullptr;
  }

private:
  nsAutoPtr<T> mPtr;
};






class ConnectWatcher MOZ_FINAL : public SocketMessageWatcher
{
public:
  ConnectWatcher(int aFd, BluetoothSocketResultHandler* aRes)
  : SocketMessageWatcher(aFd)
  , mRes(aRes)
  { }

  void Proceed(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    if (mRes) {
      DispatchBluetoothSocketHALResult(
        mRes, &BluetoothSocketResultHandler::Connect, GetFd(),
        GetBdAddress(), GetConnectionStatus(), aStatus);
    }
    MessageLoopForIO::current()->PostTask(
      FROM_HERE, new DeleteTask<ConnectWatcher>(this));
  }

private:
  nsRefPtr<BluetoothSocketResultHandler> mRes;
};

void
BluetoothSocketHALInterface::Connect(const nsAString& aBdAddr,
                                     BluetoothSocketType aType,
                                     const uint8_t aUuid[16],
                                     int aChannel, bool aEncrypt,
                                     bool aAuth,
                                     BluetoothSocketResultHandler* aRes)
{
  int fd;
  bt_status_t status;
  bt_bdaddr_t bdAddr;
  btsock_type_t type = BTSOCK_RFCOMM; 

  if (NS_SUCCEEDED(Convert(aBdAddr, bdAddr)) &&
      NS_SUCCEEDED(Convert(aType, type))) {
    status = mInterface->connect(&bdAddr, type, aUuid, aChannel, &fd,
                                 (BTSOCK_FLAG_ENCRYPT * aEncrypt) |
                                 (BTSOCK_FLAG_AUTH * aAuth));
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (status == BT_STATUS_SUCCESS) {
    
    Task* t = new SocketMessageWatcherTask(new ConnectWatcher(fd, aRes));
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE, t);
  } else if (aRes) {
    DispatchBluetoothSocketHALResult(
      aRes, &BluetoothSocketResultHandler::Connect, -1, EmptyString(), 0,
      ConvertDefault(status, STATUS_FAIL));
  }
}








class AcceptWatcher MOZ_FINAL : public SocketMessageWatcher
{
public:
  AcceptWatcher(int aFd, BluetoothSocketResultHandler* aRes)
  : SocketMessageWatcher(aFd)
  , mRes(aRes)
  {
    
    MOZ_ASSERT(mRes);
  }

  void Proceed(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    if (mRes) {
      DispatchBluetoothSocketHALResult(
        mRes, &BluetoothSocketResultHandler::Accept, GetClientFd(),
        GetBdAddress(), GetConnectionStatus(), aStatus);
    }
    MessageLoopForIO::current()->PostTask(
      FROM_HERE, new DeleteTask<AcceptWatcher>(this));
  }

private:
  nsRefPtr<BluetoothSocketResultHandler> mRes;
};

void
BluetoothSocketHALInterface::Accept(int aFd,
                                    BluetoothSocketResultHandler* aRes)
{
  
  Task* t = new SocketMessageWatcherTask(new AcceptWatcher(aFd, aRes));
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, t);
}

BluetoothSocketHALInterface::BluetoothSocketHALInterface(
  const btsock_interface_t* aInterface)
: mInterface(aInterface)
{
  MOZ_ASSERT(mInterface);
}

BluetoothSocketHALInterface::~BluetoothSocketHALInterface()
{ }

END_BLUETOOTH_NAMESPACE
