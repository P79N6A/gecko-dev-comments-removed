




































#ifndef GFX_READBACKLAYERD3D10_H
#define GFX_READBACKLAYERD3D10_H

#include "LayerManagerD3D10.h"
#include "ReadbackLayer.h"

namespace mozilla {
namespace layers {

class THEBES_API ReadbackLayerD3D10 :
  public ReadbackLayer,
  public LayerD3D10
{
public:
    ReadbackLayerD3D10(LayerManagerD3D10 *aManager)
    : ReadbackLayer(aManager, NULL),
      LayerD3D10(aManager)
  {
      mImplData = static_cast<LayerD3D10*>(this);
  }

  virtual Layer* GetLayer() { return this; }
  virtual void RenderLayer() {}
};

} 
} 
#endif 
