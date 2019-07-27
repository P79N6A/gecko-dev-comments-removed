




#ifndef GFX_PAINTEDLAYERD3D9_H
#define GFX_PAINTEDLAYERD3D9_H

#include "Layers.h"
#include "LayerManagerD3D9.h"
#include "ReadbackProcessor.h"

namespace mozilla {
namespace layers {

class ReadbackProcessor;

class PaintedLayerD3D9 : public PaintedLayer,
                        public LayerD3D9
{
public:
  PaintedLayerD3D9(LayerManagerD3D9 *aManager);
  virtual ~PaintedLayerD3D9();

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  Layer* GetLayer();
  virtual bool IsEmpty();
  virtual void RenderLayer() { RenderPaintedLayer(nullptr); }
  virtual void CleanResources();
  virtual void LayerManagerDestroyed();

  void RenderPaintedLayer(ReadbackProcessor* aReadback);

private:
  


  nsRefPtr<IDirect3DTexture9> mTexture;
  


  nsRefPtr<IDirect3DTexture9> mTextureOnWhite;
  


  nsIntRect mTextureRect;

  bool HaveTextures(SurfaceMode aMode)
  {
    return mTexture && (aMode != SurfaceMode::SURFACE_COMPONENT_ALPHA || mTextureOnWhite);
  }

  
  void VerifyContentType(SurfaceMode aMode);

  



  void UpdateTextures(SurfaceMode aMode);

  


  void RenderRegion(const nsIntRegion& aRegion);

  
  void DrawRegion(nsIntRegion &aRegion, SurfaceMode aMode,
                  const nsTArray<ReadbackProcessor::Update>& aReadbackUpdates);

  
  void CreateNewTextures(const gfx::IntSize &aSize, SurfaceMode aMode);

  void CopyRegion(IDirect3DTexture9* aSrc, const nsIntPoint &aSrcOffset,
                  IDirect3DTexture9* aDest, const nsIntPoint &aDestOffset,
                  const nsIntRegion &aCopyRegion, nsIntRegion* aValidRegion);
};

} 
} 
#endif 
