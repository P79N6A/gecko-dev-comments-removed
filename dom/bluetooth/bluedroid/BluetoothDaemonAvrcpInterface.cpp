





#include "BluetoothDaemonAvrcpInterface.h"
#include "BluetoothDaemonSetupInterface.h"
#include "mozilla/unused.h"

BEGIN_BLUETOOTH_NAMESPACE





const int BluetoothDaemonAvrcpModule::MAX_NUM_CLIENTS = 1;

BluetoothAvrcpNotificationHandler*
  BluetoothDaemonAvrcpModule::sNotificationHandler;

void
BluetoothDaemonAvrcpModule::SetNotificationHandler(
  BluetoothAvrcpNotificationHandler* aNotificationHandler)
{
  sNotificationHandler = aNotificationHandler;
}

nsresult
BluetoothDaemonAvrcpModule::Send(BluetoothDaemonPDU* aPDU,
                                 BluetoothAvrcpResultHandler* aRes)
{
  if (aRes) {
    aRes->AddRef(); 
  }
  return Send(aPDU, static_cast<void*>(aRes));
}

void
BluetoothDaemonAvrcpModule::HandleSvc(const BluetoothDaemonPDUHeader& aHeader,
                                      BluetoothDaemonPDU& aPDU, void* aUserData)
{
  static void (BluetoothDaemonAvrcpModule::* const HandleOp[])(
    const BluetoothDaemonPDUHeader&, BluetoothDaemonPDU&, void*) = {
    INIT_ARRAY_AT(0, &BluetoothDaemonAvrcpModule::HandleRsp),
    INIT_ARRAY_AT(1, &BluetoothDaemonAvrcpModule::HandleNtf),
  };

  MOZ_ASSERT(!NS_IsMainThread());

  unsigned int isNtf = !!(aHeader.mOpcode & 0x80);

  (this->*(HandleOp[isNtf]))(aHeader, aPDU, aUserData);
}




nsresult
BluetoothDaemonAvrcpModule::GetPlayStatusRspCmd(
  ControlPlayStatus aPlayStatus, uint32_t aSongLen, uint32_t aSongPos,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_GET_PLAY_STATUS_RSP,
                           1 + 
                           4 + 
                           4)); 

  nsresult rv = PackPDU(aPlayStatus, aSongLen, aSongPos, *pdu);
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
BluetoothDaemonAvrcpModule::ListPlayerAppAttrRspCmd(
  int aNumAttr, const BluetoothAvrcpPlayerAttribute* aPAttrs,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_LIST_PLAYER_APP_ATTR_RSP,
                           1 + 
                           aNumAttr)); 

  nsresult rv = PackPDU(
    PackConversion<int, uint8_t>(aNumAttr),
    PackArray<BluetoothAvrcpPlayerAttribute>(aPAttrs, aNumAttr), *pdu);
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
BluetoothDaemonAvrcpModule::ListPlayerAppValueRspCmd(
  int aNumVal, uint8_t* aPVals, BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_LIST_PLAYER_APP_VALUE_RSP,
                           1 + 
                           aNumVal)); 

  nsresult rv = PackPDU(PackConversion<int, uint8_t>(aNumVal),
                        PackArray<uint8_t>(aPVals, aNumVal), *pdu);
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
BluetoothDaemonAvrcpModule::GetPlayerAppValueRspCmd(
  uint8_t aNumAttrs, const uint8_t* aIds, const uint8_t* aValues,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_GET_PLAYER_APP_VALUE_RSP,
                           1 + 
                           2 * aNumAttrs)); 
  nsresult rv = PackPDU(
    aNumAttrs,
    BluetoothAvrcpAttributeValuePairs(aIds, aValues, aNumAttrs), *pdu);
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
BluetoothDaemonAvrcpModule::GetPlayerAppAttrTextRspCmd(
  int aNumAttr, const uint8_t* aIds, const char** aTexts,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_GET_PLAYER_APP_ATTR_TEXT_RSP,
                           0)); 
  nsresult rv = PackPDU(
    PackConversion<int, uint8_t>(aNumAttr),
    BluetoothAvrcpAttributeTextPairs(aIds, aTexts, aNumAttr), *pdu);
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
BluetoothDaemonAvrcpModule::GetPlayerAppValueTextRspCmd(
  int aNumVal, const uint8_t* aIds, const char** aTexts,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_GET_PLAYER_APP_VALUE_TEXT_RSP,
                           0)); 
  nsresult rv = PackPDU(
    PackConversion<int, uint8_t>(aNumVal),
    BluetoothAvrcpAttributeTextPairs(aIds, aTexts, aNumVal), *pdu);
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
BluetoothDaemonAvrcpModule::GetElementAttrRspCmd(
  uint8_t aNumAttr, const BluetoothAvrcpElementAttribute* aAttr,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_GET_ELEMENT_ATTR_RSP,
                           0)); 
  nsresult rv = PackPDU(
    aNumAttr,
    PackArray<BluetoothAvrcpElementAttribute>(aAttr, aNumAttr), *pdu);
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
BluetoothDaemonAvrcpModule::SetPlayerAppValueRspCmd(
  BluetoothAvrcpStatus aRspStatus, BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_SET_PLAYER_APP_VALUE_RSP,
                           1)); 

  nsresult rv = PackPDU(aRspStatus, *pdu);
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
BluetoothDaemonAvrcpModule::RegisterNotificationRspCmd(
  BluetoothAvrcpEvent aEvent, BluetoothAvrcpNotification aType,
  const BluetoothAvrcpNotificationParam& aParam,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_REGISTER_NOTIFICATION_RSP,
                           1 + 
                           1 + 
                           1 + 
                           256)); 

  nsresult rv = PackPDU(aEvent, aType,
                        BluetoothAvrcpEventParamPair(aEvent, aParam), *pdu);
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
BluetoothDaemonAvrcpModule::SetVolumeCmd(uint8_t aVolume,
                                         BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<BluetoothDaemonPDU> pdu(
    new BluetoothDaemonPDU(SERVICE_ID, OPCODE_SET_VOLUME,
                           1)); 

  nsresult rv = PackPDU(aVolume, *pdu);
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
BluetoothDaemonAvrcpModule::ErrorRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ErrorRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::OnError, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::GetPlayStatusRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::GetPlayStatusRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::ListPlayerAppAttrRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::ListPlayerAppAttrRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::ListPlayerAppValueRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::ListPlayerAppValueRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::GetPlayerAppValueRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::GetPlayerAppValueRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::GetPlayerAppAttrTextRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::GetPlayerAppAttrTextRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::GetPlayerAppValueTextRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::GetPlayerAppValueTextRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::GetElementAttrRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::GetElementAttrRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::SetPlayerAppValueRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::SetPlayerAppValueRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::RegisterNotificationRspRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::RegisterNotificationRsp,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::SetVolumeRsp(
  const BluetoothDaemonPDUHeader& aHeader,
  BluetoothDaemonPDU& aPDU, BluetoothAvrcpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::SetVolume,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::HandleRsp(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU,
  void* aUserData)
{
  static void (BluetoothDaemonAvrcpModule::* const HandleRsp[])(
    const BluetoothDaemonPDUHeader&,
    BluetoothDaemonPDU&,
    BluetoothAvrcpResultHandler*) = {
    INIT_ARRAY_AT(OPCODE_ERROR,
      &BluetoothDaemonAvrcpModule::ErrorRsp),
    INIT_ARRAY_AT(OPCODE_GET_PLAY_STATUS_RSP,
      &BluetoothDaemonAvrcpModule::GetPlayStatusRspRsp),
    INIT_ARRAY_AT(OPCODE_LIST_PLAYER_APP_ATTR_RSP,
      &BluetoothDaemonAvrcpModule::ListPlayerAppAttrRspRsp),
    INIT_ARRAY_AT(OPCODE_LIST_PLAYER_APP_VALUE_RSP,
      &BluetoothDaemonAvrcpModule::ListPlayerAppValueRspRsp),
    INIT_ARRAY_AT(OPCODE_GET_PLAYER_APP_VALUE_RSP,
      &BluetoothDaemonAvrcpModule::GetPlayerAppValueRspRsp),
    INIT_ARRAY_AT(OPCODE_GET_PLAYER_APP_ATTR_TEXT_RSP,
      &BluetoothDaemonAvrcpModule::GetPlayerAppAttrTextRspRsp),
    INIT_ARRAY_AT(OPCODE_GET_PLAYER_APP_VALUE_TEXT_RSP,
      &BluetoothDaemonAvrcpModule::GetPlayerAppValueTextRspRsp),
    INIT_ARRAY_AT(OPCODE_GET_ELEMENT_ATTR_RSP,
      &BluetoothDaemonAvrcpModule::GetElementAttrRspRsp),
    INIT_ARRAY_AT(OPCODE_SET_PLAYER_APP_VALUE_RSP,
      &BluetoothDaemonAvrcpModule::SetPlayerAppValueRspRsp),
    INIT_ARRAY_AT(OPCODE_REGISTER_NOTIFICATION_RSP,
      &BluetoothDaemonAvrcpModule::RegisterNotificationRspRsp),
    INIT_ARRAY_AT(OPCODE_SET_VOLUME,
      &BluetoothDaemonAvrcpModule::SetVolumeRsp)
  };

  MOZ_ASSERT(!NS_IsMainThread()); 

  if (NS_WARN_IF(!(aHeader.mOpcode < MOZ_ARRAY_LENGTH(HandleRsp))) ||
      NS_WARN_IF(!HandleRsp[aHeader.mOpcode])) {
    return;
  }

  nsRefPtr<BluetoothAvrcpResultHandler> res =
    already_AddRefed<BluetoothAvrcpResultHandler>(
      static_cast<BluetoothAvrcpResultHandler*>(aUserData));

  if (!res) {
    return; 
  }

  (this->*(HandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
}





class BluetoothDaemonAvrcpModule::NotificationHandlerWrapper MOZ_FINAL
{
public:
  typedef BluetoothAvrcpNotificationHandler ObjectType;

  static ObjectType* GetInstance()
  {
    MOZ_ASSERT(NS_IsMainThread());

    return sNotificationHandler;
  }
};


class BluetoothDaemonAvrcpModule::RemoteFeatureInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  RemoteFeatureInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (nsString& aArg1, unsigned long& aArg2) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    
    nsresult rv = UnpackPDU(
      pdu, UnpackConversion<BluetoothAddress, nsAString>(aArg1));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(
      pdu,
      UnpackConversion<BluetoothAvrcpRemoteFeature, unsigned long>(aArg2));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonAvrcpModule::RemoteFeatureNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  RemoteFeatureNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::RemoteFeatureNotification,
    RemoteFeatureInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::GetPlayStatusNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  GetPlayStatusNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::GetPlayStatusNotification,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::ListPlayerAppAttrNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  ListPlayerAppAttrNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::ListPlayerAppAttrNotification,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::ListPlayerAppValuesNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  ListPlayerAppValuesNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::ListPlayerAppValuesNotification,
    UnpackPDUInitOp(aPDU));
}


class BluetoothDaemonAvrcpModule::GetPlayerAppValueInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  GetPlayerAppValueInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (uint8_t& aArg1,
               nsAutoArrayPtr<BluetoothAvrcpPlayerAttribute>& aArg2) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(
      pdu, UnpackArray<BluetoothAvrcpPlayerAttribute>(aArg2, aArg1));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonAvrcpModule::GetPlayerAppValueNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  GetPlayerAppValueNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::GetPlayerAppValueNotification,
    GetPlayerAppValueInitOp(aPDU));
}


class BluetoothDaemonAvrcpModule::GetPlayerAppAttrsTextInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  GetPlayerAppAttrsTextInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (uint8_t& aArg1,
               nsAutoArrayPtr<BluetoothAvrcpPlayerAttribute>& aArg2) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(
      pdu, UnpackArray<BluetoothAvrcpPlayerAttribute>(aArg2, aArg1));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonAvrcpModule::GetPlayerAppAttrsTextNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  GetPlayerAppAttrsTextNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::GetPlayerAppAttrsTextNotification,
    GetPlayerAppAttrsTextInitOp(aPDU));
}


class BluetoothDaemonAvrcpModule::GetPlayerAppValuesTextInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  GetPlayerAppValuesTextInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (uint8_t& aArg1, uint8_t& aArg2,
               nsAutoArrayPtr<uint8_t>& aArg3) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(pdu, aArg2);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(pdu, UnpackArray<uint8_t>(aArg3, aArg2));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonAvrcpModule::GetPlayerAppValuesTextNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  GetPlayerAppValuesTextNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::GetPlayerAppValuesTextNotification,
    GetPlayerAppValuesTextInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::SetPlayerAppValueNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  SetPlayerAppValueNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::SetPlayerAppValueNotification,
    UnpackPDUInitOp(aPDU));
}


class BluetoothDaemonAvrcpModule::GetElementAttrInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  GetElementAttrInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (uint8_t& aArg1,
               nsAutoArrayPtr<BluetoothAvrcpMediaAttribute>& aArg2) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = UnpackPDU(
      pdu, UnpackArray<BluetoothAvrcpMediaAttribute>(aArg2, aArg1));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonAvrcpModule::GetElementAttrNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  GetElementAttrNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::GetElementAttrNotification,
    GetElementAttrInitOp(aPDU));
}

void
BluetoothDaemonAvrcpModule::RegisterNotificationNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  RegisterNotificationNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::RegisterNotificationNotification,
    UnpackPDUInitOp(aPDU));
}

#if ANDROID_VERSION >= 19
void
BluetoothDaemonAvrcpModule::VolumeChangeNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  VolumeChangeNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::VolumeChangeNotification,
    UnpackPDUInitOp(aPDU));
}


class BluetoothDaemonAvrcpModule::PassthroughCmdInitOp MOZ_FINAL
  : private PDUInitOp
{
public:
  PassthroughCmdInitOp(BluetoothDaemonPDU& aPDU)
    : PDUInitOp(aPDU)
  { }

  nsresult
  operator () (int& aArg1, int& aArg2) const
  {
    BluetoothDaemonPDU& pdu = GetPDU();

    nsresult rv = UnpackPDU(pdu, UnpackConversion<uint8_t, int>(aArg1));
    if (NS_FAILED(rv)) {
      return rv;
    }
    rv = UnpackPDU(pdu, UnpackConversion<uint8_t, int>(aArg2));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void
BluetoothDaemonAvrcpModule::PassthroughCmdNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU)
{
  PassthroughCmdNotification::Dispatch(
    &BluetoothAvrcpNotificationHandler::PassthroughCmdNotification,
    PassthroughCmdInitOp(aPDU));
}
#endif

void
BluetoothDaemonAvrcpModule::HandleNtf(
  const BluetoothDaemonPDUHeader& aHeader, BluetoothDaemonPDU& aPDU,
  void* aUserData)
{
  static void (BluetoothDaemonAvrcpModule::* const HandleNtf[])(
    const BluetoothDaemonPDUHeader&, BluetoothDaemonPDU&) = {
#if ANDROID_VERSION >= 19
    INIT_ARRAY_AT(0, &BluetoothDaemonAvrcpModule::RemoteFeatureNtf),
    INIT_ARRAY_AT(1, &BluetoothDaemonAvrcpModule::GetPlayStatusNtf),
    INIT_ARRAY_AT(2, &BluetoothDaemonAvrcpModule::ListPlayerAppAttrNtf),
    INIT_ARRAY_AT(3, &BluetoothDaemonAvrcpModule::ListPlayerAppValuesNtf),
    INIT_ARRAY_AT(4, &BluetoothDaemonAvrcpModule::GetPlayerAppValueNtf),
    INIT_ARRAY_AT(5, &BluetoothDaemonAvrcpModule::GetPlayerAppAttrsTextNtf),
    INIT_ARRAY_AT(6, &BluetoothDaemonAvrcpModule::GetPlayerAppValuesTextNtf),
    INIT_ARRAY_AT(7, &BluetoothDaemonAvrcpModule::SetPlayerAppValueNtf),
    INIT_ARRAY_AT(8, &BluetoothDaemonAvrcpModule::GetElementAttrNtf),
    INIT_ARRAY_AT(9, &BluetoothDaemonAvrcpModule::RegisterNotificationNtf),
    INIT_ARRAY_AT(10, &BluetoothDaemonAvrcpModule::VolumeChangeNtf),
    INIT_ARRAY_AT(11, &BluetoothDaemonAvrcpModule::PassthroughCmdNtf)
#else
    INIT_ARRAY_AT(0, &BluetoothDaemonAvrcpModule::GetPlayStatusNtf),
    INIT_ARRAY_AT(1, &BluetoothDaemonAvrcpModule::ListPlayerAppAttrNtf),
    INIT_ARRAY_AT(2, &BluetoothDaemonAvrcpModule::ListPlayerAppValuesNtf),
    INIT_ARRAY_AT(3, &BluetoothDaemonAvrcpModule::GetPlayerAppValueNtf),
    INIT_ARRAY_AT(4, &BluetoothDaemonAvrcpModule::GetPlayerAppAttrsTextNtf),
    INIT_ARRAY_AT(5, &BluetoothDaemonAvrcpModule::GetPlayerAppValuesTextNtf),
    INIT_ARRAY_AT(6, &BluetoothDaemonAvrcpModule::SetPlayerAppValueNtf),
    INIT_ARRAY_AT(7, &BluetoothDaemonAvrcpModule::GetElementAttrNtf),
    INIT_ARRAY_AT(8, &BluetoothDaemonAvrcpModule::RegisterNotificationNtf)
#endif
  };

  MOZ_ASSERT(!NS_IsMainThread());

  uint8_t index = aHeader.mOpcode - 0x81;

  if (NS_WARN_IF(!(index < MOZ_ARRAY_LENGTH(HandleNtf))) ||
      NS_WARN_IF(!HandleNtf[index])) {
    return;
  }

  (this->*(HandleNtf[index]))(aHeader, aPDU);
}





BluetoothDaemonAvrcpInterface::BluetoothDaemonAvrcpInterface(
  BluetoothDaemonAvrcpModule* aModule)
  : mModule(aModule)
{ }

BluetoothDaemonAvrcpInterface::~BluetoothDaemonAvrcpInterface()
{ }

class BluetoothDaemonAvrcpInterface::InitResultHandler MOZ_FINAL
  : public BluetoothSetupResultHandler
{
public:
  InitResultHandler(BluetoothAvrcpResultHandler* aRes)
    : mRes(aRes)
  {
    MOZ_ASSERT(mRes);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    mRes->OnError(aStatus);
  }

  void RegisterModule() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    mRes->Init();
  }

private:
  nsRefPtr<BluetoothAvrcpResultHandler> mRes;
};

void
BluetoothDaemonAvrcpInterface::Init(
  BluetoothAvrcpNotificationHandler* aNotificationHandler,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  
  
  mModule->SetNotificationHandler(aNotificationHandler);

  InitResultHandler* res;

  if (aRes) {
    res = new InitResultHandler(aRes);
  } else {
    
    res = nullptr;
  }

  nsresult rv = mModule->RegisterModule(
    BluetoothDaemonAvrcpModule::SERVICE_ID,
    BluetoothDaemonAvrcpModule::MAX_NUM_CLIENTS, 0x00, res);

  if (NS_FAILED(rv) && aRes) {
    DispatchError(aRes, rv);
  }
}

class BluetoothDaemonAvrcpInterface::CleanupResultHandler MOZ_FINAL
  : public BluetoothSetupResultHandler
{
public:
  CleanupResultHandler(BluetoothDaemonAvrcpModule* aModule,
                       BluetoothAvrcpResultHandler* aRes)
    : mModule(aModule)
    , mRes(aRes)
  {
    MOZ_ASSERT(mModule);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mRes) {
      mRes->OnError(aStatus);
    }
  }

  void UnregisterModule() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    
    
    mModule->SetNotificationHandler(nullptr);

    if (mRes) {
      mRes->Cleanup();
    }
  }

private:
  BluetoothDaemonAvrcpModule* mModule;
  nsRefPtr<BluetoothAvrcpResultHandler> mRes;
};

void
BluetoothDaemonAvrcpInterface::Cleanup(
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->UnregisterModule(
    BluetoothDaemonAvrcpModule::SERVICE_ID,
    new CleanupResultHandler(mModule, aRes));
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::GetPlayStatusRsp(
  ControlPlayStatus aPlayStatus, uint32_t aSongLen, uint32_t aSongPos,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->GetPlayStatusRspCmd(aPlayStatus, aSongLen,
                                             aSongPos, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::ListPlayerAppAttrRsp(
  int aNumAttr, const BluetoothAvrcpPlayerAttribute* aPAttrs,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ListPlayerAppAttrRspCmd(aNumAttr, aPAttrs, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::ListPlayerAppValueRsp(
  int aNumVal, uint8_t* aPVals, BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ListPlayerAppValueRspCmd(aNumVal, aPVals, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::GetPlayerAppValueRsp(
  uint8_t aNumAttrs, const uint8_t* aIds, const uint8_t* aValues,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->GetPlayerAppValueRspCmd(aNumAttrs, aIds,
                                                 aValues, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::GetPlayerAppAttrTextRsp(
  int aNumAttr, const uint8_t* aIds, const char** aTexts,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->GetPlayerAppAttrTextRspCmd(aNumAttr, aIds,
                                                    aTexts, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::GetPlayerAppValueTextRsp(
  int aNumVal, const uint8_t* aIds, const char** aTexts,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->GetPlayerAppValueTextRspCmd(aNumVal, aIds,
                                                     aTexts, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::GetElementAttrRsp(
  uint8_t aNumAttr, const BluetoothAvrcpElementAttribute* aAttr,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->GetElementAttrRspCmd(aNumAttr, aAttr, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::SetPlayerAppValueRsp(
  BluetoothAvrcpStatus aRspStatus, BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->SetPlayerAppValueRspCmd(aRspStatus, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::RegisterNotificationRsp(
  BluetoothAvrcpEvent aEvent,
  BluetoothAvrcpNotification aType,
  const BluetoothAvrcpNotificationParam& aParam,
  BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->RegisterNotificationRspCmd(aEvent, aType,
                                                    aParam, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::SetVolume(
  uint8_t aVolume, BluetoothAvrcpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->SetVolumeCmd(aVolume, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonAvrcpInterface::DispatchError(
  BluetoothAvrcpResultHandler* aRes, BluetoothStatus aStatus)
{
  BluetoothResultRunnable1<BluetoothAvrcpResultHandler, void,
                           BluetoothStatus, BluetoothStatus>::Dispatch(
    aRes, &BluetoothAvrcpResultHandler::OnError,
    ConstantInitOp1<BluetoothStatus>(aStatus));
}

void
BluetoothDaemonAvrcpInterface::DispatchError(
  BluetoothAvrcpResultHandler* aRes, nsresult aRv)
{
  BluetoothStatus status;

  if (NS_WARN_IF(NS_FAILED(Convert(aRv, status)))) {
    status = STATUS_FAIL;
  }
  DispatchError(aRes, status);
}

END_BLUETOOTH_NAMESPACE
