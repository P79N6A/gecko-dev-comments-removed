




#ifndef GFX_LayerManagerComposite_H
#define GFX_LayerManagerComposite_H

#include <stdint.h>                     
#include "GLDefs.h"                     
#include "Layers.h"
#include "Units.h"                      
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/Maybe.h"              
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "nsAString.h"
#include "nsRefPtr.h"                   
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nscore.h"                     
#include "LayerTreeInvalidation.h"

class gfxContext;

#ifdef XP_WIN
#include <windows.h>
#endif

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace layers {

class CanvasLayerComposite;
class ColorLayerComposite;
class CompositableHost;
class Compositor;
class ContainerLayerComposite;
struct EffectChain;
class ImageLayer;
class ImageLayerComposite;
class LayerComposite;
class RefLayerComposite;
class PaintedLayerComposite;
class TiledLayerComposer;
class TextRenderer;
class CompositingRenderTarget;
struct FPSState;

static const int kVisualWarningDuration = 150; 

class LayerManagerComposite final : public LayerManager
{
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::IntSize IntSize;
  typedef mozilla::gfx::SurfaceFormat SurfaceFormat;

public:
  explicit LayerManagerComposite(Compositor* aCompositor);
  ~LayerManagerComposite();

  virtual void Destroy() override;

  


  bool Initialize();

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion)
  {
    mClippingRegion = aClippingRegion;
  }

  


  virtual LayerManagerComposite* AsLayerManagerComposite() override
  {
    return this;
  }

  void UpdateRenderBounds(const gfx::IntRect& aRect);

  virtual void BeginTransaction() override;
  virtual void BeginTransactionWithTarget(gfxContext* aTarget) override
  {
    MOZ_CRASH("Use BeginTransactionWithDrawTarget");
  }
  void BeginTransactionWithDrawTarget(gfx::DrawTarget* aTarget, const gfx::IntRect& aRect);

  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) override;
  virtual void EndTransaction(DrawPaintedLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) override;

  virtual void SetRoot(Layer* aLayer) override { mRoot = aLayer; }

  
  
  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize) override;

  virtual int32_t GetMaxTextureSize() const override
  {
    MOZ_CRASH("Call on compositor, not LayerManagerComposite");
  }

  virtual void ClearCachedResources(Layer* aSubtree = nullptr) override;

  virtual already_AddRefed<PaintedLayer> CreatePaintedLayer() override;
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() override;
  virtual already_AddRefed<ImageLayer> CreateImageLayer() override;
  virtual already_AddRefed<ColorLayer> CreateColorLayer() override;
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() override;
  already_AddRefed<PaintedLayerComposite> CreatePaintedLayerComposite();
  already_AddRefed<ContainerLayerComposite> CreateContainerLayerComposite();
  already_AddRefed<ImageLayerComposite> CreateImageLayerComposite();
  already_AddRefed<ColorLayerComposite> CreateColorLayerComposite();
  already_AddRefed<CanvasLayerComposite> CreateCanvasLayerComposite();
  already_AddRefed<RefLayerComposite> CreateRefLayerComposite();

  virtual LayersBackend GetBackendType() override
  {
    MOZ_CRASH("Shouldn't be called for composited layer manager");
  }
  virtual void GetBackendName(nsAString& name) override
  {
    MOZ_CRASH("Shouldn't be called for composited layer manager");
  }

  virtual bool AreComponentAlphaLayersEnabled() override;

  virtual already_AddRefed<DrawTarget>
    CreateOptimalMaskDrawTarget(const IntSize &aSize) override;

  virtual const char* Name() const override { return ""; }

  




  void ApplyOcclusionCulling(Layer* aLayer, nsIntRegion& aOpaqueRegion);

  



  class AutoAddMaskEffect
  {
  public:
    AutoAddMaskEffect(Layer* aMaskLayer,
                      EffectChain& aEffect,
                      bool aIs3D = false);
    ~AutoAddMaskEffect();

    bool Failed() const { return mFailed; }
  private:
    CompositableHost* mCompositable;
    bool mFailed;
  };

  



  virtual already_AddRefed<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize& aSize,
                     mozilla::gfx::SurfaceFormat aFormat) override;

  






  float ComputeRenderIntegrity();

  



  static bool SupportsDirectTexturing();

  static void PlatformSyncBeforeReplyUpdate();

  void AddInvalidRegion(const nsIntRegion& aRegion)
  {
    mInvalidRegion.Or(mInvalidRegion, aRegion);
  }

  Compositor* GetCompositor() const
  {
    return mCompositor;
  }

  



  bool DebugOverlayWantsNextFrame() { return mDebugOverlayWantsNextFrame; }
  void SetDebugOverlayWantsNextFrame(bool aVal)
  { mDebugOverlayWantsNextFrame = aVal; }

  void NotifyShadowTreeTransaction();

  TextRenderer* GetTextRenderer() { return mTextRenderer; }

  



  void VisualFrameWarning(float severity) {
    mozilla::TimeStamp now = TimeStamp::Now();
    if (mWarnTime.IsNull() ||
        severity > mWarningLevel ||
        mWarnTime + TimeDuration::FromMilliseconds(kVisualWarningDuration) < now) {
      mWarnTime = now;
      mWarningLevel = severity;
    }
  }

  void UnusedApzTransformWarning() {
    mUnusedApzTransformWarning = true;
  }

  bool LastFrameMissedHWC() { return mLastFrameMissedHWC; }

  bool AsyncPanZoomEnabled() const override;

private:
  
  nsIntRegion mClippingRegion;
  gfx::IntRect mRenderBounds;

  
  LayerComposite* RootLayer() const;

  





  static void ComputeRenderIntegrityInternal(Layer* aLayer,
                                             nsIntRegion& aScreenRegion,
                                             nsIntRegion& aLowPrecisionScreenRegion,
                                             const gfx::Matrix4x4& aTransform);

  


  void Render();
#ifdef MOZ_WIDGET_ANDROID
  void RenderToPresentationSurface();
#endif

  


  void RenderDebugOverlay(const gfx::Rect& aBounds);


  RefPtr<CompositingRenderTarget> PushGroupForLayerEffects();
  void PopGroupForLayerEffects(RefPtr<CompositingRenderTarget> aPreviousTarget,
                               gfx::IntRect aClipRect,
                               bool aGrayscaleEffect,
                               bool aInvertEffect,
                               float aContrastEffect);

  float mWarningLevel;
  mozilla::TimeStamp mWarnTime;
  bool mUnusedApzTransformWarning;
  RefPtr<Compositor> mCompositor;
  UniquePtr<LayerProperties> mClonedLayerTreeProperties;

  


  RefPtr<gfx::DrawTarget> mTarget;
  gfx::IntRect mTargetBounds;

  nsIntRegion mInvalidRegion;
  UniquePtr<FPSState> mFPS;

  bool mInTransaction;
  bool mIsCompositorReady;
  bool mDebugOverlayWantsNextFrame;

  RefPtr<CompositingRenderTarget> mTwoPassTmpTarget;
  RefPtr<TextRenderer> mTextRenderer;
  bool mGeometryChanged;

  
  
  bool mLastFrameMissedHWC;
};



















class LayerComposite
{
public:
  explicit LayerComposite(LayerManagerComposite* aManager);

  virtual ~LayerComposite();

  virtual LayerComposite* GetFirstChildComposite()
  {
    return nullptr;
  }

  


  virtual void Destroy();

  virtual Layer* GetLayer() = 0;

  virtual void SetLayerManager(LayerManagerComposite* aManager);

  





  virtual void Prepare(const RenderTargetIntRect& aClipRect) {}

  
  virtual void RenderLayer(const gfx::IntRect& aClipRect) = 0;

  virtual bool SetCompositableHost(CompositableHost*)
  {
    
    NS_WARNING("called SetCompositableHost for a layer type not accepting a compositable");
    return false;
  }
  virtual CompositableHost* GetCompositableHost() = 0;

  virtual void CleanupResources() = 0;

  virtual TiledLayerComposer* GetTiledLayerComposer() { return nullptr; }

  virtual void DestroyFrontBuffer() { }

  void AddBlendModeEffect(EffectChain& aEffectChain);

  virtual void GenEffectChain(EffectChain& aEffect) { }

  






  void SetShadowVisibleRegion(const nsIntRegion& aRegion)
  {
    mShadowVisibleRegion = aRegion;
  }

  void SetShadowOpacity(float aOpacity)
  {
    mShadowOpacity = aOpacity;
  }

  void SetShadowClipRect(const Maybe<ParentLayerIntRect>& aRect)
  {
    mShadowClipRect = aRect;
  }

  void SetShadowTransform(const gfx::Matrix4x4& aMatrix)
  {
    mShadowTransform = aMatrix;
  }
  void SetShadowTransformSetByAnimation(bool aSetByAnimation)
  {
    mShadowTransformSetByAnimation = aSetByAnimation;
  }

  void SetLayerComposited(bool value)
  {
    mLayerComposited = value;
  }

  void SetClearRect(const gfx::IntRect& aRect)
  {
    mClearRect = aRect;
  }

  
  float GetShadowOpacity() { return mShadowOpacity; }
  const Maybe<ParentLayerIntRect>& GetShadowClipRect() { return mShadowClipRect; }
  const nsIntRegion& GetShadowVisibleRegion() { return mShadowVisibleRegion; }
  const gfx::Matrix4x4& GetShadowTransform() { return mShadowTransform; }
  bool GetShadowTransformSetByAnimation() { return mShadowTransformSetByAnimation; }
  bool HasLayerBeenComposited() { return mLayerComposited; }
  gfx::IntRect GetClearRect() { return mClearRect; }

  




  nsIntRegion GetFullyRenderedRegion();

protected:
  gfx::Matrix4x4 mShadowTransform;
  nsIntRegion mShadowVisibleRegion;
  Maybe<ParentLayerIntRect> mShadowClipRect;
  LayerManagerComposite* mCompositeManager;
  RefPtr<Compositor> mCompositor;
  float mShadowOpacity;
  bool mShadowTransformSetByAnimation;
  bool mDestroyed;
  bool mLayerComposited;
  gfx::IntRect mClearRect;
};


} 
} 

#endif 
