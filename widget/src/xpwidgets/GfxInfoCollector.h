






































#ifndef __mozilla_widget_GfxInfoCollector_h__
#define __mozilla_widget_GfxInfoCollector_h__

#include "jsapi.h"

namespace mozilla {
namespace widget {




class InfoObject
{
  friend class GfxInfoBase;

  public:
  void DefineProperty(const char *name, int value)
  {
    if (!mOk)
      return;

    mOk = JS_DefineProperty(mCx, mObj, name, INT_TO_JSVAL(value), NULL, NULL, JSPROP_ENUMERATE);
  }

  void DefineProperty(const char *name, nsAString &value)
  {
    if (!mOk)
      return;

    const nsPromiseFlatString &flat = PromiseFlatString(value);
    JSString *string = JS_NewUCStringCopyN(mCx, static_cast<const jschar*>(flat.get()), flat.Length());
    if (!string)
      mOk = JS_FALSE;

    if (!mOk)
      return;

    mOk = JS_DefineProperty(mCx, mObj, name, STRING_TO_JSVAL(string), NULL, NULL, JSPROP_ENUMERATE);
  }

  void DefineProperty(const char *name, const char *value)
  { 
    nsAutoString string = NS_ConvertASCIItoUTF16(value);
    DefineProperty(name, string); 
  }

  private:
  
  InfoObject(JSContext *aCx) : mCx(aCx), mOk(JS_TRUE)
  {
    mObj = JS_NewObject(mCx, NULL, NULL, NULL);
    if (!mObj)
      mOk = JS_FALSE;
  }
  InfoObject(InfoObject&);

  JSContext *mCx;
  JSObject *mObj;
  JSBool mOk;
};




























class GfxInfoCollectorBase
{
  public:
  GfxInfoCollectorBase();
  virtual void GetInfo(InfoObject &obj) = 0;
  virtual ~GfxInfoCollectorBase();
};

template<class T>
class GfxInfoCollector : public GfxInfoCollectorBase
{
  public:
  GfxInfoCollector(T* aPointer, void (T::*aFunc)(InfoObject &obj)) : mPointer(aPointer), mFunc(aFunc) {
  }
  virtual void GetInfo(InfoObject &obj) {
    (mPointer->*mFunc)(obj);
  }

  protected:
  T* mPointer;
  void (T::*mFunc)(InfoObject &obj);

};

}
}

#endif
