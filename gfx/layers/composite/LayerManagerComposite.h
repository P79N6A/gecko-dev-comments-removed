




#ifndef GFX_LayerManagerComposite_H
#define GFX_LayerManagerComposite_H

#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/ShadowLayers.h"
#include "Composer2D.h"
#include "mozilla/TimeStamp.h"

#ifdef XP_WIN
#include <windows.h>
#endif

#include "gfxContext.h"
#include "gfx3DMatrix.h"

namespace mozilla {
namespace layers {

class LayerComposite;
class ShadowThebesLayer;
class ShadowContainerLayer;
class ShadowImageLayer;
class ShadowCanvasLayer;
class ShadowColorLayer;
class CompositableHost;





















class THEBES_API LayerManagerComposite : public ShadowLayerManager
{
public:
  LayerManagerComposite(Compositor* aCompositor);
  virtual ~LayerManagerComposite()
  {
    Destroy();
  }

  virtual void Destroy();

  


  bool Initialize();

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion)
  {
    mClippingRegion = aClippingRegion;
  }

  


  virtual ShadowLayerManager* AsShadowManager() MOZ_OVERRIDE
  {
    return this;
  }

  void UpdateRenderBounds(const nsIntRect& aRect);

  void BeginTransaction() MOZ_OVERRIDE;
  void BeginTransactionWithTarget(gfxContext* aTarget) MOZ_OVERRIDE;

  virtual void NotifyShadowTreeTransaction() MOZ_OVERRIDE
  {
    mCompositor->NotifyLayersTransaction();
  }

  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT) MOZ_OVERRIDE;

  virtual void SetRoot(Layer* aLayer) MOZ_OVERRIDE { mRoot = aLayer; }

  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize) MOZ_OVERRIDE
  {
    return mCompositor->CanUseCanvasLayerForSize(aSize);
  }

  virtual TextureFactoryIdentifier GetTextureFactoryIdentifier() MOZ_OVERRIDE
  {
    return mCompositor->GetTextureFactoryIdentifier();
  }

  virtual int32_t GetMaxTextureSize() const MOZ_OVERRIDE
  {
    return mCompositor->GetMaxTextureSize();
  }

  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ImageLayer> CreateImageLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ColorLayer> CreateColorLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer() MOZ_OVERRIDE;
  virtual already_AddRefed<ShadowRefLayer> CreateShadowRefLayer() MOZ_OVERRIDE;

  virtual LayersBackend GetBackendType() MOZ_OVERRIDE
  {
    return LAYERS_NONE;
  }
  virtual void GetBackendName(nsAString& name) MOZ_OVERRIDE
  {
    MOZ_ASSERT(false, "Shouldn't be called for composited layer manager");
    name.AssignLiteral("Composite");
  }

  virtual already_AddRefed<gfxASurface>
    CreateOptimalMaskSurface(const gfxIntSize &aSize) MOZ_OVERRIDE;


  DrawThebesLayerCallback GetThebesLayerCallback() const
  { return mThebesLayerCallback; }

  void* GetThebesLayerCallbackData() const
  { return mThebesLayerCallbackData; }

  


  void CallThebesLayerDrawCallback(ThebesLayer* aLayer,
                                   gfxContext* aContext,
                                   const nsIntRegion& aRegionToDraw)
  {
    NS_ASSERTION(mThebesLayerCallback,
                 "CallThebesLayerDrawCallback without callback!");
    mThebesLayerCallback(aLayer, aContext,
                         aRegionToDraw, nsIntRegion(),
                         mThebesLayerCallbackData);
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const MOZ_OVERRIDE { return ""; }
#endif 

  enum WorldTransforPolicy {
    ApplyWorldTransform,
    DontApplyWorldTransform
  };

  




  void SetWorldTransform(const gfxMatrix& aMatrix);
  gfxMatrix& GetWorldTransform(void);

  static bool AddMaskEffect(Layer* aMaskLayer,
                            EffectChain& aEffect,
                            bool aIs3D = false);

  



  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize &aSize,
                     mozilla::gfx::SurfaceFormat aFormat) MOZ_OVERRIDE;

  const nsIntSize& GetWidgetSize()
  {
    return mCompositor->GetWidgetSize();
  }

  






  float ComputeRenderIntegrity();

private:
  
  nsIntRegion mClippingRegion;
  nsIntRect mRenderBounds;

  
  LayerComposite *RootLayer() const;

  





  static void ComputeRenderIntegrityInternal(Layer* aLayer,
                                             nsIntRegion& aScreenRegion,
                                             nsIntRegion& aLowPrecisionScreenRegion,
                                             const gfx3DMatrix& aTransform);

  


  void Render();

  void WorldTransformRect(nsIntRect& aRect);

  
  nsRefPtr<Composer2D> mComposer2D;

  

  DrawThebesLayerCallback mThebesLayerCallback;
  void *mThebesLayerCallbackData;
  gfxMatrix mWorldMatrix;
  bool mInTransaction;
};







class LayerComposite
{
public:
  LayerComposite(LayerManagerComposite *aManager)
    : mCompositeManager(aManager)
    , mCompositor(aManager->GetCompositor())
    , mDestroyed(false)
  { }

  virtual ~LayerComposite() {}

  virtual LayerComposite *GetFirstChildComposite()
  {
    return nullptr;
  }

  


  virtual void Destroy();

  virtual Layer* GetLayer() = 0;

  virtual void RenderLayer(const nsIntPoint& aOffset,
                           const nsIntRect& aClipRect) = 0;

  virtual void SetCompositableHost(CompositableHost* aHost)
  {
    MOZ_ASSERT(false, "called SetCompositableHost for a layer without a compositable host");
  }
  virtual CompositableHost* GetCompositableHost() = 0;

  virtual void CleanupResources() = 0;

  virtual TiledLayerComposer* AsTiledLayerComposer() { return NULL; }

  virtual void EnsureBuffer(CompositableType aType)
  {
    MOZ_ASSERT(false, "Should not be called unless overriden.");
  }

protected:
  LayerManagerComposite* mCompositeManager;
  RefPtr<Compositor> mCompositor;
  bool mDestroyed;
};


} 
} 

#endif 
