




#ifndef GFX_ContainerLayerComposite_H
#define GFX_ContainerLayerComposite_H

#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersTypes.h"  

class gfx3DMatrix;
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

  ~ContainerLayerComposite();

  
  virtual Layer* GetLayer() MOZ_OVERRIDE { return this; }

  virtual void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite();

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) MOZ_OVERRIDE
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  
  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const MOZ_OVERRIDE { return "ContainerLayerComposite"; }
#endif
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
  ~RefLayerComposite();

  
  Layer* GetLayer() MOZ_OVERRIDE { return this; }

  void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite();

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) MOZ_OVERRIDE
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  
  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const MOZ_OVERRIDE { return "RefLayerComposite"; }
#endif
};

} 
} 

#endif 
