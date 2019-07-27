




#ifdef MOZILLA_INTERNAL_API
#ifndef mozilla_EventDispatcher_h_
#define mozilla_EventDispatcher_h_

#include "mozilla/EventForwards.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"


#undef CreateEvent

class nsIContent;
class nsIDOMEvent;
class nsPresContext;

template<class E> class nsCOMArray;

namespace mozilla {
namespace dom {
class EventTarget;
} 





















class EventChainVisitor
{
public:
  EventChainVisitor(nsPresContext* aPresContext,
                    WidgetEvent* aEvent,
                    nsIDOMEvent* aDOMEvent,
                    nsEventStatus aEventStatus = nsEventStatus_eIgnore)
    : mPresContext(aPresContext)
    , mEvent(aEvent)
    , mDOMEvent(aDOMEvent)
    , mEventStatus(aEventStatus)
    , mItemFlags(0)
  {
  }

  


  nsPresContext* const  mPresContext;

  


  WidgetEvent* const mEvent;

  



  nsIDOMEvent*          mDOMEvent;

  



  nsEventStatus         mEventStatus;

  









  uint16_t              mItemFlags;

  







  nsCOMPtr<nsISupports> mItemData;
};

class EventChainPreVisitor : public EventChainVisitor
{
public:
  EventChainPreVisitor(nsPresContext* aPresContext,
                       WidgetEvent* aEvent,
                       nsIDOMEvent* aDOMEvent,
                       nsEventStatus aEventStatus,
                       bool aIsInAnon)
    : EventChainVisitor(aPresContext, aEvent, aDOMEvent, aEventStatus)
    , mCanHandle(true)
    , mAutomaticChromeDispatch(true)
    , mForceContentDispatch(false)
    , mRelatedTargetIsInAnon(false)
    , mOriginalTargetIsInAnon(aIsInAnon)
    , mWantsWillHandleEvent(false)
    , mMayHaveListenerManager(true)
    , mParentTarget(nullptr)
    , mEventTargetAtParent(nullptr)
  {
  }

  void Reset()
  {
    mItemFlags = 0;
    mItemData = nullptr;
    mCanHandle = true;
    mAutomaticChromeDispatch = true;
    mForceContentDispatch = false;
    mWantsWillHandleEvent = false;
    mMayHaveListenerManager = true;
    mParentTarget = nullptr;
    mEventTargetAtParent = nullptr;
  }

  





  bool                  mCanHandle;

  



  bool                  mAutomaticChromeDispatch;

  




  bool                  mForceContentDispatch;

  



  bool                  mRelatedTargetIsInAnon;

  



  bool                  mOriginalTargetIsInAnon;

  



  bool                  mWantsWillHandleEvent;

  



  bool                  mMayHaveListenerManager;

  


  dom::EventTarget* mParentTarget;

  



  dom::EventTarget* mEventTargetAtParent;

  




  nsTArray<nsIContent*> mDestInsertionPoints;
};

class EventChainPostVisitor : public mozilla::EventChainVisitor
{
public:
  explicit EventChainPostVisitor(EventChainVisitor& aOther)
    : EventChainVisitor(aOther.mPresContext, aOther.mEvent,
                        aOther.mDOMEvent, aOther.mEventStatus)
  {
  }
};







class MOZ_STACK_CLASS EventDispatchingCallback
{
public:
  virtual void HandleEvent(EventChainPostVisitor& aVisitor) = 0;
};





class EventDispatcher
{
public:
  















  static nsresult Dispatch(nsISupports* aTarget,
                           nsPresContext* aPresContext,
                           WidgetEvent* aEvent,
                           nsIDOMEvent* aDOMEvent = nullptr,
                           nsEventStatus* aEventStatus = nullptr,
                           EventDispatchingCallback* aCallback = nullptr,
                           nsTArray<dom::EventTarget*>* aTargets = nullptr);

  







  static nsresult DispatchDOMEvent(nsISupports* aTarget,
                                   WidgetEvent* aEvent,
                                   nsIDOMEvent* aDOMEvent,
                                   nsPresContext* aPresContext,
                                   nsEventStatus* aEventStatus);

  


  static nsresult CreateEvent(dom::EventTarget* aOwner,
                              nsPresContext* aPresContext,
                              WidgetEvent* aEvent,
                              const nsAString& aEventType,
                              nsIDOMEvent** aDOMEvent);

  


  static void Shutdown();
};

} 

#endif 
#endif
