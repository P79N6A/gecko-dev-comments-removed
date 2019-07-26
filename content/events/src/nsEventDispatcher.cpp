





#include "nsEventDispatcher.h"
#include "nsDOMEvent.h"
#include "nsPresContext.h"
#include "nsEventListenerManager.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsError.h"
#include "nsMutationEvent.h"
#include <new>
#include "nsINode.h"
#include "nsPIDOMWindow.h"
#include "nsFrameLoader.h"
#include "nsDOMTouchEvent.h"
#include "GeckoProfiler.h"
#include "GeneratedEvents.h"
#include "mozilla/dom/EventTarget.h"

using namespace mozilla;
using namespace mozilla::dom;

class ELMCreationDetector
{
public:
  ELMCreationDetector() :
    
    mDefault(!NS_IsMainThread()),
    mInitialCount(mDefault ? 0 : nsEventListenerManager::sMainThreadCreatedCount)
  {
  }

  bool MayHaveNewListenerManager()
  {
    return mDefault ||
           mInitialCount != nsEventListenerManager::sMainThreadCreatedCount;
  }
private:
  bool mDefault;
  uint32_t mInitialCount;
};

#define NS_TARGET_CHAIN_FORCE_CONTENT_DISPATCH  (1 << 0)
#define NS_TARGET_CHAIN_WANTS_WILL_HANDLE_EVENT (1 << 1)
#define NS_TARGET_CHAIN_MAY_HAVE_MANAGER        (1 << 2)


class nsEventTargetChainItem
{
private:
  nsEventTargetChainItem(EventTarget* aTarget,
                         nsEventTargetChainItem* aChild = nullptr);
public:
  nsEventTargetChainItem()
  : mChild(nullptr), mParent(nullptr), mFlags(0), mItemFlags(0)
  {
  }

  static nsEventTargetChainItem* Create(nsTArray<nsEventTargetChainItem>& aPool,
                                        EventTarget* aTarget,
                                        nsEventTargetChainItem* aChild = nullptr)
  {
    
    return new (aPool.AppendElement()) nsEventTargetChainItem(aTarget, aChild);
  }

  static void Destroy(nsEventTargetChainItem* aItem)
  {
    
    
    if (aItem->mChild) {
      aItem->mChild->mParent = nullptr;
      aItem->mChild = nullptr;
    }
  }

  bool IsValid()
  {
    NS_WARN_IF_FALSE(!!(mTarget), "Event target is not valid!");
    return !!(mTarget);
  }

  EventTarget* GetNewTarget()
  {
    return mNewTarget;
  }

  void SetNewTarget(EventTarget* aNewTarget)
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
  
  EventTarget* CurrentTarget()
  {
    return mTarget;
  }

  





  nsresult HandleEventTargetChain(nsEventChainPostVisitor& aVisitor,
                                  nsDispatchingCallback* aCallback,
                                  ELMCreationDetector& aCd,
                                  nsCxPusher* aPusher);

  



  nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  



  nsresult HandleEvent(nsEventChainPostVisitor& aVisitor,
                       ELMCreationDetector& aCd,
                       nsCxPusher* aPusher)
  {
    if (WantsWillHandleEvent()) {
      mTarget->WillHandleEvent(aVisitor);
    }
    if (aVisitor.mEvent->mFlags.mPropagationStopped) {
      return NS_OK;
    }
    if (!mManager) {
      if (!MayHaveListenerManager() && !aCd.MayHaveNewListenerManager()) {
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

  nsCOMPtr<EventTarget>             mTarget;
  nsEventTargetChainItem*           mChild;
  nsEventTargetChainItem*           mParent;
  uint16_t                          mFlags;
  uint16_t                          mItemFlags;
  nsCOMPtr<nsISupports>             mItemData;
  
  nsCOMPtr<EventTarget>             mNewTarget;
  
  nsRefPtr<nsEventListenerManager>  mManager;
};

nsEventTargetChainItem::nsEventTargetChainItem(EventTarget* aTarget,
                                               nsEventTargetChainItem* aChild)
: mTarget(aTarget), mChild(aChild), mParent(nullptr), mFlags(0), mItemFlags(0)
{
  MOZ_ASSERT(!aTarget || mTarget == aTarget->GetTargetForEventTargetChain());
  if (mChild) {
    mChild->mParent = this;
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
                          ELMCreationDetector& aCd,
                          nsCxPusher* aPusher)
{
  
  nsCOMPtr<EventTarget> firstTarget = aVisitor.mEvent->target;

  
  nsEventTargetChainItem* item = this;
  aVisitor.mEvent->mFlags.mInCapturePhase = true;
  aVisitor.mEvent->mFlags.mInBubblingPhase = false;
  while (item->mChild) {
    if ((!aVisitor.mEvent->mFlags.mNoContentDispatch ||
         item->ForceContentDispatch()) &&
        !aVisitor.mEvent->mFlags.mPropagationStopped) {
      item->HandleEvent(aVisitor, aCd, aPusher);
    }

    if (item->GetNewTarget()) {
      
      nsEventTargetChainItem* nextTarget = item->mChild;
      while (nextTarget) {
        EventTarget* newTarget = nextTarget->GetNewTarget();
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
    item->HandleEvent(aVisitor, aCd, aPusher);
  }
  if (aVisitor.mEvent->mFlags.mInSystemGroup) {
    item->PostHandleEvent(aVisitor, aPusher);
  }

  
  aVisitor.mEvent->mFlags.mInCapturePhase = false;
  item = item->mParent;
  while (item) {
    EventTarget* newTarget = item->GetNewTarget();
    if (newTarget) {
      
      
      aVisitor.mEvent->target = newTarget;
    }

    if (aVisitor.mEvent->mFlags.mBubbles || newTarget) {
      if ((!aVisitor.mEvent->mFlags.mNoContentDispatch ||
           item->ForceContentDispatch()) &&
          !aVisitor.mEvent->mFlags.mPropagationStopped) {
        item->HandleEvent(aVisitor, aCd, aPusher);
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
                           aCd,
                           aPusher);
    aVisitor.mEvent->mFlags.mInSystemGroup = false;

    
    
    aVisitor.mEvent->mFlags.mPropagationStopped = false;
    aVisitor.mEvent->mFlags.mImmediatePropagationStopped = false;
  }

  return NS_OK;
}

nsEventTargetChainItem*
EventTargetChainItemForChromeTarget(nsTArray<nsEventTargetChainItem>& aPool,
                                    nsINode* aNode,
                                    nsEventTargetChainItem* aChild = nullptr)
{
  if (!aNode->IsInDoc()) {
    return nullptr;
  }
  nsPIDOMWindow* win = aNode->OwnerDoc()->GetInnerWindow();
  EventTarget* piTarget = win ? win->GetParentTarget() : nullptr;
  NS_ENSURE_TRUE(piTarget, nullptr);

  nsEventTargetChainItem* etci =
    nsEventTargetChainItem::Create(aPool,
                                   piTarget->GetTargetForEventTargetChain(),
                                   aChild);
  if (!etci->IsValid()) {
    nsEventTargetChainItem::Destroy(etci);
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
                            nsCOMArray<EventTarget>* aTargets)
{
  PROFILER_LABEL("nsEventDispatcher", "Dispatch");
  NS_ASSERTION(aEvent, "Trying to dispatch without nsEvent!");
  NS_ENSURE_TRUE(!aEvent->mFlags.mIsBeingDispatched,
                 NS_ERROR_DOM_INVALID_STATE_ERR);
  NS_ASSERTION(!aTargets || !aEvent->message, "Wrong parameters!");

  
  
  
  NS_ENSURE_TRUE(aEvent->message || !aDOMEvent || aTargets,
                 NS_ERROR_DOM_INVALID_STATE_ERR);

  nsCOMPtr<EventTarget> target = do_QueryInterface(aTarget);

  bool retargeted = false;

  if (aEvent->mFlags.mRetargetToNonNativeAnonymous) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(target);
    if (content && content->IsInNativeAnonymousSubtree()) {
      nsCOMPtr<EventTarget> newTarget =
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
        node = win->GetExtantDoc();
      }
    }

    NS_ENSURE_STATE(node);
    nsIDocument* doc = node->OwnerDoc();
    if (!nsContentUtils::IsChromeDoc(doc)) {
      nsPIDOMWindow* win = doc ? doc->GetInnerWindow() : nullptr;
      
      EventTarget* piTarget = win ? win->GetParentTarget() : nullptr;
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

  
  nsTArray<nsEventTargetChainItem> pool(128);

  
  nsEventTargetChainItem* targetEtci =
    nsEventTargetChainItem::Create(pool, target->GetTargetForEventTargetChain());
  if (!targetEtci->IsValid()) {
    nsEventTargetChainItem::Destroy(targetEtci);
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
    
    nsEventTargetChainItem::Destroy(targetEtci);
    targetEtci = EventTargetChainItemForChromeTarget(pool, content);
    NS_ENSURE_STATE(targetEtci);
    targetEtci->PreHandleEvent(preVisitor);
  }
  if (preVisitor.mCanHandle) {
    
    
    nsCOMPtr<EventTarget> t = do_QueryInterface(aEvent->target);
    targetEtci->SetNewTarget(t);
    nsEventTargetChainItem* topEtci = targetEtci;
    while (preVisitor.mParentTarget) {
      EventTarget* parentTarget = preVisitor.mParentTarget;
      nsEventTargetChainItem* parentEtci =
        nsEventTargetChainItem::Create(pool, preVisitor.mParentTarget, topEtci);
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
        nsEventTargetChainItem::Destroy(parentEtci);
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
        ELMCreationDetector cd;
        rv = topEtci->HandleEventTargetChain(postVisitor,
                                             aCallback,
                                             cd,
                                             &pusher);

        preVisitor.mEventStatus = postVisitor.mEventStatus;
        
        if (!preVisitor.mDOMEvent && postVisitor.mDOMEvent) {
          preVisitor.mDOMEvent = postVisitor.mDOMEvent;
        }
      }
    }
  }

  nsEventTargetChainItem::Destroy(targetEtci);
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
nsEventDispatcher::CreateEvent(mozilla::dom::EventTarget* aOwner,
                               nsPresContext* aPresContext,
                               nsEvent* aEvent,
                               const nsAString& aEventType,
                               nsIDOMEvent** aDOMEvent)
{
  *aDOMEvent = nullptr;

  if (aEvent) {
    switch(aEvent->eventStructType) {
    case NS_MUTATION_EVENT:
      return NS_NewDOMMutationEvent(aDOMEvent, aOwner, aPresContext,
                                    static_cast<nsMutationEvent*>(aEvent));
    case NS_GUI_EVENT:
    case NS_SCROLLPORT_EVENT:
    case NS_UI_EVENT:
      return NS_NewDOMUIEvent(aDOMEvent, aOwner, aPresContext,
                              static_cast<nsGUIEvent*>(aEvent));
    case NS_SCROLLAREA_EVENT:
      return NS_NewDOMScrollAreaEvent(aDOMEvent, aOwner, aPresContext,
                                      static_cast<nsScrollAreaEvent *>(aEvent));
    case NS_KEY_EVENT:
      return NS_NewDOMKeyboardEvent(aDOMEvent, aOwner, aPresContext,
                                    static_cast<nsKeyEvent*>(aEvent));
    case NS_COMPOSITION_EVENT:
      return NS_NewDOMCompositionEvent(
        aDOMEvent, aOwner,
        aPresContext, static_cast<nsCompositionEvent*>(aEvent));
    case NS_MOUSE_EVENT:
      return NS_NewDOMMouseEvent(aDOMEvent, aOwner, aPresContext,
                                 static_cast<nsInputEvent*>(aEvent));
    case NS_FOCUS_EVENT:
      return NS_NewDOMFocusEvent(aDOMEvent, aOwner, aPresContext,
                                 static_cast<nsFocusEvent*>(aEvent));
    case NS_MOUSE_SCROLL_EVENT:
      return NS_NewDOMMouseScrollEvent(aDOMEvent, aOwner, aPresContext,
                                 static_cast<nsInputEvent*>(aEvent));
    case NS_WHEEL_EVENT:
      return NS_NewDOMWheelEvent(aDOMEvent, aOwner, aPresContext,
                                 static_cast<widget::WheelEvent*>(aEvent));
    case NS_DRAG_EVENT:
      return NS_NewDOMDragEvent(aDOMEvent, aOwner, aPresContext,
                                 static_cast<nsDragEvent*>(aEvent));
    case NS_TEXT_EVENT:
      return NS_NewDOMTextEvent(aDOMEvent, aOwner, aPresContext,
                                static_cast<nsTextEvent*>(aEvent));
    case NS_CLIPBOARD_EVENT:
      return NS_NewDOMClipboardEvent(aDOMEvent, aOwner, aPresContext,
                                     static_cast<nsClipboardEvent*>(aEvent));
    case NS_SVGZOOM_EVENT:
      return NS_NewDOMSVGZoomEvent(aDOMEvent, aOwner, aPresContext,
                                   static_cast<nsGUIEvent*>(aEvent));
    case NS_SMIL_TIME_EVENT:
      return NS_NewDOMTimeEvent(aDOMEvent, aOwner, aPresContext, aEvent);

    case NS_COMMAND_EVENT:
      return NS_NewDOMCommandEvent(aDOMEvent, aOwner, aPresContext,
                                   static_cast<nsCommandEvent*>(aEvent));
    case NS_SIMPLE_GESTURE_EVENT:
      return NS_NewDOMSimpleGestureEvent(aDOMEvent, aOwner, aPresContext,
                                         static_cast<nsSimpleGestureEvent*>(aEvent));
    case NS_TOUCH_EVENT:
      return NS_NewDOMTouchEvent(aDOMEvent, aOwner, aPresContext,
                                 static_cast<nsTouchEvent*>(aEvent));
    case NS_TRANSITION_EVENT:
      return NS_NewDOMTransitionEvent(aDOMEvent, aOwner, aPresContext,
                                      static_cast<nsTransitionEvent*>(aEvent));
    case NS_ANIMATION_EVENT:
      return NS_NewDOMAnimationEvent(aDOMEvent, aOwner, aPresContext,
                                     static_cast<nsAnimationEvent*>(aEvent));
    default:
      
      return NS_NewDOMEvent(aDOMEvent, aOwner, aPresContext, aEvent);
    }
  }

  

  if (aEventType.LowerCaseEqualsLiteral("mouseevent") ||
      aEventType.LowerCaseEqualsLiteral("mouseevents") ||
      aEventType.LowerCaseEqualsLiteral("popupevents"))
    return NS_NewDOMMouseEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mousescrollevents"))
    return NS_NewDOMMouseScrollEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("dragevent") ||
      aEventType.LowerCaseEqualsLiteral("dragevents"))
    return NS_NewDOMDragEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("keyboardevent") ||
      aEventType.LowerCaseEqualsLiteral("keyevents"))
    return NS_NewDOMKeyboardEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("compositionevent"))
    return NS_NewDOMCompositionEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mutationevent") ||
        aEventType.LowerCaseEqualsLiteral("mutationevents"))
    return NS_NewDOMMutationEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("textevent") ||
      aEventType.LowerCaseEqualsLiteral("textevents"))
    return NS_NewDOMTextEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("popupblockedevents"))
    return NS_NewDOMPopupBlockedEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("deviceorientationevent"))
    return NS_NewDOMDeviceOrientationEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("devicemotionevent"))
    return NS_NewDOMDeviceMotionEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("uievent") ||
      aEventType.LowerCaseEqualsLiteral("uievents"))
    return NS_NewDOMUIEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("event") ||
      aEventType.LowerCaseEqualsLiteral("events") ||
      aEventType.LowerCaseEqualsLiteral("htmlevents") ||
      aEventType.LowerCaseEqualsLiteral("svgevent") ||
      aEventType.LowerCaseEqualsLiteral("svgevents"))
    return NS_NewDOMEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("svgzoomevent") ||
      aEventType.LowerCaseEqualsLiteral("svgzoomevents"))
    return NS_NewDOMSVGZoomEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("timeevent") ||
      aEventType.LowerCaseEqualsLiteral("timeevents"))
    return NS_NewDOMTimeEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("xulcommandevent") ||
      aEventType.LowerCaseEqualsLiteral("xulcommandevents"))
    return NS_NewDOMXULCommandEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("commandevent") ||
      aEventType.LowerCaseEqualsLiteral("commandevents"))
    return NS_NewDOMCommandEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("elementreplace"))
    return NS_NewDOMElementReplaceEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("datacontainerevent") ||
      aEventType.LowerCaseEqualsLiteral("datacontainerevents"))
    return NS_NewDOMDataContainerEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("messageevent"))
    return NS_NewDOMMessageEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("notifypaintevent"))
    return NS_NewDOMNotifyPaintEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("simplegestureevent"))
    return NS_NewDOMSimpleGestureEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("beforeunloadevent"))
    return NS_NewDOMBeforeUnloadEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("pagetransition"))
    return NS_NewDOMPageTransitionEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("domtransaction"))
    return NS_NewDOMDOMTransactionEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("scrollareaevent"))
    return NS_NewDOMScrollAreaEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("popstateevent"))
    return NS_NewDOMPopStateEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mozaudioavailableevent"))
    return NS_NewDOMAudioAvailableEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("closeevent"))
    return NS_NewDOMCloseEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("touchevent") &&
      nsDOMTouchEvent::PrefEnabled())
    return NS_NewDOMTouchEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("hashchangeevent"))
    return NS_NewDOMHashChangeEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("customevent"))
    return NS_NewDOMCustomEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mozsmsevent"))
    return NS_NewDOMMozSmsEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("mozmmsevent"))
    return NS_NewDOMMozMmsEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  if (aEventType.LowerCaseEqualsLiteral("storageevent")) {
    return NS_NewDOMStorageEvent(aDOMEvent, aOwner, aPresContext, nullptr);
  }
  
  

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
