















#ifndef mozilla_dom_CallbackObject_h
#define mozilla_dom_CallbackObject_h

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

class CallbackObject : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CallbackObject)

  






  CallbackObject(JSContext* cx, JSObject* aOwner, JSObject* aCallback,
                 bool* aInited)
    : mCallback(nullptr)
  {
    
    
    if (aOwner) {
      aOwner = js::UnwrapObject(aOwner);
      JSAutoCompartment ac(cx, aOwner);
      if (!JS_WrapObject(cx, &aCallback)) {
        *aInited = false;
        return;
      }
    }

    
    
    mCallback = aCallback;
    
    nsLayoutStatics::AddRef();
    NS_HOLD_JS_OBJECTS(this, CallbackObject);
    *aInited = true;
  }

  virtual ~CallbackObject()
  {
    DropCallback();
  }

  JSObject* Callback() const
  {
    xpc_UnmarkGrayObject(mCallback);
    return mCallback;
  }

  enum ExceptionHandling {
    eReportExceptions,
    eRethrowExceptions
  };

protected:
  explicit CallbackObject(CallbackObject* aCallbackFunction)
    : mCallback(aCallbackFunction->mCallback)
  {
    
    
    
    nsLayoutStatics::AddRef();
    NS_HOLD_JS_OBJECTS(this, CallbackObject);
  }

  void DropCallback()
  {
    if (mCallback) {
      mCallback = nullptr;
      NS_DROP_JS_OBJECTS(this, CallbackObject);
      nsLayoutStatics::Release();
    }
  }

  JSObject* mCallback;

  class NS_STACK_CLASS CallSetup
  {
    





  public:
    CallSetup(JSObject* const aCallable, ErrorResult& aRv,
              ExceptionHandling aExceptionHandling);
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

    
    
    ErrorResult& mErrorResult;
    const ExceptionHandling mExceptionHandling;
    uint32_t mSavedJSContextOptions;
  };
};

} 
} 

#endif 
