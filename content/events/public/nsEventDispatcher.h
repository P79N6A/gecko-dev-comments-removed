




#ifdef MOZILLA_INTERNAL_API
#ifndef nsEventDispatcher_h___
#define nsEventDispatcher_h___

#include "nsCOMPtr.h"
#include "nsEvent.h"

class nsPresContext;
class nsIDOMEvent;
class nsIScriptGlobalObject;
class nsIDOMEventTarget;
class nsEventTargetChainItem;
template<class E> class nsCOMArray;
namespace mozilla {
namespace dom {
class EventTarget;
}
}





















class nsEventChainVisitor {
public:
  nsEventChainVisitor(nsPresContext* aPresContext,
                      nsEvent* aEvent,
                      nsIDOMEvent* aDOMEvent,
                      nsEventStatus aEventStatus = nsEventStatus_eIgnore)
  : mPresContext(aPresContext), mEvent(aEvent), mDOMEvent(aDOMEvent),
    mEventStatus(aEventStatus), mItemFlags(0)
  {}

  


  nsPresContext* const  mPresContext;

  


  nsEvent* const        mEvent;

  



  nsIDOMEvent*          mDOMEvent;

  



  nsEventStatus         mEventStatus;

  









  uint16_t              mItemFlags;

  







  nsCOMPtr<nsISupports> mItemData;
};

class nsEventChainPreVisitor : public nsEventChainVisitor {
public:
  nsEventChainPreVisitor(nsPresContext* aPresContext,
                         nsEvent* aEvent,
                         nsIDOMEvent* aDOMEvent,
                         nsEventStatus aEventStatus,
                         bool aIsInAnon)
  : nsEventChainVisitor(aPresContext, aEvent, aDOMEvent, aEventStatus),
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

  


  nsIDOMEventTarget*   mParentTarget;

  



  nsIDOMEventTarget*   mEventTargetAtParent;
};

class nsEventChainPostVisitor : public nsEventChainVisitor {
public:
  nsEventChainPostVisitor(nsEventChainVisitor& aOther)
  : nsEventChainVisitor(aOther.mPresContext, aOther.mEvent, aOther.mDOMEvent,
                        aOther.mEventStatus)
  {}
};







class NS_STACK_CLASS nsDispatchingCallback {
public:
  virtual void HandleEvent(nsEventChainPostVisitor& aVisitor) = 0;
};





class nsEventDispatcher
{
public:
  















  static nsresult Dispatch(nsISupports* aTarget,
                           nsPresContext* aPresContext,
                           nsEvent* aEvent,
                           nsIDOMEvent* aDOMEvent = nullptr,
                           nsEventStatus* aEventStatus = nullptr,
                           nsDispatchingCallback* aCallback = nullptr,
                           nsCOMArray<nsIDOMEventTarget>* aTargets = nullptr);

  







  static nsresult DispatchDOMEvent(nsISupports* aTarget,
                                   nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                   nsPresContext* aPresContext,
                                   nsEventStatus* aEventStatus);

  


  static nsresult CreateEvent(mozilla::dom::EventTarget* aOwner,
                              nsPresContext* aPresContext,
                              nsEvent* aEvent,
                              const nsAString& aEventType,
                              nsIDOMEvent** aDOMEvent);

};

#endif
#endif
