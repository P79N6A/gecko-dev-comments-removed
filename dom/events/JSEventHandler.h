




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

namespace mozilla {

class TypedEventHandler
{
public:
  enum HandlerType
  {
    eUnset = 0,
    eNormal = 0x1,
    eOnError = 0x2,
    eOnBeforeUnload = 0x3,
    eTypeBits = 0x3
  };

  TypedEventHandler()
    : mBits(0)
  {
  }

  TypedEventHandler(dom::EventHandlerNonNull* aHandler)
  {
    Assign(aHandler, eNormal);
  }

  TypedEventHandler(dom::OnErrorEventHandlerNonNull* aHandler)
  {
    Assign(aHandler, eOnError);
  }

  TypedEventHandler(dom::OnBeforeUnloadEventHandlerNonNull* aHandler)
  {
    Assign(aHandler, eOnBeforeUnload);
  }

  TypedEventHandler(const TypedEventHandler& aOther)
  {
    if (aOther.HasEventHandler()) {
      
      Assign(aOther.Ptr(), aOther.Type());
    } else {
      mBits = 0;
    }
  }

  ~TypedEventHandler()
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

  void SetHandler(const TypedEventHandler& aHandler)
  {
    if (aHandler.HasEventHandler()) {
      ReleaseHandler();
      Assign(aHandler.Ptr(), aHandler.Type());
    } else {
      ForgetHandler();
    }
  }

  dom::EventHandlerNonNull* NormalEventHandler() const
  {
    MOZ_ASSERT(Type() == eNormal && Ptr());
    return reinterpret_cast<dom::EventHandlerNonNull*>(Ptr());
  }

  void SetHandler(dom::EventHandlerNonNull* aHandler)
  {
    ReleaseHandler();
    Assign(aHandler, eNormal);
  }

  dom::OnBeforeUnloadEventHandlerNonNull* OnBeforeUnloadEventHandler() const
  {
    MOZ_ASSERT(Type() == eOnBeforeUnload);
    return reinterpret_cast<dom::OnBeforeUnloadEventHandlerNonNull*>(Ptr());
  }

  void SetHandler(dom::OnBeforeUnloadEventHandlerNonNull* aHandler)
  {
    ReleaseHandler();
    Assign(aHandler, eOnBeforeUnload);
  }

  dom::OnErrorEventHandlerNonNull* OnErrorEventHandler() const
  {
    MOZ_ASSERT(Type() == eOnError);
    return reinterpret_cast<dom::OnErrorEventHandlerNonNull*>(Ptr());
  }

  void SetHandler(dom::OnErrorEventHandlerNonNull* aHandler)
  {
    ReleaseHandler();
    Assign(aHandler, eOnError);
  }

  dom::CallbackFunction* Ptr() const
  {
    
    
    return reinterpret_cast<dom::CallbackFunction*>(mBits &
                                                      ~uintptr_t(eTypeBits));
  }

  void ForgetHandler()
  {
    ReleaseHandler();
    mBits = 0;
  }

  bool operator==(const TypedEventHandler& aOther) const
  {
    return
      Ptr() && aOther.Ptr() &&
      Ptr()->CallbackPreserveColor() == aOther.Ptr()->CallbackPreserveColor();
  }

private:
  void operator=(const TypedEventHandler&) MOZ_DELETE;

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









#define NS_JSEVENTHANDLER_IID \
{ 0x4f486881, 0x1956, 0x4079, \
  { 0x8c, 0xa0, 0xf3, 0xbd, 0x60, 0x5c, 0xc2, 0x79 } }

class JSEventHandler : public nsIDOMEventListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_JSEVENTHANDLER_IID)

  JSEventHandler(nsISupports* aTarget, nsIAtom* aType,
                 const TypedEventHandler& aTypedHandler);

  virtual ~JSEventHandler();

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

  const TypedEventHandler& GetTypedEventHandler() const
  {
    return mTypedHandler;
  }

  void ForgetHandler()
  {
    mTypedHandler.ForgetHandler();
  }

  nsIAtom* EventName() const
  {
    return mEventName;
  }

  
  
  void SetHandler(const TypedEventHandler& aTypedHandler)
  {
    mTypedHandler.SetHandler(aTypedHandler);
  }
  void SetHandler(dom::EventHandlerNonNull* aHandler)
  {
    mTypedHandler.SetHandler(aHandler);
  }
  void SetHandler(dom::OnBeforeUnloadEventHandlerNonNull* aHandler)
  {
    mTypedHandler.SetHandler(aHandler);
  }
  void SetHandler(dom::OnErrorEventHandlerNonNull* aHandler)
  {
    mTypedHandler.SetHandler(aHandler);
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return 0;

    
    
    
    
    
    
    
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf)
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_CLASS(JSEventHandler)

  bool IsBlackForCC();

protected:
  nsISupports* mTarget;
  nsCOMPtr<nsIAtom> mEventName;
  TypedEventHandler mTypedHandler;
};

NS_DEFINE_STATIC_IID_ACCESSOR(JSEventHandler, NS_JSEVENTHANDLER_IID)

} 





nsresult NS_NewJSEventHandler(nsISupports* aTarget,
                              nsIAtom* aType,
                              const mozilla::TypedEventHandler& aTypedHandler,
                              mozilla::JSEventHandler** aReturn);

#endif

