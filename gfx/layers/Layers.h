




#ifndef GFX_LAYERS_H
#define GFX_LAYERS_H

#include "mozilla/DebugOnly.h"

#include "gfxTypes.h"
#include "gfxASurface.h"
#include "nsRegion.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "gfx3DMatrix.h"
#include "gfxColor.h"
#include "gfxPattern.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsStyleAnimation.h"
#include "LayersTypes.h"
#include "FrameMetrics.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/TimeStamp.h"

#if defined(DEBUG) || defined(PR_LOGGING)
#  include <stdio.h>            
#  include "prlog.h"
#  ifndef MOZ_LAYERS_HAVE_LOG
#    define MOZ_LAYERS_HAVE_LOG
#  endif
#  define MOZ_LAYERS_LOG(_args)                             \
  PR_LOG(LayerManager::GetLog(), PR_LOG_DEBUG, _args)
#else
struct PRLogModuleInfo;
#  define MOZ_LAYERS_LOG(_args)
#endif  

class gfxContext;
class nsPaintEvent;

extern uint8_t gLayerManagerLayerBuilder;

namespace mozilla {

class FrameLayerBuilder;

namespace gl {
class GLContext;
}

namespace css {
class ComputedTimingFunction;
}

namespace layers {

class Animation;
class AnimationData;
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
class ShadowLayer;
class ShadowableLayer;
class ShadowLayerForwarder;
class ShadowLayerManager;
class SpecificLayerAttributes;

#define MOZ_LAYER_DECL_NAME(n, e)                           \
  virtual const char* Name() const { return n; }            \
  virtual LayerType GetType() const { return e; }




class THEBES_API LayerUserData {
public:
  virtual ~LayerUserData() {}
};



























static void LayerManagerUserDataDestroy(void *data)
{
  delete static_cast<LayerUserData*>(data);
}
























class THEBES_API LayerManager {
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

  





  virtual void Destroy() { mDestroyed = true; mUserData.Destroy(); }
  bool IsDestroyed() { return mDestroyed; }

  virtual ShadowLayerForwarder* AsShadowForwarder()
  { return nullptr; }

  virtual ShadowLayerManager* AsShadowManager()
  { return nullptr; }

  



  virtual bool IsWidgetLayerManager() { return true; }

  





  virtual void BeginTransaction() = 0;
  






  virtual void BeginTransactionWithTarget(gfxContext* aTarget) = 0;

  enum EndTransactionFlags {
    END_DEFAULT = 0,
    END_NO_IMMEDIATE_REDRAW = 1 << 0,  
    END_NO_COMPOSITE = 1 << 1 
  };

  FrameLayerBuilder* GetLayerBuilder() {
    return reinterpret_cast<FrameLayerBuilder*>(GetUserData(&gLayerManagerLayerBuilder));
  }

  







  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) = 0;

  

























  typedef void (* DrawThebesLayerCallback)(ThebesLayer* aLayer,
                                           gfxContext* aContext,
                                           const nsIntRegion& aRegionToDraw,
                                           const nsIntRegion& aRegionToInvalidate,
                                           void* aCallbackData);

  






  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) = 0;

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

  



  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() = 0;
  



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
    CreateOptimalSurface(const gfxIntSize &aSize,
                         gfxASurface::gfxImageFormat imageFormat);
 
  





  virtual already_AddRefed<gfxASurface>
    CreateOptimalMaskSurface(const gfxIntSize &aSize);

  



  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize &aSize,
                     mozilla::gfx::SurfaceFormat aFormat);

  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize) { return true; }

  



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
  



  LayerUserData* GetUserData(void* aKey)
  { 
    return static_cast<LayerUserData*>(mUserData.Get(static_cast<gfx::UserDataKey*>(aKey)));
  }

  















  virtual void ClearCachedResources(Layer* aSubtree = nullptr) {}

  


  virtual void SetIsFirstPaint() {}

  
  
  
  
  virtual const char* Name() const { return "???"; }

  



  void Dump(FILE* aFile=NULL, const char* aPrefix="", bool aDumpHtml=false);
  



  void DumpSelf(FILE* aFile=NULL, const char* aPrefix="");

  



  void Log(const char* aPrefix="");
  



  void LogSelf(const char* aPrefix="");

  











  


  uint32_t StartFrameTimeRecording();

  



  void StopFrameTimeRecording(uint32_t         aStartIndex,
                              nsTArray<float>& aFrameIntervals,
                              nsTArray<float>& aPaintTimes);

  void SetPaintStartTime(TimeStamp& aTime);

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
    TimeStamp mPaintStartTime;
    nsTArray<float> mIntervals;
    nsTArray<float> mPaints;
    uint32_t mLatestStartIndex;
    uint32_t mCurrentRunStartIndex;
  };
  FramesTimingRecording mRecording;

  TimeStamp mTabSwitchStart;
};

class ThebesLayer;
typedef InfallibleTArray<Animation> AnimationArray;

struct AnimData {
  InfallibleTArray<nsStyleAnimation::Value> mStartValues;
  InfallibleTArray<nsStyleAnimation::Value> mEndValues;
  InfallibleTArray<mozilla::css::ComputedTimingFunction*> mFunctions;
};





class THEBES_API Layer {
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
    




    CONTENT_MAY_CHANGE_TRANSFORM = 0x08
  };
  





  void SetContentFlags(uint32_t aFlags)
  {
    NS_ASSERTION((aFlags & (CONTENT_OPAQUE | CONTENT_COMPONENT_ALPHA)) !=
                 (CONTENT_OPAQUE | CONTENT_COMPONENT_ALPHA),
                 "Can't be opaque and require component alpha");
    if (mContentFlags != aFlags) {
      mContentFlags = aFlags;
      Mutated();
    }
  }
  












  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    if (!mVisibleRegion.IsEqual(aRegion)) {
      mVisibleRegion = aRegion;
      Mutated();
    }
  }

  




  void SetOpacity(float aOpacity)
  {
    if (mOpacity != aOpacity) {
      mOpacity = aOpacity;
      Mutated();
    }
  }

  









  void SetClipRect(const nsIntRect* aRect)
  {
    if (mUseClipRect) {
      if (!aRect) {
        mUseClipRect = false;
        Mutated();
      } else {
        if (!aRect->IsEqualEdges(mClipRect)) {
          mClipRect = *aRect;
          Mutated();
        }
      }
    } else {
      if (aRect) {
        Mutated();
        mUseClipRect = true;
        if (!aRect->IsEqualEdges(mClipRect)) {
          mClipRect = *aRect;
        }
      }
    }
  }

  









  void IntersectClipRect(const nsIntRect& aRect)
  {
    if (mUseClipRect) {
      mClipRect.IntersectRect(mClipRect, aRect);
    } else {
      mUseClipRect = true;
      mClipRect = aRect;
    }
    Mutated();
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
    mPostXScale = aXScale;
    mPostYScale = aYScale;
    Mutated();
  }

  





  void SetIsFixedPosition(bool aFixedPosition) { mIsFixedPosition = aFixedPosition; }

  
  
  
  Animation* AddAnimation(mozilla::TimeStamp aStart, mozilla::TimeDuration aDuration,
                          float aIterations, int aDirection,
                          nsCSSProperty aProperty, const AnimationData& aData);
  
  void ClearAnimations();
  
  
  void SetAnimations(const AnimationArray& aAnimations);

  






  void SetFixedPositionAnchor(const gfxPoint& aAnchor) { mAnchor = aAnchor; }

  
  float GetOpacity() { return mOpacity; }
  const nsIntRect* GetClipRect() { return mUseClipRect ? &mClipRect : nullptr; }
  uint32_t GetContentFlags() { return mContentFlags; }
  const nsIntRegion& GetVisibleRegion() { return mVisibleRegion; }
  ContainerLayer* GetParent() { return mParent; }
  Layer* GetNextSibling() { return mNextSibling; }
  Layer* GetPrevSibling() { return mPrevSibling; }
  virtual Layer* GetFirstChild() { return nullptr; }
  virtual Layer* GetLastChild() { return nullptr; }
  const gfx3DMatrix GetTransform();
  const gfx3DMatrix& GetBaseTransform() { return mTransform; }
  float GetPostXScale() { return mPostXScale; }
  float GetPostYScale() { return mPostYScale; }
  bool GetIsFixedPosition() { return mIsFixedPosition; }
  gfxPoint GetFixedPositionAnchor() { return mAnchor; }
  Layer* GetMaskLayer() { return mMaskLayer; }

  
  
  AnimationArray& GetAnimations() { return mAnimations; }
  InfallibleTArray<AnimData>& GetAnimationData() { return mAnimationData; }

  uint64_t GetAnimationGeneration() { return mAnimationGeneration; }
  void SetAnimationGeneration(uint64_t aCount) { mAnimationGeneration = aCount; }

  





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
  



  LayerUserData* GetUserData(void* aKey)
  { 
    return static_cast<LayerUserData*>(mUserData.Get(static_cast<gfx::UserDataKey*>(aKey)));
  }

  








  virtual void Disconnect() {}

  



  virtual ThebesLayer* AsThebesLayer() { return nullptr; }

  



  virtual ContainerLayer* AsContainerLayer() { return nullptr; }

   



  virtual RefLayer* AsRefLayer() { return nullptr; }

   



  virtual ColorLayer* AsColorLayer() { return nullptr; }

  



  virtual ShadowLayer* AsShadowLayer() { return nullptr; }

  



  virtual ShadowableLayer* AsShadowableLayer() { return nullptr; }

  
  
  
  const nsIntRect* GetEffectiveClipRect();
  const nsIntRegion& GetEffectiveVisibleRegion();
  



  float GetEffectiveOpacity();
  







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

  



  void Dump(FILE* aFile=NULL, const char* aPrefix="", bool aDumpHtml=false);
  



  void DumpSelf(FILE* aFile=NULL, const char* aPrefix="");

  



  void Log(const char* aPrefix="");
  



  void LogSelf(const char* aPrefix="");

  static bool IsLogEnabled() { return LayerManager::IsLogEnabled(); }

  



  const nsIntRegion& GetInvalidRegion() { return mInvalidRegion; }

  


  void SetInvalidRectToVisibleRegion() { mInvalidRegion = GetVisibleRegion(); }

  


  void AddInvalidRect(const nsIntRect& aRect) { mInvalidRegion.Or(mInvalidRegion, aRect); }

  



  void ClearInvalidRect() { mInvalidRegion.SetEmpty(); }

  void ApplyPendingUpdatesForThisTransaction();

#ifdef DEBUG
  void SetDebugColorIndex(uint32_t aIndex) { mDebugColorIndex = aIndex; }
  uint32_t GetDebugColorIndex() { return mDebugColorIndex; }
#endif

protected:
  Layer(LayerManager* aManager, void* aImplData);

  void Mutated() { mManager->Mutated(this); }

  
  
  
  
  
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  



  const gfx3DMatrix GetLocalTransform();

  



  const float GetLocalOpacity();

  
























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
  gfx3DMatrix mTransform;
  
  
  
  nsAutoPtr<gfx3DMatrix> mPendingTransform;
  float mPostXScale;
  float mPostYScale;
  gfx3DMatrix mEffectiveTransform;
  AnimationArray mAnimations;
  InfallibleTArray<AnimData> mAnimationData;
  float mOpacity;
  nsIntRect mClipRect;
  nsIntRect mTileSourceRect;
  nsIntRegion mInvalidRegion;
  uint32_t mContentFlags;
  bool mUseClipRect;
  bool mUseTileSourceRect;
  bool mIsFixedPosition;
  gfxPoint mAnchor;
  DebugOnly<uint32_t> mDebugColorIndex;
  
  
  uint64_t mAnimationGeneration;
};












class THEBES_API ThebesLayer : public Layer {
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





class THEBES_API ContainerLayer : public Layer {
public:
  






  virtual void InsertAfter(Layer* aChild, Layer* aAfter) = 0;
  




  virtual void RemoveChild(Layer* aChild) = 0;
  






  virtual void RepositionChild(Layer* aChild, Layer* aAfter) = 0;

  




  void SetFrameMetrics(const FrameMetrics& aFrameMetrics)
  {
    if (mFrameMetrics != aFrameMetrics) {
      mFrameMetrics = aFrameMetrics;
      Mutated();
    }
  }

  void SetPreScale(float aXScale, float aYScale)
  {
    if (mPreXScale == aXScale && mPreYScale == aYScale) {
      return;
    }

    mPreXScale = aXScale;
    mPreYScale = aYScale;
    Mutated();
  }

  void SetInheritedScale(float aXScale, float aYScale)
  { 
    if (mInheritedXScale == aXScale && mInheritedYScale == aYScale) {
      return;
    }

    mInheritedXScale = aXScale;
    mInheritedYScale = aYScale;
    Mutated();
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs);

  void SortChildrenBy3DZOrder(nsTArray<Layer*>& aArray);

  

  virtual ContainerLayer* AsContainerLayer() { return this; }

  virtual Layer* GetFirstChild() { return mFirstChild; }
  virtual Layer* GetLastChild() { return mLastChild; }
  const FrameMetrics& GetFrameMetrics() { return mFrameMetrics; }
  float GetPreXScale() { return mPreXScale; }
  float GetPreYScale() { return mPreYScale; }
  float GetInheritedXScale() { return mInheritedXScale; }
  float GetInheritedYScale() { return mInheritedYScale; }

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

  void DidInsertChild(Layer* aLayer);
  void DidRemoveChild(Layer* aLayer);

  ContainerLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData),
      mFirstChild(nullptr),
      mLastChild(nullptr),
      mPreXScale(1.0f),
      mPreYScale(1.0f),
      mInheritedXScale(1.0f),
      mInheritedYScale(1.0f),
      mUseIntermediateSurface(false),
      mSupportsComponentAlphaChildren(false),
      mMayHaveReadbackChild(false)
  {
    mContentFlags = 0; 
  }

  



  void DefaultComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface);

  


  void ComputeEffectiveTransformsForChildren(const gfx3DMatrix& aTransformToSurface);

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  Layer* mFirstChild;
  Layer* mLastChild;
  FrameMetrics mFrameMetrics;
  float mPreXScale;
  float mPreYScale;
  
  
  float mInheritedXScale;
  float mInheritedYScale;
  bool mUseIntermediateSurface;
  bool mSupportsComponentAlphaChildren;
  bool mMayHaveReadbackChild;
};






class THEBES_API ColorLayer : public Layer {
public:
  virtual ColorLayer* AsColorLayer() { return this; }

  



  virtual void SetColor(const gfxRGBA& aColor)
  {
    mColor = aColor;
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

  gfxRGBA mColor;
};











class THEBES_API CanvasLayer : public Layer {
public:
  struct Data {
    Data()
      : mSurface(nullptr), mGLContext(nullptr)
      , mDrawTarget(nullptr), mGLBufferIsPremultiplied(false)
    { }

    
    gfxASurface* mSurface;  
    mozilla::gl::GLContext* mGLContext; 
    mozilla::gfx::DrawTarget *mDrawTarget; 

    
    nsIntSize mSize;

    


    bool mGLBufferIsPremultiplied;
  };

  







  virtual void Initialize(const Data& aData) = 0;

  



  void Updated() { mDirty = true; SetInvalidRectToVisibleRegion(); }

  



  void Painted() { mDirty = false; }

  



  bool IsDirty() 
  { 
    
    
    if (!mManager || !mManager->IsWidgetLayerManager()) {
      return true;
    }
    return mDirty; 
  }

  


  typedef void (* DidTransactionCallback)(void* aClosureData);
  void SetDidTransactionCallback(DidTransactionCallback aCallback, void* aClosureData)
  {
    mCallback = aCallback;
    mCallbackData = aClosureData;
  }

  



  void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }
  gfxPattern::GraphicsFilter GetFilter() const { return mFilter; }

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
    : Layer(aManager, aImplData),
      mCallback(nullptr), mCallbackData(nullptr), mFilter(gfxPattern::FILTER_GOOD),
      mDirty(false) {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  void FireDidTransactionCallback()
  {
    if (mCallback) {
      mCallback(mCallbackData);
    }
  }

  


  nsIntRect mBounds;
  DidTransactionCallback mCallback;
  void* mCallbackData;
  gfxPattern::GraphicsFilter mFilter;

private:
  


  bool mDirty;
};


















class THEBES_API RefLayer : public ContainerLayer {
  friend class LayerManager;

private:
  virtual void InsertAfter(Layer* aChild, Layer* aAfter)
  { MOZ_NOT_REACHED("no"); }

  virtual void RemoveChild(Layer* aChild)
  { MOZ_NOT_REACHED("no"); }

  virtual void RepositionChild(Layer* aChild, Layer* aAfter)
  { MOZ_NOT_REACHED("no"); }

  using ContainerLayer::SetFrameMetrics;

public:
  



  void SetReferentId(uint64_t aId)
  {
    MOZ_ASSERT(aId != 0);
    if (mId != aId) {
      mId = aId;
      Mutated();
    }
  }
  




  void ConnectReferentLayer(Layer* aLayer)
  {
    MOZ_ASSERT(!mFirstChild && !mLastChild);
    MOZ_ASSERT(!aLayer->GetParent());

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

#ifdef MOZ_DUMP_PAINTING
void WriteSnapshotToDumpFile(Layer* aLayer, gfxASurface* aSurf);
void WriteSnapshotToDumpFile(LayerManager* aManager, gfxASurface* aSurf);
#endif

}
}

#endif 
