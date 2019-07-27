





#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include "base/message_loop.h"
#include "BluetoothInterface.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#if MOZ_IS_GCC && MOZ_GCC_VERSION_AT_LEAST(4, 7, 0)

#define CONVERT(in_, out_) \
  [in_] = out_
#else

#define CONVERT(in_, out_) \
  out_
#endif

BEGIN_BLUETOOTH_NAMESPACE

template<class T>
struct interface_traits
{ };





static nsresult
Convert(const nsAString& aIn, bt_property_type_t& aOut)
{
  if (aIn.EqualsLiteral("Name")) {
    aOut = BT_PROPERTY_BDNAME;
  } else if (aIn.EqualsLiteral("Discoverable")) {
    aOut = BT_PROPERTY_ADAPTER_SCAN_MODE;
  } else if (aIn.EqualsLiteral("DiscoverableTimeout")) {
    aOut = BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT;
  } else {
    BT_LOGR("Invalid property name: %s", NS_ConvertUTF16toUTF8(aIn).get());
    return NS_ERROR_ILLEGAL_VALUE;
  }
  return NS_OK;
}

static nsresult
Convert(bool aIn, bt_scan_mode_t& aOut)
{
  static const bt_scan_mode_t sScanMode[] = {
    CONVERT(false, BT_SCAN_MODE_CONNECTABLE),
    CONVERT(true, BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE)
  };
  if (aIn >= MOZ_ARRAY_LENGTH(sScanMode)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  aOut = sScanMode[aIn];
  return NS_OK;
}

struct ConvertNamedValue
{
  ConvertNamedValue(const BluetoothNamedValue& aNamedValue)
  : mNamedValue(aNamedValue)
  { }

  const BluetoothNamedValue& mNamedValue;

  
  nsCString mStringValue;
  bt_scan_mode_t mScanMode;
};

static nsresult
Convert(ConvertNamedValue& aIn, bt_property_t& aOut)
{
  nsresult rv = Convert(aIn.mNamedValue.name(), aOut.type);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aIn.mNamedValue.value().type() == BluetoothValue::Tuint32_t) {
    
    aOut.val =
      reinterpret_cast<void*>(aIn.mNamedValue.value().get_uint32_t());
  } else if (aIn.mNamedValue.value().type() == BluetoothValue::TnsString) {
    
    aIn.mStringValue =
      NS_ConvertUTF16toUTF8(aIn.mNamedValue.value().get_nsString());
    aOut.val =
      const_cast<void*>(static_cast<const void*>(aIn.mStringValue.get()));
    aOut.len = strlen(static_cast<char*>(aOut.val));
  } else if (aIn.mNamedValue.value().type() == BluetoothValue::Tbool) {
    
    rv = Convert(aIn.mNamedValue.value().get_bool(), aIn.mScanMode);
    if (NS_FAILED(rv)) {
      return rv;
    }
    aOut.val = &aIn.mScanMode;
    aOut.len = sizeof(aIn.mScanMode);
  } else {
    BT_LOGR("Invalid property value type");
    return NS_ERROR_ILLEGAL_VALUE;
  }

  return NS_OK;
}

static nsresult
Convert(const nsAString& aIn, bt_bdaddr_t& aOut)
{
  NS_ConvertUTF16toUTF8 bdAddressUTF8(aIn);
  const char* str = bdAddressUTF8.get();

  for (size_t i = 0; i < MOZ_ARRAY_LENGTH(aOut.address); ++i, ++str) {
    aOut.address[i] =
      static_cast<uint8_t>(strtoul(str, const_cast<char**>(&str), 16));
  }

  return NS_OK;
}

static nsresult
Convert(const nsAString& aIn, bt_ssp_variant_t& aOut)
{
  if (aIn.EqualsLiteral("PasskeyConfirmation")) {
    aOut = BT_SSP_VARIANT_PASSKEY_CONFIRMATION;
  } else if (aIn.EqualsLiteral("PasskeyEntry")) {
    aOut = BT_SSP_VARIANT_PASSKEY_ENTRY;
  } else if (aIn.EqualsLiteral("Consent")) {
    aOut = BT_SSP_VARIANT_CONSENT;
  } else if (aIn.EqualsLiteral("PasskeyNotification")) {
    aOut = BT_SSP_VARIANT_PASSKEY_NOTIFICATION;
  } else {
    BT_LOGR("Invalid SSP variant name: %s", NS_ConvertUTF16toUTF8(aIn).get());
    aOut = BT_SSP_VARIANT_PASSKEY_CONFIRMATION; 
    return NS_ERROR_ILLEGAL_VALUE;
  }
  return NS_OK;
}

static nsresult
Convert(const bool& aIn, uint8_t& aOut)
{
  
  aOut = static_cast<uint8_t>(aIn);
  return NS_OK;
}

static nsresult
Convert(const uint8_t aIn[16], bt_uuid_t& aOut)
{
  if (sizeof(aOut.uu) != 16) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  memcpy(aOut.uu, aIn, sizeof(aOut.uu));

  return NS_OK;
}

static nsresult
Convert(const nsAString& aIn, bt_pin_code_t& aOut)
{
  if (aIn.Length() > MOZ_ARRAY_LENGTH(aOut.pin)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  NS_ConvertUTF16toUTF8 pinCodeUTF8(aIn);
  const char* str = pinCodeUTF8.get();

  nsAString::size_type i;

  
  for (i = 0; i < aIn.Length(); ++i, ++str) {
    aOut.pin[i] = static_cast<uint8_t>(*str);
  }

  
  size_t ntrailing =
    (MOZ_ARRAY_LENGTH(aOut.pin) - aIn.Length()) * sizeof(aOut.pin[0]);
  memset(aOut.pin + aIn.Length(), 0, ntrailing);

  return NS_OK;
}

static nsresult
Convert(const bt_bdaddr_t& aIn, nsAString& aOut)
{
  char str[BLUETOOTH_ADDRESS_LENGTH + 1];

  int res = snprintf(str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     static_cast<int>(aIn.address[0]),
                     static_cast<int>(aIn.address[1]),
                     static_cast<int>(aIn.address[2]),
                     static_cast<int>(aIn.address[3]),
                     static_cast<int>(aIn.address[4]),
                     static_cast<int>(aIn.address[5]));
  if (res < 0) {
    return NS_ERROR_ILLEGAL_VALUE;
  } else if ((size_t)res >= sizeof(str)) {
    return NS_ERROR_OUT_OF_MEMORY; 
  }

  aOut = NS_ConvertUTF8toUTF16(str);

  return NS_OK;
}

static nsresult
Convert(BluetoothSocketType aIn, btsock_type_t& aOut)
{
  
  
  
  static const btsock_type_t sSocketType[] = {
    CONVERT(0, static_cast<btsock_type_t>(0)), 
    CONVERT(BluetoothSocketType::RFCOMM, BTSOCK_RFCOMM),
    CONVERT(BluetoothSocketType::SCO, BTSOCK_SCO),
    CONVERT(BluetoothSocketType::L2CAP, BTSOCK_L2CAP),
    
  };
  if (aIn == BluetoothSocketType::EL2CAP ||
      aIn >= MOZ_ARRAY_LENGTH(sSocketType) || !sSocketType[aIn]) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  aOut = sSocketType[aIn];
  return NS_OK;
}





template <typename Obj, typename Res>
class BluetoothInterfaceRunnable0 : public nsRunnable
{
public:
  BluetoothInterfaceRunnable0(Obj* aObj, Res (Obj::*aMethod)())
  : mObj(aObj)
  , mMethod(aMethod)
  {
    MOZ_ASSERT(mObj);
    MOZ_ASSERT(mMethod);
  }

  NS_METHOD
  Run() MOZ_OVERRIDE
  {
    ((*mObj).*mMethod)();
    return NS_OK;
  }

private:
  nsRefPtr<Obj> mObj;
  void (Obj::*mMethod)();
};

template <typename Obj, typename Res, typename Tin1, typename Arg1>
class BluetoothInterfaceRunnable1 : public nsRunnable
{
public:
  BluetoothInterfaceRunnable1(Obj* aObj, Res (Obj::*aMethod)(Arg1),
                              const Arg1& aArg1)
  : mObj(aObj)
  , mMethod(aMethod)
  , mArg1(aArg1)
  {
    MOZ_ASSERT(mObj);
    MOZ_ASSERT(mMethod);
  }

  NS_METHOD
  Run() MOZ_OVERRIDE
  {
    ((*mObj).*mMethod)(mArg1);
    return NS_OK;
  }

private:
  nsRefPtr<Obj> mObj;
  Res (Obj::*mMethod)(Arg1);
  Tin1 mArg1;
};

template <typename Obj, typename Res,
          typename Tin1, typename Tin2, typename Tin3,
          typename Arg1, typename Arg2, typename Arg3>
class BluetoothInterfaceRunnable3 : public nsRunnable
{
public:
  BluetoothInterfaceRunnable3(Obj* aObj,
                              Res (Obj::*aMethod)(Arg1, Arg2, Arg3),
                              const Arg1& aArg1, const Arg2& aArg2,
                              const Arg3& aArg3)
  : mObj(aObj)
  , mMethod(aMethod)
  , mArg1(aArg1)
  , mArg2(aArg2)
  , mArg3(aArg3)
  {
    MOZ_ASSERT(mObj);
    MOZ_ASSERT(mMethod);
  }

  NS_METHOD
  Run() MOZ_OVERRIDE
  {
    ((*mObj).*mMethod)(mArg1, mArg2, mArg3);
    return NS_OK;
  }

private:
  nsRefPtr<Obj> mObj;
  Res (Obj::*mMethod)(Arg1, Arg2, Arg3);
  Tin1 mArg1;
  Tin2 mArg2;
  Tin3 mArg3;
};





template<>
struct interface_traits<BluetoothSocketInterface>
{
  typedef const btsock_interface_t const_interface_type;

  static const char* profile_id()
  {
    return BT_PROFILE_SOCKETS_ID;
  }
};

typedef
  BluetoothInterfaceRunnable1<BluetoothSocketResultHandler, void, int, int>
  BluetoothSocketIntResultRunnable;

typedef
  BluetoothInterfaceRunnable3<BluetoothSocketResultHandler, void,
                              int, const nsString, int,
                              int, const nsAString_internal&, int>
  BluetoothSocketIntStringIntResultRunnable;

typedef
  BluetoothInterfaceRunnable1<BluetoothSocketResultHandler, void,
                              bt_status_t, bt_status_t>
  BluetoothSocketErrorRunnable;

static nsresult
DispatchBluetoothSocketResult(BluetoothSocketResultHandler* aRes,
                              void (BluetoothSocketResultHandler::*aMethod)(int),
                              int aArg, bt_status_t aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == BT_STATUS_SUCCESS) {
    runnable = new BluetoothSocketIntResultRunnable(aRes, aMethod, aArg);
  } else {
    runnable = new BluetoothSocketErrorRunnable(aRes,
      &BluetoothSocketResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_WARNING("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}

static nsresult
DispatchBluetoothSocketResult(
  BluetoothSocketResultHandler* aRes,
  void (BluetoothSocketResultHandler::*aMethod)(int, const nsAString&, int),
  int aArg1, const nsAString& aArg2, int aArg3, bt_status_t aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == BT_STATUS_SUCCESS) {
    runnable = new BluetoothSocketIntStringIntResultRunnable(aRes, aMethod,
                                                             aArg1, aArg2,
                                                             aArg3);
  } else {
    runnable = new BluetoothSocketErrorRunnable(aRes,
      &BluetoothSocketResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_WARNING("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}

void
BluetoothSocketInterface::Listen(BluetoothSocketType aType,
                                 const nsAString& aServiceName,
                                 const uint8_t aServiceUuid[16],
                                 int aChannel, bool aEncrypt, bool aAuth,
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
    DispatchBluetoothSocketResult(aRes, &BluetoothSocketResultHandler::Listen,
                                  fd, status);
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

  virtual void Proceed(bt_status_t aStatus) = 0;

  void OnFileCanReadWithoutBlocking(int aFd) MOZ_OVERRIDE
  {
    bt_status_t status;

    switch (mLen) {
      case 0:
        status = RecvMsg1();
        break;
      case MSG1_SIZE:
        status = RecvMsg2();
        break;
      default:
        
        status = BT_STATUS_FAIL;
        break;
    }

    if (IsComplete() || status != BT_STATUS_SUCCESS) {
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
  bt_status_t RecvMsg1()
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
      return BT_STATUS_FAIL;
    }

    mLen += res;

    return BT_STATUS_SUCCESS;
  }

  bt_status_t RecvMsg2()
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
      return BT_STATUS_FAIL;
    }

    mLen += res;

    if (msg.msg_flags & (MSG_CTRUNC | MSG_OOB | MSG_ERRQUEUE)) {
      return BT_STATUS_FAIL;
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

    return BT_STATUS_SUCCESS;
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

  void Proceed(bt_status_t aStatus) MOZ_OVERRIDE
  {
    if (mRes) {
      DispatchBluetoothSocketResult(mRes,
                                   &BluetoothSocketResultHandler::Connect,
                                    GetFd(), GetBdAddress(),
                                    GetConnectionStatus(), aStatus);
    }
    MessageLoopForIO::current()->PostTask(
      FROM_HERE, new DeleteTask<ConnectWatcher>(this));
  }

private:
  nsRefPtr<BluetoothSocketResultHandler> mRes;
};

void
BluetoothSocketInterface::Connect(const nsAString& aBdAddr,
                                  BluetoothSocketType aType,
                                  const uint8_t aUuid[16],
                                  int aChannel, bool aEncrypt, bool aAuth,
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
    DispatchBluetoothSocketResult(aRes,
                                  &BluetoothSocketResultHandler::Connect,
                                  -1, EmptyString(), 0, status);
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

  void Proceed(bt_status_t aStatus) MOZ_OVERRIDE
  {
    if (mRes) {
      DispatchBluetoothSocketResult(mRes,
                                    &BluetoothSocketResultHandler::Accept,
                                    GetClientFd(), GetBdAddress(),
                                    GetConnectionStatus(),
                                    aStatus);
    }
    MessageLoopForIO::current()->PostTask(
      FROM_HERE, new DeleteTask<AcceptWatcher>(this));
  }

private:
  nsRefPtr<BluetoothSocketResultHandler> mRes;
};

void
BluetoothSocketInterface::Accept(int aFd, BluetoothSocketResultHandler* aRes)
{
  
  Task* t = new SocketMessageWatcherTask(new AcceptWatcher(aFd, aRes));
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, t);
}

BluetoothSocketInterface::BluetoothSocketInterface(
  const btsock_interface_t* aInterface)
: mInterface(aInterface)
{
  MOZ_ASSERT(mInterface);
}

BluetoothSocketInterface::~BluetoothSocketInterface()
{ }





template<>
struct interface_traits<BluetoothHandsfreeInterface>
{
  typedef const bthf_interface_t const_interface_type;

  static const char* profile_id()
  {
    return BT_PROFILE_HANDSFREE_ID;
  }
};

typedef
  BluetoothInterfaceRunnable0<BluetoothHandsfreeResultHandler, void>
  BluetoothHandsfreeResultRunnable;

typedef
  BluetoothInterfaceRunnable1<BluetoothHandsfreeResultHandler, void,
                              bt_status_t, bt_status_t>
  BluetoothHandsfreeErrorRunnable;

static nsresult
DispatchBluetoothHandsfreeResult(
  BluetoothHandsfreeResultHandler* aRes,
  void (BluetoothHandsfreeResultHandler::*aMethod)(),
  bt_status_t aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == BT_STATUS_SUCCESS) {
    runnable = new BluetoothHandsfreeResultRunnable(aRes, aMethod);
  } else {
    runnable = new BluetoothHandsfreeErrorRunnable(aRes,
      &BluetoothHandsfreeResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_WARNING("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}

BluetoothHandsfreeInterface::BluetoothHandsfreeInterface(
  const bthf_interface_t* aInterface)
: mInterface(aInterface)
{
  MOZ_ASSERT(mInterface);
}

BluetoothHandsfreeInterface::~BluetoothHandsfreeInterface()
{ }

void
BluetoothHandsfreeInterface::Init(bthf_callbacks_t* aCallbacks,
                                  BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->init(aCallbacks);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(aRes,
                                     &BluetoothHandsfreeResultHandler::Init,
                                     status);
  }
}

void
BluetoothHandsfreeInterface::Cleanup(BluetoothHandsfreeResultHandler* aRes)
{
  mInterface->cleanup();

  if (aRes) {
    DispatchBluetoothHandsfreeResult(aRes,
                                     &BluetoothHandsfreeResultHandler::Cleanup,
                                     BT_STATUS_SUCCESS);
  }
}



void
BluetoothHandsfreeInterface::Connect(bt_bdaddr_t* aBdAddr,
                                     BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->connect(aBdAddr);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::Connect, status);
  }
}

void
BluetoothHandsfreeInterface::Disconnect(bt_bdaddr_t* aBdAddr,
                                        BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->disconnect(aBdAddr);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::Disconnect, status);
  }
}

void
BluetoothHandsfreeInterface::ConnectAudio(
  bt_bdaddr_t* aBdAddr, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->connect_audio(aBdAddr);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::ConnectAudio, status);
  }
}

void
BluetoothHandsfreeInterface::DisconnectAudio(
  bt_bdaddr_t* aBdAddr, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->disconnect_audio(aBdAddr);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::DisconnectAudio, status);
  }
}



void
BluetoothHandsfreeInterface::StartVoiceRecognition(
  BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->start_voice_recognition();

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::StartVoiceRecognition, status);
  }
}

void
BluetoothHandsfreeInterface::StopVoiceRecognition(
  BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->stop_voice_recognition();

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::StopVoiceRecognition, status);
  }
}



void
BluetoothHandsfreeInterface::VolumeControl(
  bthf_volume_type_t aType, int aVolume, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->volume_control(aType, aVolume);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::VolumeControl, status);
  }
}



void
BluetoothHandsfreeInterface::DeviceStatusNotification(
  bthf_network_state_t aNtkState, bthf_service_type_t aSvcType, int aSignal,
  int aBattChg, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->device_status_notification(
    aNtkState, aSvcType, aSignal, aBattChg);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::DeviceStatusNotification,
      status);
  }
}



void
BluetoothHandsfreeInterface::CopsResponse(
  const char* aCops, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->cops_response(aCops);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::CopsResponse, status);
  }
}

void
BluetoothHandsfreeInterface::CindResponse(
  int aSvc, int aNumActive, int aNumHeld, bthf_call_state_t aCallSetupState,
  int aSignal, int aRoam, int aBattChg, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->cind_response(aSvc, aNumActive, aNumHeld,
                                                 aCallSetupState, aSignal,
                                                 aRoam, aBattChg);
  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::CindResponse, status);
  }
}

void
BluetoothHandsfreeInterface::FormattedAtResponse(
  const char* aRsp, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->formatted_at_response(aRsp);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::FormattedAtResponse, status);
  }
}

void
BluetoothHandsfreeInterface::AtResponse(bthf_at_response_t aResponseCode,
                                        int aErrorCode,
                                        BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->at_response(aResponseCode, aErrorCode);

  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::AtResponse, status);
  }
}

void
BluetoothHandsfreeInterface::ClccResponse(
  int aIndex, bthf_call_direction_t aDir, bthf_call_state_t aState,
  bthf_call_mode_t aMode, bthf_call_mpty_type_t aMpty, const char* aNumber,
  bthf_call_addrtype_t aType, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->clcc_response(aIndex, aDir, aState, aMode,
                                                 aMpty, aNumber, aType);
  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::ClccResponse, status);
  }
}



void
BluetoothHandsfreeInterface::PhoneStateChange(int aNumActive, int aNumHeld,
  bthf_call_state_t aCallSetupState, const char* aNumber,
  bthf_call_addrtype_t aType, BluetoothHandsfreeResultHandler* aRes)
{
  bt_status_t status = mInterface->phone_state_change(aNumActive, aNumHeld,
                                                      aCallSetupState,
                                                      aNumber, aType);
  if (aRes) {
    DispatchBluetoothHandsfreeResult(
      aRes, &BluetoothHandsfreeResultHandler::PhoneStateChange, status);
  }
}





template<>
struct interface_traits<BluetoothA2dpInterface>
{
  typedef const btav_interface_t const_interface_type;

  static const char* profile_id()
  {
    return BT_PROFILE_ADVANCED_AUDIO_ID;
  }
};

typedef
  BluetoothInterfaceRunnable0<BluetoothA2dpResultHandler, void>
  BluetoothA2dpResultRunnable;

typedef
  BluetoothInterfaceRunnable1<BluetoothA2dpResultHandler, void,
                              bt_status_t, bt_status_t>
  BluetoothA2dpErrorRunnable;

static nsresult
DispatchBluetoothA2dpResult(
  BluetoothA2dpResultHandler* aRes,
  void (BluetoothA2dpResultHandler::*aMethod)(),
  bt_status_t aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == BT_STATUS_SUCCESS) {
    runnable = new BluetoothA2dpResultRunnable(aRes, aMethod);
  } else {
    runnable = new BluetoothA2dpErrorRunnable(aRes,
      &BluetoothA2dpResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_WARNING("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}

BluetoothA2dpInterface::BluetoothA2dpInterface(
  const btav_interface_t* aInterface)
: mInterface(aInterface)
{
  MOZ_ASSERT(mInterface);
}

BluetoothA2dpInterface::~BluetoothA2dpInterface()
{ }

void
BluetoothA2dpInterface::Init(btav_callbacks_t* aCallbacks,
                             BluetoothA2dpResultHandler* aRes)
{
  bt_status_t status = mInterface->init(aCallbacks);

  if (aRes) {
    DispatchBluetoothA2dpResult(aRes, &BluetoothA2dpResultHandler::Init,
                                status);
  }
}

void
BluetoothA2dpInterface::Cleanup(BluetoothA2dpResultHandler* aRes)
{
  mInterface->cleanup();

  if (aRes) {
    DispatchBluetoothA2dpResult(aRes, &BluetoothA2dpResultHandler::Cleanup,
                                BT_STATUS_SUCCESS);
  }
}

void
BluetoothA2dpInterface::Connect(bt_bdaddr_t *aBdAddr,
                                BluetoothA2dpResultHandler* aRes)
{
  bt_status_t status = mInterface->connect(aBdAddr);

  if (aRes) {
    DispatchBluetoothA2dpResult(aRes, &BluetoothA2dpResultHandler::Connect,
                                status);
  }
}

void
BluetoothA2dpInterface::Disconnect(bt_bdaddr_t *aBdAddr,
                                   BluetoothA2dpResultHandler* aRes)
{
  bt_status_t status = mInterface->disconnect(aBdAddr);

  if (aRes) {
    DispatchBluetoothA2dpResult(aRes, &BluetoothA2dpResultHandler::Disconnect,
                                status);
  }
}





#if ANDROID_VERSION >= 18
template<>
struct interface_traits<BluetoothAvrcpInterface>
{
  typedef const btrc_interface_t const_interface_type;

  static const char* profile_id()
  {
    return BT_PROFILE_AV_RC_ID;
  }
};

typedef
  BluetoothInterfaceRunnable0<BluetoothAvrcpResultHandler, void>
  BluetoothAvrcpResultRunnable;

typedef
  BluetoothInterfaceRunnable1<BluetoothAvrcpResultHandler, void,
                              bt_status_t, bt_status_t>
  BluetoothAvrcpErrorRunnable;

static nsresult
DispatchBluetoothAvrcpResult(
  BluetoothAvrcpResultHandler* aRes,
  void (BluetoothAvrcpResultHandler::*aMethod)(),
  bt_status_t aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == BT_STATUS_SUCCESS) {
    runnable = new BluetoothAvrcpResultRunnable(aRes, aMethod);
  } else {
    runnable = new BluetoothAvrcpErrorRunnable(aRes,
      &BluetoothAvrcpResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_WARNING("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}

BluetoothAvrcpInterface::BluetoothAvrcpInterface(
  const btrc_interface_t* aInterface)
: mInterface(aInterface)
{
  MOZ_ASSERT(mInterface);
}

BluetoothAvrcpInterface::~BluetoothAvrcpInterface()
{ }

void
BluetoothAvrcpInterface::Init(btrc_callbacks_t* aCallbacks,
                              BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->init(aCallbacks);

  if (aRes) {
    DispatchBluetoothAvrcpResult(aRes, &BluetoothAvrcpResultHandler::Init,
                                 status);
  }
}

void
BluetoothAvrcpInterface::Cleanup(BluetoothAvrcpResultHandler* aRes)
{
  mInterface->cleanup();

  if (aRes) {
    DispatchBluetoothAvrcpResult(aRes, &BluetoothAvrcpResultHandler::Cleanup,
                                 BT_STATUS_SUCCESS);
  }
}

void
BluetoothAvrcpInterface::GetPlayStatusRsp(btrc_play_status_t aPlayStatus,
                                          uint32_t aSongLen, uint32_t aSongPos,
                                          BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->get_play_status_rsp(aPlayStatus, aSongLen,
                                                       aSongPos);
  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::GetPlayStatusRsp, status);
  }
}

void
BluetoothAvrcpInterface::ListPlayerAppAttrRsp(
  int aNumAttr, btrc_player_attr_t* aPAttrs,
  BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->list_player_app_attr_rsp(aNumAttr, aPAttrs);

  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::ListPlayerAppAttrRsp, status);
  }
}

void
BluetoothAvrcpInterface::ListPlayerAppValueRsp(
  int aNumVal, uint8_t* aPVals, BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->list_player_app_value_rsp(aNumVal, aPVals);

  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::ListPlayerAppValueRsp, status);
  }
}

void
BluetoothAvrcpInterface::GetPlayerAppValueRsp(
  btrc_player_settings_t* aPVals, BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->get_player_app_value_rsp(aPVals);

  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::GetPlayerAppValueRsp, status);
  }
}

void
BluetoothAvrcpInterface::GetPlayerAppAttrTextRsp(
  int aNumAttr, btrc_player_setting_text_t* aPAttrs,
  BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->get_player_app_attr_text_rsp(aNumAttr,
                                                                aPAttrs);
  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::GetPlayerAppAttrTextRsp, status);
  }
}

void
BluetoothAvrcpInterface::GetPlayerAppValueTextRsp(
  int aNumVal, btrc_player_setting_text_t* aPVals,
  BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->get_player_app_value_text_rsp(aNumVal,
                                                                 aPVals);
  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::GetPlayerAppValueTextRsp, status);
  }
}

void
BluetoothAvrcpInterface::GetElementAttrRsp(
  uint8_t aNumAttr, btrc_element_attr_val_t* aPAttrs,
  BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->get_element_attr_rsp(aNumAttr, aPAttrs);

  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::GetElementAttrRsp, status);
  }
}

void
BluetoothAvrcpInterface::SetPlayerAppValueRsp(
  btrc_status_t aRspStatus, BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->set_player_app_value_rsp(aRspStatus);

  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::SetPlayerAppValueRsp, status);
  }
}

void
BluetoothAvrcpInterface::RegisterNotificationRsp(
  btrc_event_id_t aEventId, btrc_notification_type_t aType,
  btrc_register_notification_t* aPParam, BluetoothAvrcpResultHandler* aRes)
{
  bt_status_t status = mInterface->register_notification_rsp(aEventId, aType,
                                                             aPParam);
  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::RegisterNotificationRsp, status);
  }
}

void
BluetoothAvrcpInterface::SetVolume(uint8_t aVolume,
                                   BluetoothAvrcpResultHandler* aRes)
{
#if ANDROID_VERSION >= 19
  bt_status_t status = mInterface->set_volume(aVolume);
#else
  bt_status_t status = BT_STATUS_UNSUPPORTED;
#endif

  if (aRes) {
    DispatchBluetoothAvrcpResult(
      aRes, &BluetoothAvrcpResultHandler::SetVolume, status);
  }
}
#endif 





typedef
  BluetoothInterfaceRunnable0<BluetoothResultHandler, void>
  BluetoothResultRunnable;

typedef
  BluetoothInterfaceRunnable1<BluetoothResultHandler, void, int, int>
  BluetoothErrorRunnable;

static nsresult
DispatchBluetoothResult(BluetoothResultHandler* aRes,
                        void (BluetoothResultHandler::*aMethod)(),
                        int aStatus)
{
  MOZ_ASSERT(aRes);

  nsRunnable* runnable;

  if (aStatus == BT_STATUS_SUCCESS) {
    runnable = new BluetoothResultRunnable(aRes, aMethod);
  } else {
    runnable = new
      BluetoothErrorRunnable(aRes, &BluetoothResultHandler::OnError, aStatus);
  }
  nsresult rv = NS_DispatchToMainThread(runnable);
  if (NS_FAILED(rv)) {
    BT_LOGR("NS_DispatchToMainThread failed: %X", rv);
  }
  return rv;
}




#define container(_t, _v, _m) \
  ( (_t*)( ((const unsigned char*)(_v)) - offsetof(_t, _m) ) )

BluetoothInterface*
BluetoothInterface::GetInstance()
{
  static BluetoothInterface* sBluetoothInterface;

  if (sBluetoothInterface) {
    return sBluetoothInterface;
  }

  

  const hw_module_t* module;
  int err = hw_get_module(BT_HARDWARE_MODULE_ID, &module);
  if (err) {
    BT_WARNING("hw_get_module failed: %s", strerror(err));
    return nullptr;
  }

  

  hw_device_t* device;
  err = module->methods->open(module, BT_HARDWARE_MODULE_ID, &device);
  if (err) {
    BT_WARNING("open failed: %s", strerror(err));
    return nullptr;
  }

  const bluetooth_device_t* bt_device =
    container(bluetooth_device_t, device, common);

  

  const bt_interface_t* bt_interface = bt_device->get_bluetooth_interface();
  if (!bt_interface) {
    BT_WARNING("get_bluetooth_interface failed");
    goto err_get_bluetooth_interface;
  }

  if (bt_interface->size != sizeof(*bt_interface)) {
    BT_WARNING("interface of incorrect size");
    goto err_bt_interface_size;
  }

  sBluetoothInterface = new BluetoothInterface(bt_interface);

  return sBluetoothInterface;

err_bt_interface_size:
err_get_bluetooth_interface:
  err = device->close(device);
  if (err) {
    BT_WARNING("close failed: %s", strerror(err));
  }
  return nullptr;
}

BluetoothInterface::BluetoothInterface(const bt_interface_t* aInterface)
: mInterface(aInterface)
{
  MOZ_ASSERT(mInterface);
}

BluetoothInterface::~BluetoothInterface()
{ }

void
BluetoothInterface::Init(bt_callbacks_t* aCallbacks,
                         BluetoothResultHandler* aRes)
{
  int status = mInterface->init(aCallbacks);

  if (aRes) {
    DispatchBluetoothResult(aRes, &BluetoothResultHandler::Init, status);
  }
}

void
BluetoothInterface::Cleanup(BluetoothResultHandler* aRes)
{
  mInterface->cleanup();

  if (aRes) {
    DispatchBluetoothResult(aRes, &BluetoothResultHandler::Cleanup,
                            BT_STATUS_SUCCESS);
  }
}

void
BluetoothInterface::Enable(BluetoothResultHandler* aRes)
{
  int status = mInterface->enable();

  if (aRes) {
    DispatchBluetoothResult(aRes, &BluetoothResultHandler::Enable, status);
  }
}

void
BluetoothInterface::Disable(BluetoothResultHandler* aRes)
{
  int status = mInterface->disable();

  if (aRes) {
    DispatchBluetoothResult(aRes, &BluetoothResultHandler::Disable, status);
  }
}



void
BluetoothInterface::GetAdapterProperties(BluetoothResultHandler* aRes)
{
  int status = mInterface->get_adapter_properties();

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::GetAdapterProperties,
                            status);
  }
}

void
BluetoothInterface::GetAdapterProperty(const nsAString& aName,
                                       BluetoothResultHandler* aRes)
{
  int status;
  bt_property_type_t type;

  
  NS_NOTREACHED("Conversion function missing");

  if (false ) {
    status = mInterface->get_adapter_property(type);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::GetAdapterProperties,
                            status);
  }
}

void
BluetoothInterface::SetAdapterProperty(const BluetoothNamedValue& aProperty,
                                       BluetoothResultHandler* aRes)
{
  int status;
  ConvertNamedValue convertProperty(aProperty);
  bt_property_t property;

  if (NS_SUCCEEDED(Convert(convertProperty, property))) {
    status = mInterface->set_adapter_property(&property);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::SetAdapterProperty,
                            status);
  }
}



void
BluetoothInterface::GetRemoteDeviceProperties(const nsAString& aRemoteAddr,
                                              BluetoothResultHandler* aRes)
{
  int status;
  bt_bdaddr_t addr;

  if (NS_SUCCEEDED(Convert(aRemoteAddr, addr))) {
    status = mInterface->get_remote_device_properties(&addr);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::GetRemoteDeviceProperties,
                            status);
  }
}

void
BluetoothInterface::GetRemoteDeviceProperty(const nsAString& aRemoteAddr,
                                            const nsAString& aName,
                                            BluetoothResultHandler* aRes)
{
  int status;
  bt_bdaddr_t remoteAddr;
  bt_property_type_t name;

  
  NS_NOTREACHED("Conversion function missing");

  if (NS_SUCCEEDED(Convert(aRemoteAddr, remoteAddr)) &&
      false ) {
    status = mInterface->get_remote_device_property(&remoteAddr, name);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::GetRemoteDeviceProperty,
                            status);
  }
}

void
BluetoothInterface::SetRemoteDeviceProperty(const nsAString& aRemoteAddr,
                                            const BluetoothNamedValue& aProperty,
                                            BluetoothResultHandler* aRes)
{
  int status;
  bt_bdaddr_t remoteAddr;
  bt_property_t property;

  
  NS_NOTREACHED("Conversion function missing");

  if (NS_SUCCEEDED(Convert(aRemoteAddr, remoteAddr)) &&
      false ) {
    status = mInterface->set_remote_device_property(&remoteAddr, &property);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::SetRemoteDeviceProperty,
                            status);
  }
}



void
BluetoothInterface::GetRemoteServiceRecord(const nsAString& aRemoteAddr,
                                           const uint8_t aUuid[16],
                                           BluetoothResultHandler* aRes)
{
  int status;
  bt_bdaddr_t remoteAddr;
  bt_uuid_t uuid;

  if (NS_SUCCEEDED(Convert(aRemoteAddr, remoteAddr)) &&
      NS_SUCCEEDED(Convert(aUuid, uuid))) {
    status = mInterface->get_remote_service_record(&remoteAddr, &uuid);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::GetRemoteServiceRecord,
                            status);
  }
}

void
BluetoothInterface::GetRemoteServices(const nsAString& aRemoteAddr,
                                      BluetoothResultHandler* aRes)
{
  int status;
  bt_bdaddr_t remoteAddr;

  if (NS_SUCCEEDED(Convert(aRemoteAddr, remoteAddr))) {
    status = mInterface->get_remote_services(&remoteAddr);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::GetRemoteServices,
                            status);
  }
}



void
BluetoothInterface::StartDiscovery(BluetoothResultHandler* aRes)
{
  int status = mInterface->start_discovery();

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::StartDiscovery,
                            status);
  }
}

void
BluetoothInterface::CancelDiscovery(BluetoothResultHandler* aRes)
{
  int status = mInterface->cancel_discovery();

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::CancelDiscovery,
                            status);
  }
}



void
BluetoothInterface::CreateBond(const nsAString& aBdAddr,
                               BluetoothResultHandler* aRes)
{
  bt_bdaddr_t bdAddr;
  int status;

  if (NS_SUCCEEDED(Convert(aBdAddr, bdAddr))) {
    status = mInterface->create_bond(&bdAddr);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::CreateBond,
                            status);
  }
}

void
BluetoothInterface::RemoveBond(const nsAString& aBdAddr,
                               BluetoothResultHandler* aRes)
{
  bt_bdaddr_t bdAddr;
  int status;

  if (NS_SUCCEEDED(Convert(aBdAddr, bdAddr))) {
    status = mInterface->remove_bond(&bdAddr);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::RemoveBond,
                            status);
  }
}

void
BluetoothInterface::CancelBond(const nsAString& aBdAddr,
                               BluetoothResultHandler* aRes)
{
  bt_bdaddr_t bdAddr;
  int status;

  if (NS_SUCCEEDED(Convert(aBdAddr, bdAddr))) {
    status = mInterface->cancel_bond(&bdAddr);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::CancelBond,
                            status);
  }
}



void
BluetoothInterface::PinReply(const nsAString& aBdAddr, bool aAccept,
                             const nsAString& aPinCode,
                             BluetoothResultHandler* aRes)
{
  int status;
  bt_bdaddr_t bdAddr;
  uint8_t accept;
  bt_pin_code_t pinCode;

  if (NS_SUCCEEDED(Convert(aBdAddr, bdAddr)) &&
      NS_SUCCEEDED(Convert(aAccept, accept)) &&
      NS_SUCCEEDED(Convert(aPinCode, pinCode))) {
    status = mInterface->pin_reply(&bdAddr, accept, aPinCode.Length(),
                                   &pinCode);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::PinReply,
                            status);
  }
}

void
BluetoothInterface::SspReply(const nsAString& aBdAddr,
                             const nsAString& aVariant,
                             bool aAccept, uint32_t aPasskey,
                             BluetoothResultHandler* aRes)
{
  int status;
  bt_bdaddr_t bdAddr;
  bt_ssp_variant_t variant;
  uint8_t accept;

  if (NS_SUCCEEDED(Convert(aBdAddr, bdAddr)) &&
      NS_SUCCEEDED(Convert(aVariant, variant)) &&
      NS_SUCCEEDED(Convert(aAccept, accept))) {
    status = mInterface->ssp_reply(&bdAddr, variant, accept, aPasskey);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::SspReply,
                            status);
  }
}



void
BluetoothInterface::DutModeConfigure(bool aEnable,
                                     BluetoothResultHandler* aRes)
{
  int status;
  uint8_t enable;

  if (NS_SUCCEEDED(Convert(aEnable, enable))) {
    status = mInterface->dut_mode_configure(enable);
  } else {
    status = BT_STATUS_PARM_INVALID;
  }

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::DutModeConfigure,
                            status);
  }
}

void
BluetoothInterface::DutModeSend(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                                BluetoothResultHandler* aRes)
{
  int status = mInterface->dut_mode_send(aOpcode, aBuf, aLen);

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::DutModeSend,
                            status);
  }
}



void
BluetoothInterface::LeTestMode(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                               BluetoothResultHandler* aRes)
{
#if ANDROID_VERSION >= 18
  int status = mInterface->le_test_mode(aOpcode, aBuf, aLen);
#else
  int status = BT_STATUS_UNSUPPORTED;
#endif

  if (aRes) {
    DispatchBluetoothResult(aRes,
                            &BluetoothResultHandler::LeTestMode,
                            status);
  }
}



template <class T>
T*
BluetoothInterface::GetProfileInterface()
{
  static T* sBluetoothProfileInterface;

  if (sBluetoothProfileInterface) {
    return sBluetoothProfileInterface;
  }

  typename interface_traits<T>::const_interface_type* interface =
    reinterpret_cast<typename interface_traits<T>::const_interface_type*>(
      mInterface->get_profile_interface(interface_traits<T>::profile_id()));

  if (!interface) {
    BT_WARNING("Bluetooth profile '%s' is not supported",
               interface_traits<T>::profile_id());
    return nullptr;
  }

  if (interface->size != sizeof(*interface)) {
    BT_WARNING("interface of incorrect size");
    return nullptr;
  }

  sBluetoothProfileInterface = new T(interface);

  return sBluetoothProfileInterface;
}

BluetoothSocketInterface*
BluetoothInterface::GetBluetoothSocketInterface()
{
  return GetProfileInterface<BluetoothSocketInterface>();
}

BluetoothHandsfreeInterface*
BluetoothInterface::GetBluetoothHandsfreeInterface()
{
  return GetProfileInterface<BluetoothHandsfreeInterface>();
}

BluetoothA2dpInterface*
BluetoothInterface::GetBluetoothA2dpInterface()
{
  return GetProfileInterface<BluetoothA2dpInterface>();
}

BluetoothAvrcpInterface*
BluetoothInterface::GetBluetoothAvrcpInterface()
{
#if ANDROID_VERSION >= 18
  return GetProfileInterface<BluetoothAvrcpInterface>();
#else
  return nullptr;
#endif
}

END_BLUETOOTH_NAMESPACE
