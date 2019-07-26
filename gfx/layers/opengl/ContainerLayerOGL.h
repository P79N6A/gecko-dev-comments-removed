




#ifndef GFX_CONTAINERLAYEROGL_H
#define GFX_CONTAINERLAYEROGL_H

#include "LayerManagerOGL.h"            
#include "Layers.h"                     
class gfx3DMatrix;
struct nsIntPoint;

namespace mozilla {
namespace layers {

class ContainerLayerOGL : public ContainerLayer,
                          public LayerOGL
{
public:
  ContainerLayerOGL(LayerManagerOGL *aManager);
  ~ContainerLayerOGL();

  
  Layer* GetLayer() { return this; }

  void Destroy();

  LayerOGL* GetFirstChildOGL();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }

  virtual void CleanupResources();
};

} 
} 

#endif 
