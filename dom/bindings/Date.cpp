





#include "mozilla/dom/Date.h"

#include "jsapi.h" 
#include "jsfriendapi.h" 
#include "js/RootingAPI.h" 
#include "js/Value.h" 
#include "mozilla/FloatingPoint.h" 

namespace mozilla {
namespace dom {

Date::Date()
  : mMsecSinceEpoch(UnspecifiedNaN<double>())
{
}

bool
Date::IsUndefined() const
{
  return IsNaN(mMsecSinceEpoch);
}

bool
Date::SetTimeStamp(JSContext* aCx, JSObject* aObject)
{
  JS::Rooted<JSObject*> obj(aCx, aObject);
  MOZ_ASSERT(JS_ObjectIsDate(aCx, obj));
  mMsecSinceEpoch = js::DateGetMsecSinceEpoch(aCx, obj);
  return true;
}

bool
Date::ToDateObject(JSContext* aCx, JS::MutableHandle<JS::Value> aRval) const
{
  JSObject* obj = JS_NewDateObjectMsec(aCx, mMsecSinceEpoch);
  if (!obj) {
    return false;
  }

  aRval.setObject(*obj);
  return true;
}

} 
} 
