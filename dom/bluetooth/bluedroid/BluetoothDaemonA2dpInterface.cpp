





#include "BluetoothDaemonA2dpInterface.h"
#include "mozilla/unused.h"

BEGIN_BLUETOOTH_NAMESPACE





BluetoothA2dpNotificationHandler*
  BluetoothDaemonA2dpModule::sNotificationHandler;

void
BluetoothDaemonA2dpModule::SetNotificationHandler(
  BluetoothA2dpNotificationHandler* aNotificationHandler)
{
  sNotificationHandler = aNotificationHandler;
}

nsresult
BluetoothDaemonA2dpModule::Send(BluetoothDaemonPDU* aPDU,
                                BluetoothA2dpResultHandler* aRes)
{
  aRes->AddRef(); 
  return Send(aPDU, static_cast<void*>(aRes));
}

void
BluetoothDaemonA2dpModule::HandleSvc(const BluetoothDaemonPDUHeader& aHeader,
                                     BluetoothDaemonPDU& aPDU, void* aUserData)
{
  static void (BluetoothDaemonA2dpModule::* const HandleOp[])(
    const BluetoothDaemonPDUHeader&, BluetoothDaemonPDU&, void*) = {
    INIT_ARRAY_AT(0, &BluetoothDaemonA2dpModule::HandleRsp),
    INIT_ARRAY_AT(1, &BluetoothDaemonA2dpModule::HandleNtf),
  };

  MOZ_ASSERT(!NS_IsMainThread());

  
  unsigned int isNtf = !!(aHeader.mOpcode & 0x80);

  (this->*(HandleOp[isNtf]))(aHeader, aPDU, aUserData);
}




nsresult
BluetoothDaemonA2dpModule::ConnectCmd(
  const nsAString& aRemoteAddr, BluetoothA2dpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(new BluetoothDaemonPDU(SERVICE_ID,
                                                           OPCODE_CONNECT,
                                                           6)); 
  nsresult rv = PackPDU(
    PackConversion<nsAString, BluetoothAddress>(aRemoteAddr), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu, aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  unused << pdu.forget();
  return NS_OK;
}

nsresult
BluetoothDaemonA2dpModule::DisconnectCmd(
  const nsAString& aRemoteAddr, BluetoothA2dpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(new BluetoothDaemonPDU(SERVICE_ID,
                                                           OPCODE_DISCONNECT,
                                                           6)); 
  nsresult rv = PackPDU(
    PackConversion<nsAString, BluetoothAddress>(aRemoteAddr), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu, aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  unused << pdu.forget();
  return NS_OK;
}




void
BluetoothDaemonA2dpModule::ErrorRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothA2dpResultHandler* aRes)
{
  ErrorRunnable::Dispatch(
    aRes, &BluetoothA2dpResultHandler::OnError, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonA2dpModule::ConnectRsp(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU,
  BluetoothA2dpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothA2dpResultHandler::Connect, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonA2dpModule::DisconnectRsp(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU,
  BluetoothA2dpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothA2dpResultHandler::Disconnect, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonA2dpModule::HandleRsp(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU,
  void* aUserData)
{
  static void (BluetoothDaemonA2dpModule::* const HandleRsp[])(
    const BluetoothDaemonPDUHeader&,
    BluetoothDaemonPDU&,
    BluetoothA2dpResultHandler*) = {
    INIT_ARRAY_AT(OPCODE_ERROR,
      &BluetoothDaemonA2dpModule::ErrorRsp),
    INIT_ARRAY_AT(OPCODE_CONNECT,
      &BluetoothDaemonA2dpModule::ConnectRsp),
    INIT_ARRAY_AT(OPCODE_DISCONNECT,
      &BluetoothDaemonA2dpModule::DisconnectRsp),
  };

  MOZ_ASSERT(!NS_IsMainThread()); 

  if (NS_WARN_IF(!(aHeader.mOpcode < MOZ_ARRAY_LENGTH(HandleRsp))) ||
      NS_WARN_IF(!HandleRsp[aHeader.mOpcode])) {
    return;
  }

  nsRefPtr<BluetoothA2dpResultHandler> res =
    already_AddRefed<BluetoothA2dpResultHandler>(
      static_cast<BluetoothA2dpResultHandler*>(aUserData));

  if (!res) {
    return; 
  }

  (this->*(HandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
}





class BluetoothDaemonA2dpModule::NotificationHandlerWrapper MOZ_FINAL
{
public:
  typedef BluetoothA2dpNotificationHandler ObjectType;

  static ObjectType* GetInstance()
  {
    MOZ_ASSERT(NS_IsMainThread());

    return sNotificationHandler;
  }
};


class BluetoothDaemonA2dpModule::ConnectionStateInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  ConnectionStateInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (BluetoothA2dpConnectionState& aArg1, nsString& aArg2) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(
      pdu, UnpackConversion<BluetoothAddress, nsAString>(aArg2));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonA2dpModule::ConnectionStateNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  ConnectionStateNotification::Dispatch(
    &BluetoothA2dpNotificationHandler::ConnectionStateNotification,
    ConnectionStateInitOp(aPDU));
}


class BluetoothDaemonA2dpModule::AudioStateInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  AudioStateInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (BluetoothA2dpAudioState& aArg1,
               nsString& aArg2) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(
      pdu, UnpackConversion<BluetoothAddress, nsAString>(aArg2));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonA2dpModule::AudioStateNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  AudioStateNotification::Dispatch(
    &BluetoothA2dpNotificationHandler::AudioStateNotification,
    AudioStateInitOp(aPDU));
}

void
BluetoothDaemonA2dpModule::HandleNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU,
  void* aUserData)
{
  static void (BluetoothDaemonA2dpModule::* const HandleNtf[])(
    const BluetoothDaemonPDUHeader&, BluetoothDaemonPDU&) = {
    INIT_ARRAY_AT(0, &BluetoothDaemonA2dpModule::ConnectionStateNtf),
    INIT_ARRAY_AT(1, &BluetoothDaemonA2dpModule::AudioStateNtf),
  };

  MOZ_ASSERT(!NS_IsMainThread());

  uint8_t index = aHeader.mOpcode - 0x81;

  if (NS_WARN_IF(!(index < MOZ_ARRAY_LENGTH(HandleNtf))) ||
      NS_WARN_IF(!HandleNtf[index])) {
    return;
  }

  (this->*(HandleNtf[index]))(aHeader, aPDU);
}

END_BLUETOOTH_NAMESPACE
