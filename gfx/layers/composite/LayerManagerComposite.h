




#ifndef GFX_LayerManagerComposite_H
#define GFX_LayerManagerComposite_H

#include <stdint.h>                     
#include "GLDefs.h"                     
#include "Layers.h"
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/RefPtr.h"
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nscore.h"                     
#include "LayerTreeInvalidation.h"

class gfxContext;
struct nsIntPoint;
struct nsIntSize;

#ifdef XP_WIN
#include <windows.h>
#endif

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace gl {
class GLContext;
class TextureImage;
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
class SurfaceDescriptor;
class ThebesLayerComposite;
class TiledLayerComposer;
class TextRenderer;
struct FPSState;

class LayerManagerComposite : public LayerManager
{
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::IntSize IntSize;
  typedef mozilla::gfx::SurfaceFormat SurfaceFormat;

public:
  LayerManagerComposite(Compositor* aCompositor);
  ~LayerManagerComposite();
  
  virtual void Destroy() MOZ_OVERRIDE;

  


  bool Initialize();

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion)
  {
    mClippingRegion = aClippingRegion;
  }

  


  virtual LayerManagerComposite* AsLayerManagerComposite() MOZ_OVERRIDE
  {
    return this;
  }

  void UpdateRenderBounds(const nsIntRect& aRect);

  virtual void BeginTransaction() MOZ_OVERRIDE;
  virtual void BeginTransactionWithTarget(gfxContext* aTarget) MOZ_OVERRIDE
  {
    MOZ_CRASH("Use BeginTransactionWithDrawTarget");
  }
  void BeginTransactionWithDrawTarget(gfx::DrawTarget* aTarget, const nsIntRect& aRect);

  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;

  virtual void SetRoot(Layer* aLayer) MOZ_OVERRIDE { mRoot = aLayer; }

  
  
  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize) MOZ_OVERRIDE;

  virtual int32_t GetMaxTextureSize() const MOZ_OVERRIDE
  {
    MOZ_CRASH("Call on compositor, not LayerManagerComposite");
  }

  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ImageLayer> CreateImageLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ColorLayer> CreateColorLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() MOZ_OVERRIDE;
  already_AddRefed<ThebesLayerComposite> CreateThebesLayerComposite();
  already_AddRefed<ContainerLayerComposite> CreateContainerLayerComposite();
  already_AddRefed<ImageLayerComposite> CreateImageLayerComposite();
  already_AddRefed<ColorLayerComposite> CreateColorLayerComposite();
  already_AddRefed<CanvasLayerComposite> CreateCanvasLayerComposite();
  already_AddRefed<RefLayerComposite> CreateRefLayerComposite();

  virtual LayersBackend GetBackendType() MOZ_OVERRIDE
  {
    MOZ_CRASH("Shouldn't be called for composited layer manager");
  }
  virtual void GetBackendName(nsAString& name) MOZ_OVERRIDE
  {
    MOZ_CRASH("Shouldn't be called for composited layer manager");
  }

  virtual TemporaryRef<DrawTarget>
    CreateOptimalMaskDrawTarget(const IntSize &aSize) MOZ_OVERRIDE;

  virtual const char* Name() const MOZ_OVERRIDE { return ""; }

  enum WorldTransforPolicy {
    ApplyWorldTransform,
    DontApplyWorldTransform
  };

  




  void SetWorldTransform(const gfx::Matrix& aMatrix);
  gfx::Matrix& GetWorldTransform(void);

  



  class AutoAddMaskEffect
  {
  public:
    AutoAddMaskEffect(Layer* aMaskLayer,
                      EffectChain& aEffect,
                      bool aIs3D = false);
    ~AutoAddMaskEffect();

  private:
    CompositableHost* mCompositable;
  };

  



  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize& aSize,
                     mozilla::gfx::SurfaceFormat aFormat) MOZ_OVERRIDE;

  






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

private:
  
  nsIntRegion mClippingRegion;
  nsIntRect mRenderBounds;

  
  LayerComposite* RootLayer() const;

  





  static void ComputeRenderIntegrityInternal(Layer* aLayer,
                                             nsIntRegion& aScreenRegion,
                                             nsIntRegion& aLowPrecisionScreenRegion,
                                             const gfx3DMatrix& aTransform);

  


  void Render();

  


  void RenderDebugOverlay(const gfx::Rect& aBounds);

  void WorldTransformRect(nsIntRect& aRect);

  RefPtr<Compositor> mCompositor;
  nsAutoPtr<LayerProperties> mClonedLayerTreeProperties;

  


  RefPtr<gfx::DrawTarget> mTarget;
  nsIntRect mTargetBounds;

  gfx::Matrix mWorldMatrix;
  nsIntRegion mInvalidRegion;
  nsAutoPtr<FPSState> mFPS;

  bool mInTransaction;
  bool mIsCompositorReady;
  bool mDebugOverlayWantsNextFrame;

  RefPtr<TextRenderer> mTextRenderer;
  bool mGeometryChanged;
};



















class LayerComposite
{
public:
  LayerComposite(LayerManagerComposite* aManager);

  virtual ~LayerComposite();

  virtual LayerComposite* GetFirstChildComposite()
  {
    return nullptr;
  }

  


  virtual void Destroy();

  virtual Layer* GetLayer() = 0;

  virtual void RenderLayer(const nsIntRect& aClipRect) = 0;

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

  






  void SetShadowVisibleRegion(const nsIntRegion& aRegion)
  {
    mShadowVisibleRegion = aRegion;
  }

  void SetShadowOpacity(float aOpacity)
  {
    mShadowOpacity = aOpacity;
  }

  void SetShadowClipRect(const nsIntRect* aRect)
  {
    mUseShadowClipRect = aRect != nullptr;
    if (aRect) {
      mShadowClipRect = *aRect;
    }
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

  void SetClearRect(const nsIntRect& aRect)
  {
    mClearRect = aRect;
  }

  
  float GetShadowOpacity() { return mShadowOpacity; }
  const nsIntRect* GetShadowClipRect() { return mUseShadowClipRect ? &mShadowClipRect : nullptr; }
  const nsIntRegion& GetShadowVisibleRegion() { return mShadowVisibleRegion; }
  const gfx::Matrix4x4& GetShadowTransform() { return mShadowTransform; }
  bool GetShadowTransformSetByAnimation() { return mShadowTransformSetByAnimation; }
  bool HasLayerBeenComposited() { return mLayerComposited; }
  nsIntRect GetClearRect() { return mClearRect; }

protected:
  gfx::Matrix4x4 mShadowTransform;
  nsIntRegion mShadowVisibleRegion;
  nsIntRect mShadowClipRect;
  LayerManagerComposite* mCompositeManager;
  RefPtr<Compositor> mCompositor;
  float mShadowOpacity;
  bool mUseShadowClipRect;
  bool mShadowTransformSetByAnimation;
  bool mDestroyed;
  bool mLayerComposited;
  nsIntRect mClearRect;
};


} 
} 

#endif 
