




#ifndef mozilla_dom_EventTarget_h_
#define mozilla_dom_EventTarget_h_

#include "nsIDOMEventTarget.h"
#include "nsWrapperCache.h"
#include "nsIDOMEventListener.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/Nullable.h"
#include "nsIAtom.h"

class nsDOMEvent;

namespace mozilla {
namespace dom {

class EventListener;
class EventHandlerNonNull;


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
    return GetEventHandler(type);
  }

  void SetEventHandler(const nsAString& aType, EventHandlerNonNull* aHandler,
                       ErrorResult& rv)
  {
    nsCOMPtr<nsIAtom> type = do_GetAtom(aType);
    return SetEventHandler(type, aHandler, rv);
  }

  
  virtual void EventListenerAdded(nsIAtom* aType) {}
  virtual void EventListenerRemoved(nsIAtom* aType) {}

protected:
  EventHandlerNonNull* GetEventHandler(nsIAtom* aType);
  void SetEventHandler(nsIAtom* aType, EventHandlerNonNull* aHandler,
                       ErrorResult& rv);
};

NS_DEFINE_STATIC_IID_ACCESSOR(EventTarget, NS_EVENTTARGET_IID)

} 
} 

#endif 
