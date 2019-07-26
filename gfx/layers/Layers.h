




#ifndef GFX_LAYERS_H
#define GFX_LAYERS_H

#include <stdint.h>                     
#include <stdio.h>                      
#include <sys/types.h>                  
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfx3DMatrix.h"                
#include "gfxContext.h"                 
#include "gfxTypes.h"
#include "gfxColor.h"                   
#include "gfxMatrix.h"                  
#include "GraphicsFilter.h"             
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
#include "mozilla/Assertions.h"         
#include "mozilla/DebugOnly.h"          
#include "mozilla/EventForwards.h"      
#include "mozilla/RefPtr.h"             
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
#include "nsStyleAnimation.h"           
#include "nsTArray.h"                   
#include "nsTArrayForwardDeclare.h"     
#include "nscore.h"                     
#include "prlog.h"                      

class gfxASurface;
class gfxContext;

extern uint8_t gLayerManagerLayerBuilder;

namespace mozilla {

class FrameLayerBuilder;
class WebGLContext;

namespace gl {
class GLContext;
}

namespace gfx {
class DrawTarget;
}

namespace css {
class ComputedTimingFunction;
}

namespace layers {

class Animation;
class AnimationData;
class AsyncPanZoomController;
class CommonLayerAttributes;
class Layer;
class ThebesLayer;
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

#define MOZ_LAYER_DECL_NAME(n, e)                           \
  virtual const char* Name() const { return n; }            \
  virtual LayerType GetType() const { return e; }




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

public:
  LayerManager()
    : mDestroyed(false)
    , mSnapEffectiveTransforms(true)
    , mId(0)
    , mInTransaction(false)
  {
    InitLog();
  }
  virtual ~LayerManager() {}

  





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

  



  virtual bool IsWidgetLayerManager() { return true; }

  





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

  

























  typedef void (* DrawThebesLayerCallback)(ThebesLayer* aLayer,
                                           gfxContext* aContext,
                                           const nsIntRegion& aRegionToDraw,
                                           DrawRegionClip aClip,
                                           const nsIntRegion& aRegionToInvalidate,
                                           void* aCallbackData);

  






  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) = 0;

  




  virtual void Composite() {}

  virtual bool HasShadowManagerInternal() const { return false; }
  bool HasShadowManager() const { return HasShadowManagerInternal(); }

  bool IsSnappingEffectiveTransforms() { return mSnapEffectiveTransforms; }

  




  virtual bool AreComponentAlphaLayersEnabled() { return true; }

  




  virtual void SetRoot(Layer* aLayer) = 0;
  


  Layer* GetRoot() { return mRoot; }

  




  Layer* GetPrimaryScrollableLayer();

  



  void GetScrollableLayers(nsTArray<Layer*>& aArray);

  





#ifdef DEBUG
  
  virtual void Mutated(Layer* aLayer);
#else
  virtual void Mutated(Layer* aLayer) { }
#endif

  






  enum ThebesLayerCreationHint {
    NONE, SCROLLABLE
  };

  



  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() = 0;
  




  virtual already_AddRefed<ThebesLayer> CreateThebesLayerWithHint(ThebesLayerCreationHint) {
    return CreateThebesLayer();
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

  



  virtual already_AddRefed<gfxASurface>
    CreateOptimalSurface(const gfx::IntSize &aSize,
                         gfxImageFormat imageFormat);

  





  virtual already_AddRefed<gfxASurface>
    CreateOptimalMaskSurface(const gfx::IntSize &aSize);

  



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

  



  void Dump(FILE* aFile=nullptr, const char* aPrefix="", bool aDumpHtml=false);
  



  void DumpSelf(FILE* aFile=nullptr, const char* aPrefix="");

  



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
    
    
    return LAYERS_BASIC != aBackend && LAYERS_NONE != aBackend;
  }

  virtual bool IsCompositingCheap() { return true; }

  bool IsInTransaction() const { return mInTransaction; }

protected:
  nsRefPtr<Layer> mRoot;
  gfx::UserData mUserData;
  bool mDestroyed;
  bool mSnapEffectiveTransforms;

  
  
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  static void InitLog();
  static PRLogModuleInfo* sLog;
  uint64_t mId;
  bool mInTransaction;
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
  InfallibleTArray<nsStyleAnimation::Value> mStartValues;
  InfallibleTArray<nsStyleAnimation::Value> mEndValues;
  InfallibleTArray<nsAutoPtr<mozilla::css::ComputedTimingFunction> > mFunctions;
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
    TYPE_THEBES
  };

  virtual ~Layer();

  




  LayerManager* Manager() { return mManager; }

  enum {
    




    CONTENT_OPAQUE = 0x01,
    






    CONTENT_COMPONENT_ALPHA = 0x02,

    



    CONTENT_PRESERVE_3D = 0x04,
    




    CONTENT_MAY_CHANGE_TRANSFORM = 0x08,

    



    CONTENT_DISABLE_SUBPIXEL_AA = 0x10
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
  












  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    if (!mVisibleRegion.IsEqual(aRegion)) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) VisibleRegion was %s is %s", this,
        mVisibleRegion.ToString().get(), aRegion.ToString().get()));
      mVisibleRegion = aRegion;
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

  void SetMixBlendMode(gfxContext::GraphicsOperator aMixBlendMode)
  {
    if (mMixBlendMode != aMixBlendMode) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) MixBlendMode", this));
      mMixBlendMode = aMixBlendMode;
      Mutated();
    }
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
      gfxMatrix maskTransform;
      bool maskIs2D = aMaskLayer->GetTransform().CanDraw2D(&maskTransform);
      NS_ASSERTION(maskIs2D, "Mask layer has invalid transform.");
    }
#endif

    if (mMaskLayer != aMaskLayer) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) MaskLayer", this));
      mMaskLayer = aMaskLayer;
      Mutated();
    }
  }

  




  void SetBaseTransform(const gfx3DMatrix& aMatrix)
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

  







  void SetBaseTransformForNextTransaction(const gfx3DMatrix& aMatrix)
  {
    mPendingTransform = new gfx3DMatrix(aMatrix);
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

  
  
  
  Animation* AddAnimation(mozilla::TimeStamp aStart, mozilla::TimeDuration aDuration,
                          float aIterations, int aDirection,
                          nsCSSProperty aProperty, const AnimationData& aData);
  
  void ClearAnimations();
  
  
  void SetAnimations(const AnimationArray& aAnimations);

  






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

  




  void SetScrollbarData(FrameMetrics::ViewID aScrollId, ScrollDirection aDir)
  {
    if (mScrollbarTargetId != aScrollId ||
        mScrollbarDirection != aDir) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ScrollbarData", this));
      mScrollbarTargetId = aScrollId;
      mScrollbarDirection = aDir;
      Mutated();
    }
  }

  
  float GetOpacity() { return mOpacity; }
  gfxContext::GraphicsOperator GetMixBlendMode() const { return mMixBlendMode; }
  const nsIntRect* GetClipRect() { return mUseClipRect ? &mClipRect : nullptr; }
  uint32_t GetContentFlags() { return mContentFlags; }
  const nsIntRegion& GetVisibleRegion() { return mVisibleRegion; }
  const EventRegions& GetEventRegions() const { return mEventRegions; }
  ContainerLayer* GetParent() { return mParent; }
  Layer* GetNextSibling() { return mNextSibling; }
  const Layer* GetNextSibling() const { return mNextSibling; }
  Layer* GetPrevSibling() { return mPrevSibling; }
  const Layer* GetPrevSibling() const { return mPrevSibling; }
  virtual Layer* GetFirstChild() const { return nullptr; }
  virtual Layer* GetLastChild() const { return nullptr; }
  const gfx3DMatrix GetTransform() const;
  const gfx3DMatrix& GetBaseTransform() const { return mTransform; }
  float GetPostXScale() const { return mPostXScale; }
  float GetPostYScale() const { return mPostYScale; }
  bool GetIsFixedPosition() { return mIsFixedPosition; }
  bool GetIsStickyPosition() { return mStickyPositionData; }
  LayerPoint GetFixedPositionAnchor() { return mAnchor; }
  const LayerMargin& GetFixedPositionMargins() { return mMargins; }
  FrameMetrics::ViewID GetStickyScrollContainerId() { return mStickyPositionData->mScrollId; }
  const LayerRect& GetStickyScrollRangeOuter() { return mStickyPositionData->mOuter; }
  const LayerRect& GetStickyScrollRangeInner() { return mStickyPositionData->mInner; }
  FrameMetrics::ViewID GetScrollbarTargetContainerId() { return mScrollbarTargetId; }
  ScrollDirection GetScrollbarDirection() { return mScrollbarDirection; }
  Layer* GetMaskLayer() const { return mMaskLayer; }

  
  
  AnimationArray& GetAnimations() { return mAnimations; }
  InfallibleTArray<AnimData>& GetAnimationData() { return mAnimationData; }

  uint64_t GetAnimationGeneration() { return mAnimationGeneration; }
  void SetAnimationGeneration(uint64_t aCount) { mAnimationGeneration = aCount; }

  



  const gfx3DMatrix GetLocalTransform();

  



  const float GetLocalOpacity();

  





  void ApplyPendingUpdatesToSubtree();

  





  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs) { }

  
  
  
  
  
  bool CanUseOpaqueSurface();

  enum SurfaceMode {
    SURFACE_OPAQUE,
    SURFACE_SINGLE_CHANNEL_ALPHA,
    SURFACE_COMPONENT_ALPHA
  };
  SurfaceMode GetSurfaceMode()
  {
    if (CanUseOpaqueSurface())
      return SURFACE_OPAQUE;
    if (mContentFlags & CONTENT_COMPONENT_ALPHA)
      return SURFACE_COMPONENT_ALPHA;
    return SURFACE_SINGLE_CHANNEL_ALPHA;
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

  



  virtual ThebesLayer* AsThebesLayer() { return nullptr; }

  



  virtual ContainerLayer* AsContainerLayer() { return nullptr; }
  virtual const ContainerLayer* AsContainerLayer() const { return nullptr; }

   



  virtual RefLayer* AsRefLayer() { return nullptr; }

   



  virtual ColorLayer* AsColorLayer() { return nullptr; }

  



  virtual LayerComposite* AsLayerComposite() { return nullptr; }

  



  virtual ShadowableLayer* AsShadowableLayer() { return nullptr; }

  
  
  
  const nsIntRect* GetEffectiveClipRect();
  const nsIntRegion& GetEffectiveVisibleRegion();

  



  float GetEffectiveOpacity();
  
  


  gfxContext::GraphicsOperator GetEffectiveMixBlendMode();
  
  







  const gfx3DMatrix& GetEffectiveTransform() const { return mEffectiveTransform; }

  










  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) = 0;

  


  void ComputeEffectiveTransformForMaskLayer(const gfx3DMatrix& aTransformToSurface);

  









  nsIntRect CalculateScissorRect(const nsIntRect& aCurrentScissorRect,
                                 const gfxMatrix* aWorldTransform);

  virtual const char* Name() const =0;
  virtual LayerType GetType() const =0;

  




  void* ImplData() { return mImplData; }

  


  void SetParent(ContainerLayer* aParent) { mParent = aParent; }
  void SetNextSibling(Layer* aSibling) { mNextSibling = aSibling; }
  void SetPrevSibling(Layer* aSibling) { mPrevSibling = aSibling; }

  



  void Dump(FILE* aFile=nullptr, const char* aPrefix="", bool aDumpHtml=false);
  



  void DumpSelf(FILE* aFile=nullptr, const char* aPrefix="");

  



  void Log(const char* aPrefix="");
  



  void LogSelf(const char* aPrefix="");

  static bool IsLogEnabled() { return LayerManager::IsLogEnabled(); }

  



  const nsIntRegion& GetInvalidRegion() { return mInvalidRegion; }
  const void SetInvalidRegion(const nsIntRegion& aRect) { mInvalidRegion = aRect; }

  


  void SetInvalidRectToVisibleRegion() { mInvalidRegion = GetVisibleRegion(); }

  


  void AddInvalidRect(const nsIntRect& aRect) { mInvalidRegion.Or(mInvalidRegion, aRect); }

  



  void ClearInvalidRect() { mInvalidRegion.SetEmpty(); }

  void ApplyPendingUpdatesForThisTransaction();

#ifdef DEBUG
  void SetDebugColorIndex(uint32_t aIndex) { mDebugColorIndex = aIndex; }
  uint32_t GetDebugColorIndex() { return mDebugColorIndex; }
#endif

  virtual LayerRenderState GetRenderState() { return LayerRenderState(); }

protected:
  Layer(LayerManager* aManager, void* aImplData);

  void Mutated()
  {
    mManager->Mutated(this);
  }

  
  
  
  
  
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  
























  gfx3DMatrix SnapTransformTranslation(const gfx3DMatrix& aTransform,
                                       gfxMatrix* aResidualTransform);
  










  gfx3DMatrix SnapTransform(const gfx3DMatrix& aTransform,
                            const gfxRect& aSnapRect,
                            gfxMatrix* aResidualTransform);

  





  bool MayResample();

  LayerManager* mManager;
  ContainerLayer* mParent;
  Layer* mNextSibling;
  Layer* mPrevSibling;
  void* mImplData;
  nsRefPtr<Layer> mMaskLayer;
  gfx::UserData mUserData;
  nsIntRegion mVisibleRegion;
  EventRegions mEventRegions;
  gfx3DMatrix mTransform;
  
  
  
  nsAutoPtr<gfx3DMatrix> mPendingTransform;
  float mPostXScale;
  float mPostYScale;
  gfx3DMatrix mEffectiveTransform;
  AnimationArray mAnimations;
  InfallibleTArray<AnimData> mAnimationData;
  float mOpacity;
  gfxContext::GraphicsOperator mMixBlendMode;
  bool mForceIsolatedGroup;
  nsIntRect mClipRect;
  nsIntRect mTileSourceRect;
  nsIntRegion mInvalidRegion;
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
  DebugOnly<uint32_t> mDebugColorIndex;
  
  
  uint64_t mAnimationGeneration;
};












class ThebesLayer : public Layer {
public:
  





  virtual void InvalidateRegion(const nsIntRegion& aRegion) = 0;
  











  void SetAllowResidualTranslation(bool aAllow) { mAllowResidualTranslation = aAllow; }

  


  const nsIntRegion& GetValidRegion() const { return mValidRegion; }

  virtual ThebesLayer* AsThebesLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ThebesLayer", TYPE_THEBES)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    gfx3DMatrix idealTransform = GetLocalTransform()*aTransformToSurface;
    gfxMatrix residual;
    mEffectiveTransform = SnapTransformTranslation(idealTransform,
        mAllowResidualTranslation ? &residual : nullptr);
    
    
    NS_ASSERTION(!residual.HasNonTranslation(),
                 "Residual transform can only be a translation");
    if (!residual.GetTranslation().WithinEpsilonOf(mResidualTranslation, 1e-3f)) {
      mResidualTranslation = residual.GetTranslation();
      NS_ASSERTION(-0.5 <= mResidualTranslation.x && mResidualTranslation.x < 0.5 &&
                   -0.5 <= mResidualTranslation.y && mResidualTranslation.y < 0.5,
                   "Residual translation out of range");
      mValidRegion.SetEmpty();
    }
    ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  }

  bool UsedForReadback() { return mUsedForReadback; }
  void SetUsedForReadback(bool aUsed) { mUsedForReadback = aUsed; }
  







  gfxPoint GetResidualTranslation() const { return mResidualTranslation; }

protected:
  ThebesLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData)
    , mValidRegion()
    , mUsedForReadback(false)
    , mAllowResidualTranslation(false)
  {
    mContentFlags = 0; 
  }

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  




  gfxPoint mResidualTranslation;
  nsIntRegion mValidRegion;
  



  bool mUsedForReadback;
  


  bool mAllowResidualTranslation;
};





class ContainerLayer : public Layer {
public:

  ~ContainerLayer();

  






  virtual void InsertAfter(Layer* aChild, Layer* aAfter);
  




  virtual void RemoveChild(Layer* aChild);
  






  virtual void RepositionChild(Layer* aChild, Layer* aAfter);

  




  void SetFrameMetrics(const FrameMetrics& aFrameMetrics)
  {
    if (mFrameMetrics != aFrameMetrics) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) FrameMetrics", this));
      mFrameMetrics = aFrameMetrics;
      Mutated();
    }
  }

  
  
  
  void SetAsyncPanZoomController(AsyncPanZoomController *controller);
  AsyncPanZoomController* GetAsyncPanZoomController() const;

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

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs);

  void SortChildrenBy3DZOrder(nsTArray<Layer*>& aArray);

  

  virtual ContainerLayer* AsContainerLayer() { return this; }
  virtual const ContainerLayer* AsContainerLayer() const { return this; }

  virtual Layer* GetFirstChild() const { return mFirstChild; }
  virtual Layer* GetLastChild() const { return mLastChild; }
  const FrameMetrics& GetFrameMetrics() const { return mFrameMetrics; }
  float GetPreXScale() const { return mPreXScale; }
  float GetPreYScale() const { return mPreYScale; }
  float GetInheritedXScale() const { return mInheritedXScale; }
  float GetInheritedYScale() const { return mInheritedYScale; }

  MOZ_LAYER_DECL_NAME("ContainerLayer", TYPE_CONTAINER)

  





  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) = 0;

  





  bool UseIntermediateSurface() { return mUseIntermediateSurface; }

  



  nsIntRect GetIntermediateSurfaceRect()
  {
    NS_ASSERTION(mUseIntermediateSurface, "Must have intermediate surface");
    return mVisibleRegion.GetBounds();
  }

  


  bool HasMultipleChildren();

  



  bool SupportsComponentAlphaChildren() { return mSupportsComponentAlphaChildren; }

protected:
  friend class ReadbackProcessor;

  static bool HasOpaqueAncestorLayer(Layer* aLayer);

  void DidInsertChild(Layer* aLayer);
  void DidRemoveChild(Layer* aLayer);

  ContainerLayer(LayerManager* aManager, void* aImplData);

  



  void DefaultComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface);

  


  void ComputeEffectiveTransformsForChildren(const gfx3DMatrix& aTransformToSurface);

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  Layer* mFirstChild;
  Layer* mLastChild;
  FrameMetrics mFrameMetrics;
  nsRefPtr<AsyncPanZoomController> mAPZC;
  float mPreXScale;
  float mPreYScale;
  
  
  float mInheritedXScale;
  float mInheritedYScale;
  bool mUseIntermediateSurface;
  bool mSupportsComponentAlphaChildren;
  bool mMayHaveReadbackChild;
};






class ColorLayer : public Layer {
public:
  virtual ColorLayer* AsColorLayer() { return this; }

  



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

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    gfx3DMatrix idealTransform = GetLocalTransform()*aTransformToSurface;
    mEffectiveTransform = SnapTransformTranslation(idealTransform, nullptr);
    ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
  }

protected:
  ColorLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mColor(0.0, 0.0, 0.0, 0.0)
  {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  nsIntRect mBounds;
  gfxRGBA mColor;
};











class CanvasLayer : public Layer {
public:
  struct Data {
    Data()
      : mSurface(nullptr)
      , mDrawTarget(nullptr)
      , mGLContext(nullptr)
      , mSize(0,0)
      , mIsGLAlphaPremult(false)
    { }

    
    gfxASurface* mSurface;  
    mozilla::gfx::DrawTarget *mDrawTarget; 

    
    mozilla::gl::GLContext* mGLContext;

    
    nsIntSize mSize;

    
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

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
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

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

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
  virtual void InsertAfter(Layer* aChild, Layer* aAfter)
  { MOZ_CRASH(); }

  virtual void RemoveChild(Layer* aChild)
  { MOZ_CRASH(); }

  virtual void RepositionChild(Layer* aChild, Layer* aAfter)
  { MOZ_CRASH(); }

  using ContainerLayer::SetFrameMetrics;

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
    MOZ_ASSERT(aLayer->Manager() == Manager());

    mFirstChild = mLastChild = aLayer;
    aLayer->SetParent(this);
  }

  



  void DetachReferentLayer(Layer* aLayer)
  {
    MOZ_ASSERT(aLayer == mFirstChild && mFirstChild == mLastChild);
    MOZ_ASSERT(aLayer->GetParent() == this);

    mFirstChild = mLastChild = nullptr;
    aLayer->SetParent(nullptr);
  }

  
  virtual RefLayer* AsRefLayer() { return this; }

  virtual int64_t GetReferentId() { return mId; }

  


  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs);

  MOZ_LAYER_DECL_NAME("RefLayer", TYPE_REF)

protected:
  RefLayer(LayerManager* aManager, void* aImplData)
    : ContainerLayer(aManager, aImplData) , mId(0)
  {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  Layer* mTempReferent;
  
  uint64_t mId;
};

void
SetAntialiasingFlags(Layer* aLayer, gfxContext* aTarget);

#ifdef MOZ_DUMP_PAINTING
void WriteSnapshotToDumpFile(Layer* aLayer, gfxASurface* aSurf);
void WriteSnapshotToDumpFile(LayerManager* aManager, gfxASurface* aSurf);
void WriteSnapshotToDumpFile(Compositor* aCompositor, gfx::DrawTarget* aTarget);
#endif

}
}

#endif 
