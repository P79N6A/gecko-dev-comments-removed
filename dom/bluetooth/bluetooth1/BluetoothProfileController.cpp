





#include "BluetoothProfileController.h"

#include "BluetoothA2dpManager.h"
#include "BluetoothHfpManager.h"
#include "BluetoothHidManager.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsComponentManagerUtils.h"

USING_BLUETOOTH_NAMESPACE

#define BT_LOGR_PROFILE(mgr, msg, ...)               \
  do {                                               \
    nsCString name;                                  \
    mgr->GetName(name);                              \
    BT_LOGR("[%s] " msg, name.get(), ##__VA_ARGS__); \
  } while(0)

#define CONNECTION_TIMEOUT_MS 15000

class CheckProfileStatusCallback : public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  CheckProfileStatusCallback(BluetoothProfileController* aController)
    : mController(aController)
  {
    MOZ_ASSERT(aController);
  }

protected:
  virtual ~CheckProfileStatusCallback()
  {
    mController = nullptr;
  }

private:
  nsRefPtr<BluetoothProfileController> mController;
};

BluetoothProfileController::BluetoothProfileController(
                                   bool aConnect,
                                   const nsAString& aDeviceAddress,
                                   BluetoothReplyRunnable* aRunnable,
                                   BluetoothProfileControllerCallback aCallback,
                                   uint16_t aServiceUuid,
                                   uint32_t aCod)
  : mConnect(aConnect)
  , mDeviceAddress(aDeviceAddress)
  , mRunnable(aRunnable)
  , mCallback(aCallback)
  , mCurrentProfileFinished(false)
  , mSuccess(false)
  , mProfilesIndex(-1)
{
  MOZ_ASSERT(!aDeviceAddress.IsEmpty());
  MOZ_ASSERT(aRunnable);
  MOZ_ASSERT(aCallback);

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  MOZ_ASSERT(mTimer);

  mProfiles.Clear();

  



  if (!aServiceUuid) {
    mTarget.cod = aCod;
    SetupProfiles(false);
  } else {
    BluetoothServiceClass serviceClass =
      BluetoothUuidHelper::GetBluetoothServiceClass(aServiceUuid);
    mTarget.service = serviceClass;
    SetupProfiles(true);
  }
}

BluetoothProfileController::~BluetoothProfileController()
{
  mProfiles.Clear();
  mRunnable = nullptr;
  mCallback = nullptr;

  if (mTimer) {
    mTimer->Cancel();
  }
}

void
BluetoothProfileController::AddProfileWithServiceClass(
                                                   BluetoothServiceClass aClass)
{
  BluetoothProfileManagerBase* profile;
  switch (aClass) {
    case BluetoothServiceClass::HANDSFREE:
    case BluetoothServiceClass::HEADSET:
      profile = BluetoothHfpManager::Get();
      break;
    case BluetoothServiceClass::A2DP:
      profile = BluetoothA2dpManager::Get();
      break;
    case BluetoothServiceClass::HID:
      profile = BluetoothHidManager::Get();
      break;
    default:
      DispatchBluetoothReply(mRunnable, BluetoothValue(),
                             NS_LITERAL_STRING(ERR_UNKNOWN_PROFILE));
      mCallback();
      return;
  }

  AddProfile(profile);
}

void
BluetoothProfileController::AddProfile(BluetoothProfileManagerBase* aProfile,
                                       bool aCheckConnected)
{
  if (!aProfile) {
    DispatchBluetoothReply(mRunnable, BluetoothValue(),
                           NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    mCallback();
    return;
  }

  if (aCheckConnected && !aProfile->IsConnected()) {
    BT_WARNING("The profile is not connected.");
    return;
  }

  mProfiles.AppendElement(aProfile);
}

void
BluetoothProfileController::SetupProfiles(bool aAssignServiceClass)
{
  MOZ_ASSERT(NS_IsMainThread());

  



  if (aAssignServiceClass) {
    AddProfileWithServiceClass(mTarget.service);
    return;
  }

  
  if (!mConnect) {
    AddProfile(BluetoothHidManager::Get(), true);
    AddProfile(BluetoothA2dpManager::Get(), true);
    AddProfile(BluetoothHfpManager::Get(), true);
    return;
  }

  



  bool hasAudio = HAS_AUDIO(mTarget.cod);
  bool hasRendering = HAS_RENDERING(mTarget.cod);
  bool isPeripheral = IS_PERIPHERAL(mTarget.cod);
  bool isRemoteControl = IS_REMOTE_CONTROL(mTarget.cod);
  bool isKeyboard = IS_KEYBOARD(mTarget.cod);
  bool isPointingDevice = IS_POINTING_DEVICE(mTarget.cod);
  bool isInvalid = IS_INVALID_COD(mTarget.cod);

  
  
  if (isInvalid) {
    AddProfile(BluetoothHfpManager::Get());
    AddProfile(BluetoothA2dpManager::Get());
    AddProfile(BluetoothHidManager::Get());
    return;
  }

  NS_ENSURE_TRUE_VOID(hasAudio || hasRendering || isPeripheral);

  
  if (hasAudio) {
    AddProfile(BluetoothHfpManager::Get());
  }

  
  
  
  if (hasRendering || (isPeripheral && isRemoteControl)) {
    AddProfile(BluetoothA2dpManager::Get());
  }

  
  
  if (isPeripheral && (isKeyboard || isPointingDevice)) {
    AddProfile(BluetoothHidManager::Get());
  }
}

NS_IMPL_ISUPPORTS(CheckProfileStatusCallback, nsITimerCallback)

NS_IMETHODIMP
CheckProfileStatusCallback::Notify(nsITimer* aTimer)
{
  MOZ_ASSERT(mController);
  
  
  mController->GiveupAndContinue();

  return NS_OK;
}

void
BluetoothProfileController::StartSession()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mDeviceAddress.IsEmpty());
  MOZ_ASSERT(mProfilesIndex == -1);
  MOZ_ASSERT(mTimer);

  if (!IsBtServiceAvailable()) {
    EndSession();
    return;
  }

  if (mProfiles.Length() < 1) {
    BT_LOGR("No queued profile.");
    EndSession();
    return;
  }

  if (mTimer) {
    mTimer->InitWithCallback(new CheckProfileStatusCallback(this),
                             CONNECTION_TIMEOUT_MS, nsITimer::TYPE_ONE_SHOT);
  }

  BT_LOGR("%s", mConnect ? "connecting" : "disconnecting");

  Next();
}

void
BluetoothProfileController::EndSession()
{
  MOZ_ASSERT(mRunnable && mCallback);

  BT_LOGR("mSuccess %d", mSuccess);

  
  
  if (mTimer) {
    mTimer->Cancel();
  }

  
  
  if (mSuccess) {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true), EmptyString());
  } else if (mConnect) {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true),
                           NS_LITERAL_STRING(ERR_CONNECTION_FAILED));
  } else {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true),
                           NS_LITERAL_STRING(ERR_DISCONNECTION_FAILED));
  }

  mCallback();
}

void
BluetoothProfileController::Next()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mDeviceAddress.IsEmpty());
  MOZ_ASSERT(mProfilesIndex < (int)mProfiles.Length());
  MOZ_ASSERT(mTimer);

  mCurrentProfileFinished = false;

  if (!IsBtServiceAvailable()) {
    EndSession();
    return;
  }

  if (++mProfilesIndex >= (int)mProfiles.Length()) {
    EndSession();
    return;
  }

  BT_LOGR_PROFILE(mProfiles[mProfilesIndex], "");

  if (mConnect) {
    mProfiles[mProfilesIndex]->Connect(mDeviceAddress, this);
  } else {
    mProfiles[mProfilesIndex]->Disconnect(this);
  }
}

bool
BluetoothProfileController::IsBtServiceAvailable() const
{
  BluetoothService* bs = BluetoothService::Get();
  return (bs && bs->IsEnabled() && !bs->IsToggling());
}

void
BluetoothProfileController::NotifyCompletion(const nsAString& aErrorStr)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mTimer);
  MOZ_ASSERT(mProfiles.Length() > 0);

  BT_LOGR_PROFILE(mProfiles[mProfilesIndex], "<%s>",
                  NS_ConvertUTF16toUTF8(aErrorStr).get());

  mCurrentProfileFinished = true;

  if (mTimer) {
    mTimer->Cancel();
  }

  mSuccess |= aErrorStr.IsEmpty();

  Next();
}

void
BluetoothProfileController::GiveupAndContinue()
{
  MOZ_ASSERT(!mCurrentProfileFinished);
  MOZ_ASSERT(mProfilesIndex < (int)mProfiles.Length());

  BT_LOGR_PROFILE(mProfiles[mProfilesIndex], ERR_OPERATION_TIMEOUT);
  mProfiles[mProfilesIndex]->Reset();

  if (IsBtServiceAvailable()) {
    Next();
  } else {
    EndSession();
  }
}

