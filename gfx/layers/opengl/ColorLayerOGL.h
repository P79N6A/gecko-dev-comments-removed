




#ifndef GFX_COLORLAYEROGL_H
#define GFX_COLORLAYEROGL_H

#include "LayerManagerOGL.h"            
#include "Layers.h"                     

struct nsIntPoint;

namespace mozilla {
namespace layers {

class ColorLayerOGL : public ColorLayer,
                      public LayerOGL
{
public:
  ColorLayerOGL(LayerManagerOGL *aManager)
    : ColorLayer(aManager, nullptr)
    , LayerOGL(aManager)
  { 
    mImplData = static_cast<LayerOGL*>(this);
  }
  ~ColorLayerOGL() { Destroy(); }

  
  virtual Layer* GetLayer() { return this; }

  virtual void Destroy() { mDestroyed = true; }

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources() {};
};

} 
} 
#endif 
