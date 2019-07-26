




#ifndef GFX_COLORLAYERD3D9_H
#define GFX_COLORLAYERD3D9_H

#include "LayerManagerD3D9.h"

namespace mozilla {
namespace layers {

class ColorLayerD3D9 : public ColorLayer,
                       public LayerD3D9
{
public:
  ColorLayerD3D9(LayerManagerD3D9 *aManager)
    : ColorLayer(aManager, nullptr)
    , LayerD3D9(aManager)
  {
    mImplData = static_cast<LayerD3D9*>(this);
  }

  
  virtual Layer* GetLayer();

  virtual void RenderLayer();
};

} 
} 

#endif 
