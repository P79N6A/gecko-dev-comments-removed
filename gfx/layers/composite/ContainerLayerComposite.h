




#ifndef GFX_ContainerLayerComposite_H
#define GFX_ContainerLayerComposite_H

#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/UniquePtr.h"          
#include "mozilla/layers/LayerManagerComposite.h"

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace layers {

class CompositableHost;
class CompositingRenderTarget;
struct PreparedData;

class ContainerLayerComposite : public ContainerLayer,
                                public LayerComposite
{
  template<class ContainerT>
  friend void ContainerPrepare(ContainerT* aContainer,
                               LayerManagerComposite* aManager,
                               const RenderTargetIntRect& aClipRect);
  template<class ContainerT>
  friend void ContainerRender(ContainerT* aContainer,
                              LayerManagerComposite* aManager,
                              const RenderTargetIntRect& aClipRect);
  template<class ContainerT>
  friend void RenderLayers(ContainerT* aContainer,
                           LayerManagerComposite* aManager,
                           const RenderTargetIntRect& aClipRect);
  template<class ContainerT>
  friend void RenderIntermediate(ContainerT* aContainer,
                   LayerManagerComposite* aManager,
                   const nsIntRect& aClipRect,
                   RefPtr<CompositingRenderTarget> surface);
  template<class ContainerT>
  friend RefPtr<CompositingRenderTarget>
  CreateTemporaryTargetAndCopyFromBackground(ContainerT* aContainer,
                                             LayerManagerComposite* aManager,
                                             const RenderTargetIntRect& aClipRect);
  template<class ContainerT>
  friend RefPtr<CompositingRenderTarget>
  CreateOrRecycleTarget(ContainerT* aContainer,
                        LayerManagerComposite* aManager,
                        const RenderTargetIntRect& aClipRect);

public:
  explicit ContainerLayerComposite(LayerManagerComposite *aManager);

protected:
  ~ContainerLayerComposite();

public:
  
  virtual Layer* GetLayer() MOZ_OVERRIDE { return this; }

  virtual void SetLayerManager(LayerManagerComposite* aManager) MOZ_OVERRIDE
  {
    LayerComposite::SetLayerManager(aManager);
    mManager = aManager;

    for (Layer* l = GetFirstChild(); l; l = l->GetNextSibling()) {
      LayerComposite* child = l->AsLayerComposite();
      child->SetLayerManager(aManager);
    }
  }

  virtual void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite() MOZ_OVERRIDE;

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;
  virtual void Prepare(const RenderTargetIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) MOZ_OVERRIDE
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  
  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

  virtual const char* Name() const MOZ_OVERRIDE { return "ContainerLayerComposite"; }
  UniquePtr<PreparedData> mPrepared;

  RefPtr<CompositingRenderTarget> mLastIntermediateSurface;
};

class RefLayerComposite : public RefLayer,
                          public LayerComposite
{
  template<class ContainerT>
  friend void ContainerPrepare(ContainerT* aContainer,
                               LayerManagerComposite* aManager,
                               const RenderTargetIntRect& aClipRect);
  template<class ContainerT>
  friend void ContainerRender(ContainerT* aContainer,
                              LayerManagerComposite* aManager,
                              const nsIntRect& aClipRect);
  template<class ContainerT>
  friend void RenderLayers(ContainerT* aContainer,
                           LayerManagerComposite* aManager,
                           const nsIntRect& aClipRect);
  template<class ContainerT>
  friend void RenderIntermediate(ContainerT* aContainer,
                   LayerManagerComposite* aManager,
                   const nsIntRect& aClipRect,
                   RefPtr<CompositingRenderTarget> surface);
  template<class ContainerT>
  friend RefPtr<CompositingRenderTarget>
  CreateTemporaryTargetAndCopyFromBackground(ContainerT* aContainer,
                                             LayerManagerComposite* aManager,
                                             const nsIntRect& aClipRect);
  template<class ContainerT>
  friend RefPtr<CompositingRenderTarget>
  CreateTemporaryTarget(ContainerT* aContainer,
                        LayerManagerComposite* aManager,
                        const nsIntRect& aClipRect);

public:
  explicit RefLayerComposite(LayerManagerComposite *aManager);

protected:
  ~RefLayerComposite();

public:
  
  Layer* GetLayer() MOZ_OVERRIDE { return this; }

  void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite() MOZ_OVERRIDE;

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;
  virtual void Prepare(const RenderTargetIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) MOZ_OVERRIDE
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  
  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

  virtual const char* Name() const MOZ_OVERRIDE { return "RefLayerComposite"; }
  UniquePtr<PreparedData> mPrepared;
  RefPtr<CompositingRenderTarget> mLastIntermediateSurface;
};

} 
} 

#endif 
