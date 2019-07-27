















#include <android/log.h>
#include <string.h>

#include "ImageLayers.h"
#include "libdisplay/GonkDisplay.h"
#include "HwcComposer2D.h"
#include "HwcUtils.h"
#include "LayerScope.h"
#include "Units.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/PLayerTransaction.h"
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include "mozilla/layers/TextureHostOGL.h"  
#include "mozilla/StaticPtr.h"
#include "cutils/properties.h"
#include "gfx2DGlue.h"
#include "gfxPlatform.h"
#include "VsyncSource.h"

#if ANDROID_VERSION >= 17
#include "libdisplay/DisplaySurface.h"
#include "gfxPrefs.h"
#include "nsThreadUtils.h"
#endif

#if ANDROID_VERSION >= 21
#ifndef HWC_BLIT
#define HWC_BLIT 0xFF
#endif
#elif ANDROID_VERSION >= 17
#ifndef HWC_BLIT
#define HWC_BLIT (HWC_FRAMEBUFFER_TARGET + 1)
#endif
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "HWComposer"







#ifdef HWC_DEBUG
#define LOGD(args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, ## args)
#else
#define LOGD(args...) ((void)0)
#endif

#define LOGI(args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ## args)
#define LOGE(args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ## args)

#define LAYER_COUNT_INCREMENTS 5

using namespace android;
using namespace mozilla::gfx;
using namespace mozilla::layers;

namespace mozilla {

#if ANDROID_VERSION >= 17
static void
HookInvalidate(const struct hwc_procs* aProcs)
{
    HwcComposer2D::GetInstance()->Invalidate();
}

static void
HookVsync(const struct hwc_procs* aProcs, int aDisplay,
          int64_t aTimestamp)
{
    HwcComposer2D::GetInstance()->Vsync(aDisplay, aTimestamp);
}

static void
HookHotplug(const struct hwc_procs* aProcs, int aDisplay,
            int aConnected)
{
    
}

static const hwc_procs_t sHWCProcs = {
    &HookInvalidate, 
    &HookVsync,      
    &HookHotplug     
};
#endif

static StaticRefPtr<HwcComposer2D> sInstance;

HwcComposer2D::HwcComposer2D()
    : mHwc(nullptr)
    , mList(nullptr)
    , mDpy(EGL_NO_DISPLAY)
    , mSur(EGL_NO_SURFACE)
    , mGLContext(nullptr)
    , mMaxLayerCount(0)
    , mColorFill(false)
    , mRBSwapSupport(false)
    , mPrepared(false)
    , mHasHWVsync(false)
    , mLock("mozilla.HwcComposer2D.mLock")
{
#if ANDROID_VERSION >= 17
    RegisterHwcEventCallback();
#endif

    mHwc = (HwcDevice*)GetGonkDisplay()->GetHWCDevice();
    if (!mHwc) {
        LOGD("no hwc support");
        return;
    }

    nsIntSize screenSize;

    ANativeWindow *win = GetGonkDisplay()->GetNativeWindow();
    win->query(win, NATIVE_WINDOW_WIDTH, &screenSize.width);
    win->query(win, NATIVE_WINDOW_HEIGHT, &screenSize.height);
    mScreenRect = gfx::IntRect(gfx::IntPoint(0, 0), screenSize);

#if ANDROID_VERSION >= 17
    int supported = 0;

    if (mHwc->query) {
        if (mHwc->query(mHwc, HwcUtils::HWC_COLOR_FILL, &supported) == NO_ERROR) {
            mColorFill = !!supported;
        }
        if (mHwc->query(mHwc, HwcUtils::HWC_FORMAT_RB_SWAP, &supported) == NO_ERROR) {
            mRBSwapSupport = !!supported;
        }
    } else {
        mColorFill = false;
        mRBSwapSupport = false;
    }
#else
    char propValue[PROPERTY_VALUE_MAX];
    property_get("ro.display.colorfill", propValue, "0");
    mColorFill = (atoi(propValue) == 1) ? true : false;
    mRBSwapSupport = true;
#endif
}

HwcComposer2D::~HwcComposer2D() {
    free(mList);
}

HwcComposer2D*
HwcComposer2D::GetInstance()
{
    if (!sInstance) {
        LOGI("Creating new instance");
        sInstance = new HwcComposer2D();
    }
    return sInstance;
}

bool
HwcComposer2D::EnableVsync(bool aEnable)
{
    
    
#if (ANDROID_VERSION == 19 || ANDROID_VERSION >= 21)
    MOZ_ASSERT(NS_IsMainThread());
    if (!mHasHWVsync) {
      return false;
    }

    HwcDevice* device = (HwcDevice*)GetGonkDisplay()->GetHWCDevice();
    if (!device) {
      return false;
    }

    return !device->eventControl(device, HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, aEnable) && aEnable;
#else
    return false;
#endif
}

#if ANDROID_VERSION >= 17
bool
HwcComposer2D::RegisterHwcEventCallback()
{
    HwcDevice* device = (HwcDevice*)GetGonkDisplay()->GetHWCDevice();
    if (!device || !device->registerProcs) {
        LOGE("Failed to get hwc");
        return false;
    }

    
    device->eventControl(device, HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, false);
    device->registerProcs(device, &sHWCProcs);

    
    
#if (ANDROID_VERSION == 19 || ANDROID_VERSION >= 21)
    mHasHWVsync = gfxPrefs::HardwareVsyncEnabled();
#else
    mHasHWVsync = false;
#endif
    return mHasHWVsync;
}

void
HwcComposer2D::Vsync(int aDisplay, nsecs_t aVsyncTimestamp)
{
    
    
#if (ANDROID_VERSION == 19 || ANDROID_VERSION >= 21)
    TimeStamp vsyncTime = mozilla::TimeStamp::FromSystemTime(aVsyncTimestamp);
    gfxPlatform::GetPlatform()->GetHardwareVsync()->GetGlobalDisplay().NotifyVsync(vsyncTime);
#else
    
    MOZ_ASSERT(false);
#endif
}


void
HwcComposer2D::Invalidate()
{
    if (!mHwc) {
        LOGE("HwcComposer2D::Invalidate failed!");
        return;
    }

    MutexAutoLock lock(mLock);
    if (mCompositorParent) {
        mCompositorParent->ScheduleRenderOnCompositorThread();
    }
}
#endif

void
HwcComposer2D::SetCompositorParent(CompositorParent* aCompositorParent)
{
    MutexAutoLock lock(mLock);
    mCompositorParent = aCompositorParent;
}

void
HwcComposer2D::SetEGLInfo(hwc_display_t aDisplay, hwc_surface_t aSurface, gl::GLContext* aGLContext)
{
    mDpy = aDisplay;
    mSur = aSurface;
    mGLContext = aGLContext;
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

void
HwcComposer2D::setCrop(HwcLayer* layer, hwc_rect_t srcCrop)
{
#if ANDROID_VERSION >= 19
    if (mHwc->common.version >= HWC_DEVICE_API_VERSION_1_3) {
        layer->sourceCropf.left = srcCrop.left;
        layer->sourceCropf.top = srcCrop.top;
        layer->sourceCropf.right = srcCrop.right;
        layer->sourceCropf.bottom = srcCrop.bottom;
    } else {
        layer->sourceCrop = srcCrop;
    }
#else
    layer->sourceCrop = srcCrop;
#endif
}

void
HwcComposer2D::setHwcGeometry(bool aGeometryChanged)
{
#if ANDROID_VERSION >= 19
    mList->flags = aGeometryChanged ? HWC_GEOMETRY_CHANGED : 0;
#else
    mList->flags = HWC_GEOMETRY_CHANGED;
#endif
}

bool
HwcComposer2D::PrepareLayerList(Layer* aLayer,
                                const nsIntRect& aClip,
                                const Matrix& aParentTransform)
{
    
    
    

    bool fillColor = false;

    const nsIntRegion& visibleRegion = aLayer->GetEffectiveVisibleRegion();
    if (visibleRegion.IsEmpty()) {
        return true;
    }

    uint8_t opacity = std::min(0xFF, (int)(aLayer->GetEffectiveOpacity() * 256.0));
    if (opacity == 0) {
        LOGD("%s Layer has zero opacity; skipping", aLayer->Name());
        return true;
    }
#if ANDROID_VERSION < 18
    if (opacity < 0xFF) {
        LOGD("%s Layer has planar semitransparency which is unsupported by hwcomposer", aLayer->Name());
        return false;
    }
#endif

    if (aLayer->GetMaskLayer()) {
      LOGD("%s Layer has MaskLayer which is unsupported by hwcomposer", aLayer->Name());
      return false;
    }

    nsIntRect clip;
    nsIntRect layerClip = aLayer->GetEffectiveClipRect() ?
                          ParentLayerIntRect::ToUntyped(*aLayer->GetEffectiveClipRect()) : nsIntRect();
    nsIntRect* layerClipPtr = aLayer->GetEffectiveClipRect() ? &layerClip : nullptr;
    if (!HwcUtils::CalculateClipRect(aParentTransform,
                                     layerClipPtr,
                                     aClip,
                                     &clip))
    {
        LOGD("%s Clip rect is empty. Skip layer", aLayer->Name());
        return true;
    }

    
    
    
    
    
    
    
    
    
    Matrix layerTransform;
    if (!aLayer->GetEffectiveTransform().Is2D(&layerTransform) ||
        !layerTransform.PreservesAxisAlignedRectangles()) {
        LOGD("Layer EffectiveTransform has a 3D transform or a non-square angle rotation");
        return false;
    }

    Matrix layerBufferTransform;
    if (!aLayer->GetEffectiveTransformForBuffer().Is2D(&layerBufferTransform) ||
        !layerBufferTransform.PreservesAxisAlignedRectangles()) {
        LOGD("Layer EffectiveTransformForBuffer has a 3D transform or a non-square angle rotation");
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
            if (!PrepareLayerList(children[i], clip, layerTransform)) {
                return false;
            }
        }
        return true;
    }

    LayerRenderState state = aLayer->GetRenderState();

    if (!state.mSurface.get()) {
      if (aLayer->AsColorLayer() && mColorFill) {
        fillColor = true;
      } else {
          LOGD("%s Layer doesn't have a gralloc buffer", aLayer->Name());
          return false;
      }
    }

    nsIntRect visibleRect = visibleRegion.GetBounds();

    nsIntRect bufferRect;
    if (fillColor) {
        bufferRect = nsIntRect(visibleRect);
    } else {
        nsIntRect layerRect;
        if (state.mHasOwnOffset) {
            bufferRect = nsIntRect(state.mOffset.x, state.mOffset.y,
                                   state.mSize.width, state.mSize.height);
            layerRect = bufferRect;
        } else {
            
            
            bufferRect = nsIntRect(0, 0, state.mSize.width, state.mSize.height);
            layerRect = bufferRect;
            if (aLayer->GetType() == Layer::TYPE_IMAGE) {
                ImageLayer* imageLayer = static_cast<ImageLayer*>(aLayer);
                if(imageLayer->GetScaleMode() != ScaleMode::SCALE_NONE) {
                  layerRect = nsIntRect(0, 0, imageLayer->GetScaleToSize().width, imageLayer->GetScaleToSize().height);
                }
            }
        }
        
        
        
        visibleRect.IntersectRect(visibleRect, layerRect);
    }

    
    
    if (state.BufferRotated()) {
        LOGD("%s Layer has a rotated buffer", aLayer->Name());
        return false;
    }

    const bool needsYFlip = state.OriginBottomLeft() ? true
                                                     : false;

    hwc_rect_t sourceCrop, displayFrame;
    if(!HwcUtils::PrepareLayerRects(visibleRect,
                          layerTransform,
                          layerBufferTransform,
                          clip,
                          bufferRect,
                          needsYFlip,
                          &(sourceCrop),
                          &(displayFrame)))
    {
        return true;
    }

    
    int current = mList ? mList->numHwLayers : 0;

    
    
    bool isOpaque = opacity == 0xFF &&
        (state.mFlags & LayerRenderStateFlags::OPAQUE);
    
    
    bool isSimpleVisibleRegion = visibleRegion.Contains(visibleRect);
    if (current && isOpaque && isSimpleVisibleRegion) {
        nsIntRect displayRect = nsIntRect(displayFrame.left, displayFrame.top,
            displayFrame.right - displayFrame.left, displayFrame.bottom - displayFrame.top);
        if (displayRect.Contains(mScreenRect)) {
            
            
            mList->numHwLayers = current = 0;
            mHwcLayerMap.Clear();
        }
    }

    if (!mList || current >= mMaxLayerCount) {
        if (!ReallocLayerList() || current >= mMaxLayerCount) {
            LOGE("PrepareLayerList failed! Could not increase the maximum layer count");
            return false;
        }
    }

    HwcLayer& hwcLayer = mList->hwLayers[current];
    hwcLayer.displayFrame = displayFrame;
    setCrop(&hwcLayer, sourceCrop);
    buffer_handle_t handle = fillColor ? nullptr : state.mSurface->getNativeBuffer()->handle;
    hwcLayer.handle = handle;

    hwcLayer.flags = 0;
    hwcLayer.hints = 0;
    hwcLayer.blending = isOpaque ? HWC_BLENDING_NONE : HWC_BLENDING_PREMULT;
#if ANDROID_VERSION >= 17
    hwcLayer.compositionType = HWC_FRAMEBUFFER;

    hwcLayer.acquireFenceFd = -1;
    hwcLayer.releaseFenceFd = -1;
#if ANDROID_VERSION >= 18
    hwcLayer.planeAlpha = opacity;
#endif
#else
    hwcLayer.compositionType = HwcUtils::HWC_USE_COPYBIT;
#endif

    if (!fillColor) {
        if (state.FormatRBSwapped()) {
            if (!mRBSwapSupport) {
                LOGD("No R/B swap support in H/W Composer");
                return false;
            }
            hwcLayer.flags |= HwcUtils::HWC_FORMAT_RB_SWAP;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        gfx::Matrix rotation = layerTransform;
        
        if (fabs(rotation._11) < 1e-6) {
            if (rotation._21 < 0) {
                if (rotation._12 > 0) {
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_90;
                    LOGD("Layer rotated 90 degrees");
                }
                else {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_H;
                    LOGD("Layer vertically reflected then rotated 270 degrees");
                }
            } else {
                if (rotation._12 < 0) {
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_270;
                    LOGD("Layer rotated 270 degrees");
                }
                else {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    hwcLayer.transform = HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_V;
                    LOGD("Layer horizontally reflected then rotated 270 degrees");
                }
            }
        } else if (rotation._11 < 0) {
            if (rotation._22 > 0) {
                
                
                
                
                
                hwcLayer.transform = HWC_TRANSFORM_FLIP_H;
                LOGD("Layer rotated 180 degrees");
            }
            else {
                
                
                
                
                
                
                
                
                
                
                hwcLayer.transform = HWC_TRANSFORM_ROT_180;
                LOGD("Layer rotated 180 degrees");
            }
        } else {
            if (rotation._22 < 0) {
                
                
                
                
                
                hwcLayer.transform = HWC_TRANSFORM_FLIP_V;
                LOGD("Layer rotated 180 degrees");
            }
            else {
                
                
                
                
                
                hwcLayer.transform = 0;
            }
        }

        const bool needsYFlip = state.OriginBottomLeft() ? true
                                                         : false;

        if (needsYFlip) {
           
           hwcLayer.transform ^= HWC_TRANSFORM_FLIP_V;
        }
        hwc_region_t region;
        if (visibleRegion.GetNumRects() > 1) {
            mVisibleRegions.push_back(HwcUtils::RectVector());
            HwcUtils::RectVector* visibleRects = &(mVisibleRegions.back());
            bool isVisible = false;
            if(!HwcUtils::PrepareVisibleRegion(visibleRegion,
                                     layerTransform,
                                     layerBufferTransform,
                                     clip,
                                     bufferRect,
                                     visibleRects,
                                     isVisible)) {
                LOGD("A region of layer is too small to be rendered by HWC");
                return false;
            }
            if (!isVisible) {
                
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

    mHwcLayerMap.AppendElement(static_cast<LayerComposite*>(aLayer->ImplData()));
    mList->numHwLayers++;
    return true;
}


#if ANDROID_VERSION >= 17
bool
HwcComposer2D::TryHwComposition()
{
    DisplaySurface* dispSurface = (DisplaySurface*)(GetGonkDisplay()->GetDispSurface());

    if (!(dispSurface && dispSurface->lastHandle)) {
        LOGD("H/W Composition failed. DispSurface not initialized.");
        return false;
    }

    
    int idx = mList->numHwLayers++;
    if (idx >= mMaxLayerCount) {
        if (!ReallocLayerList() || idx >= mMaxLayerCount) {
            LOGE("TryHwComposition failed! Could not add FB layer");
            return false;
        }
    }

    Prepare(dispSurface->lastHandle, -1);

    





    bool gpuComposite = false;
    bool blitComposite = false;
    bool overlayComposite = true;

    for (int j=0; j < idx; j++) {
        if (mList->hwLayers[j].compositionType == HWC_FRAMEBUFFER ||
            mList->hwLayers[j].compositionType == HWC_BLIT) {
            
            
            overlayComposite = false;
            break;
        }
    }

    if (!overlayComposite) {
        for (int k=0; k < idx; k++) {
            switch (mList->hwLayers[k].compositionType) {
                case HWC_FRAMEBUFFER:
                    gpuComposite = true;
                    break;
                case HWC_BLIT:
                    blitComposite = true;
                    break;
                case HWC_OVERLAY:
                    
                    
                    
                    mHwcLayerMap[k]->SetLayerComposited(true);
                    if ((mList->hwLayers[k].hints & HWC_HINT_CLEAR_FB) &&
                        (mList->hwLayers[k].blending == HWC_BLENDING_NONE)) {
                        
                        hwc_rect_t r = mList->hwLayers[k].displayFrame;
                        mHwcLayerMap[k]->SetClearRect(nsIntRect(r.left, r.top,
                                                                r.right - r.left,
                                                                r.bottom - r.top));
                    }
                    break;
                default:
                    break;
            }
        }

        if (gpuComposite) {
            
            return false;
        } else if (blitComposite) {
            
            GetGonkDisplay()->UpdateDispSurface(mDpy, mSur);
            DisplaySurface* dispSurface = (DisplaySurface*)(GetGonkDisplay()->GetDispSurface());
            if (!dispSurface) {
                LOGE("H/W Composition failed. NULL DispSurface.");
                return false;
            }
            mList->hwLayers[idx].handle = dispSurface->lastHandle;
            mList->hwLayers[idx].acquireFenceFd = dispSurface->GetPrevDispAcquireFd();
        }
    }

    
    Commit();

    GetGonkDisplay()->SetDispReleaseFd(mList->hwLayers[idx].releaseFenceFd);
    mList->hwLayers[idx].releaseFenceFd = -1;
    return true;
}

bool
HwcComposer2D::Render()
{
    
    if (!mHwc || !mList) {
        return GetGonkDisplay()->SwapBuffers(mDpy, mSur);
    }

    DisplaySurface* dispSurface = (DisplaySurface*)(GetGonkDisplay()->GetDispSurface());
    if (!dispSurface) {
        LOGE("H/W Composition failed. DispSurface not initialized.");
        return false;
    }

    if (mPrepared) {
        
        mList->hwLayers[mList->numHwLayers - 1].handle = dispSurface->lastHandle;
        mList->hwLayers[mList->numHwLayers - 1].acquireFenceFd = dispSurface->GetPrevDispAcquireFd();
    } else {
        mList->flags = HWC_GEOMETRY_CHANGED;
        mList->numHwLayers = 2;
        mList->hwLayers[0].hints = 0;
        mList->hwLayers[0].compositionType = HWC_FRAMEBUFFER;
        mList->hwLayers[0].flags = HWC_SKIP_LAYER;
        mList->hwLayers[0].backgroundColor = {0};
        mList->hwLayers[0].acquireFenceFd = -1;
        mList->hwLayers[0].releaseFenceFd = -1;
        mList->hwLayers[0].displayFrame = {0, 0, mScreenRect.width, mScreenRect.height};
        Prepare(dispSurface->lastHandle, dispSurface->GetPrevDispAcquireFd());
    }

    
    Commit();

    GetGonkDisplay()->SetDispReleaseFd(mList->hwLayers[mList->numHwLayers - 1].releaseFenceFd);
    mList->hwLayers[mList->numHwLayers - 1].releaseFenceFd = -1;
    return true;
}

void
HwcComposer2D::Prepare(buffer_handle_t dispHandle, int fence)
{
    int idx = mList->numHwLayers - 1;
    const hwc_rect_t r = {0, 0, mScreenRect.width, mScreenRect.height};
    hwc_display_contents_1_t *displays[HWC_NUM_DISPLAY_TYPES] = { nullptr };

    displays[HWC_DISPLAY_PRIMARY] = mList;
    mList->outbufAcquireFenceFd = -1;
    mList->outbuf = nullptr;
    mList->retireFenceFd = -1;

    mList->hwLayers[idx].hints = 0;
    mList->hwLayers[idx].flags = 0;
    mList->hwLayers[idx].transform = 0;
    mList->hwLayers[idx].handle = dispHandle;
    mList->hwLayers[idx].blending = HWC_BLENDING_PREMULT;
    mList->hwLayers[idx].compositionType = HWC_FRAMEBUFFER_TARGET;
    setCrop(&mList->hwLayers[idx], r);
    mList->hwLayers[idx].displayFrame = r;
    mList->hwLayers[idx].visibleRegionScreen.numRects = 1;
    mList->hwLayers[idx].visibleRegionScreen.rects = &mList->hwLayers[idx].displayFrame;
    mList->hwLayers[idx].acquireFenceFd = fence;
    mList->hwLayers[idx].releaseFenceFd = -1;
#if ANDROID_VERSION >= 18
    mList->hwLayers[idx].planeAlpha = 0xFF;
#endif
    if (mPrepared) {
        LOGE("Multiple hwc prepare calls!");
    }
    mHwc->prepare(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
    mPrepared = true;
}

bool
HwcComposer2D::Commit()
{
    hwc_display_contents_1_t *displays[HWC_NUM_DISPLAY_TYPES] = { nullptr };
    displays[HWC_DISPLAY_PRIMARY] = mList;

    for (uint32_t j=0; j < (mList->numHwLayers - 1); j++) {
        mList->hwLayers[j].acquireFenceFd = -1;
        if (mHwcLayerMap.IsEmpty() ||
            (mList->hwLayers[j].compositionType == HWC_FRAMEBUFFER)) {
            continue;
        }
        LayerRenderState state = mHwcLayerMap[j]->GetLayer()->GetRenderState();
        if (!state.mTexture) {
            continue;
        }
        FenceHandle fence = state.mTexture->GetAndResetAcquireFenceHandle();
        if (fence.IsValid()) {
            nsRefPtr<FenceHandle::FdObj> fdObj = fence.GetAndResetFdObj();
            mList->hwLayers[j].acquireFenceFd = fdObj->GetAndResetFd();
        }
    }

    int err = mHwc->set(mHwc, HWC_NUM_DISPLAY_TYPES, displays);

    mPrevRetireFence.TransferToAnotherFenceHandle(mPrevDisplayFence);

    for (uint32_t j=0; j < (mList->numHwLayers - 1); j++) {
        if (mList->hwLayers[j].releaseFenceFd >= 0) {
            int fd = mList->hwLayers[j].releaseFenceFd;
            mList->hwLayers[j].releaseFenceFd = -1;
            nsRefPtr<FenceHandle::FdObj> fdObj = new FenceHandle::FdObj(fd);
            FenceHandle fence(fdObj);

            LayerRenderState state = mHwcLayerMap[j]->GetLayer()->GetRenderState();
            if (!state.mTexture) {
                continue;
            }
            state.mTexture->SetReleaseFenceHandle(fence);
        }
    }

    if (mList->retireFenceFd >= 0) {
        mPrevRetireFence = FenceHandle(new FenceHandle::FdObj(mList->retireFenceFd));
    }

    mPrepared = false;
    return !err;
}

void
HwcComposer2D::Reset()
{
    LOGD("hwcomposer is already prepared, reset with null set");
    hwc_display_contents_1_t *displays[HWC_NUM_DISPLAY_TYPES] = { nullptr };
    displays[HWC_DISPLAY_PRIMARY] = nullptr;
    mHwc->set(mHwc, HWC_DISPLAY_PRIMARY, displays);
    mPrepared = false;
}
#else
bool
HwcComposer2D::TryHwComposition()
{
    return !mHwc->set(mHwc, mDpy, mSur, mList);
}

bool
HwcComposer2D::Render()
{
    return GetGonkDisplay()->SwapBuffers(mDpy, mSur);
}

void
HwcComposer2D::Reset()
{
    mPrepared = false;
}
#endif

bool
HwcComposer2D::TryRenderWithHwc(Layer* aRoot,
                                bool aGeometryChanged)
{
    if (!mHwc) {
        return false;
    }

    if (mList) {
        setHwcGeometry(aGeometryChanged);
        mList->numHwLayers = 0;
        mHwcLayerMap.Clear();
    }

    if (mPrepared) {
        Reset();
    }

    
    
    mVisibleRegions.clear();

    MOZ_ASSERT(mHwcLayerMap.IsEmpty());
    if (!PrepareLayerList(aRoot,
                          mScreenRect,
                          gfx::Matrix()))
    {
        mHwcLayerMap.Clear();
        LOGD("Render aborted. Nothing was drawn to the screen");
        return false;
    }

    
    SendtoLayerScope();

    if (!TryHwComposition()) {
        LOGD("Full HWC Composition failed. Fallback to GPU Composition or partial OVERLAY Composition");
        LayerScope::CleanLayer();
        return false;
    }

    LOGD("Frame rendered");
    return true;
}

void
HwcComposer2D::SendtoLayerScope()
{
    if (!LayerScope::CheckSendable()) {
        return;
    }

    const int len = mList->numHwLayers;
    for (int i = 0; i < len; ++i) {
        LayerComposite* layer = mHwcLayerMap[i];
        const hwc_rect_t r = mList->hwLayers[i].displayFrame;
        LayerScope::SendLayer(layer, r.right - r.left, r.bottom - r.top);
    }
}

} 
