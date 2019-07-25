




































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

  virtual void SetVisibleRegion(const nsIntRegion& aRegion) { mVisibleRegion = aRegion; }

  
  virtual LayerType GetType();

  virtual Layer* GetLayer();

  virtual void RenderLayer(int aPreviousDestination,
                           DrawThebesLayerCallback aCallback,
                           void* aCallbackData);

protected:
  nsIntRegion mVisibleRegion;
};

} 
} 
#endif 
