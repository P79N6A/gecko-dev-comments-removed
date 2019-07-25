




































#ifndef GFX_CONTAINERLAYEROGL_H
#define GFX_CONTAINERLAYEROGL_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "Layers.h"
#include "LayerManagerOGL.h"

namespace mozilla {
namespace layers {

template<class Container>
static void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
template<class Container>
static void ContainerRemoveChild(Container* aContainer, Layer* aChild);
template<class Container>
static void ContainerDestroy(Container* aContainer);
template<class Container>
static void ContainerRender(Container* aContainer,
                            int aPreviousFrameBuffer,
                            const nsIntPoint& aOffset,
                            LayerManagerOGL* aManager);

class ContainerLayerOGL : public ContainerLayer,
                          public LayerOGL
{
  template<class Container>
  friend void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
  template<class Container>
  friend void ContainerRemoveChild(Container* aContainer, Layer* aChild);
  template<class Container>
  friend void ContainerDestroy(Container* aContainer);
  template<class Container>
  friend void ContainerRender(Container* aContainer,
                              int aPreviousFrameBuffer,
                              const nsIntPoint& aOffset,
                              LayerManagerOGL* aManager);

public:
  ContainerLayerOGL(LayerManagerOGL *aManager);
  ~ContainerLayerOGL();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  Layer* GetLayer() { return this; }

  void Destroy();

  LayerOGL* GetFirstChildOGL();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }
};

class ShadowContainerLayerOGL : public ShadowContainerLayer,
                                public LayerOGL
{
  template<class Container>
  friend void ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter);
  template<class Container>
  friend void ContainerRemoveChild(Container* aContainer, Layer* aChild);
  template<class Container>
  friend void ContainerDestroy(Container* aContainer);
  template<class Container>
  friend void ContainerRender(Container* aContainer,
                              int aPreviousFrameBuffer,
                              const nsIntPoint& aOffset,
                              LayerManagerOGL* aManager);

public:
  ShadowContainerLayerOGL(LayerManagerOGL *aManager);
  ~ShadowContainerLayerOGL();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  virtual Layer* GetLayer() { return this; }

  virtual void Destroy();

  LayerOGL* GetFirstChildOGL();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    DefaultComputeEffectiveTransforms(aTransformToSurface);
  }
};

} 
} 

#endif 
