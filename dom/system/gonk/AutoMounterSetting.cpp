



#include "AutoMounter.h"
#include "AutoMounterSetting.h"

#include "base/message_loop.h"
#include "jsapi.h"
#include "mozilla/Services.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsJSUtils.h"
#include "nsPrintfCString.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "xpcpublic.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/SettingChangeNotificationBinding.h"

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AutoMounterSetting" , ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "AutoMounterSetting" , ## args)

#define UMS_MODE                  "ums.mode"
#define UMS_STATUS                "ums.status"
#define UMS_VOLUME_ENABLED_PREFIX "ums.volume."
#define UMS_VOLUME_ENABLED_SUFFIX ".enabled"
#define MOZSETTINGS_CHANGED       "mozsettings-changed"

using namespace mozilla::dom;

namespace mozilla {
namespace system {

class SettingsServiceCallback MOZ_FINAL : public nsISettingsServiceCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  SettingsServiceCallback() {}

  NS_IMETHOD Handle(const nsAString& aName, JS::Handle<JS::Value> aResult)
  {
    if (aResult.isInt32()) {
      int32_t mode = aResult.toInt32();
      SetAutoMounterMode(mode);
    }
    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString& aName)
  {
    ERR("SettingsCallback::HandleError: %s\n", NS_LossyConvertUTF16toASCII(aName).get());
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS(SettingsServiceCallback, nsISettingsServiceCallback)

class CheckVolumeSettingsCallback MOZ_FINAL : public nsISettingsServiceCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  CheckVolumeSettingsCallback(const nsACString& aVolumeName)
  : mVolumeName(aVolumeName) {}

  NS_IMETHOD Handle(const nsAString& aName, JS::Handle<JS::Value> aResult)
  {
    if (aResult.isBoolean()) {
      bool isSharingEnabled = aResult.toBoolean();
      SetAutoMounterSharingMode(mVolumeName, isSharingEnabled);
    }
    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString& aName)
  {
    ERR("CheckVolumeSettingsCallback::HandleError: %s\n", NS_LossyConvertUTF16toASCII(aName).get());
    return NS_OK;
  }
private:
  nsCString mVolumeName;
};

NS_IMPL_ISUPPORTS(CheckVolumeSettingsCallback, nsISettingsServiceCallback)

AutoMounterSetting::AutoMounterSetting()
  : mStatus(AUTOMOUNTER_STATUS_DISABLED)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService) {
    ERR("GetObserverService failed");
    return;
  }
  nsresult rv;
  rv = observerService->AddObserver(this, MOZSETTINGS_CHANGED, false);
  if (NS_FAILED(rv)) {
    ERR("AddObserver failed");
    return;
  }

  
  
  
  
  nsCOMPtr<nsISettingsService> settingsService =
    do_GetService("@mozilla.org/settingsService;1");
  if (!settingsService) {
    ERR("Failed to get settingsLock service!");
    return;
  }
  nsCOMPtr<nsISettingsServiceLock> lock;
  settingsService->CreateLock(nullptr, getter_AddRefs(lock));
  nsCOMPtr<nsISettingsServiceCallback> callback = new SettingsServiceCallback();
  mozilla::AutoSafeJSContext cx;
  JS::Rooted<JS::Value> value(cx);
  value.setInt32(AUTOMOUNTER_DISABLE);
  lock->Set(UMS_MODE, value, callback, nullptr);
  value.setInt32(mStatus);
  lock->Set(UMS_STATUS, value, nullptr, nullptr);
}

AutoMounterSetting::~AutoMounterSetting()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->RemoveObserver(this, MOZSETTINGS_CHANGED);
  }
}

NS_IMPL_ISUPPORTS(AutoMounterSetting, nsIObserver)

const char *
AutoMounterSetting::StatusStr(int32_t aStatus)
{
  switch (aStatus) {
    case AUTOMOUNTER_STATUS_DISABLED:   return "Disabled";
    case AUTOMOUNTER_STATUS_ENABLED:    return "Enabled";
    case AUTOMOUNTER_STATUS_FILES_OPEN: return "FilesOpen";
  }
  return "??? Unknown ???";
}

class CheckVolumeSettingsRunnable : public nsRunnable
{
public:
  CheckVolumeSettingsRunnable(const nsACString& aVolumeName)
    : mVolumeName(aVolumeName) {}

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    nsCOMPtr<nsISettingsService> settingsService =
      do_GetService("@mozilla.org/settingsService;1");
    NS_ENSURE_TRUE(settingsService, NS_ERROR_FAILURE);
    nsCOMPtr<nsISettingsServiceLock> lock;
    settingsService->CreateLock(nullptr, getter_AddRefs(lock));
    nsCOMPtr<nsISettingsServiceCallback> callback =
      new CheckVolumeSettingsCallback(mVolumeName);
    nsPrintfCString setting(UMS_VOLUME_ENABLED_PREFIX "%s" UMS_VOLUME_ENABLED_SUFFIX,
                            mVolumeName.get());
    lock->Get(setting.get(), callback);
    return NS_OK;
  }

private:
  nsCString mVolumeName;
};


void
AutoMounterSetting::CheckVolumeSettings(const nsACString& aVolumeName)
{
  NS_DispatchToMainThread(new CheckVolumeSettingsRunnable(aVolumeName));
}

class SetStatusRunnable : public nsRunnable
{
public:
  SetStatusRunnable(int32_t aStatus) : mStatus(aStatus) {}

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    nsCOMPtr<nsISettingsService> settingsService =
      do_GetService("@mozilla.org/settingsService;1");
    NS_ENSURE_TRUE(settingsService, NS_ERROR_FAILURE);
    nsCOMPtr<nsISettingsServiceLock> lock;
    settingsService->CreateLock(nullptr, getter_AddRefs(lock));
    
    if (lock) {
      mozilla::AutoSafeJSContext cx;
      JS::Rooted<JS::Value> value(cx, JS::Int32Value(mStatus));
      lock->Set(UMS_STATUS, value, nullptr, nullptr);
    }
    return NS_OK;
  }

private:
  int32_t mStatus;
};


void
AutoMounterSetting::SetStatus(int32_t aStatus)
{
  if (aStatus != mStatus) {
    LOG("Changing status from '%s' to '%s'",
        StatusStr(mStatus), StatusStr(aStatus));
    mStatus = aStatus;
    NS_DispatchToMainThread(new SetStatusRunnable(aStatus));
  }
}

NS_IMETHODIMP
AutoMounterSetting::Observe(nsISupports* aSubject,
                            const char* aTopic,
                            const char16_t* aData)
{
  if (strcmp(aTopic, MOZSETTINGS_CHANGED) != 0) {
    return NS_OK;
  }

  
  
  
  
  

  AutoJSAPI jsapi;
  jsapi.Init();
  JSContext* cx = jsapi.cx();
  RootedDictionary<SettingChangeNotification> setting(cx);
  if (!WrappedJSToDictionary(cx, aSubject, setting)) {
    return NS_OK;
  }

  
  if (setting.mKey.EqualsASCII(UMS_MODE)) {
    if (!setting.mValue.isInt32()) {
      return NS_OK;
    }
    int32_t mode = setting.mValue.toInt32();
    SetAutoMounterMode(mode);
    return NS_OK;
  }

  
  if (StringBeginsWith(setting.mKey, NS_LITERAL_STRING(UMS_VOLUME_ENABLED_PREFIX)) &&
      StringEndsWith(setting.mKey, NS_LITERAL_STRING(UMS_VOLUME_ENABLED_SUFFIX))) {
    if (!setting.mValue.isBoolean()) {
      return NS_OK;
    }
    const size_t prefixLen = sizeof(UMS_VOLUME_ENABLED_PREFIX) - 1;
    const size_t suffixLen = sizeof(UMS_VOLUME_ENABLED_SUFFIX) - 1;
    nsDependentSubstring volumeName =
      Substring(setting.mKey, prefixLen, setting.mKey.Length() - prefixLen - suffixLen);
    bool isSharingEnabled = setting.mValue.toBoolean();
    SetAutoMounterSharingMode(NS_LossyConvertUTF16toASCII(volumeName), isSharingEnabled);
    return NS_OK;
  }

  return NS_OK;
}

}   
}   
