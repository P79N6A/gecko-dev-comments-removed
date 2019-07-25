




































#include "ImageLayerD3D9.h"
#include "gfxImageSurface.h"
#include "yuv_convert.h"
#include "nsIServiceManager.h" 
#include "nsIConsoleService.h" 
#include "nsPrintfCString.h" 
#include "Nv3DVUtils.h"

namespace mozilla {
namespace layers {

static already_AddRefed<IDirect3DTexture9>
SurfaceToTexture(IDirect3DDevice9 *aDevice,
                 gfxASurface *aSurface,
                 const gfxIntSize &aSize)
{

  nsRefPtr<gfxImageSurface> imageSurface = aSurface->GetAsImageSurface();

  if (!imageSurface) {
    imageSurface = new gfxImageSurface(aSize,
                                       gfxASurface::ImageFormatARGB32);
    
    nsRefPtr<gfxContext> context = new gfxContext(imageSurface);
    context->SetSource(aSurface);
    context->SetOperator(gfxContext::OPERATOR_SOURCE);
    context->Paint();
  }

  nsRefPtr<IDirect3DTexture9> texture;
  nsRefPtr<IDirect3DDevice9Ex> deviceEx;
  aDevice->QueryInterface(IID_IDirect3DDevice9Ex,
                          (void**)getter_AddRefs(deviceEx));

  if (deviceEx) {
    
    
    
    if (FAILED(aDevice->
               CreateTexture(aSize.width, aSize.height,
                             1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                             getter_AddRefs(texture), NULL)))
    {
      return NULL;
    }

    nsRefPtr<IDirect3DSurface9> surface;
    if (FAILED(aDevice->
               CreateOffscreenPlainSurface(aSize.width,
                                           aSize.height,
                                           D3DFMT_A8R8G8B8,
                                           D3DPOOL_SYSTEMMEM,
                                           getter_AddRefs(surface),
                                           NULL)))
    {
      return NULL;
    }

    D3DLOCKED_RECT lockedRect;
    surface->LockRect(&lockedRect, NULL, 0);
    for (int y = 0; y < aSize.height; y++) {
      memcpy((char*)lockedRect.pBits + lockedRect.Pitch * y,
             imageSurface->Data() + imageSurface->Stride() * y,
             aSize.width * 4);
    }
    surface->UnlockRect();
    nsRefPtr<IDirect3DSurface9> dstSurface;
    texture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));
    aDevice->UpdateSurface(surface, NULL, dstSurface, NULL);
  } else {
    if (FAILED(aDevice->
               CreateTexture(aSize.width, aSize.height,
                             1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                             getter_AddRefs(texture), NULL)))
    {
      return NULL;
    }

    D3DLOCKED_RECT lockrect;
    
    texture->LockRect(0, &lockrect, NULL, 0);

    
    
    for (int y = 0; y < aSize.height; y++) {
      memcpy((char*)lockrect.pBits + lockrect.Pitch * y,
             imageSurface->Data() + imageSurface->Stride() * y,
             aSize.width * 4);
    }

    texture->UnlockRect(0);
  }

  return texture.forget();
}

ImageContainerD3D9::ImageContainerD3D9(IDirect3DDevice9 *aDevice)
  : ImageContainer(nsnull)
  , mDevice(aDevice)
{
}

already_AddRefed<Image>
ImageContainerD3D9::CreateImage(const Image::Format *aFormats,
                               PRUint32 aNumFormats)
{
  if (!aNumFormats) {
    return nsnull;
  }
  nsRefPtr<Image> img;
  if (aFormats[0] == Image::PLANAR_YCBCR) {
    img = new PlanarYCbCrImageD3D9();
  } else if (aFormats[0] == Image::CAIRO_SURFACE) {
    img = new CairoImageD3D9(mDevice);
  }
  return img.forget();
}

void
ImageContainerD3D9::SetCurrentImage(Image *aImage)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mActiveImage = aImage;
  CurrentImageChanged();
}

already_AddRefed<Image>
ImageContainerD3D9::GetCurrentImage()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  nsRefPtr<Image> retval = mActiveImage;
  return retval.forget();
}

already_AddRefed<gfxASurface>
ImageContainerD3D9::GetCurrentAsSurface(gfxIntSize *aSize)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (!mActiveImage) {
    return nsnull;
  }

  if (mActiveImage->GetFormat() == Image::PLANAR_YCBCR) {
    PlanarYCbCrImageD3D9 *yuvImage =
      static_cast<PlanarYCbCrImageD3D9*>(mActiveImage.get());
    if (yuvImage->HasData()) {
      *aSize = yuvImage->mSize;
    }
  } else if (mActiveImage->GetFormat() == Image::CAIRO_SURFACE) {
    CairoImageD3D9 *cairoImage =
      static_cast<CairoImageD3D9*>(mActiveImage.get());
    *aSize = cairoImage->GetSize();
  }

  return static_cast<ImageD3D9*>(mActiveImage->GetImplData())->GetAsSurface();
}

gfxIntSize
ImageContainerD3D9::GetCurrentSize()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (!mActiveImage) {
    return gfxIntSize(0,0);
  }
  if (mActiveImage->GetFormat() == Image::PLANAR_YCBCR) {
    PlanarYCbCrImageD3D9 *yuvImage =
      static_cast<PlanarYCbCrImageD3D9*>(mActiveImage.get());
    if (!yuvImage->HasData()) {
      return gfxIntSize(0,0);
    }
    return yuvImage->mSize;

  } else if (mActiveImage->GetFormat() == Image::CAIRO_SURFACE) {
    CairoImageD3D9 *cairoImage =
      static_cast<CairoImageD3D9*>(mActiveImage.get());
    return cairoImage->GetSize();
  }

  return gfxIntSize(0,0);
}

PRBool
ImageContainerD3D9::SetLayerManager(LayerManager *aManager)
{
  if (aManager->GetBackendType() == LayerManager::LAYERS_D3D9) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

Layer*
ImageLayerD3D9::GetLayer()
{
  return this;
}

void
ImageLayerD3D9::RenderLayer()
{
  if (!GetContainer()) {
    return;
  }

  nsRefPtr<Image> image = GetContainer()->GetCurrentImage();
  if (!image) {
    return;
  }

  SetShaderTransformAndOpacity();

  if (GetContainer()->GetBackendType() != LayerManager::LAYERS_D3D9)
  {
    gfxIntSize size;
    nsRefPtr<gfxASurface> surface =
      GetContainer()->GetCurrentAsSurface(&size);
    nsRefPtr<IDirect3DTexture9> texture =
      SurfaceToTexture(device(), surface, size);

    device()->SetVertexShaderConstantF(CBvLayerQuad,
                                       ShaderConstantRect(0,
                                                          0,
                                                          size.width,
                                                          size.height),
                                       1);

    if (surface->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA) {
      mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);
    } else {
      mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBLAYER);
    }

    device()->SetTexture(0, texture);
    device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  } else if (image->GetFormat() == Image::PLANAR_YCBCR) {
    PlanarYCbCrImageD3D9 *yuvImage =
      static_cast<PlanarYCbCrImageD3D9*>(image.get());

    if (!yuvImage->HasData()) {
      return;
    }
    yuvImage->AllocateTextures(device());

    device()->SetVertexShaderConstantF(CBvLayerQuad,
                                       ShaderConstantRect(0,
                                                          0,
                                                          yuvImage->mSize.width,
                                                          yuvImage->mSize.height),
                                       1);

    device()->SetVertexShaderConstantF(CBvTextureCoords,
      ShaderConstantRect(
        (float)yuvImage->mData.mPicX / yuvImage->mData.mYSize.width,
        (float)yuvImage->mData.mPicY / yuvImage->mData.mYSize.height,
        (float)yuvImage->mData.mPicSize.width / yuvImage->mData.mYSize.width,
        (float)yuvImage->mData.mPicSize.height / yuvImage->mData.mYSize.height
      ),
      1);

    mD3DManager->SetShaderMode(DeviceManagerD3D9::YCBCRLAYER);

    


    if (mD3DManager->GetNv3DVUtils()) {
      Nv_Stereo_Mode mode;
      switch (yuvImage->mData.mStereoMode) {
      case STEREO_MODE_LEFT_RIGHT:
        mode = NV_STEREO_MODE_LEFT_RIGHT;
        break;
      case STEREO_MODE_RIGHT_LEFT:
        mode = NV_STEREO_MODE_RIGHT_LEFT;
        break;
      case STEREO_MODE_BOTTOM_TOP:
        mode = NV_STEREO_MODE_BOTTOM_TOP;
        break;
      case STEREO_MODE_TOP_BOTTOM:
        mode = NV_STEREO_MODE_TOP_BOTTOM;
        break;
      case STEREO_MODE_MONO:
        mode = NV_STEREO_MODE_MONO;
        break;
      }

      
      mD3DManager->GetNv3DVUtils()->SendNv3DVControl(mode, true, FIREFOX_3DV_APP_HANDLE);

      if (yuvImage->mData.mStereoMode != STEREO_MODE_MONO) {
        mD3DManager->GetNv3DVUtils()->SendNv3DVControl(mode, true, FIREFOX_3DV_APP_HANDLE);

        nsRefPtr<IDirect3DSurface9> renderTarget;
        device()->GetRenderTarget(0, getter_AddRefs(renderTarget));
        mD3DManager->GetNv3DVUtils()->SendNv3DVMetaData((unsigned int)yuvImage->mSize.width,
                                                        (unsigned int)yuvImage->mSize.height, (HANDLE)(yuvImage->mYTexture), (HANDLE)(renderTarget));
      }
    }

    
    
    
    device()->SetTexture(0, yuvImage->mYTexture);
    device()->SetTexture(1, yuvImage->mCbTexture);
    device()->SetTexture(2, yuvImage->mCrTexture);

    device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    device()->SetVertexShaderConstantF(CBvTextureCoords,
      ShaderConstantRect(0, 0, 1.0f, 1.0f), 1);

  } else if (image->GetFormat() == Image::CAIRO_SURFACE) {
    CairoImageD3D9 *cairoImage =
      static_cast<CairoImageD3D9*>(image.get());
    ImageContainerD3D9 *container =
      static_cast<ImageContainerD3D9*>(GetContainer());

    if (container->device() != device()) {
      
      container->SetDevice(device());
    }

    if (cairoImage->device() != device()) {
      cairoImage->SetDevice(device());
    }

    device()->SetVertexShaderConstantF(CBvLayerQuad,
                                       ShaderConstantRect(0,
                                                          0,
                                                          cairoImage->GetSize().width,
                                                          cairoImage->GetSize().height),
                                       1);

    if (cairoImage->HasAlpha()) {
      mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);
    } else {
      mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBLAYER);
    }

    if (mFilter == gfxPattern::FILTER_NEAREST) {
      device()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
      device()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    }
    device()->SetTexture(0, cairoImage->GetOrCreateTexture());
    device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    if (mFilter == gfxPattern::FILTER_NEAREST) {
      device()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      device()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    }
  }

  GetContainer()->NotifyPaintedImage(image);
}

PlanarYCbCrImageD3D9::PlanarYCbCrImageD3D9()
  : PlanarYCbCrImage(static_cast<ImageD3D9*>(this))
  , mHasData(PR_FALSE)
{
}

void
PlanarYCbCrImageD3D9::SetData(const PlanarYCbCrImage::Data &aData)
{
  PRUint32 dummy;
  mBuffer = CopyData(mData, mSize, dummy, aData);

  mHasData = PR_TRUE;
}

void
PlanarYCbCrImageD3D9::AllocateTextures(IDirect3DDevice9 *aDevice)
{
  D3DLOCKED_RECT lockrectY;
  D3DLOCKED_RECT lockrectCb;
  D3DLOCKED_RECT lockrectCr;
  PRUint8* src;
  PRUint8* dest;

  nsRefPtr<IDirect3DSurface9> tmpSurfaceY;
  nsRefPtr<IDirect3DSurface9> tmpSurfaceCb;
  nsRefPtr<IDirect3DSurface9> tmpSurfaceCr;

  nsRefPtr<IDirect3DDevice9Ex> deviceEx;
  aDevice->QueryInterface(IID_IDirect3DDevice9Ex,
                          getter_AddRefs(deviceEx));

  bool isD3D9Ex = deviceEx;

  if (isD3D9Ex) {
    nsRefPtr<IDirect3DTexture9> tmpYTexture;
    nsRefPtr<IDirect3DTexture9> tmpCbTexture;
    nsRefPtr<IDirect3DTexture9> tmpCrTexture;
    
    
    
    aDevice->CreateTexture(mData.mYSize.width, mData.mYSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_DEFAULT,
                            getter_AddRefs(mYTexture), NULL);
    aDevice->CreateTexture(mData.mCbCrSize.width, mData.mCbCrSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_DEFAULT,
                            getter_AddRefs(mCbTexture), NULL);
    aDevice->CreateTexture(mData.mCbCrSize.width, mData.mCbCrSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_DEFAULT,
                            getter_AddRefs(mCrTexture), NULL);
    aDevice->CreateTexture(mData.mYSize.width, mData.mYSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM,
                            getter_AddRefs(tmpYTexture), NULL);
    aDevice->CreateTexture(mData.mCbCrSize.width, mData.mCbCrSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM,
                            getter_AddRefs(tmpCbTexture), NULL);
    aDevice->CreateTexture(mData.mCbCrSize.width, mData.mCbCrSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM,
                            getter_AddRefs(tmpCrTexture), NULL);
    tmpYTexture->GetSurfaceLevel(0, getter_AddRefs(tmpSurfaceY));
    tmpCbTexture->GetSurfaceLevel(0, getter_AddRefs(tmpSurfaceCb));
    tmpCrTexture->GetSurfaceLevel(0, getter_AddRefs(tmpSurfaceCr));
    tmpSurfaceY->LockRect(&lockrectY, NULL, 0);
    tmpSurfaceCb->LockRect(&lockrectCb, NULL, 0);
    tmpSurfaceCr->LockRect(&lockrectCr, NULL, 0);
  } else {
    aDevice->CreateTexture(mData.mYSize.width, mData.mYSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_MANAGED,
                            getter_AddRefs(mYTexture), NULL);
    aDevice->CreateTexture(mData.mCbCrSize.width, mData.mCbCrSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_MANAGED,
                            getter_AddRefs(mCbTexture), NULL);
    aDevice->CreateTexture(mData.mCbCrSize.width, mData.mCbCrSize.height,
                            1, 0, D3DFMT_L8, D3DPOOL_MANAGED,
                            getter_AddRefs(mCrTexture), NULL);

    
    mYTexture->LockRect(0, &lockrectY, NULL, 0);
    mCbTexture->LockRect(0, &lockrectCb, NULL, 0);
    mCrTexture->LockRect(0, &lockrectCr, NULL, 0);
  }

  src  = mData.mYChannel;
  
  dest = (PRUint8*)lockrectY.pBits;

  
  for (int h=0; h<mData.mYSize.height; h++) {
    memcpy(dest, src, mData.mYSize.width);
    dest += lockrectY.Pitch;
    src += mData.mYStride;
  }

  src  = mData.mCbChannel;
  
  dest = (PRUint8*)lockrectCb.pBits;

  
  for (int h=0; h<mData.mCbCrSize.height; h++) {
    memcpy(dest, src, mData.mCbCrSize.width);
    dest += lockrectCb.Pitch;
    src += mData.mCbCrStride;
  }

  src  = mData.mCrChannel;
  
  dest = (PRUint8*)lockrectCr.pBits;

  
  for (int h=0; h<mData.mCbCrSize.height; h++) {
    memcpy(dest, src, mData.mCbCrSize.width);
    dest += lockrectCr.Pitch;
    src += mData.mCbCrStride;
  }

  if (isD3D9Ex) {
    tmpSurfaceY->UnlockRect();
    tmpSurfaceCb->UnlockRect();
    tmpSurfaceCr->UnlockRect();
    nsRefPtr<IDirect3DSurface9> dstSurface;
    mYTexture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));
    aDevice->UpdateSurface(tmpSurfaceY, NULL, dstSurface, NULL);
    mCbTexture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));
    aDevice->UpdateSurface(tmpSurfaceCb, NULL, dstSurface, NULL);
    mCrTexture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));
    aDevice->UpdateSurface(tmpSurfaceCr, NULL, dstSurface, NULL);
  } else {
    mYTexture->UnlockRect(0);
    mCbTexture->UnlockRect(0);
    mCrTexture->UnlockRect(0);
  }
}

void
PlanarYCbCrImageD3D9::FreeTextures()
{
}

already_AddRefed<gfxASurface>
PlanarYCbCrImageD3D9::GetAsSurface()
{
  nsRefPtr<gfxImageSurface> imageSurface =
    new gfxImageSurface(mSize, gfxASurface::ImageFormatRGB24);
  
  gfx::YUVType type = 
    gfx::TypeFromSize(mData.mYSize.width,
                      mData.mYSize.height,
                      mData.mCbCrSize.width,
                      mData.mCbCrSize.height);

  
  gfx::ConvertYCbCrToRGB32(mData.mYChannel,
                           mData.mCbChannel,
                           mData.mCrChannel,
                           imageSurface->Data(),
                           mData.mPicX,
                           mData.mPicY,
                           mData.mPicSize.width,
                           mData.mPicSize.height,
                           mData.mYStride,
                           mData.mCbCrStride,
                           imageSurface->Stride(),
                           type);

  return imageSurface.forget().get();
}

CairoImageD3D9::~CairoImageD3D9()
{
}

void
CairoImageD3D9::SetDevice(IDirect3DDevice9 *aDevice)
{
  mTexture = NULL;
  mDevice = aDevice;
}

void
CairoImageD3D9::SetData(const CairoImage::Data &aData)
{
  mSize = aData.mSize;
  mCachedSurface = aData.mSurface;
  mTexture = NULL;
}

IDirect3DTexture9*
CairoImageD3D9::GetOrCreateTexture()
{
  if (mTexture)
    return mTexture;

  mTexture = SurfaceToTexture(mDevice, mCachedSurface, mSize);

  
  return mTexture;
}

already_AddRefed<gfxASurface>
CairoImageD3D9::GetAsSurface()
{
  nsRefPtr<gfxASurface> surface = mCachedSurface;
  return surface.forget();
}

} 
} 
