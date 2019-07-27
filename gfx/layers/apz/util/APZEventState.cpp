




#include "APZEventState.h"

#include "ActiveElementManager.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/Preferences.h"
#include "nsCOMPtr.h"
#include "nsDocShell.h"
#include "nsIDOMWindowUtils.h"
#include "nsITimer.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIWidget.h"
#include "TouchManager.h"

#define APZES_LOG(...)



namespace {

int32_t
WidgetModifiersToDOMModifiers(mozilla::Modifiers aModifiers)
{
  int32_t result = 0;
  if (aModifiers & mozilla::MODIFIER_SHIFT) {
    result |= nsIDOMWindowUtils::MODIFIER_SHIFT;
  }
  if (aModifiers & mozilla::MODIFIER_CONTROL) {
    result |= nsIDOMWindowUtils::MODIFIER_CONTROL;
  }
  if (aModifiers & mozilla::MODIFIER_ALT) {
    result |= nsIDOMWindowUtils::MODIFIER_ALT;
  }
  if (aModifiers & mozilla::MODIFIER_META) {
    result |= nsIDOMWindowUtils::MODIFIER_META;
  }
  if (aModifiers & mozilla::MODIFIER_ALTGRAPH) {
    result |= nsIDOMWindowUtils::MODIFIER_ALTGRAPH;
  }
  if (aModifiers & mozilla::MODIFIER_CAPSLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_CAPSLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_FN) {
    result |= nsIDOMWindowUtils::MODIFIER_FN;
  }
  if (aModifiers & mozilla::MODIFIER_FNLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_FNLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_NUMLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_NUMLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_SCROLLLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_SCROLLLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_SYMBOL) {
    result |= nsIDOMWindowUtils::MODIFIER_SYMBOL;
  }
  if (aModifiers & mozilla::MODIFIER_SYMBOLLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_SYMBOLLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_OS) {
    result |= nsIDOMWindowUtils::MODIFIER_OS;
  }
  return result;
}

}

namespace mozilla {
namespace layers {

static int32_t sActiveDurationMs = 10;
static bool sActiveDurationMsSet = false;

APZEventState::APZEventState(nsIWidget* aWidget,
                             const nsRefPtr<ContentReceivedInputBlockCallback>& aCallback)
  : mWidget(nullptr)  
  , mActiveElementManager(new ActiveElementManager())
  , mContentReceivedInputBlockCallback(aCallback)
  , mPendingTouchPreventedResponse(false)
  , mPendingTouchPreventedBlockId(0)
  , mEndTouchIsClick(false)
  , mTouchEndCancelled(false)
  , mActiveAPZTransforms(0)
{
  nsresult rv;
  mWidget = do_GetWeakReference(aWidget, &rv);
  MOZ_ASSERT(NS_SUCCEEDED(rv), "APZEventState constructed with a widget that"
      " does not support weak references. APZ will NOT work!");

  if (!sActiveDurationMsSet) {
    Preferences::AddIntVarCache(&sActiveDurationMs,
                                "ui.touch_activation.duration_ms",
                                sActiveDurationMs);
    sActiveDurationMsSet = true;
  }
}

APZEventState::~APZEventState()
{}

class DelayedFireSingleTapEvent final : public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS

  DelayedFireSingleTapEvent(nsWeakPtr aWidget,
                            LayoutDevicePoint& aPoint,
                            Modifiers aModifiers,
                            nsITimer* aTimer)
    : mWidget(aWidget)
    , mPoint(aPoint)
    , mModifiers(aModifiers)
    
    , mTimer(aTimer)
  {
  }

  NS_IMETHODIMP Notify(nsITimer*) override
  {
    if (nsCOMPtr<nsIWidget> widget = do_QueryReferent(mWidget)) {
      APZCCallbackHelper::FireSingleTapEvent(mPoint, mModifiers, widget);
    }
    mTimer = nullptr;
    return NS_OK;
  }

  void ClearTimer() {
    mTimer = nullptr;
  }

private:
  ~DelayedFireSingleTapEvent()
  {
  }

  nsWeakPtr mWidget;
  LayoutDevicePoint mPoint;
  Modifiers mModifiers;
  nsCOMPtr<nsITimer> mTimer;
};

NS_IMPL_ISUPPORTS(DelayedFireSingleTapEvent, nsITimerCallback)

void
APZEventState::ProcessSingleTap(const CSSPoint& aPoint,
                                Modifiers aModifiers,
                                const ScrollableLayerGuid& aGuid,
                                float aPresShellResolution)
{
  APZES_LOG("Handling single tap at %s on %s with %d\n",
    Stringify(aPoint).c_str(), Stringify(aGuid).c_str(), mTouchEndCancelled);

  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget) {
    return;
  }

  if (mTouchEndCancelled) {
    return;
  }

  LayoutDevicePoint currentPoint =
      APZCCallbackHelper::ApplyCallbackTransform(aPoint, aGuid, aPresShellResolution)
    * widget->GetDefaultScale();;
  if (!mActiveElementManager->ActiveElementUsesStyle()) {
    
    
    
    APZCCallbackHelper::FireSingleTapEvent(currentPoint, aModifiers, widget);
    return;
  }

  APZES_LOG("Active element uses style, scheduling timer for click event\n");
  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
  nsRefPtr<DelayedFireSingleTapEvent> callback =
    new DelayedFireSingleTapEvent(mWidget, currentPoint, aModifiers, timer);
  nsresult rv = timer->InitWithCallback(callback,
                                        sActiveDurationMs,
                                        nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv)) {
    
    
    callback->ClearTimer();
  }
}

void
APZEventState::ProcessLongTap(const nsCOMPtr<nsIDOMWindowUtils>& aUtils,
                              const CSSPoint& aPoint,
                              Modifiers aModifiers,
                              const ScrollableLayerGuid& aGuid,
                              uint64_t aInputBlockId,
                              float aPresShellResolution)
{
  APZES_LOG("Handling long tap at %s\n", Stringify(aPoint).c_str());

  nsCOMPtr<nsIWidget> widget = GetWidget();
  if (!widget) {
    return;
  }

  SendPendingTouchPreventedResponse(false, aGuid);

  
  
  
  
  bool eventHandled =
      APZCCallbackHelper::DispatchMouseEvent(aUtils, NS_LITERAL_STRING("contextmenu"),
                         APZCCallbackHelper::ApplyCallbackTransform(aPoint, aGuid, aPresShellResolution),
                         2, 1, WidgetModifiersToDOMModifiers(aModifiers), true,
                         nsIDOMMouseEvent::MOZ_SOURCE_TOUCH);

  APZES_LOG("Contextmenu event handled: %d\n", eventHandled);

  
  if (!eventHandled) {
    LayoutDevicePoint currentPoint =
        APZCCallbackHelper::ApplyCallbackTransform(aPoint, aGuid, aPresShellResolution)
      * widget->GetDefaultScale();
    int time = 0;
    nsEventStatus status =
        APZCCallbackHelper::DispatchSynthesizedMouseEvent(NS_MOUSE_MOZLONGTAP, time, currentPoint, aModifiers, widget);
    eventHandled = (status == nsEventStatus_eConsumeNoDefault);
    APZES_LOG("MOZLONGTAP event handled: %d\n", eventHandled);
  }

  mContentReceivedInputBlockCallback->Run(aGuid, aInputBlockId, eventHandled);
}

void
APZEventState::ProcessTouchEvent(const WidgetTouchEvent& aEvent,
                                 const ScrollableLayerGuid& aGuid,
                                 uint64_t aInputBlockId)
{
  if (aEvent.message == NS_TOUCH_START && aEvent.touches.Length() > 0) {
    mActiveElementManager->SetTargetElement(aEvent.touches[0]->GetTarget());
  }

  bool isTouchPrevented = TouchManager::gPreventMouseEvents ||
      aEvent.mFlags.mMultipleActionsPrevented;
  switch (aEvent.message) {
  case NS_TOUCH_START: {
    mTouchEndCancelled = false;
    if (mPendingTouchPreventedResponse) {
      
      
      mContentReceivedInputBlockCallback->Run(mPendingTouchPreventedGuid,
          mPendingTouchPreventedBlockId, false);
      mPendingTouchPreventedResponse = false;
    }
    if (isTouchPrevented) {
      mContentReceivedInputBlockCallback->Run(aGuid, aInputBlockId, isTouchPrevented);
    } else {
      mPendingTouchPreventedResponse = true;
      mPendingTouchPreventedGuid = aGuid;
      mPendingTouchPreventedBlockId = aInputBlockId;
    }
    break;
  }

  case NS_TOUCH_END:
    if (isTouchPrevented) {
      mTouchEndCancelled = true;
      mEndTouchIsClick = false;
    }
    
  case NS_TOUCH_CANCEL:
    mActiveElementManager->HandleTouchEndEvent(mEndTouchIsClick);
    
  case NS_TOUCH_MOVE: {
    SendPendingTouchPreventedResponse(isTouchPrevented, aGuid);
    break;
  }

  default:
    NS_WARNING("Unknown touch event type");
  }
}

void
APZEventState::ProcessWheelEvent(const WidgetWheelEvent& aEvent,
                                 const ScrollableLayerGuid& aGuid,
                                 uint64_t aInputBlockId)
{
  mContentReceivedInputBlockCallback->Run(aGuid, aInputBlockId, aEvent.mFlags.mDefaultPrevented);
}

void
APZEventState::ProcessAPZStateChange(const nsCOMPtr<nsIDocument>& aDocument,
                                     ViewID aViewId,
                                     APZStateChange aChange,
                                     int aArg)
{
  switch (aChange)
  {
  case APZStateChange::TransformBegin:
  {
    nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aViewId);
    if (sf) {
      sf->SetTransformingByAPZ(true);
    }
    nsIScrollbarMediator* scrollbarMediator = do_QueryFrame(sf);
    if (scrollbarMediator) {
      scrollbarMediator->ScrollbarActivityStarted();
    }

    if (aDocument && mActiveAPZTransforms == 0) {
      nsCOMPtr<nsIDocShell> docshell(aDocument->GetDocShell());
      if (docshell && sf) {
        nsDocShell* nsdocshell = static_cast<nsDocShell*>(docshell.get());
        nsdocshell->NotifyAsyncPanZoomStarted();
      }
    }
    mActiveAPZTransforms++;
    break;
  }
  case APZStateChange::TransformEnd:
  {
    mActiveAPZTransforms--;
    nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aViewId);
    if (sf) {
      sf->SetTransformingByAPZ(false);
    }
    nsIScrollbarMediator* scrollbarMediator = do_QueryFrame(sf);
    if (scrollbarMediator) {
      scrollbarMediator->ScrollbarActivityStopped();
    }

    if (aDocument && mActiveAPZTransforms == 0) {
      nsCOMPtr<nsIDocShell> docshell(aDocument->GetDocShell());
      if (docshell && sf) {
        nsDocShell* nsdocshell = static_cast<nsDocShell*>(docshell.get());
        nsdocshell->NotifyAsyncPanZoomStopped();
      }
    }
    break;
  }
  case APZStateChange::StartTouch:
  {
    mActiveElementManager->HandleTouchStart(aArg);
    break;
  }
  case APZStateChange::StartPanning:
  {
    mActiveElementManager->HandlePanStart();
    break;
  }
  case APZStateChange::EndTouch:
  {
    mEndTouchIsClick = aArg;
    mActiveElementManager->HandleTouchEnd();
    break;
  }
  default:
    
    
    break;
  }
}

void
APZEventState::SendPendingTouchPreventedResponse(bool aPreventDefault,
                                                 const ScrollableLayerGuid& aGuid)
{
  if (mPendingTouchPreventedResponse) {
    MOZ_ASSERT(aGuid == mPendingTouchPreventedGuid);
    mContentReceivedInputBlockCallback->Run(mPendingTouchPreventedGuid,
        mPendingTouchPreventedBlockId, aPreventDefault);
    mPendingTouchPreventedResponse = false;
  }
}

already_AddRefed<nsIWidget>
APZEventState::GetWidget() const
{
  nsCOMPtr<nsIWidget> result = do_QueryReferent(mWidget);
  return result.forget();
}

}
}

