




#include "APZCCallbackHelper.h"
#include "gfxPrefs.h" 
#include "mozilla/Preferences.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsIDOMElement.h"
#include "nsIInterfaceRequestorUtils.h"
#include "TiledLayerBuffer.h" 

namespace mozilla {
namespace widget {

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

  
  gfxFloat left = TILEDLAYERBUFFER_TILE_SIZE
    * floor(displayPortInLayerSpace.x / TILEDLAYERBUFFER_TILE_SIZE);
  gfxFloat top = TILEDLAYERBUFFER_TILE_SIZE
    * floor(displayPortInLayerSpace.y / TILEDLAYERBUFFER_TILE_SIZE);
  gfxFloat right = TILEDLAYERBUFFER_TILE_SIZE
    * ceil(displayPortInLayerSpace.XMost() / TILEDLAYERBUFFER_TILE_SIZE);
  gfxFloat bottom = TILEDLAYERBUFFER_TILE_SIZE
    * ceil(displayPortInLayerSpace.YMost() / TILEDLAYERBUFFER_TILE_SIZE);

  displayPortInLayerSpace = LayerRect(left, top, right - left, bottom - top);
  CSSRect displayPort = displayPortInLayerSpace / aLayerPixelsPerCSSPixel;

  return displayPort;
}

static void
MaybeAlignAndClampDisplayPort(mozilla::layers::FrameMetrics& aFrameMetrics,
                              const CSSPoint& aActualScrollOffset)
{
  
  
  CSSRect& displayPort = aFrameMetrics.mDisplayPort;
  displayPort += aFrameMetrics.mScrollOffset - aActualScrollOffset;

  
  
  if (gfxPrefs::LayersTilesEnabled()) {
    
    
    displayPort =
      ExpandDisplayPortToTileBoundaries(displayPort + aActualScrollOffset,
                                        aFrameMetrics.mZoom *
                                        ScreenToLayerScale(1.0))
      - aActualScrollOffset;
  }

  
  CSSRect scrollableRect = aFrameMetrics.GetExpandedScrollableRect();
  displayPort = scrollableRect.Intersect(displayPort + aActualScrollOffset)
    - aActualScrollOffset;
}

static void
RecenterDisplayPort(mozilla::layers::FrameMetrics& aFrameMetrics)
{
    CSSRect compositionBounds = aFrameMetrics.CalculateCompositedRectInCssPixels();
    aFrameMetrics.mDisplayPort.x = (compositionBounds.width - aFrameMetrics.mDisplayPort.width) / 2;
    aFrameMetrics.mDisplayPort.y = (compositionBounds.height - aFrameMetrics.mDisplayPort.height) / 2;
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
    if (aMetrics.mScrollId == FrameMetrics::NULL_SCROLL_ID) {
        return;
    }

    
    
    
    
    
    
    
    CSSSize scrollPort = aMetrics.CalculateCompositedRectInCssPixels().Size();
    aUtils->SetScrollPositionClampingScrollPortSize(scrollPort.width, scrollPort.height);

    
    nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aMetrics.mScrollId);
    bool scrollUpdated = false;
    CSSPoint actualScrollOffset = ScrollFrameTo(sf, aMetrics.mScrollOffset, scrollUpdated);

    if (!scrollUpdated) {
      
      
      
      
      
      
      RecenterDisplayPort(aMetrics);
    }

    
    
    
    MaybeAlignAndClampDisplayPort(aMetrics, actualScrollOffset);

    aMetrics.mScrollOffset = actualScrollOffset;

    
    
    
    
    
    
    
    
    mozilla::layers::ParentLayerToLayerScale presShellResolution =
        aMetrics.mZoom
        / aMetrics.mDevPixelsPerCSSPixel
        / aMetrics.GetParentResolution()
        * ScreenToLayerScale(1.0f);
    aUtils->SetResolution(presShellResolution.scale, presShellResolution.scale);

    
    nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(aMetrics.mScrollId);
    if (!content) {
        return;
    }
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(content);
    if (!element) {
        return;
    }
    aUtils->SetDisplayPortForElement(aMetrics.mDisplayPort.x,
                                     aMetrics.mDisplayPort.y,
                                     aMetrics.mDisplayPort.width,
                                     aMetrics.mDisplayPort.height,
                                     element);
}

void
APZCCallbackHelper::UpdateSubFrame(nsIContent* aContent,
                                   FrameMetrics& aMetrics)
{
    
    MOZ_ASSERT(aContent);
    if (aMetrics.mScrollId == FrameMetrics::NULL_SCROLL_ID) {
        return;
    }

    nsCOMPtr<nsIDOMWindowUtils> utils = GetDOMWindowUtils(aContent);
    if (!utils) {
        return;
    }

    
    

    nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aMetrics.mScrollId);
    bool scrollUpdated = false;
    CSSPoint actualScrollOffset = ScrollFrameTo(sf, aMetrics.mScrollOffset, scrollUpdated);

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aContent);
    if (element) {
        if (!scrollUpdated) {
            RecenterDisplayPort(aMetrics);
        }
        MaybeAlignAndClampDisplayPort(aMetrics, actualScrollOffset);
        utils->SetDisplayPortForElement(aMetrics.mDisplayPort.x,
                                        aMetrics.mDisplayPort.y,
                                        aMetrics.mDisplayPort.width,
                                        aMetrics.mDisplayPort.height,
                                        element);
    }

    aMetrics.mScrollOffset = actualScrollOffset;
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
APZCCallbackHelper::GetScrollIdentifiers(const nsIContent* aContent,
                                         uint32_t* aPresShellIdOut,
                                         FrameMetrics::ViewID* aViewIdOut)
{
    if (!aContent || !nsLayoutUtils::FindIDFor(aContent, aViewIdOut)) {
        return false;
    }
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

}
}
