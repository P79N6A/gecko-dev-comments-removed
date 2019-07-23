




































#include "nsEventDispatcher.h"
#include "nsDOMEvent.h"
#include "nsPIDOMEventTarget.h"
#include "nsPresContext.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEventListenerManager.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"
#include "nsMutationEvent.h"
#include NEW_H
#include "nsFixedSizeAllocator.h"

#define NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH (1 << 0)


class nsEventTargetChainItem
{
private:
  nsEventTargetChainItem(nsISupports* aTarget,
                         nsEventTargetChainItem* aChild = nsnull);

  void Destroy(nsFixedSizeAllocator* aAllocator);

public:
  static nsEventTargetChainItem* Create(nsFixedSizeAllocator* aAllocator, 
                                        nsISupports* aTarget,
                                        nsEventTargetChainItem* aChild = nsnull)
  {
    void* place = aAllocator->Alloc(sizeof(nsEventTargetChainItem));
    return place
      ? ::new (place) nsEventTargetChainItem(aTarget, aChild)
      : nsnull;
  }

  static void Destroy(nsFixedSizeAllocator* aAllocator,
                      nsEventTargetChainItem* aItem)
  {
    aItem->Destroy(aAllocator);
    aItem->~nsEventTargetChainItem();
    aAllocator->Free(aItem, sizeof(nsEventTargetChainItem));
  }

  PRBool IsValid()
  {
    return !!(mTarget);
  }

  nsISupports* GetNewTarget()
  {
    return mNewTarget;
  }

  void SetNewTarget(nsISupports* aNewTarget)
  {
    mNewTarget = aNewTarget;
  }

  void SetForceContentDispatch(PRBool aForce) {
    if (aForce) {
      mFlags |= NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH;
    } else {
      mFlags &= ~NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH;
    }
  }

  PRBool ForceContentDispatch() {
    return !!(mFlags & NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH);
  }

  nsPIDOMEventTarget* CurrentTarget()
  {
    return mTarget;
  }

  





  nsresult HandleEventTargetChain(nsEventChainPostVisitor& aVisitor,
                                  PRUint32 aFlags,
                                  nsDispatchingCallback* aCallback);

  



  nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  




  nsresult HandleEvent(nsEventChainPostVisitor& aVisitor, PRUint32 aFlags);

  


  nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);


  nsCOMPtr<nsPIDOMEventTarget> mTarget;
  nsEventTargetChainItem*      mChild;
  nsEventTargetChainItem*      mParent;
  PRUint16                     mFlags;
  PRUint16                     mItemFlags;
  nsCOMPtr<nsISupports>        mItemData;
  
  nsCOMPtr<nsISupports>        mNewTarget;
};

nsEventTargetChainItem::nsEventTargetChainItem(nsISupports* aTarget,
                                               nsEventTargetChainItem* aChild)
: mChild(aChild), mParent(nsnull), mFlags(0), mItemFlags(0)
{
  nsCOMPtr<nsPIDOMEventTarget> t = do_QueryInterface(aTarget);
  if (t) {
    mTarget = t->GetTargetForEventTargetChain();
  }
  if (mChild) {
    mChild->mParent = this;
  }
}

void
nsEventTargetChainItem::Destroy(nsFixedSizeAllocator* aAllocator)
{
  if (mChild) {
    mChild->mParent = nsnull;
    mChild = nsnull;
  }

  if (mParent) {
    Destroy(aAllocator, mParent);
    mParent = nsnull;
  }

  mTarget = nsnull;
}

nsresult
nsEventTargetChainItem::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.Reset();
  nsresult rv = mTarget->PreHandleEvent(aVisitor);
  SetForceContentDispatch(aVisitor.mForceContentDispatch);
  mItemFlags = aVisitor.mItemFlags;
  mItemData = aVisitor.mItemData;
  return rv;
}

nsresult
nsEventTargetChainItem::HandleEvent(nsEventChainPostVisitor& aVisitor,
                                    PRUint32 aFlags)
{
  nsCOMPtr<nsIEventListenerManager> lm;
  mTarget->GetListenerManager(PR_FALSE, getter_AddRefs(lm));
  aVisitor.mEvent->currentTarget = CurrentTarget()->GetTargetForDOMEvent(); 
  if (lm && aVisitor.mEvent->currentTarget) {
    lm->HandleEvent(aVisitor.mPresContext, aVisitor.mEvent, &aVisitor.mDOMEvent,
                    aVisitor.mEvent->currentTarget, aFlags,
                    &aVisitor.mEventStatus);
    aVisitor.mEvent->currentTarget = nsnull;
  }
  return NS_OK;
}

nsresult
nsEventTargetChainItem::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  aVisitor.mItemFlags = mItemFlags;
  aVisitor.mItemData = mItemData;
  mTarget->PostHandleEvent(aVisitor);
  return NS_OK;
}

nsresult
nsEventTargetChainItem::HandleEventTargetChain(nsEventChainPostVisitor& aVisitor, PRUint32 aFlags,
                                               nsDispatchingCallback* aCallback)
{
  
  nsCOMPtr<nsISupports> firstTarget = aVisitor.mEvent->target;

  
  nsEventTargetChainItem* item = this;
  aVisitor.mEvent->flags |= NS_EVENT_FLAG_CAPTURE;
  aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_BUBBLE;
  while (item->mChild) {
    if ((!(aVisitor.mEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH) ||
         item->ForceContentDispatch()) &&
        !(aVisitor.mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
      item->HandleEvent(aVisitor, aFlags & NS_EVENT_CAPTURE_MASK);
    }

    if (item->GetNewTarget()) {
      
      nsEventTargetChainItem* nextTarget = item->mChild;
      while (nextTarget) {
        nsISupports* newTarget = nextTarget->GetNewTarget();
        if (newTarget) {
          aVisitor.mEvent->target = newTarget;
          break;
        }
        nextTarget = nextTarget->mChild;
      }
    }

    item = item->mChild;
  }

  
  aVisitor.mEvent->flags |= NS_EVENT_FLAG_BUBBLE;
  if (!(aVisitor.mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) &&
      (!(aVisitor.mEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH) ||
       item->ForceContentDispatch())) {
    
    
    
    item->HandleEvent(aVisitor, aFlags);
  }
  if (aFlags & NS_EVENT_FLAG_SYSTEM_EVENT) {
    item->PostHandleEvent(aVisitor);
  }

  
  aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_CAPTURE;
  item = item->mParent;
  while (item) {
    nsISupports* newTarget = item->GetNewTarget();
    if (newTarget) {
      
      
      aVisitor.mEvent->target = newTarget;
    }

    if (!(aVisitor.mEvent->flags & NS_EVENT_FLAG_CANT_BUBBLE) || newTarget) {
      if ((!(aVisitor.mEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH) ||
           item->ForceContentDispatch()) &&
          (!(aFlags & NS_EVENT_FLAG_SYSTEM_EVENT) ||
           aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault) &&
          !(aVisitor.mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
        item->HandleEvent(aVisitor, aFlags & NS_EVENT_BUBBLE_MASK);
      }
      if (aFlags & NS_EVENT_FLAG_SYSTEM_EVENT) {
        item->PostHandleEvent(aVisitor);
      }
    }
    item = item->mParent;
  }
  aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_BUBBLE;

  if (!(aFlags & NS_EVENT_FLAG_SYSTEM_EVENT)) {
    
    
    
    aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_STOP_DISPATCH;

    
    aVisitor.mEvent->target = aVisitor.mEvent->originalTarget;

    
    
    if (aCallback) {
      aCallback->HandleEvent(aVisitor);
    }

    
    
    aVisitor.mEvent->target = firstTarget;
    HandleEventTargetChain(aVisitor, aFlags | NS_EVENT_FLAG_SYSTEM_EVENT,
                           aCallback);
  }

  return NS_OK;
}

class ChainItemPool {
public:
  ChainItemPool() {
    if (!sEtciPool) {
      sEtciPool = new nsFixedSizeAllocator();
      if (sEtciPool) {
        static const size_t kBucketSizes[] = { sizeof(nsEventTargetChainItem) };
        static const PRInt32 kNumBuckets = sizeof(kBucketSizes) / sizeof(size_t);
        static const PRInt32 kInitialPoolSize =
          NS_SIZE_IN_HEAP(sizeof(nsEventTargetChainItem)) * 128;
        nsresult rv = sEtciPool->Init("EventTargetChainItem Pool", kBucketSizes,
                                      kNumBuckets, kInitialPoolSize);
        if (NS_FAILED(rv)) {
          delete sEtciPool;
          sEtciPool = nsnull;
        }
      }
    }
    if (sEtciPool) {
      ++sEtciPoolUsers;
    }
  }

  ~ChainItemPool() {
    if (sEtciPool) {
      --sEtciPoolUsers;
    }
    if (!sEtciPoolUsers) {
      delete sEtciPool;
      sEtciPool = nsnull;
    }
  }

  nsFixedSizeAllocator* GetPool() { return sEtciPool; }

  static nsFixedSizeAllocator* sEtciPool;
  static PRInt32               sEtciPoolUsers;
};

nsFixedSizeAllocator* ChainItemPool::sEtciPool = nsnull;
PRInt32 ChainItemPool::sEtciPoolUsers = 0;

 nsresult
nsEventDispatcher::Dispatch(nsISupports* aTarget,
                            nsPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent* aDOMEvent,
                            nsEventStatus* aEventStatus,
                            nsDispatchingCallback* aCallback)
{
  NS_ASSERTION(aEvent, "Trying to dispatch without nsEvent!");
  NS_ENSURE_TRUE(!NS_IS_EVENT_IN_DISPATCH(aEvent),
                 NS_ERROR_ILLEGAL_VALUE);
  
  
  NS_ENSURE_TRUE(!(aDOMEvent &&
                   (aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH_IMMEDIATELY)),
                 NS_ERROR_ILLEGAL_VALUE);

#ifdef DEBUG
  if (aDOMEvent) {
    nsCOMPtr<nsIPrivateDOMEvent> privEvt(do_QueryInterface(aDOMEvent));
    if (privEvt) {
      nsEvent* innerEvent = nsnull;
      privEvt->GetInternalNSEvent(&innerEvent);
      NS_ASSERTION(innerEvent == aEvent,
                    "The inner event of aDOMEvent is not the same as aEvent!");
    }
  }
#endif

  nsresult rv = NS_OK;
  PRBool externalDOMEvent = !!(aDOMEvent);

  
  
  nsCOMPtr<nsPresContext> kungFuDeathGrip(aPresContext);
  ChainItemPool pool;
  NS_ENSURE_TRUE(pool.GetPool(), NS_ERROR_OUT_OF_MEMORY);

  
  nsEventTargetChainItem* targetEtci =
    nsEventTargetChainItem::Create(pool.GetPool(), aTarget);
  NS_ENSURE_TRUE(targetEtci, NS_ERROR_OUT_OF_MEMORY);
  if (!targetEtci->IsValid()) {
    nsEventTargetChainItem::Destroy(pool.GetPool(), targetEtci);
    return NS_ERROR_FAILURE;
  }

  
  
  if (!aEvent->target) {
    
    
    aEvent->target = targetEtci->CurrentTarget();
  } else {
    
    
    
    
    
    nsCOMPtr<nsPIDOMEventTarget> t = do_QueryInterface(aEvent->target);
    NS_ENSURE_STATE(t);
    aEvent->target = t->GetTargetForEventTargetChain();
    NS_ENSURE_STATE(aEvent->target);
  }
  aEvent->originalTarget = aEvent->target;

  NS_MARK_EVENT_DISPATCH_STARTED(aEvent);

  
  
  nsEventStatus status = aEventStatus ? *aEventStatus : nsEventStatus_eIgnore;
  nsEventChainPreVisitor preVisitor(aPresContext, aEvent, aDOMEvent, status);
  targetEtci->PreHandleEvent(preVisitor);

  if (preVisitor.mCanHandle) {
    
    
    targetEtci->SetNewTarget(aEvent->target);
    nsEventTargetChainItem* topEtci = targetEtci;
    while (preVisitor.mParentTarget) {
      nsEventTargetChainItem* parentEtci =
        nsEventTargetChainItem::Create(pool.GetPool(), preVisitor.mParentTarget,
                                       topEtci);
      if (!parentEtci) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }
      if (!parentEtci->IsValid()) {
        rv = NS_ERROR_FAILURE;
        break;
      }

      
      if (preVisitor.mEventTargetAtParent) {
        
        
        preVisitor.mEvent->target = preVisitor.mEventTargetAtParent;
        parentEtci->SetNewTarget(preVisitor.mEventTargetAtParent);
      }

      parentEtci->PreHandleEvent(preVisitor);
      if (preVisitor.mCanHandle) {
        topEtci = parentEtci;
      } else {
        nsEventTargetChainItem::Destroy(pool.GetPool(), parentEtci);
        parentEtci = nsnull;
        break;
      }
    }
    if (NS_SUCCEEDED(rv)) {
      
      nsEventChainPostVisitor postVisitor(preVisitor);
      rv = topEtci->HandleEventTargetChain(postVisitor,
                                           NS_EVENT_FLAG_BUBBLE |
                                           NS_EVENT_FLAG_CAPTURE,
                                           aCallback);

      preVisitor.mEventStatus = postVisitor.mEventStatus;
      
      if (!preVisitor.mDOMEvent && postVisitor.mDOMEvent) {
        preVisitor.mDOMEvent = postVisitor.mDOMEvent;
      }
    }
  }

  nsEventTargetChainItem::Destroy(pool.GetPool(), targetEtci);
  targetEtci = nsnull;

  NS_MARK_EVENT_DISPATCH_DONE(aEvent);

  if (!externalDOMEvent && preVisitor.mDOMEvent) {
    
    
    nsrefcnt rc = 0;
    NS_RELEASE2(preVisitor.mDOMEvent, rc);
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent =
      do_QueryInterface(preVisitor.mDOMEvent);
    if (privateEvent) {
      privateEvent->DuplicatePrivateData();
    }
  }

  if (aEventStatus) {
    *aEventStatus = preVisitor.mEventStatus;
  }
  return rv;
}

 nsresult
nsEventDispatcher::DispatchDOMEvent(nsISupports* aTarget,
                                    nsEvent* aEvent,
                                    nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus)
{
  if (aDOMEvent) {
    nsCOMPtr<nsIPrivateDOMEvent> privEvt(do_QueryInterface(aDOMEvent));
    if (privEvt) {
      nsEvent* innerEvent = nsnull;
      privEvt->GetInternalNSEvent(&innerEvent);
      NS_ENSURE_TRUE(innerEvent, NS_ERROR_ILLEGAL_VALUE);

      PRBool trusted;
      nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(privEvt));

      nsevent->GetIsTrusted(&trusted);

      if (!trusted) {
        
        privEvt->SetTrusted(nsContentUtils::IsCallerTrustedForWrite());
      }

      return nsEventDispatcher::Dispatch(aTarget, aPresContext, innerEvent,
                                         aDOMEvent, aEventStatus);
    }
  } else if (aEvent) {
    return nsEventDispatcher::Dispatch(aTarget, aPresContext, aEvent,
                                       aDOMEvent, aEventStatus);
  }
  return NS_ERROR_ILLEGAL_VALUE;
}

 nsresult
nsEventDispatcher::CreateEvent(nsPresContext* aPresContext,
                               nsEvent* aEvent,
                               const nsAString& aEventType,
                               nsIDOMEvent** aDOMEvent)
{
  *aDOMEvent = nsnull;

  if (aEvent) {
    switch(aEvent->eventStructType) {
    case NS_MUTATION_EVENT:
      return NS_NewDOMMutationEvent(aDOMEvent, aPresContext,
                                    static_cast<nsMutationEvent*>(aEvent));
    case NS_GUI_EVENT:
    case NS_COMPOSITION_EVENT:
    case NS_RECONVERSION_EVENT:
    case NS_QUERYCARETRECT_EVENT:
    case NS_SCROLLPORT_EVENT:
      return NS_NewDOMUIEvent(aDOMEvent, aPresContext,
                              static_cast<nsGUIEvent*>(aEvent));
    case NS_KEY_EVENT:
      return NS_NewDOMKeyboardEvent(aDOMEvent, aPresContext,
                                    static_cast<nsKeyEvent*>(aEvent));
    case NS_MOUSE_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
    case NS_POPUP_EVENT:
      return NS_NewDOMMouseEvent(aDOMEvent, aPresContext,
                                 static_cast<nsInputEvent*>(aEvent));
    case NS_POPUPBLOCKED_EVENT:
      return NS_NewDOMPopupBlockedEvent(aDOMEvent, aPresContext,
                                        static_cast<nsPopupBlockedEvent*>
                                                   (aEvent));
    case NS_TEXT_EVENT:
      return NS_NewDOMTextEvent(aDOMEvent, aPresContext,
                                static_cast<nsTextEvent*>(aEvent));
    case NS_BEFORE_PAGE_UNLOAD_EVENT:
      return
        NS_NewDOMBeforeUnloadEvent(aDOMEvent, aPresContext,
                                   static_cast<nsBeforePageUnloadEvent*>
                                              (aEvent));
    case NS_PAGETRANSITION_EVENT:
      return NS_NewDOMPageTransitionEvent(aDOMEvent, aPresContext,
                                          static_cast<nsPageTransitionEvent*>
                                                     (aEvent));
#ifdef MOZ_SVG
    case NS_SVG_EVENT:
      return NS_NewDOMSVGEvent(aDOMEvent, aPresContext,
                               aEvent);
    case NS_SVGZOOM_EVENT:
      return NS_NewDOMSVGZoomEvent(aDOMEvent, aPresContext,
                                   static_cast<nsGUIEvent*>(aEvent));
#endif 

    case NS_XUL_COMMAND_EVENT:
      return NS_NewDOMXULCommandEvent(aDOMEvent, aPresContext,
                                      static_cast<nsXULCommandEvent*>
                                                 (aEvent));
    case NS_COMMAND_EVENT:
      return NS_NewDOMCommandEvent(aDOMEvent, aPresContext,
                                   static_cast<nsCommandEvent*>(aEvent));
    }

    
    return NS_NewDOMEvent(aDOMEvent, aPresContext, aEvent);
  }

  

  if (aEventType.LowerCaseEqualsLiteral("mouseevent") ||
      aEventType.LowerCaseEqualsLiteral("mouseevents") ||
      aEventType.LowerCaseEqualsLiteral("mousescrollevents") ||
      aEventType.LowerCaseEqualsLiteral("popupevents"))
    return NS_NewDOMMouseEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("keyboardevent") ||
      aEventType.LowerCaseEqualsLiteral("keyevents"))
    return NS_NewDOMKeyboardEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("mutationevent") ||
        aEventType.LowerCaseEqualsLiteral("mutationevents"))
    return NS_NewDOMMutationEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("textevent") ||
      aEventType.LowerCaseEqualsLiteral("textevents"))
    return NS_NewDOMTextEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("popupblockedevents"))
    return NS_NewDOMPopupBlockedEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("uievent") ||
      aEventType.LowerCaseEqualsLiteral("uievents"))
    return NS_NewDOMUIEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("event") ||
      aEventType.LowerCaseEqualsLiteral("events") ||
      aEventType.LowerCaseEqualsLiteral("htmlevents"))
    return NS_NewDOMEvent(aDOMEvent, aPresContext, nsnull);
#ifdef MOZ_SVG
  if (aEventType.LowerCaseEqualsLiteral("svgevent") ||
      aEventType.LowerCaseEqualsLiteral("svgevents"))
    return NS_NewDOMSVGEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("svgzoomevent") ||
      aEventType.LowerCaseEqualsLiteral("svgzoomevents"))
    return NS_NewDOMSVGZoomEvent(aDOMEvent, aPresContext, nsnull);
#endif 
  if (aEventType.LowerCaseEqualsLiteral("xulcommandevent") ||
      aEventType.LowerCaseEqualsLiteral("xulcommandevents"))
    return NS_NewDOMXULCommandEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("commandevent") ||
      aEventType.LowerCaseEqualsLiteral("commandevents"))
    return NS_NewDOMCommandEvent(aDOMEvent, aPresContext, nsnull);

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
