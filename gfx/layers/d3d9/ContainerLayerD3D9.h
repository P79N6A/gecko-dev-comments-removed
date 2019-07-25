




































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

  const nsIntRect &GetVisibleRect();

  
  void SetVisibleRegion(const nsIntRegion& aRegion);

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  LayerType GetType();

  Layer* GetLayer();

  LayerD3D9* GetFirstChildD3D9();

  PRBool IsEmpty();

  void RenderLayer();

private:
  nsIntRect mVisibleRect;
};

} 
} 

#endif 
