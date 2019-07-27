






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

  mOk = JS_DefineProperty(mCx, mObj, name, value, JSPROP_ENUMERATE);
}

void
InfoObject::DefineProperty(const char *name, nsAString &value)
{
  if (!mOk)
    return;

  const nsString &flat = PromiseFlatString(value);
  JS::Rooted<JSString*> string(mCx, JS_NewUCStringCopyN(mCx, static_cast<const char16_t*>(flat.get()),
                                                        flat.Length()));
  if (!string)
    mOk = false;

  if (!mOk)
    return;

  mOk = JS_DefineProperty(mCx, mObj, name, string, JSPROP_ENUMERATE);
}

void
InfoObject::DefineProperty(const char *name, const char *value)
{
  nsAutoString string = NS_ConvertASCIItoUTF16(value);
  DefineProperty(name, string);
}

InfoObject::InfoObject(JSContext *aCx) : mCx(aCx), mObj(aCx), mOk(true)
{
  mObj = JS_NewObject(mCx, nullptr, JS::NullPtr(), JS::NullPtr());
  if (!mObj)
    mOk = false;
}
