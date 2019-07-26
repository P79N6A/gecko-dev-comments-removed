




#ifndef mozilla_JSEventHandler_h_
#define mozilla_JSEventHandler_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/EventHandlerBinding.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIAtom.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMEventListener.h"
#include "nsIScriptContext.h"

class nsEventHandler
{
public:
  typedef mozilla::dom::EventHandlerNonNull EventHandlerNonNull;
  typedef mozilla::dom::OnBeforeUnloadEventHandlerNonNull
    OnBeforeUnloadEventHandlerNonNull;
  typedef mozilla::dom::OnErrorEventHandlerNonNull OnErrorEventHandlerNonNull;
  typedef mozilla::dom::CallbackFunction CallbackFunction;

  enum HandlerType
  {
    eUnset = 0,
    eNormal = 0x1,
    eOnError = 0x2,
    eOnBeforeUnload = 0x3,
    eTypeBits = 0x3
  };

  nsEventHandler()
    : mBits(0)
  {
  }

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

  HandlerType Type() const
  {
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

  void Assign(nsISupports* aHandler, HandlerType aType)
  {
    MOZ_ASSERT(aHandler, "Must have handler");
    NS_ADDREF(aHandler);
    mBits = uintptr_t(aHandler) | uintptr_t(aType);
  }

  uintptr_t mBits;
};









#define NS_JSEVENTLISTENER_IID \
{ 0x4f486881, 0x1956, 0x4079, \
  { 0x8c, 0xa0, 0xf3, 0xbd, 0x60, 0x5c, 0xc2, 0x79 } }

class nsJSEventListener : public nsIDOMEventListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_JSEVENTLISTENER_IID)

  nsJSEventListener(nsISupports* aTarget, nsIAtom* aType,
                    const nsEventHandler& aHandler);

  virtual ~nsJSEventListener()
  {
    NS_ASSERTION(!mTarget, "Should have called Disconnect()!");
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMEVENTLISTENER

  nsISupports* GetEventTarget() const
  {
    return mTarget;
  }

  void Disconnect()
  {
    mTarget = nullptr;
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

  
  
  void SetHandler(const nsEventHandler& aHandler)
  {
    mHandler.SetHandler(aHandler);
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

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return 0;

    
    
    
    
    
    
    
  }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_CLASS(nsJSEventListener)

  bool IsBlackForCC();

protected:
  nsISupports* mTarget;
  nsCOMPtr<nsIAtom> mEventName;
  nsEventHandler mHandler;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsJSEventListener, NS_JSEVENTLISTENER_IID)





nsresult NS_NewJSEventHandler(nsISupports* aTarget,
                              nsIAtom* aType,
                              const nsEventHandler& aHandler,
                              nsJSEventListener** aReturn);

#endif 

