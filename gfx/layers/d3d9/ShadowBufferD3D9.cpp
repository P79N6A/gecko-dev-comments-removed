




































#include "LayerManagerD3D9.h"
#include "ShadowBufferD3D9.h"

#include "gfxWindowsSurface.h"
#include "gfxWindowsPlatform.h"


namespace mozilla {
namespace layers {

void 
ShadowBufferD3D9::Upload(gfxASurface* aUpdate, 
                         const nsIntRect& aVisibleRect)
{

  gfxIntSize size = aUpdate->GetSize();

  if (GetSize() != nsIntSize(size.width, size.height)) {
    mLayer->device()->CreateTexture(size.width, size.height, 1,
                          D3DUSAGE_DYNAMIC,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT, getter_AddRefs(mTexture), NULL);

    mTextureRect = aVisibleRect;
  }

  LockTextureRectD3D9 textureLock(mTexture);
  if (!textureLock.HasLock()) {
    NS_WARNING("Failed to lock ShadowBufferD3D9 texture.");
    return;
  }

  D3DLOCKED_RECT r = textureLock.GetLockRect();

  nsRefPtr<gfxImageSurface> imgSurface =
    new gfxImageSurface((unsigned char *)r.pBits,
                        GetSize(),
                        r.Pitch,
                        gfxASurface::ImageFormatARGB32);

  nsRefPtr<gfxContext> context = new gfxContext(imgSurface);
  context->SetSource(aUpdate);
  context->SetOperator(gfxContext::OPERATOR_SOURCE);
  context->Paint();

  imgSurface = NULL;
}

void 
ShadowBufferD3D9::RenderTo(LayerManagerD3D9 *aD3DManager, 
                           const nsIntRegion& aVisibleRegion)
{
  mLayer->SetShaderTransformAndOpacity();

  aD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);
  mLayer->device()->SetTexture(0, mTexture);

  nsIntRegionRectIterator iter(aVisibleRegion);

  const nsIntRect *iterRect;
  while ((iterRect = iter.Next())) {
    mLayer->device()->SetVertexShaderConstantF(CBvLayerQuad,
                                       ShaderConstantRect(iterRect->x,
                                                          iterRect->y,
                                                          iterRect->width,
                                                          iterRect->height),
                                       1);

    mLayer->device()->SetVertexShaderConstantF(CBvTextureCoords,
      ShaderConstantRect(
        (float)(iterRect->x - mTextureRect.x) / (float)mTextureRect.width,
        (float)(iterRect->y - mTextureRect.y) / (float)mTextureRect.height,
        (float)iterRect->width / (float)mTextureRect.width,
        (float)iterRect->height / (float)mTextureRect.height), 1);

    mLayer->device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }
}

} 
} 
