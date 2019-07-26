















#ifndef mozilla_dom_CallbackObject_h
#define mozilla_dom_CallbackObject_h

#include "nsISupports.h"
#include "nsISupportsImpl.h"
#include "nsCycleCollectionParticipant.h"
#include "jsapi.h"
#include "jswrapper.h"
#include "mozilla/Assertions.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/Util.h"
#include "nsContentUtils.h" 
#include "nsWrapperCache.h"
#include "nsJSEnvironment.h"
#include "xpcpublic.h"
#include "nsLayoutStatics.h"
#include "js/RootingAPI.h"

namespace mozilla {
namespace dom {

#define DOM_CALLBACKOBJECT_IID \
{ 0xbe74c190, 0x6d76, 0x4991, \
 { 0x84, 0xb9, 0x65, 0x06, 0x99, 0xe6, 0x93, 0x2b } }

class CallbackObject : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(DOM_CALLBACKOBJECT_IID)

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CallbackObject)

  explicit CallbackObject(JSObject* aCallback)
  {
    Init(aCallback);
  }

  virtual ~CallbackObject()
  {
    DropCallback();
  }

  JS::Handle<JSObject*> Callback() const
  {
    xpc_UnmarkGrayObject(mCallback);
    return CallbackPreserveColor();
  }

  









  JS::Handle<JSObject*> CallbackPreserveColor() const
  {
    return JS::Handle<JSObject*>::fromMarkedLocation(&mCallback);
  }

  enum ExceptionHandling {
    eReportExceptions,
    eRethrowExceptions
  };

protected:
  explicit CallbackObject(CallbackObject* aCallbackObject)
  {
    Init(aCallbackObject->mCallback);
  }

private:
  inline void Init(JSObject* aCallback)
  {
    
    
    mCallback = aCallback;
    
    nsLayoutStatics::AddRef();
    NS_HOLD_JS_OBJECTS(this, CallbackObject);
  }

protected:
  void DropCallback()
  {
    if (mCallback) {
      mCallback = nullptr;
      NS_DROP_JS_OBJECTS(this, CallbackObject);
      nsLayoutStatics::Release();
    }
  }

  JSObject* mCallback;

  class MOZ_STACK_CLASS CallSetup
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
                                                const nsIID& aIID) const;
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

  void operator=(WebIDLCallbackT* aCallback)
  {
    UnlinkSelf();
    mPtrBits = reinterpret_cast<uintptr_t>(aCallback);
    NS_IF_ADDREF(aCallback);
  }

  void operator=(XPCOMCallbackT* aCallback)
  {
    UnlinkSelf();
    mPtrBits = reinterpret_cast<uintptr_t>(aCallback) | XPCOMCallbackFlag;
    NS_IF_ADDREF(aCallback);
  }

  void operator=(const CallbackObjectHolder& aOther)
  {
    UnlinkSelf();
    mPtrBits = aOther.mPtrBits;
    NS_IF_ADDREF(GetISupports());
  }

  nsISupports* GetISupports() const
  {
    return reinterpret_cast<nsISupports*>(mPtrBits & ~XPCOMCallbackFlag);
  }

  
  operator bool() const
  {
    return GetISupports();
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
      js::UncheckedUnwrap(GetWebIDLCallback()->CallbackPreserveColor());
    JSObject* otherObj =
      js::UncheckedUnwrap(aOtherCallback->CallbackPreserveColor());
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

  
  already_AddRefed<XPCOMCallbackT> ToXPCOMCallback() const
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

  
  already_AddRefed<WebIDLCallbackT> ToWebIDLCallback() const
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

    nsRefPtr<WebIDLCallbackT> newCallback = new WebIDLCallbackT(obj);
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

NS_DEFINE_STATIC_IID_ACCESSOR(CallbackObject, DOM_CALLBACKOBJECT_IID)

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
