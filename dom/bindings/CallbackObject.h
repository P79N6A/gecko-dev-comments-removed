















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

  







  JSObject* CallbackPreserveColor() const
  {
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

template<class WebIDLCallbackT, class XPCOMCallbackT>
class CallbackObjectHolder;

template<class T, class U>
void ImplCycleCollectionUnlink(CallbackObjectHolder<T, U>& aField);

class CallbackObjectHolderBase
{
protected:
  
  already_AddRefed<nsISupports> ToXPCOMCallback(CallbackObject* aCallback,
                                                const nsIID& aIID);
};

template<class WebIDLCallbackT, class XPCOMCallbackT>
class CallbackObjectHolder : CallbackObjectHolderBase
{
  








public:
  explicit CallbackObjectHolder(WebIDLCallbackT* aCallback)
    : mPtrBits(reinterpret_cast<uintptr_t>(aCallback))
  {
    NS_IF_ADDREF(aCallback);
  }

  explicit CallbackObjectHolder(XPCOMCallbackT* aCallback)
    : mPtrBits(reinterpret_cast<uintptr_t>(aCallback) | XPCOMCallbackFlag)
  {
    NS_IF_ADDREF(aCallback);
  }

  explicit CallbackObjectHolder(const CallbackObjectHolder& aOther)
    : mPtrBits(aOther.mPtrBits)
  {
    NS_IF_ADDREF(GetISupports());
  }

  CallbackObjectHolder()
    : mPtrBits(0)
  {}

  ~CallbackObjectHolder()
  {
    UnlinkSelf();
  }

  nsISupports* GetISupports() const
  {
    return reinterpret_cast<nsISupports*>(mPtrBits & ~XPCOMCallbackFlag);
  }

  
  
  bool HasWebIDLCallback() const
  {
    return !(mPtrBits & XPCOMCallbackFlag);
  }

  WebIDLCallbackT* GetWebIDLCallback() const
  {
    MOZ_ASSERT(HasWebIDLCallback());
    return reinterpret_cast<WebIDLCallbackT*>(mPtrBits);
  }

  XPCOMCallbackT* GetXPCOMCallback() const
  {
    MOZ_ASSERT(!HasWebIDLCallback());
    return reinterpret_cast<XPCOMCallbackT*>(mPtrBits & ~XPCOMCallbackFlag);
  }

  bool operator==(WebIDLCallbackT* aOtherCallback) const
  {
    if (!aOtherCallback) {
      
      return !GetISupports();
    }

    if (!HasWebIDLCallback() || !GetWebIDLCallback()) {
      
      
      return false;
    }

    JSObject* thisObj =
      js::UnwrapObject(GetWebIDLCallback()->CallbackPreserveColor());
    JSObject* otherObj =
      js::UnwrapObject(aOtherCallback->CallbackPreserveColor());
    return thisObj == otherObj;
  }

  bool operator==(XPCOMCallbackT* aOtherCallback) const
  {
    return (!aOtherCallback && !GetISupports()) ||
      (!HasWebIDLCallback() && GetXPCOMCallback() == aOtherCallback);
  }

  bool operator==(const CallbackObjectHolder& aOtherCallback) const
  {
    if (aOtherCallback.HasWebIDLCallback()) {
      return *this == aOtherCallback.GetWebIDLCallback();
    }

    return *this == aOtherCallback.GetXPCOMCallback();
  }

  
  already_AddRefed<XPCOMCallbackT> ToXPCOMCallback()
  {
    if (!HasWebIDLCallback()) {
      nsRefPtr<XPCOMCallbackT> callback = GetXPCOMCallback();
      return callback.forget();
    }

    nsCOMPtr<nsISupports> supp =
      CallbackObjectHolderBase::ToXPCOMCallback(GetWebIDLCallback(),
                                                NS_GET_TEMPLATE_IID(XPCOMCallbackT));
    
    return static_cast<XPCOMCallbackT*>(supp.forget().get());
  }

  
  already_AddRefed<WebIDLCallbackT> ToWebIDLCallback()
  {
    if (HasWebIDLCallback()) {
      nsRefPtr<WebIDLCallbackT> callback = GetWebIDLCallback();
      return callback.forget();
    }

    XPCOMCallbackT* callback = GetXPCOMCallback();
    if (!callback) {
      return nullptr;
    }

    nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS = do_QueryInterface(callback);
    if (!wrappedJS) {
      return nullptr;
    }

    JSObject* obj;
    if (NS_FAILED(wrappedJS->GetJSObject(&obj)) || !obj) {
      return nullptr;
    }

    SafeAutoJSContext cx;
    JSAutoCompartment ac(cx, obj);

    bool inited;
    nsRefPtr<WebIDLCallbackT> newCallback =
      new WebIDLCallbackT(cx, nullptr, obj, &inited);
    if (!inited) {
      return nullptr;
    }
    return newCallback.forget();
  }

private:
  static const uintptr_t XPCOMCallbackFlag = 1u;

  friend void
  ImplCycleCollectionUnlink<WebIDLCallbackT,
                            XPCOMCallbackT>(CallbackObjectHolder& aField);

  void UnlinkSelf()
  {
    
    nsISupports* ptr = GetISupports();
    NS_IF_RELEASE(ptr);
    mPtrBits = 0;
  }

  uintptr_t mPtrBits;
};

template<class T, class U>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            CallbackObjectHolder<T, U>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteChild(aCallback, aField.GetISupports(), aName, aFlags);
}

template<class T, class U>
void
ImplCycleCollectionUnlink(CallbackObjectHolder<T, U>& aField)
{
  aField.UnlinkSelf();
}

} 
} 

#endif 
