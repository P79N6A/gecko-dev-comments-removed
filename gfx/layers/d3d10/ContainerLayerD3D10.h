




#ifndef GFX_CONTAINERLAYERD3D10_H
#define GFX_CONTAINERLAYERD3D10_H

#include "LayerManagerD3D10.h"

namespace mozilla {
namespace layers {

class ContainerLayerD3D10 : public ContainerLayer,
                            public LayerD3D10
{
public:
  ContainerLayerD3D10(LayerManagerD3D10 *aManager);
  ~ContainerLayerD3D10();

  nsIntRect GetVisibleRect() { return mVisibleRegion.GetBounds(); }

  
  virtual void InsertAfter(Layer* aChild, Layer* aAfter);

  virtual void RemoveChild(Layer* aChild);

  
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
public:
  ShadowContainerLayerD3D10(LayerManagerD3D10 *aManager);
  ~ShadowContainerLayerD3D10();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

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
