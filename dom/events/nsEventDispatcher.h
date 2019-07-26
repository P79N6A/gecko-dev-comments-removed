




#ifdef MOZILLA_INTERNAL_API
#ifndef nsEventDispatcher_h___
#define nsEventDispatcher_h___

#include "mozilla/EventForwards.h"
#include "nsCOMPtr.h"


#undef CreateEvent

class nsEventTargetChainItem;
class nsIDOMEvent;
class nsIScriptGlobalObject;
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

} 

class nsEventChainPreVisitor : public mozilla::EventChainVisitor
{
public:
  nsEventChainPreVisitor(nsPresContext* aPresContext,
                         mozilla::WidgetEvent* aEvent,
                         nsIDOMEvent* aDOMEvent,
                         nsEventStatus aEventStatus,
                         bool aIsInAnon)
  : mozilla::EventChainVisitor(aPresContext, aEvent, aDOMEvent, aEventStatus),
    mCanHandle(true), mAutomaticChromeDispatch(true),
    mForceContentDispatch(false), mRelatedTargetIsInAnon(false),
    mOriginalTargetIsInAnon(aIsInAnon), mWantsWillHandleEvent(false),
    mMayHaveListenerManager(true), mParentTarget(nullptr),
    mEventTargetAtParent(nullptr) {}

  void Reset() {
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

  


  mozilla::dom::EventTarget* mParentTarget;

  



  mozilla::dom::EventTarget* mEventTargetAtParent;
};

class nsEventChainPostVisitor : public mozilla::EventChainVisitor
{
public:
  nsEventChainPostVisitor(mozilla::EventChainVisitor& aOther)
  : mozilla::EventChainVisitor(aOther.mPresContext, aOther.mEvent,
                               aOther.mDOMEvent, aOther.mEventStatus)
  {
  }
};







class MOZ_STACK_CLASS nsDispatchingCallback {
public:
  virtual void HandleEvent(nsEventChainPostVisitor& aVisitor) = 0;
};





class nsEventDispatcher
{
public:
  















  static nsresult Dispatch(nsISupports* aTarget,
                           nsPresContext* aPresContext,
                           mozilla::WidgetEvent* aEvent,
                           nsIDOMEvent* aDOMEvent = nullptr,
                           nsEventStatus* aEventStatus = nullptr,
                           nsDispatchingCallback* aCallback = nullptr,
                           nsCOMArray<mozilla::dom::EventTarget>* aTargets = nullptr);

  







  static nsresult DispatchDOMEvent(nsISupports* aTarget,
                                   mozilla::WidgetEvent* aEvent,
                                   nsIDOMEvent* aDOMEvent,
                                   nsPresContext* aPresContext,
                                   nsEventStatus* aEventStatus);

  


  static nsresult CreateEvent(mozilla::dom::EventTarget* aOwner,
                              nsPresContext* aPresContext,
                              mozilla::WidgetEvent* aEvent,
                              const nsAString& aEventType,
                              nsIDOMEvent** aDOMEvent);

};

#endif
#endif
