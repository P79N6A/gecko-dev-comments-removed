




#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"
#include "xpcpublic.h"
#include "nsIDOMEventListener.h"
#include "nsIAtom.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/EventHandlerBinding.h"

#define NS_IJSEVENTLISTENER_IID \
{ 0x92f9212b, 0xa6aa, 0x4867, \
  { 0x93, 0x8a, 0x56, 0xbe, 0x17, 0x67, 0x4f, 0xd4 } }

class nsEventHandler
{
public:
  typedef mozilla::dom::EventHandlerNonNull EventHandlerNonNull;
  typedef mozilla::dom::OnBeforeUnloadEventHandlerNonNull
    OnBeforeUnloadEventHandlerNonNull;
  typedef mozilla::dom::OnErrorEventHandlerNonNull OnErrorEventHandlerNonNull;
  typedef mozilla::dom::CallbackFunction CallbackFunction;

  enum HandlerType {
    eUnset = 0,
    eNormal = 0x1,
    eOnError = 0x2,
    eOnBeforeUnload = 0x3,
    eTypeBits = 0x3
  };

  nsEventHandler() :
    mBits(0)
  {}

  nsEventHandler(EventHandlerNonNull* aHandler)
  {
    Assign(aHandler, eNormal);
  }

  nsEventHandler(OnErrorEventHandlerNonNull* aHandler)
  {
    Assign(aHandler, eOnError);
  }

  nsEventHandler(OnBeforeUnloadEventHandlerNonNull* aHandler)
  {
    Assign(aHandler, eOnBeforeUnload);
  }

  nsEventHandler(const nsEventHandler& aOther)
  {
    if (aOther.HasEventHandler()) {
      
      Assign(aOther.Ptr(), aOther.Type());
    } else {
      mBits = 0;
    }
  }

  ~nsEventHandler()
  {
    ReleaseHandler();
  }

  HandlerType Type() const {
    return HandlerType(mBits & eTypeBits);
  }

  bool HasEventHandler() const
  {
    return !!Ptr();
  }

  void SetHandler(const nsEventHandler& aHandler)
  {
    if (aHandler.HasEventHandler()) {
      ReleaseHandler();
      Assign(aHandler.Ptr(), aHandler.Type());
    } else {
      ForgetHandler();
    }
  }

  EventHandlerNonNull* EventHandler() const
  {
    MOZ_ASSERT(Type() == eNormal && Ptr());
    return reinterpret_cast<EventHandlerNonNull*>(Ptr());
  }

  void SetHandler(EventHandlerNonNull* aHandler)
  {
    ReleaseHandler();
    Assign(aHandler, eNormal);
  }

  OnBeforeUnloadEventHandlerNonNull* OnBeforeUnloadEventHandler() const
  {
    MOZ_ASSERT(Type() == eOnBeforeUnload);
    return reinterpret_cast<OnBeforeUnloadEventHandlerNonNull*>(Ptr());
  }

  void SetHandler(OnBeforeUnloadEventHandlerNonNull* aHandler)
  {
    ReleaseHandler();
    Assign(aHandler, eOnBeforeUnload);
  }

  OnErrorEventHandlerNonNull* OnErrorEventHandler() const
  {
    MOZ_ASSERT(Type() == eOnError);
    return reinterpret_cast<OnErrorEventHandlerNonNull*>(Ptr());
  }

  void SetHandler(OnErrorEventHandlerNonNull* aHandler)
  {
    ReleaseHandler();
    Assign(aHandler, eOnError);
  }

  CallbackFunction* Ptr() const
  {
    
    
    return reinterpret_cast<CallbackFunction*>(mBits & ~uintptr_t(eTypeBits));
  }

  void ForgetHandler()
  {
    ReleaseHandler();
    mBits = 0;
  }

  bool operator==(const nsEventHandler& aOther) const
  {
    return
      Ptr() && aOther.Ptr() &&
      Ptr()->CallbackPreserveColor() == aOther.Ptr()->CallbackPreserveColor();
  }
private:
  void operator=(const nsEventHandler&) MOZ_DELETE;

  void ReleaseHandler()
  {
    nsISupports* ptr = Ptr();
    NS_IF_RELEASE(ptr);
  }

  void Assign(nsISupports* aHandler, HandlerType aType) {
    MOZ_ASSERT(aHandler, "Must have handler");
    NS_ADDREF(aHandler);
    mBits = uintptr_t(aHandler) | uintptr_t(aType);
  }

  uintptr_t mBits;
};








class nsIJSEventListener : public nsIDOMEventListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSEVENTLISTENER_IID)

  nsIJSEventListener(nsIScriptContext* aContext, JSObject* aScopeObject,
                     nsISupports *aTarget, nsIAtom* aType,
                     const nsEventHandler& aHandler)
  : mContext(aContext), mScopeObject(aScopeObject), mEventName(aType),
    mHandler(aHandler)
  {
    nsCOMPtr<nsISupports> base = do_QueryInterface(aTarget);
    mTarget = base.get();
  }

  
  nsIScriptContext *GetEventContext() const
  {
    return mContext;
  }

  nsISupports *GetEventTarget() const
  {
    return mTarget;
  }

  void Disconnect()
  {
    mTarget = nullptr;
  }

  
  JSObject* GetEventScope() const
  {
    if (!mScopeObject) {
      return nullptr;
    }

    JS::ExposeObjectToActiveJS(mScopeObject);
    return mScopeObject;
  }

  const nsEventHandler& GetHandler() const
  {
    return mHandler;
  }

  void ForgetHandler()
  {
    mHandler.ForgetHandler();
  }

  nsIAtom* EventName() const
  {
    return mEventName;
  }

  
  
  void SetHandler(const nsEventHandler& aHandler, nsIScriptContext* aContext,
                  JS::Handle<JSObject*> aScopeObject)
  {
    mHandler.SetHandler(aHandler);
    mContext = aContext;
    UpdateScopeObject(aScopeObject);
  }
  void SetHandler(mozilla::dom::EventHandlerNonNull* aHandler)
  {
    mHandler.SetHandler(aHandler);
  }
  void SetHandler(mozilla::dom::OnBeforeUnloadEventHandlerNonNull* aHandler)
  {
    mHandler.SetHandler(aHandler);
  }
  void SetHandler(mozilla::dom::OnErrorEventHandlerNonNull* aHandler)
  {
    mHandler.SetHandler(aHandler);
  }

  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return 0;

    
    
    
    
    
    
    
    
    
    
  }

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  virtual ~nsIJSEventListener()
  {
    NS_ASSERTION(!mTarget, "Should have called Disconnect()!");
  }

  
  
  virtual void UpdateScopeObject(JS::Handle<JSObject*> aScopeObject) = 0;

  nsCOMPtr<nsIScriptContext> mContext;
  JS::Heap<JSObject*> mScopeObject;
  nsISupports* mTarget;
  nsCOMPtr<nsIAtom> mEventName;
  nsEventHandler mHandler;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSEventListener, NS_IJSEVENTLISTENER_IID)




nsresult NS_NewJSEventListener(nsIScriptContext *aContext,
                               JSObject* aScopeObject, nsISupports* aTarget,
                               nsIAtom* aType, const nsEventHandler& aHandler,
                               nsIJSEventListener **aReturn);

#endif 
