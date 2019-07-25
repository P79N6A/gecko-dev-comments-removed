




































#ifndef GFX_CONTAINERLAYERD3D9_H
#define GFX_CONTAINERLAYERD3D9_H

#include "Layers.h"
#include "LayerManagerD3D9.h"

namespace mozilla {
namespace layers {

class ContainerLayerD3D9 : public ContainerLayer,
                           public LayerD3D9
{
public:
  ContainerLayerD3D9(LayerManagerD3D9 *aManager);
  ~ContainerLayerD3D9();

  nsIntRect GetVisibleRect() { return mVisibleRegion.GetBounds(); }

  
  virtual void InsertAfter(Layer* aChild, Layer* aAfter);

  virtual void RemoveChild(Layer* aChild);

  
  Layer* GetLayer();

  LayerD3D9* GetFirstChildD3D9();

  PRBool IsEmpty();

  void RenderLayer();

  virtual void LayerManagerDestroyed();
};

} 
} 

#endif 
