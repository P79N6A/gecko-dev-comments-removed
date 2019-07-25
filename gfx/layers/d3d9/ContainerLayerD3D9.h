




































#ifndef GFX_CONTAINERLAYERD3D9_H
#define GFX_CONTAINERLAYERD3D9_H

#include "Layers.h"
#include "LayerManagerD3D9.h"

namespace mozilla {
namespace layers {
  
template<class Container>
static void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
template<class Container>
static void ContainerRemoveChild(Container* aContainer, Layer* aChild);
template<class Container>
static void ContainerRender(Container* aContainer, LayerManagerD3D9* aManager);

class ContainerLayerD3D9 : public ContainerLayer,
                           public LayerD3D9
{
  template<class Container>
  friend void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
  template<class Container>
  friend void ContainerRemoveChild(Container* aContainer, Layer* aChild);
  template<class Container>
  friend void ContainerRender(Container* aContainer, LayerManagerD3D9* aManager);

public:
  ContainerLayerD3D9(LayerManagerD3D9 *aManager);
  ~ContainerLayerD3D9();

  nsIntRect GetVisibleRect() { return mVisibleRegion.GetBounds(); }

  
  virtual void InsertAfter(Layer* aChild, Layer* aAfter);

  virtual void RemoveChild(Layer* aChild);

  
  Layer* GetLayer();

  LayerD3D9* GetFirstChildD3D9();

  bool IsEmpty();

  void RenderLayer();

  virtual void LayerManagerDestroyed();

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }
};

class ShadowContainerLayerD3D9 : public ShadowContainerLayer,
                                public LayerD3D9
{
  template<class Container>
  friend void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
  template<class Container>
  friend void ContainerRemoveChild(Container* aContainer, Layer* aChild);
  template<class Container>
  friend void ContainerRender(Container* aContainer, LayerManagerD3D9* aManager);

public:
  ShadowContainerLayerD3D9(LayerManagerD3D9 *aManager);
  ~ShadowContainerLayerD3D9();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  virtual Layer* GetLayer() { return this; }

  virtual void Destroy();

  LayerD3D9* GetFirstChildD3D9();

  virtual void RenderLayer();

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }
};

} 
} 

#endif 
