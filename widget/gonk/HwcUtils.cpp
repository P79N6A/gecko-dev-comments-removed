















#include <android/log.h>
#include "HwcUtils.h"
#include "gfxUtils.h"
#include "gfx2DGlue.h"

#define LOG_TAG "HwcUtils"

#if (LOG_NDEBUG == 0)
#define LOGD(args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, ## args)
#else
#define LOGD(args...) ((void)0)
#endif

#define LOGE(args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ## args)


namespace mozilla {





 bool
HwcUtils::PrepareLayerRects(nsIntRect aVisible, const gfx::Matrix& transform,
                            nsIntRect aClip, nsIntRect aBufferRect,
                            bool aYFlipped,
                            hwc_rect_t* aSourceCrop, hwc_rect_t* aVisibleRegionScreen) {

    gfxMatrix aTransform = gfx::ThebesMatrix(transform);
    gfxRect visibleRect(aVisible);
    gfxRect clip(aClip);
    gfxRect visibleRectScreen = aTransform.TransformBounds(visibleRect);
    
    visibleRectScreen.IntersectRect(visibleRectScreen, clip);

    if (visibleRectScreen.IsEmpty()) {
        return false;
    }

    gfxMatrix inverse = aTransform;
    inverse.Invert();
    gfxRect crop = inverse.TransformBounds(visibleRectScreen);

    
    crop.IntersectRect(crop, aBufferRect);
    crop.Round();

    if (crop.IsEmpty()) {
        return false;
    }

    
    visibleRectScreen = aTransform.TransformBounds(crop);
    visibleRectScreen.Round();

    
    crop -= aBufferRect.TopLeft();
    if (aYFlipped) {
        crop.y = aBufferRect.height - (crop.y + crop.height);
    }

    aSourceCrop->left = crop.x;
    aSourceCrop->top  = crop.y;
    aSourceCrop->right  = crop.x + crop.width;
    aSourceCrop->bottom = crop.y + crop.height;

    aVisibleRegionScreen->left = visibleRectScreen.x;
    aVisibleRegionScreen->top  = visibleRectScreen.y;
    aVisibleRegionScreen->right  = visibleRectScreen.x + visibleRectScreen.width;
    aVisibleRegionScreen->bottom = visibleRectScreen.y + visibleRectScreen.height;

    return true;
}

 bool
HwcUtils::PrepareVisibleRegion(const nsIntRegion& aVisible,
                               const gfx::Matrix& transform,
                               nsIntRect aClip, nsIntRect aBufferRect,
                               RectVector* aVisibleRegionScreen) {

    gfxMatrix aTransform = gfx::ThebesMatrix(transform);
    nsIntRegionRectIterator rect(aVisible);
    bool isVisible = false;
    while (const nsIntRect* visibleRect = rect.Next()) {
        hwc_rect_t visibleRectScreen;
        gfxRect screenRect;

        screenRect.IntersectRect(gfxRect(*visibleRect), aBufferRect);
        screenRect = aTransform.TransformBounds(screenRect);
        screenRect.IntersectRect(screenRect, aClip);
        screenRect.Round();
        if (screenRect.IsEmpty()) {
            continue;
        }
        visibleRectScreen.left = screenRect.x;
        visibleRectScreen.top  = screenRect.y;
        visibleRectScreen.right  = screenRect.XMost();
        visibleRectScreen.bottom = screenRect.YMost();
        aVisibleRegionScreen->push_back(visibleRectScreen);
        isVisible = true;
    }

    return isVisible;
}

 bool
HwcUtils::CalculateClipRect(const gfx::Matrix& transform,
                            const nsIntRect* aLayerClip,
                            nsIntRect aParentClip, nsIntRect* aRenderClip) {

    gfxMatrix aTransform = gfx::ThebesMatrix(transform);
    *aRenderClip = aParentClip;

    if (!aLayerClip) {
        return true;
    }

    if (aLayerClip->IsEmpty()) {
        return false;
    }

    nsIntRect clip = *aLayerClip;

    gfxRect r(clip);
    gfxRect trClip = aTransform.TransformBounds(r);
    trClip.Round();
    gfxUtils::GfxRectToIntRect(trClip, &clip);

    aRenderClip->IntersectRect(*aRenderClip, clip);
    return true;
}

nsIntRect
HwcUtils::HwcToIntRect(hwc_rect_t aRect) {
    return nsIntRect(aRect.left, aRect.top, aRect.right - aRect.left,
                aRect.bottom - aRect.top);
}

} 
