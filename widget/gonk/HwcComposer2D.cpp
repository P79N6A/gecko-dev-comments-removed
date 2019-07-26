















#include <android/log.h>
#include <string.h>

#include "libdisplay/GonkDisplay.h"
#include "Framebuffer.h"
#include "HwcUtils.h"
#include "HwcComposer2D.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/PLayerTransaction.h"
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include "mozilla/StaticPtr.h"
#include "cutils/properties.h"

#if ANDROID_VERSION >= 18
#include "libdisplay/FramebufferSurface.h"
#endif

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

namespace mozilla {

static StaticRefPtr<HwcComposer2D> sInstance;

HwcComposer2D::HwcComposer2D()
    : mMaxLayerCount(0)
    , mList(nullptr)
    , mHwc(nullptr)
{
}

HwcComposer2D::~HwcComposer2D() {
    free(mList);
}

int
HwcComposer2D::Init(hwc_display_t dpy, hwc_surface_t sur)
{
    MOZ_ASSERT(!Initialized());

    mHwc = (HwcDevice*)GetGonkDisplay()->GetHWCDevice();
    if (!mHwc) {
        LOGE("Failed to initialize hwc");
        return -1;
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
    int size = sizeof(HwcList) +
        ((mMaxLayerCount + LAYER_COUNT_INCREMENTS) * sizeof(HwcLayer));

    HwcList* listrealloc = (HwcList*)realloc(mList, size);

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
    if (opacity < 1) {
        LOGD("%s Layer has planar semitransparency which is unsupported", aLayer->Name());
        return false;
    }

    nsIntRect clip;
    if (!HwcUtils::CalculateClipRect(aParentTransform * aGLWorldTransform,
                                     aLayer->GetEffectiveClipRect(),
                                     aClip,
                                     &clip))
    {
        LOGD("%s Clip rect is empty. Skip layer", aLayer->Name());
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

    LayerRenderState state = aLayer->GetRenderState();
    nsIntSize surfaceSize;

    if (state.mSurface.get()) {
        surfaceSize = state.mSize;
    } else {
        if (aLayer->AsColorLayer() && mColorFill) {
            fillColor = true;
        } else {
            LOGD("%s Layer doesn't have a gralloc buffer", aLayer->Name());
            return false;
        }
    }
    
    
    if (state.BufferRotated()) {
        LOGD("%s Layer has a rotated buffer", aLayer->Name());
        return false;
    }


    

    int current = mList ? mList->numHwLayers : 0;
    if (!mList || current >= mMaxLayerCount) {
        if (!ReallocLayerList() || current >= mMaxLayerCount) {
            LOGE("PrepareLayerList failed! Could not increase the maximum layer count");
            return false;
        }
    }

    nsIntRect visibleRect = visibleRegion.GetBounds();

    nsIntRect bufferRect;
    if (fillColor) {
        bufferRect = nsIntRect(visibleRect);
    } else {
        if(state.mHasOwnOffset) {
            bufferRect = nsIntRect(state.mOffset.x, state.mOffset.y,
                                   state.mSize.width, state.mSize.height);
        } else {
            
            
            bufferRect = nsIntRect(0, 0, state.mSize.width, state.mSize.height);
        }
    }

    HwcLayer& hwcLayer = mList->hwLayers[current];

    if(!HwcUtils::PrepareLayerRects(visibleRect,
                          transform * aGLWorldTransform,
                          clip,
                          bufferRect,
                          &(hwcLayer.sourceCrop),
                          &(hwcLayer.displayFrame)))
    {
        return true;
    }

    buffer_handle_t handle = fillColor ? nullptr : state.mSurface->getNativeBuffer()->handle;
    hwcLayer.handle = handle;

    hwcLayer.flags = 0;
    hwcLayer.hints = 0;
    hwcLayer.blending = HWC_BLENDING_PREMULT;
#if ANDROID_VERSION >= 18
    hwcLayer.compositionType = HWC_FRAMEBUFFER;

    hwcLayer.acquireFenceFd = -1;
    hwcLayer.releaseFenceFd = -1;
    hwcLayer.planeAlpha = 0xFF; 
#else
    hwcLayer.compositionType = HwcUtils::HWC_USE_COPYBIT;
#endif

    if (!fillColor) {
        if (state.FormatRBSwapped()) {
            hwcLayer.flags |= HwcUtils::HWC_FORMAT_RB_SWAP;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        gfxMatrix rotation = transform * aGLWorldTransform;
        
        if (fabs(rotation.xx) < 1e-6) {
            if (rotation.xy < 0) {
                if (rotation.yx > 0) {
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_90;
                    LOGD("Layer rotated 90 degrees");
                }
                else {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_H;
                    LOGD("Layer vertically reflected then rotated 270 degrees");
                }
            } else {
                if (rotation.yx < 0) {
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_270;
                    LOGD("Layer rotated 270 degrees");
                }
                else {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_V;
                    LOGD("Layer horizontally reflected then rotated 270 degrees");
                }
            }
        } else if (rotation.xx < 0) {
            if (rotation.yy > 0) {
                
                
                
                
                
                hwcLayer.transform = HWC_TRANSFORM_FLIP_H;
                LOGD("Layer rotated 180 degrees");
            }
            else {
                
                
                
                
                
                
                
                
                
                
                hwcLayer.transform = HWC_TRANSFORM_ROT_180;
                LOGD("Layer rotated 180 degrees");
            }
        } else {
            if (rotation.yy < 0) {
                
                
                
                
                
                hwcLayer.transform = HWC_TRANSFORM_FLIP_V;
                LOGD("Layer rotated 180 degrees");
            }
            else {
                
                
                
                
                
                hwcLayer.transform = 0;
            }
        }

        if (state.YFlipped()) {
           
           hwcLayer.transform ^= HWC_TRANSFORM_FLIP_V;
        }
        hwc_region_t region;
        if (visibleRegion.GetNumRects() > 1) {
            mVisibleRegions.push_back(HwcUtils::RectVector());
            HwcUtils::RectVector* visibleRects = &(mVisibleRegions.back());
            if(!HwcUtils::PrepareVisibleRegion(visibleRegion,
                                     transform * aGLWorldTransform,
                                     clip,
                                     bufferRect,
                                     visibleRects)) {
                return true;
            }
            region.numRects = visibleRects->size();
            region.rects = &((*visibleRects)[0]);
        } else {
            region.numRects = 1;
            region.rects = &(hwcLayer.displayFrame);
        }
        hwcLayer.visibleRegionScreen = region;
    } else {
        hwcLayer.flags |= HwcUtils::HWC_COLOR_FILL;
        ColorLayer* colorLayer = aLayer->AsColorLayer();
        if (colorLayer->GetColor().a < 1.0) {
            LOGD("Color layer has semitransparency which is unsupported");
            return false;
        }
        hwcLayer.transform = colorLayer->GetColor().Packed();
    }

    mList->numHwLayers++;
    return true;
}


#if ANDROID_VERSION >= 18
bool
HwcComposer2D::TryHwComposition()
{
    FramebufferSurface* fbsurface = (FramebufferSurface*)(GetGonkDisplay()->GetFBSurface());

    if (!fbsurface) {
        LOGE("H/W Composition failed. FBSurface not initialized.");
        return false;
    }

    hwc_display_contents_1_t *displays[HWC_NUM_DISPLAY_TYPES] = {NULL};
    const hwc_rect_t r = {0, 0, mScreenRect.width, mScreenRect.height};
    int idx = mList->numHwLayers;

    displays[HWC_DISPLAY_PRIMARY] = mList;
    mList->flags = HWC_GEOMETRY_CHANGED;
    mList->retireFenceFd = -1;

    mList->hwLayers[idx].hints = 0;
    mList->hwLayers[idx].flags = 0;
    mList->hwLayers[idx].transform = 0;
    mList->hwLayers[idx].handle = fbsurface->lastHandle;
    mList->hwLayers[idx].blending = HWC_BLENDING_PREMULT;
    mList->hwLayers[idx].compositionType = HWC_FRAMEBUFFER_TARGET;
    mList->hwLayers[idx].sourceCrop = r;
    mList->hwLayers[idx].displayFrame = r;
    mList->hwLayers[idx].visibleRegionScreen.numRects = 1;
    mList->hwLayers[idx].visibleRegionScreen.rects = &mList->hwLayers[idx].sourceCrop;
    mList->hwLayers[idx].acquireFenceFd = -1;
    mList->hwLayers[idx].releaseFenceFd = -1;
    mList->hwLayers[idx].planeAlpha = 0xFF;
    mList->numHwLayers++;

    mHwc->prepare(mHwc, HWC_NUM_DISPLAY_TYPES, displays);

    for (int j = 0; j < idx; j++) {
        if (mList->hwLayers[j].compositionType == HWC_FRAMEBUFFER) {
            LOGD("GPU or Partial MDP Composition");
            return false;
        }
    }

    
    mHwc->set(mHwc, HWC_NUM_DISPLAY_TYPES, displays);

    for (int i = 0; i <= MAX_HWC_LAYERS; i++) {
        if (mPrevRelFd[i] <= 0) {
            break;
        }
        if (!i) {
            
            
            
            
            sp<Fence> fence = new Fence(mPrevRelFd[i]);
            if (fence->wait(1000) == -ETIME) {
                LOGE("Wait timed-out for retireFenceFd %d", mPrevRelFd[i]);
            }
        }
        close(mPrevRelFd[i]);
        mPrevRelFd[i] = -1;
    }

    mPrevRelFd[0] = mList->retireFenceFd;
    for (uint32_t j = 0; j < idx; j++) {
        if (mList->hwLayers[j].compositionType == HWC_OVERLAY) {
            mPrevRelFd[j + 1] = mList->hwLayers[j].releaseFenceFd;
            mList->hwLayers[j].releaseFenceFd = -1;
        }
    }

    close(mList->hwLayers[idx].releaseFenceFd);
    mList->hwLayers[idx].releaseFenceFd = -1;
    mList->retireFenceFd = -1;
    mList->numHwLayers = 0;
    return true;
}
#else
bool
HwcComposer2D::TryHwComposition()
{
    return !mHwc->set(mHwc, mDpy, mSur, mList);
}
#endif

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

    
    
    mVisibleRegions.clear();

    if (!PrepareLayerList(aRoot,
                          mScreenRect,
                          gfxMatrix(),
                          aGLWorldTransform))
    {
        LOGD("Render aborted. Nothing was drawn to the screen");
        return false;
    }

    if (!TryHwComposition()) {
      
      LOGE("H/W Composition failed");
      return false;
    }

    LOGD("Frame rendered");
    return true;
}

} 
