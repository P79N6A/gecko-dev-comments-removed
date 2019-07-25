



#include "base/message_loop.h"
#include "jsapi.h"
#include "mozilla/Attributes.h"
#include "mozilla/Hal.h"
#include "mozilla/Services.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIJSContextStack.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsJSUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "TimeSetting.h"
#include "xpcpublic.h"

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Time Setting" , ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "Time Setting" , ## args)

#define TIME_TIMEZONE       "time.timezone"
#define MOZSETTINGS_CHANGED "mozsettings-changed"

namespace mozilla {
namespace system {

class InitTimezoneCb MOZ_FINAL : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  InitTimezoneCb() {}

  NS_IMETHOD Handle(const nsAString &aName, const JS::Value &aResult, JSContext *aContext) {
    
    
    
    
    if (aResult.isNull()) {
      
      nsCString curTimezone = hal::GetTimezone();
      NS_ConvertUTF8toUTF16 utf16Str(curTimezone);
      JSString *jsStr = JS_NewUCStringCopyN(aContext, utf16Str.get(), utf16Str.Length());

      
      nsCOMPtr<nsISettingsServiceLock> lock;
      nsCOMPtr<nsISettingsService> settingsService =
        do_GetService("@mozilla.org/settingsService;1");
      if (!settingsService) {
        ERR("Failed to get settingsLock service!");
        return NS_OK;
      }
      settingsService->GetLock(getter_AddRefs(lock));
      lock->Set(TIME_TIMEZONE, STRING_TO_JSVAL(jsStr), nullptr, nullptr);
      return NS_OK;
    }

    
    if (aResult.isString()) {
      return TimeSetting::SetTimezone(aResult, aContext);
    }

    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString &aName, JSContext *aContext) {
    ERR("InitTimezoneCb::HandleError: %s\n", NS_LossyConvertUTF16toASCII(aName).get());
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(InitTimezoneCb, nsISettingsServiceCallback)

TimeSetting::TimeSetting()
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
  settingsService->GetLock(getter_AddRefs(lock));
  nsCOMPtr<nsISettingsServiceCallback> callback = new InitTimezoneCb();
  lock->Get(TIME_TIMEZONE, callback);
}

nsresult TimeSetting::SetTimezone(const JS::Value &aValue, JSContext *aContext)
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

TimeSetting::~TimeSetting()
{
  nsCOMPtr<nsIObserverService> observerService = services::GetObserverService();
  if (observerService) {
    observerService->RemoveObserver(this, MOZSETTINGS_CHANGED);
  }
}

NS_IMPL_ISUPPORTS1(TimeSetting, nsIObserver)

NS_IMETHODIMP
TimeSetting::Observe(nsISupports *aSubject,
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

  
  return SetTimezone(value, cx);
}

} 
} 
