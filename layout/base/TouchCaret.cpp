





#include "TouchCaret.h"

#include "nsCOMPtr.h"
#include "nsFrameSelection.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsCanvasFrame.h"
#include "nsRenderingContext.h"
#include "nsPresContext.h"
#include "nsBlockFrame.h"
#include "nsISelectionController.h"
#include "mozilla/Preferences.h"
#include "mozilla/BasicEvents.h"
#include "nsIDOMWindow.h"
#include "nsQueryContentEventResult.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsView.h"
#include "nsDOMTokenList.h"
#include <algorithm>

using namespace mozilla;

#define TOUCHCARET_LOG(...)





static const int32_t kBoundaryAppUnits = 61;

NS_IMPL_ISUPPORTS(TouchCaret, nsISelectionListener)

 int32_t TouchCaret::sTouchCaretMaxDistance = 0;
 int32_t TouchCaret::sTouchCaretExpirationTime = 0;

TouchCaret::TouchCaret(nsIPresShell* aPresShell)
  : mState(TOUCHCARET_NONE),
    mActiveTouchId(-1),
    mCaretCenterToDownPointOffsetY(0),
    mVisible(false)
{
  MOZ_ASSERT(NS_IsMainThread());

  static bool addedTouchCaretPref = false;
  if (!addedTouchCaretPref) {
    Preferences::AddIntVarCache(&sTouchCaretMaxDistance,
                                "touchcaret.distance.threshold");
    Preferences::AddIntVarCache(&sTouchCaretExpirationTime,
                                "touchcaret.expiration.time");
    addedTouchCaretPref = true;
  }

  
  mPresShell = do_GetWeakReference(aPresShell);
  MOZ_ASSERT(mPresShell, "Hey, pres shell should support weak refs");
}

TouchCaret::~TouchCaret()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mTouchCaretExpirationTimer) {
    mTouchCaretExpirationTimer->Cancel();
    mTouchCaretExpirationTimer = nullptr;
  }
}

nsIFrame*
TouchCaret::GetCanvasFrame()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nullptr;
  }
  return presShell->GetCanvasFrame();
}

void
TouchCaret::SetVisibility(bool aVisible)
{
  if (mVisible == aVisible) {
    return;
  }
  mVisible = aVisible;

  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return;
  }
  mozilla::dom::Element* touchCaretElement = presShell->GetTouchCaretElement();
  if (!touchCaretElement) {
    return;
  }

  
  ErrorResult err;
  touchCaretElement->ClassList()->Toggle(NS_LITERAL_STRING("hidden"),
                                         dom::Optional<bool>(!mVisible),
                                         err);
  
  mVisible ? LaunchExpirationTimer() : CancelExpirationTimer();

  
  
  
  
  presShell->SetMayHaveTouchCaret(mVisible);
}

nsRect
TouchCaret::GetTouchFrameRect()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nsRect();
  }

  dom::Element* touchCaretElement = presShell->GetTouchCaretElement();
  if (!touchCaretElement) {
    return nsRect();
  }

  
  nsIFrame* touchCaretFrame = touchCaretElement->GetPrimaryFrame();
  nsRect tcRect = touchCaretFrame->GetRectRelativeToSelf();
  nsIFrame* canvasFrame = GetCanvasFrame();

  nsLayoutUtils::TransformResult rv =
    nsLayoutUtils::TransformRect(touchCaretFrame, canvasFrame, tcRect);
  return rv == nsLayoutUtils::TRANSFORM_SUCCEEDED ? tcRect : nsRect();
}

nsRect
TouchCaret::GetContentBoundary()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nsRect();
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  nsISelection* caretSelection = caret->GetCaretDOMSelection();
  nsRect focusRect;
  nsIFrame* focusFrame = caret->GetGeometry(caretSelection, &focusRect);
  nsIFrame* canvasFrame = GetCanvasFrame();

  
  dom::Element* editingHost = focusFrame->GetContent()->GetEditingHost();
  if (!editingHost) {
    return nsRect();
  }

  nsRect resultRect;
  for (nsIFrame* frame = editingHost->GetPrimaryFrame(); frame;
       frame = frame->GetNextContinuation()) {
    nsRect rect = frame->GetContentRectRelativeToSelf();
    nsLayoutUtils::TransformRect(frame, canvasFrame, rect);
    resultRect = resultRect.Union(rect);

    mozilla::layout::FrameChildListIterator lists(frame);
    for (; !lists.IsDone(); lists.Next()) {
      
      nsFrameList::Enumerator childFrames(lists.CurrentList());
      for (; !childFrames.AtEnd(); childFrames.Next()) {
        nsIFrame* kid = childFrames.get();
        nsRect overflowRect = kid->GetScrollableOverflowRect();
        nsLayoutUtils::TransformRect(kid, canvasFrame, overflowRect);
        resultRect = resultRect.Union(overflowRect);
      }
    }
  }
  
  resultRect.Deflate(kBoundaryAppUnits);

  return resultRect;
}

nscoord
TouchCaret::GetCaretYCenterPosition()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return 0;
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  nsISelection* caretSelection = caret->GetCaretDOMSelection();
  nsRect focusRect;
  nsIFrame* focusFrame = caret->GetGeometry(caretSelection, &focusRect);
  nsRect caretRect = focusFrame->GetRectRelativeToSelf();
  nsIFrame *canvasFrame = GetCanvasFrame();
  nsLayoutUtils::TransformRect(focusFrame, canvasFrame, caretRect);

  return (caretRect.y + caretRect.height / 2);
}

void
TouchCaret::SetTouchFramePos(const nsPoint& aOrigin)
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return;
  }

  mozilla::dom::Element* touchCaretElement = presShell->GetTouchCaretElement();
  if (!touchCaretElement) {
    return;
  }

  
  nsRefPtr<nsPresContext> presContext = presShell->GetPresContext();
  int32_t x = presContext->AppUnitsToIntCSSPixels(aOrigin.x);
  int32_t y = presContext->AppUnitsToIntCSSPixels(aOrigin.y);

  nsAutoString styleStr;
  styleStr.AppendLiteral("left: ");
  styleStr.AppendInt(x);
  styleStr.AppendLiteral("px; top: ");
  styleStr.AppendInt(y);
  styleStr.AppendLiteral("px;");

  touchCaretElement->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
                             styleStr, true);
}

void
TouchCaret::MoveCaret(const nsPoint& movePoint)
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return;
  }

  
  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  nsISelection* caretSelection = caret->GetCaretDOMSelection();
  nsRect focusRect;
  nsIFrame* focusFrame = caret->GetGeometry(caretSelection, &focusRect);
  nsIFrame* scrollable =
    nsLayoutUtils::GetClosestFrameOfType(focusFrame, nsGkAtoms::scrollFrame);

  
  nsIFrame* canvasFrame = GetCanvasFrame();
  if (!canvasFrame) {
    return;
  }
  nsPoint offsetToCanvasFrame = nsPoint(0,0);
  nsLayoutUtils::TransformPoint(scrollable, canvasFrame, offsetToCanvasFrame);
  nsPoint pt = movePoint - offsetToCanvasFrame;

  
  nsIFrame::ContentOffsets offsets =
    scrollable->GetContentOffsetsFromPoint(pt, nsIFrame::SKIP_HIDDEN);

  
  nsWeakFrame weakScrollable = scrollable;
  nsRefPtr<nsFrameSelection> fs = scrollable->GetFrameSelection();
  fs->HandleClick(offsets.content, offsets.StartOffset(),
                  offsets.EndOffset(),
                  false,
                  false,
                  offsets.associateWithNext);

  if (!weakScrollable.IsAlive()) {
    return;
  }

  
  nsIScrollableFrame* saf = do_QueryFrame(scrollable);
  nsIFrame* capturingFrame = saf->GetScrolledFrame();
  offsetToCanvasFrame = nsPoint(0,0);
  nsLayoutUtils::TransformPoint(capturingFrame, canvasFrame, offsetToCanvasFrame);
  pt = movePoint - offsetToCanvasFrame;
  fs->StartAutoScrollTimer(capturingFrame, pt, sAutoScrollTimerDelay);
}

bool
TouchCaret::IsOnTouchCaret(const nsPoint& aPoint)
{
  
  if (!mVisible) {
    return false;
  }

  nsRect tcRect = GetTouchFrameRect();

  
  int32_t distance;
  if (tcRect.Contains(aPoint.x, aPoint.y)) {
    distance = 0;
  } else {
    
    
    int32_t posX = (tcRect.x + (tcRect.width / 2));
    int32_t posY = (tcRect.y + (tcRect.height / 2));
    int32_t dx = Abs(aPoint.x - posX);
    int32_t dy = Abs(aPoint.y - posY);
    distance = dx + dy;
  }
  return (distance <= TouchCaretMaxDistance());
}

nsresult
TouchCaret::NotifySelectionChanged(nsIDOMDocument* aDoc, nsISelection* aSel,
                                     int16_t aReason)
{
  
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return NS_OK;
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) {
    SetVisibility(false);
    return NS_OK;
  }

  
  
  
  
  
  if (aSel != caret->GetCaretDOMSelection()) {
    return NS_OK;
  }

  
  
  if ((aReason == nsISelectionListener::NO_REASON) ||
      (aReason & nsISelectionListener::KEYPRESS_REASON)) {
    UpdateTouchCaret(false);
  } else {
    UpdateTouchCaret(true);
  }

  return NS_OK;
}

void
TouchCaret::UpdateTouchCaret(bool aVisible)
{
  
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return;
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) {
    SetVisibility(false);
    return;
  }

  
  bool caretVisible = false;
  caret->GetCaretVisible(&caretVisible);
  if (!caretVisible) {
    SetVisibility(false);
    return;
  }

  
  nsISelection* caretSelection = caret->GetCaretDOMSelection();
  nsRect focusRect;
  nsIFrame* focusFrame = caret->GetGeometry(caretSelection, &focusRect);
  if (!focusFrame || focusRect.IsEmpty()) {
    SetVisibility(false);
    return;
  }

  
  nsPoint pos = nsPoint(focusRect.x + (focusRect.width / 2),
                        focusRect.y + focusRect.height);

  
  nsIFrame* canvasFrame = GetCanvasFrame();
  if (!canvasFrame) {
    return;
  }
  nsLayoutUtils::TransformPoint(focusFrame, canvasFrame, pos);

  
  nsIFrame* closestScrollFrame =
    nsLayoutUtils::GetClosestFrameOfType(focusFrame, nsGkAtoms::scrollFrame);
  while (closestScrollFrame) {
    nsIScrollableFrame* sf = do_QueryFrame(closestScrollFrame);
    nsRect visualRect = sf->GetScrollPortRect();
    
    nsLayoutUtils::TransformRect(closestScrollFrame, canvasFrame, visualRect);
    pos = visualRect.ClampPoint(pos);

    
    closestScrollFrame =
      nsLayoutUtils::GetClosestFrameOfType(closestScrollFrame->GetParent(),
                                           nsGkAtoms::scrollFrame);
  }

  SetTouchFramePos(pos);
  SetVisibility(aVisible);
}

void
TouchCaret::DisableTouchCaretCallback(nsITimer* aTimer, void* aTouchCaret)
{
  nsRefPtr<TouchCaret> self = static_cast<TouchCaret*>(aTouchCaret);
  NS_PRECONDITION(aTimer == self->mTouchCaretExpirationTimer,
                  "Unexpected timer");

  self->SetVisibility(false);
}

void
TouchCaret::LaunchExpirationTimer()
{
  if (TouchCaretExpirationTime() > 0) {
    if (!mTouchCaretExpirationTimer) {
      mTouchCaretExpirationTimer = do_CreateInstance("@mozilla.org/timer;1");
    }

    if (mTouchCaretExpirationTimer) {
      mTouchCaretExpirationTimer->Cancel();
      mTouchCaretExpirationTimer->InitWithFuncCallback(DisableTouchCaretCallback,
                                                       this,
                                                       TouchCaretExpirationTime(),
                                                       nsITimer::TYPE_ONE_SHOT);
    }
  }
}

void
TouchCaret::CancelExpirationTimer()
{
  if (mTouchCaretExpirationTimer) {
    mTouchCaretExpirationTimer->Cancel();
  }
}

nsEventStatus
TouchCaret::HandleEvent(WidgetEvent* aEvent)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nsEventStatus_eIgnore;
  }

  mozilla::dom::Element* touchCaretElement = presShell->GetTouchCaretElement();
  if (!touchCaretElement) {
    return nsEventStatus_eIgnore;
  }

  nsEventStatus status = nsEventStatus_eIgnore;

  switch (aEvent->message) {
    case NS_TOUCH_START:
    case NS_TOUCH_ENTER:
      status = HandleTouchDownEvent(aEvent->AsTouchEvent());
      break;
    case NS_MOUSE_BUTTON_DOWN:
      status = HandleMouseDownEvent(aEvent->AsMouseEvent());
      break;
    case NS_TOUCH_END:
      status = HandleTouchUpEvent(aEvent->AsTouchEvent());
      break;
   case NS_MOUSE_BUTTON_UP:
      status = HandleMouseUpEvent(aEvent->AsMouseEvent());
      break;
    case NS_TOUCH_MOVE:
      status = HandleTouchMoveEvent(aEvent->AsTouchEvent());
      break;
    case NS_MOUSE_MOVE:
      status = HandleMouseMoveEvent(aEvent->AsMouseEvent());
      break;
    case NS_TOUCH_CANCEL:
      mTouchesId.Clear();
      SetState(TOUCHCARET_NONE);
      LaunchExpirationTimer();
      break;
    case NS_KEY_UP:
    case NS_KEY_DOWN:
    case NS_KEY_PRESS:
    case NS_WHEEL_EVENT_START:
      
      SetVisibility(false);
      break;
    default:
      break;
  }

  return status;
}

nsPoint
TouchCaret::GetEventPosition(WidgetTouchEvent* aEvent, int32_t aIdentifier)
{
  for (size_t i = 0; i < aEvent->touches.Length(); i++) {
    if (aEvent->touches[i]->mIdentifier == aIdentifier) {
      
      nsIFrame* canvasFrame = GetCanvasFrame();
      nsIntPoint touchIntPoint = aEvent->touches[i]->mRefPoint;
      return nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                          touchIntPoint,
                                                          canvasFrame);
    }
  }
  return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
}

nsPoint
TouchCaret::GetEventPosition(WidgetMouseEvent* aEvent)
{
  
  nsIFrame* canvasFrame = GetCanvasFrame();
  nsIntPoint mouseIntPoint =
    LayoutDeviceIntPoint::ToUntyped(aEvent->AsGUIEvent()->refPoint);
  return nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                      mouseIntPoint,
                                                      canvasFrame);
}

nsEventStatus
TouchCaret::HandleMouseMoveEvent(WidgetMouseEvent* aEvent)
{
  TOUCHCARET_LOG("%p got a mouse-move in state %d\n", this, mState);
  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      break;

    case TOUCHCARET_MOUSEDRAG_ACTIVE:
      {
        nsPoint movePoint = GetEventPosition(aEvent);
        movePoint.y += mCaretCenterToDownPointOffsetY;
        nsRect contentBoundary = GetContentBoundary();
        movePoint = contentBoundary.ClampPoint(movePoint);

        MoveCaret(movePoint);
        status = nsEventStatus_eConsumeNoDefault;
      }
      break;

    case TOUCHCARET_TOUCHDRAG_ACTIVE:
    case TOUCHCARET_TOUCHDRAG_INACTIVE:
      
      status = nsEventStatus_eConsumeNoDefault;
      break;
  }

  return status;
}

nsEventStatus
TouchCaret::HandleTouchMoveEvent(WidgetTouchEvent* aEvent)
{
  TOUCHCARET_LOG("%p got a touch-move in state %d\n", this, mState);
  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      break;

    case TOUCHCARET_MOUSEDRAG_ACTIVE:
      
      status = nsEventStatus_eConsumeNoDefault;
      break;

    case TOUCHCARET_TOUCHDRAG_ACTIVE:
      {
        nsPoint movePoint = GetEventPosition(aEvent, mActiveTouchId);
        movePoint.y += mCaretCenterToDownPointOffsetY;
        nsRect contentBoundary = GetContentBoundary();
        movePoint = contentBoundary.ClampPoint(movePoint);

        MoveCaret(movePoint);
        status = nsEventStatus_eConsumeNoDefault;
      }
      break;

    case TOUCHCARET_TOUCHDRAG_INACTIVE:
      
      status = nsEventStatus_eConsumeNoDefault;
      break;
  }

  return status;
}

nsEventStatus
TouchCaret::HandleMouseUpEvent(WidgetMouseEvent* aEvent)
{
  TOUCHCARET_LOG("%p got a mouse-up in state %d\n", this, mState);
  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      break;

    case TOUCHCARET_MOUSEDRAG_ACTIVE:
      if (aEvent->button == WidgetMouseEvent::eLeftButton) {
        LaunchExpirationTimer();
        SetState(TOUCHCARET_NONE);
        status = nsEventStatus_eConsumeNoDefault;
      }
      break;

    case TOUCHCARET_TOUCHDRAG_ACTIVE:
    case TOUCHCARET_TOUCHDRAG_INACTIVE:
      
      status = nsEventStatus_eConsumeNoDefault;
      break;
  }

  return status;
}

nsEventStatus
TouchCaret::HandleTouchUpEvent(WidgetTouchEvent* aEvent)
{
  TOUCHCARET_LOG("%p got a touch-end in state %d\n", this, mState);
  
  if (mState == TOUCHCARET_TOUCHDRAG_ACTIVE ||
      mState == TOUCHCARET_TOUCHDRAG_INACTIVE) {
    for (size_t i = 0; i < aEvent->touches.Length(); i++) {
      nsTArray<int32_t>::index_type index =
        mTouchesId.IndexOf(aEvent->touches[i]->mIdentifier);
      MOZ_ASSERT(index != nsTArray<int32_t>::NoIndex);
      mTouchesId.RemoveElementAt(index);
    }
  }

  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      break;

    case TOUCHCARET_MOUSEDRAG_ACTIVE:
      
      status = nsEventStatus_eConsumeNoDefault;
      break;

    case TOUCHCARET_TOUCHDRAG_ACTIVE:
      if (mTouchesId.Length() == 0) {
        
        SetState(TOUCHCARET_NONE);
        LaunchExpirationTimer();
      } else {
        
        if (aEvent->touches[0]->mIdentifier == mActiveTouchId) {
          
          SetState(TOUCHCARET_TOUCHDRAG_INACTIVE);
          LaunchExpirationTimer();
        } else {
          
          
        }
      }
      status = nsEventStatus_eConsumeNoDefault;
      break;

    case TOUCHCARET_TOUCHDRAG_INACTIVE:
      if (mTouchesId.Length() == 0) {
        
        SetState(TOUCHCARET_NONE);
      }
      status = nsEventStatus_eConsumeNoDefault;
      break;
  }

  return status;
}

nsEventStatus
TouchCaret::HandleMouseDownEvent(WidgetMouseEvent* aEvent)
{
  TOUCHCARET_LOG("%p got a mouse-down in state %d\n", this, mState);
  if (!GetVisibility()) {
    
    return nsEventStatus_eIgnore;
  }

  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      if (aEvent->button == WidgetMouseEvent::eLeftButton) {
        nsPoint point = GetEventPosition(aEvent);
        if (IsOnTouchCaret(point)) {
          
          mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - point.y;
          
          SetState(TOUCHCARET_MOUSEDRAG_ACTIVE);
          CancelExpirationTimer();
          status = nsEventStatus_eConsumeNoDefault;
        } else {
          
          SetVisibility(false);
          status = nsEventStatus_eIgnore;
        }
      } else {
        
        SetVisibility(false);
        status = nsEventStatus_eIgnore;
      }
      break;

    case TOUCHCARET_MOUSEDRAG_ACTIVE:
      SetVisibility(false);
      SetState(TOUCHCARET_NONE);
      break;

    case TOUCHCARET_TOUCHDRAG_ACTIVE:
    case TOUCHCARET_TOUCHDRAG_INACTIVE:
      
      status = nsEventStatus_eConsumeNoDefault;
      break;
  }

  return status;
}

nsEventStatus
TouchCaret::HandleTouchDownEvent(WidgetTouchEvent* aEvent)
{
  TOUCHCARET_LOG("%p got a touch-start in state %d\n", this, mState);

  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      if (!GetVisibility()) {
        
        status = nsEventStatus_eIgnore;
      } else {
        nsPoint point = GetEventPosition(aEvent, 0);
        if (IsOnTouchCaret(point)) {
          
          mActiveTouchId = aEvent->touches[0]->mIdentifier;
          
          mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - point.y;
          
          SetState(TOUCHCARET_TOUCHDRAG_ACTIVE);
          CancelExpirationTimer();
          status = nsEventStatus_eConsumeNoDefault;
        } else {
          
          SetVisibility(false);
          status = nsEventStatus_eIgnore;
        }
      }
      break;

    case TOUCHCARET_MOUSEDRAG_ACTIVE:
    case TOUCHCARET_TOUCHDRAG_ACTIVE:
    case TOUCHCARET_TOUCHDRAG_INACTIVE:
      
      status = nsEventStatus_eConsumeNoDefault;
      break;
  }

  
  if (mState == TOUCHCARET_TOUCHDRAG_ACTIVE ||
      mState == TOUCHCARET_TOUCHDRAG_INACTIVE) {
    mTouchesId.Clear();
    for (size_t i = 0; i < aEvent->touches.Length(); i++) {
      mTouchesId.AppendElement(aEvent->touches[i]->mIdentifier);
    }
  }

  return status;
}

void
TouchCaret::SetState(TouchCaretState aState)
{
  TOUCHCARET_LOG("%p state changed from %d to %d\n", this, mState, aState);
  if (mState == TOUCHCARET_NONE) {
    MOZ_ASSERT(aState != TOUCHCARET_TOUCHDRAG_INACTIVE,
               "mState: NONE => TOUCHDRAG_INACTIVE isn't allowed!");
  }

  if (mState == TOUCHCARET_TOUCHDRAG_ACTIVE) {
    MOZ_ASSERT(aState != TOUCHCARET_MOUSEDRAG_ACTIVE,
               "mState: TOUCHDRAG_ACTIVE => MOUSEDRAG_ACTIVE isn't allowed!");
  }

  if (mState == TOUCHCARET_MOUSEDRAG_ACTIVE) {
    MOZ_ASSERT(aState == TOUCHCARET_MOUSEDRAG_ACTIVE ||
               aState == TOUCHCARET_NONE,
               "MOUSEDRAG_ACTIVE allowed next state: NONE!");
  }

  if (mState == TOUCHCARET_TOUCHDRAG_INACTIVE) {
    MOZ_ASSERT(aState == TOUCHCARET_TOUCHDRAG_INACTIVE ||
               aState == TOUCHCARET_NONE,
               "TOUCHDRAG_INACTIVE allowed next state: NONE!");
  }

  mState = aState;

  if (mState == TOUCHCARET_NONE) {
    mActiveTouchId = -1;
    mCaretCenterToDownPointOffsetY = 0;
  }
}
