




































#ifdef MOZILLA_INTERNAL_API
#ifndef nsEventDispatcher_h___
#define nsEventDispatcher_h___

#include "nsGUIEvent.h"

class nsIContent;
class nsIDocument;
class nsPresContext;
class nsPIDOMEventTarget;
class nsIScriptGlobalObject;
class nsEventTargetChainItem;






















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

  









  PRUint16              mItemFlags;

  







  nsCOMPtr<nsISupports> mItemData;
};

class nsEventChainPreVisitor : public nsEventChainVisitor {
public:
  nsEventChainPreVisitor(nsPresContext* aPresContext,
                         nsEvent* aEvent,
                         nsIDOMEvent* aDOMEvent,
                         nsEventStatus aEventStatus = nsEventStatus_eIgnore)
  : nsEventChainVisitor(aPresContext, aEvent, aDOMEvent, aEventStatus),
    mCanHandle(PR_TRUE), mForceContentDispatch(PR_FALSE),
    mRelatedTargetIsInAnon(PR_FALSE) {}

  void Reset() {
    mItemFlags = 0;
    mItemData = nsnull;
    mCanHandle = PR_TRUE;
    mForceContentDispatch = PR_FALSE;
    mParentTarget = nsnull;
    mEventTargetAtParent = nsnull;
  }

  





  PRPackedBool          mCanHandle;

  




  PRPackedBool          mForceContentDispatch;

  



  PRPackedBool          mRelatedTargetIsInAnon;

  


  nsCOMPtr<nsISupports> mParentTarget;

  



  nsCOMPtr<nsISupports> mEventTargetAtParent;
};

class nsEventChainPostVisitor : public nsEventChainVisitor {
public:
  nsEventChainPostVisitor(nsEventChainVisitor& aOther)
  : nsEventChainVisitor(aOther.mPresContext, aOther.mEvent, aOther.mDOMEvent,
                        aOther.mEventStatus)
  {}
};







class nsDispatchingCallback {
public:
  virtual void HandleEvent(nsEventChainPostVisitor& aVisitor) = 0;
};





class nsEventDispatcher
{
public:
  











  static nsresult Dispatch(nsISupports* aTarget,
                           nsPresContext* aPresContext,
                           nsEvent* aEvent,
                           nsIDOMEvent* aDOMEvent = nsnull,
                           nsEventStatus* aEventStatus = nsnull,
                           nsDispatchingCallback* aCallback = nsnull);

  







  static nsresult DispatchDOMEvent(nsISupports* aTarget,
                                   nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                   nsPresContext* aPresContext,
                                   nsEventStatus* aEventStatus);

  


  static nsresult CreateEvent(nsPresContext* aPresContext,
                              nsEvent* aEvent,
                              const nsAString& aEventType,
                              nsIDOMEvent** aDOMEvent);

};

#endif
#endif
