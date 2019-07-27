





#include "AccessibleCaretEventHub.h"

#include "AccessibleCaretLogger.h"
#include "AccessibleCaretManager.h"
#include "Layers.h"
#include "gfxPrefs.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"
#include "nsDocShell.h"
#include "nsFocusManager.h"
#include "nsFrameSelection.h"
#include "nsITimer.h"
#include "nsPresContext.h"

namespace mozilla {

#undef AC_LOG
#define AC_LOG(message, ...)                                                   \
  AC_LOG_BASE("AccessibleCaretEventHub (%p): " message, this, ##__VA_ARGS__);

#undef AC_LOGV
#define AC_LOGV(message, ...)                                                  \
  AC_LOGV_BASE("AccessibleCaretEventHub (%p): " message, this, ##__VA_ARGS__);

NS_IMPL_ISUPPORTS(AccessibleCaretEventHub, nsIReflowObserver, nsIScrollObserver,
                  nsISelectionListener, nsISupportsWeakReference);




class AccessibleCaretEventHub::NoActionState
  : public AccessibleCaretEventHub::State
{
public:
  NS_IMPL_STATE_UTILITIES(NoActionState)

  virtual nsEventStatus OnPress(AccessibleCaretEventHub* aContext,
                                const nsPoint& aPoint,
                                int32_t aTouchId) override
  {
    nsEventStatus rv = nsEventStatus_eIgnore;

    if (NS_SUCCEEDED(aContext->mManager->PressCaret(aPoint))) {
      aContext->SetState(aContext->PressCaretState());
      rv = nsEventStatus_eConsumeNoDefault;
    } else {
      aContext->SetState(aContext->PressNoCaretState());
    }

    aContext->mPressPoint = aPoint;
    aContext->mActiveTouchId = aTouchId;

    return rv;
  }

  virtual void OnScrollStart(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnScrollStart();
    aContext->SetState(aContext->ScrollState());
  }

  virtual void OnScrolling(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnScrolling();
  }

  virtual void OnScrollPositionChanged(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnScrollPositionChanged();
  }

  virtual void OnSelectionChanged(AccessibleCaretEventHub* aContext,
                                  nsIDOMDocument* aDoc, nsISelection* aSel,
                                  int16_t aReason) override
  {
    aContext->mManager->OnSelectionChanged(aDoc, aSel, aReason);
  }

  virtual void OnBlur(AccessibleCaretEventHub* aContext,
                      bool aIsLeavingDocument) override
  {
    aContext->mManager->OnBlur();
  }

  virtual void OnReflow(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnReflow();
  }

  virtual void Enter(AccessibleCaretEventHub* aContext) override
  {
    aContext->mPressPoint = nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
    aContext->mActiveTouchId = kInvalidTouchId;
  }
};




class AccessibleCaretEventHub::PressCaretState
  : public AccessibleCaretEventHub::State
{
public:
  NS_IMPL_STATE_UTILITIES(PressCaretState)

  virtual nsEventStatus OnMove(AccessibleCaretEventHub* aContext,
                               const nsPoint& aPoint) override
  {
    if (aContext->MoveDistanceIsLarge(aPoint)) {
      if (NS_SUCCEEDED(aContext->mManager->DragCaret(aPoint))) {
        aContext->SetState(aContext->DragCaretState());
      }
    }

    return nsEventStatus_eConsumeNoDefault;
  }

  virtual nsEventStatus OnRelease(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->ReleaseCaret();
    aContext->mManager->TapCaret(aContext->mPressPoint);
    aContext->SetState(aContext->NoActionState());

    return nsEventStatus_eConsumeNoDefault;
  }

  virtual nsEventStatus OnLongTap(AccessibleCaretEventHub* aContext,
                                  const nsPoint& aPoint) override
  {
    return nsEventStatus_eConsumeNoDefault;
  }
};




class AccessibleCaretEventHub::DragCaretState
  : public AccessibleCaretEventHub::State
{
public:
  NS_IMPL_STATE_UTILITIES(DragCaretState)

  virtual nsEventStatus OnMove(AccessibleCaretEventHub* aContext,
                               const nsPoint& aPoint) override
  {
    aContext->mManager->DragCaret(aPoint);

    return nsEventStatus_eConsumeNoDefault;
  }

  virtual nsEventStatus OnRelease(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->ReleaseCaret();
    aContext->SetState(aContext->NoActionState());

    return nsEventStatus_eConsumeNoDefault;
  }
};




class AccessibleCaretEventHub::PressNoCaretState
  : public AccessibleCaretEventHub::State
{
public:
  NS_IMPL_STATE_UTILITIES(PressNoCaretState)

  virtual nsEventStatus OnMove(AccessibleCaretEventHub* aContext,
                               const nsPoint& aPoint) override
  {
    if (aContext->MoveDistanceIsLarge(aPoint)) {
      aContext->SetState(aContext->NoActionState());
    }

    return nsEventStatus_eIgnore;
  }

  virtual nsEventStatus OnRelease(AccessibleCaretEventHub* aContext) override
  {
    aContext->SetState(aContext->NoActionState());

    return nsEventStatus_eIgnore;
  }

  virtual nsEventStatus OnLongTap(AccessibleCaretEventHub* aContext,
                                  const nsPoint& aPoint) override
  {
    aContext->SetState(aContext->LongTapState());

    return aContext->GetState()->OnLongTap(aContext, aPoint);
  }

  virtual void OnScrollStart(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnScrollStart();
    aContext->SetState(aContext->ScrollState());
  }

  virtual void OnBlur(AccessibleCaretEventHub* aContext,
                      bool aIsLeavingDocument) override
  {
    aContext->mManager->OnBlur();
    if (aIsLeavingDocument) {
      aContext->SetState(aContext->NoActionState());
    }
  }

  virtual void OnSelectionChanged(AccessibleCaretEventHub* aContext,
                                  nsIDOMDocument* aDoc, nsISelection* aSel,
                                  int16_t aReason) override
  {
    aContext->mManager->OnSelectionChanged(aDoc, aSel, aReason);
  }

  virtual void OnReflow(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnReflow();
  }

  virtual void Enter(AccessibleCaretEventHub* aContext) override
  {
    aContext->LaunchLongTapInjector();
  }

  virtual void Leave(AccessibleCaretEventHub* aContext) override
  {
    aContext->CancelLongTapInjector();
  }
};




class AccessibleCaretEventHub::ScrollState
  : public AccessibleCaretEventHub::State
{
public:
  NS_IMPL_STATE_UTILITIES(ScrollState)

  virtual void OnScrollEnd(AccessibleCaretEventHub* aContext) override
  {
    aContext->SetState(aContext->PostScrollState());
  }

  virtual void OnBlur(AccessibleCaretEventHub* aContext,
                      bool aIsLeavingDocument) override
  {
    aContext->mManager->OnBlur();
    if (aIsLeavingDocument) {
      aContext->SetState(aContext->NoActionState());
    }
  }
};





class AccessibleCaretEventHub::PostScrollState
  : public AccessibleCaretEventHub::State
{
public:
  NS_IMPL_STATE_UTILITIES(PostScrollState)

  virtual nsEventStatus OnPress(AccessibleCaretEventHub* aContext,
                                const nsPoint& aPoint,
                                int32_t aTouchId) override
  {
    aContext->mManager->OnScrollEnd();
    aContext->SetState(aContext->NoActionState());

    return aContext->GetState()->OnPress(aContext, aPoint, aTouchId);
  }

  virtual void OnScrollStart(AccessibleCaretEventHub* aContext) override
  {
    aContext->SetState(aContext->ScrollState());
  }

  virtual void OnScrollEnd(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnScrollEnd();
    aContext->SetState(aContext->NoActionState());
  }

  virtual void OnScrolling(AccessibleCaretEventHub* aContext) override
  {
    
    aContext->LaunchScrollEndInjector();
  }

  virtual void OnBlur(AccessibleCaretEventHub* aContext,
                      bool aIsLeavingDocument) override
  {
    aContext->mManager->OnBlur();
    if (aIsLeavingDocument) {
      aContext->SetState(aContext->NoActionState());
    }
  }

  virtual void Enter(AccessibleCaretEventHub* aContext) override
  {
    
    aContext->LaunchScrollEndInjector();
  }

  virtual void Leave(AccessibleCaretEventHub* aContext) override
  {
    aContext->CancelScrollEndInjector();
  }
};




class AccessibleCaretEventHub::LongTapState
  : public AccessibleCaretEventHub::State
{
public:
  NS_IMPL_STATE_UTILITIES(LongTapState)

  virtual nsEventStatus OnLongTap(AccessibleCaretEventHub* aContext,
                                  const nsPoint& aPoint) override
  {
    nsEventStatus rv = nsEventStatus_eIgnore;

    if (NS_SUCCEEDED(aContext->mManager->SelectWordOrShortcut(aPoint))) {
      rv = nsEventStatus_eConsumeNoDefault;
    }

    aContext->SetState(aContext->NoActionState());

    return rv;
  }

  virtual void OnReflow(AccessibleCaretEventHub* aContext) override
  {
    aContext->mManager->OnReflow();
  }
};




AccessibleCaretEventHub::State*
AccessibleCaretEventHub::GetState() const
{
  return mState;
}

void
AccessibleCaretEventHub::SetState(State* aState)
{
  MOZ_ASSERT(aState);

  AC_LOG("%s -> %s", mState->Name(), aState->Name());

  mState->Leave(this);
  mState = aState;
  mState->Enter(this);
}

NS_IMPL_STATE_CLASS_GETTER(NoActionState)
NS_IMPL_STATE_CLASS_GETTER(PressCaretState)
NS_IMPL_STATE_CLASS_GETTER(DragCaretState)
NS_IMPL_STATE_CLASS_GETTER(PressNoCaretState)
NS_IMPL_STATE_CLASS_GETTER(ScrollState)
NS_IMPL_STATE_CLASS_GETTER(PostScrollState)
NS_IMPL_STATE_CLASS_GETTER(LongTapState)

AccessibleCaretEventHub::AccessibleCaretEventHub()
{
}

AccessibleCaretEventHub::~AccessibleCaretEventHub()
{
}

void
AccessibleCaretEventHub::Init(nsIPresShell* aPresShell)
{
  if (mInitialized || !aPresShell || !aPresShell->GetCanvasFrame() ||
      !aPresShell->GetCanvasFrame()->GetCustomContentContainer()) {
    return;
  }

  
  
  
  
  
  
  
  nsAutoScriptBlocker scriptBlocker;

  mPresShell = aPresShell;

  nsPresContext* presContext = mPresShell->GetPresContext();
  MOZ_ASSERT(presContext, "PresContext should be given in PresShell::Init()");

  nsIDocShell* docShell = presContext->GetDocShell();
  if (!docShell) {
    return;
  }

#if defined(MOZ_WIDGET_GONK)
  if (layers::LayerManager* lm = mPresShell->GetLayerManager()) {
    mUseAsyncPanZoom = lm->AsyncPanZoomEnabled();
  }
#endif

  docShell->AddWeakReflowObserver(this);
  docShell->AddWeakScrollObserver(this);

  mDocShell = static_cast<nsDocShell*>(docShell);

  mLongTapInjectorTimer = do_CreateInstance("@mozilla.org/timer;1");
  mScrollEndInjectorTimer = do_CreateInstance("@mozilla.org/timer;1");

  mManager = MakeUnique<AccessibleCaretManager>(mPresShell);

  mInitialized = true;
}

void
AccessibleCaretEventHub::Terminate()
{
  if (!mInitialized) {
    return;
  }

  nsRefPtr<nsDocShell> docShell(mDocShell.get());
  if (docShell) {
    docShell->RemoveWeakReflowObserver(this);
    docShell->RemoveWeakScrollObserver(this);
  }

  if (mLongTapInjectorTimer) {
    mLongTapInjectorTimer->Cancel();
  }

  if (mScrollEndInjectorTimer) {
    mScrollEndInjectorTimer->Cancel();
  }

  mManager = nullptr;
  mPresShell = nullptr;
  mInitialized = false;
}

nsEventStatus
AccessibleCaretEventHub::HandleEvent(WidgetEvent* aEvent)
{
  nsEventStatus status = nsEventStatus_eIgnore;

  if (!mInitialized) {
    return status;
  }

  switch (aEvent->mClass) {
  case eMouseEventClass:
    status = HandleMouseEvent(aEvent->AsMouseEvent());
    break;

  case eWheelEventClass:
    status = HandleWheelEvent(aEvent->AsWheelEvent());
    break;

  case eTouchEventClass:
    status = HandleTouchEvent(aEvent->AsTouchEvent());
    break;

  case eKeyboardEventClass:
    status = HandleKeyboardEvent(aEvent->AsKeyboardEvent());
    break;

  default:
    break;
  }

  return status;
}

nsEventStatus
AccessibleCaretEventHub::HandleMouseEvent(WidgetMouseEvent* aEvent)
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  if (aEvent->button != WidgetMouseEvent::eLeftButton) {
    return rv;
  }

  int32_t id = (mActiveTouchId == kInvalidTouchId ?
                kDefaultTouchId : mActiveTouchId);
  nsPoint point = GetMouseEventPosition(aEvent);

  switch (aEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
    AC_LOGV("Before NS_MOUSE_BUTTON_DOWN, state: %s", mState->Name());
    rv = mState->OnPress(this, point, id);
    AC_LOGV("After NS_MOUSE_BUTTON_DOWN, state: %s, consume: %d",
            mState->Name(), rv);
    break;

  case NS_MOUSE_MOVE:
    AC_LOGV("Before NS_MOUSE_MOVE, state: %s", mState->Name());
    rv = mState->OnMove(this, point);
    AC_LOGV("After NS_MOUSE_MOVE, state: %s, consume: %d", mState->Name(), rv);
    break;

  case NS_MOUSE_BUTTON_UP:
    AC_LOGV("Before NS_MOUSE_BUTTON_UP, state: %s", mState->Name());
    rv = mState->OnRelease(this);
    AC_LOGV("After NS_MOUSE_BUTTON_UP, state: %s, consume: %d", mState->Name(),
            rv);
    break;

  case NS_MOUSE_MOZLONGTAP:
    AC_LOGV("Before NS_MOUSE_MOZLONGTAP, state: %s", mState->Name());
    rv = mState->OnLongTap(this, point);
    AC_LOGV("After NS_MOUSE_MOZLONGTAP, state: %s, consume: %d", mState->Name(),
            rv);
    break;

  default:
    break;
  }

  return rv;
}

nsEventStatus
AccessibleCaretEventHub::HandleWheelEvent(WidgetWheelEvent* aEvent)
{
  switch (aEvent->message) {
  case NS_WHEEL_WHEEL:
    AC_LOGV("NS_WHEEL_WHEEL, isMomentum %d, state: %s", aEvent->isMomentum,
            mState->Name());
    mState->OnScrolling(this);
    break;

  case NS_WHEEL_START:
    AC_LOGV("NS_WHEEL_START, state: %s", mState->Name());
    mState->OnScrollStart(this);
    break;

  case NS_WHEEL_STOP:
    AC_LOGV("NS_WHEEL_STOP, state: %s", mState->Name());
    mState->OnScrollEnd(this);
    break;

  default:
    break;
  }

  
  
  return nsEventStatus_eIgnore;
}

nsEventStatus
AccessibleCaretEventHub::HandleTouchEvent(WidgetTouchEvent* aEvent)
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  int32_t id = (mActiveTouchId == kInvalidTouchId ?
                aEvent->touches[0]->Identifier() : mActiveTouchId);
  nsPoint point = GetTouchEventPosition(aEvent, id);

  switch (aEvent->message) {
  case NS_TOUCH_START:
    AC_LOGV("Before NS_TOUCH_START, state: %s", mState->Name());
    rv = mState->OnPress(this, point, id);
    AC_LOGV("After NS_TOUCH_START, state: %s, consume: %d", mState->Name(), rv);
    break;

  case NS_TOUCH_MOVE:
    AC_LOGV("Before NS_TOUCH_MOVE, state: %s", mState->Name());
    rv = mState->OnMove(this, point);
    AC_LOGV("After NS_TOUCH_MOVE, state: %s, consume: %d", mState->Name(), rv);
    break;

  case NS_TOUCH_END:
    AC_LOGV("Before NS_TOUCH_END, state: %s", mState->Name());
    rv = mState->OnRelease(this);
    AC_LOGV("After NS_TOUCH_END, state: %s, consume: %d", mState->Name(), rv);
    break;

  case NS_TOUCH_CANCEL:
    AC_LOGV("Before NS_TOUCH_CANCEL, state: %s", mState->Name());
    rv = mState->OnRelease(this);
    AC_LOGV("After NS_TOUCH_CANCEL, state: %s, consume: %d", mState->Name(),
            rv);
    break;

  default:
    break;
  }

  return rv;
}

nsEventStatus
AccessibleCaretEventHub::HandleKeyboardEvent(WidgetKeyboardEvent* aEvent)
{
  switch (aEvent->message) {
  case NS_KEY_UP:
  case NS_KEY_DOWN:
  case NS_KEY_PRESS:
    mManager->OnKeyboardEvent();
    break;

  default:
    break;
  }

  return nsEventStatus_eIgnore;
}

bool
AccessibleCaretEventHub::MoveDistanceIsLarge(const nsPoint& aPoint) const
{
  nsPoint delta = aPoint - mPressPoint;
  return NS_hypot(delta.x, delta.y) >
         nsPresContext::AppUnitsPerCSSPixel() * kMoveStartToleranceInPixel;
}

void
AccessibleCaretEventHub::LaunchLongTapInjector()
{
  if (mUseAsyncPanZoom) {
    return;
  }

  if (!mLongTapInjectorTimer) {
    return;
  }

  int32_t longTapDelay = gfxPrefs::UiClickHoldContextMenusDelay();
  mLongTapInjectorTimer->InitWithFuncCallback(FireLongTap, this, longTapDelay,
                                              nsITimer::TYPE_ONE_SHOT);
}

void
AccessibleCaretEventHub::CancelLongTapInjector()
{
  if (mUseAsyncPanZoom) {
    return;
  }

  if (!mLongTapInjectorTimer) {
    return;
  }

  mLongTapInjectorTimer->Cancel();
}

 void
AccessibleCaretEventHub::FireLongTap(nsITimer* aTimer,
                                     void* aAccessibleCaretEventHub)
{
  auto self = static_cast<AccessibleCaretEventHub*>(aAccessibleCaretEventHub);
  self->mState->OnLongTap(self, self->mPressPoint);
}

NS_IMETHODIMP
AccessibleCaretEventHub::Reflow(DOMHighResTimeStamp aStart,
                                DOMHighResTimeStamp aEnd)
{
  if (!mInitialized) {
    return NS_OK;
  }

  AC_LOG("%s, state: %s", __FUNCTION__, mState->Name());
  mState->OnReflow(this);
  return NS_OK;
}

NS_IMETHODIMP
AccessibleCaretEventHub::ReflowInterruptible(DOMHighResTimeStamp aStart,
                                             DOMHighResTimeStamp aEnd)
{
  if (!mInitialized) {
    return NS_OK;
  }

  return Reflow(aStart, aEnd);
}

void
AccessibleCaretEventHub::AsyncPanZoomStarted()
{
  if (!mInitialized) {
    return;
  }

  AC_LOG("%s, state: %s", __FUNCTION__, mState->Name());
  mState->OnScrollStart(this);
}

void
AccessibleCaretEventHub::AsyncPanZoomStopped()
{
  if (!mInitialized) {
    return;
  }

  AC_LOG("%s, state: %s", __FUNCTION__, mState->Name());
  mState->OnScrollEnd(this);
}

void
AccessibleCaretEventHub::ScrollPositionChanged()
{
  if (!mInitialized) {
    return;
  }

  AC_LOG("%s, state: %s", __FUNCTION__, mState->Name());
  mState->OnScrollPositionChanged(this);
}

void
AccessibleCaretEventHub::LaunchScrollEndInjector()
{
  if (!mScrollEndInjectorTimer) {
    return;
  }

  mScrollEndInjectorTimer->InitWithFuncCallback(
    FireScrollEnd, this, kScrollEndTimerDelay, nsITimer::TYPE_ONE_SHOT);
}

void
AccessibleCaretEventHub::CancelScrollEndInjector()
{
  if (!mScrollEndInjectorTimer) {
    return;
  }

  mScrollEndInjectorTimer->Cancel();
}

 void
AccessibleCaretEventHub::FireScrollEnd(nsITimer* aTimer,
                                       void* aAccessibleCaretEventHub)
{
  auto self = static_cast<AccessibleCaretEventHub*>(aAccessibleCaretEventHub);
  self->mState->OnScrollEnd(self);
}

nsresult
AccessibleCaretEventHub::NotifySelectionChanged(nsIDOMDocument* aDoc,
                                                nsISelection* aSel,
                                                int16_t aReason)
{
  if (!mInitialized) {
    return NS_OK;
  }

  AC_LOG("%s, state: %s, reason: %d", __FUNCTION__, mState->Name(), aReason);
  mState->OnSelectionChanged(this, aDoc, aSel, aReason);
  return NS_OK;
}

void
AccessibleCaretEventHub::NotifyBlur(bool aIsLeavingDocument)
{
  if (!mInitialized) {
    return;
  }

  AC_LOG("%s, state: %s", __FUNCTION__, mState->Name());
  mState->OnBlur(this, aIsLeavingDocument);
}

nsPoint
AccessibleCaretEventHub::GetTouchEventPosition(WidgetTouchEvent* aEvent,
                                               int32_t aIdentifier) const
{
  for (dom::Touch* touch : aEvent->touches) {
    if (touch->Identifier() == aIdentifier) {
      LayoutDeviceIntPoint touchIntPoint = touch->mRefPoint;

      
      nsIFrame* rootFrame = mPresShell->GetRootFrame();
      return nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, touchIntPoint,
                                                          rootFrame);
    }
  }
  return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
}

nsPoint
AccessibleCaretEventHub::GetMouseEventPosition(WidgetMouseEvent* aEvent) const
{
  LayoutDeviceIntPoint mouseIntPoint = aEvent->AsGUIEvent()->refPoint;

  
  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  return nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, mouseIntPoint,
                                                      rootFrame);
}

} 
