




































#ifndef GFX_COLORLAYEROGL_H
#define GFX_COLORLAYEROGL_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "LayerManagerOGL.h"

namespace mozilla {
namespace layers {

class THEBES_API ColorLayerOGL : public ColorLayer,
                                 public LayerOGL
{
public:
  ColorLayerOGL(LayerManagerOGL *aManager)
    : ColorLayer(aManager, NULL)
    , LayerOGL(aManager)
  { 
    mImplData = static_cast<LayerOGL*>(this);
  }
  ~ColorLayerOGL() { Destroy(); }

  
  virtual Layer* GetLayer() { return this; }

  virtual void Destroy() { mDestroyed = PR_TRUE; }

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
};

class ShadowColorLayerOGL : public ShadowColorLayer,
                            public LayerOGL
{
public:
  ShadowColorLayerOGL(LayerManagerOGL *aManager)
    : ShadowColorLayer(aManager, NULL)
    , LayerOGL(aManager)
  { 
    mImplData = static_cast<LayerOGL*>(this);
  }
  ~ShadowColorLayerOGL() { Destroy(); }

  
  virtual Layer* GetLayer() { return this; }

  virtual void Destroy() { mDestroyed = PR_TRUE; }

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
};

} 
} 
#endif 
