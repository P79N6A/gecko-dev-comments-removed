





#include "BluetoothProfileController.h"
#include "BluetoothReplyRunnable.h"

#include "BluetoothA2dpManager.h"
#include "BluetoothHfpManager.h"
#include "BluetoothHidManager.h"

#include "BluetoothUtils.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"

USING_BLUETOOTH_NAMESPACE

#define BT_LOGR_PROFILE(mgr, args...)                 \
  do {                                                \
    nsCString name;                                   \
    mgr->GetName(name);                               \
    BT_LOGR("%s: [%s] %s", __FUNCTION__, name.get(),  \
      nsPrintfCString(args).get());                   \
  } while(0)

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
  , mSuccess(false)
  , mProfilesIndex(-1)
{
  MOZ_ASSERT(!aDeviceAddress.IsEmpty());
  MOZ_ASSERT(aRunnable);
  MOZ_ASSERT(aCallback);

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

  NS_ENSURE_TRUE_VOID(hasAudio || hasRendering || isPeripheral);

  


  if (hasAudio) {
    AddProfile(BluetoothHfpManager::Get());
  }
  if (hasRendering) {
    AddProfile(BluetoothA2dpManager::Get());
  }
  if (isPeripheral) {
    AddProfile(BluetoothHidManager::Get());
  }
}

void
BluetoothProfileController::Start()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mDeviceAddress.IsEmpty());
  MOZ_ASSERT(mProfilesIndex == -1);

  ++mProfilesIndex;
  BT_LOGR_PROFILE(mProfiles[mProfilesIndex], "");

  if (mConnect) {
    mProfiles[mProfilesIndex]->Connect(mDeviceAddress, this);
  } else {
    mProfiles[mProfilesIndex]->Disconnect(this);
  }
}

void
BluetoothProfileController::Next()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mDeviceAddress.IsEmpty());
  MOZ_ASSERT(mProfilesIndex < mProfiles.Length());

  if (++mProfilesIndex < mProfiles.Length()) {
    BT_LOGR_PROFILE(mProfiles[mProfilesIndex], "");

    if (mConnect) {
      mProfiles[mProfilesIndex]->Connect(mDeviceAddress, this);
    } else {
      mProfiles[mProfilesIndex]->Disconnect(this);
    }
    return;
  }

  MOZ_ASSERT(mRunnable && mCallback);

  
  
  if (mSuccess) {
    DispatchBluetoothReply(mRunnable, BluetoothValue(true), EmptyString());
  } else {
    DispatchBluetoothReply(mRunnable, BluetoothValue(),
                           NS_LITERAL_STRING(ERR_CONNECTION_FAILED));
  }
  mCallback();
}

void
BluetoothProfileController::OnConnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(NS_IsMainThread());
  BT_LOGR_PROFILE(mProfiles[mProfilesIndex], "<%s>",
    NS_ConvertUTF16toUTF8(aErrorStr).get());

  if (!aErrorStr.IsEmpty()) {
    BT_WARNING(NS_ConvertUTF16toUTF8(aErrorStr).get());
  } else {
    mSuccess = true;
  }

  Next();
}

void
BluetoothProfileController::OnDisconnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(NS_IsMainThread());
  BT_LOGR_PROFILE(mProfiles[mProfilesIndex], "<%s>",
    NS_ConvertUTF16toUTF8(aErrorStr).get());

  if (!aErrorStr.IsEmpty()) {
    BT_WARNING(NS_ConvertUTF16toUTF8(aErrorStr).get());
  } else {
    mSuccess = true;
  }

  Next();
}
