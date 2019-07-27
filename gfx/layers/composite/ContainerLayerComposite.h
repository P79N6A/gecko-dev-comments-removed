




#ifndef GFX_ContainerLayerComposite_H
#define GFX_ContainerLayerComposite_H

#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/UniquePtr.h"          
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/gfx/Rect.h"

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
                   const gfx::IntRect& aClipRect,
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
  
  virtual Layer* GetLayer() override { return this; }

  virtual void SetLayerManager(LayerManagerComposite* aManager) override
  {
    LayerComposite::SetLayerManager(aManager);
    mManager = aManager;

    for (Layer* l = GetFirstChild(); l; l = l->GetNextSibling()) {
      LayerComposite* child = l->AsLayerComposite();
      child->SetLayerManager(aManager);
    }
  }

  virtual void Destroy() override;

  LayerComposite* GetFirstChildComposite() override;

  virtual void RenderLayer(const gfx::IntRect& aClipRect) override;
  virtual void Prepare(const RenderTargetIntRect& aClipRect) override;

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() override;

  virtual LayerComposite* AsLayerComposite() override { return this; }

  
  CompositableHost* GetCompositableHost() override { return nullptr; }

  
  
  
  
  
  virtual float GetPostXScale() const override {
    if (mScaleToResolution) {
      return mPostXScale * mPresShellResolution;
    }
    return mPostXScale;
  }
  virtual float GetPostYScale() const override {
    if (mScaleToResolution) {
      return mPostYScale * mPresShellResolution;
    }
    return mPostYScale;
  }

  virtual const char* Name() const override { return "ContainerLayerComposite"; }
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
                              const gfx::IntRect& aClipRect);
  template<class ContainerT>
  friend void RenderLayers(ContainerT* aContainer,
                           LayerManagerComposite* aManager,
                           const gfx::IntRect& aClipRect);
  template<class ContainerT>
  friend void RenderIntermediate(ContainerT* aContainer,
                   LayerManagerComposite* aManager,
                   const gfx::IntRect& aClipRect,
                   RefPtr<CompositingRenderTarget> surface);
  template<class ContainerT>
  friend RefPtr<CompositingRenderTarget>
  CreateTemporaryTargetAndCopyFromBackground(ContainerT* aContainer,
                                             LayerManagerComposite* aManager,
                                             const gfx::IntRect& aClipRect);
  template<class ContainerT>
  friend RefPtr<CompositingRenderTarget>
  CreateTemporaryTarget(ContainerT* aContainer,
                        LayerManagerComposite* aManager,
                        const gfx::IntRect& aClipRect);

public:
  explicit RefLayerComposite(LayerManagerComposite *aManager);

protected:
  ~RefLayerComposite();

public:
  
  Layer* GetLayer() override { return this; }

  void Destroy() override;

  LayerComposite* GetFirstChildComposite() override;

  virtual void RenderLayer(const gfx::IntRect& aClipRect) override;
  virtual void Prepare(const RenderTargetIntRect& aClipRect) override;

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() override;

  virtual LayerComposite* AsLayerComposite() override { return this; }

  
  CompositableHost* GetCompositableHost() override { return nullptr; }

  virtual const char* Name() const override { return "RefLayerComposite"; }
  UniquePtr<PreparedData> mPrepared;
  RefPtr<CompositingRenderTarget> mLastIntermediateSurface;
};

} 
} 

#endif 
