




































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

  nsIntRect GetVisibleRect() { return mVisibleRegion.GetBounds(); }

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  Layer* GetLayer();

  LayerOGL* GetFirstChildOGL();

  PRBool IsEmpty();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
private:
  GLuint mTexture;
};

} 
} 

#endif 
