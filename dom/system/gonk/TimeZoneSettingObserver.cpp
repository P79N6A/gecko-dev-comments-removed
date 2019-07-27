



#include "base/message_loop.h"
#include "jsapi.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/Attributes.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Hal.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsJSUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "TimeZoneSettingObserver.h"
#include "xpcpublic.h"
#include "nsContentUtils.h"
#include "nsPrintfCString.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/SettingChangeNotificationBinding.h"

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Time Zone Setting" , ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "Time Zone Setting" , ## args)

#define TIME_TIMEZONE       "time.timezone"
#define MOZSETTINGS_CHANGED "mozsettings-changed"

using namespace mozilla;
using namespace mozilla::dom;

namespace {

class TimeZoneSettingObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  TimeZoneSettingObserver();
  virtual ~TimeZoneSettingObserver();
  static nsresult SetTimeZone(const JS::Value &aValue, JSContext *aContext);
};

class TimeZoneSettingCb MOZ_FINAL : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  TimeZoneSettingCb() {}

  NS_IMETHOD Handle(const nsAString &aName, JS::Handle<JS::Value> aResult) {

    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    NS_ENSURE_TRUE(cx, NS_OK);

    
    
    
    
    if (aResult.isNull()) {
      
      
      
      
      int32_t timeZoneOffset = hal::GetTimezoneOffset();
      nsPrintfCString curTimeZone("UTC%+03d:%02d",
                                  -timeZoneOffset / 60,
                                  abs(timeZoneOffset) % 60);

      
      NS_ConvertUTF8toUTF16 utf16Str(curTimeZone);

      JS::Rooted<JSString*> jsStr(cx, JS_NewUCStringCopyN(cx,
                                                          utf16Str.get(),
                                                          utf16Str.Length()));
      if (!jsStr) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      
      nsCOMPtr<nsISettingsServiceLock> lock;
      nsCOMPtr<nsISettingsService> settingsService =
        do_GetService("@mozilla.org/settingsService;1");
      if (!settingsService) {
        ERR("Failed to get settingsLock service!");
        return NS_OK;
      }
      settingsService->CreateLock(nullptr, getter_AddRefs(lock));
      JS::Rooted<JS::Value> value(cx, JS::StringValue(jsStr));
      lock->Set(TIME_TIMEZONE, value, nullptr, nullptr);
      return NS_OK;
    }

    
    if (aResult.isString()) {
      return TimeZoneSettingObserver::SetTimeZone(aResult, cx);
    }

    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString &aName) {
    ERR("TimeZoneSettingCb::HandleError: %s\n", NS_LossyConvertUTF16toASCII(aName).get());
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS(TimeZoneSettingCb, nsISettingsServiceCallback)

TimeZoneSettingObserver::TimeZoneSettingObserver()
{
  
  nsCOMPtr<nsIObserverService> observerService = services::GetObserverService();
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

  
  
  nsCOMPtr<nsISettingsServiceLock> lock;
  nsCOMPtr<nsISettingsService> settingsService =
    do_GetService("@mozilla.org/settingsService;1");
  if (!settingsService) {
    ERR("Failed to get settingsLock service!");
    return;
  }
  settingsService->CreateLock(nullptr, getter_AddRefs(lock));
  nsCOMPtr<nsISettingsServiceCallback> callback = new TimeZoneSettingCb();
  lock->Get(TIME_TIMEZONE, callback);
}

nsresult TimeZoneSettingObserver::SetTimeZone(const JS::Value &aValue, JSContext *aContext)
{
  
  
  nsAutoJSString valueStr;
  if (!valueStr.init(aContext, aValue.toString())) {
    ERR("Failed to convert JS value to nsCString");
    return NS_ERROR_FAILURE;
  }
  NS_ConvertUTF16toUTF8 newTimezone(valueStr);

  
  
  if (newTimezone.Find(NS_LITERAL_CSTRING("UTC+")) == 0) {
    if (!newTimezone.SetCharAt('-', 3)) {
      return NS_ERROR_FAILURE;
    }
  } else if (newTimezone.Find(NS_LITERAL_CSTRING("UTC-")) == 0) {
    if (!newTimezone.SetCharAt('+', 3)) {
      return NS_ERROR_FAILURE;
    }
  }

  
  nsCString curTimezone = hal::GetTimezone();
  if (!curTimezone.Equals(newTimezone)) {
    hal::SetTimezone(newTimezone);
  }

  return NS_OK;
}

TimeZoneSettingObserver::~TimeZoneSettingObserver()
{
  nsCOMPtr<nsIObserverService> observerService = services::GetObserverService();
  if (observerService) {
    observerService->RemoveObserver(this, MOZSETTINGS_CHANGED);
  }
}

NS_IMPL_ISUPPORTS(TimeZoneSettingObserver, nsIObserver)

NS_IMETHODIMP
TimeZoneSettingObserver::Observe(nsISupports *aSubject,
                                 const char *aTopic,
                                 const char16_t *aData)
{
  if (strcmp(aTopic, MOZSETTINGS_CHANGED) != 0) {
    return NS_OK;
  }

  
  
  
  
  
  

  AutoSafeJSContext cx;
  RootedDictionary<SettingChangeNotification> setting(cx);
  if (!WrappedJSToDictionary(cx, aSubject, setting)) {
    return NS_OK;
  }
  if (!setting.mKey.EqualsASCII(TIME_TIMEZONE)) {
    return NS_OK;
  }
  if (!setting.mValue.isString()) {
    return NS_OK;
  }

  
  return SetTimeZone(setting.mValue, cx);
}

} 

static mozilla::StaticRefPtr<TimeZoneSettingObserver> sTimeZoneSettingObserver;
namespace mozilla {
namespace system {
void
InitializeTimeZoneSettingObserver()
{
  sTimeZoneSettingObserver = new TimeZoneSettingObserver();
  ClearOnShutdown(&sTimeZoneSettingObserver);
}

} 
} 
