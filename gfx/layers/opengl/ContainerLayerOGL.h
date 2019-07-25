




































#ifndef GFX_CONTAINERLAYEROGL_H
#define GFX_CONTAINERLAYEROGL_H

#include "Layers.h"
#include "LayerManagerOGL.h"

namespace mozilla {
namespace layers {

class ContainerLayerOGL : public ContainerLayer, 
                          public LayerOGL
{
public:
  ContainerLayerOGL(LayerManagerOGL *aManager);
  ~ContainerLayerOGL();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  Layer* GetLayer();

  void Destroy();

  LayerOGL* GetFirstChildOGL();

  PRBool IsEmpty();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
};

} 
} 

#endif 
