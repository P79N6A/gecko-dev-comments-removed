




#include "APZCCallbackHelper.h"
#include "gfxPlatform.h" 
#include "gfxPrefs.h"    
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsIDOMElement.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"

namespace mozilla {
namespace layers {

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
    
    
    LayerPoint shift =
        (aFrameMetrics.GetScrollOffset() - aActualScrollOffset) *
        aFrameMetrics.LayersPixelsPerCSSPixel();
    LayerMargin margins = aFrameMetrics.GetDisplayPortMargins();
    margins.left -= shift.x;
    margins.right += shift.x;
    margins.top -= shift.y;
    margins.bottom += shift.y;
    aFrameMetrics.SetDisplayPortMargins(margins);
}

static void
RecenterDisplayPort(mozilla::layers::FrameMetrics& aFrameMetrics)
{
    LayerMargin margins = aFrameMetrics.GetDisplayPortMargins();
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

    
    nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aMetrics.GetScrollId());
    bool scrollUpdated = false;
    CSSPoint actualScrollOffset = ScrollFrameTo(sf, aMetrics.GetScrollOffset(), scrollUpdated);

    if (scrollUpdated) {
        
        
        AdjustDisplayPortForScrollDelta(aMetrics, actualScrollOffset);
    } else {
        
        
        
        
        
        
        RecenterDisplayPort(aMetrics);
    }

    aMetrics.SetScrollOffset(actualScrollOffset);

    
    
    
    
    
    
    
    
    ParentLayerToLayerScale presShellResolution =
        aMetrics.GetZoom()
        / aMetrics.mDevPixelsPerCSSPixel
        / aMetrics.GetParentResolution()
        * ScreenToLayerScale(1.0f);
    aUtils->SetResolution(presShellResolution.scale, presShellResolution.scale);

    
    nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aMetrics.GetScrollId());
    if (!content) {
        return;
    }
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(content);
    if (!element) {
        return;
    }

    gfx::IntSize alignment = gfxPlatform::GetPlatform()->UseTiling()
        ? gfx::IntSize(gfxPrefs::LayersTileWidth(), gfxPrefs::LayersTileHeight()) :
          gfx::IntSize(0, 0);
    LayerMargin margins = aMetrics.GetDisplayPortMargins();
    aUtils->SetDisplayPortMarginsForElement(margins.left,
                                            margins.top,
                                            margins.right,
                                            margins.bottom,
                                            alignment.width,
                                            alignment.height,
                                            element, 0);
    CSSRect baseCSS = aMetrics.CalculateCompositedRectInCssPixels();
    nsRect base(baseCSS.x * nsPresContext::AppUnitsPerCSSPixel(),
                baseCSS.y * nsPresContext::AppUnitsPerCSSPixel(),
                baseCSS.width * nsPresContext::AppUnitsPerCSSPixel(),
                baseCSS.height * nsPresContext::AppUnitsPerCSSPixel());
    nsLayoutUtils::SetDisplayPortBaseIfNotSet(content, base);
}

void
APZCCallbackHelper::UpdateSubFrame(nsIContent* aContent,
                                   FrameMetrics& aMetrics)
{
    
    MOZ_ASSERT(aContent);
    MOZ_ASSERT(aMetrics.GetUseDisplayPortMargins());
    if (aMetrics.GetScrollId() == FrameMetrics::NULL_SCROLL_ID) {
        return;
    }

    nsCOMPtr<nsIDOMWindowUtils> utils = GetDOMWindowUtils(aContent);
    if (!utils) {
        return;
    }

    
    

    nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aMetrics.GetScrollId());
    bool scrollUpdated = false;
    CSSPoint actualScrollOffset = ScrollFrameTo(sf, aMetrics.GetScrollOffset(), scrollUpdated);

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aContent);
    if (element) {
        if (scrollUpdated) {
            AdjustDisplayPortForScrollDelta(aMetrics, actualScrollOffset);
        } else {
            RecenterDisplayPort(aMetrics);
        }
        gfx::IntSize alignment = gfxPlatform::GetPlatform()->UseTiling()
            ? gfx::IntSize(gfxPrefs::LayersTileWidth(), gfxPrefs::LayersTileHeight()) :
              gfx::IntSize(0, 0);
        LayerMargin margins = aMetrics.GetDisplayPortMargins();
        utils->SetDisplayPortMarginsForElement(margins.left,
                                               margins.top,
                                               margins.right,
                                               margins.bottom,
                                               alignment.width,
                                               alignment.height,
                                               element, 0);
        CSSRect baseCSS = aMetrics.CalculateCompositedRectInCssPixels();
        nsRect base(baseCSS.x * nsPresContext::AppUnitsPerCSSPixel(),
                    baseCSS.y * nsPresContext::AppUnitsPerCSSPixel(),
                    baseCSS.width * nsPresContext::AppUnitsPerCSSPixel(),
                    baseCSS.height * nsPresContext::AppUnitsPerCSSPixel());
        nsLayoutUtils::SetDisplayPortBaseIfNotSet(aContent, base);
    }

    aMetrics.SetScrollOffset(actualScrollOffset);
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

void
APZCCallbackHelper::UpdateCallbackTransform(const FrameMetrics& aApzcMetrics, const FrameMetrics& aActualMetrics)
{
    nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aApzcMetrics.GetScrollId());
    if (!content) {
        return;
    }
    CSSPoint scrollDelta = aApzcMetrics.GetScrollOffset() - aActualMetrics.GetScrollOffset();
    content->SetProperty(nsGkAtoms::apzCallbackTransform, new CSSPoint(scrollDelta),
                         nsINode::DeleteProperty<CSSPoint>);
}

CSSPoint
APZCCallbackHelper::ApplyCallbackTransform(const CSSPoint& aInput, const ScrollableLayerGuid& aGuid)
{
    
    
    
    
    
    
    
    
    
    

    if (aGuid.mScrollId != FrameMetrics::NULL_SCROLL_ID) {
        nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aGuid.mScrollId);
        if (content) {
            void* property = content->GetProperty(nsGkAtoms::apzCallbackTransform);
            if (property) {
                CSSPoint delta = (*static_cast<CSSPoint*>(property));
                return aInput + delta;
            }
        }
    }
    return aInput;
}

nsIntPoint
APZCCallbackHelper::ApplyCallbackTransform(const nsIntPoint& aPoint,
                                        const ScrollableLayerGuid& aGuid,
                                        const CSSToLayoutDeviceScale& aScale)
{
    LayoutDevicePoint point = LayoutDevicePoint(aPoint.x, aPoint.y);
    point = ApplyCallbackTransform(point / aScale, aGuid) * aScale;
    LayoutDeviceIntPoint ret = gfx::RoundedToInt(point);
    return nsIntPoint(ret.x, ret.y);
}

}
}
