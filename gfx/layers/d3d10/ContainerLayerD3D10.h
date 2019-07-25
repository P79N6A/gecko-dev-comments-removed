




#ifndef GFX_CONTAINERLAYERD3D10_H
#define GFX_CONTAINERLAYERD3D10_H

#include "LayerManagerD3D10.h"

namespace mozilla {
namespace layers {

template<class Container>
static void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
template<class Container>
static void ContainerRemoveChild(Container* aContainer, Layer* aChild);
template<class Container>
static void ContainerRepositionChild(Container* aContainer, Layer* aChild, Layer* aAfter);

class ContainerLayerD3D10 : public ContainerLayer,
                            public LayerD3D10
{
  template<class Container>
  friend void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
  template<class Container>
  friend void ContainerRemoveChild(Container* aContainer, Layer* aChild);
  template<class Container>
  friend void ContainerRepositionChild(Container* aContainer, Layer* aChild, Layer* aAfter);
public:
  ContainerLayerD3D10(LayerManagerD3D10 *aManager);
  ~ContainerLayerD3D10();

  nsIntRect GetVisibleRect() { return mVisibleRegion.GetBounds(); }

  
  virtual void InsertAfter(Layer* aChild, Layer* aAfter);

  virtual void RemoveChild(Layer* aChild);

  virtual void RepositionChild(Layer* aChild, Layer* aAfter);

  
  virtual Layer* GetLayer();

  virtual LayerD3D10* GetFirstChildD3D10();

  virtual void RenderLayer();
  virtual void Validate();

  virtual void LayerManagerDestroyed();

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }
};




class ShadowContainerLayerD3D10 : public ShadowContainerLayer,
                                  public LayerD3D10
{
  template<class Container>
  friend void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
  template<class Container>
  friend void ContainerRemoveChild(Container* aContainer, Layer* aChild);
  template<class Container>
  friend void ContainerRepositionChild(Container* aContainer, Layer* aChild, Layer* aAfter);
public:
  ShadowContainerLayerD3D10(LayerManagerD3D10 *aManager);
  ~ShadowContainerLayerD3D10();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  void RepositionChild(Layer* aChild, Layer* aAfter);

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface);

  
  virtual LayerD3D10 *GetFirstChildD3D10();
  virtual Layer* GetLayer() { return this; }
  virtual void RenderLayer();
  virtual void Validate();
  virtual void LayerManagerDestroyed();

private:
    
};

} 
} 

#endif
