




#ifndef GFX_PAINTEDLAYERD3D10_H
#define GFX_PAINTEDLAYERD3D10_H

#include "LayerManagerD3D10.h"

namespace mozilla {
namespace layers {

class PaintedLayerD3D10 : public PaintedLayer,
                         public LayerD3D10
{
public:
  PaintedLayerD3D10(LayerManagerD3D10 *aManager);
  virtual ~PaintedLayerD3D10();

  void Validate(ReadbackProcessor *aReadback);

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  virtual Layer* GetLayer();
  virtual void RenderLayer();
  virtual void Validate() { Validate(nullptr); }
  virtual void LayerManagerDestroyed();

private:
  
  nsRefPtr<ID3D10Texture2D> mTexture;

  
  nsRefPtr<ID3D10ShaderResourceView> mSRView;

  
  nsRefPtr<ID3D10Texture2D> mTextureOnWhite;

  
  nsRefPtr<ID3D10ShaderResourceView> mSRViewOnWhite;

  
  nsIntRect mTextureRect;

  
  SurfaceMode mCurrentSurfaceMode;

  
  void VerifyContentType(SurfaceMode aMode);

  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;

  
  void DrawRegion(nsIntRegion &aRegion, SurfaceMode aMode);

  
  void CreateNewTextures(const gfx::IntSize &aSize, SurfaceMode aMode);

  
  void FillTexturesBlackWhite(const nsIntRegion& aRegion, const nsIntPoint& aOffset);

  
  void CopyRegion(ID3D10Texture2D* aSrc, const nsIntPoint &aSrcOffset,
                  ID3D10Texture2D* aDest, const nsIntPoint &aDestOffset,
                  const nsIntRegion &aCopyRegion, nsIntRegion* aValidRegion);
};

} 
} 
#endif 
