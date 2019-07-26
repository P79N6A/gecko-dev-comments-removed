




#include "nsEventDispatcher.h"
#include "nsDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsPresContext.h"
#include "nsEventListenerManager.h"
#include "nsContentUtils.h"
#include "nsError.h"
#include "nsMutationEvent.h"
#include NEW_H
#include "nsFixedSizeAllocator.h"
#include "nsINode.h"
#include "nsPIDOMWindow.h"
#include "nsFrameLoader.h"
#include "nsDOMTouchEvent.h"
#include "nsDOMStorage.h"
#include "sampler.h"
#include "GeneratedEvents.h"

using namespace mozilla;

#define NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH  (1 << 0)
#define NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT (1 << 1)
#define NS_TARGET_CHAIN_MAY_HAVE_MANAGER        (1 << 2)

static nsEventTargetChainItem* gCachedETCI = nullptr;


class nsEventTargetChainItem
{
private:
  nsEventTargetChainItem(nsIDOMEventTarget* aTarget,
                         nsEventTargetChainItem* aChild = nullptr);

public:
  static nsEventTargetChainItem* Create(nsFixedSizeAllocator* aAllocator, 
                                        nsIDOMEventTarget* aTarget,
                                        nsEventTargetChainItem* aChild = nullptr)
  {
    void* place = nullptr;
    if (gCachedETCI) {
      place = gCachedETCI;
      gCachedETCI = gCachedETCI->mNext;
    } else {
      place = aAllocator->Alloc(sizeof(nsEventTargetChainItem));
    }
    return place
      ? ::new (place) nsEventTargetChainItem(aTarget, aChild)
      : nullptr;
  }

  static void Destroy(nsFixedSizeAllocator* aAllocator,
                      nsEventTargetChainItem* aItem)
  {
    
    nsEventTargetChainItem* item = aItem;
    if (item->mChild) {
      item->mChild->mParent = nullptr;
      item->mChild = nullptr;
    }
    while (item) {
      nsEventTargetChainItem* parent = item->mParent;
      item->~nsEventTargetChainItem();
      item->mNext = gCachedETCI;
      gCachedETCI = item;
      --sCurrentEtciCount;
      item = parent;
    }
  }

  bool IsValid()
  {
    NS_WARN_IF_FALSE(!!(mTarget), "Event target is not valid!");
    return !!(mTarget);
  }

  nsIDOMEventTarget* GetNewTarget()
  {
    return mNewTarget;
  }

  void SetNewTarget(nsIDOMEventTarget* aNewTarget)
  {
    mNewTarget = aNewTarget;
  }

  void SetForceContentDispatch(bool aForce)
  {
    if (aForce) {
      mFlags |= NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH;
    } else {
      mFlags &= ~NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH;
    }
  }

  bool ForceContentDispatch()
  {
    return !!(mFlags & NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH);
  }

  void SetWantsWillHandleEvent(bool aWants)
  {
    if (aWants) {
      mFlags |= NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT;
    } else {
      mFlags &= ~NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT;
    }
  }

  bool WantsWillHandleEvent()
  {
    return !!(mFlags & NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT);
  }

  void SetMayHaveListenerManager(bool aMayHave)
  {
    if (aMayHave) {
      mFlags |= NS_TARGET_CHAIN_MAY_HAVE_MANAGER;
    } else {
      mFlags &= ~NS_TARGET_CHAIN_MAY_HAVE_MANAGER;
    }
  }

  bool MayHaveListenerManager()
  {
    return !!(mFlags & NS_TARGET_CHAIN_MAY_HAVE_MANAGER);
  }
  
  nsIDOMEventTarget* CurrentTarget()
  {
    return mTarget;
  }

  





  nsresult HandleEventTargetChain(nsEventChainPostVisitor& aVisitor,
                                  nsDispatchingCallback* aCallback,
                                  bool aMayHaveNewListenerManagers,
                                  nsCxPusher* aPusher);

  



  nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  



  nsresult HandleEvent(nsEventChainPostVisitor& aVisitor,
                       bool aMayHaveNewListenerManagers,
                       nsCxPusher* aPusher)
  {
    if (WantsWillHandleEvent()) {
      mTarget->WillHandleEvent(aVisitor);
    }
    if (aVisitor.mEvent->mFlags.mPropagationStopped) {
      return NS_OK;
    }
    if (!mManager) {
      if (!MayHaveListenerManager() && !aMayHaveNewListenerManagers) {
        return NS_OK;
      }
      mManager =
        static_cast<nsEventListenerManager*>(mTarget->GetListenerManager(false));
    }
    if (mManager) {
      NS_ASSERTION(aVisitor.mEvent->currentTarget == nullptr,
                   "CurrentTarget should be null!");
      mManager->HandleEvent(aVisitor.mPresContext, aVisitor.mEvent,
                            &aVisitor.mDOMEvent,
                            CurrentTarget(),
                            &aVisitor.mEventStatus,
                            aPusher);
      NS_ASSERTION(aVisitor.mEvent->currentTarget == nullptr,
                   "CurrentTarget should be null!");
    }
    return NS_OK;
  }

  


  nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor,
                           nsCxPusher* aPusher);

  static uint32_t MaxEtciCount() { return sMaxEtciCount; }

  static void ResetMaxEtciCount()
  {
    MOZ_ASSERT(!sCurrentEtciCount, "Wrong time to call ResetMaxEtciCount()!");
    sMaxEtciCount = 0;
  }

  nsCOMPtr<nsIDOMEventTarget>       mTarget;
  nsEventTargetChainItem*           mChild;
  union {
    nsEventTargetChainItem*         mParent;
     
    nsEventTargetChainItem*         mNext;
  };
  uint16_t                          mFlags;
  uint16_t                          mItemFlags;
  nsCOMPtr<nsISupports>             mItemData;
  
  nsCOMPtr<nsIDOMEventTarget>       mNewTarget;
  
  nsRefPtr<nsEventListenerManager>  mManager;

  static uint32_t                   sMaxEtciCount;
  static uint32_t                   sCurrentEtciCount;
};

uint32_t nsEventTargetChainItem::sMaxEtciCount = 0;
uint32_t nsEventTargetChainItem::sCurrentEtciCount = 0;

nsEventTargetChainItem::nsEventTargetChainItem(nsIDOMEventTarget* aTarget,
                                               nsEventTargetChainItem* aChild)
: mTarget(aTarget), mChild(aChild), mParent(nullptr), mFlags(0), mItemFlags(0)
{
  MOZ_ASSERT(!aTarget || mTarget == aTarget->GetTargetForEventTargetChain());
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
  SetMayHaveListenerManager(aVisitor.mMayHaveListenerManager);
  mItemFlags = aVisitor.mItemFlags;
  mItemData = aVisitor.mItemData;
  return rv;
}

nsresult
nsEventTargetChainItem::PostHandleEvent(nsEventChainPostVisitor& aVisitor,
                                        nsCxPusher* aPusher)
{
  aPusher->Pop();
  aVisitor.mItemFlags = mItemFlags;
  aVisitor.mItemData = mItemData;
  mTarget->PostHandleEvent(aVisitor);
  return NS_OK;
}

nsresult
nsEventTargetChainItem::HandleEventTargetChain(
                          nsEventChainPostVisitor& aVisitor,
                          nsDispatchingCallback* aCallback,
                          bool aMayHaveNewListenerManagers,
                          nsCxPusher* aPusher)
{
  uint32_t createdELMs = nsEventListenerManager::sCreatedCount;
  
  nsCOMPtr<nsIDOMEventTarget> firstTarget = aVisitor.mEvent->target;

  
  nsEventTargetChainItem* item = this;
  aVisitor.mEvent->mFlags.mInCapturePhase = true;
  aVisitor.mEvent->mFlags.mInBubblingPhase = false;
  while (item->mChild) {
    if ((!aVisitor.mEvent->mFlags.mNoContentDispatch ||
         item->ForceContentDispatch()) &&
        !aVisitor.mEvent->mFlags.mPropagationStopped) {
      item->HandleEvent(aVisitor,
                        aMayHaveNewListenerManagers ||
                        createdELMs != nsEventListenerManager::sCreatedCount,
                        aPusher);
    }

    if (item->GetNewTarget()) {
      
      nsEventTargetChainItem* nextTarget = item->mChild;
      while (nextTarget) {
        nsIDOMEventTarget* newTarget = nextTarget->GetNewTarget();
        if (newTarget) {
          aVisitor.mEvent->target = newTarget;
          break;
        }
        nextTarget = nextTarget->mChild;
      }
    }

    item = item->mChild;
  }

  
  aVisitor.mEvent->mFlags.mInBubblingPhase = true;
  if (!aVisitor.mEvent->mFlags.mPropagationStopped &&
      (!aVisitor.mEvent->mFlags.mNoContentDispatch ||
       item->ForceContentDispatch())) {
    item->HandleEvent(aVisitor,
                      aMayHaveNewListenerManagers ||
                      createdELMs != nsEventListenerManager::sCreatedCount,
                      aPusher);
  }
  if (aVisitor.mEvent->mFlags.mInSystemGroup) {
    item->PostHandleEvent(aVisitor, aPusher);
  }

  
  aVisitor.mEvent->mFlags.mInCapturePhase = false;
  item = item->mParent;
  while (item) {
    nsIDOMEventTarget* newTarget = item->GetNewTarget();
    if (newTarget) {
      
      
      aVisitor.mEvent->target = newTarget;
    }

    if (aVisitor.mEvent->mFlags.mBubbles || newTarget) {
      if ((!aVisitor.mEvent->mFlags.mNoContentDispatch ||
           item->ForceContentDispatch()) &&
          !aVisitor.mEvent->mFlags.mPropagationStopped) {
        item->HandleEvent(aVisitor,
                          createdELMs != nsEventListenerManager::sCreatedCount,
                          aPusher);
      }
      if (aVisitor.mEvent->mFlags.mInSystemGroup) {
        item->PostHandleEvent(aVisitor, aPusher);
      }
    }
    item = item->mParent;
  }
  aVisitor.mEvent->mFlags.mInBubblingPhase = false;

  if (!aVisitor.mEvent->mFlags.mInSystemGroup) {
    
    
    aVisitor.mEvent->mFlags.mPropagationStopped = false;
    aVisitor.mEvent->mFlags.mImmediatePropagationStopped = false;

    
    aVisitor.mEvent->target = aVisitor.mEvent->originalTarget;

    
    
    if (aCallback) {
      aPusher->Pop();
      aCallback->HandleEvent(aVisitor);
    }

    
    
    aVisitor.mEvent->target = firstTarget;
    aVisitor.mEvent->mFlags.mInSystemGroup = true;
    HandleEventTargetChain(aVisitor,
                           aCallback,
                           createdELMs != nsEventListenerManager::sCreatedCount,
                           aPusher);
    aVisitor.mEvent->mFlags.mInSystemGroup = false;

    
    
    aVisitor.mEvent->mFlags.mPropagationStopped = false;
    aVisitor.mEvent->mFlags.mImmediatePropagationStopped = false;
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
        static const int32_t kNumBuckets = sizeof(kBucketSizes) / sizeof(size_t);
        static const int32_t kInitialPoolSize =
          sizeof(nsEventTargetChainItem) * NS_CHAIN_POOL_SIZE;
        nsresult rv = sEtciPool->Init("EventTargetChainItem Pool", kBucketSizes,
                                      kNumBuckets, kInitialPoolSize);
        if (NS_FAILED(rv)) {
          delete sEtciPool;
          sEtciPool = nullptr;
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
        gCachedETCI = nullptr;
        delete sEtciPool;
        sEtciPool = nullptr;
        nsEventTargetChainItem::ResetMaxEtciCount();
      }
    }
  }

  static void Shutdown()
  {
    if (!sEtciPoolUsers) {
      gCachedETCI = nullptr;
      delete sEtciPool;
      sEtciPool = nullptr;
      nsEventTargetChainItem::ResetMaxEtciCount();
    }
  }

  nsFixedSizeAllocator* GetPool() { return sEtciPool; }

  static nsFixedSizeAllocator* sEtciPool;
  static int32_t               sEtciPoolUsers;
};

nsFixedSizeAllocator* ChainItemPool::sEtciPool = nullptr;
int32_t ChainItemPool::sEtciPoolUsers = 0;

void NS_ShutdownChainItemPool() { ChainItemPool::Shutdown(); }

nsEventTargetChainItem*
EventTargetChainItemForChromeTarget(ChainItemPool& aPool,
                                    nsINode* aNode,
                                    nsEventTargetChainItem* aChild = nullptr)
{
  if (!aNode->IsInDoc()) {
    return nullptr;
  }
  nsPIDOMWindow* win = aNode->OwnerDoc()->GetInnerWindow();
  nsIDOMEventTarget* piTarget = win ? win->GetParentTarget() : nullptr;
  NS_ENSURE_TRUE(piTarget, nullptr);

  nsEventTargetChainItem* etci =
    nsEventTargetChainItem::Create(aPool.GetPool(),
                                   piTarget->GetTargetForEventTargetChain(),
                                   aChild);
  NS_ENSURE_TRUE(etci, nullptr);
  if (!etci->IsValid()) {
    nsEventTargetChainItem::Destroy(aPool.GetPool(), etci);
    return nullptr;
  }
  return etci;
}

 nsresult
nsEventDispatcher::Dispatch(nsISupports* aTarget,
                            nsPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent* aDOMEvent,
                            nsEventStatus* aEventStatus,
                            nsDispatchingCallback* aCallback,
                            nsCOMArray<nsIDOMEventTarget>* aTargets)
{
  SAMPLE_LABEL("nsEventDispatcher", "Dispatch");
  NS_ASSERTION(aEvent, "Trying to dispatch without nsEvent!");
  NS_ENSURE_TRUE(!aEvent->mFlags.mIsBeingDispatched,
                 NS_ERROR_ILLEGAL_VALUE);
  NS_ASSERTION(!aTargets || !aEvent->message, "Wrong parameters!");

  
  
  
  NS_ENSURE_TRUE(aEvent->message || !aDOMEvent || aTargets,
                 NS_ERROR_DOM_INVALID_STATE_ERR);

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(aTarget);

  bool retargeted = false;

  if (aEvent->mFlags.mRetargetToNonNativeAnonymous) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(target);
    if (content && content->IsInNativeAnonymousSubtree()) {
      nsCOMPtr<nsIDOMEventTarget> newTarget =
        do_QueryInterface(content->FindFirstNonChromeOnlyAccessContent());
      NS_ENSURE_STATE(newTarget);

      aEvent->originalTarget = target;
      target = newTarget;
      retargeted = true;
    }
  }

  if (aEvent->mFlags.mOnlyChromeDispatch) {
    nsCOMPtr<nsINode> node = do_QueryInterface(aTarget);
    if (!node) {
      nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aTarget);
      if (win) {
        node = do_QueryInterface(win->GetExtantDocument());
      }
    }

    NS_ENSURE_STATE(node);
    nsIDocument* doc = node->OwnerDoc();
    if (!nsContentUtils::IsChromeDoc(doc)) {
      nsPIDOMWindow* win = doc ? doc->GetInnerWindow() : nullptr;
      
      nsIDOMEventTarget* piTarget = win ? win->GetParentTarget() : nullptr;
      NS_ENSURE_TRUE(piTarget, NS_OK);
      
      
      aEvent->target = target;
      
      target = piTarget;
    }
  }

#ifdef DEBUG
  if (!nsContentUtils::IsSafeToRunScript()) {
    nsresult rv = NS_ERROR_FAILURE;
    if (target->GetContextForEventHandlers(&rv) ||
        NS_FAILED(rv)) {
      nsCOMPtr<nsINode> node = do_QueryInterface(target);
      if (node && nsContentUtils::IsChromeDoc(node->OwnerDoc())) {
        NS_WARNING("Fix the caller!");
      } else {
        NS_ERROR("This is unsafe! Fix the caller!");
      }
    }
  }

  if (aDOMEvent) {
    nsEvent* innerEvent = aDOMEvent->GetInternalNSEvent();
    NS_ASSERTION(innerEvent == aEvent,
                  "The inner event of aDOMEvent is not the same as aEvent!");
  }
#endif

  nsresult rv = NS_OK;
  bool externalDOMEvent = !!(aDOMEvent);

  
  
  nsRefPtr<nsPresContext> kungFuDeathGrip(aPresContext);
  ChainItemPool pool;
  NS_ENSURE_TRUE(pool.GetPool(), NS_ERROR_OUT_OF_MEMORY);

  
  nsEventTargetChainItem* targetEtci =
    nsEventTargetChainItem::Create(pool.GetPool(),
                                   target->GetTargetForEventTargetChain());
  NS_ENSURE_TRUE(targetEtci, NS_ERROR_OUT_OF_MEMORY);
  if (!targetEtci->IsValid()) {
    nsEventTargetChainItem::Destroy(pool.GetPool(), targetEtci);
    return NS_ERROR_FAILURE;
  }

  
  
  if (!aEvent->target) {
    
    
    aEvent->target = targetEtci->CurrentTarget();
  } else {
    
    
    
    
    
    aEvent->target = aEvent->target->GetTargetForEventTargetChain();
    NS_ENSURE_STATE(aEvent->target);
  }

  if (retargeted) {
    aEvent->originalTarget =
      aEvent->originalTarget->GetTargetForEventTargetChain();
    NS_ENSURE_STATE(aEvent->originalTarget);
  }
  else {
    aEvent->originalTarget = aEvent->target;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(aEvent->originalTarget);
  bool isInAnon = (content && content->IsInAnonymousSubtree());

  aEvent->mFlags.mIsBeingDispatched = true;

  
  
  nsEventStatus status = aEventStatus ? *aEventStatus : nsEventStatus_eIgnore;
  nsEventChainPreVisitor preVisitor(aPresContext, aEvent, aDOMEvent, status,
                                    isInAnon);
  targetEtci->PreHandleEvent(preVisitor);

  if (!preVisitor.mCanHandle && preVisitor.mAutomaticChromeDispatch && content) {
    
    nsEventTargetChainItem::Destroy(pool.GetPool(), targetEtci);
    targetEtci = EventTargetChainItemForChromeTarget(pool, content);
    NS_ENSURE_STATE(targetEtci);
    targetEtci->PreHandleEvent(preVisitor);
  }
  if (preVisitor.mCanHandle) {
    
    
    nsCOMPtr<nsIDOMEventTarget> t = aEvent->target;
    targetEtci->SetNewTarget(t);
    nsEventTargetChainItem* topEtci = targetEtci;
    while (preVisitor.mParentTarget) {
      nsIDOMEventTarget* parentTarget = preVisitor.mParentTarget;
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
        parentEtci = nullptr;
        if (preVisitor.mAutomaticChromeDispatch && content) {
          
          
          nsCOMPtr<nsINode> disabledTarget = do_QueryInterface(parentTarget);
          if (disabledTarget) {
            parentEtci = EventTargetChainItemForChromeTarget(pool,
                                                             disabledTarget,
                                                             topEtci);
            if (parentEtci) {
              parentEtci->PreHandleEvent(preVisitor);
              if (preVisitor.mCanHandle) {
                targetEtci->SetNewTarget(parentTarget);
                topEtci = parentEtci;
                continue;
              }
            }
          }
        }
        break;
      }
    }
    if (NS_SUCCEEDED(rv)) {
      if (aTargets) {
        aTargets->Clear();
        nsEventTargetChainItem* item = targetEtci;
        while(item) {
          aTargets->AppendObject(item->CurrentTarget()->GetTargetForDOMEvent());
          item = item->mParent;
        }
      } else {
        
        nsEventChainPostVisitor postVisitor(preVisitor);
        nsCxPusher pusher;
        rv = topEtci->HandleEventTargetChain(postVisitor,
                                             aCallback,
                                             false,
                                             &pusher);
  
        preVisitor.mEventStatus = postVisitor.mEventStatus;
        
        if (!preVisitor.mDOMEvent && postVisitor.mDOMEvent) {
          preVisitor.mDOMEvent = postVisitor.mDOMEvent;
        }
      }
    }
  }

  nsEventTargetChainItem::Destroy(pool.GetPool(), targetEtci);
  targetEtci = nullptr;

  aEvent->mFlags.mIsBeingDispatched = false;
  aEvent->mFlags.mDispatchedAtLeastOnce = true;

  if (!externalDOMEvent && preVisitor.mDOMEvent) {
    
    
    nsrefcnt rc = 0;
    NS_RELEASE2(preVisitor.mDOMEvent, rc);
    if (preVisitor.mDOMEvent) {
      preVisitor.mDOMEvent->DuplicatePrivateData();
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
    nsEvent* innerEvent = aDOMEvent->GetInternalNSEvent();
    NS_ENSURE_TRUE(innerEvent, NS_ERROR_ILLEGAL_VALUE);

    bool dontResetTrusted = false;
    if (innerEvent->mFlags.mDispatchedAtLeastOnce) {
      innerEvent->target = nullptr;
      innerEvent->originalTarget = nullptr;
    } else {
      aDOMEvent->GetIsTrusted(&dontResetTrusted);
    }

    if (!dontResetTrusted) {
      
      aDOMEvent->SetTrusted(nsContentUtils::IsCallerChrome());
    }

    return nsEventDispatcher::Dispatch(aTarget, aPresContext, innerEvent,
                                       aDOMEvent, aEventStatus);
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
  *aDOMEvent = nullptr;

  if (aEvent) {
    switch(aEvent->eventStructType) {
    case NS_MUTATION_EVENT:
      return NS_NewDOMMutationEvent(aDOMEvent, aPresContext,
                                    static_cast<nsMutationEvent*>(aEvent));
    case NS_GUI_EVENT:
    case NS_SCROLLPORT_EVENT:
    case NS_UI_EVENT:
      return NS_NewDOMUIEvent(aDOMEvent, aPresContext,
                              static_cast<nsGUIEvent*>(aEvent));
    case NS_SCROLLAREA_EVENT:
      return NS_NewDOMScrollAreaEvent(aDOMEvent, aPresContext,
                                      static_cast<nsScrollAreaEvent *>(aEvent));
    case NS_KEY_EVENT:
      return NS_NewDOMKeyboardEvent(aDOMEvent, aPresContext,
                                    static_cast<nsKeyEvent*>(aEvent));
    case NS_COMPOSITION_EVENT:
      return NS_NewDOMCompositionEvent(
        aDOMEvent, aPresContext, static_cast<nsCompositionEvent*>(aEvent));
    case NS_MOUSE_EVENT:
      return NS_NewDOMMouseEvent(aDOMEvent, aPresContext,
                                 static_cast<nsInputEvent*>(aEvent));
    case NS_MOUSE_SCROLL_EVENT:
      return NS_NewDOMMouseScrollEvent(aDOMEvent, aPresContext,
                                 static_cast<nsInputEvent*>(aEvent));
    case NS_WHEEL_EVENT:
      return NS_NewDOMWheelEvent(aDOMEvent, aPresContext,
                                 static_cast<widget::WheelEvent*>(aEvent));
    case NS_DRAG_EVENT:
      return NS_NewDOMDragEvent(aDOMEvent, aPresContext,
                                 static_cast<nsDragEvent*>(aEvent));
    case NS_TEXT_EVENT:
      return NS_NewDOMTextEvent(aDOMEvent, aPresContext,
                                static_cast<nsTextEvent*>(aEvent));
    case NS_SVG_EVENT:
      return NS_NewDOMSVGEvent(aDOMEvent, aPresContext,
                               aEvent);
    case NS_SVGZOOM_EVENT:
      return NS_NewDOMSVGZoomEvent(aDOMEvent, aPresContext,
                                   static_cast<nsGUIEvent*>(aEvent));
    case NS_SMIL_TIME_EVENT:
      return NS_NewDOMTimeEvent(aDOMEvent, aPresContext, aEvent);

    case NS_COMMAND_EVENT:
      return NS_NewDOMCommandEvent(aDOMEvent, aPresContext,
                                   static_cast<nsCommandEvent*>(aEvent));
    case NS_SIMPLE_GESTURE_EVENT:
      return NS_NewDOMSimpleGestureEvent(aDOMEvent, aPresContext,
                                         static_cast<nsSimpleGestureEvent*>(aEvent));
    case NS_TOUCH_EVENT:
      return NS_NewDOMTouchEvent(aDOMEvent, aPresContext,
                                 static_cast<nsTouchEvent*>(aEvent));
    case NS_TRANSITION_EVENT:
      return NS_NewDOMTransitionEvent(aDOMEvent, aPresContext,
                                      static_cast<nsTransitionEvent*>(aEvent));
    case NS_ANIMATION_EVENT:
      return NS_NewDOMAnimationEvent(aDOMEvent, aPresContext,
                                     static_cast<nsAnimationEvent*>(aEvent));
    default:
      
      return NS_NewDOMEvent(aDOMEvent, aPresContext, aEvent);
    }
  }

  

  if (aEventType.LowerCaseEqualsLiteral("mouseevent") ||
      aEventType.LowerCaseEqualsLiteral("mouseevents") ||
      aEventType.LowerCaseEqualsLiteral("popupevents"))
    return NS_NewDOMMouseEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mousescrollevents"))
    return NS_NewDOMMouseScrollEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("dragevent") ||
      aEventType.LowerCaseEqualsLiteral("dragevents"))
    return NS_NewDOMDragEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("keyboardevent") ||
      aEventType.LowerCaseEqualsLiteral("keyevents"))
    return NS_NewDOMKeyboardEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("compositionevent"))
    return NS_NewDOMCompositionEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mutationevent") ||
        aEventType.LowerCaseEqualsLiteral("mutationevents"))
    return NS_NewDOMMutationEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("textevent") ||
      aEventType.LowerCaseEqualsLiteral("textevents"))
    return NS_NewDOMTextEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("popupblockedevents"))
    return NS_NewDOMPopupBlockedEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("deviceorientationevent"))
    return NS_NewDOMDeviceOrientationEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("devicemotionevent"))
    return NS_NewDOMDeviceMotionEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("uievent") ||
      aEventType.LowerCaseEqualsLiteral("uievents"))
    return NS_NewDOMUIEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("event") ||
      aEventType.LowerCaseEqualsLiteral("events") ||
      aEventType.LowerCaseEqualsLiteral("htmlevents"))
    return NS_NewDOMEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("svgevent") ||
      aEventType.LowerCaseEqualsLiteral("svgevents"))
    return NS_NewDOMSVGEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("svgzoomevent") ||
      aEventType.LowerCaseEqualsLiteral("svgzoomevents"))
    return NS_NewDOMSVGZoomEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("timeevent") ||
      aEventType.LowerCaseEqualsLiteral("timeevents"))
    return NS_NewDOMTimeEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("xulcommandevent") ||
      aEventType.LowerCaseEqualsLiteral("xulcommandevents"))
    return NS_NewDOMXULCommandEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("commandevent") ||
      aEventType.LowerCaseEqualsLiteral("commandevents"))
    return NS_NewDOMCommandEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("elementreplace"))
    return NS_NewDOMElementReplaceEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("datacontainerevent") ||
      aEventType.LowerCaseEqualsLiteral("datacontainerevents"))
    return NS_NewDOMDataContainerEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("messageevent"))
    return NS_NewDOMMessageEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("notifypaintevent"))
    return NS_NewDOMNotifyPaintEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("simplegestureevent"))
    return NS_NewDOMSimpleGestureEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("beforeunloadevent"))
    return NS_NewDOMBeforeUnloadEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("pagetransition"))
    return NS_NewDOMPageTransitionEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("domtransaction"))
    return NS_NewDOMDOMTransactionEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("scrollareaevent"))
    return NS_NewDOMScrollAreaEvent(aDOMEvent, aPresContext, nullptr);
  
  
  if (aEventType.LowerCaseEqualsLiteral("transitionevent"))
    return NS_NewDOMTransitionEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("animationevent"))
    return NS_NewDOMAnimationEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("popstateevent"))
    return NS_NewDOMPopStateEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mozaudioavailableevent"))
    return NS_NewDOMAudioAvailableEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("closeevent"))
    return NS_NewDOMCloseEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("touchevent") &&
      nsDOMTouchEvent::PrefEnabled())
    return NS_NewDOMTouchEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("hashchangeevent"))
    return NS_NewDOMHashChangeEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("customevent"))
    return NS_NewDOMCustomEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mozsmsevent"))
    return NS_NewDOMMozSmsEvent(aDOMEvent, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("storageevent")) {
    return NS_NewDOMStorageEvent(aDOMEvent, aPresContext, nullptr);
  }
    

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
