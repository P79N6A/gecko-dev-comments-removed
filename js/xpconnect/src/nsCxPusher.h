





#ifndef nsCxPusher_h
#define nsCxPusher_h

#include "jsapi.h"
#include "mozilla/Maybe.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
class EventTarget;
}
}

class nsIScriptContext;

namespace mozilla {





class MOZ_STACK_CLASS AutoCxPusher
{
public:
  explicit AutoCxPusher(JSContext *aCx, bool aAllowNull = false);
  
  ~AutoCxPusher();

  nsIScriptContext* GetScriptContext() { return mScx; }

  
  
  bool IsStackTop() const;

private:
  mozilla::Maybe<JSAutoRequest> mAutoRequest;
  mozilla::Maybe<JSAutoCompartment> mAutoCompartment;
  nsCOMPtr<nsIScriptContext> mScx;
  uint32_t mStackDepthAfterPush;
#ifdef DEBUG
  JSContext* mPushedContext;
  unsigned mCompartmentDepthOnEntry;
#endif
};

} 













class MOZ_STACK_CLASS nsCxPusher
{
public:
  
  bool Push(mozilla::dom::EventTarget *aCurrentTarget);
  
  
  bool RePush(mozilla::dom::EventTarget *aCurrentTarget);
  
  
  void Push(JSContext *cx);
  
  void PushNull();

  
  void Pop();

  nsIScriptContext* GetCurrentScriptContext() {
    return mPusher.empty() ? nullptr : mPusher.ref().GetScriptContext();
  }

private:
  mozilla::Maybe<mozilla::AutoCxPusher> mPusher;
};

namespace mozilla {






class MOZ_STACK_CLASS AutoJSContext {
public:
  explicit AutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
  operator JSContext*() const;

protected:
  explicit AutoJSContext(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

  
  
  
  void Init(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

  JSContext* mCx;
  Maybe<AutoCxPusher> mPusher;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};





class MOZ_STACK_CLASS ThreadsafeAutoJSContext {
public:
  explicit ThreadsafeAutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
  operator JSContext*() const;

private:
  JSContext* mCx; 
  Maybe<JSAutoRequest> mRequest; 
  Maybe<AutoJSContext> mAutoJSContext; 
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};





class MOZ_STACK_CLASS AutoSafeJSContext : public AutoJSContext {
public:
  explicit AutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
private:
  JSAutoCompartment mAc;
};




class MOZ_STACK_CLASS ThreadsafeAutoSafeJSContext {
public:
  explicit ThreadsafeAutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM);
  operator JSContext*() const;

private:
  JSContext* mCx; 
  Maybe<JSAutoRequest> mRequest; 
  Maybe<AutoSafeJSContext> mAutoSafeJSContext; 
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};















class MOZ_STACK_CLASS AutoPushJSContext {
  Maybe<AutoCxPusher> mPusher;
  JSContext* mCx;

public:
  explicit AutoPushJSContext(JSContext* aCx);
  operator JSContext*() { return mCx; }
};

} 

#endif
