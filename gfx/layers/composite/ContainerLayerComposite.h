




#ifndef GFX_ContainerLayerComposite_H
#define GFX_ContainerLayerComposite_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "Layers.h"
#include "LayerManagerComposite.h"
#include "mozilla/layers/Effects.h"

#include "gfxUtils.h"
#include "gfx2DGlue.h"

namespace mozilla {
namespace layers {

class ContainerLayerComposite : public ShadowContainerLayer,
                                public LayerComposite
{
  template<class ContainerT>
  friend void ContainerRender(ContainerT* aContainer,
                              const nsIntPoint& aOffset,
                              LayerManagerComposite* aManager,
                              const nsIntRect& aClipRect);
public:
  ContainerLayerComposite(LayerManagerComposite *aManager);
  ~ContainerLayerComposite();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  void RepositionChild(Layer* aChild, Layer* aAfter);

  
  virtual Layer* GetLayer() MOZ_OVERRIDE { return this; }

  virtual void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite();

  virtual void RenderLayer(const nsIntPoint& aOffset,
                           const nsIntRect& aClipRect) MOZ_OVERRIDE;

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

class RefLayerComposite : public ShadowRefLayer,
                          public LayerComposite
{
  template<class ContainerT>
  friend void ContainerRender(ContainerT* aContainer,
                              const nsIntPoint& aOffset,
                              LayerManagerComposite* aManager,
                              const nsIntRect& aClipRect);
public:
  RefLayerComposite(LayerManagerComposite *aManager);
  ~RefLayerComposite();

  
  Layer* GetLayer() MOZ_OVERRIDE { return this; }

  void Destroy() MOZ_OVERRIDE;

  LayerComposite* GetFirstChildComposite();

  virtual void RenderLayer(const nsIntPoint& aOffset,
                           const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) MOZ_OVERRIDE
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources() MOZ_OVERRIDE;

  
  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const MOZ_OVERRIDE { return "RefLayerComposite"; }
#endif
};

} 
} 

#endif 
