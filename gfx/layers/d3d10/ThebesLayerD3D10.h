




































#ifndef GFX_THEBESLAYERD3D10_H
#define GFX_THEBESLAYERD3D10_H

#include "Layers.h"
#include "LayerManagerD3D10.h"

namespace mozilla {
namespace layers {

class ThebesLayerD3D10 : public ThebesLayer,
                         public LayerD3D10
{
public:
  ThebesLayerD3D10(LayerManagerD3D10 *aManager);
  virtual ~ThebesLayerD3D10();

  void Validate(ReadbackProcessor *aReadback);

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  virtual Layer* GetLayer();
  virtual void RenderLayer();
  virtual void Validate() { Validate(nsnull); }
  virtual void LayerManagerDestroyed();

private:
  
  nsRefPtr<ID3D10Texture2D> mTexture;

  
  nsRefPtr<ID3D10ShaderResourceView> mSRView;

  
  nsRefPtr<ID3D10Texture2D> mTextureOnWhite;

  
  nsRefPtr<ID3D10ShaderResourceView> mSRViewOnWhite;

  
  nsIntRect mTextureRect;

  
  SurfaceMode mCurrentSurfaceMode;

  
  void VerifyContentType(SurfaceMode aMode);

  
  nsRefPtr<gfxASurface> mD2DSurface;

  
  nsRefPtr<gfxASurface> mD2DSurfaceOnWhite;

  
  void DrawRegion(nsIntRegion &aRegion, SurfaceMode aMode);

  
  void CreateNewTextures(const gfxIntSize &aSize, SurfaceMode aMode);

  
  void CopyRegion(ID3D10Texture2D* aSrc, const nsIntPoint &aSrcOffset,
                  ID3D10Texture2D* aDest, const nsIntPoint &aDestOffset,
                  const nsIntRegion &aCopyRegion, nsIntRegion* aValidRegion);
};

} 
} 
#endif 
