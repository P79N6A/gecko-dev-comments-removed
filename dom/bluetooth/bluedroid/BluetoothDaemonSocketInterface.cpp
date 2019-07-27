





#include "BluetoothDaemonSocketInterface.h"
#include "BluetoothSocketMessageWatcher.h"
#include "nsXULAppAPI.h"
#include "mozilla/unused.h"

BEGIN_BLUETOOTH_NAMESPACE





const int BluetoothDaemonSocketModule::MAX_NUM_CLIENTS = 1;




nsresult
BluetoothDaemonSocketModule::ListenCmd(BluetoothSocketType aType,
                                       const nsAString& aServiceName,
                                       const uint8_t aServiceUuid[16],
                                       int aChannel, bool aEncrypt,
                                       bool aAuth,
                                       BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(new BluetoothDaemonPDU(0x02, 0x01, 0));

  nsresult rv = PackPDU(
    aType,
    PackConversion<nsAString, BluetoothServiceName>(aServiceName),
    PackArray<uint8_t>(aServiceUuid, 16),
    PackConversion<int, int32_t>(aChannel),
    SocketFlags(aEncrypt, aAuth), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu, aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  unused << pdu.forget();
  return rv;
}

nsresult
BluetoothDaemonSocketModule::ConnectCmd(const nsAString& aBdAddr,
                                        BluetoothSocketType aType,
                                        const uint8_t aUuid[16],
                                        int aChannel, bool aEncrypt,
                                        bool aAuth,
                                        BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(new BluetoothDaemonPDU(0x02, 0x02, 0));

  nsresult rv = PackPDU(
    PackConversion<nsAString, BluetoothAddress>(aBdAddr),
    aType,
    PackArray<uint8_t>(aUuid, 16),
    PackConversion<int, int32_t>(aChannel),
    SocketFlags(aEncrypt, aAuth), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu, aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  unused << pdu.forget();
  return rv;
}



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








class BluetoothDaemonSocketModule::AcceptWatcher MOZ_FINAL
  : public SocketMessageWatcher
{
public:
  AcceptWatcher(int aFd, BluetoothSocketResultHandler* aRes)
  : SocketMessageWatcher(aFd, aRes)
  { }

  void Proceed(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    if (aStatus == STATUS_SUCCESS) {
      IntStringIntResultRunnable::Dispatch(
        GetResultHandler(), &BluetoothSocketResultHandler::Accept,
        ConstantInitOp3<int, nsString, int>(GetClientFd(), GetBdAddress(),
                                            GetConnectionStatus()));
    } else {
      ErrorRunnable::Dispatch(GetResultHandler(),
                              &BluetoothSocketResultHandler::OnError,
                              ConstantInitOp1<BluetoothStatus>(aStatus));
    }

    MessageLoopForIO::current()->PostTask(
      FROM_HERE, new DeleteTask<AcceptWatcher>(this));
  }
};

nsresult
BluetoothDaemonSocketModule::AcceptCmd(int aFd,
                                       BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  Task* t = new SocketMessageWatcherTask(new AcceptWatcher(aFd, aRes));
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, t);

  return NS_OK;
}

nsresult
BluetoothDaemonSocketModule::CloseCmd(BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  Task* t = new DeleteSocketMessageWatcherTask(aRes);
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, t);

  return NS_OK;
}

void
BluetoothDaemonSocketModule::HandleSvc(const BluetoothDaemonPDUHeader& aHeader,
                                       BluetoothDaemonPDU& aPDU,
                                       void* aUserData)
{
  static void (BluetoothDaemonSocketModule::* const HandleRsp[])(
    const BluetoothDaemonPDUHeader&,
    BluetoothDaemonPDU&,
    BluetoothSocketResultHandler*) = {
    INIT_ARRAY_AT(0x00, &BluetoothDaemonSocketModule::ErrorRsp),
    INIT_ARRAY_AT(0x01, &BluetoothDaemonSocketModule::ListenRsp),
    INIT_ARRAY_AT(0x02, &BluetoothDaemonSocketModule::ConnectRsp),
  };

  if (NS_WARN_IF(MOZ_ARRAY_LENGTH(HandleRsp) <= aHeader.mOpcode) ||
      NS_WARN_IF(!HandleRsp[aHeader.mOpcode])) {
    return;
  }

  nsRefPtr<BluetoothSocketResultHandler> res =
    already_AddRefed<BluetoothSocketResultHandler>(
      static_cast<BluetoothSocketResultHandler*>(aUserData));

  if (!res) {
    return; 
  }

  (this->*(HandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
}

nsresult
BluetoothDaemonSocketModule::Send(BluetoothDaemonPDU* aPDU,
                                  BluetoothSocketResultHandler* aRes)
{
  aRes->AddRef(); 
  return Send(aPDU, static_cast<void*>(aRes));
}

uint8_t
BluetoothDaemonSocketModule::SocketFlags(bool aEncrypt, bool aAuth)
{
  return (0x01 * aEncrypt) | (0x02 * aAuth);
}




void
BluetoothDaemonSocketModule::ErrorRsp(const BluetoothDaemonPDUHeader& aHeader,
                                      BluetoothDaemonPDU& aPDU,
                                      BluetoothSocketResultHandler* aRes)
{
  ErrorRunnable::Dispatch(
    aRes, &BluetoothSocketResultHandler::OnError, UnpackPDUInitOp(aPDU));
}

class BluetoothDaemonSocketModule::ListenInitOp MOZ_FINAL : private PDUInitOp
{
public:
  ListenInitOp(BluetoothDaemonPDU& aPDU)
  : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (int& aArg1) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    aArg1 = pdu.AcquireFd();

    if (NS_WARN_IF(aArg1 < 0)) {
      return NS_ERROR_ILLEGAL_VALUE;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonSocketModule::ListenRsp(const BluetoothDaemonPDUHeader& aHeader,
                                       BluetoothDaemonPDU& aPDU,
                                       BluetoothSocketResultHandler* aRes)
{
  IntResultRunnable::Dispatch(
    aRes, &BluetoothSocketResultHandler::Listen, ListenInitOp(aPDU));
}






class BluetoothDaemonSocketModule::ConnectWatcher MOZ_FINAL
  : public SocketMessageWatcher
{
public:
  ConnectWatcher(int aFd, BluetoothSocketResultHandler* aRes)
  : SocketMessageWatcher(aFd, aRes)
  { }

  void Proceed(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    if (aStatus == STATUS_SUCCESS) {
      IntStringIntResultRunnable::Dispatch(
        GetResultHandler(), &BluetoothSocketResultHandler::Connect,
        ConstantInitOp3<int, nsString, int>(GetFd(), GetBdAddress(),
                                            GetConnectionStatus()));
    } else {
      ErrorRunnable::Dispatch(GetResultHandler(),
                              &BluetoothSocketResultHandler::OnError,
                              ConstantInitOp1<BluetoothStatus>(aStatus));
    }

    MessageLoopForIO::current()->PostTask(
      FROM_HERE, new DeleteTask<ConnectWatcher>(this));
  }
};

void
BluetoothDaemonSocketModule::ConnectRsp(const BluetoothDaemonPDUHeader& aHeader,
                                        BluetoothDaemonPDU& aPDU,
                                        BluetoothSocketResultHandler* aRes)
{
  
  int fd = aPDU.AcquireFd();
  if (fd < 0) {
    ErrorRunnable::Dispatch(aRes, &BluetoothSocketResultHandler::OnError,
                            ConstantInitOp1<BluetoothStatus>(STATUS_FAIL));
    return;
  }

  
  Task* t = new SocketMessageWatcherTask(new ConnectWatcher(fd, aRes));
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, t);
}





BluetoothDaemonSocketInterface::BluetoothDaemonSocketInterface(
  BluetoothDaemonSocketModule* aModule)
: mModule(aModule)
{
  MOZ_ASSERT(mModule);
}

BluetoothDaemonSocketInterface::~BluetoothDaemonSocketInterface()
{ }

void
BluetoothDaemonSocketInterface::Listen(BluetoothSocketType aType,
                                       const nsAString& aServiceName,
                                       const uint8_t aServiceUuid[16],
                                       int aChannel, bool aEncrypt,
                                       bool aAuth,
                                       BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ListenCmd(aType, aServiceName, aServiceUuid,
                                   aChannel, aEncrypt, aAuth, aRes);
  if (NS_FAILED(rv))  {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonSocketInterface::Connect(const nsAString& aBdAddr,
                                        BluetoothSocketType aType,
                                        const uint8_t aUuid[16],
                                        int aChannel, bool aEncrypt,
                                        bool aAuth,
                                        BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ConnectCmd(aBdAddr, aType, aUuid, aChannel,
                                    aEncrypt, aAuth, aRes);
  if (NS_FAILED(rv))  {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonSocketInterface::Accept(int aFd,
                                    BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->AcceptCmd(aFd, aRes);
  if (NS_FAILED(rv))  {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonSocketInterface::Close(BluetoothSocketResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->CloseCmd(aRes);
  if (NS_FAILED(rv))  {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonSocketInterface::DispatchError(
  BluetoothSocketResultHandler* aRes, BluetoothStatus aStatus)
{
  BluetoothResultRunnable1<BluetoothSocketResultHandler, void,
                           BluetoothStatus, BluetoothStatus>::Dispatch(
    aRes, &BluetoothSocketResultHandler::OnError,
    ConstantInitOp1<BluetoothStatus>(aStatus));
}

void
BluetoothDaemonSocketInterface::DispatchError(
  BluetoothSocketResultHandler* aRes, nsresult aRv)
{
  BluetoothStatus status;

  if (NS_WARN_IF(NS_FAILED(Convert(aRv, status)))) {
    status = STATUS_FAIL;
  }
  DispatchError(aRes, status);
}

END_BLUETOOTH_NAMESPACE
