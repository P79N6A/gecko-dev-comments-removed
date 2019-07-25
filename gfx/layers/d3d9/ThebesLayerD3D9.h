




































#ifndef GFX_THEBESLAYERD3D9_H
#define GFX_THEBESLAYERD3D9_H

#include "Layers.h"
#include "LayerManagerD3D9.h"
#include "gfxImageSurface.h"
#include "ReadbackProcessor.h"

namespace mozilla {
namespace layers {

class ReadbackProcessor;
class ShadowBufferD3D9;

class ThebesLayerD3D9 : public ThebesLayer,
                        public LayerD3D9
{
public:
  ThebesLayerD3D9(LayerManagerD3D9 *aManager);
  virtual ~ThebesLayerD3D9();

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  Layer* GetLayer();
  virtual bool IsEmpty();
  virtual void RenderLayer() { RenderThebesLayer(nsnull); }
  virtual void CleanResources();
  virtual void LayerManagerDestroyed();

  void RenderThebesLayer(ReadbackProcessor* aReadback);

private:
  


  nsRefPtr<IDirect3DTexture9> mTexture;
  


  nsRefPtr<IDirect3DTexture9> mTextureOnWhite;
  


  nsIntRect mTextureRect;

  bool HaveTextures(SurfaceMode aMode)
  {
    return mTexture && (aMode != SURFACE_COMPONENT_ALPHA || mTextureOnWhite);
  }

  
  void VerifyContentType(SurfaceMode aMode);

  



  void UpdateTextures(SurfaceMode aMode);

  


  void RenderRegion(const nsIntRegion& aRegion);

  
  void DrawRegion(nsIntRegion &aRegion, SurfaceMode aMode,
                  const nsTArray<ReadbackProcessor::Update>& aReadbackUpdates);

  
  void CreateNewTextures(const gfxIntSize &aSize, SurfaceMode aMode);

  void CopyRegion(IDirect3DTexture9* aSrc, const nsIntPoint &aSrcOffset,
                  IDirect3DTexture9* aDest, const nsIntPoint &aDestOffset,
                  const nsIntRegion &aCopyRegion, nsIntRegion* aValidRegion);
};

class ShadowThebesLayerD3D9 : public ShadowThebesLayer,
                              public LayerD3D9
{
public:
  ShadowThebesLayerD3D9(LayerManagerD3D9 *aManager);
  virtual ~ShadowThebesLayerD3D9();

  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       OptionalThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       OptionalThebesBuffer* aReadOnlyFront, nsIntRegion* aFrontUpdatedRegion);
  virtual void DestroyFrontBuffer();

  virtual void Disconnect();

  
  Layer* GetLayer();
  virtual bool IsEmpty();
  virtual void RenderLayer() { RenderThebesLayer(); }
  virtual void CleanResources();
  virtual void LayerManagerDestroyed();

  void RenderThebesLayer();

private:
  nsRefPtr<ShadowBufferD3D9> mBuffer;
};

} 
} 
#endif 
