



#include "base/message_loop.h"
#include "jsapi.h"
#include "mozilla/Attributes.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Hal.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIJSContextStack.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsJSUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "TimeZoneSettingObserver.h"
#include "xpcpublic.h"
#include "nsContentUtils.h"

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Time Zone Setting" , ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "Time Zone Setting" , ## args)

#define TIME_TIMEZONE       "time.timezone"
#define MOZSETTINGS_CHANGED "mozsettings-changed"

using namespace mozilla;

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

  NS_IMETHOD Handle(const nsAString &aName, const JS::Value &aResult) {

    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    NS_ENSURE_TRUE(cx, NS_OK);

    
    
    
    
    if (aResult.isNull()) {
      
      nsCString curTimezone = hal::GetTimezone();
      NS_ConvertUTF8toUTF16 utf16Str(curTimezone);
      JSString *jsStr = JS_NewUCStringCopyN(cx, utf16Str.get(), utf16Str.Length());

      
      nsCOMPtr<nsISettingsServiceLock> lock;
      nsCOMPtr<nsISettingsService> settingsService =
        do_GetService("@mozilla.org/settingsService;1");
      if (!settingsService) {
        ERR("Failed to get settingsLock service!");
        return NS_OK;
      }
      settingsService->CreateLock(getter_AddRefs(lock));
      lock->Set(TIME_TIMEZONE, STRING_TO_JSVAL(jsStr), nullptr, nullptr);
      return NS_OK;
    }

    
    if (aResult.isString()) {
      JSAutoRequest ar(cx);
      JSAutoCompartment ac(cx, aResult.toString());
      return TimeZoneSettingObserver::SetTimeZone(aResult, cx);
    }

    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString &aName) {
    ERR("TimeZoneSettingCb::HandleError: %s\n", NS_LossyConvertUTF16toASCII(aName).get());
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(TimeZoneSettingCb, nsISettingsServiceCallback)

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
  settingsService->CreateLock(getter_AddRefs(lock));
  nsCOMPtr<nsISettingsServiceCallback> callback = new TimeZoneSettingCb();
  lock->Get(TIME_TIMEZONE, callback);
}

nsresult TimeZoneSettingObserver::SetTimeZone(const JS::Value &aValue, JSContext *aContext)
{
  
  nsDependentJSString valueStr;
  if (!valueStr.init(aContext, aValue.toString())) {
    ERR("Failed to convert JS value to nsCString");
    return NS_ERROR_FAILURE;
  }
  nsCString newTimezone = NS_ConvertUTF16toUTF8(valueStr);

  
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

NS_IMPL_ISUPPORTS1(TimeZoneSettingObserver, nsIObserver)

NS_IMETHODIMP
TimeZoneSettingObserver::Observe(nsISupports *aSubject,
                     const char *aTopic,
                     const PRUnichar *aData)
{
  if (strcmp(aTopic, MOZSETTINGS_CHANGED) != 0) {
    return NS_OK;
  }

  
  
  
  
  

  
  nsCOMPtr<nsIThreadJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (!stack) {
    ERR("Failed to get JSContextStack");
    return NS_OK;
  }
  JSContext *cx = stack->GetSafeJSContext();
  if (!cx) {
    ERR("Failed to GetSafeJSContext");
    return NS_OK;
  }

  
  nsDependentString dataStr(aData);
  JS::Value val;
  if (!JS_ParseJSON(cx, dataStr.get(), dataStr.Length(), &val) ||
      !val.isObject()) {
    return NS_OK;
  }

  
  JSObject &obj(val.toObject());
  JS::Value key;
  if (!JS_GetProperty(cx, &obj, "key", &key) ||
      !key.isString()) {
    return NS_OK;
  }
  JSBool match;
  if (!JS_StringEqualsAscii(cx, key.toString(), TIME_TIMEZONE, &match) ||
      match != JS_TRUE) {
    return NS_OK;
  }

  
  JS::Value value;
  if (!JS_GetProperty(cx, &obj, "value", &value) ||
      !value.isString()) {
    return NS_OK;
  }

  
  return SetTimeZone(value, cx);
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
