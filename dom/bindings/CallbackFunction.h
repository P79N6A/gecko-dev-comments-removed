















#pragma once

#include "nsISupports.h"
#include "nsISupportsImpl.h"
#include "nsCycleCollectionParticipant.h"
#include "jsapi.h"
#include "jswrapper.h"
#include "mozilla/Assertions.h"
#include "mozilla/Util.h"
#include "nsContentUtils.h"
#include "nsWrapperCache.h"
#include "nsJSEnvironment.h"
#include "xpcpublic.h"
#include "nsLayoutStatics.h"

namespace mozilla {
namespace dom {

class CallbackFunction : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CallbackFunction)

  






  CallbackFunction(JSContext* cx, JSObject* aOwner, JSObject* aCallable,
                   bool* aInited)
    : mCallable(nullptr)
  {
    MOZ_ASSERT(JS_ObjectIsCallable(cx, aCallable));
    
    
    if (aOwner) {
      aOwner = js::UnwrapObject(aOwner);
      JSAutoCompartment ac(cx, aOwner);
      if (!JS_WrapObject(cx, &aCallable)) {
        *aInited = false;
        return;
      }
    }

    
    
    mCallable = aCallable;
    
    nsLayoutStatics::AddRef();
    NS_HOLD_JS_OBJECTS(this, CallbackFunction);
    *aInited = true;
  }

  virtual ~CallbackFunction()
  {
    DropCallback();
  }

  JSObject* Callable() const
  {
    xpc_UnmarkGrayObject(mCallable);
    return mCallable;
  }

  bool HasGrayCallable() const
  {
    
    return mCallable && xpc_IsGrayGCThing(mCallable);
  }

protected:
  explicit CallbackFunction(CallbackFunction* aCallbackFunction)
    : mCallable(aCallbackFunction->mCallable)
  {
    
    
    
    nsLayoutStatics::AddRef();
    NS_HOLD_JS_OBJECTS(this, CallbackFunction);
  }

  void DropCallback()
  {
    if (mCallable) {
      mCallable = nullptr;
      NS_DROP_JS_OBJECTS(this, CallbackFunction);
      nsLayoutStatics::Release();
    }
  }

  JSObject* mCallable;

  class NS_STACK_CLASS CallSetup
  {
    





  public:
    CallSetup(JSObject* const aCallable);
    ~CallSetup();

    JSContext* GetContext() const
    {
      return mCx;
    }

  private:
    
    CallSetup(const CallSetup&) MOZ_DELETE;

    
    JSContext* mCx;
    nsCOMPtr<nsIScriptContext> mCtx;

    

    
    
    nsAutoMicroTask mMt;

    
    
    Maybe<XPCAutoRequest> mAr;

    
    
    Maybe<nsJSContext::TerminationFuncHolder> mTerminationFuncHolder;

    nsCxPusher mCxPusher;

    
    
    
    
    Maybe<JSAutoCompartment> mAc;
  };
};

} 
} 
