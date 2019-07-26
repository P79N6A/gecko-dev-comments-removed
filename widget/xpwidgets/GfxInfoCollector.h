






#ifndef __mozilla_widget_GfxInfoCollector_h__
#define __mozilla_widget_GfxInfoCollector_h__

#include "jsapi.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace widget {




class MOZ_STACK_CLASS InfoObject
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

    const nsString &flat = PromiseFlatString(value);
    JSString *string = JS_NewUCStringCopyN(mCx, static_cast<const jschar*>(flat.get()), flat.Length());
    if (!string)
      mOk = false;

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
  
  InfoObject(JSContext *aCx) : mCx(aCx), mObj(aCx), mOk(true)
  {
    mObj = JS_NewObject(mCx, NULL, NULL, NULL);
    if (!mObj)
      mOk = false;
  }
  InfoObject(InfoObject&);

  JSContext *mCx;
  JS::Rooted<JSObject*> mObj;
  bool mOk;
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
