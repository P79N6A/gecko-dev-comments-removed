















#include <android/log.h>

#include "Framebuffer.h"
#include "HwcComposer2D.h"
#include "LayerManagerOGL.h"
#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include "mozilla/StaticPtr.h"
#include "cutils/properties.h"
#include "gfxUtils.h"

#define LOG_TAG "HWComposer"

#if (LOG_NDEBUG == 0)
#define LOGD(args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, ## args)
#else
#define LOGD(args...) ((void)0)
#endif

#define LOGE(args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ## args)

#define LAYER_COUNT_INCREMENTS 5

using namespace android;
using namespace mozilla::layers;

enum {
    HWC_USE_GPU = HWC_FRAMEBUFFER,
    HWC_USE_OVERLAY = HWC_OVERLAY,
    HWC_USE_COPYBIT
};


enum {
    
    
    
    HWC_COLOR_FILL = 0x8
};

namespace mozilla {

static StaticRefPtr<HwcComposer2D> sInstance;

HwcComposer2D::HwcComposer2D()
    : mMaxLayerCount(0)
    , mList(nullptr)
{
}

HwcComposer2D::~HwcComposer2D() {
    free(mList);
}

int
HwcComposer2D::Init(hwc_display_t dpy, hwc_surface_t sur)
{
    MOZ_ASSERT(!Initialized());

    if (int err = init()) {
        LOGE("Failed to initialize hwc");
        return err;
    }

    nsIntSize screenSize;

    mozilla::Framebuffer::GetSize(&screenSize);
    mScreenRect  = nsIntRect(nsIntPoint(0, 0), screenSize);

    char propValue[PROPERTY_VALUE_MAX];
    property_get("ro.display.colorfill", propValue, "0");
    mColorFill = (atoi(propValue) == 1) ? true : false;

    mDpy = dpy;
    mSur = sur;

    return 0;
}

HwcComposer2D*
HwcComposer2D::GetInstance()
{
    if (!sInstance) {
        LOGD("Creating new instance");
        sInstance = new HwcComposer2D();
    }
    return sInstance;
}

bool
HwcComposer2D::ReallocLayerList()
{
    int size = sizeof(hwc_layer_list_t) +
        ((mMaxLayerCount + LAYER_COUNT_INCREMENTS) * sizeof(hwc_layer_t));

    hwc_layer_list_t* listrealloc = (hwc_layer_list_t*)realloc(mList, size);

    if (!listrealloc) {
        return false;
    }

    if (!mList) {
        
        listrealloc->numHwLayers = 0;
        listrealloc->flags = 0;
    }

    mList = listrealloc;
    mMaxLayerCount += LAYER_COUNT_INCREMENTS;
    return true;
}



















static bool
PrepareLayerRects(nsIntRect aVisible, const gfxMatrix& aTransform,
                  nsIntRect aClip, nsIntRect aBufferRect,
                  hwc_rect_t* aSourceCrop, hwc_rect_t* aVisibleRegionScreen) {

    gfxRect visibleRect(aVisible);
    gfxRect clip(aClip);
    gfxRect visibleRectScreen = aTransform.TransformBounds(visibleRect);
    
    visibleRectScreen.IntersectRect(visibleRectScreen, clip);

    if (visibleRectScreen.IsEmpty()) {
        LOGD("Skip layer");
        return false;
    }

    gfxMatrix inverse(aTransform);
    inverse.Invert();
    gfxRect crop = inverse.TransformBounds(visibleRectScreen);

    
    crop.IntersectRect(crop, aBufferRect);
    crop.RoundOut();

    if (crop.IsEmpty()) {
        LOGD("Skip layer");
        return false;
    }

    
    visibleRectScreen = aTransform.TransformBounds(crop);
    visibleRectScreen.RoundOut();

    
    crop -= aBufferRect.TopLeft();

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
















static bool
CalculateClipRect(const gfxMatrix& aTransform, const nsIntRect* aLayerClip,
                  nsIntRect aParentClip, nsIntRect* aRenderClip) {

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

bool
HwcComposer2D::PrepareLayerList(Layer* aLayer,
                                const nsIntRect& aClip,
                                const gfxMatrix& aParentTransform,
                                const gfxMatrix& aGLWorldTransform)
{
    
    
    

    bool fillColor = false;

    const nsIntRegion& visibleRegion = aLayer->GetEffectiveVisibleRegion();
    if (visibleRegion.IsEmpty()) {
        return true;
    }

    float opacity = aLayer->GetEffectiveOpacity();
    if (opacity <= 0) {
        LOGD("Layer is fully transparent so skip rendering");
        return true;
    }
    else if (opacity < 1) {
        LOGD("Layer has planar semitransparency which is unsupported");
        return false;
    }

    if (visibleRegion.GetNumRects() > 1) {
        
        LOGD("Layer has nontrivial visible region");
        return false;
    }

    nsIntRect clip;
    if (!CalculateClipRect(aParentTransform * aGLWorldTransform,
                           aLayer->GetEffectiveClipRect(),
                           aClip,
                           &clip))
    {
        LOGD("Clip rect is empty. Skip layer");
        return true;
    }

    gfxMatrix transform;
    const gfx3DMatrix& transform3D = aLayer->GetEffectiveTransform();
    if (!transform3D.Is2D(&transform) || !transform.PreservesAxisAlignedRectangles()) {
        LOGD("Layer has a 3D transform or a non-square angle rotation");
        return false;
    }


    if (ContainerLayer* container = aLayer->AsContainerLayer()) {
        if (container->UseIntermediateSurface()) {
            LOGD("Container layer needs intermediate surface");
            return false;
        }
        nsAutoTArray<Layer*, 12> children;
        container->SortChildrenBy3DZOrder(children);

        for (uint32_t i = 0; i < children.Length(); i++) {
            if (!PrepareLayerList(children[i], clip, transform, aGLWorldTransform)) {
                return false;
            }
        }
        return true;
    }

    LayerOGL* layerGL = static_cast<LayerOGL*>(aLayer->ImplData());
    LayerRenderState state = layerGL->GetRenderState();

    if (!state.mSurface ||
        state.mSurface->type() != SurfaceDescriptor::TSurfaceDescriptorGralloc) {
        if (aLayer->AsColorLayer() && mColorFill) {
            fillColor = true;
        } else {
            LOGD("Layer doesn't have a gralloc buffer");
            return false;
        }
    }
    if (state.BufferRotated()) {
        LOGD("Layer has a rotated buffer");
        return false;
    }


    

    int current = mList ? mList->numHwLayers : 0;
    if (!mList || current >= mMaxLayerCount) {
        if (!ReallocLayerList() || current >= mMaxLayerCount) {
            LOGE("PrepareLayerList failed! Could not increase the maximum layer count");
            return false;
        }
    }

    sp<GraphicBuffer> buffer = fillColor ? nullptr : GrallocBufferActor::GetFrom(*state.mSurface);

    nsIntRect visibleRect = visibleRegion.GetBounds();

    nsIntRect bufferRect;
    if (fillColor) {
        bufferRect = nsIntRect(visibleRect);
    } else {
        if(state.mHasOwnOffset) {
            bufferRect = nsIntRect(state.mOffset.x, state.mOffset.y,
                int(buffer->getWidth()), int(buffer->getHeight()));
        } else {
            bufferRect = nsIntRect(visibleRect.x, visibleRect.y,
                int(buffer->getWidth()), int(buffer->getHeight()));
        }
    }

    hwc_layer_t& hwcLayer = mList->hwLayers[current];

    if(!PrepareLayerRects(visibleRect,
                          transform * aGLWorldTransform,
                          clip,
                          bufferRect,
                          &(hwcLayer.sourceCrop),
                          &(hwcLayer.displayFrame)))
    {
        return true;
    }

    buffer_handle_t handle = fillColor ? nullptr : buffer->getNativeBuffer()->handle;
    hwcLayer.handle = handle;

    hwcLayer.flags = 0;
    hwcLayer.hints = 0;
    hwcLayer.blending = HWC_BLENDING_NONE;
    hwcLayer.compositionType = HWC_USE_COPYBIT;

    if (!fillColor) {
        gfxMatrix rotation = transform * aGLWorldTransform;
        
        if (fabs(rotation.xx) < 1e-6) {
            if (rotation.xy < 0) {
                hwcLayer.transform = HWC_TRANSFORM_ROT_90;
                LOGD("Layer buffer rotated 90 degrees");
            } else {
                hwcLayer.transform = HWC_TRANSFORM_ROT_270;
                LOGD("Layer buffer rotated 270 degrees");
            }
        } else if (rotation.xx < 0) {
            hwcLayer.transform = HWC_TRANSFORM_ROT_180;
            LOGD("Layer buffer rotated 180 degrees");
        } else {
            hwcLayer.transform = 0;
        }

        hwcLayer.transform |= state.YFlipped() ? HWC_TRANSFORM_FLIP_V : 0;
        hwc_region_t region;
        region.numRects = 1;
        region.rects = &(hwcLayer.displayFrame);
        hwcLayer.visibleRegionScreen = region;
    } else {
        hwcLayer.flags |= HWC_COLOR_FILL;
        ColorLayer* colorLayer = static_cast<ColorLayer*>(layerGL->GetLayer());
        hwcLayer.transform = colorLayer->GetColor().Packed();
    }

    mList->numHwLayers++;
    return true;
}

bool
HwcComposer2D::TryRender(Layer* aRoot,
                         const gfxMatrix& aGLWorldTransform)
{
    if (!aGLWorldTransform.PreservesAxisAlignedRectangles()) {
        LOGD("Render aborted. World transform has non-square angle rotation");
        return false;
    }

    MOZ_ASSERT(Initialized());
    if (mList) {
        mList->numHwLayers = 0;
    }

    if (!PrepareLayerList(aRoot,
                          mScreenRect,
                          gfxMatrix(),
                          aGLWorldTransform))
    {
        LOGD("Render aborted. Nothing was drawn to the screen");
        return false;
    }

    if (mHwc->set(mHwc, mDpy, mSur, mList)) {
        LOGE("Hardware device failed to render");
        return false;
    }

    LOGD("Frame rendered");
    return true;
}

} 
