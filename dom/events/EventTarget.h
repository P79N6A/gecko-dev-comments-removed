





#ifndef mozilla_dom_EventTarget_h_
#define mozilla_dom_EventTarget_h_

#include "nsIDOMEventTarget.h"
#include "nsWrapperCache.h"
#include "nsIAtom.h"

class nsIDOMWindow;
class nsIGlobalObject;

namespace mozilla {

class ErrorResult;
class EventListenerManager;

namespace dom {

class Event;
class EventListener;
class EventHandlerNonNull;
template <class T> struct Nullable;


#define NS_EVENTTARGET_IID \
{ 0x605158a9, 0xe229, 0x45b1, \
 { 0xbc, 0x12, 0x02, 0x9f, 0xa3, 0xa9, 0x3f, 0xcb } }

class EventTarget : public nsIDOMEventTarget,
                    public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_EVENTTARGET_IID)

  
  using nsIDOMEventTarget::AddEventListener;
  using nsIDOMEventTarget::RemoveEventListener;
  using nsIDOMEventTarget::DispatchEvent;
  virtual void AddEventListener(const nsAString& aType,
                                EventListener* aCallback,
                                bool aCapture,
                                const Nullable<bool>& aWantsUntrusted,
                                ErrorResult& aRv) = 0;
  virtual void RemoveEventListener(const nsAString& aType,
                                   EventListener* aCallback,
                                   bool aCapture,
                                   ErrorResult& aRv);
  bool DispatchEvent(Event& aEvent, ErrorResult& aRv);

  
  EventHandlerNonNull* GetEventHandler(const nsAString& aType)
  {
    nsCOMPtr<nsIAtom> type = do_GetAtom(aType);
    return GetEventHandler(type, EmptyString());
  }

  
  void SetEventHandler(const nsAString& aType, EventHandlerNonNull* aHandler,
                       ErrorResult& rv);

  
  virtual void EventListenerAdded(nsIAtom* aType) {}
  virtual void EventListenerRemoved(nsIAtom* aType) {}

  
  
  
  virtual nsIDOMWindow* GetOwnerGlobalForBindings() = 0;

  
  
  
  virtual nsIGlobalObject* GetOwnerGlobal() const = 0;

  


  virtual EventListenerManager* GetOrCreateListenerManager() = 0;

  



  virtual EventListenerManager* GetExistingListenerManager() const = 0;

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
