




#ifndef mozilla_dom_EventTarget_h_
#define mozilla_dom_EventTarget_h_

#include "nsIDOMEventTarget.h"
#include "nsWrapperCache.h"
#include "nsIAtom.h"

class nsDOMEvent;
class nsIDOMWindow;
class nsIDOMEventListener;

namespace mozilla {

class ErrorResult;

namespace dom {

class EventListener;
class EventHandlerNonNull;
template <class T> struct Nullable;


#define NS_EVENTTARGET_IID \
{ 0x0a5aed21, 0x0bab, 0x48b3, \
 { 0xbe, 0x4b, 0xd4, 0xf9, 0xd4, 0xea, 0xc7, 0xdb } }

class EventTarget : public nsIDOMEventTarget,
                    public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_EVENTTARGET_IID)

  
  using nsIDOMEventTarget::AddEventListener;
  using nsIDOMEventTarget::RemoveEventListener;
  using nsIDOMEventTarget::DispatchEvent;
  virtual void AddEventListener(const nsAString& aType,
                                nsIDOMEventListener* aCallback,
                                bool aCapture,
                                const Nullable<bool>& aWantsUntrusted,
                                ErrorResult& aRv) = 0;
  virtual void RemoveEventListener(const nsAString& aType,
                                   nsIDOMEventListener* aCallback,
                                   bool aCapture,
                                   ErrorResult& aRv);
  bool DispatchEvent(nsDOMEvent& aEvent, ErrorResult& aRv);

  
  EventHandlerNonNull* GetEventHandler(const nsAString& aType)
  {
    nsCOMPtr<nsIAtom> type = do_GetAtom(aType);
    return GetEventHandler(type, EmptyString());
  }

  
  void SetEventHandler(const nsAString& aType, EventHandlerNonNull* aHandler,
                       ErrorResult& rv);

  
  virtual void EventListenerAdded(nsIAtom* aType) {}
  virtual void EventListenerRemoved(nsIAtom* aType) {}

  
  
  
  virtual nsIDOMWindow* GetOwnerGlobal() = 0;

protected:
  EventHandlerNonNull* GetEventHandler(nsIAtom* aType,
                                       const nsAString& aTypeString);
  void SetEventHandler(nsIAtom* aType, const nsAString& aTypeString,
                       EventHandlerNonNull* aHandler);
};

NS_DEFINE_STATIC_IID_ACCESSOR(EventTarget, NS_EVENTTARGET_IID)

} 
} 

#endif 
