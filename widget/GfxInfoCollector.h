






#ifndef __mozilla_widget_GfxInfoCollector_h__
#define __mozilla_widget_GfxInfoCollector_h__

#include "mozilla/Attributes.h"
#include "nsStringFwd.h"
#include "js/RootingAPI.h"

namespace mozilla {
namespace widget {




class MOZ_STACK_CLASS InfoObject
{
  friend class GfxInfoBase;

  public:
  void DefineProperty(const char *name, int value);
  void DefineProperty(const char *name, nsAString &value);
  void DefineProperty(const char *name, const char *value);

  private:
  
  explicit InfoObject(JSContext *aCx);
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
