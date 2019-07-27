






#include "TouchManager.h"
#include "nsPresShell.h"

bool TouchManager::gPreventMouseEvents = false;
nsRefPtrHashtable<nsUint32HashKey, dom::Touch>* TouchManager::gCaptureTouchList;

 void
TouchManager::InitializeStatics()
{
  NS_ASSERTION(!gCaptureTouchList, "InitializeStatics called multiple times!");
  gCaptureTouchList = new nsRefPtrHashtable<nsUint32HashKey, dom::Touch>;
}

 void
TouchManager::ReleaseStatics()
{
  NS_ASSERTION(gCaptureTouchList, "ReleaseStatics called without Initialize!");
  delete gCaptureTouchList;
  gCaptureTouchList = nullptr;
}

void
TouchManager::Init(PresShell* aPresShell, nsIDocument* aDocument)
{
  mPresShell = aPresShell;
  mDocument = aDocument;
}

void
TouchManager::Destroy()
{
  EvictTouches();
  mDocument = nullptr;
  mPresShell = nullptr;
}

static void
EvictTouchPoint(nsRefPtr<dom::Touch>& aTouch,
                nsIDocument* aLimitToDocument = nullptr)
{
  nsCOMPtr<nsINode> node(do_QueryInterface(aTouch->mTarget));
  if (node) {
    nsIDocument* doc = node->GetCurrentDoc();
    if (doc && (!aLimitToDocument || aLimitToDocument == doc)) {
      nsIPresShell* presShell = doc->GetShell();
      if (presShell) {
        nsIFrame* frame = presShell->GetRootFrame();
        if (frame) {
          nsPoint pt(aTouch->mRefPoint.x, aTouch->mRefPoint.y);
          nsCOMPtr<nsIWidget> widget = frame->GetView()->GetNearestWidget(&pt);
          if (widget) {
            WidgetTouchEvent event(true, NS_TOUCH_END, widget);
            event.widget = widget;
            event.time = PR_IntervalNow();
            event.touches.AppendElement(aTouch);
            nsEventStatus status;
            widget->DispatchEvent(&event, status);
            return;
          }
        }
      }
    }
  }
  if (!node || !aLimitToDocument || node->OwnerDoc() == aLimitToDocument) {
    
    TouchManager::gCaptureTouchList->Remove(aTouch->Identifier());
  }
}

static PLDHashOperator
AppendToTouchList(const uint32_t& aKey, nsRefPtr<dom::Touch>& aData, void *aTouchList)
{
  WidgetTouchEvent::TouchArray* touches =
    static_cast<WidgetTouchEvent::TouchArray*>(aTouchList);
  aData->mChanged = false;
  touches->AppendElement(aData);
  return PL_DHASH_NEXT;
}

void
TouchManager::EvictTouches()
{
  WidgetTouchEvent::AutoTouchArray touches;
  gCaptureTouchList->Enumerate(&AppendToTouchList, &touches);
  for (uint32_t i = 0; i < touches.Length(); ++i) {
    EvictTouchPoint(touches[i], mDocument);
  }
}

bool
TouchManager::PreHandleEvent(WidgetEvent* aEvent,
                             nsEventStatus* aStatus,
                             bool& aTouchIsNew,
                             bool& aIsHandlingUserInput,
                             nsCOMPtr<nsIContent>& aCurrentEventContent)
{
  switch (aEvent->message) {
    case NS_TOUCH_START: {
      aIsHandlingUserInput = true;
      WidgetTouchEvent* touchEvent = aEvent->AsTouchEvent();
      
      
      
      if (touchEvent->touches.Length() == 1) {
        WidgetTouchEvent::AutoTouchArray touches;
        gCaptureTouchList->Enumerate(&AppendToTouchList, (void *)&touches);
        for (uint32_t i = 0; i < touches.Length(); ++i) {
          EvictTouchPoint(touches[i]);
        }
      }
      
      for (uint32_t i = 0; i < touchEvent->touches.Length(); ++i) {
        dom::Touch* touch = touchEvent->touches[i];
        int32_t id = touch->Identifier();
        if (!gCaptureTouchList->Get(id, nullptr)) {
          
          touch->mChanged = true;
        }
        touch->mMessage = aEvent->message;
        gCaptureTouchList->Put(id, touch);
      }
      break;
    }
    case NS_TOUCH_MOVE: {
      
      WidgetTouchEvent* touchEvent = aEvent->AsTouchEvent();
      WidgetTouchEvent::TouchArray& touches = touchEvent->touches;
      bool haveChanged = false;
      for (int32_t i = touches.Length(); i; ) {
        --i;
        dom::Touch* touch = touches[i];
        if (!touch) {
          continue;
        }
        int32_t id = touch->Identifier();
        touch->mMessage = aEvent->message;

        nsRefPtr<dom::Touch> oldTouch = gCaptureTouchList->GetWeak(id);
        if (!oldTouch) {
          touches.RemoveElementAt(i);
          continue;
        }
        if (!touch->Equals(oldTouch)) {
          touch->mChanged = true;
          haveChanged = true;
        }

        nsCOMPtr<dom::EventTarget> targetPtr = oldTouch->mTarget;
        if (!targetPtr) {
          touches.RemoveElementAt(i);
          continue;
        }
        touch->SetTarget(targetPtr);

        gCaptureTouchList->Put(id, touch);
        
        
        if (oldTouch->mMessage != touch->mMessage) {
          aTouchIsNew = true;
        }
      }
      
      if (!haveChanged) {
        if (aTouchIsNew) {
          
          
          
          
          
          
          for (uint32_t i = 0; i < touchEvent->touches.Length(); ++i) {
            if (touchEvent->touches[i]) {
              touchEvent->touches[i]->mChanged = true;
              break;
            }
          }
        } else {
          if (gPreventMouseEvents) {
            *aStatus = nsEventStatus_eConsumeNoDefault;
          }
          return false;
        }
      }
      break;
    }
    case NS_TOUCH_END:
      aIsHandlingUserInput = true;
      
    case NS_TOUCH_CANCEL: {
      
      
      WidgetTouchEvent* touchEvent = aEvent->AsTouchEvent();
      WidgetTouchEvent::TouchArray& touches = touchEvent->touches;
      for (uint32_t i = 0; i < touches.Length(); ++i) {
        dom::Touch* touch = touches[i];
        if (!touch) {
          continue;
        }
        touch->mMessage = aEvent->message;
        touch->mChanged = true;

        int32_t id = touch->Identifier();
        nsRefPtr<dom::Touch> oldTouch = gCaptureTouchList->GetWeak(id);
        if (!oldTouch) {
          continue;
        }
        nsCOMPtr<EventTarget> targetPtr = oldTouch->mTarget;

        aCurrentEventContent = do_QueryInterface(targetPtr);
        touch->SetTarget(targetPtr);
        gCaptureTouchList->Remove(id);
      }
      
      gCaptureTouchList->Enumerate(&AppendToTouchList, (void *)&touches);
      break;
    }
    default:
      break;
  }
  return true;
}
