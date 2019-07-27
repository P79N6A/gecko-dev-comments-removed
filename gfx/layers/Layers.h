




#ifndef GFX_LAYERS_H
#define GFX_LAYERS_H

#include <stdint.h>                     
#include <stdio.h>                      
#include <sys/types.h>                  
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfxContext.h"                 
#include "gfxTypes.h"
#include "gfxColor.h"                   
#include "GraphicsFilter.h"             
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
#include "gfx2DGlue.h"
#include "mozilla/Assertions.h"         
#include "mozilla/DebugOnly.h"          
#include "mozilla/EventForwards.h"      
#include "mozilla/RefPtr.h"             
#include "mozilla/StyleAnimationValue.h" 
#include "mozilla/TimeStamp.h"          
#include "mozilla/gfx/BaseMargin.h"     
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/gfx/UserData.h"       
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsCSSProperty.h"              
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsSize.h"                     
#include "nsString.h"                   
#include "nsTArray.h"                   
#include "nsTArrayForwardDeclare.h"     
#include "nscore.h"                     
#include "prlog.h"                      
#include "nsIWidget.h"                  
#include "gfxVR.h"

class gfxContext;

extern uint8_t gLayerManagerLayerBuilder;

namespace mozilla {

class ComputedTimingFunction;
class FrameLayerBuilder;
class StyleAnimationValue;
class WebGLContext;

namespace gl {
class GLContext;
class SharedSurface;
}

namespace gfx {
class DrawTarget;
}

namespace dom {
class OverfillCallback;
}

namespace layers {

class Animation;
class AnimationData;
class AsyncPanZoomController;
class ClientLayerManager;
class CommonLayerAttributes;
class Layer;
class LayerMetricsWrapper;
class PaintedLayer;
class ContainerLayer;
class ImageLayer;
class ColorLayer;
class ImageContainer;
class CanvasLayer;
class ReadbackLayer;
class ReadbackProcessor;
class RefLayer;
class LayerComposite;
class ShadowableLayer;
class ShadowLayerForwarder;
class LayerManagerComposite;
class SpecificLayerAttributes;
class SurfaceDescriptor;
class Compositor;
struct TextureFactoryIdentifier;
struct EffectMask;

namespace layerscope {
class LayersPacket;
}

#define MOZ_LAYER_DECL_NAME(n, e)                              \
  virtual const char* Name() const override { return n; }  \
  virtual LayerType GetType() const override { return e; }




class LayerUserData {
public:
  virtual ~LayerUserData() {}
};



























static void LayerManagerUserDataDestroy(void *data)
{
  delete static_cast<LayerUserData*>(data);
}
























class LayerManager {
  NS_INLINE_DECL_REFCOUNTING(LayerManager)

protected:
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::IntSize IntSize;
  typedef mozilla::gfx::SurfaceFormat SurfaceFormat;

public:
  LayerManager()
    : mDestroyed(false)
    , mSnapEffectiveTransforms(true)
    , mId(0)
    , mInTransaction(false)
  {
    InitLog();
  }

  





  virtual void Destroy()
  {
    mDestroyed = true;
    mUserData.Destroy();
    mRoot = nullptr;
  }
  bool IsDestroyed() { return mDestroyed; }

  virtual ShadowLayerForwarder* AsShadowForwarder()
  { return nullptr; }

  virtual LayerManagerComposite* AsLayerManagerComposite()
  { return nullptr; }

  virtual ClientLayerManager* AsClientLayerManager()
  { return nullptr; }

  



  virtual bool IsWidgetLayerManager() { return true; }
  virtual bool IsInactiveLayerManager() { return false; }

  





  virtual void BeginTransaction() = 0;
  






  virtual void BeginTransactionWithTarget(gfxContext* aTarget) = 0;

  enum EndTransactionFlags {
    END_DEFAULT = 0,
    END_NO_IMMEDIATE_REDRAW = 1 << 0,  
    END_NO_COMPOSITE = 1 << 1, 
    END_NO_REMOTE_COMPOSITE = 1 << 2 
  };

  FrameLayerBuilder* GetLayerBuilder() {
    return reinterpret_cast<FrameLayerBuilder*>(GetUserData(&gLayerManagerLayerBuilder));
  }

  







  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) = 0;

  

























  typedef void (* DrawPaintedLayerCallback)(PaintedLayer* aLayer,
                                           gfxContext* aContext,
                                           const nsIntRegion& aRegionToDraw,
                                           DrawRegionClip aClip,
                                           const nsIntRegion& aRegionToInvalidate,
                                           void* aCallbackData);

  






  virtual void EndTransaction(DrawPaintedLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) = 0;

  




  virtual void Composite() {}

  virtual bool HasShadowManagerInternal() const { return false; }
  bool HasShadowManager() const { return HasShadowManagerInternal(); }
  virtual void StorePluginWidgetConfigurations(const nsTArray<nsIWidget::Configuration>& aConfigurations) {}
  bool IsSnappingEffectiveTransforms() { return mSnapEffectiveTransforms; }


  




  virtual bool ShouldAvoidComponentAlphaLayers() { return false; }

  




  virtual bool AreComponentAlphaLayersEnabled();

  




  virtual void SetRoot(Layer* aLayer) = 0;
  


  Layer* GetRoot() { return mRoot; }

  





  FrameMetrics::ViewID GetRootScrollableLayerId();

  






  void GetRootScrollableLayers(nsTArray<Layer*>& aArray);

  




  void GetScrollableLayers(nsTArray<Layer*>& aArray);

  





#ifdef DEBUG
  
  virtual void Mutated(Layer* aLayer);
#else
  virtual void Mutated(Layer* aLayer) { }
#endif

  






  enum PaintedLayerCreationHint {
    NONE, SCROLLABLE
  };

  


  virtual bool IsOptimizedFor(PaintedLayer* aLayer,
                              PaintedLayerCreationHint aCreationHint)
  { return true; }

  



  virtual already_AddRefed<PaintedLayer> CreatePaintedLayer() = 0;
  




  virtual already_AddRefed<PaintedLayer> CreatePaintedLayerWithHint(PaintedLayerCreationHint) {
    return CreatePaintedLayer();
  }
  



  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() = 0;
  



  virtual already_AddRefed<ImageLayer> CreateImageLayer() = 0;
  



  virtual already_AddRefed<ColorLayer> CreateColorLayer() = 0;
  



  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() = 0;
  



  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer() { return nullptr; }
  



  virtual already_AddRefed<RefLayer> CreateRefLayer() { return nullptr; }


  





  static already_AddRefed<ImageContainer> CreateImageContainer();

  







  static already_AddRefed<ImageContainer> CreateAsynchronousImageContainer();

  




  virtual LayersBackend GetBackendType() = 0;

  




  virtual LayersBackend GetCompositorBackendType() { return GetBackendType(); }

  



  virtual TemporaryRef<DrawTarget>
    CreateOptimalDrawTarget(const IntSize &aSize,
                            SurfaceFormat imageFormat);

  





  virtual TemporaryRef<DrawTarget>
    CreateOptimalMaskDrawTarget(const IntSize &aSize);

  



  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize &aSize,
                     mozilla::gfx::SurfaceFormat aFormat);

  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize) { return true; }

  



  virtual int32_t GetMaxTextureSize() const = 0;

  


  virtual void GetBackendName(nsAString& aName) = 0;

  



  void SetUserData(void* aKey, LayerUserData* aData)
  {
    mUserData.Add(static_cast<gfx::UserDataKey*>(aKey), aData, LayerManagerUserDataDestroy);
  }
  


  nsAutoPtr<LayerUserData> RemoveUserData(void* aKey)
  {
    nsAutoPtr<LayerUserData> d(static_cast<LayerUserData*>(mUserData.Remove(static_cast<gfx::UserDataKey*>(aKey))));
    return d;
  }
  


  bool HasUserData(void* aKey)
  {
    return mUserData.Has(static_cast<gfx::UserDataKey*>(aKey));
  }
  



  LayerUserData* GetUserData(void* aKey) const
  {
    return static_cast<LayerUserData*>(mUserData.Get(static_cast<gfx::UserDataKey*>(aKey)));
  }

  















  virtual void ClearCachedResources(Layer* aSubtree = nullptr) {}

  


  virtual void SetIsFirstPaint() {}

  






  virtual void FlushRendering() { }

  



  virtual bool NeedsWidgetInvalidation() { return true; }

  virtual const char* Name() const { return "???"; }

  



  void Dump(std::stringstream& aStream, const char* aPrefix="", bool aDumpHtml=false);
  


  void DumpSelf(std::stringstream& aStream, const char* aPrefix="");
  void Dump();

  



  void Dump(layerscope::LayersPacket* aPacket);

  



  void Log(const char* aPrefix="");
  



  void LogSelf(const char* aPrefix="");

  











  


  virtual uint32_t StartFrameTimeRecording(int32_t aBufferSize);

  




  virtual void StopFrameTimeRecording(uint32_t         aStartIndex,
                                      nsTArray<float>& aFrameIntervals);

  void RecordFrame();
  void PostPresent();

  void BeginTabSwitch();

  static bool IsLogEnabled();
  static PRLogModuleInfo* GetLog() { return sLog; }

  bool IsCompositingCheap(LayersBackend aBackend)
  {
    
    
    return LayersBackend::LAYERS_BASIC != aBackend && LayersBackend::LAYERS_NONE != aBackend;
  }

  virtual bool IsCompositingCheap() { return true; }

  bool IsInTransaction() const { return mInTransaction; }
  virtual bool RequestOverfill(mozilla::dom::OverfillCallback* aCallback) { return true; }
  virtual void RunOverfillCallback(const uint32_t aOverfill) { }

  virtual void SetRegionToClear(const nsIntRegion& aRegion)
  {
    mRegionToClear = aRegion;
  }

  virtual bool SupportsMixBlendModes(EnumSet<gfx::CompositionOp>& aMixBlendModes)
  {
    return false;
  }

  bool SupportsMixBlendMode(gfx::CompositionOp aMixBlendMode)
  {
    EnumSet<gfx::CompositionOp> modes(aMixBlendMode);
    return SupportsMixBlendModes(modes);
  }

  virtual float RequestProperty(const nsAString& property) { return -1; }

  const TimeStamp& GetAnimationReadyTime() const {
    return mAnimationReadyTime;
  }

protected:
  nsRefPtr<Layer> mRoot;
  gfx::UserData mUserData;
  bool mDestroyed;
  bool mSnapEffectiveTransforms;

  nsIntRegion mRegionToClear;

  
  virtual ~LayerManager() {}

  
  
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  
  
  virtual void DumpPacket(layerscope::LayersPacket* aPacket);

  static void InitLog();
  static PRLogModuleInfo* sLog;
  uint64_t mId;
  bool mInTransaction;
  
  
  TimeStamp mAnimationReadyTime;
private:
  struct FramesTimingRecording
  {
    
    
    FramesTimingRecording()
      : mIsPaused(true)
      , mNextIndex(0)
    {}
    bool mIsPaused;
    uint32_t mNextIndex;
    TimeStamp mLastFrameTime;
    nsTArray<float> mIntervals;
    uint32_t mLatestStartIndex;
    uint32_t mCurrentRunStartIndex;
  };
  FramesTimingRecording mRecording;

  TimeStamp mTabSwitchStart;
};

typedef InfallibleTArray<Animation> AnimationArray;

struct AnimData {
  InfallibleTArray<mozilla::StyleAnimationValue> mStartValues;
  InfallibleTArray<mozilla::StyleAnimationValue> mEndValues;
  InfallibleTArray<nsAutoPtr<mozilla::ComputedTimingFunction> > mFunctions;
};





class Layer {
  NS_INLINE_DECL_REFCOUNTING(Layer)

public:
  
  enum LayerType {
    TYPE_CANVAS,
    TYPE_COLOR,
    TYPE_CONTAINER,
    TYPE_IMAGE,
    TYPE_READBACK,
    TYPE_REF,
    TYPE_SHADOW,
    TYPE_PAINTED
  };

  




  LayerManager* Manager() { return mManager; }

  enum {
    




    CONTENT_OPAQUE = 0x01,
    






    CONTENT_COMPONENT_ALPHA = 0x02,

    



    CONTENT_COMPONENT_ALPHA_DESCENDANT = 0x04,

    



    CONTENT_PRESERVE_3D = 0x08,
    




    CONTENT_MAY_CHANGE_TRANSFORM = 0x10,

    



    CONTENT_DISABLE_SUBPIXEL_AA = 0x20
  };
  





  void SetContentFlags(uint32_t aFlags)
  {
    NS_ASSERTION((aFlags & (CONTENT_OPAQUE | CONTENT_COMPONENT_ALPHA)) !=
                 (CONTENT_OPAQUE | CONTENT_COMPONENT_ALPHA),
                 "Can't be opaque and require component alpha");
    if (mContentFlags != aFlags) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ContentFlags", this));
      mContentFlags = aFlags;
      Mutated();
    }
  }

  







  virtual void SetLayerBounds(const nsIntRect& aLayerBounds)
  {
    if (!mLayerBounds.IsEqualEdges(aLayerBounds)) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) LayerBounds", this));
      mLayerBounds = aLayerBounds;
      Mutated();
    }
  }

  












  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    if (!mVisibleRegion.IsEqual(aRegion)) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) VisibleRegion was %s is %s", this,
        mVisibleRegion.ToString().get(), aRegion.ToString().get()));
      mVisibleRegion = aRegion;
      Mutated();
    }
  }

  







  void SetFrameMetrics(const FrameMetrics& aFrameMetrics)
  {
    if (mFrameMetrics.Length() != 1 || mFrameMetrics[0] != aFrameMetrics) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) FrameMetrics", this));
      mFrameMetrics.ReplaceElementsAt(0, mFrameMetrics.Length(), aFrameMetrics);
      FrameMetricsChanged();
      Mutated();
    }
  }

  
















  void SetFrameMetrics(const nsTArray<FrameMetrics>& aMetricsArray)
  {
    if (mFrameMetrics != aMetricsArray) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) FrameMetrics", this));
      mFrameMetrics = aMetricsArray;
      FrameMetricsChanged();
      Mutated();
    }
  }

  
























  



  void SetEventRegions(const EventRegions& aRegions)
  {
    if (mEventRegions != aRegions) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) eventregions were %s, now %s", this,
        mEventRegions.ToString().get(), aRegions.ToString().get()));
      mEventRegions = aRegions;
      Mutated();
    }
  }

  




  void SetOpacity(float aOpacity)
  {
    if (mOpacity != aOpacity) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) Opacity", this));
      mOpacity = aOpacity;
      Mutated();
    }
  }

  void SetMixBlendMode(gfx::CompositionOp aMixBlendMode)
  {
    if (mMixBlendMode != aMixBlendMode) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) MixBlendMode", this));
      mMixBlendMode = aMixBlendMode;
      Mutated();
    }
  }

  void DeprecatedSetMixBlendMode(gfxContext::GraphicsOperator aMixBlendMode)
  {
    SetMixBlendMode(gfx::CompositionOpForOp(aMixBlendMode));
  }

  void SetForceIsolatedGroup(bool aForceIsolatedGroup)
  {
    if(mForceIsolatedGroup != aForceIsolatedGroup) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ForceIsolatedGroup", this));
      mForceIsolatedGroup = aForceIsolatedGroup;
      Mutated();
    }
  }

  bool GetForceIsolatedGroup() const
  {
    return mForceIsolatedGroup;
  }

  









  void SetClipRect(const nsIntRect* aRect)
  {
    if (mUseClipRect) {
      if (!aRect) {
        MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ClipRect was %d,%d,%d,%d is <none>", this,
                         mClipRect.x, mClipRect.y, mClipRect.width, mClipRect.height));
        mUseClipRect = false;
        Mutated();
      } else {
        if (!aRect->IsEqualEdges(mClipRect)) {
          MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ClipRect was %d,%d,%d,%d is %d,%d,%d,%d", this,
                           mClipRect.x, mClipRect.y, mClipRect.width, mClipRect.height,
                           aRect->x, aRect->y, aRect->width, aRect->height));
          mClipRect = *aRect;
          Mutated();
        }
      }
    } else {
      if (aRect) {
        MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ClipRect was <none> is %d,%d,%d,%d", this,
                         aRect->x, aRect->y, aRect->width, aRect->height));
        mUseClipRect = true;
        mClipRect = *aRect;
        Mutated();
      }
    }
  }

  














  void SetMaskLayer(Layer* aMaskLayer)
  {
#ifdef DEBUG
    if (aMaskLayer) {
      bool maskIs2D = aMaskLayer->GetTransform().CanDraw2D();
      NS_ASSERTION(maskIs2D, "Mask layer has invalid transform.");
    }
#endif

    if (mMaskLayer != aMaskLayer) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) MaskLayer", this));
      mMaskLayer = aMaskLayer;
      Mutated();
    }
  }

  




  void SetBaseTransform(const gfx::Matrix4x4& aMatrix)
  {
    NS_ASSERTION(!aMatrix.IsSingular(),
                 "Shouldn't be trying to draw with a singular matrix!");
    mPendingTransform = nullptr;
    if (mTransform == aMatrix) {
      return;
    }
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) BaseTransform", this));
    mTransform = aMatrix;
    Mutated();
  }

  







  void SetBaseTransformForNextTransaction(const gfx::Matrix4x4& aMatrix)
  {
    mPendingTransform = new gfx::Matrix4x4(aMatrix);
  }

  void SetPostScale(float aXScale, float aYScale)
  {
    if (mPostXScale == aXScale && mPostYScale == aYScale) {
      return;
    }
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) PostScale", this));
    mPostXScale = aXScale;
    mPostYScale = aYScale;
    Mutated();
  }

  





  void SetIsFixedPosition(bool aFixedPosition)
  {
    if (mIsFixedPosition != aFixedPosition) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) IsFixedPosition", this));
      mIsFixedPosition = aFixedPosition;
      Mutated();
    }
  }

  
  
  
  Animation* AddAnimation();
  
  void ClearAnimations();
  
  
  void SetAnimations(const AnimationArray& aAnimations);
  
  
  
  
  void StartPendingAnimations(const TimeStamp& aReadyTime);

  
  
  
  
  Animation* AddAnimationForNextTransaction();
  void ClearAnimationsForNextTransaction();

  






  void SetFixedPositionAnchor(const LayerPoint& aAnchor)
  {
    if (mAnchor != aAnchor) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) FixedPositionAnchor", this));
      mAnchor = aAnchor;
      Mutated();
    }
  }

  










  void SetFixedPositionMargins(const LayerMargin& aMargins)
  {
    if (mMargins != aMargins) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) FixedPositionMargins", this));
      mMargins = aMargins;
      Mutated();
    }
  }

  








  void SetStickyPositionData(FrameMetrics::ViewID aScrollId, LayerRect aOuter,
                             LayerRect aInner)
  {
    if (!mStickyPositionData ||
        !mStickyPositionData->mOuter.IsEqualEdges(aOuter) ||
        !mStickyPositionData->mInner.IsEqualEdges(aInner)) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) StickyPositionData", this));
      if (!mStickyPositionData) {
        mStickyPositionData = new StickyPositionData;
      }
      mStickyPositionData->mScrollId = aScrollId;
      mStickyPositionData->mOuter = aOuter;
      mStickyPositionData->mInner = aInner;
      Mutated();
    }
  }

  enum ScrollDirection {
    NONE,
    VERTICAL,
    HORIZONTAL
  };

  




  void SetScrollbarData(FrameMetrics::ViewID aScrollId, ScrollDirection aDir, float aThumbRatio)
  {
    if (mScrollbarTargetId != aScrollId ||
        mScrollbarDirection != aDir ||
        mScrollbarThumbRatio != aThumbRatio)
    {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ScrollbarData", this));
      mScrollbarTargetId = aScrollId;
      mScrollbarDirection = aDir;
      mScrollbarThumbRatio = aThumbRatio;
      Mutated();
    }
  }

  
  void SetIsScrollbarContainer()
  {
    if (!mIsScrollbarContainer) {
      mIsScrollbarContainer = true;
      Mutated();
    }
  }

  
  float GetOpacity() { return mOpacity; }
  gfx::CompositionOp GetMixBlendMode() const { return mMixBlendMode; }
  const nsIntRect* GetClipRect() { return mUseClipRect ? &mClipRect : nullptr; }
  uint32_t GetContentFlags() { return mContentFlags; }
  const nsIntRect& GetLayerBounds() const { return mLayerBounds; }
  const nsIntRegion& GetVisibleRegion() const { return mVisibleRegion; }
  const FrameMetrics& GetFrameMetrics(uint32_t aIndex) const;
  uint32_t GetFrameMetricsCount() const { return mFrameMetrics.Length(); }
  const nsTArray<FrameMetrics>& GetAllFrameMetrics() { return mFrameMetrics; }
  bool HasScrollableFrameMetrics() const;
  bool IsScrollInfoLayer() const;
  const EventRegions& GetEventRegions() const { return mEventRegions; }
  ContainerLayer* GetParent() { return mParent; }
  Layer* GetNextSibling() { return mNextSibling; }
  const Layer* GetNextSibling() const { return mNextSibling; }
  Layer* GetPrevSibling() { return mPrevSibling; }
  const Layer* GetPrevSibling() const { return mPrevSibling; }
  virtual Layer* GetFirstChild() const { return nullptr; }
  virtual Layer* GetLastChild() const { return nullptr; }
  const gfx::Matrix4x4 GetTransform() const;
  const gfx::Matrix4x4& GetBaseTransform() const { return mTransform; }
  
  virtual float GetPostXScale() const { return mPostXScale; }
  virtual float GetPostYScale() const { return mPostYScale; }
  bool GetIsFixedPosition() { return mIsFixedPosition; }
  bool GetIsStickyPosition() { return mStickyPositionData; }
  LayerPoint GetFixedPositionAnchor() { return mAnchor; }
  const LayerMargin& GetFixedPositionMargins() { return mMargins; }
  FrameMetrics::ViewID GetStickyScrollContainerId() { return mStickyPositionData->mScrollId; }
  const LayerRect& GetStickyScrollRangeOuter() { return mStickyPositionData->mOuter; }
  const LayerRect& GetStickyScrollRangeInner() { return mStickyPositionData->mInner; }
  FrameMetrics::ViewID GetScrollbarTargetContainerId() { return mScrollbarTargetId; }
  ScrollDirection GetScrollbarDirection() { return mScrollbarDirection; }
  float GetScrollbarThumbRatio() { return mScrollbarThumbRatio; }
  bool IsScrollbarContainer() { return mIsScrollbarContainer; }
  Layer* GetMaskLayer() const { return mMaskLayer; }


  














  bool GetVisibleRegionRelativeToRootLayer(nsIntRegion& aResult,
                                           nsIntPoint* aLayerOffset);

  
  
  AnimationArray& GetAnimations() { return mAnimations; }
  InfallibleTArray<AnimData>& GetAnimationData() { return mAnimationData; }

  uint64_t GetAnimationGeneration() { return mAnimationGeneration; }
  void SetAnimationGeneration(uint64_t aCount) { mAnimationGeneration = aCount; }

  bool HasTransformAnimation() const;

  



  const gfx::Matrix4x4 GetLocalTransform();

  



  const float GetLocalOpacity();

  





  void ApplyPendingUpdatesToSubtree();

  





  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) { }

  
  
  
  
  
  bool CanUseOpaqueSurface();

  SurfaceMode GetSurfaceMode()
  {
    if (CanUseOpaqueSurface())
      return SurfaceMode::SURFACE_OPAQUE;
    if (mContentFlags & CONTENT_COMPONENT_ALPHA)
      return SurfaceMode::SURFACE_COMPONENT_ALPHA;
    return SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
  }

  



  void SetUserData(void* aKey, LayerUserData* aData)
  {
    mUserData.Add(static_cast<gfx::UserDataKey*>(aKey), aData, LayerManagerUserDataDestroy);
  }
  


  nsAutoPtr<LayerUserData> RemoveUserData(void* aKey)
  {
    nsAutoPtr<LayerUserData> d(static_cast<LayerUserData*>(mUserData.Remove(static_cast<gfx::UserDataKey*>(aKey))));
    return d;
  }
  


  bool HasUserData(void* aKey)
  {
    return mUserData.Has(static_cast<gfx::UserDataKey*>(aKey));
  }
  



  LayerUserData* GetUserData(void* aKey) const
  {
    return static_cast<LayerUserData*>(mUserData.Get(static_cast<gfx::UserDataKey*>(aKey)));
  }

  








  virtual void Disconnect() {}

  



  virtual PaintedLayer* AsPaintedLayer() { return nullptr; }

  



  virtual ContainerLayer* AsContainerLayer() { return nullptr; }
  virtual const ContainerLayer* AsContainerLayer() const { return nullptr; }

   



  virtual RefLayer* AsRefLayer() { return nullptr; }

   



  virtual ColorLayer* AsColorLayer() { return nullptr; }

  



  virtual LayerComposite* AsLayerComposite() { return nullptr; }

  



  virtual ShadowableLayer* AsShadowableLayer() { return nullptr; }

  
  
  
  const nsIntRect* GetEffectiveClipRect();
  const nsIntRegion& GetEffectiveVisibleRegion();

  



  float GetEffectiveOpacity();

  


  gfx::CompositionOp GetEffectiveMixBlendMode();
  gfxContext::GraphicsOperator DeprecatedGetEffectiveMixBlendMode();

  







  const gfx::Matrix4x4& GetEffectiveTransform() const { return mEffectiveTransform; }

  











  virtual const gfx::Matrix4x4& GetEffectiveTransformForBuffer() const
  {
    return mEffectiveTransform;
  }

  










  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) = 0;

  


  void ComputeEffectiveTransformForMaskLayer(const gfx::Matrix4x4& aTransformToSurface);

  






  RenderTargetIntRect CalculateScissorRect(const RenderTargetIntRect& aCurrentScissorRect);

  virtual const char* Name() const =0;
  virtual LayerType GetType() const =0;

  




  void* ImplData() { return mImplData; }

  


  void SetParent(ContainerLayer* aParent) { mParent = aParent; }
  void SetNextSibling(Layer* aSibling) { mNextSibling = aSibling; }
  void SetPrevSibling(Layer* aSibling) { mPrevSibling = aSibling; }

  



  void Dump(std::stringstream& aStream, const char* aPrefix="", bool aDumpHtml=false);
  


  void DumpSelf(std::stringstream& aStream, const char* aPrefix="");

  



  void Dump(layerscope::LayersPacket* aPacket, const void* aParent);

  



  void Log(const char* aPrefix="");
  



  void LogSelf(const char* aPrefix="");

  
  
  
  
  
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  
  
  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent);

  static bool IsLogEnabled() { return LayerManager::IsLogEnabled(); }

  



  const nsIntRegion& GetInvalidRegion() { return mInvalidRegion; }
  const void SetInvalidRegion(const nsIntRegion& aRect) { mInvalidRegion = aRect; }

  


  void SetInvalidRectToVisibleRegion() { mInvalidRegion = GetVisibleRegion(); }

  


  void AddInvalidRect(const nsIntRect& aRect) { mInvalidRegion.Or(mInvalidRegion, aRect); }

  



  void ClearInvalidRect() { mInvalidRegion.SetEmpty(); }

  
  
  
  
  
  void SetAsyncPanZoomController(uint32_t aIndex, AsyncPanZoomController *controller);
  AsyncPanZoomController* GetAsyncPanZoomController(uint32_t aIndex) const;
  
  
private:
  void FrameMetricsChanged();
public:

  void ApplyPendingUpdatesForThisTransaction();

#ifdef DEBUG
  void SetDebugColorIndex(uint32_t aIndex) { mDebugColorIndex = aIndex; }
  uint32_t GetDebugColorIndex() { return mDebugColorIndex; }
#endif

  virtual LayerRenderState GetRenderState() { return LayerRenderState(); }

  void Mutated()
  {
    mManager->Mutated(this);
  }

  virtual int32_t GetMaxLayerSize() { return Manager()->GetMaxTextureSize(); }

  





  bool MayResample();

  RenderTargetRect TransformRectToRenderTarget(const LayerIntRect& aRect);

  


  void AddExtraDumpInfo(const nsACString& aStr)
  {
#ifdef MOZ_DUMP_PAINTING
    mExtraDumpInfo.AppendElement(aStr);
#endif
  }

  


  void ClearExtraDumpInfo()
  {
#ifdef MOZ_DUMP_PAINTING
     mExtraDumpInfo.Clear();
#endif
  }

protected:
  Layer(LayerManager* aManager, void* aImplData);

  
  virtual ~Layer();

  
























  gfx::Matrix4x4 SnapTransformTranslation(const gfx::Matrix4x4& aTransform,
                                          gfx::Matrix* aResidualTransform);
  










  gfx::Matrix4x4 SnapTransform(const gfx::Matrix4x4& aTransform,
                               const gfxRect& aSnapRect,
                               gfx::Matrix* aResidualTransform);

  LayerManager* mManager;
  ContainerLayer* mParent;
  Layer* mNextSibling;
  Layer* mPrevSibling;
  void* mImplData;
  nsRefPtr<Layer> mMaskLayer;
  gfx::UserData mUserData;
  nsIntRect mLayerBounds;
  nsIntRegion mVisibleRegion;
  nsTArray<FrameMetrics> mFrameMetrics;
  EventRegions mEventRegions;
  gfx::Matrix4x4 mTransform;
  
  
  
  nsAutoPtr<gfx::Matrix4x4> mPendingTransform;
  float mPostXScale;
  float mPostYScale;
  gfx::Matrix4x4 mEffectiveTransform;
  AnimationArray mAnimations;
  
  nsAutoPtr<AnimationArray> mPendingAnimations;
  InfallibleTArray<AnimData> mAnimationData;
  float mOpacity;
  gfx::CompositionOp mMixBlendMode;
  bool mForceIsolatedGroup;
  nsIntRect mClipRect;
  nsIntRect mTileSourceRect;
  nsIntRegion mInvalidRegion;
  nsTArray<nsRefPtr<AsyncPanZoomController> > mApzcs;
  uint32_t mContentFlags;
  bool mUseClipRect;
  bool mUseTileSourceRect;
  bool mIsFixedPosition;
  LayerPoint mAnchor;
  LayerMargin mMargins;
  struct StickyPositionData {
    FrameMetrics::ViewID mScrollId;
    LayerRect mOuter;
    LayerRect mInner;
  };
  nsAutoPtr<StickyPositionData> mStickyPositionData;
  FrameMetrics::ViewID mScrollbarTargetId;
  ScrollDirection mScrollbarDirection;
  float mScrollbarThumbRatio; 
                              
  bool mIsScrollbarContainer;
  DebugOnly<uint32_t> mDebugColorIndex;
  
  
  uint64_t mAnimationGeneration;
#ifdef MOZ_DUMP_PAINTING
  nsTArray<nsCString> mExtraDumpInfo;
#endif
};












class PaintedLayer : public Layer {
public:
  





  virtual void InvalidateRegion(const nsIntRegion& aRegion) = 0;
  











  void SetAllowResidualTranslation(bool aAllow) { mAllowResidualTranslation = aAllow; }

  


  const nsIntRegion& GetValidRegion() const { return mValidRegion; }

  virtual PaintedLayer* AsPaintedLayer() override { return this; }

  MOZ_LAYER_DECL_NAME("PaintedLayer", TYPE_PAINTED)

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override
  {
    gfx::Matrix4x4 idealTransform = GetLocalTransform() * aTransformToSurface;
    gfx::Matrix residual;
    mEffectiveTransform = SnapTransformTranslation(idealTransform,
        mAllowResidualTranslation ? &residual : nullptr);
    
    
    NS_ASSERTION(residual.IsTranslation(),
                 "Residual transform can only be a translation");
    if (!gfx::ThebesPoint(residual.GetTranslation()).WithinEpsilonOf(mResidualTranslation, 1e-3f)) {
      mResidualTranslation = gfx::ThebesPoint(residual.GetTranslation());
      NS_ASSERTION(-0.5 <= mResidualTranslation.x && mResidualTranslation.x < 0.5 &&
                   -0.5 <= mResidualTranslation.y && mResidualTranslation.y < 0.5,
                   "Residual translation out of range");
      mValidRegion.SetEmpty();
    }
    ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  }

  LayerManager::PaintedLayerCreationHint GetCreationHint() const { return mCreationHint; }

  bool UsedForReadback() { return mUsedForReadback; }
  void SetUsedForReadback(bool aUsed) { mUsedForReadback = aUsed; }
  







  gfxPoint GetResidualTranslation() const { return mResidualTranslation; }

protected:
  PaintedLayer(LayerManager* aManager, void* aImplData,
              LayerManager::PaintedLayerCreationHint aCreationHint = LayerManager::NONE)
    : Layer(aManager, aImplData)
    , mValidRegion()
    , mCreationHint(aCreationHint)
    , mUsedForReadback(false)
    , mAllowResidualTranslation(false)
  {
    mContentFlags = 0; 
  }

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent) override;

  




  gfxPoint mResidualTranslation;
  nsIntRegion mValidRegion;
  


  const LayerManager::PaintedLayerCreationHint mCreationHint;
  



  bool mUsedForReadback;
  


  bool mAllowResidualTranslation;
};





class ContainerLayer : public Layer {
public:

  ~ContainerLayer();

  






  virtual bool InsertAfter(Layer* aChild, Layer* aAfter);
  




  virtual bool RemoveChild(Layer* aChild);
  






  virtual bool RepositionChild(Layer* aChild, Layer* aAfter);

  void SetPreScale(float aXScale, float aYScale)
  {
    if (mPreXScale == aXScale && mPreYScale == aYScale) {
      return;
    }

    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) PreScale", this));
    mPreXScale = aXScale;
    mPreYScale = aYScale;
    Mutated();
  }

  void SetInheritedScale(float aXScale, float aYScale)
  {
    if (mInheritedXScale == aXScale && mInheritedYScale == aYScale) {
      return;
    }

    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) InheritedScale", this));
    mInheritedXScale = aXScale;
    mInheritedYScale = aYScale;
    Mutated();
  }

  void SetScaleToResolution(bool aScaleToResolution, float aResolution)
  {
    if (mScaleToResolution == aScaleToResolution && mPresShellResolution == aResolution) {
      return;
    }

    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ScaleToResolution", this));
    mScaleToResolution = aScaleToResolution;
    mPresShellResolution = aResolution;
    Mutated();
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) override;

  void SortChildrenBy3DZOrder(nsTArray<Layer*>& aArray);

  

  virtual ContainerLayer* AsContainerLayer() override { return this; }
  virtual const ContainerLayer* AsContainerLayer() const override { return this; }

  virtual Layer* GetFirstChild() const override { return mFirstChild; }
  virtual Layer* GetLastChild() const override { return mLastChild; }
  float GetPreXScale() const { return mPreXScale; }
  float GetPreYScale() const { return mPreYScale; }
  float GetInheritedXScale() const { return mInheritedXScale; }
  float GetInheritedYScale() const { return mInheritedYScale; }
  float GetPresShellResolution() const { return mPresShellResolution; }
  bool ScaleToResolution() const { return mScaleToResolution; }

  MOZ_LAYER_DECL_NAME("ContainerLayer", TYPE_CONTAINER)

  





  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override = 0;

  





  bool UseIntermediateSurface() { return mUseIntermediateSurface; }

  






  RenderTargetIntRect GetIntermediateSurfaceRect()
  {
    NS_ASSERTION(mUseIntermediateSurface, "Must have intermediate surface");
    return RenderTargetPixel::FromUntyped(mVisibleRegion.GetBounds());
  }

  


  bool HasMultipleChildren();

  



  bool SupportsComponentAlphaChildren() { return mSupportsComponentAlphaChildren; }

  



  static bool HasOpaqueAncestorLayer(Layer* aLayer);

  void SetChildrenChanged(bool aVal) {
    mChildrenChanged = aVal;
  }

  void SetEventRegionsOverride(EventRegionsOverride aVal) {
    if (mEventRegionsOverride == aVal) {
      return;
    }

    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) EventRegionsOverride", this));
    mEventRegionsOverride = aVal;
    Mutated();
  }

  EventRegionsOverride GetEventRegionsOverride() const {
    return mEventRegionsOverride;
  }

  


  void SetVRHMDInfo(gfx::VRHMDInfo* aHMD) { mHMDInfo = aHMD; }
  gfx::VRHMDInfo* GetVRHMDInfo() { return mHMDInfo; }

protected:
  friend class ReadbackProcessor;

  void DidInsertChild(Layer* aLayer);
  void DidRemoveChild(Layer* aLayer);

  ContainerLayer(LayerManager* aManager, void* aImplData);

  



  void DefaultComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface);

  





  void DefaultComputeSupportsComponentAlphaChildren(bool* aNeedsSurfaceCopy = nullptr);

  


  void ComputeEffectiveTransformsForChildren(const gfx::Matrix4x4& aTransformToSurface);

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent) override;

  Layer* mFirstChild;
  Layer* mLastChild;
  float mPreXScale;
  float mPreYScale;
  
  
  float mInheritedXScale;
  float mInheritedYScale;
  
  
  float mPresShellResolution;
  
  bool mScaleToResolution;
  bool mUseIntermediateSurface;
  bool mSupportsComponentAlphaChildren;
  bool mMayHaveReadbackChild;
  
  
  bool mChildrenChanged;
  EventRegionsOverride mEventRegionsOverride;
  nsRefPtr<gfx::VRHMDInfo> mHMDInfo;
};






class ColorLayer : public Layer {
public:
  virtual ColorLayer* AsColorLayer() override { return this; }

  



  virtual void SetColor(const gfxRGBA& aColor)
  {
    if (mColor != aColor) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) Color", this));
      mColor = aColor;
      Mutated();
    }
  }

  void SetBounds(const nsIntRect& aBounds)
  {
    if (!mBounds.IsEqualEdges(aBounds)) {
      mBounds = aBounds;
      Mutated();
    }
  }

  const nsIntRect& GetBounds()
  {
    return mBounds;
  }

  
  virtual const gfxRGBA& GetColor() { return mColor; }

  MOZ_LAYER_DECL_NAME("ColorLayer", TYPE_COLOR)

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override
  {
    gfx::Matrix4x4 idealTransform = GetLocalTransform() * aTransformToSurface;
    mEffectiveTransform = SnapTransformTranslation(idealTransform, nullptr);
    ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  }

protected:
  ColorLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mColor(0.0, 0.0, 0.0, 0.0)
  {}

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent) override;

  nsIntRect mBounds;
  gfxRGBA mColor;
};











class CanvasLayer : public Layer {
public:
  struct Data {
    Data()
      : mDrawTarget(nullptr)
      , mGLContext(nullptr)
      , mFrontbufferGLTex(0)
      , mSize(0,0)
      , mHasAlpha(false)
      , mIsGLAlphaPremult(true)
    { }

    
    mozilla::gfx::DrawTarget* mDrawTarget; 
    mozilla::gl::GLContext* mGLContext; 

    
    uint32_t mFrontbufferGLTex;

    
    nsIntSize mSize;

    
    bool mHasAlpha;

    
    bool mIsGLAlphaPremult;
  };

  







  virtual void Initialize(const Data& aData) = 0;

  


  virtual bool IsDataValid(const Data& aData) { return true; }

  



  void Updated() { mDirty = true; SetInvalidRectToVisibleRegion(); }

  



  void Painted() { mDirty = false; }

  



  bool IsDirty()
  {
    
    
    if (!mManager || !mManager->IsWidgetLayerManager()) {
      return true;
    }
    return mDirty;
  }

  


  typedef void PreTransactionCallback(void* closureData);
  void SetPreTransactionCallback(PreTransactionCallback* callback, void* closureData)
  {
    mPreTransCallback = callback;
    mPreTransCallbackData = closureData;
  }

protected:
  void FirePreTransactionCallback()
  {
    if (mPreTransCallback) {
      mPreTransCallback(mPreTransCallbackData);
    }
  }

public:
  


  typedef void (* DidTransactionCallback)(void* aClosureData);
  void SetDidTransactionCallback(DidTransactionCallback aCallback, void* aClosureData)
  {
    mPostTransCallback = aCallback;
    mPostTransCallbackData = aClosureData;
  }

  



  void SetFilter(GraphicsFilter aFilter)
  {
    if (mFilter != aFilter) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) Filter", this));
      mFilter = aFilter;
      Mutated();
    }
  }
  GraphicsFilter GetFilter() const { return mFilter; }

  MOZ_LAYER_DECL_NAME("CanvasLayer", TYPE_CANVAS)

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override
  {
    
    
    
    
    mEffectiveTransform =
        SnapTransform(GetLocalTransform(), gfxRect(0, 0, mBounds.width, mBounds.height),
                      nullptr)*
        SnapTransformTranslation(aTransformToSurface, nullptr);
    ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  }

protected:
  CanvasLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData)
    , mPreTransCallback(nullptr)
    , mPreTransCallbackData(nullptr)
    , mPostTransCallback(nullptr)
    , mPostTransCallbackData(nullptr)
    , mFilter(GraphicsFilter::FILTER_GOOD)
    , mDirty(false)
  {}

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent) override;

  void FireDidTransactionCallback()
  {
    if (mPostTransCallback) {
      mPostTransCallback(mPostTransCallbackData);
    }
  }

  


  nsIntRect mBounds;
  PreTransactionCallback* mPreTransCallback;
  void* mPreTransCallbackData;
  DidTransactionCallback mPostTransCallback;
  void* mPostTransCallbackData;
  GraphicsFilter mFilter;

private:
  


  bool mDirty;
};


















class RefLayer : public ContainerLayer {
  friend class LayerManager;

private:
  virtual bool InsertAfter(Layer* aChild, Layer* aAfter) override
  { MOZ_CRASH(); return false; }

  virtual bool RemoveChild(Layer* aChild) override
  { MOZ_CRASH(); return false; }

  virtual bool RepositionChild(Layer* aChild, Layer* aAfter) override
  { MOZ_CRASH(); return false; }

  using Layer::SetFrameMetrics;

public:
  



  void SetReferentId(uint64_t aId)
  {
    MOZ_ASSERT(aId != 0);
    if (mId != aId) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ReferentId", this));
      mId = aId;
      Mutated();
    }
  }
  




  void ConnectReferentLayer(Layer* aLayer)
  {
    MOZ_ASSERT(!mFirstChild && !mLastChild);
    MOZ_ASSERT(!aLayer->GetParent());
    if (aLayer->Manager() != Manager()) {
      
      
      
      
      
      NS_WARNING("ConnectReferentLayer failed - Incorrect LayerManager");
      return;
    }

    mFirstChild = mLastChild = aLayer;
    aLayer->SetParent(this);
  }

  



  void DetachReferentLayer(Layer* aLayer)
  {
    mFirstChild = mLastChild = nullptr;
    aLayer->SetParent(nullptr);
  }

  
  virtual RefLayer* AsRefLayer() override { return this; }

  virtual int64_t GetReferentId() { return mId; }

  


  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) override;

  MOZ_LAYER_DECL_NAME("RefLayer", TYPE_REF)

protected:
  RefLayer(LayerManager* aManager, void* aImplData)
    : ContainerLayer(aManager, aImplData) , mId(0)
  {}

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent) override;

  Layer* mTempReferent;
  
  uint64_t mId;
};

void SetAntialiasingFlags(Layer* aLayer, gfx::DrawTarget* aTarget);

#ifdef MOZ_DUMP_PAINTING
void WriteSnapshotToDumpFile(Layer* aLayer, gfx::DataSourceSurface* aSurf);
void WriteSnapshotToDumpFile(LayerManager* aManager, gfx::DataSourceSurface* aSurf);
void WriteSnapshotToDumpFile(Compositor* aCompositor, gfx::DrawTarget* aTarget);
#endif


nsIntRect ToOutsideIntRect(const gfxRect &aRect);

}
}

#endif 
