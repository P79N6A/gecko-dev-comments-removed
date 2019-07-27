















#ifndef mozilla_dom_CallbackObject_h
#define mozilla_dom_CallbackObject_h

#include "nsISupports.h"
#include "nsISupportsImpl.h"
#include "nsCycleCollectionParticipant.h"
#include "jswrapper.h"
#include "mozilla/Assertions.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/HoldDropJSObjects.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsContentUtils.h"
#include "nsWrapperCache.h"
#include "nsJSEnvironment.h"
#include "xpcpublic.h"

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

  
  
  
  
  explicit CallbackObject(JS::Handle<JSObject*> aCallback, nsIGlobalObject *aIncumbentGlobal)
  {
    Init(aCallback, aIncumbentGlobal);
  }

  JS::Handle<JSObject*> Callback() const
  {
    JS::ExposeObjectToActiveJS(mCallback);
    return CallbackPreserveColor();
  }

  







  JS::Handle<JSObject*> CallbackPreserveColor() const
  {
    
    
    return JS::Handle<JSObject*>::fromMarkedLocation(mCallback.address());
  }

  nsIGlobalObject* IncumbentGlobalOrNull() const
  {
    return mIncumbentGlobal;
  }

  enum ExceptionHandling {
    
    eReportExceptions,
    
    
    
    eRethrowContentExceptions,
    
    
    
    
    eRethrowExceptions
  };

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this);
  }

protected:
  virtual ~CallbackObject()
  {
    DropJSObjects();
  }

  explicit CallbackObject(CallbackObject* aCallbackObject)
  {
    Init(aCallbackObject->mCallback, aCallbackObject->mIncumbentGlobal);
  }

  bool operator==(const CallbackObject& aOther) const
  {
    JSObject* thisObj =
      js::UncheckedUnwrap(CallbackPreserveColor());
    JSObject* otherObj =
      js::UncheckedUnwrap(aOther.CallbackPreserveColor());
    return thisObj == otherObj;
  }

private:
  inline void Init(JSObject* aCallback, nsIGlobalObject* aIncumbentGlobal)
  {
    MOZ_ASSERT(aCallback && !mCallback);
    
    
    mCallback = aCallback;
    if (aIncumbentGlobal) {
      mIncumbentGlobal = aIncumbentGlobal;
      mIncumbentJSGlobal = aIncumbentGlobal->GetGlobalJSObject();
    }
    mozilla::HoldJSObjects(this);
  }

  CallbackObject(const CallbackObject&) = delete;
  CallbackObject& operator =(const CallbackObject&) = delete;

protected:
  void DropJSObjects()
  {
    MOZ_ASSERT_IF(mIncumbentJSGlobal, mCallback);
    if (mCallback) {
      mCallback = nullptr;
      mIncumbentJSGlobal = nullptr;
      mozilla::DropJSObjects(this);
    }
  }

  JS::Heap<JSObject*> mCallback;
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIGlobalObject> mIncumbentGlobal;
  JS::TenuredHeap<JSObject*> mIncumbentJSGlobal;

  class MOZ_STACK_CLASS CallSetup
  {
    





  public:
    
    
    
    
    
    
    
    CallSetup(CallbackObject* aCallback, ErrorResult& aRv,
              const char* aExecutionReason,
              ExceptionHandling aExceptionHandling,
              JSCompartment* aCompartment = nullptr,
              bool aIsJSImplementedWebIDL = false);
    ~CallSetup();

    JSContext* GetContext() const
    {
      return mCx;
    }

  private:
    
    CallSetup(const CallSetup&) = delete;

    bool ShouldRethrowException(JS::Handle<JS::Value> aException);

    
    JSContext* mCx;

    
    
    JSCompartment* mCompartment;

    
    Maybe<AutoEntryScript> mAutoEntryScript;
    Maybe<AutoIncumbentScript> mAutoIncumbentScript;

    
    
    Maybe<JS::Rooted<JSObject*> > mRootedCallable;

    
    
    
    
    Maybe<JSAutoCompartment> mAc;

    
    
    ErrorResult& mErrorResult;
    const ExceptionHandling mExceptionHandling;
    JS::ContextOptions mSavedJSContextOptions;
    const bool mIsMainThread;
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

  
  explicit operator bool() const
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

    return *GetWebIDLCallback() == *aOtherCallback;
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
    
    return supp.forget().downcast<XPCOMCallbackT>();
  }

  
  already_AddRefed<WebIDLCallbackT> ToWebIDLCallback() const
  {
    if (HasWebIDLCallback()) {
      nsRefPtr<WebIDLCallbackT> callback = GetWebIDLCallback();
      return callback.forget();
    }
    return nullptr;
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
