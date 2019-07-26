




#include "APZCCallbackHelper.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsIDOMElement.h"
#include "nsIInterfaceRequestorUtils.h"

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

void
APZCCallbackHelper::UpdateRootFrame(nsIDOMWindowUtils* aUtils,
                                    const FrameMetrics& aMetrics)
{
    
    MOZ_ASSERT(aUtils);
    if (aMetrics.mScrollId == FrameMetrics::NULL_SCROLL_ID) {
        return;
    }

    
    
    
    
    
    
    
    CSSSize scrollPort = aMetrics.CalculateCompositedRectInCssPixels().Size();
    aUtils->SetScrollPositionClampingScrollPortSize(scrollPort.width, scrollPort.height);

    
    aUtils->ScrollToCSSPixelsApproximate(aMetrics.mScrollOffset.x, aMetrics.mScrollOffset.y, nullptr);

    
    
    
    
    
    
    
    
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
                                   const FrameMetrics& aMetrics)
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
    if (sf) {
        sf->ScrollToCSSPixelsApproximate(aMetrics.mScrollOffset);
    }

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aContent);
    if (element) {
        utils->SetDisplayPortForElement(aMetrics.mDisplayPort.x,
                                        aMetrics.mDisplayPort.y,
                                        aMetrics.mDisplayPort.width,
                                        aMetrics.mDisplayPort.height,
                                        element);
    }
}

already_AddRefed<nsIDOMWindowUtils>
APZCCallbackHelper::GetDOMWindowUtils(nsIDocument* doc)
{
    nsCOMPtr<nsIDOMWindowUtils> utils;
    nsCOMPtr<nsIDOMWindow> window = doc->GetDefaultView();
    if (window) {
        utils = do_GetInterface(window);
    }
    return utils.forget();
}

already_AddRefed<nsIDOMWindowUtils>
APZCCallbackHelper::GetDOMWindowUtils(nsIContent* content)
{
    nsCOMPtr<nsIDOMWindowUtils> utils;
    nsIDocument* doc = content->GetCurrentDoc();
    if (doc) {
        utils = GetDOMWindowUtils(doc);
    }
    return utils.forget();
}

}
}
