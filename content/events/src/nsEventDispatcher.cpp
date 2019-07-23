




































#include "nsEventDispatcher.h"
#include "nsDOMEvent.h"
#include "nsPIDOMEventTarget.h"
#include "nsPresContext.h"
#include "nsIPrivateDOMEvent.h"
#include "nsEventListenerManager.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"
#include "nsMutationEvent.h"
#include NEW_H
#include "nsFixedSizeAllocator.h"

#define NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH  (1 << 0)
#define NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT (1 << 1)


class nsEventTargetChainItem
{
private:
  nsEventTargetChainItem(nsPIDOMEventTarget* aTarget,
                         nsEventTargetChainItem* aChild = nsnull);

public:
  static nsEventTargetChainItem* Create(nsFixedSizeAllocator* aAllocator, 
                                        nsPIDOMEventTarget* aTarget,
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
    
    nsEventTargetChainItem* item = aItem;
    if (item->mChild) {
      item->mChild->mParent = nsnull;
      item->mChild = nsnull;
    }
    while (item) {
      nsEventTargetChainItem* parent = item->mParent;
      item->~nsEventTargetChainItem();
      aAllocator->Free(item, sizeof(nsEventTargetChainItem));
      --sCurrentEtciCount;
      item = parent;
    }
  }

  PRBool IsValid()
  {
    NS_WARN_IF_FALSE(!!(mTarget), "Event target is not valid!");
    return !!(mTarget);
  }

  nsISupports* GetNewTarget()
  {
    return mNewTarget;
  }

  void SetNewTarget(nsPIDOMEventTarget* aNewTarget)
  {
    mNewTarget = aNewTarget;
  }

  void SetForceContentDispatch(PRBool aForce)
  {
    if (aForce) {
      mFlags |= NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH;
    } else {
      mFlags &= ~NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH;
    }
  }

  PRBool ForceContentDispatch()
  {
    return !!(mFlags & NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH);
  }


  void SetWantsWillHandleEvent(PRBool aWants)
  {
    if (aWants) {
      mFlags |= NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT;
    } else {
      mFlags &= ~NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT;
    }
  }

  PRBool WantsWillHandleEvent()
  {
    return !!(mFlags & NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT);
  }

  nsPIDOMEventTarget* CurrentTarget()
  {
    return mTarget;
  }

  





  nsresult HandleEventTargetChain(nsEventChainPostVisitor& aVisitor,
                                  PRUint32 aFlags,
                                  nsDispatchingCallback* aCallback,
                                  PRBool aMayHaveNewListenerManagers);

  



  nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  




  nsresult HandleEvent(nsEventChainPostVisitor& aVisitor, PRUint32 aFlags,
                       PRBool aMayHaveNewListenerManagers);

  


  nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  static PRUint32 MaxEtciCount() { return sMaxEtciCount; }

  static void ResetMaxEtciCount()
  {
    NS_ASSERTION(!sCurrentEtciCount, "Wrong time to call ResetMaxEtciCount()!");
    sMaxEtciCount = 0;
  }

  nsCOMPtr<nsPIDOMEventTarget>      mTarget;
  nsEventTargetChainItem*           mChild;
  nsEventTargetChainItem*           mParent;
  PRUint16                          mFlags;
  PRUint16                          mItemFlags;
  nsCOMPtr<nsISupports>             mItemData;
  
  nsCOMPtr<nsPIDOMEventTarget>      mNewTarget;
  
  nsCOMPtr<nsIEventListenerManager> mManager;

  static PRUint32                   sMaxEtciCount;
  static PRUint32                   sCurrentEtciCount;
};

PRUint32 nsEventTargetChainItem::sMaxEtciCount = 0;
PRUint32 nsEventTargetChainItem::sCurrentEtciCount = 0;

nsEventTargetChainItem::nsEventTargetChainItem(nsPIDOMEventTarget* aTarget,
                                               nsEventTargetChainItem* aChild)
: mChild(aChild), mParent(nsnull), mFlags(0), mItemFlags(0)
{
  mTarget = aTarget->GetTargetForEventTargetChain();
  if (mChild) {
    mChild->mParent = this;
  }

  if (++sCurrentEtciCount > sMaxEtciCount) {
    sMaxEtciCount = sCurrentEtciCount;
  }
}

nsresult
nsEventTargetChainItem::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.Reset();
  nsresult rv = mTarget->PreHandleEvent(aVisitor);
  SetForceContentDispatch(aVisitor.mForceContentDispatch);
  SetWantsWillHandleEvent(aVisitor.mWantsWillHandleEvent);
  mItemFlags = aVisitor.mItemFlags;
  mItemData = aVisitor.mItemData;
  return rv;
}

nsresult
nsEventTargetChainItem::HandleEvent(nsEventChainPostVisitor& aVisitor,
                                    PRUint32 aFlags,
                                    PRBool aMayHaveNewListenerManagers)
{
  if (WantsWillHandleEvent()) {
    mTarget->WillHandleEvent(aVisitor);
  }
  if (aVisitor.mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) {
    return NS_OK;
  }
  if (!mManager) {
    if (!aMayHaveNewListenerManagers) {
      return NS_OK;
    }
    mManager = mTarget->GetListenerManager(PR_FALSE);
  }
  if (mManager) {
    NS_ASSERTION(aVisitor.mEvent->currentTarget == nsnull,
                 "CurrentTarget should be null!");
    mManager->HandleEvent(aVisitor.mPresContext, aVisitor.mEvent,
                          &aVisitor.mDOMEvent,
                          CurrentTarget(), aFlags,
                          &aVisitor.mEventStatus);
    NS_ASSERTION(aVisitor.mEvent->currentTarget == nsnull,
                 "CurrentTarget should be null!");
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
                                               nsDispatchingCallback* aCallback,
                                               PRBool aMayHaveNewListenerManagers)
{
  PRUint32 createdELMs = nsEventListenerManager::sCreatedCount;
  
  nsCOMPtr<nsISupports> firstTarget = aVisitor.mEvent->target;

  
  nsEventTargetChainItem* item = this;
  aVisitor.mEvent->flags |= NS_EVENT_FLAG_CAPTURE;
  aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_BUBBLE;
  while (item->mChild) {
    if ((!(aVisitor.mEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH) ||
         item->ForceContentDispatch()) &&
        !(aVisitor.mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
      item->HandleEvent(aVisitor, aFlags & NS_EVENT_CAPTURE_MASK,
                        aMayHaveNewListenerManagers ||
                        createdELMs != nsEventListenerManager::sCreatedCount);
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
    
    
    
    item->HandleEvent(aVisitor, aFlags,
                      aMayHaveNewListenerManagers ||
                      createdELMs != nsEventListenerManager::sCreatedCount);
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
          !(aVisitor.mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
        item->HandleEvent(aVisitor, aFlags & NS_EVENT_BUBBLE_MASK,
                          createdELMs != nsEventListenerManager::sCreatedCount);
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
                           aCallback,
                           createdELMs != nsEventListenerManager::sCreatedCount);
  }

  return NS_OK;
}

#define NS_CHAIN_POOL_SIZE 128

class ChainItemPool {
public:
  ChainItemPool() {
    if (!sEtciPool) {
      sEtciPool = new nsFixedSizeAllocator();
      if (sEtciPool) {
        static const size_t kBucketSizes[] = { sizeof(nsEventTargetChainItem) };
        static const PRInt32 kNumBuckets = sizeof(kBucketSizes) / sizeof(size_t);
        static const PRInt32 kInitialPoolSize =
          NS_SIZE_IN_HEAP(sizeof(nsEventTargetChainItem)) * NS_CHAIN_POOL_SIZE;
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
      if (nsEventTargetChainItem::MaxEtciCount() > NS_CHAIN_POOL_SIZE) {
        delete sEtciPool;
        sEtciPool = nsnull;
        nsEventTargetChainItem::ResetMaxEtciCount();
      }
    }
  }

  static void Shutdown()
  {
    if (!sEtciPoolUsers) {
      delete sEtciPool;
      sEtciPool = nsnull;
      nsEventTargetChainItem::ResetMaxEtciCount();
    }
  }

  nsFixedSizeAllocator* GetPool() { return sEtciPool; }

  static nsFixedSizeAllocator* sEtciPool;
  static PRInt32               sEtciPoolUsers;
};

nsFixedSizeAllocator* ChainItemPool::sEtciPool = nsnull;
PRInt32 ChainItemPool::sEtciPoolUsers = 0;

void NS_ShutdownChainItemPool() { ChainItemPool::Shutdown(); }

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

  nsCOMPtr<nsPIDOMEventTarget> target = do_QueryInterface(aTarget);
#ifdef DEBUG
  if (aDOMEvent) {
    nsCOMPtr<nsIPrivateDOMEvent> privEvt(do_QueryInterface(aDOMEvent));
    if (privEvt) {
      nsEvent* innerEvent = privEvt->GetInternalNSEvent();
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
    nsEventTargetChainItem::Create(pool.GetPool(), target);
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
  nsCOMPtr<nsIContent> content = do_QueryInterface(aEvent->originalTarget);
  PRBool isInAnon = (content && content->IsInAnonymousSubtree());

  NS_MARK_EVENT_DISPATCH_STARTED(aEvent);

  
  
  nsEventStatus status = aEventStatus ? *aEventStatus : nsEventStatus_eIgnore;
  nsEventChainPreVisitor preVisitor(aPresContext, aEvent, aDOMEvent, status,
                                    isInAnon);
  targetEtci->PreHandleEvent(preVisitor);

  if (preVisitor.mCanHandle) {
    
    
    nsCOMPtr<nsPIDOMEventTarget> t = do_QueryInterface(aEvent->target);
    targetEtci->SetNewTarget(t);
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
                                           aCallback,
                                           PR_TRUE);

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
      nsEvent* innerEvent = privEvt->GetInternalNSEvent();
      NS_ENSURE_TRUE(innerEvent, NS_ERROR_ILLEGAL_VALUE);

      PRBool dontResetTrusted = PR_FALSE;
      if (innerEvent->flags & NS_EVENT_DISPATCHED) {
        innerEvent->target = nsnull;
        innerEvent->originalTarget = nsnull;
      }
      else {
        nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(privEvt));
        nsevent->GetIsTrusted(&dontResetTrusted);
      }

      if (!dontResetTrusted) {
        
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
    case NS_SCROLLPORT_EVENT:
      return NS_NewDOMUIEvent(aDOMEvent, aPresContext,
                              static_cast<nsGUIEvent*>(aEvent));
    case NS_SCROLLAREA_EVENT:
      return NS_NewDOMScrollAreaEvent(aDOMEvent, aPresContext,
                                      static_cast<nsScrollAreaEvent *>(aEvent));
    case NS_KEY_EVENT:
      return NS_NewDOMKeyboardEvent(aDOMEvent, aPresContext,
                                    static_cast<nsKeyEvent*>(aEvent));
    case NS_MOUSE_EVENT:
    case NS_POPUP_EVENT:
      return NS_NewDOMMouseEvent(aDOMEvent, aPresContext,
                                 static_cast<nsInputEvent*>(aEvent));
    case NS_MOUSE_SCROLL_EVENT:
      return NS_NewDOMMouseScrollEvent(aDOMEvent, aPresContext,
                                 static_cast<nsInputEvent*>(aEvent));
    case NS_DRAG_EVENT:
      return NS_NewDOMDragEvent(aDOMEvent, aPresContext,
                                 static_cast<nsDragEvent*>(aEvent));
    case NS_TEXT_EVENT:
      return NS_NewDOMTextEvent(aDOMEvent, aPresContext,
                                static_cast<nsTextEvent*>(aEvent));
#ifdef MOZ_SVG
    case NS_SVG_EVENT:
      return NS_NewDOMSVGEvent(aDOMEvent, aPresContext,
                               aEvent);
    case NS_SVGZOOM_EVENT:
      return NS_NewDOMSVGZoomEvent(aDOMEvent, aPresContext,
                                   static_cast<nsGUIEvent*>(aEvent));
#endif 

    case NS_COMMAND_EVENT:
      return NS_NewDOMCommandEvent(aDOMEvent, aPresContext,
                                   static_cast<nsCommandEvent*>(aEvent));
    case NS_SIMPLE_GESTURE_EVENT:
      return NS_NewDOMSimpleGestureEvent(aDOMEvent, aPresContext,
                                         static_cast<nsSimpleGestureEvent*>(aEvent));
    }

    
    return NS_NewDOMEvent(aDOMEvent, aPresContext, aEvent);
  }

  

  if (aEventType.LowerCaseEqualsLiteral("mouseevent") ||
      aEventType.LowerCaseEqualsLiteral("mouseevents") ||
      aEventType.LowerCaseEqualsLiteral("popupevents"))
    return NS_NewDOMMouseEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("mousescrollevents"))
    return NS_NewDOMMouseScrollEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("dragevent") ||
      aEventType.LowerCaseEqualsLiteral("dragevents"))
    return NS_NewDOMDragEvent(aDOMEvent, aPresContext, nsnull);
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
  if (aEventType.LowerCaseEqualsLiteral("orientation"))
    return NS_NewDOMOrientationEvent(aDOMEvent, aPresContext, nsnull);
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
  if (aEventType.LowerCaseEqualsLiteral("datacontainerevent") ||
      aEventType.LowerCaseEqualsLiteral("datacontainerevents"))
    return NS_NewDOMDataContainerEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("messageevent"))
    return NS_NewDOMMessageEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("progressevent"))
    return NS_NewDOMProgressEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("notifypaintevent"))
    return NS_NewDOMNotifyPaintEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("simplegestureevent"))
    return NS_NewDOMSimpleGestureEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("beforeunloadevent"))
    return NS_NewDOMBeforeUnloadEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("pagetransition"))
    return NS_NewDOMPageTransitionEvent(aDOMEvent, aPresContext, nsnull);
  if (aEventType.LowerCaseEqualsLiteral("scrollareaevent"))
    return NS_NewDOMScrollAreaEvent(aDOMEvent, aPresContext, nsnull);

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
