





#include "BluetoothDaemonInterface.h"
#include "BluetoothDaemonHelpers.h"
#include "BluetoothDaemonSetupInterface.h"
#include "mozilla/unused.h"

using namespace mozilla::ipc;

BEGIN_BLUETOOTH_NAMESPACE

template<class T>
struct interface_traits
{ };





class BluetoothDaemonSetupModule
{
public:
  virtual nsresult Send(BluetoothDaemonPDU* aPDU, void* aUserData) = 0;

  
  

  nsresult RegisterModuleCmd(uint8_t aId, uint8_t aMode,
                             BluetoothSetupResultHandler* aRes)
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsAutoPtr<BluetoothDaemonPDU> pdu(new BluetoothDaemonPDU(0x00, 0x01, 0));

    nsresult rv = PackPDU(aId, aMode, *pdu);
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

  nsresult UnregisterModuleCmd(uint8_t aId,
                               BluetoothSetupResultHandler* aRes)
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsAutoPtr<BluetoothDaemonPDU> pdu(new BluetoothDaemonPDU(0x00, 0x02, 0));

    nsresult rv = PackPDU(aId, *pdu);
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

  nsresult ConfigurationCmd(const BluetoothConfigurationParameter* aParam,
                            uint8_t aLen, BluetoothSetupResultHandler* aRes)
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsAutoPtr<BluetoothDaemonPDU> pdu(new BluetoothDaemonPDU(0x00, 0x03, 0));

    nsresult rv = PackPDU(
      aLen, PackArray<BluetoothConfigurationParameter>(aParam, aLen), *pdu);
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

protected:

  
  
  void HandleSvc(const BluetoothDaemonPDUHeader& aHeader,
                 BluetoothDaemonPDU& aPDU, void* aUserData)
  {
    static void (BluetoothDaemonSetupModule::* const HandleRsp[])(
      const BluetoothDaemonPDUHeader&,
      BluetoothDaemonPDU&,
      BluetoothSetupResultHandler*) = {
      [0] = &BluetoothDaemonSetupModule::ErrorRsp,
      [1] = &BluetoothDaemonSetupModule::RegisterModuleRsp,
      [2] = &BluetoothDaemonSetupModule::UnregisterModuleRsp,
      [3] = &BluetoothDaemonSetupModule::ConfigurationRsp
    };

    if (NS_WARN_IF(aHeader.mOpcode >= MOZ_ARRAY_LENGTH(HandleRsp)) ||
        NS_WARN_IF(!HandleRsp[aHeader.mOpcode])) {
      return;
    }

    nsRefPtr<BluetoothSetupResultHandler> res =
      already_AddRefed<BluetoothSetupResultHandler>(
        static_cast<BluetoothSetupResultHandler*>(aUserData));

    if (!res) {
      return; 
    }

    (this->*(HandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
  }

  nsresult Send(BluetoothDaemonPDU* aPDU, BluetoothSetupResultHandler* aRes)
  {
    aRes->AddRef(); 
    return Send(aPDU, static_cast<void*>(aRes));
  }

private:

  
  

  typedef
    BluetoothDaemonInterfaceRunnable0<BluetoothSetupResultHandler, void>
    ResultRunnable;

  typedef
    BluetoothDaemonInterfaceRunnable1<BluetoothSetupResultHandler, void,
                                      BluetoothStatus, BluetoothStatus>
    ErrorRunnable;

  void
  ErrorRsp(const BluetoothDaemonPDUHeader& aHeader,
           BluetoothDaemonPDU& aPDU,
           BluetoothSetupResultHandler* aRes)
  {
    ErrorRunnable::Dispatch<0x00, 0x00>(
      aRes, &BluetoothSetupResultHandler::OnError, aPDU);
  }

  void
  RegisterModuleRsp(const BluetoothDaemonPDUHeader& aHeader,
                    BluetoothDaemonPDU& aPDU,
                    BluetoothSetupResultHandler* aRes)
  {
    ResultRunnable::Dispatch(
      aRes, &BluetoothSetupResultHandler::RegisterModule);
  }

  void
  UnregisterModuleRsp(const BluetoothDaemonPDUHeader& aHeader,
                      BluetoothDaemonPDU& aPDU,
                      BluetoothSetupResultHandler* aRes)
  {
    ResultRunnable::Dispatch(
      aRes, &BluetoothSetupResultHandler::UnregisterModule);
  }

  void
  ConfigurationRsp(const BluetoothDaemonPDUHeader& aHeader,
                   BluetoothDaemonPDU& aPDU,
                   BluetoothSetupResultHandler* aRes)
  {
    ResultRunnable::Dispatch(
      aRes, &BluetoothSetupResultHandler::Configuration);
  }
};





static BluetoothNotificationHandler* sNotificationHandler;





class BluetoothDaemonProtocol MOZ_FINAL
  : public BluetoothDaemonPDUConsumer
  , public BluetoothDaemonSetupModule
{
public:
  BluetoothDaemonProtocol(BluetoothDaemonConnection* aConnection);

  
  

  nsresult Send(BluetoothDaemonPDU* aPDU, void* aUserData) MOZ_OVERRIDE;

  void StoreUserData(const BluetoothDaemonPDU& aPDU) MOZ_OVERRIDE;

  
  

  void Handle(BluetoothDaemonPDU& aPDU) MOZ_OVERRIDE;

  void* FetchUserData(const BluetoothDaemonPDUHeader& aHeader);

private:
  void HandleSetupSvc(const BluetoothDaemonPDUHeader& aHeader,
                      BluetoothDaemonPDU& aPDU, void* aUserData);

  BluetoothDaemonConnection* mConnection;
  nsTArray<void*> mUserDataQ;
};

BluetoothDaemonProtocol::BluetoothDaemonProtocol(
  BluetoothDaemonConnection* aConnection)
  : mConnection(aConnection)
{
  MOZ_ASSERT(mConnection);
}

nsresult
BluetoothDaemonProtocol::Send(BluetoothDaemonPDU* aPDU, void* aUserData)
{
  MOZ_ASSERT(aPDU);

  aPDU->SetUserData(aUserData);
  aPDU->UpdateHeader();
  return mConnection->Send(aPDU); 
}

void
BluetoothDaemonProtocol::HandleSetupSvc(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU,
  void* aUserData)
{
  BluetoothDaemonSetupModule::HandleSvc(aHeader, aPDU, aUserData);
}

void
BluetoothDaemonProtocol::Handle(BluetoothDaemonPDU& aPDU)
{
  static void (BluetoothDaemonProtocol::* const HandleSvc[])(
    const BluetoothDaemonPDUHeader&, BluetoothDaemonPDU&, void*) = {
    [0] = &BluetoothDaemonProtocol::HandleSetupSvc
  };

  BluetoothDaemonPDUHeader header;

  if (NS_FAILED(UnpackPDU(aPDU, header)) ||
      NS_WARN_IF(!(header.mService < MOZ_ARRAY_LENGTH(HandleSvc))) ||
      NS_WARN_IF(!(HandleSvc[header.mService]))) {
    return;
  }

  (this->*(HandleSvc[header.mService]))(header, aPDU, FetchUserData(header));
}

void
BluetoothDaemonProtocol::StoreUserData(const BluetoothDaemonPDU& aPDU)
{
  MOZ_ASSERT(!NS_IsMainThread());

  mUserDataQ.AppendElement(aPDU.GetUserData());
}

void*
BluetoothDaemonProtocol::FetchUserData(const BluetoothDaemonPDUHeader& aHeader)
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (aHeader.mOpcode & 0x80) {
    return nullptr; 
  }

  void* userData = mUserDataQ.ElementAt(0);
  mUserDataQ.RemoveElementAt(0);

  return userData;
}





class BluetoothDaemonChannel MOZ_FINAL : public BluetoothDaemonConnection
{
public:
  BluetoothDaemonChannel(BluetoothDaemonInterface::Channel aChannel);

  nsresult ConnectSocket(BluetoothDaemonInterface* aInterface,
                         BluetoothDaemonPDUConsumer* aConsumer);

  
  

  void OnConnectSuccess() MOZ_OVERRIDE;
  void OnConnectError() MOZ_OVERRIDE;
  void OnDisconnect() MOZ_OVERRIDE;

private:
  BluetoothDaemonInterface* mInterface;
  BluetoothDaemonInterface::Channel mChannel;
};

BluetoothDaemonChannel::BluetoothDaemonChannel(
  BluetoothDaemonInterface::Channel aChannel)
: mInterface(nullptr)
, mChannel(aChannel)
{ }

nsresult
BluetoothDaemonChannel::ConnectSocket(BluetoothDaemonInterface* aInterface,
                                      BluetoothDaemonPDUConsumer* aConsumer)
{
  MOZ_ASSERT(aInterface);

  mInterface = aInterface;

  return BluetoothDaemonConnection::ConnectSocket(aConsumer);
}

void
BluetoothDaemonChannel::OnConnectSuccess()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mInterface);

  mInterface->OnConnectSuccess(mChannel);
}

void
BluetoothDaemonChannel::OnConnectError()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mInterface);

  mInterface->OnConnectError(mChannel);
  mInterface = nullptr;
}

void
BluetoothDaemonChannel::OnDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mInterface);

  mInterface->OnDisconnect(mChannel);
  mInterface = nullptr;
}








#define container(_t, _v, _m) \
  ( (_t*)( ((const unsigned char*)(_v)) - offsetof(_t, _m) ) )

BluetoothDaemonInterface*
BluetoothDaemonInterface::GetInstance()
{
  static BluetoothDaemonInterface* sBluetoothInterface;

  if (sBluetoothInterface) {
    return sBluetoothInterface;
  }

  
  

  BluetoothDaemonChannel* cmdChannel =
    new BluetoothDaemonChannel(BluetoothDaemonInterface::CMD_CHANNEL);

  BluetoothDaemonChannel* ntfChannel =
    new BluetoothDaemonChannel(BluetoothDaemonInterface::NTF_CHANNEL);

  
  

  sBluetoothInterface =
    new BluetoothDaemonInterface(cmdChannel,
                                 ntfChannel,
                                 new BluetoothDaemonProtocol(cmdChannel));

  return sBluetoothInterface;
}

BluetoothDaemonInterface::BluetoothDaemonInterface(
  BluetoothDaemonChannel* aCmdChannel,
  BluetoothDaemonChannel* aNtfChannel,
  BluetoothDaemonProtocol* aProtocol)
: mCmdChannel(aCmdChannel)
, mNtfChannel(aNtfChannel)
, mProtocol(aProtocol)
{
  MOZ_ASSERT(mCmdChannel);
  MOZ_ASSERT(mNtfChannel);
  MOZ_ASSERT(mProtocol);
}

BluetoothDaemonInterface::~BluetoothDaemonInterface()
{ }

class BluetoothDaemonInterface::InitResultHandler MOZ_FINAL
  : public BluetoothSetupResultHandler
{
public:
  InitResultHandler(BluetoothDaemonInterface* aInterface,
                    BluetoothResultHandler* aRes)
    : mInterface(aInterface)
    , mRes(aRes)
    , mRegisteredSocketModule(false)
  {
    MOZ_ASSERT(mInterface);
  }

  
  
  

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    NS_WARNING(__func__);
    

    if (mRes) {
      mRes->OnError(aStatus);
    }
  }

  void RegisterModule() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mInterface->mProtocol);

    NS_WARNING(__func__);
    if (!mRegisteredSocketModule) {
      mRegisteredSocketModule = true;
      
      mInterface->mProtocol->RegisterModuleCmd(0x02, 0x00, this);
    } else if (mRes) {
      
      mRes->Init();
    }
  }

private:
  BluetoothDaemonInterface* mInterface;
  nsRefPtr<BluetoothResultHandler> mRes;
  bool mRegisteredSocketModule;
};

void
BluetoothDaemonInterface::OnConnectSuccess(enum Channel aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mResultHandlerQ.IsEmpty());

  switch (aChannel) {
    case CMD_CHANNEL:
      
      if (mNtfChannel->GetConnectionStatus() != SOCKET_CONNECTED) {
        nsresult rv = mNtfChannel->ConnectSocket(this, mProtocol);
        if (NS_FAILED(rv)) {
          OnConnectError(CMD_CHANNEL);
        }
      } else {
        
        OnConnectSuccess(NTF_CHANNEL);
      }
      break;

    case NTF_CHANNEL: {
        nsRefPtr<BluetoothResultHandler> res = mResultHandlerQ.ElementAt(0);
        mResultHandlerQ.RemoveElementAt(0);

        
        nsresult rv = mProtocol->RegisterModuleCmd(
          0x01, 0x00, new InitResultHandler(this, res));
        if (NS_FAILED(rv) && res) {
          DispatchError(res, STATUS_FAIL);
        }
      }
      break;
  }
}

void
BluetoothDaemonInterface::OnConnectError(enum Channel aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mResultHandlerQ.IsEmpty());

  switch (aChannel) {
    case NTF_CHANNEL:
      
      mCmdChannel->CloseSocket();
    case CMD_CHANNEL: {
        
        nsRefPtr<BluetoothResultHandler> res = mResultHandlerQ.ElementAt(0);
        mResultHandlerQ.RemoveElementAt(0);

        if (res) {
          DispatchError(res, STATUS_FAIL);
        }
      }
      break;
  }
}

void
BluetoothDaemonInterface::OnDisconnect(enum Channel aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mResultHandlerQ.IsEmpty());

  switch (aChannel) {
    case NTF_CHANNEL:
      
      mCmdChannel->CloseSocket();
      break;
    case CMD_CHANNEL: {
        nsRefPtr<BluetoothResultHandler> res = mResultHandlerQ.ElementAt(0);
        mResultHandlerQ.RemoveElementAt(0);

        
        if (res) {
          res->Cleanup();
        }
      }
      break;
  }
}

void
BluetoothDaemonInterface::Init(
  BluetoothNotificationHandler* aNotificationHandler,
  BluetoothResultHandler* aRes)
{
  sNotificationHandler = aNotificationHandler;

  mResultHandlerQ.AppendElement(aRes);

  
  if (mCmdChannel->GetConnectionStatus() != SOCKET_CONNECTED) {
    nsresult rv = mCmdChannel->ConnectSocket(this, mProtocol);
    if (NS_FAILED(rv)) {
      OnConnectError(CMD_CHANNEL);
    }
  } else {
    
    OnConnectSuccess(CMD_CHANNEL);
  }
}

class BluetoothDaemonInterface::CleanupResultHandler MOZ_FINAL
  : public BluetoothSetupResultHandler
{
public:
  CleanupResultHandler(BluetoothDaemonInterface* aInterface)
    : mInterface(aInterface)
    , mUnregisteredCoreModule(false)
  {
    MOZ_ASSERT(mInterface);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    Proceed();
  }

  void UnregisterModule() MOZ_OVERRIDE
  {
    Proceed();
  }

private:
  void Proceed()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mInterface->mProtocol);

    if (!mUnregisteredCoreModule) {
      mUnregisteredCoreModule = true;
      
      mInterface->mProtocol->UnregisterModuleCmd(0x01, this);
    } else {
      
      mInterface->mNtfChannel->CloseSocket();
    }
  }

  BluetoothDaemonInterface* mInterface;
  bool mUnregisteredCoreModule;
};

void
BluetoothDaemonInterface::Cleanup(BluetoothResultHandler* aRes)
{
  sNotificationHandler = nullptr;

  mResultHandlerQ.AppendElement(aRes);

  
  mProtocol->UnregisterModuleCmd(0x02, new CleanupResultHandler(this));
}

void
BluetoothDaemonInterface::Enable(BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::Disable(BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::GetAdapterProperties(BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::GetAdapterProperty(const nsAString& aName,
                                             BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::SetAdapterProperty(
  const BluetoothNamedValue& aProperty, BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::GetRemoteDeviceProperties(
  const nsAString& aRemoteAddr, BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::GetRemoteDeviceProperty(
  const nsAString& aRemoteAddr, const nsAString& aName,
  BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::SetRemoteDeviceProperty(
  const nsAString& aRemoteAddr, const BluetoothNamedValue& aProperty,
  BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::GetRemoteServiceRecord(const nsAString& aRemoteAddr,
                                                 const uint8_t aUuid[16],
                                                 BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::GetRemoteServices(const nsAString& aRemoteAddr,
                                            BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::StartDiscovery(BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::CancelDiscovery(BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::CreateBond(const nsAString& aBdAddr,
                                     BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::RemoveBond(const nsAString& aBdAddr,
                                     BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::CancelBond(const nsAString& aBdAddr,
                                     BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::PinReply(const nsAString& aBdAddr, bool aAccept,
                                   const nsAString& aPinCode,
                                   BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::SspReply(const nsAString& aBdAddr,
                                   const nsAString& aVariant,
                                   bool aAccept, uint32_t aPasskey,
                                   BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::DutModeConfigure(bool aEnable,
                                           BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::DutModeSend(uint16_t aOpcode, uint8_t* aBuf,
                                      uint8_t aLen,
                                      BluetoothResultHandler* aRes)
{
}



void
BluetoothDaemonInterface::LeTestMode(uint16_t aOpcode, uint8_t* aBuf,
                                     uint8_t aLen,
                                     BluetoothResultHandler* aRes)
{
}

void
BluetoothDaemonInterface::DispatchError(BluetoothResultHandler* aRes,
                                        BluetoothStatus aStatus)
{
  BluetoothDaemonInterfaceRunnable1<
    BluetoothResultHandler, void, BluetoothStatus, BluetoothStatus>::Dispatch(
    aRes, &BluetoothResultHandler::OnError, aStatus);
}

BluetoothSocketInterface*
BluetoothDaemonInterface::GetBluetoothSocketInterface()
{
  return nullptr;
}

BluetoothHandsfreeInterface*
BluetoothDaemonInterface::GetBluetoothHandsfreeInterface()
{
  return nullptr;
}

BluetoothA2dpInterface*
BluetoothDaemonInterface::GetBluetoothA2dpInterface()
{
  return nullptr;
}

BluetoothAvrcpInterface*
BluetoothDaemonInterface::GetBluetoothAvrcpInterface()
{
  return nullptr;
}

END_BLUETOOTH_NAMESPACE
