





































#ifndef GFX_COLORLAYERD3D9_H
#define GFX_COLORLAYERD3D9_H

#include "LayerManagerD3D9.h"

namespace mozilla {
namespace layers {

class THEBES_API ColorLayerD3D9 : public ColorLayer,
                                 public LayerD3D9
{
public:
  ColorLayerD3D9(LayerManagerD3D9 *aManager)
    : ColorLayer(aManager, NULL)
    , LayerD3D9(aManager)
  {
    mImplData = static_cast<LayerD3D9*>(this);
  }

  
  virtual Layer* GetLayer();

  virtual void RenderLayer();
};

class ShadowColorLayerD3D9 : public ShadowColorLayer,
                            public LayerD3D9
{
public:
  ShadowColorLayerD3D9(LayerManagerD3D9 *aManager)
    : ShadowColorLayer(aManager, NULL)
    , LayerD3D9(aManager)
  { 
    mImplData = static_cast<LayerD3D9*>(this);
  }
  ~ShadowColorLayerD3D9() { Destroy(); }

  
  virtual Layer* GetLayer() { return this; }

  virtual void Destroy() { }

  virtual void RenderLayer();
};

} 
} 

#endif 
