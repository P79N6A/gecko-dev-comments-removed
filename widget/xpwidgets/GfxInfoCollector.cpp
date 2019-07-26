






#include "GfxInfoCollector.h"
#include "jsapi.h"
#include "nsString.h"

using namespace mozilla;
using namespace widget;

void
InfoObject::DefineProperty(const char *name, int value)
{
  if (!mOk)
    return;

  mOk = JS_DefineProperty(mCx, mObj, name, INT_TO_JSVAL(value),
                          nullptr, nullptr, JSPROP_ENUMERATE);
}

void
InfoObject::DefineProperty(const char *name, nsAString &value)
{
  if (!mOk)
    return;

  const nsString &flat = PromiseFlatString(value);
  JSString *string = JS_NewUCStringCopyN(mCx, static_cast<const jschar*>(flat.get()), flat.Length());
  if (!string)
    mOk = false;

  if (!mOk)
    return;

  mOk = JS_DefineProperty(mCx, mObj, name, STRING_TO_JSVAL(string),
                          nullptr, nullptr, JSPROP_ENUMERATE);
}

void
InfoObject::DefineProperty(const char *name, const char *value)
{
  nsAutoString string = NS_ConvertASCIItoUTF16(value);
  DefineProperty(name, string);
}

InfoObject::InfoObject(JSContext *aCx) : mCx(aCx), mObj(aCx), mOk(true)
{
  mObj = JS_NewObject(mCx, nullptr, nullptr, nullptr);
  if (!mObj)
    mOk = false;
}
