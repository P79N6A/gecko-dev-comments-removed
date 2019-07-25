




































#ifndef GFX_COLORLAYERD3D10_H
#define GFX_COLORLAYERD3D10_H

#include "Layers.h"
#include "LayerManagerD3D10.h"

namespace mozilla {
namespace layers {

class ColorLayerD3D10 : public ColorLayer,
                        public LayerD3D10
{
public:
  ColorLayerD3D10(LayerManagerD3D10 *aManager);

  
  virtual Layer* GetLayer();
  virtual void RenderLayer(float aOpacity, const gfx3DMatrix &aTransform);
};

} 
} 
#endif 
