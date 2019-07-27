




#ifndef GFX_ContainerLayerComposite_H
#define GFX_ContainerLayerComposite_H

#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/layers/LayerManagerComposite.h"

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace layers {

class CompositableHost;

class ContainerLayerComposite : public ContainerLayer,
                                public LayerComposite
{
  template<class ContainerT>
  friend void ContainerRender(ContainerT* aContainer,
                              LayerManagerComposite* aManager,
                              const nsIntRect& aClipRect);
public:
  ContainerLayerComposite(LayerManagerComposite *aManager);

protected:
  ~ContainerLayerComposite();

public:
  
  virtual Layer* GetLayer() MOZ_OVERRIDE { return this; }

  virtual void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite();

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) MOZ_OVERRIDE
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  
  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

  virtual const char* Name() const MOZ_OVERRIDE { return "ContainerLayerComposite"; }
};

class RefLayerComposite : public RefLayer,
                          public LayerComposite
{
  template<class ContainerT>
  friend void ContainerRender(ContainerT* aContainer,
                              LayerManagerComposite* aManager,
                              const nsIntRect& aClipRect);
public:
  RefLayerComposite(LayerManagerComposite *aManager);

protected:
  ~RefLayerComposite();

public:
  
  Layer* GetLayer() MOZ_OVERRIDE { return this; }

  void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite();

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) MOZ_OVERRIDE
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  
  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

  virtual const char* Name() const MOZ_OVERRIDE { return "RefLayerComposite"; }
};

} 
} 

#endif 
