





#ifndef mozilla_dom_RootedDictionary_h__
#define mozilla_dom_RootedDictionary_h__

#include "mozilla/GuardObjects.h"
#include "mozilla/dom/Nullable.h"
#include "jsapi.h"

namespace mozilla {
namespace dom {

template<typename T>
class MOZ_STACK_CLASS RootedDictionary : public T,
                                         private JS::CustomAutoRooter
{
public:
  explicit RootedDictionary(JSContext* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) :
    T(),
    JS::CustomAutoRooter(cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
  {
  }

  virtual void trace(JSTracer *trc) MOZ_OVERRIDE
  {
    this->TraceDictionary(trc);
  }
};

template<typename T>
class MOZ_STACK_CLASS NullableRootedDictionary : public Nullable<T>,
                                                 private JS::CustomAutoRooter
{
public:
  explicit NullableRootedDictionary(JSContext* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) :
    Nullable<T>(),
    JS::CustomAutoRooter(cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
  {
  }

  virtual void trace(JSTracer *trc) MOZ_OVERRIDE
  {
    if (!this->IsNull()) {
      this->Value().TraceDictionary(trc);
    }
  }
};

} 
} 

#endif 
