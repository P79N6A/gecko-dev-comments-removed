




































#ifndef GFX_THEBESLAYERD3D9_H
#define GFX_THEBESLAYERD3D9_H

#include "Layers.h"
#include "LayerManagerD3D9.h"
#include "gfxImageSurface.h"

namespace mozilla {
namespace layers {

class ThebesLayerD3D9 : public ThebesLayer,
                        public LayerD3D9
{
public:
  ThebesLayerD3D9(LayerManagerD3D9 *aManager);
  virtual ~ThebesLayerD3D9();

  
  void SetVisibleRegion(const nsIntRegion& aRegion);

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  Layer* GetLayer();
  virtual PRBool IsEmpty();
  virtual void RenderLayer();
  virtual void CleanResources();

private:
  


  nsRefPtr<IDirect3DTexture9> mTexture;

  
  nsRefPtr<gfxASurface> mD2DSurface;

  bool mD2DSurfaceInitialized;

  
  void DrawRegion(const nsIntRegion &aRegion);

  
  void CreateNewTexture(const gfxIntSize &aSize);
};

} 
} 
#endif 
