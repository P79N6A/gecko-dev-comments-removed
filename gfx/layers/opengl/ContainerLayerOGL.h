




































#ifndef GFX_CONTAINERLAYEROGL_H
#define GFX_CONTAINERLAYEROGL_H

#ifdef MOZ_IPC
# include "mozilla/layers/PLayers.h"
# include "mozilla/layers/ShadowLayers.h"
#endif

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
template<class Container>
static bool ShouldUseIntermediate(Container* aContainer,
                                  float aOpacity,
                                  const gfx3DMatrix& aMatrix);

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
  template<class Container>
  friend bool ShouldUseIntermediate(Container* aContainer,
                                    float aOpacity,
                                    const gfx3DMatrix& aMatrix);

public:
  ContainerLayerOGL(LayerManagerOGL *aManager);
  ~ContainerLayerOGL();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  Layer* GetLayer() { return this; }

  void Destroy();

  LayerOGL* GetFirstChildOGL();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset,
                           float aOpacity,
                           const gfx3DMatrix& aMatrix);
};

#ifdef MOZ_IPC
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
  template<class Container>
  friend bool ShouldUseIntermediate(Container* aContainer,
                                    float aOpacity,
                                    const gfx3DMatrix& aMatrix);

public:
  ShadowContainerLayerOGL(LayerManagerOGL *aManager);
  ~ShadowContainerLayerOGL();

  void InsertAfter(Layer* aChild, Layer* aAfter);

  void RemoveChild(Layer* aChild);

  
  virtual Layer* GetLayer() { return this; }

  virtual void Destroy();

  LayerOGL* GetFirstChildOGL();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset,
                           float aOpacity,
                           const gfx3DMatrix& aMatrix);
};
#endif  

} 
} 

#endif 
