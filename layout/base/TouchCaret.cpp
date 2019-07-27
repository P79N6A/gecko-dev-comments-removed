





#include "mozilla/Logging.h"
#include "TouchCaret.h"

#include <algorithm>

#include "nsBlockFrame.h"
#include "nsCanvasFrame.h"
#include "nsCaret.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsDOMTokenList.h"
#include "nsFrameSelection.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPresShell.h"
#include "nsIScrollableFrame.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsPresContext.h"
#include "nsQueryContentEventResult.h"
#include "nsView.h"
#include "mozilla/dom/SelectionStateChangedEvent.h"
#include "mozilla/dom/CustomEvent.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

static PRLogModuleInfo* gTouchCaretLog;
static const char* kTouchCaretLogModuleName = "TouchCaret";



#define TOUCHCARET_LOG(message, ...)                                           \
  MOZ_LOG(gTouchCaretLog, LogLevel::Debug,                                         \
         ("TouchCaret (%p): %s:%d : " message "\n", this, __FUNCTION__,        \
          __LINE__, ##__VA_ARGS__));

#define TOUCHCARET_LOG_STATIC(message, ...)                                    \
  MOZ_LOG(gTouchCaretLog, LogLevel::Debug,                                         \
         ("TouchCaret: %s:%d : " message "\n", __FUNCTION__, __LINE__,         \
          ##__VA_ARGS__));




static const int32_t kBoundaryAppUnits = 61;

NS_IMPL_ISUPPORTS(TouchCaret,
                  nsISelectionListener,
                  nsIScrollObserver,
                  nsISupportsWeakReference)

 int32_t TouchCaret::sTouchCaretInflateSize = 0;
 int32_t TouchCaret::sTouchCaretExpirationTime = 0;
 bool TouchCaret::sCaretManagesAndroidActionbar = false;
 bool TouchCaret::sTouchcaretExtendedvisibility = false;

 uint32_t TouchCaret::sActionBarViewCount = 0;

TouchCaret::TouchCaret(nsIPresShell* aPresShell)
  : mState(TOUCHCARET_NONE),
    mActiveTouchId(-1),
    mCaretCenterToDownPointOffsetY(0),
    mInAsyncPanZoomGesture(false),
    mVisible(false),
    mIsValidTap(false),
    mActionBarViewID(0)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!gTouchCaretLog) {
    gTouchCaretLog = PR_NewLogModule(kTouchCaretLogModuleName);
  }

  TOUCHCARET_LOG("Constructor, PresShell=%p", aPresShell);

  static bool addedTouchCaretPref = false;
  if (!addedTouchCaretPref) {
    Preferences::AddIntVarCache(&sTouchCaretInflateSize,
                                "touchcaret.inflatesize.threshold");
    Preferences::AddIntVarCache(&sTouchCaretExpirationTime,
                                "touchcaret.expiration.time");
    Preferences::AddBoolVarCache(&sCaretManagesAndroidActionbar,
                                 "caret.manages-android-actionbar");
    Preferences::AddBoolVarCache(&sTouchcaretExtendedvisibility,
                                 "touchcaret.extendedvisibility");
    addedTouchCaretPref = true;
  }

  
  mPresShell = do_GetWeakReference(aPresShell);
  MOZ_ASSERT(mPresShell, "Hey, pres shell should support weak refs");
}

void
TouchCaret::Init()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return;
  }

  nsPresContext* presContext = presShell->GetPresContext();
  MOZ_ASSERT(presContext, "PresContext should be given in PresShell::Init()");

  nsIDocShell* docShell = presContext->GetDocShell();
  if (!docShell) {
    return;
  }

  docShell->AddWeakScrollObserver(this);
  mDocShell = static_cast<nsDocShell*>(docShell);
}

void
TouchCaret::Terminate()
{
  nsRefPtr<nsDocShell> docShell(mDocShell.get());
  if (docShell) {
    docShell->RemoveWeakScrollObserver(this);
  }

  if (mScrollEndDetectorTimer) {
    mScrollEndDetectorTimer->Cancel();
    mScrollEndDetectorTimer = nullptr;
  }

  mDocShell = WeakPtr<nsDocShell>();
  mPresShell = nullptr;
}

TouchCaret::~TouchCaret()
{
  TOUCHCARET_LOG("Destructor");
  MOZ_ASSERT(NS_IsMainThread());

  if (mTouchCaretExpirationTimer) {
    mTouchCaretExpirationTimer->Cancel();
    mTouchCaretExpirationTimer = nullptr;
  }
}

nsIFrame*
TouchCaret::GetCaretFocusFrame(nsRect* aOutRect)
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nullptr;
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) {
    return nullptr;
  }

  nsRect rect;
  nsIFrame* frame = caret->GetGeometry(&rect);

  if (aOutRect) {
    *aOutRect = rect;
  }

  return frame;
}

nsCanvasFrame*
TouchCaret::GetCanvasFrame()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nullptr;
  }
  return presShell->GetCanvasFrame();
}

nsIFrame*
TouchCaret::GetRootFrame()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nullptr;
  }
  return presShell->GetRootFrame();
}

void
TouchCaret::SetVisibility(bool aVisible)
{
  if (mVisible == aVisible) {
    TOUCHCARET_LOG("Set visibility %s, same as the old one",
                   (aVisible ? "shown" : "hidden"));
    return;
  }

  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return;
  }

  mozilla::dom::Element* touchCaretElement = presShell->GetTouchCaretElement();
  if (!touchCaretElement) {
    return;
  }

  mVisible = aVisible;

  
  ErrorResult err;
  touchCaretElement->ClassList()->Toggle(NS_LITERAL_STRING("hidden"),
                                         dom::Optional<bool>(!mVisible),
                                         err);
  TOUCHCARET_LOG("Set visibility %s", (mVisible ? "shown" : "hidden"));

  
  mVisible ? LaunchExpirationTimer() : CancelExpirationTimer();

  
  
  if (!mVisible && sCaretManagesAndroidActionbar) {
    UpdateAndroidActionBarVisibility(false, mActionBarViewID);
  }
}








void
TouchCaret::UpdateAndroidActionBarVisibility(bool aVisibility, uint32_t& aViewID)
{
  
  if (aVisibility) {
    
    aViewID = ++sActionBarViewCount;
  }

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    nsString topic = (aVisibility) ?
      NS_LITERAL_STRING("ActionBar:OpenNew") : NS_LITERAL_STRING("ActionBar:Close");
    nsAutoString viewCount;
    viewCount.AppendInt(aViewID);
    os->NotifyObservers(nullptr, NS_ConvertUTF16toUTF8(topic).get(), viewCount.get());
  }
}

nsRect
TouchCaret::GetTouchFrameRect()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return nsRect();
  }

  dom::Element* touchCaretElement = presShell->GetTouchCaretElement();
  nsIFrame* canvasFrame = GetCanvasFrame();
  return nsLayoutUtils::GetRectRelativeToFrame(touchCaretElement, canvasFrame);
}

nsRect
TouchCaret::GetContentBoundary()
{
  nsIFrame* focusFrame = GetCaretFocusFrame();
  nsIFrame* canvasFrame = GetCanvasFrame();
  if (!focusFrame || !canvasFrame) {
    return nsRect();
  }

  
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
  nsRect caretRect;
  nsIFrame* focusFrame = GetCaretFocusFrame(&caretRect);
  nsIFrame* canvasFrame = GetCanvasFrame();

  nsLayoutUtils::TransformRect(focusFrame, canvasFrame, caretRect);

  return (caretRect.y + caretRect.height / 2);
}

void
TouchCaret::SetTouchFramePos(const nsRect& aCaretRect)
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
  int32_t x = presContext->AppUnitsToIntCSSPixels(aCaretRect.Center().x);
  int32_t y = presContext->AppUnitsToIntCSSPixels(aCaretRect.y);
  int32_t padding = presContext->AppUnitsToIntCSSPixels(aCaretRect.height);

  nsAutoString styleStr;
  styleStr.AppendLiteral("left: ");
  styleStr.AppendInt(x);
  styleStr.AppendLiteral("px; top: ");
  styleStr.AppendInt(y);
  styleStr.AppendLiteral("px; padding-top: ");
  styleStr.AppendInt(padding);
  styleStr.AppendLiteral("px;");

  TOUCHCARET_LOG("Set style: %s", NS_ConvertUTF16toUTF8(styleStr).get());

  touchCaretElement->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
                             styleStr, true);
}

void
TouchCaret::MoveCaret(const nsPoint& movePoint)
{
  nsIFrame* focusFrame = GetCaretFocusFrame();
  nsIFrame* canvasFrame = GetCanvasFrame();
  if (!focusFrame && !canvasFrame) {
    return;
  }
  nsIFrame* scrollable =
    nsLayoutUtils::GetClosestFrameOfType(focusFrame, nsGkAtoms::scrollFrame);

  
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
                  offsets.associate);

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
  return mVisible && nsLayoutUtils::ContainsPoint(GetTouchFrameRect(), aPoint,
                                                  TouchCaretInflateSize());
}

nsresult
TouchCaret::NotifySelectionChanged(nsIDOMDocument* aDoc, nsISelection* aSel,
                                   int16_t aReason)
{
  TOUCHCARET_LOG("aSel (%p), Reason=%d", aSel, aReason);

  
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return NS_OK;
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) {
    SetVisibility(false);
    return NS_OK;
  }

  
  
  
  
  
  if (aSel != caret->GetSelection()) {
    TOUCHCARET_LOG("Return for selection mismatch!");
    return NS_OK;
  }

  
  
  
  if (aReason & nsISelectionListener::KEYPRESS_REASON ||
      aReason & nsISelectionListener::COLLAPSETOSTART_REASON ||
      aReason & nsISelectionListener::COLLAPSETOEND_REASON) {
    TOUCHCARET_LOG("KEYPRESS_REASON");
    SetVisibility(false);
  } else {
    SyncVisibilityWithCaret();

    
    if (mVisible && sCaretManagesAndroidActionbar) {
      
      if (aReason & nsISelectionListener::MOUSEUP_REASON) {
        UpdateAndroidActionBarVisibility(true, mActionBarViewID);
      } else {
        
        
        
        bool isCollapsed;
        if (NS_SUCCEEDED(aSel->GetIsCollapsed(&isCollapsed)) && isCollapsed) {
          nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
          if (os) {
            os->NotifyObservers(nullptr, "ActionBar:UpdateState", nullptr);
          }
        }
      }
    }
  }

  return NS_OK;
}





void
TouchCaret::AsyncPanZoomStarted()
{
  if (mVisible) {
    if (sTouchcaretExtendedvisibility) {
      mInAsyncPanZoomGesture = true;
    }
  }
}

void
TouchCaret::AsyncPanZoomStopped()
{
  if (mInAsyncPanZoomGesture) {
    mInAsyncPanZoomGesture = false;
    UpdatePosition();
  }
}





void
TouchCaret::ScrollPositionChanged()
{
  if (mVisible) {
    if (sTouchcaretExtendedvisibility) {
      
      LaunchScrollEndDetector();
    }
  }
}

void
TouchCaret::LaunchScrollEndDetector()
{
  if (!mScrollEndDetectorTimer) {
    mScrollEndDetectorTimer = do_CreateInstance("@mozilla.org/timer;1");
  }
  MOZ_ASSERT(mScrollEndDetectorTimer);

  mScrollEndDetectorTimer->InitWithFuncCallback(FireScrollEnd,
                                                this,
                                                sScrollEndTimerDelay,
                                                nsITimer::TYPE_ONE_SHOT);
}

void
TouchCaret::CancelScrollEndDetector()
{
  if (mScrollEndDetectorTimer) {
    mScrollEndDetectorTimer->Cancel();
  }
}


void
TouchCaret::FireScrollEnd(nsITimer* aTimer, void* aTouchCaret)
{
  nsRefPtr<TouchCaret> self = static_cast<TouchCaret*>(aTouchCaret);
  NS_PRECONDITION(aTimer == self->mScrollEndDetectorTimer,
                  "Unexpected timer");
  self->UpdatePosition();
}

void
TouchCaret::SyncVisibilityWithCaret()
{
  TOUCHCARET_LOG("SyncVisibilityWithCaret");

  if (!IsDisplayable()) {
    SetVisibility(false);
    return;
  }

  SetVisibility(true);
  if (mVisible) {
    UpdatePosition();
  }
}

void
TouchCaret::UpdatePositionIfNeeded()
{
  TOUCHCARET_LOG("UpdatePositionIfNeeded");

  if (!IsDisplayable()) {
    SetVisibility(false);
    return;
  }

  if (mVisible) {
    UpdatePosition();
  }
}

bool
TouchCaret::IsDisplayable()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    TOUCHCARET_LOG("PresShell is nullptr!");
    return false;
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) {
    TOUCHCARET_LOG("Caret is nullptr!");
    return false;
  }

  nsIFrame* canvasFrame = GetCanvasFrame();
  if (!canvasFrame) {
    TOUCHCARET_LOG("No canvas frame!");
    return false;
  }

  nsIFrame* rootFrame = GetRootFrame();
  if (!rootFrame) {
    TOUCHCARET_LOG("No root frame!");
    return false;
  }

  dom::Element* touchCaretElement = presShell->GetTouchCaretElement();
  if (!touchCaretElement) {
    TOUCHCARET_LOG("No touch caret frame element!");
    return false;
  }

  if (presShell->IsPaintingSuppressed()) {
    TOUCHCARET_LOG("PresShell is suppressing painting!");
    return false;
  }

  if (!caret->IsVisible()) {
    TOUCHCARET_LOG("Caret is not visible!");
    return false;
  }

  nsRect focusRect;
  nsIFrame* focusFrame = caret->GetGeometry(&focusRect);
  if (!focusFrame) {
    TOUCHCARET_LOG("Focus frame is not valid!");
    return false;
  }

  dom::Element* editingHost = focusFrame->GetContent()->GetEditingHost();
  if (!editingHost) {
    TOUCHCARET_LOG("Cannot get editing host!");
    return false;
  }

  
  if (sTouchcaretExtendedvisibility) {
    return true;
  }

  if (focusRect.IsEmpty()) {
    TOUCHCARET_LOG("Focus rect is empty!");
    return false;
  }

  if (!nsContentUtils::HasNonEmptyTextContent(
         editingHost, nsContentUtils::eRecurseIntoChildren)) {
    TOUCHCARET_LOG("The content is empty!");
    return false;
  }

  if (mState != TOUCHCARET_TOUCHDRAG_ACTIVE &&
        !nsLayoutUtils::IsRectVisibleInScrollFrames(focusFrame, focusRect)) {
    TOUCHCARET_LOG("Caret does not show in the scrollable frame!");
    return false;
  }

  TOUCHCARET_LOG("Touch caret is displayable!");
  return true;
}

void
TouchCaret::UpdatePosition()
{
  MOZ_ASSERT(mVisible);

  nsRect rect = GetTouchCaretRect();
  rect = ClampRectToScrollFrame(rect);
  SetTouchFramePos(rect);
}

nsRect
TouchCaret::GetTouchCaretRect()
{
  nsRect focusRect;
  nsIFrame* focusFrame = GetCaretFocusFrame(&focusRect);
  nsIFrame* rootFrame = GetRootFrame();
  
  nsLayoutUtils::TransformRect(focusFrame, rootFrame, focusRect);

  return focusRect;
}

nsRect
TouchCaret::ClampRectToScrollFrame(const nsRect& aRect)
{
  nsRect rect = aRect;
  nsIFrame* focusFrame = GetCaretFocusFrame();
  nsIFrame* rootFrame = GetRootFrame();

  
  nsIFrame* closestScrollFrame =
    nsLayoutUtils::GetClosestFrameOfType(focusFrame, nsGkAtoms::scrollFrame);

  while (closestScrollFrame) {
    nsIScrollableFrame* sf = do_QueryFrame(closestScrollFrame);
    nsRect visualRect = sf->GetScrollPortRect();

    
    nsLayoutUtils::TransformRect(closestScrollFrame, rootFrame, visualRect);
    rect = rect.Intersect(visualRect);

    
    closestScrollFrame =
      nsLayoutUtils::GetClosestFrameOfType(closestScrollFrame->GetParent(),
                                           nsGkAtoms::scrollFrame);
  }

  return rect;
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

void
TouchCaret::SetSelectionDragState(bool aState)
{
  nsIFrame* caretFocusFrame = GetCaretFocusFrame();
  if (!caretFocusFrame) {
    return;
  }

  nsRefPtr<nsFrameSelection> fs = caretFocusFrame->GetFrameSelection();
  fs->SetDragState(aState);
}

nsEventStatus
TouchCaret::HandleEvent(WidgetEvent* aEvent)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!IsDisplayable()) {
    return nsEventStatus_eIgnore;
  }

  nsEventStatus status = nsEventStatus_eIgnore;

  switch (aEvent->message) {
    case NS_TOUCH_START:
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
    case NS_WHEEL_WHEEL:
    case NS_WHEEL_START:
    case NS_WHEEL_STOP:
      
      TOUCHCARET_LOG("Receive key/wheel event %d", aEvent->message);
      SetVisibility(false);
      break;
    case NS_MOUSE_MOZLONGTAP:
      if (mState == TOUCHCARET_TOUCHDRAG_ACTIVE) {
        
        status = nsEventStatus_eConsumeNoDefault;
      }
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
      LayoutDeviceIntPoint touchIntPoint = aEvent->touches[i]->mRefPoint;
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
  LayoutDeviceIntPoint mouseIntPoint = aEvent->AsGUIEvent()->refPoint;
  return nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                      mouseIntPoint,
                                                      canvasFrame);
}

nsEventStatus
TouchCaret::HandleMouseMoveEvent(WidgetMouseEvent* aEvent)
{
  TOUCHCARET_LOG("Got a mouse-move in state %d", mState);
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
        mIsValidTap = false;
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
  TOUCHCARET_LOG("Got a touch-move in state %d", mState);
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
        mIsValidTap = false;
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
  TOUCHCARET_LOG("Got a mouse-up in state %d", mState);
  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      break;

    case TOUCHCARET_MOUSEDRAG_ACTIVE:
      if (aEvent->button == WidgetMouseEvent::eLeftButton) {
        SetSelectionDragState(false);
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
  TOUCHCARET_LOG("Got a touch-end in state %d", mState);
  
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
        SetSelectionDragState(false);
        
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
  TOUCHCARET_LOG("Got a mouse-down in state %d", mState);
  if (!GetVisibility()) {
    
    return nsEventStatus_eIgnore;
  }

  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      if (aEvent->button == WidgetMouseEvent::eLeftButton) {
        nsPoint point = GetEventPosition(aEvent);
        if (IsOnTouchCaret(point)) {
          SetSelectionDragState(true);
          
          mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - point.y;
          
          SetState(TOUCHCARET_MOUSEDRAG_ACTIVE);
          CancelExpirationTimer();
          status = nsEventStatus_eConsumeNoDefault;
        } else {
          
          
          if (sTouchcaretExtendedvisibility) {
            UpdatePositionIfNeeded();
            break;
          }
          
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
  TOUCHCARET_LOG("Got a touch-start in state %d", mState);

  nsEventStatus status = nsEventStatus_eIgnore;

  switch (mState) {
    case TOUCHCARET_NONE:
      if (!GetVisibility()) {
        
        status = nsEventStatus_eIgnore;
      } else {
        
        
        for (size_t i = 0; i < aEvent->touches.Length(); ++i) {
          int32_t touchId = aEvent->touches[i]->Identifier();
          nsPoint point = GetEventPosition(aEvent, touchId);
          if (IsOnTouchCaret(point)) {
            SetSelectionDragState(true);
            
            mActiveTouchId = touchId;
            
            mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - point.y;
            
            SetState(TOUCHCARET_TOUCHDRAG_ACTIVE);
            CancelExpirationTimer();
            status = nsEventStatus_eConsumeNoDefault;
            break;
          }
        }
        
        
        if (mActiveTouchId == -1) {
          
          if (sTouchcaretExtendedvisibility) {
            
            UpdatePositionIfNeeded();
          } else {
            SetVisibility(false);
            status = nsEventStatus_eIgnore;
          }
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
TouchCaret::DispatchTapEvent()
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (!presShell) {
    return;
  }

  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) {
    return;
  }

  dom::Selection* sel = static_cast<dom::Selection*>(caret->GetSelection());
  if (!sel) {
    return;
  }

  nsIDocument* doc = presShell->GetDocument();

  MOZ_ASSERT(doc);

  dom::SelectionStateChangedEventInit init;
  init.mBubbles = true;

  
  presShell->FlushPendingNotifications(Flush_Layout);
  nsRect rect = nsContentUtils::GetSelectionBoundingRect(sel);
  nsRefPtr<dom::DOMRect>domRect = new dom::DOMRect(ToSupports(doc));

  domRect->SetLayoutRect(rect);
  init.mBoundingClientRect = domRect;
  init.mVisible = false;

  sel->Stringify(init.mSelectedText);

  dom::Sequence<dom::SelectionState> state;
  state.AppendElement(dom::SelectionState::Taponcaret, fallible);
  init.mStates = state;

  nsRefPtr<dom::SelectionStateChangedEvent> event =
    dom::SelectionStateChangedEvent::Constructor(doc, NS_LITERAL_STRING("mozselectionstatechanged"), init);

  event->SetTrusted(true);
  event->GetInternalNSEvent()->mFlags.mOnlyChromeDispatch = true;
  bool ret;
  doc->DispatchEvent(event, &ret);
}

void
TouchCaret::SetState(TouchCaretState aState)
{
  TOUCHCARET_LOG("state changed from %d to %d", mState, aState);
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
    if (mIsValidTap) {
      DispatchTapEvent();
      mIsValidTap = false;
    }
  } else if (mState == TOUCHCARET_TOUCHDRAG_ACTIVE ||
             mState == TOUCHCARET_MOUSEDRAG_ACTIVE) {
    mIsValidTap = true;
  }
}
