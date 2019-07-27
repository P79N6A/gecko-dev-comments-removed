





#include "base/basictypes.h"

#include "BluetoothA2dpManager.h"
#include "BluetoothCommon.h"
#include "BluetoothService.h"
#include "BluetoothSocket.h"
#include "BluetoothUtils.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "MainThreadUtils.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"

using namespace mozilla;
USING_BLUETOOTH_NAMESPACE

#define AVRC_ID_REWIND  0x48
#define AVRC_ID_FAST_FOR 0x49
#define AVRC_KEY_PRESS_STATE  1
#define AVRC_KEY_RELEASE_STATE  0

#define AVRC_MAX_ATTR_STR_LEN 255

namespace {
  StaticRefPtr<BluetoothA2dpManager> sBluetoothA2dpManager;
  bool sInShutdown = false;
  static BluetoothA2dpInterface* sBtA2dpInterface;
  static BluetoothAvrcpInterface* sBtAvrcpInterface;
} 




static void
ConvertAttributeString(BluetoothAvrcpMediaAttribute aAttrId,
                       nsAString& aAttrStr)
{
  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  NS_ENSURE_TRUE_VOID(a2dp);

  switch (aAttrId) {
    case AVRCP_MEDIA_ATTRIBUTE_TITLE:
      a2dp->GetTitle(aAttrStr);
      


      if (aAttrStr.Length() >= AVRC_MAX_ATTR_STR_LEN) {
        aAttrStr.Truncate(AVRC_MAX_ATTR_STR_LEN - 1);
        BT_WARNING("Truncate media item attribute title, length is over 255");
      }
      break;
    case AVRCP_MEDIA_ATTRIBUTE_ARTIST:
      a2dp->GetArtist(aAttrStr);
      if (aAttrStr.Length() >= AVRC_MAX_ATTR_STR_LEN) {
        aAttrStr.Truncate(AVRC_MAX_ATTR_STR_LEN - 1);
        BT_WARNING("Truncate media item attribute artist, length is over 255");
      }
      break;
    case AVRCP_MEDIA_ATTRIBUTE_ALBUM:
      a2dp->GetAlbum(aAttrStr);
      if (aAttrStr.Length() >= AVRC_MAX_ATTR_STR_LEN) {
        aAttrStr.Truncate(AVRC_MAX_ATTR_STR_LEN - 1);
        BT_WARNING("Truncate media item attribute album, length is over 255");
      }
      break;
    case AVRCP_MEDIA_ATTRIBUTE_TRACK_NUM:
      aAttrStr.AppendInt(a2dp->GetMediaNumber());
      break;
    case AVRCP_MEDIA_ATTRIBUTE_NUM_TRACKS:
      aAttrStr.AppendInt(a2dp->GetTotalMediaNumber());
      break;
    case AVRCP_MEDIA_ATTRIBUTE_GENRE:
      
      aAttrStr.Truncate();
      break;
    case AVRCP_MEDIA_ATTRIBUTE_PLAYING_TIME:
      aAttrStr.AppendInt(a2dp->GetDuration());
      break;
  }
}

NS_IMETHODIMP
BluetoothA2dpManager::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const char16_t* aData)
{
  MOZ_ASSERT(sBluetoothA2dpManager);

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    HandleShutdown();
    return NS_OK;
  }

  MOZ_ASSERT(false, "BluetoothA2dpManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

BluetoothA2dpManager::BluetoothA2dpManager()
{
  Reset();
}

void
BluetoothA2dpManager::Reset()
{
  ResetA2dp();
  ResetAvrcp();
}

static void
AvStatusToSinkString(BluetoothA2dpConnectionState aState, nsAString& aString)
{
  switch (aState) {
    case A2DP_CONNECTION_STATE_DISCONNECTED:
      aString.AssignLiteral("disconnected");
      break;
    case A2DP_CONNECTION_STATE_CONNECTING:
      aString.AssignLiteral("connecting");
      break;
    case A2DP_CONNECTION_STATE_CONNECTED:
      aString.AssignLiteral("connected");
      break;
    case A2DP_CONNECTION_STATE_DISCONNECTING:
      aString.AssignLiteral("disconnecting");
      break;
    default:
      BT_WARNING("Unknown sink state %d", static_cast<int>(aState));
      return;
  }
}

class BluetoothA2dpManager::InitAvrcpResultHandler final
  : public BluetoothAvrcpResultHandler
{
public:
  InitAvrcpResultHandler(BluetoothProfileResultHandler* aRes)
    : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothAvrcpInterface::Init failed: %d",
               (int)aStatus);
    if (mRes) {
      if (aStatus == STATUS_UNSUPPORTED) {
        



        mRes->Init();
      } else {
        mRes->OnError(NS_ERROR_FAILURE);
      }
    }
  }

  void Init() override
  {
    if (mRes) {
      mRes->Init();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class BluetoothA2dpManager::InitA2dpResultHandler final
  : public BluetoothA2dpResultHandler
{
public:
  InitA2dpResultHandler(BluetoothProfileResultHandler* aRes)
    : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothA2dpInterface::Init failed: %d",
               (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Init() override
  {
    BluetoothInterface* btInf = BluetoothInterface::GetInstance();
    if (NS_WARN_IF(!btInf)) {
      mRes->OnError(NS_ERROR_FAILURE);
      return;
    }

    sBtAvrcpInterface = btInf->GetBluetoothAvrcpInterface();
    if (NS_WARN_IF(!sBtAvrcpInterface)) {
      mRes->OnError(NS_ERROR_FAILURE);
      return;
    }

    BluetoothA2dpManager* a2dpManager = BluetoothA2dpManager::Get();
    sBtAvrcpInterface->Init(a2dpManager, new InitAvrcpResultHandler(mRes));
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class BluetoothA2dpManager::OnErrorProfileResultHandlerRunnable final
  : public nsRunnable
{
public:
  OnErrorProfileResultHandlerRunnable(BluetoothProfileResultHandler* aRes,
                                      nsresult aRv)
    : mRes(aRes)
    , mRv(aRv)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() override
  {
    mRes->OnError(mRv);
    return NS_OK;
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
  nsresult mRv;
};








void
BluetoothA2dpManager::InitA2dpInterface(BluetoothProfileResultHandler* aRes)
{
  BluetoothInterface* btInf = BluetoothInterface::GetInstance();
  if (NS_WARN_IF(!btInf)) {
    
    
    nsRefPtr<nsRunnable> r =
      new OnErrorProfileResultHandlerRunnable(aRes, NS_ERROR_FAILURE);
    if (NS_FAILED(NS_DispatchToMainThread(r))) {
      BT_LOGR("Failed to dispatch HFP OnError runnable");
    }
    return;
  }

  sBtA2dpInterface = btInf->GetBluetoothA2dpInterface();
  if (NS_WARN_IF(!sBtA2dpInterface)) {
    
    
    nsRefPtr<nsRunnable> r =
      new OnErrorProfileResultHandlerRunnable(aRes, NS_ERROR_FAILURE);
    if (NS_FAILED(NS_DispatchToMainThread(r))) {
      BT_LOGR("Failed to dispatch HFP OnError runnable");
    }
    return;
  }

  BluetoothA2dpManager* a2dpManager = BluetoothA2dpManager::Get();
  sBtA2dpInterface->Init(a2dpManager, new InitA2dpResultHandler(aRes));
}

BluetoothA2dpManager::~BluetoothA2dpManager()
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);
  if (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID))) {
    BT_WARNING("Failed to remove shutdown observer!");
  }
}

void
BluetoothA2dpManager::ResetA2dp()
{
  mA2dpConnected = false;
  mSinkState = SinkState::SINK_DISCONNECTED;
  mController = nullptr;
}

void
BluetoothA2dpManager::ResetAvrcp()
{
  mAvrcpConnected = false;
  mDuration = 0;
  mMediaNumber = 0;
  mTotalMediaCount = 0;
  mPosition = 0;
#ifdef MOZ_B2G_BT_API_V2
  mPlayStatus = ControlPlayStatus::PLAYSTATUS_UNKNOWN;
#else
  mPlayStatus = ControlPlayStatus::PLAYSTATUS_STOPPED;
#endif
}





static BluetoothA2dpManager::SinkState
StatusStringToSinkState(const nsAString& aStatus)
{
  BluetoothA2dpManager::SinkState state =
    BluetoothA2dpManager::SinkState::SINK_UNKNOWN;
  if (aStatus.EqualsLiteral("disconnected")) {
    state = BluetoothA2dpManager::SinkState::SINK_DISCONNECTED;
  } else if (aStatus.EqualsLiteral("connecting")) {
    state = BluetoothA2dpManager::SinkState::SINK_CONNECTING;
  } else if (aStatus.EqualsLiteral("connected")) {
    state = BluetoothA2dpManager::SinkState::SINK_CONNECTED;
  } else if (aStatus.EqualsLiteral("playing")) {
    state = BluetoothA2dpManager::SinkState::SINK_PLAYING;
  } else {
    BT_WARNING("Unknown sink state");
  }
  return state;
}


BluetoothA2dpManager*
BluetoothA2dpManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (sBluetoothA2dpManager) {
    return sBluetoothA2dpManager;
  }

  
  NS_ENSURE_FALSE(sInShutdown, nullptr);

  
  BluetoothA2dpManager* manager = new BluetoothA2dpManager();
  sBluetoothA2dpManager = manager;
  return sBluetoothA2dpManager;
}

class BluetoothA2dpManager::CleanupAvrcpResultHandler final
  : public BluetoothAvrcpResultHandler
{
public:
  CleanupAvrcpResultHandler(BluetoothProfileResultHandler* aRes)
    : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothAvrcpInterface::Cleanup failed: %d",
               (int)aStatus);

    sBtAvrcpInterface = nullptr;

    if (mRes) {
      if (aStatus == STATUS_UNSUPPORTED) {
        



        mRes->Deinit();
      } else {
        mRes->OnError(NS_ERROR_FAILURE);
      }
    }
  }

  void Cleanup() override
  {
    sBtAvrcpInterface = nullptr;
    if (mRes) {
      mRes->Deinit();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class BluetoothA2dpManager::CleanupA2dpResultHandler final
  : public BluetoothA2dpResultHandler
{
public:
  CleanupA2dpResultHandler(BluetoothProfileResultHandler* aRes)
    : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) override
  {
    BT_WARNING("BluetoothA2dpInterface::Cleanup failed: %d",
               (int)aStatus);

    sBtA2dpInterface = nullptr;

    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Cleanup() override
  {
    sBtA2dpInterface = nullptr;
    if (sBtAvrcpInterface) {
      sBtAvrcpInterface->Cleanup(new CleanupAvrcpResultHandler(mRes));
    } else if (mRes) {
      


      mRes->Deinit();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class BluetoothA2dpManager::CleanupA2dpResultHandlerRunnable final
  : public nsRunnable
{
public:
  CleanupA2dpResultHandlerRunnable(BluetoothProfileResultHandler* aRes)
    : mRes(aRes)
  { }

  NS_IMETHOD Run() override
  {
    sBtA2dpInterface = nullptr;
    if (sBtAvrcpInterface) {
      sBtAvrcpInterface->Cleanup(new CleanupAvrcpResultHandler(mRes));
    } else if (mRes) {
      


      mRes->Deinit();
    }

    return NS_OK;
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};


void
BluetoothA2dpManager::DeinitA2dpInterface(BluetoothProfileResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sBtA2dpInterface) {
    sBtA2dpInterface->Cleanup(new CleanupA2dpResultHandler(aRes));
  } else if (aRes) {
    
    
    nsRefPtr<nsRunnable> r = new CleanupA2dpResultHandlerRunnable(aRes);
    if (NS_FAILED(NS_DispatchToMainThread(r))) {
      BT_LOGR("Failed to dispatch cleanup-result-handler runnable");
    }
  }
}

void
BluetoothA2dpManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  sInShutdown = true;
  Disconnect(nullptr);
  sBluetoothA2dpManager = nullptr;
}

void
BluetoothA2dpManager::OnConnectError()
{
  MOZ_ASSERT(NS_IsMainThread());

  mController->NotifyCompletion(NS_LITERAL_STRING(ERR_CONNECTION_FAILED));

  mController = nullptr;
  mDeviceAddress.Truncate();
}

class BluetoothA2dpManager::ConnectResultHandler final
  : public BluetoothA2dpResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) override
  {
    BT_LOGR("BluetoothA2dpInterface::Connect failed: %d", (int)aStatus);

    NS_ENSURE_TRUE_VOID(sBluetoothA2dpManager);
    sBluetoothA2dpManager->OnConnectError();
  }
};

void
BluetoothA2dpManager::Connect(const nsAString& aDeviceAddress,
                              BluetoothProfileController* aController)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aDeviceAddress.IsEmpty());
  MOZ_ASSERT(aController);

  BluetoothService* bs = BluetoothService::Get();
  if (!bs || sInShutdown) {
    aController->NotifyCompletion(NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    return;
  }

  if (mA2dpConnected) {
    aController->NotifyCompletion(NS_LITERAL_STRING(ERR_ALREADY_CONNECTED));
    return;
  }

  mDeviceAddress = aDeviceAddress;
  mController = aController;

  if (!sBtA2dpInterface) {
    BT_LOGR("sBluetoothA2dpInterface is null");
    aController->NotifyCompletion(NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    return;
  }

  sBtA2dpInterface->Connect(aDeviceAddress, new ConnectResultHandler());
}

void
BluetoothA2dpManager::OnDisconnectError()
{
  MOZ_ASSERT(NS_IsMainThread());

  mController->NotifyCompletion(NS_LITERAL_STRING(ERR_DISCONNECTION_FAILED));
}

class BluetoothA2dpManager::DisconnectResultHandler final
  : public BluetoothA2dpResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) override
  {
    BT_LOGR("BluetoothA2dpInterface::Disconnect failed: %d", (int)aStatus);

    NS_ENSURE_TRUE_VOID(sBluetoothA2dpManager);
    sBluetoothA2dpManager->OnDisconnectError();
  }
};

void
BluetoothA2dpManager::Disconnect(BluetoothProfileController* aController)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mController);

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    if (aController) {
      aController->NotifyCompletion(NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    }
    return;
  }

  if (!mA2dpConnected) {
    if (aController) {
      aController->NotifyCompletion(NS_LITERAL_STRING(ERR_ALREADY_DISCONNECTED));
    }
    return;
  }

  MOZ_ASSERT(!mDeviceAddress.IsEmpty());

  mController = aController;

  if (!sBtA2dpInterface) {
    BT_LOGR("sBluetoothA2dpInterface is null");
    aController->NotifyCompletion(NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    return;
  }

  sBtA2dpInterface->Disconnect(mDeviceAddress, new DisconnectResultHandler());
}

void
BluetoothA2dpManager::OnConnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(NS_IsMainThread());

  



  NS_ENSURE_TRUE_VOID(mController);

  nsRefPtr<BluetoothProfileController> controller = mController.forget();
  controller->NotifyCompletion(aErrorStr);
}

void
BluetoothA2dpManager::OnDisconnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(NS_IsMainThread());

  



  NS_ENSURE_TRUE_VOID(mController);

  nsRefPtr<BluetoothProfileController> controller = mController.forget();
  controller->NotifyCompletion(aErrorStr);

  Reset();
}



















void
BluetoothA2dpManager::HandleSinkPropertyChanged(const BluetoothSignal& aSignal)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aSignal.value().type() ==
             BluetoothValue::TArrayOfBluetoothNamedValue);

  const nsString& address = aSignal.path();
  




  NS_ENSURE_TRUE_VOID(mDeviceAddress.IsEmpty() ||
                      mDeviceAddress.Equals(address));

  const InfallibleTArray<BluetoothNamedValue>& arr =
    aSignal.value().get_ArrayOfBluetoothNamedValue();
  MOZ_ASSERT(arr.Length() == 1);

  








  const nsString& name = arr[0].name();
  NS_ENSURE_TRUE_VOID(name.EqualsLiteral("State"));

  const BluetoothValue& value = arr[0].value();
  MOZ_ASSERT(value.type() == BluetoothValue::TnsString);
  SinkState newState = StatusStringToSinkState(value.get_nsString());
  NS_ENSURE_TRUE_VOID((newState != SinkState::SINK_UNKNOWN) &&
                      (newState != mSinkState));

  SinkState prevState = mSinkState;
  mSinkState = newState;

  switch(mSinkState) {
    case SinkState::SINK_CONNECTING:
      
      MOZ_ASSERT(prevState == SinkState::SINK_DISCONNECTED);
      break;
    case SinkState::SINK_PLAYING:
      
      MOZ_ASSERT(prevState == SinkState::SINK_CONNECTED);
      break;
    case SinkState::SINK_CONNECTED:
      
      if (prevState == SinkState::SINK_PLAYING ||
          prevState == SinkState::SINK_CONNECTED) {
        break;
      }

      
      mA2dpConnected = true;
      mDeviceAddress = address;
      NotifyConnectionStatusChanged();

      OnConnect(EmptyString());
      break;
    case SinkState::SINK_DISCONNECTED:
      
      if (prevState == SinkState::SINK_CONNECTING) {
        OnConnect(NS_LITERAL_STRING(ERR_CONNECTION_FAILED));
        break;
      }

      
      MOZ_ASSERT(prevState == SinkState::SINK_CONNECTED ||
                 prevState == SinkState::SINK_PLAYING) ;

      mA2dpConnected = false;
      NotifyConnectionStatusChanged();
      mDeviceAddress.Truncate();
      OnDisconnect(EmptyString());
      break;
    default:
      break;
  }
}

void
BluetoothA2dpManager::NotifyConnectionStatusChanged()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);

  if (NS_FAILED(obs->NotifyObservers(this,
                                     BLUETOOTH_A2DP_STATUS_CHANGED_ID,
                                     mDeviceAddress.get()))) {
    BT_WARNING("Failed to notify bluetooth-a2dp-status-changed observsers!");
  }

  
  DispatchStatusChangedEvent(
    NS_LITERAL_STRING(A2DP_STATUS_CHANGED_ID), mDeviceAddress, mA2dpConnected);
}

void
BluetoothA2dpManager::OnGetServiceChannel(const nsAString& aDeviceAddress,
                                          const nsAString& aServiceUuid,
                                          int aChannel)
{
}

void
BluetoothA2dpManager::OnUpdateSdpRecords(const nsAString& aDeviceAddress)
{
}

void
BluetoothA2dpManager::GetAddress(nsAString& aDeviceAddress)
{
  aDeviceAddress = mDeviceAddress;
}

bool
BluetoothA2dpManager::IsConnected()
{
  return mA2dpConnected;
}





void
BluetoothA2dpManager::SetAvrcpConnected(bool aConnected)
{
  mAvrcpConnected = aConnected;
  if (!aConnected) {
    ResetAvrcp();
  }
}

bool
BluetoothA2dpManager::IsAvrcpConnected()
{
  return mAvrcpConnected;
}





void
BluetoothA2dpManager::UpdateMetaData(const nsAString& aTitle,
                                     const nsAString& aArtist,
                                     const nsAString& aAlbum,
                                     uint64_t aMediaNumber,
                                     uint64_t aTotalMediaCount,
                                     uint32_t aDuration)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE_VOID(sBtAvrcpInterface);

  
  
  if (mMediaNumber != aMediaNumber &&
      mTrackChangedNotifyType == AVRCP_NTF_INTERIM) {
    BluetoothAvrcpNotificationParam param;
    
    
    
    for (int i = 0; i < AVRCP_UID_SIZE; ++i) {
      param.mTrack[i] = (aMediaNumber >> (56 - 8 * i));
    }
    mTrackChangedNotifyType = AVRCP_NTF_CHANGED;
    sBtAvrcpInterface->RegisterNotificationRsp(AVRCP_EVENT_TRACK_CHANGE,
                                               AVRCP_NTF_CHANGED,
                                               param, nullptr);
    if (mPlayPosChangedNotifyType == AVRCP_NTF_INTERIM) {
      param.mSongPos = mPosition;
      
      mPlayPosChangedNotifyType = AVRCP_NTF_CHANGED;
      sBtAvrcpInterface->RegisterNotificationRsp(AVRCP_EVENT_PLAY_POS_CHANGED,
                                                 AVRCP_NTF_CHANGED,
                                                 param, nullptr);
    }
  }

  mTitle.Assign(aTitle);
  mArtist.Assign(aArtist);
  mAlbum.Assign(aAlbum);
  mMediaNumber = aMediaNumber;
  mTotalMediaCount = aTotalMediaCount;
  mDuration = aDuration;
}





void
BluetoothA2dpManager::UpdatePlayStatus(uint32_t aDuration,
                                       uint32_t aPosition,
                                       ControlPlayStatus aPlayStatus)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE_VOID(sBtAvrcpInterface);
  
  sBtAvrcpInterface->GetPlayStatusRsp(aPlayStatus, aDuration,
                                      aPosition, nullptr);
  
  if (mPlayStatus != aPlayStatus &&
      mPlayStatusChangedNotifyType == AVRCP_NTF_INTERIM) {
    BluetoothAvrcpNotificationParam param;
    param.mPlayStatus = aPlayStatus;
    mPlayStatusChangedNotifyType = AVRCP_NTF_CHANGED;
    sBtAvrcpInterface->RegisterNotificationRsp(AVRCP_EVENT_PLAY_STATUS_CHANGED,
                                               AVRCP_NTF_CHANGED,
                                               param, nullptr);
  }

  if (mPosition != aPosition &&
      mPlayPosChangedNotifyType == AVRCP_NTF_INTERIM) {
    BluetoothAvrcpNotificationParam param;
    param.mSongPos = aPosition;
    mPlayPosChangedNotifyType = AVRCP_NTF_CHANGED;
    sBtAvrcpInterface->RegisterNotificationRsp(AVRCP_EVENT_PLAY_POS_CHANGED,
                                               AVRCP_NTF_CHANGED,
                                               param, nullptr);
  }

  mDuration = aDuration;
  mPosition = aPosition;
  mPlayStatus = aPlayStatus;
}








void
BluetoothA2dpManager::UpdateRegisterNotification(BluetoothAvrcpEvent aEvent,
                                                 uint32_t aParam)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE_VOID(sBtAvrcpInterface);

  BluetoothAvrcpNotificationParam param;

  switch (aEvent) {
    case AVRCP_EVENT_PLAY_STATUS_CHANGED:
      mPlayStatusChangedNotifyType = AVRCP_NTF_INTERIM;
      param.mPlayStatus = mPlayStatus;
      break;
    case AVRCP_EVENT_TRACK_CHANGE:
      
      
      
      
      
      
      
      mTrackChangedNotifyType = AVRCP_NTF_INTERIM;
      
      
      for (int index = 0; index < AVRCP_UID_SIZE; ++index) {
        
        
        if (mSinkState == BluetoothA2dpManager::SinkState::SINK_PLAYING) {
          param.mTrack[index] = 0x0;
        } else {
          param.mTrack[index] = 0xFF;
        }
      }
      break;
    case AVRCP_EVENT_PLAY_POS_CHANGED:
      
      mPlayPosChangedNotifyType = AVRCP_NTF_INTERIM;
      if (mSinkState == BluetoothA2dpManager::SinkState::SINK_PLAYING) {
        param.mSongPos = mPosition;
      } else {
        param.mSongPos = 0xFFFFFFFF;
      }
      mPlaybackInterval = aParam;
      break;
#ifdef MOZ_B2G_BT_API_V2
      
#else
    case AVRCP_EVENT_APP_SETTINGS_CHANGED:
      mAppSettingsChangedNotifyType = AVRCP_NTF_INTERIM;
      param.mNumAttr = 2;
      param.mIds[0] = AVRCP_PLAYER_ATTRIBUTE_REPEAT;
      param.mValues[0] = AVRCP_PLAYER_VAL_OFF_REPEAT;
      param.mIds[1] = AVRCP_PLAYER_ATTRIBUTE_SHUFFLE;
      param.mValues[1] = AVRCP_PLAYER_VAL_OFF_SHUFFLE;
      break;
#endif
    default:
      break;
  }

  sBtAvrcpInterface->RegisterNotificationRsp(aEvent, AVRCP_NTF_INTERIM,
                                             param, nullptr);
}

void
BluetoothA2dpManager::GetAlbum(nsAString& aAlbum)
{
  aAlbum.Assign(mAlbum);
}

uint32_t
BluetoothA2dpManager::GetDuration()
{
  return mDuration;
}

ControlPlayStatus
BluetoothA2dpManager::GetPlayStatus()
{
  return mPlayStatus;
}

uint32_t
BluetoothA2dpManager::GetPosition()
{
  return mPosition;
}

uint64_t
BluetoothA2dpManager::GetMediaNumber()
{
  return mMediaNumber;
}

uint64_t
BluetoothA2dpManager::GetTotalMediaNumber()
{
  return mTotalMediaCount;
}

void
BluetoothA2dpManager::GetTitle(nsAString& aTitle)
{
  aTitle.Assign(mTitle);
}

void
BluetoothA2dpManager::GetArtist(nsAString& aArtist)
{
  aArtist.Assign(mArtist);
}





void
BluetoothA2dpManager::ConnectionStateNotification(BluetoothA2dpConnectionState aState,
                                                  const nsAString& aBdAddr)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsString a2dpState;
  AvStatusToSinkString(aState, a2dpState);

  InfallibleTArray<BluetoothNamedValue> props;
  BT_APPEND_NAMED_VALUE(props, "State", a2dpState);

  HandleSinkPropertyChanged(BluetoothSignal(NS_LITERAL_STRING("AudioSink"),
                                            nsString(aBdAddr), props));
}

void
BluetoothA2dpManager::AudioStateNotification(BluetoothA2dpAudioState aState,
                                             const nsAString& aBdAddr)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsString a2dpState;

  if (aState == A2DP_AUDIO_STATE_STARTED) {
    a2dpState = NS_LITERAL_STRING("playing");
  } else if (aState == A2DP_AUDIO_STATE_STOPPED) {
    
    a2dpState = NS_LITERAL_STRING("connected");
  } else if (aState == A2DP_AUDIO_STATE_REMOTE_SUSPEND) {
    
    a2dpState = NS_LITERAL_STRING("connected");
  }

  InfallibleTArray<BluetoothNamedValue> props;
  BT_APPEND_NAMED_VALUE(props, "State", a2dpState);

  HandleSinkPropertyChanged(BluetoothSignal(NS_LITERAL_STRING("AudioSink"),
                                            nsString(aBdAddr), props));
}





void
BluetoothA2dpManager::GetPlayStatusNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    return;
  }

#ifdef MOZ_B2G_BT_API_V2
  bs->DistributeSignal(NS_LITERAL_STRING(REQUEST_MEDIA_PLAYSTATUS_ID),
                       NS_LITERAL_STRING(KEY_ADAPTER));
#else
  bs->DistributeSignal(
    BluetoothSignal(NS_LITERAL_STRING(REQUEST_MEDIA_PLAYSTATUS_ID),
                    NS_LITERAL_STRING(KEY_ADAPTER),
                    InfallibleTArray<BluetoothNamedValue>()));
#endif
}





void
BluetoothA2dpManager::ListPlayerAppAttrNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothA2dpManager::ListPlayerAppValuesNotification(
  BluetoothAvrcpPlayerAttribute aAttrId)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothA2dpManager::GetPlayerAppValueNotification(
  uint8_t aNumAttrs, const BluetoothAvrcpPlayerAttribute* aAttrs)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothA2dpManager::GetPlayerAppAttrsTextNotification(
  uint8_t aNumAttrs, const BluetoothAvrcpPlayerAttribute* aAttrs)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothA2dpManager::GetPlayerAppValuesTextNotification(
  uint8_t aAttrId, uint8_t aNumVals, const uint8_t* aValues)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothA2dpManager::SetPlayerAppValueNotification(
  const BluetoothAvrcpPlayerSettings& aSettings)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}






void
BluetoothA2dpManager::GetElementAttrNotification(
  uint8_t aNumAttrs, const BluetoothAvrcpMediaAttribute* aAttrs)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoArrayPtr<BluetoothAvrcpElementAttribute> attrs(
    new BluetoothAvrcpElementAttribute[aNumAttrs]);

  for (uint8_t i = 0; i < aNumAttrs; ++i) {
    attrs[i].mId = aAttrs[i];
    ConvertAttributeString(
      static_cast<BluetoothAvrcpMediaAttribute>(attrs[i].mId),
      attrs[i].mValue);
  }

  MOZ_ASSERT(sBtAvrcpInterface);
  sBtAvrcpInterface->GetElementAttrRsp(aNumAttrs, attrs, nullptr);
}

void
BluetoothA2dpManager::RegisterNotificationNotification(
  BluetoothAvrcpEvent aEvent, uint32_t aParam)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothA2dpManager* a2dp = BluetoothA2dpManager::Get();
  if (!a2dp) {
    return;
  }

  a2dp->UpdateRegisterNotification(aEvent, aParam);
}






void
BluetoothA2dpManager::RemoteFeatureNotification(
    const nsAString& aBdAddr, unsigned long aFeatures)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}




void
BluetoothA2dpManager::VolumeChangeNotification(uint8_t aVolume,
                                               uint8_t aCType)
{
  MOZ_ASSERT(NS_IsMainThread());

  
}

void
BluetoothA2dpManager::PassthroughCmdNotification(int aId, int aKeyState)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsAutoString name;
  NS_ENSURE_TRUE_VOID(aKeyState == AVRC_KEY_PRESS_STATE ||
                      aKeyState == AVRC_KEY_RELEASE_STATE);
  switch (aId) {
    case AVRC_ID_FAST_FOR:
      if (aKeyState == AVRC_KEY_PRESS_STATE) {
        name.AssignLiteral("media-fast-forward-button-press");
      } else {
        name.AssignLiteral("media-fast-forward-button-release");
      }
      break;
    case AVRC_ID_REWIND:
      if (aKeyState == AVRC_KEY_PRESS_STATE) {
        name.AssignLiteral("media-rewind-button-press");
      } else {
        name.AssignLiteral("media-rewind-button-release");
      }
      break;
    default:
      BT_WARNING("Unable to handle the unknown PassThrough command %d", aId);
      return;
  }

  NS_NAMED_LITERAL_STRING(type, "media-button");
  BroadcastSystemMessage(type, BluetoothValue(name));
}

NS_IMPL_ISUPPORTS(BluetoothA2dpManager, nsIObserver)

