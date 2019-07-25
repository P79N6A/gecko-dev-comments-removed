




































#ifndef GFX_CONTAINERLAYERD3D10_H
#define GFX_CONTAINERLAYERD3D10_H

#include "Layers.h"
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

  virtual void RenderLayer(float aOpacity, const gfx3DMatrix &aTransform);
  virtual void Validate();

  virtual void LayerManagerDestroyed();

private:
  bool ShouldUseIntermediate(float aOpacity, const gfx3DMatrix &aTransform);
};

} 
} 

#endif 
