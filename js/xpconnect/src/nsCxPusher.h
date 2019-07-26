


#ifndef nsCxPusher_h___
#define nsCxPusher_h___

#include "jsapi.h"
#include "mozilla/Util.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
class EventTarget;
}
}

class nsIScriptContext;

class MOZ_STACK_CLASS nsCxPusher
{
public:
  NS_EXPORT nsCxPusher();
  NS_EXPORT ~nsCxPusher(); 

  
  bool Push(mozilla::dom::EventTarget *aCurrentTarget);
  
  
  bool RePush(mozilla::dom::EventTarget *aCurrentTarget);
  
  
  NS_EXPORT_(void) Push(JSContext *cx);
  
  void PushNull();

  
  NS_EXPORT_(void) Pop();

  nsIScriptContext* GetCurrentScriptContext() { return mScx; }
private:
  
  void DoPush(JSContext* cx);

  nsCOMPtr<nsIScriptContext> mScx;
  bool mScriptIsRunning;
  bool mPushedSomething;
#ifdef DEBUG
  JSContext* mPushedContext;
  unsigned mCompartmentDepthOnEntry;
#endif
};

namespace mozilla {






class MOZ_STACK_CLASS AutoJSContext {
public:
  AutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
  operator JSContext*();

protected:
  AutoJSContext(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

private:
  
  
  
  void Init(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

  JSContext* mCx;
  nsCxPusher mPusher;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};





class MOZ_STACK_CLASS AutoSafeJSContext : public AutoJSContext {
public:
  AutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
};















class MOZ_STACK_CLASS AutoPushJSContext {
  nsCxPusher mPusher;
  JSContext* mCx;

public:
  AutoPushJSContext(JSContext* aCx);
  operator JSContext*() { return mCx; }
};

} 

#endif
