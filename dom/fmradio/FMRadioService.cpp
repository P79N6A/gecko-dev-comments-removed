





#include "FMRadioService.h"
#include "mozilla/Hal.h"
#include "nsIAudioManager.h"
#include "AudioManager.h"
#include "nsDOMClassInfo.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/FMRadioChild.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsJSUtils.h"
#include "nsCxPusher.h"

#define BAND_87500_108000_kHz 1
#define BAND_76000_108000_kHz 2
#define BAND_76000_90000_kHz  3

#define CHANNEL_WIDTH_200KHZ 200
#define CHANNEL_WIDTH_100KHZ 100
#define CHANNEL_WIDTH_50KHZ  50

#define MOZSETTINGS_CHANGED_ID "mozsettings-changed"
#define SETTING_KEY_AIRPLANEMODE_ENABLED "airplaneMode.enabled"

using namespace mozilla::hal;
using mozilla::Preferences;

BEGIN_FMRADIO_NAMESPACE


IFMRadioService*
IFMRadioService::Singleton()
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return FMRadioChild::Singleton();
  } else {
    return FMRadioService::Singleton();
  }
}

StaticRefPtr<FMRadioService> FMRadioService::sFMRadioService;

FMRadioService::FMRadioService()
  : mPendingFrequencyInKHz(0)
  , mState(Disabled)
  , mHasReadAirplaneModeSetting(false)
  , mAirplaneModeEnabled(false)
  , mPendingRequest(nullptr)
  , mObserverList(FMRadioEventObserverList())
{

  
  mEnabled = IsFMRadioOn();
  if (mEnabled) {
    mPendingFrequencyInKHz = GetFMRadioFrequency();
    SetState(Enabled);
  }

  switch (Preferences::GetInt("dom.fmradio.band", BAND_87500_108000_kHz)) {
    case BAND_76000_90000_kHz:
      mUpperBoundInKHz = 90000;
      mLowerBoundInKHz = 76000;
      break;
    case BAND_76000_108000_kHz:
      mUpperBoundInKHz = 108000;
      mLowerBoundInKHz = 76000;
      break;
    case BAND_87500_108000_kHz:
    default:
      mUpperBoundInKHz = 108000;
      mLowerBoundInKHz = 87500;
      break;
  }

  switch (Preferences::GetInt("dom.fmradio.channelWidth",
                              CHANNEL_WIDTH_100KHZ)) {
    case CHANNEL_WIDTH_200KHZ:
      mChannelWidthInKHz = 200;
      break;
    case CHANNEL_WIDTH_50KHZ:
      mChannelWidthInKHz = 50;
      break;
    case CHANNEL_WIDTH_100KHZ:
    default:
      mChannelWidthInKHz = 100;
      break;
  }

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();

  if (obs && NS_FAILED(obs->AddObserver(this,
                                        MOZSETTINGS_CHANGED_ID,
                                         false))) {
    NS_WARNING("Failed to add settings change observer!");
  }

  RegisterFMRadioObserver(this);
}

FMRadioService::~FMRadioService()
{
  UnregisterFMRadioObserver(this);
}

class EnableRunnable MOZ_FINAL : public nsRunnable
{
public:
  EnableRunnable(int32_t aUpperLimit, int32_t aLowerLimit, int32_t aSpaceType)
    : mUpperLimit(aUpperLimit)
    , mLowerLimit(aLowerLimit)
    , mSpaceType(aSpaceType) { }

  NS_IMETHOD Run()
  {
    FMRadioSettings info;
    info.upperLimit() = mUpperLimit;
    info.lowerLimit() = mLowerLimit;
    info.spaceType() = mSpaceType;

    EnableFMRadio(info);

    return NS_OK;
  }

private:
  int32_t mUpperLimit;
  int32_t mLowerLimit;
  int32_t mSpaceType;
};





class ReadAirplaneModeSettingTask MOZ_FINAL : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  ReadAirplaneModeSettingTask(nsRefPtr<FMRadioReplyRunnable> aPendingRequest)
    : mPendingRequest(aPendingRequest) { }

  NS_IMETHOD
  Handle(const nsAString& aName, JS::Handle<JS::Value> aResult)
  {
    FMRadioService* fmRadioService = FMRadioService::Singleton();
    MOZ_ASSERT(mPendingRequest == fmRadioService->mPendingRequest);

    fmRadioService->mHasReadAirplaneModeSetting = true;

    if (!aResult.isBoolean()) {
      
      fmRadioService->TransitionState(
        ErrorResponse(NS_LITERAL_STRING("Unexpected error")), Disabled);
      return NS_OK;
    }

    fmRadioService->mAirplaneModeEnabled = aResult.toBoolean();
    if (!fmRadioService->mAirplaneModeEnabled) {
      EnableRunnable* runnable =
        new EnableRunnable(fmRadioService->mUpperBoundInKHz,
                           fmRadioService->mLowerBoundInKHz,
                           fmRadioService->mChannelWidthInKHz);
      NS_DispatchToMainThread(runnable);
    } else {
      
      fmRadioService->TransitionState(ErrorResponse(
        NS_LITERAL_STRING("Airplane mode currently enabled")), Disabled);
    }

    return NS_OK;
  }

  NS_IMETHOD
  HandleError(const nsAString& aName)
  {
    FMRadioService* fmRadioService = FMRadioService::Singleton();
    MOZ_ASSERT(mPendingRequest == fmRadioService->mPendingRequest);

    fmRadioService->TransitionState(ErrorResponse(
      NS_LITERAL_STRING("Unexpected error")), Disabled);

    return NS_OK;
  }

private:
  nsRefPtr<FMRadioReplyRunnable> mPendingRequest;
};

NS_IMPL_ISUPPORTS(ReadAirplaneModeSettingTask, nsISettingsServiceCallback)

class DisableRunnable MOZ_FINAL : public nsRunnable
{
public:
  DisableRunnable() { }

  NS_IMETHOD Run()
  {
    
    
    DisableFMRadio();
    IFMRadioService::Singleton()->EnableAudio(false);

    return NS_OK;
  }
};

class SetFrequencyRunnable MOZ_FINAL : public nsRunnable
{
public:
  SetFrequencyRunnable(int32_t aFrequency)
    : mFrequency(aFrequency) { }

  NS_IMETHOD Run()
  {
    SetFMRadioFrequency(mFrequency);
    return NS_OK;
  }

private:
  int32_t mFrequency;
};

class SeekRunnable MOZ_FINAL : public nsRunnable
{
public:
  SeekRunnable(FMRadioSeekDirection aDirection) : mDirection(aDirection) { }

  NS_IMETHOD Run()
  {
    switch (mDirection) {
      case FM_RADIO_SEEK_DIRECTION_UP:
      case FM_RADIO_SEEK_DIRECTION_DOWN:
        FMRadioSeek(mDirection);
        break;
      default:
        MOZ_CRASH();
    }

    return NS_OK;
  }

private:
  FMRadioSeekDirection mDirection;
};

void
FMRadioService::TransitionState(const FMRadioResponseType& aResponse,
                                FMRadioState aState)
{
  if (mPendingRequest) {
    mPendingRequest->SetReply(aResponse);
    NS_DispatchToMainThread(mPendingRequest);
  }

  SetState(aState);
}

void
FMRadioService::SetState(FMRadioState aState)
{
  mState = aState;
  mPendingRequest = nullptr;
}

void
FMRadioService::AddObserver(FMRadioEventObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  mObserverList.AddObserver(aObserver);
}

void
FMRadioService::RemoveObserver(FMRadioEventObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  mObserverList.RemoveObserver(aObserver);

  if (mObserverList.Length() == 0)
  {
    
    if (IsFMRadioOn()) {
      NS_DispatchToMainThread(new DisableRunnable());
    }
  }
}

void
FMRadioService::EnableAudio(bool aAudioEnabled)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIAudioManager> audioManager =
    do_GetService("@mozilla.org/telephony/audiomanager;1");
  if (!audioManager) {
    return;
  }

  bool audioEnabled;
  audioManager->GetFmRadioAudioEnabled(&audioEnabled);
  if (audioEnabled != aAudioEnabled) {
    audioManager->SetFmRadioAudioEnabled(aAudioEnabled);
  }
}













int32_t
FMRadioService::RoundFrequency(double aFrequencyInMHz)
{
  double halfChannelWidthInMHz = mChannelWidthInKHz / 1000.0 / 2;

  
  
  if (aFrequencyInMHz < mLowerBoundInKHz / 1000.0 - halfChannelWidthInMHz ||
      aFrequencyInMHz > mUpperBoundInKHz / 1000.0 + halfChannelWidthInMHz) {
    return 0;
  }

  int32_t partToBeRounded = round(aFrequencyInMHz * 1000) - mLowerBoundInKHz;
  int32_t roundedPart = round(partToBeRounded / (double)mChannelWidthInKHz) *
                        mChannelWidthInKHz;

  return mLowerBoundInKHz + roundedPart;
}

bool
FMRadioService::IsEnabled() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  return IsFMRadioOn();
}

double
FMRadioService::GetFrequency() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  if (IsEnabled()) {
    int32_t frequencyInKHz = GetFMRadioFrequency();
    return frequencyInKHz / 1000.0;
  }

  return 0;
}

double
FMRadioService::GetFrequencyUpperBound() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  return mUpperBoundInKHz / 1000.0;
}

double
FMRadioService::GetFrequencyLowerBound() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  return mLowerBoundInKHz / 1000.0;
}

double
FMRadioService::GetChannelWidth() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  return mChannelWidthInKHz / 1000.0;
}

void
FMRadioService::Enable(double aFrequencyInMHz,
                       FMRadioReplyRunnable* aReplyRunnable)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  MOZ_ASSERT(aReplyRunnable);

  switch (mState) {
    case Seeking:
    case Enabled:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently enabled")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Disabling:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently disabling")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Enabling:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently enabling")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Disabled:
      break;
  }

  int32_t roundedFrequency = RoundFrequency(aFrequencyInMHz);

  if (!roundedFrequency) {
    aReplyRunnable->SetReply(ErrorResponse(
      NS_LITERAL_STRING("Frequency is out of range")));
    NS_DispatchToMainThread(aReplyRunnable);
    return;
  }

  if (mHasReadAirplaneModeSetting && mAirplaneModeEnabled) {
    aReplyRunnable->SetReply(ErrorResponse(
      NS_LITERAL_STRING("Airplane mode currently enabled")));
    NS_DispatchToMainThread(aReplyRunnable);
    return;
  }

  SetState(Enabling);
  
  
  mPendingRequest = aReplyRunnable;

  
  mPendingFrequencyInKHz = roundedFrequency;

  if (!mHasReadAirplaneModeSetting) {
    nsCOMPtr<nsISettingsService> settings =
      do_GetService("@mozilla.org/settingsService;1");

    nsCOMPtr<nsISettingsServiceLock> settingsLock;
    nsresult rv = settings->CreateLock(nullptr, getter_AddRefs(settingsLock));
    if (NS_FAILED(rv)) {
      TransitionState(ErrorResponse(
        NS_LITERAL_STRING("Can't create settings lock")), Disabled);
      return;
    }

    nsRefPtr<ReadAirplaneModeSettingTask> callback =
      new ReadAirplaneModeSettingTask(mPendingRequest);

    rv = settingsLock->Get(SETTING_KEY_AIRPLANEMODE_ENABLED, callback);
    if (NS_FAILED(rv)) {
      TransitionState(ErrorResponse(
        NS_LITERAL_STRING("Can't get settings lock")), Disabled);
    }

    return;
  }

  NS_DispatchToMainThread(new EnableRunnable(mUpperBoundInKHz,
                                             mLowerBoundInKHz,
                                             mChannelWidthInKHz));
}

void
FMRadioService::Disable(FMRadioReplyRunnable* aReplyRunnable)
{
  
  
  
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  switch (mState) {
    case Disabling:
      if (aReplyRunnable) {
        aReplyRunnable->SetReply(
          ErrorResponse(NS_LITERAL_STRING("FM radio currently disabling")));
        NS_DispatchToMainThread(aReplyRunnable);
      }
      return;
    case Disabled:
      if (aReplyRunnable) {
        aReplyRunnable->SetReply(
          ErrorResponse(NS_LITERAL_STRING("FM radio currently disabled")));
        NS_DispatchToMainThread(aReplyRunnable);
      }
      return;
    case Enabled:
    case Enabling:
    case Seeking:
      break;
  }

  nsRefPtr<FMRadioReplyRunnable> enablingRequest = mPendingRequest;

  
  
  if (mState == Seeking) {
    TransitionState(ErrorResponse(
      NS_LITERAL_STRING("Seek action is cancelled")), Disabling);
  }

  FMRadioState preState = mState;
  SetState(Disabling);
  mPendingRequest = aReplyRunnable;

  if (preState == Enabling) {
    
    
    
    enablingRequest->SetReply(
      ErrorResponse(NS_LITERAL_STRING("Enable action is cancelled")));
    NS_DispatchToMainThread(enablingRequest);

    
    
    if (!mHasReadAirplaneModeSetting) {
      SetState(Disabled);

      if (aReplyRunnable) {
        aReplyRunnable->SetReply(SuccessResponse());
        NS_DispatchToMainThread(aReplyRunnable);
      }
    }

    return;
  }

  DoDisable();
}

void
FMRadioService::DoDisable()
{
  
  
  
  
  
  
  
  NS_DispatchToMainThread(new DisableRunnable());
}

void
FMRadioService::SetFrequency(double aFrequencyInMHz,
                             FMRadioReplyRunnable* aReplyRunnable)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  MOZ_ASSERT(aReplyRunnable);

  switch (mState) {
    case Disabled:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently disabled")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Enabling:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently enabling")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Disabling:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently disabling")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Seeking:
      CancelFMRadioSeek();
      TransitionState(ErrorResponse(
        NS_LITERAL_STRING("Seek action is cancelled")), Enabled);
      break;
    case Enabled:
      break;
  }

  int32_t roundedFrequency = RoundFrequency(aFrequencyInMHz);

  if (!roundedFrequency) {
    aReplyRunnable->SetReply(ErrorResponse(
      NS_LITERAL_STRING("Frequency is out of range")));
    NS_DispatchToMainThread(aReplyRunnable);
    return;
  }

  NS_DispatchToMainThread(new SetFrequencyRunnable(roundedFrequency));

  aReplyRunnable->SetReply(SuccessResponse());
  NS_DispatchToMainThread(aReplyRunnable);
}

void
FMRadioService::Seek(FMRadioSeekDirection aDirection,
                     FMRadioReplyRunnable* aReplyRunnable)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  MOZ_ASSERT(aReplyRunnable);

  switch (mState) {
    case Enabling:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently enabling")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Disabled:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently disabled")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Seeking:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently seeking")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Disabling:
      aReplyRunnable->SetReply(
        ErrorResponse(NS_LITERAL_STRING("FM radio currently disabling")));
      NS_DispatchToMainThread(aReplyRunnable);
      return;
    case Enabled:
      break;
  }

  SetState(Seeking);
  mPendingRequest = aReplyRunnable;

  NS_DispatchToMainThread(new SeekRunnable(aDirection));
}

void
FMRadioService::CancelSeek(FMRadioReplyRunnable* aReplyRunnable)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");
  MOZ_ASSERT(aReplyRunnable);

  
  if (mState != Seeking) {
    aReplyRunnable->SetReply(
      ErrorResponse(NS_LITERAL_STRING("FM radio currently not seeking")));
    NS_DispatchToMainThread(aReplyRunnable);
    return;
  }

  
  CancelFMRadioSeek();

  TransitionState(
    ErrorResponse(NS_LITERAL_STRING("Seek action is cancelled")), Enabled);

  aReplyRunnable->SetReply(SuccessResponse());
  NS_DispatchToMainThread(aReplyRunnable);
}

NS_IMETHODIMP
FMRadioService::Observe(nsISupports * aSubject,
                        const char * aTopic,
                        const char16_t * aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sFMRadioService);

  if (strcmp(aTopic, MOZSETTINGS_CHANGED_ID) != 0) {
    return NS_OK;
  }

  
  
  AutoSafeJSContext cx;
  const nsDependentString dataStr(aData);
  JS::Rooted<JS::Value> val(cx);
  if (!JS_ParseJSON(cx, dataStr.get(), dataStr.Length(), &val) ||
      !val.isObject()) {
    NS_WARNING("Bad JSON string format.");
    return NS_OK;
  }

  JS::Rooted<JSObject*> obj(cx, &val.toObject());
  JS::Rooted<JS::Value> key(cx);
  if (!JS_GetProperty(cx, obj, "key", &key) ||
      !key.isString()) {
    NS_WARNING("Failed to get string property `key`.");
    return NS_OK;
  }

  JS::Rooted<JSString*> jsKey(cx, key.toString());
  nsAutoJSString keyStr;
  if (!keyStr.init(cx, jsKey)) {
    return NS_OK;
  }

  if (keyStr.EqualsLiteral(SETTING_KEY_AIRPLANEMODE_ENABLED)) {
    JS::Rooted<JS::Value> value(cx);
    if (!JS_GetProperty(cx, obj, "value", &value)) {
      NS_WARNING("Failed to get property `value`.");
      return NS_OK;
    }

    if (!value.isBoolean()) {
      return NS_OK;
    }

    mAirplaneModeEnabled = value.toBoolean();
    mHasReadAirplaneModeSetting = true;

    
    if (mAirplaneModeEnabled) {
      Disable(nullptr);
    }
  }

  return NS_OK;
}

void
FMRadioService::NotifyFMRadioEvent(FMRadioEventType aType)
{
  mObserverList.Broadcast(aType);
}

void
FMRadioService::Notify(const FMRadioOperationInformation& aInfo)
{
  switch (aInfo.operation()) {
    case FM_RADIO_OPERATION_ENABLE:
      MOZ_ASSERT(IsFMRadioOn());
      MOZ_ASSERT(mState == Disabling || mState == Enabling);

      
      if (mState == Disabling) {
        DoDisable();
        return;
      }

      
      TransitionState(SuccessResponse(), Enabled);

      
      
      SetFMRadioFrequency(mPendingFrequencyInKHz);

      
      
      
      
      EnableAudio(true);

      
      
      
      mPendingFrequencyInKHz = GetFMRadioFrequency();
      UpdatePowerState();

      
      
      NotifyFMRadioEvent(FrequencyChanged);
      break;
    case FM_RADIO_OPERATION_DISABLE:
      MOZ_ASSERT(mState == Disabling);

      TransitionState(SuccessResponse(), Disabled);
      UpdatePowerState();
      break;
    case FM_RADIO_OPERATION_SEEK:

      
      
      if (mState == Seeking) {
        TransitionState(SuccessResponse(), Enabled);
      }

      UpdateFrequency();
      break;
    case FM_RADIO_OPERATION_TUNE:
      UpdateFrequency();
      break;
    default:
      MOZ_CRASH();
  }
}

void
FMRadioService::UpdatePowerState()
{
  bool enabled = IsFMRadioOn();
  if (enabled != mEnabled) {
    mEnabled = enabled;
    NotifyFMRadioEvent(EnabledChanged);
  }
}

void
FMRadioService::UpdateFrequency()
{
  int32_t frequency = GetFMRadioFrequency();
  if (mPendingFrequencyInKHz != frequency) {
    mPendingFrequencyInKHz = frequency;
    NotifyFMRadioEvent(FrequencyChanged);
  }
}


FMRadioService*
FMRadioService::Singleton()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(NS_IsMainThread());

  if (!sFMRadioService) {
    sFMRadioService = new FMRadioService();
  }

  return sFMRadioService;
}

NS_IMPL_ISUPPORTS(FMRadioService, nsIObserver)

END_FMRADIO_NAMESPACE

