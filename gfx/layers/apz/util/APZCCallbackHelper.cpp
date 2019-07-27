




#include "APZCCallbackHelper.h"

#include "gfxPlatform.h" 
#include "mozilla/dom/TabParent.h"
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
                                    FrameMetrics& aMetrics)
{
  
  MOZ_ASSERT(aUtils);
  MOZ_ASSERT(aMetrics.GetUseDisplayPortMargins());
  if (aMetrics.GetScrollId() == FrameMetrics::NULL_SCROLL_ID) {
    return;
  }

  
  
  
  
  
  
  
  
  
  CSSSize scrollPort = aMetrics.CalculateCompositedSizeInCssPixels();
  aUtils->SetScrollPositionClampingScrollPortSize(scrollPort.width, scrollPort.height);

  nsIContent* content = nsLayoutUtils::FindContentFor(aMetrics.GetScrollId());
  ScrollFrame(content, aMetrics);

  
  
  float presShellResolution = aMetrics.GetPresShellResolution()
                            * aMetrics.GetAsyncZoom().scale;
  aUtils->SetResolutionAndScaleTo(presShellResolution, presShellResolution);

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
    nsIDocument* doc = aContent->GetCurrentDoc();
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
  if (!aEvent.widget)
    return nsEventStatus_eConsumeNoDefault;

  
  if (TabParent* capturer = TabParent::GetEventCapturer()) {
    if (capturer->TryCapture(aEvent)) {
      
      
      
      MOZ_ASSERT(!XRE_IsParentProcess());

      return nsEventStatus_eConsumeNoDefault;
    }
  }
  nsEventStatus status;
  NS_ENSURE_SUCCESS(aEvent.widget->DispatchEvent(&aEvent, status),
                    nsEventStatus_eConsumeNoDefault);
  return status;
}

nsEventStatus
APZCCallbackHelper::DispatchSynthesizedMouseEvent(uint32_t aMsg,
                                                  uint64_t aTime,
                                                  const LayoutDevicePoint& aRefPoint,
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
  event.widget = aWidget;

  return DispatchWidgetEvent(event);
}

void
APZCCallbackHelper::FireSingleTapEvent(const LayoutDevicePoint& aPoint,
                                       nsIWidget* aWidget)
{
  if (aWidget->Destroyed()) {
    return;
  }
  APZCCH_LOG("Dispatching single-tap component events to %s\n",
    Stringify(aPoint).c_str());
  int time = 0;
  DispatchSynthesizedMouseEvent(NS_MOUSE_MOVE, time, aPoint, aWidget);
  DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_DOWN, time, aPoint, aWidget);
  DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_UP, time, aPoint, aWidget);
}

}
}
