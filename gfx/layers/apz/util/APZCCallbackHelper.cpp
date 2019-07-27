




#include "APZCCallbackHelper.h"

#include "ContentHelper.h"
#include "gfxPlatform.h" 
#include "mozilla/dom/TabParent.h"
#include "mozilla/layers/LayerTransactionChild.h"
#include "mozilla/layers/ShadowLayers.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsIDOMElement.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"

#define APZCCH_LOG(...)


namespace mozilla {
namespace layers {

using dom::TabParent;

bool
APZCCallbackHelper::HasValidPresShellId(nsIDOMWindowUtils* aUtils,
                                        const FrameMetrics& aMetrics)
{
    MOZ_ASSERT(aUtils);

    uint32_t presShellId;
    nsresult rv = aUtils->GetPresShellId(&presShellId);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    return NS_SUCCEEDED(rv) && aMetrics.GetPresShellId() == presShellId;
}

static void
AdjustDisplayPortForScrollDelta(mozilla::layers::FrameMetrics& aFrameMetrics,
                                const CSSPoint& aActualScrollOffset)
{
    
    
    ScreenPoint shift =
        (aFrameMetrics.GetScrollOffset() - aActualScrollOffset) *
        aFrameMetrics.DisplayportPixelsPerCSSPixel();
    ScreenMargin margins = aFrameMetrics.GetDisplayPortMargins();
    margins.left -= shift.x;
    margins.right += shift.x;
    margins.top -= shift.y;
    margins.bottom += shift.y;
    aFrameMetrics.SetDisplayPortMargins(margins);
}

static void
RecenterDisplayPort(mozilla::layers::FrameMetrics& aFrameMetrics)
{
    ScreenMargin margins = aFrameMetrics.GetDisplayPortMargins();
    margins.right = margins.left = margins.LeftRight() / 2;
    margins.top = margins.bottom = margins.TopBottom() / 2;
    aFrameMetrics.SetDisplayPortMargins(margins);
}

static CSSPoint
ScrollFrameTo(nsIScrollableFrame* aFrame, const CSSPoint& aPoint, bool& aSuccessOut)
{
  aSuccessOut = false;

  if (!aFrame) {
    return aPoint;
  }

  CSSPoint targetScrollPosition = aPoint;

  
  
  
  
  
  
  
  CSSPoint geckoScrollPosition = CSSPoint::FromAppUnits(aFrame->GetScrollPosition());
  if (aFrame->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_HIDDEN) {
    targetScrollPosition.y = geckoScrollPosition.y;
  }
  if (aFrame->GetScrollbarStyles().mHorizontal == NS_STYLE_OVERFLOW_HIDDEN) {
    targetScrollPosition.x = geckoScrollPosition.x;
  }

  
  
  
  
  
  bool scrollInProgress = aFrame->IsProcessingAsyncScroll()
      || (aFrame->LastScrollOrigin() && aFrame->LastScrollOrigin() != nsGkAtoms::apz)
      || aFrame->LastSmoothScrollOrigin();
  if (!scrollInProgress) {
    aFrame->ScrollToCSSPixelsApproximate(targetScrollPosition, nsGkAtoms::apz);
    geckoScrollPosition = CSSPoint::FromAppUnits(aFrame->GetScrollPosition());
    aSuccessOut = true;
  }
  
  
  
  return geckoScrollPosition;
}










static void
ScrollFrame(nsIContent* aContent,
            FrameMetrics& aMetrics)
{
  
  nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aMetrics.GetScrollId());
  bool scrollUpdated = false;
  CSSPoint apzScrollOffset = aMetrics.GetScrollOffset();
  CSSPoint actualScrollOffset = ScrollFrameTo(sf, apzScrollOffset, scrollUpdated);

  if (scrollUpdated) {
    
    
    AdjustDisplayPortForScrollDelta(aMetrics, actualScrollOffset);
  } else {
    
    
    
    
    
    
    RecenterDisplayPort(aMetrics);
  }

  aMetrics.SetScrollOffset(actualScrollOffset);

  
  
  
  
  if (aContent) {
    CSSPoint scrollDelta = apzScrollOffset - actualScrollOffset;
    aContent->SetProperty(nsGkAtoms::apzCallbackTransform, new CSSPoint(scrollDelta),
                          nsINode::DeleteProperty<CSSPoint>);
  }
}

static void
SetDisplayPortMargins(nsIDOMWindowUtils* aUtils,
                      nsIContent* aContent,
                      FrameMetrics& aMetrics)
{
  if (!aContent) {
    return;
  }
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aContent);
  if (!element) {
    return;
  }

  ScreenMargin margins = aMetrics.GetDisplayPortMargins();
  aUtils->SetDisplayPortMarginsForElement(margins.left,
                                          margins.top,
                                          margins.right,
                                          margins.bottom,
                                          element, 0);
  CSSRect baseCSS = aMetrics.CalculateCompositedRectInCssPixels();
  nsRect base(0, 0,
              baseCSS.width * nsPresContext::AppUnitsPerCSSPixel(),
              baseCSS.height * nsPresContext::AppUnitsPerCSSPixel());
  nsLayoutUtils::SetDisplayPortBaseIfNotSet(aContent, base);
}

void
APZCCallbackHelper::UpdateRootFrame(nsIDOMWindowUtils* aUtils,
                                    nsIPresShell* aPresShell,
                                    FrameMetrics& aMetrics)
{
  
  MOZ_ASSERT(aUtils);
  MOZ_ASSERT(aPresShell);
  MOZ_ASSERT(aMetrics.GetUseDisplayPortMargins());
  if (aMetrics.GetScrollId() == FrameMetrics::NULL_SCROLL_ID) {
    return;
  }

  float presShellResolution = nsLayoutUtils::GetResolution(aPresShell);

  
  
  
  
  if (presShellResolution != aMetrics.GetPresShellResolution()) {
    return;
  }

  
  
  
  
  
  
  
  
  
  CSSSize scrollPort = aMetrics.CalculateCompositedSizeInCssPixels();
  aUtils->SetScrollPositionClampingScrollPortSize(scrollPort.width, scrollPort.height);

  nsIContent* content = nsLayoutUtils::FindContentFor(aMetrics.GetScrollId());
  ScrollFrame(content, aMetrics);

  
  
  presShellResolution = aMetrics.GetPresShellResolution()
                      * aMetrics.GetAsyncZoom().scale;
  nsLayoutUtils::SetResolutionAndScaleTo(aPresShell, presShellResolution);

  SetDisplayPortMargins(aUtils, content, aMetrics);
}

void
APZCCallbackHelper::UpdateSubFrame(nsIContent* aContent,
                                   FrameMetrics& aMetrics)
{
  
  MOZ_ASSERT(aContent);
  MOZ_ASSERT(aMetrics.GetUseDisplayPortMargins());

  
  
  ScrollFrame(aContent, aMetrics);
  if (nsCOMPtr<nsIDOMWindowUtils> utils = GetDOMWindowUtils(aContent)) {
    SetDisplayPortMargins(utils, aContent, aMetrics);
  }
}

already_AddRefed<nsIDOMWindowUtils>
APZCCallbackHelper::GetDOMWindowUtils(const nsIDocument* aDoc)
{
    nsCOMPtr<nsIDOMWindowUtils> utils;
    nsCOMPtr<nsIDOMWindow> window = aDoc->GetDefaultView();
    if (window) {
        utils = do_GetInterface(window);
    }
    return utils.forget();
}

already_AddRefed<nsIDOMWindowUtils>
APZCCallbackHelper::GetDOMWindowUtils(const nsIContent* aContent)
{
    nsCOMPtr<nsIDOMWindowUtils> utils;
    nsIDocument* doc = aContent->GetComposedDoc();
    if (doc) {
        utils = GetDOMWindowUtils(doc);
    }
    return utils.forget();
}

bool
APZCCallbackHelper::GetOrCreateScrollIdentifiers(nsIContent* aContent,
                                                 uint32_t* aPresShellIdOut,
                                                 FrameMetrics::ViewID* aViewIdOut)
{
    if (!aContent) {
        return false;
    }
    *aViewIdOut = nsLayoutUtils::FindOrCreateIDFor(aContent);
    nsCOMPtr<nsIDOMWindowUtils> utils = GetDOMWindowUtils(aContent);
    return utils && (utils->GetPresShellId(aPresShellIdOut) == NS_OK);
}

class FlingSnapEvent : public nsRunnable
{
    typedef mozilla::layers::FrameMetrics::ViewID ViewID;

public:
    FlingSnapEvent(const ViewID& aScrollId,
                   const mozilla::CSSPoint& aDestination)
        : mScrollId(aScrollId)
        , mDestination(aDestination)
    {
    }

    NS_IMETHOD Run() {
        MOZ_ASSERT(NS_IsMainThread());

        nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(mScrollId);
        if (sf) {
            sf->FlingSnap(mDestination);
        }

        return NS_OK;
    }

protected:
    ViewID mScrollId;
    mozilla::CSSPoint mDestination;
};

void
APZCCallbackHelper::RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                                     const mozilla::CSSPoint& aDestination)
{
    nsCOMPtr<nsIRunnable> r1 = new FlingSnapEvent(aScrollId, aDestination);
    if (!NS_IsMainThread()) {
        NS_DispatchToMainThread(r1);
    } else {
        r1->Run();
    }
}

class AcknowledgeScrollUpdateEvent : public nsRunnable
{
    typedef mozilla::layers::FrameMetrics::ViewID ViewID;

public:
    AcknowledgeScrollUpdateEvent(const ViewID& aScrollId, const uint32_t& aScrollGeneration)
        : mScrollId(aScrollId)
        , mScrollGeneration(aScrollGeneration)
    {
    }

    NS_IMETHOD Run() {
        MOZ_ASSERT(NS_IsMainThread());

        nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(mScrollId);
        if (sf) {
            sf->ResetScrollInfoIfGeneration(mScrollGeneration);
        }

        
        
        
        nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(mScrollId);
        if (content) {
            content->SetProperty(nsGkAtoms::apzCallbackTransform, new CSSPoint(),
                                 nsINode::DeleteProperty<CSSPoint>);
        }

        return NS_OK;
    }

protected:
    ViewID mScrollId;
    uint32_t mScrollGeneration;
};

void
APZCCallbackHelper::AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                            const uint32_t& aScrollGeneration)
{
    nsCOMPtr<nsIRunnable> r1 = new AcknowledgeScrollUpdateEvent(aScrollId, aScrollGeneration);
    if (!NS_IsMainThread()) {
        NS_DispatchToMainThread(r1);
    } else {
        r1->Run();
    }
}

CSSPoint
APZCCallbackHelper::ApplyCallbackTransform(const CSSPoint& aInput,
                                           const ScrollableLayerGuid& aGuid,
                                           float aPresShellResolution)
{
    
    
    
    
    
    CSSPoint input = aInput / aPresShellResolution;

    
    
    
    
    
    
    
    
    
    
    

    if (aGuid.mScrollId != FrameMetrics::NULL_SCROLL_ID) {
        nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aGuid.mScrollId);
        if (content) {
            void* property = content->GetProperty(nsGkAtoms::apzCallbackTransform);
            if (property) {
                CSSPoint delta = (*static_cast<CSSPoint*>(property));
                return input + delta;
            }
        }
    }
    return input;
}

LayoutDeviceIntPoint
APZCCallbackHelper::ApplyCallbackTransform(const LayoutDeviceIntPoint& aPoint,
                                           const ScrollableLayerGuid& aGuid,
                                           const CSSToLayoutDeviceScale& aScale,
                                           float aPresShellResolution)
{
    LayoutDevicePoint point = LayoutDevicePoint(aPoint.x, aPoint.y);
    point = ApplyCallbackTransform(point / aScale, aGuid, aPresShellResolution) * aScale;
    return gfx::RoundedToInt(point);
}

void
APZCCallbackHelper::ApplyCallbackTransform(WidgetTouchEvent& aEvent,
                                           const ScrollableLayerGuid& aGuid,
                                           const CSSToLayoutDeviceScale& aScale,
                                           float aPresShellResolution)
{
  for (size_t i = 0; i < aEvent.touches.Length(); i++) {
    aEvent.touches[i]->mRefPoint = ApplyCallbackTransform(
        aEvent.touches[i]->mRefPoint, aGuid, aScale, aPresShellResolution);
  }
}

nsEventStatus
APZCCallbackHelper::DispatchWidgetEvent(WidgetGUIEvent& aEvent)
{
  nsEventStatus status = nsEventStatus_eConsumeNoDefault;
  if (aEvent.widget) {
    aEvent.widget->DispatchEvent(&aEvent, status);
  }
  return status;
}

nsEventStatus
APZCCallbackHelper::DispatchSynthesizedMouseEvent(uint32_t aMsg,
                                                  uint64_t aTime,
                                                  const LayoutDevicePoint& aRefPoint,
                                                  Modifiers aModifiers,
                                                  nsIWidget* aWidget)
{
  MOZ_ASSERT(aMsg == NS_MOUSE_MOVE || aMsg == NS_MOUSE_BUTTON_DOWN ||
             aMsg == NS_MOUSE_BUTTON_UP || aMsg == NS_MOUSE_MOZLONGTAP);

  WidgetMouseEvent event(true, aMsg, nullptr,
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  event.refPoint = LayoutDeviceIntPoint(aRefPoint.x, aRefPoint.y);
  event.time = aTime;
  event.button = WidgetMouseEvent::eLeftButton;
  event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  event.ignoreRootScrollFrame = true;
  if (aMsg != NS_MOUSE_MOVE) {
    event.clickCount = 1;
  }
  event.modifiers = aModifiers;
  event.widget = aWidget;

  return DispatchWidgetEvent(event);
}

bool
APZCCallbackHelper::DispatchMouseEvent(const nsCOMPtr<nsIDOMWindowUtils>& aUtils,
                                       const nsString& aType,
                                       const CSSPoint& aPoint,
                                       int32_t aButton,
                                       int32_t aClickCount,
                                       int32_t aModifiers,
                                       bool aIgnoreRootScrollFrame,
                                       unsigned short aInputSourceArg)
{
  NS_ENSURE_TRUE(aUtils, true);

  bool defaultPrevented = false;
  aUtils->SendMouseEvent(aType, aPoint.x, aPoint.y, aButton, aClickCount, aModifiers,
                         aIgnoreRootScrollFrame, 0, aInputSourceArg, false, 4, &defaultPrevented);
  return defaultPrevented;
}


void
APZCCallbackHelper::FireSingleTapEvent(const LayoutDevicePoint& aPoint,
                                       Modifiers aModifiers,
                                       nsIWidget* aWidget)
{
  if (aWidget->Destroyed()) {
    return;
  }
  APZCCH_LOG("Dispatching single-tap component events to %s\n",
    Stringify(aPoint).c_str());
  int time = 0;
  DispatchSynthesizedMouseEvent(NS_MOUSE_MOVE, time, aPoint, aModifiers, aWidget);
  DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_DOWN, time, aPoint, aModifiers, aWidget);
  DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_UP, time, aPoint, aModifiers, aWidget);
}

static nsIScrollableFrame*
GetScrollableAncestorFrame(nsIFrame* aTarget)
{
  if (!aTarget) {
    return nullptr;
  }
  uint32_t flags = nsLayoutUtils::SCROLLABLE_ALWAYS_MATCH_ROOT
                 | nsLayoutUtils::SCROLLABLE_ONLY_ASYNC_SCROLLABLE;
  return nsLayoutUtils::GetNearestScrollableFrame(aTarget, flags);
}

static dom::Element*
GetDisplayportElementFor(nsIScrollableFrame* aScrollableFrame)
{
  if (!aScrollableFrame) {
    return nullptr;
  }
  nsIFrame* scrolledFrame = aScrollableFrame->GetScrolledFrame();
  if (!scrolledFrame) {
    return nullptr;
  }
  
  
  
  
  nsIContent* content = scrolledFrame->GetContent();
  MOZ_ASSERT(content->IsElement()); 
  return content->AsElement();
}




static bool
PrepareForSetTargetAPZCNotification(nsIWidget* aWidget,
                                    const ScrollableLayerGuid& aGuid,
                                    nsIFrame* aRootFrame,
                                    const LayoutDeviceIntPoint& aRefPoint,
                                    nsTArray<ScrollableLayerGuid>* aTargets)
{
  ScrollableLayerGuid guid(aGuid.mLayersId, 0, FrameMetrics::NULL_SCROLL_ID);
  nsPoint point =
    nsLayoutUtils::GetEventCoordinatesRelativeTo(aWidget, aRefPoint, aRootFrame);
  nsIFrame* target =
    nsLayoutUtils::GetFrameForPoint(aRootFrame, point, nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME);
  nsIScrollableFrame* scrollAncestor = GetScrollableAncestorFrame(target);
  nsCOMPtr<dom::Element> dpElement = GetDisplayportElementFor(scrollAncestor);

  nsAutoString dpElementDesc;
  if (dpElement) {
    dpElement->Describe(dpElementDesc);
  }
  APZCCH_LOG("For event at %s found scrollable element %p (%s)\n",
      Stringify(aRefPoint).c_str(), dpElement.get(),
      NS_LossyConvertUTF16toASCII(dpElementDesc).get());

  bool guidIsValid = APZCCallbackHelper::GetOrCreateScrollIdentifiers(
    dpElement, &(guid.mPresShellId), &(guid.mScrollId));
  aTargets->AppendElement(guid);

  if (!guidIsValid || nsLayoutUtils::GetDisplayPort(dpElement, nullptr)) {
    return false;
  }

  APZCCH_LOG("%p didn't have a displayport, so setting one...\n", dpElement.get());
  return nsLayoutUtils::CalculateAndSetDisplayPortMargins(
      scrollAncestor, nsLayoutUtils::RepaintMode::Repaint);
}

static void
SendLayersDependentApzcTargetConfirmation(nsIPresShell* aShell, uint64_t aInputBlockId,
                                          const nsTArray<ScrollableLayerGuid>& aTargets)
{
  LayerManager* lm = aShell->GetLayerManager();
  if (!lm) {
    return;
  }

  LayerTransactionChild* shadow = lm->AsShadowForwarder()->GetShadowManager();
  if (!shadow) {
    return;
  }

  shadow->SendSetConfirmedTargetAPZC(aInputBlockId, aTargets);
}

class DisplayportSetListener : public nsAPostRefreshObserver {
public:
  DisplayportSetListener(nsIPresShell* aPresShell,
                         const uint64_t& aInputBlockId,
                         const nsTArray<ScrollableLayerGuid>& aTargets)
    : mPresShell(aPresShell)
    , mInputBlockId(aInputBlockId)
    , mTargets(aTargets)
  {
  }

  virtual ~DisplayportSetListener()
  {
  }

  void DidRefresh() override {
    if (!mPresShell) {
      MOZ_ASSERT_UNREACHABLE("Post-refresh observer fired again after failed attempt at unregistering it");
      return;
    }

    APZCCH_LOG("Got refresh, sending target APZCs for input block %" PRIu64 "\n", mInputBlockId);
    SendLayersDependentApzcTargetConfirmation(mPresShell, mInputBlockId, mTargets);

    if (!mPresShell->RemovePostRefreshObserver(this)) {
      MOZ_ASSERT_UNREACHABLE("Unable to unregister post-refresh observer! Leaking it instead of leaving garbage registered");
      
      mPresShell = nullptr;
      return;
    }

    delete this;
  }

private:
  nsRefPtr<nsIPresShell> mPresShell;
  uint64_t mInputBlockId;
  nsTArray<ScrollableLayerGuid> mTargets;
};



static void
SendSetTargetAPZCNotificationHelper(nsIWidget* aWidget,
                                    nsIPresShell* aShell,
                                    const uint64_t& aInputBlockId,
                                    const nsTArray<ScrollableLayerGuid>& aTargets,
                                    bool aWaitForRefresh)
{
  bool waitForRefresh = aWaitForRefresh;
  if (waitForRefresh) {
    APZCCH_LOG("At least one target got a new displayport, need to wait for refresh\n");
    waitForRefresh = aShell->AddPostRefreshObserver(
      new DisplayportSetListener(aShell, aInputBlockId, aTargets));
  }
  if (!waitForRefresh) {
    APZCCH_LOG("Sending target APZCs for input block %" PRIu64 "\n", aInputBlockId);
    aWidget->SetConfirmedTargetAPZC(aInputBlockId, aTargets);
  } else {
    APZCCH_LOG("Successfully registered post-refresh observer\n");
  }
}

void
APZCCallbackHelper::SendSetTargetAPZCNotification(nsIWidget* aWidget,
                                                  nsIDocument* aDocument,
                                                  const WidgetGUIEvent& aEvent,
                                                  const ScrollableLayerGuid& aGuid,
                                                  uint64_t aInputBlockId)
{
  if (!aWidget || !aDocument) {
    return;
  }
  if (nsIPresShell* shell = aDocument->GetShell()) {
    if (nsIFrame* rootFrame = shell->GetRootFrame()) {
      bool waitForRefresh = false;
      nsTArray<ScrollableLayerGuid> targets;

      if (const WidgetTouchEvent* touchEvent = aEvent.AsTouchEvent()) {
        for (size_t i = 0; i < touchEvent->touches.Length(); i++) {
          waitForRefresh |= PrepareForSetTargetAPZCNotification(aWidget, aGuid,
              rootFrame, touchEvent->touches[i]->mRefPoint, &targets);
        }
      } else if (const WidgetWheelEvent* wheelEvent = aEvent.AsWheelEvent()) {
        waitForRefresh = PrepareForSetTargetAPZCNotification(aWidget, aGuid,
            rootFrame, wheelEvent->refPoint, &targets);
      }
      

      if (!targets.IsEmpty()) {
        SendSetTargetAPZCNotificationHelper(
          aWidget,
          shell,
          aInputBlockId,
          targets,
          waitForRefresh);
      }
    }
  }
}

void
APZCCallbackHelper::SendSetAllowedTouchBehaviorNotification(
        nsIWidget* aWidget,
        const WidgetTouchEvent& aEvent,
        uint64_t aInputBlockId,
        const nsRefPtr<SetAllowedTouchBehaviorCallback>& aCallback)
{
  nsTArray<TouchBehaviorFlags> flags;
  for (uint32_t i = 0; i < aEvent.touches.Length(); i++) {
    flags.AppendElement(widget::ContentHelper::GetAllowedTouchBehavior(aWidget, aEvent.touches[i]->mRefPoint));
  }
  aCallback->Run(aInputBlockId, flags);
}

void
APZCCallbackHelper::NotifyMozMouseScrollEvent(const FrameMetrics::ViewID& aScrollId, const nsString& aEvent)
{
  nsCOMPtr<nsIContent> targetContent = nsLayoutUtils::FindContentFor(aScrollId);
  if (!targetContent) {
    return;
  }
  nsCOMPtr<nsIDocument> ownerDoc = targetContent->OwnerDoc();
  if (!ownerDoc) {
    return;
  }

  nsContentUtils::DispatchTrustedEvent(
    ownerDoc, targetContent,
    aEvent,
    true, true);
}

}
}

