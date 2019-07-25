




































#include "ThebesLayerD3D9.h"
#include "gfxPlatform.h"

namespace mozilla {
namespace layers {

ThebesLayerD3D9::ThebesLayerD3D9(LayerManagerD3D9 *aManager)
  : ThebesLayer(aManager, NULL)
  , LayerD3D9(aManager)
{
  mImplData = static_cast<LayerD3D9*>(this);
  aManager->mThebesLayers.AppendElement(this);
}

ThebesLayerD3D9::~ThebesLayerD3D9()
{
  mD3DManager->mThebesLayers.RemoveElement(this);
}






#define RETENTION_THRESHOLD 16384

void
ThebesLayerD3D9::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.IsEqual(mVisibleRegion)) {
    return;
  }
  nsIntRegion oldVisibleRegion = mVisibleRegion;
  nsRefPtr<IDirect3DTexture9> oldTexture = mTexture;

  ThebesLayer::SetVisibleRegion(aRegion);

  nsIntRect oldBounds = oldVisibleRegion.GetBounds();
  nsIntRect newBounds = mVisibleRegion.GetBounds();
  
  device()->CreateTexture(newBounds.width, newBounds.height, 1,
                          D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT, getter_AddRefs(mTexture), NULL);

  
  
  oldVisibleRegion.And(oldVisibleRegion, mVisibleRegion);
  
  oldVisibleRegion.And(oldVisibleRegion, mValidRegion);

  nsIntRect largeRect = oldVisibleRegion.GetLargestRectangle();

  
  
  
  
  
  
  if (!oldTexture || !mTexture ||
      largeRect.width * largeRect.height < RETENTION_THRESHOLD) {
    mValidRegion.SetEmpty();
    return;
  }

  nsRefPtr<IDirect3DSurface9> srcSurface, dstSurface;
  oldTexture->GetSurfaceLevel(0, getter_AddRefs(srcSurface));
  mTexture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));

  nsIntRegion retainedRegion;
  nsIntRegionRectIterator iter(oldVisibleRegion);
  const nsIntRect *r;
  while ((r = iter.Next())) {
    if (r->width * r->height > RETENTION_THRESHOLD) {
      RECT oldRect, newRect;

      
      
      oldRect.left = r->x - oldBounds.x;
      oldRect.top = r->y - oldBounds.y;
      oldRect.right = oldRect.left + r->width;
      oldRect.bottom = oldRect.top + r->height;

      newRect.left = r->x - newBounds.x;
      newRect.top = r->y - newBounds.y;
      newRect.right = newRect.left + r->width;
      newRect.bottom = newRect.top + r->height;

      
      HRESULT hr = device()->
        StretchRect(srcSurface, &oldRect, dstSurface, &newRect, D3DTEXF_NONE);

      if (SUCCEEDED(hr)) {
        retainedRegion.Or(retainedRegion, *r);
      }
    }
  }

  
  mValidRegion.And(mValidRegion, retainedRegion);  
}


void
ThebesLayerD3D9::InvalidateRegion(const nsIntRegion &aRegion)
{
  mValidRegion.Sub(mValidRegion, aRegion);
}

void
ThebesLayerD3D9::RenderLayer()
{
  if (mVisibleRegion.IsEmpty()) {
    return;
  }
  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  if (!mTexture) {
    device()->CreateTexture(visibleRect.width, visibleRect.height, 1,
                            D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                            D3DPOOL_DEFAULT, getter_AddRefs(mTexture), NULL);
    mValidRegion.SetEmpty();
  }

  if (!mValidRegion.IsEqual(mVisibleRegion)) {
    nsIntRegion region;
    region.Sub(mVisibleRegion, mValidRegion);
    nsIntRect bounds = region.GetBounds();

    gfxASurface::gfxImageFormat imageFormat = gfxASurface::ImageFormatARGB32;;
    nsRefPtr<gfxASurface> destinationSurface;
    nsRefPtr<gfxContext> context;

    
    
    
    
    destinationSurface =
      gfxPlatform::GetPlatform()->
        CreateOffscreenSurface(gfxIntSize(bounds.width,
                                          bounds.height),
                               imageFormat);

    context = new gfxContext(destinationSurface);
    context->Translate(gfxPoint(-bounds.x, -bounds.y));
    LayerManagerD3D9::CallbackInfo cbInfo = mD3DManager->GetCallbackInfo();
    cbInfo.Callback(this, context, region, nsIntRegion(), cbInfo.CallbackData);

    nsRefPtr<IDirect3DTexture9> tmpTexture;
    device()->CreateTexture(bounds.width, bounds.height, 1,
                            0, D3DFMT_A8R8G8B8,
                            D3DPOOL_SYSTEMMEM, getter_AddRefs(tmpTexture), NULL);

    D3DLOCKED_RECT r;
    tmpTexture->LockRect(0, &r, NULL, 0);

    nsRefPtr<gfxImageSurface> imgSurface =
      new gfxImageSurface((unsigned char *)r.pBits,
                          gfxIntSize(bounds.width,
                                     bounds.height),
                          r.Pitch,
                          imageFormat);

    context = new gfxContext(imgSurface);
    context->SetSource(destinationSurface);
    context->SetOperator(gfxContext::OPERATOR_SOURCE);
    context->Paint();

    imgSurface = NULL;

    tmpTexture->UnlockRect(0);

    nsRefPtr<IDirect3DSurface9> srcSurface;
    nsRefPtr<IDirect3DSurface9> dstSurface;

    mTexture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));
    tmpTexture->GetSurfaceLevel(0, getter_AddRefs(srcSurface));

    nsIntRegionRectIterator iter(region);
    const nsIntRect *iterRect;
    while ((iterRect = iter.Next())) {
      RECT rect;
      rect.left = iterRect->x - bounds.x;
      rect.top = iterRect->y - bounds.y;
      rect.right = rect.left + iterRect->width;
      rect.bottom = rect.top + iterRect->height;
      POINT point;
      point.x = iterRect->x - visibleRect.x;
      point.y = iterRect->y - visibleRect.y;
      device()->UpdateSurface(srcSurface, &rect, dstSurface, &point);
    }
    mValidRegion = mVisibleRegion;
  }

  float quadTransform[4][4];
  






  memset(&quadTransform, 0, sizeof(quadTransform));
  quadTransform[0][0] = (float)visibleRect.width;
  quadTransform[1][1] = (float)visibleRect.height;
  quadTransform[2][2] = 1.0f;
  quadTransform[3][0] = (float)visibleRect.x - 0.5f;
  quadTransform[3][1] = (float)visibleRect.y - 0.5f;
  quadTransform[3][3] = 1.0f;

  device()->SetVertexShaderConstantF(0, &quadTransform[0][0], 4);
  device()->SetVertexShaderConstantF(4, &mTransform._11, 4);

  float opacity[4];
  



  opacity[0] = GetOpacity();
  device()->SetPixelShaderConstantF(0, opacity, 1);

  mD3DManager->SetShaderMode(LayerManagerD3D9::RGBLAYER);

  device()->SetTexture(0, mTexture);
  device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

void
ThebesLayerD3D9::CleanResources()
{
  mTexture = nsnull;
}

Layer*
ThebesLayerD3D9::GetLayer()
{
  return this;
}

PRBool
ThebesLayerD3D9::IsEmpty()
{
  return !mTexture;
}

} 
} 
