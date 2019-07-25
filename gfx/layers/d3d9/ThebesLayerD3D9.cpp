




































#include "ThebesLayerD3D9.h"
#include "gfxPlatform.h"

#include "gfxWindowsPlatform.h"
#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"
#endif

namespace mozilla {
namespace layers {

ThebesLayerD3D9::ThebesLayerD3D9(LayerManagerD3D9 *aManager)
  : ThebesLayer(aManager, NULL)
  , LayerD3D9(aManager)
  , mD2DSurfaceInitialized(false)
{
  mImplData = static_cast<LayerD3D9*>(this);
  aManager->deviceManager()->mThebesLayers.AppendElement(this);
}

ThebesLayerD3D9::~ThebesLayerD3D9()
{
  if (mD3DManager->deviceManager()) {
    mD3DManager->deviceManager()->mThebesLayers.RemoveElement(this);
  }
}






#define RETENTION_THRESHOLD 16384

void
ThebesLayerD3D9::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.IsEqual(mVisibleRegion)) {
    return;
  }

  nsIntRegion oldVisibleRegion = mVisibleRegion;
  ThebesLayer::SetVisibleRegion(aRegion);

  if (!mTexture) {
    
    
    
    return;
  }

  D3DFORMAT fmt = (CanUseOpaqueSurface() && !mD2DSurface) ?
                    D3DFMT_X8R8G8B8 : D3DFMT_A8R8G8B8;

  D3DSURFACE_DESC desc;
  mTexture->GetLevelDesc(0, &desc);

  if (fmt != desc.Format) {
    
    
    mTexture = nsnull;
  }

  nsRefPtr<IDirect3DTexture9> oldTexture = mTexture;

  nsIntRect oldBounds = oldVisibleRegion.GetBounds();
  nsIntRect newBounds = mVisibleRegion.GetBounds();

  CreateNewTexture(gfxIntSize(newBounds.width, newBounds.height));

  
  
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

  
  
  D3DFORMAT fmt = (CanUseOpaqueSurface() && !mD2DSurface) ?
                    D3DFMT_X8R8G8B8 : D3DFMT_A8R8G8B8;

  if (mTexture) {
    D3DSURFACE_DESC desc;
    mTexture->GetLevelDesc(0, &desc);

    if (fmt != desc.Format) {
      
      
      mTexture = nsnull;
      mValidRegion.SetEmpty();
    }
  }

  if (!mTexture) {
    CreateNewTexture(gfxIntSize(visibleRect.width, visibleRect.height));
    mValidRegion.SetEmpty();
  }

  if (!mValidRegion.IsEqual(mVisibleRegion)) {
    nsIntRegion region;
    region.Sub(mVisibleRegion, mValidRegion);

    DrawRegion(region);

    mValidRegion = mVisibleRegion;
  }

  float quadTransform[4][4];
  






  memset(&quadTransform, 0, sizeof(quadTransform));
  quadTransform[0][0] = (float)visibleRect.width;
  quadTransform[1][1] = (float)visibleRect.height;
  quadTransform[2][2] = 1.0f;
  quadTransform[3][0] = (float)visibleRect.x;
  quadTransform[3][1] = (float)visibleRect.y;
  quadTransform[3][3] = 1.0f;

  device()->SetVertexShaderConstantF(0, &quadTransform[0][0], 4);
  device()->SetVertexShaderConstantF(4, &mTransform._11, 4);

  float opacity[4];
  



  opacity[0] = GetOpacity();
  device()->SetPixelShaderConstantF(0, opacity, 1);

#ifdef CAIRO_HAS_D2D_SURFACE
  if (mD2DSurface && CanUseOpaqueSurface()) {
    mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBLAYER);
  } else
#endif
  mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);

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

void
ThebesLayerD3D9::DrawRegion(const nsIntRegion &aRegion)
{
  HRESULT hr;
  nsIntRect visibleRect = mVisibleRegion.GetBounds();
  nsRefPtr<gfxContext> context;

#ifdef CAIRO_HAS_D2D_SURFACE
  if (mD2DSurface) {
    context = new gfxContext(mD2DSurface);
    nsIntRegionRectIterator iter(aRegion);

    context->Translate(gfxPoint(-visibleRect.x, -visibleRect.y));
    context->NewPath();
    const nsIntRect *iterRect;
    while ((iterRect = iter.Next())) {
      context->Rectangle(gfxRect(iterRect->x, iterRect->y, iterRect->width, iterRect->height));      
    }
    context->Clip();
    if (!mD2DSurfaceInitialized || 
        mD2DSurface->GetContentType() != gfxASurface::CONTENT_COLOR) {
      context->SetOperator(gfxContext::OPERATOR_CLEAR);
      context->Paint();
      context->SetOperator(gfxContext::OPERATOR_OVER);
      mD2DSurfaceInitialized = true;
    }

    LayerManagerD3D9::CallbackInfo cbInfo = mD3DManager->GetCallbackInfo();
    cbInfo.Callback(this, context, aRegion, nsIntRegion(), cbInfo.CallbackData);
    mD2DSurface->Flush();

    
    
    
    
    cairo_d2d_finish_device(gfxWindowsPlatform::GetPlatform()->GetD2DDevice());
    return;
  }
#endif

  D3DFORMAT fmt = CanUseOpaqueSurface() ? D3DFMT_X8R8G8B8 : D3DFMT_A8R8G8B8;
  nsIntRect bounds = aRegion.GetBounds();

  gfxASurface::gfxImageFormat imageFormat = gfxASurface::ImageFormatARGB32;
  nsRefPtr<gfxASurface> destinationSurface;

  nsRefPtr<IDirect3DTexture9> tmpTexture;
  device()->CreateTexture(bounds.width, bounds.height, 1,
                          0, fmt,
                          D3DPOOL_SYSTEMMEM, getter_AddRefs(tmpTexture), NULL);

  nsRefPtr<IDirect3DSurface9> surf;
  HDC dc;
  if (CanUseOpaqueSurface()) {
    hr = tmpTexture->GetSurfaceLevel(0, getter_AddRefs(surf));

    if (FAILED(hr)) {
      
      NS_WARNING("Failed to get texture surface level.");
      return;
    }

    hr = surf->GetDC(&dc);

    if (FAILED(hr)) {
      NS_WARNING("Failed to get device context for texture surface.");
      return;
    }

    destinationSurface = new gfxWindowsSurface(dc);
  } else {
    
    
    
    destinationSurface =
    gfxPlatform::GetPlatform()->
      CreateOffscreenSurface(gfxIntSize(bounds.width,
                                        bounds.height),
                             imageFormat);
  }

  context = new gfxContext(destinationSurface);
  context->Translate(gfxPoint(-bounds.x, -bounds.y));
  LayerManagerD3D9::CallbackInfo cbInfo = mD3DManager->GetCallbackInfo();
  cbInfo.Callback(this, context, aRegion, nsIntRegion(), cbInfo.CallbackData);

  if (CanUseOpaqueSurface()) {
    surf->ReleaseDC(dc);
  } else {
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
  }

  nsRefPtr<IDirect3DSurface9> srcSurface;
  nsRefPtr<IDirect3DSurface9> dstSurface;

  mTexture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));
  tmpTexture->GetSurfaceLevel(0, getter_AddRefs(srcSurface));

  nsIntRegionRectIterator iter(aRegion);
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
}

void
ThebesLayerD3D9::CreateNewTexture(const gfxIntSize &aSize)
{
  if (aSize.width == 0 | aSize.height == 0) {
    
    return;
  }

  mTexture = nsnull;
  PRBool canUseOpaqueSurface = CanUseOpaqueSurface();
#ifdef CAIRO_HAS_D2D_SURFACE
  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
      gfxWindowsPlatform::RENDER_DIRECT2D) {
        if (mD3DManager->deviceManager()->IsD3D9Ex()) {
          
          HANDLE sharedHandle = 0;
          device()->CreateTexture(aSize.width, aSize.height, 1,
                                  D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                                  D3DPOOL_DEFAULT, getter_AddRefs(mTexture), &sharedHandle);

          mD2DSurfaceInitialized = false;
          mD2DSurface = new gfxD2DSurface(sharedHandle, canUseOpaqueSurface ?
            gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA);

          
          if (mD2DSurface->CairoStatus()) {
            mD2DSurface = nsnull;
            mTexture = nsnull;
          }
        }
  }
#endif
  if (!mTexture) {
    device()->CreateTexture(aSize.width, aSize.height, 1,
                            D3DUSAGE_RENDERTARGET, canUseOpaqueSurface ? D3DFMT_X8R8G8B8 : D3DFMT_A8R8G8B8,
                            D3DPOOL_DEFAULT, getter_AddRefs(mTexture), NULL);
  }
}

} 
} 
