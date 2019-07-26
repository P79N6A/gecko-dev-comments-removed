




#include "APZCCallbackHelper.h"
#include "gfxPrefs.h" 
#include "mozilla/Preferences.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsIDOMElement.h"
#include "nsIInterfaceRequestorUtils.h"

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
    return NS_SUCCEEDED(rv) && aMetrics.mPresShellId == presShellId;
}





static CSSRect ExpandDisplayPortToTileBoundaries(
  const CSSRect& aDisplayPort,
  const CSSToLayerScale& aLayerPixelsPerCSSPixel)
{
  
  
  LayerRect displayPortInLayerSpace = aDisplayPort * aLayerPixelsPerCSSPixel;

  
  
  
  
  displayPortInLayerSpace.Inflate(1);

  
  int32_t tileWidth = gfxPrefs::LayersTileWidth();
  int32_t tileHeight = gfxPrefs::LayersTileHeight();
  gfxFloat left = tileWidth * floor(displayPortInLayerSpace.x / tileWidth);
  gfxFloat right = tileWidth * ceil(displayPortInLayerSpace.XMost() / tileWidth);
  gfxFloat top = tileHeight * floor(displayPortInLayerSpace.y / tileHeight);
  gfxFloat bottom = tileHeight * ceil(displayPortInLayerSpace.YMost() / tileHeight);

  displayPortInLayerSpace = LayerRect(left, top, right - left, bottom - top);
  CSSRect displayPort = displayPortInLayerSpace / aLayerPixelsPerCSSPixel;

  return displayPort;
}

static void
MaybeAlignAndClampDisplayPort(mozilla::layers::FrameMetrics& aFrameMetrics,
                              const CSSPoint& aActualScrollOffset)
{
  
  
  if (!aFrameMetrics.GetUseDisplayPortMargins()) {
      CSSRect& displayPort = aFrameMetrics.mDisplayPort;
      displayPort += aFrameMetrics.GetScrollOffset() - aActualScrollOffset;

      
      
      if (gfxPrefs::LayersTilesEnabled()) {
        
        
        displayPort =
          ExpandDisplayPortToTileBoundaries(displayPort + aActualScrollOffset,
                                            aFrameMetrics.GetZoom() *
                                            ScreenToLayerScale(1.0))
          - aActualScrollOffset;
      }

      
      CSSRect scrollableRect = aFrameMetrics.GetExpandedScrollableRect();
      displayPort = scrollableRect.Intersect(displayPort + aActualScrollOffset)
        - aActualScrollOffset;
  } else {
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
}

static void
RecenterDisplayPort(mozilla::layers::FrameMetrics& aFrameMetrics)
{
    if (!aFrameMetrics.GetUseDisplayPortMargins()) {
        CSSSize compositionSize = aFrameMetrics.CalculateCompositedSizeInCssPixels();
        aFrameMetrics.mDisplayPort.x = (compositionSize.width - aFrameMetrics.mDisplayPort.width) / 2;
        aFrameMetrics.mDisplayPort.y = (compositionSize.height - aFrameMetrics.mDisplayPort.height) / 2;
    } else {
        LayerMargin margins = aFrameMetrics.GetDisplayPortMargins();
        margins.right = margins.left = margins.LeftRight() / 2;
        margins.top = margins.bottom = margins.TopBottom() / 2;
        aFrameMetrics.SetDisplayPortMargins(margins);
    }
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

  
  
  
  
  
  if (!aFrame->IsProcessingAsyncScroll() &&
     (!aFrame->OriginOfLastScroll() || aFrame->OriginOfLastScroll() == nsGkAtoms::apz)) {
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
    if (aMetrics.GetScrollId() == FrameMetrics::NULL_SCROLL_ID) {
        return;
    }

    
    
    
    
    
    
    
    CSSSize scrollPort = aMetrics.CalculateCompositedSizeInCssPixels();
    aUtils->SetScrollPositionClampingScrollPortSize(scrollPort.width, scrollPort.height);

    
    nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aMetrics.GetScrollId());
    bool scrollUpdated = false;
    CSSPoint actualScrollOffset = ScrollFrameTo(sf, aMetrics.GetScrollOffset(), scrollUpdated);

    if (scrollUpdated) {
        
        
        
        MaybeAlignAndClampDisplayPort(aMetrics, actualScrollOffset);
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
    if (!aMetrics.GetUseDisplayPortMargins()) {
        aUtils->SetDisplayPortForElement(aMetrics.mDisplayPort.x,
                                         aMetrics.mDisplayPort.y,
                                         aMetrics.mDisplayPort.width,
                                         aMetrics.mDisplayPort.height,
                                         element, 0);
    } else {
        gfx::IntSize alignment = gfxPrefs::LayersTilesEnabled()
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
        CSSRect baseCSS = aMetrics.mCompositionBounds / aMetrics.GetZoomToParent();
        nsRect base(baseCSS.x * nsPresContext::AppUnitsPerCSSPixel(),
                    baseCSS.y * nsPresContext::AppUnitsPerCSSPixel(),
                    baseCSS.width * nsPresContext::AppUnitsPerCSSPixel(),
                    baseCSS.height * nsPresContext::AppUnitsPerCSSPixel());
        nsLayoutUtils::SetDisplayPortBaseIfNotSet(content, base);
    }
}

void
APZCCallbackHelper::UpdateSubFrame(nsIContent* aContent,
                                   FrameMetrics& aMetrics)
{
    
    MOZ_ASSERT(aContent);
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
            MaybeAlignAndClampDisplayPort(aMetrics, actualScrollOffset);
        } else {
            RecenterDisplayPort(aMetrics);
        }
        if (!aMetrics.GetUseDisplayPortMargins()) {
            utils->SetDisplayPortForElement(aMetrics.mDisplayPort.x,
                                            aMetrics.mDisplayPort.y,
                                            aMetrics.mDisplayPort.width,
                                            aMetrics.mDisplayPort.height,
                                            element, 0);
        } else {
            gfx::IntSize alignment = gfxPrefs::LayersTilesEnabled()
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
            CSSRect baseCSS = aMetrics.mCompositionBounds / aMetrics.GetZoomToParent();
            nsRect base(baseCSS.x * nsPresContext::AppUnitsPerCSSPixel(),
                        baseCSS.y * nsPresContext::AppUnitsPerCSSPixel(),
                        baseCSS.width * nsPresContext::AppUnitsPerCSSPixel(),
                        baseCSS.height * nsPresContext::AppUnitsPerCSSPixel());
            nsLayoutUtils::SetDisplayPortBaseIfNotSet(aContent, base);
        }
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
            sf->ResetOriginIfScrollAtGeneration(mScrollGeneration);
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
