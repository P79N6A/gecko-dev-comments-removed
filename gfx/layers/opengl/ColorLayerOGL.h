




































#ifndef GFX_COLORLAYEROGL_H
#define GFX_COLORLAYEROGL_H

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

  
  virtual Layer* GetLayer();

  virtual void Destroy() { mDestroyed = PR_TRUE; }

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
};

} 
} 
#endif 
