




































#include "LayerManagerD3D9.h"

#include "ThebesLayerD3D9.h"
#include "ContainerLayerD3D9.h"
#include "ImageLayerD3D9.h"
#include "ColorLayerD3D9.h"
#include "CanvasLayerD3D9.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "gfxWindowsPlatform.h"
#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"
#endif

namespace mozilla {
namespace layers {

DeviceManagerD3D9 *LayerManagerD3D9::mDeviceManager = nsnull;

LayerManagerD3D9::LayerManagerD3D9(nsIWidget *aWidget)
  : mIs3DEnabled(PR_FALSE)
{
    mWidget = aWidget;
    mCurrentCallbackInfo.Callback = NULL;
    mCurrentCallbackInfo.CallbackData = NULL;
}

LayerManagerD3D9::~LayerManagerD3D9()
{
  


  mSwapChain = nsnull;

  if (mDeviceManager && mDeviceManager->Release() == 0) {
    mDeviceManager = nsnull;
  }
}

PRBool
LayerManagerD3D9::Initialize()
{
   
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID); 
  prefs->GetBoolPref("gfx.3d_video.enabled", &mIs3DEnabled); 

  if (!mDeviceManager) {
    mDeviceManager = new DeviceManagerD3D9;
    mDeviceManager->AddRef();

    if (!mDeviceManager->Init()) {
      mDeviceManager->Release();
      mDeviceManager = nsnull;
      return PR_FALSE;
    }
  } else {
    mDeviceManager->AddRef();
  }

  mSwapChain = mDeviceManager->
    CreateSwapChain((HWND)mWidget->GetNativeData(NS_NATIVE_WINDOW));

  if (!mSwapChain) {
    return PR_FALSE;
  }

  return PR_TRUE;
}

void
LayerManagerD3D9::SetClippingRegion(const nsIntRegion &aClippingRegion)
{
  mClippingRegion = aClippingRegion;
}

void
LayerManagerD3D9::BeginTransaction()
{
}

void
LayerManagerD3D9::BeginTransactionWithTarget(gfxContext *aTarget)
{
  mTarget = aTarget;
}

void
LayerManagerD3D9::EndConstruction()
{
}

void
LayerManagerD3D9::EndTransaction(DrawThebesLayerCallback aCallback,
                                 void* aCallbackData)
{
  mCurrentCallbackInfo.Callback = aCallback;
  mCurrentCallbackInfo.CallbackData = aCallbackData;
  Render();
  
  mCurrentCallbackInfo.Callback = NULL;
  mCurrentCallbackInfo.CallbackData = NULL;
  
  mTarget = NULL;
}

void
LayerManagerD3D9::SetRoot(Layer *aLayer)
{
  mRoot = aLayer;
}

already_AddRefed<ThebesLayer>
LayerManagerD3D9::CreateThebesLayer()
{
  nsRefPtr<ThebesLayer> layer = new ThebesLayerD3D9(this);
  return layer.forget();
}

already_AddRefed<ContainerLayer>
LayerManagerD3D9::CreateContainerLayer()
{
  nsRefPtr<ContainerLayer> layer = new ContainerLayerD3D9(this);
  return layer.forget();
}

already_AddRefed<ImageLayer>
LayerManagerD3D9::CreateImageLayer()
{
  nsRefPtr<ImageLayer> layer = new ImageLayerD3D9(this);
  return layer.forget();
}

already_AddRefed<ColorLayer>
LayerManagerD3D9::CreateColorLayer()
{
  nsRefPtr<ColorLayer> layer = new ColorLayerD3D9(this);
  return layer.forget();
}

already_AddRefed<CanvasLayer>
LayerManagerD3D9::CreateCanvasLayer()
{
  nsRefPtr<CanvasLayer> layer = new CanvasLayerD3D9(this);
  return layer.forget();
}

already_AddRefed<ImageContainer>
LayerManagerD3D9::CreateImageContainer()
{
  nsRefPtr<ImageContainer> container = new ImageContainerD3D9(this);
  return container.forget();
}

cairo_user_data_key_t gKeyD3D9Texture;

void ReleaseTexture(void *texture)
{
  static_cast<IDirect3DTexture9*>(texture)->Release();
}

already_AddRefed<gfxASurface>
LayerManagerD3D9::CreateOptimalSurface(const gfxIntSize &aSize,
                                   gfxASurface::gfxImageFormat aFormat)
{
#ifdef CAIRO_HAS_D2D_SURFACE
  if ((aFormat != gfxASurface::ImageFormatRGB24 &&
       aFormat != gfxASurface::ImageFormatARGB32) ||
      gfxWindowsPlatform::GetPlatform()->GetRenderMode() !=
        gfxWindowsPlatform::RENDER_DIRECT2D ||
      !deviceManager()->IsD3D9Ex()) {
    return LayerManager::CreateOptimalSurface(aSize, aFormat);
  }

  nsRefPtr<IDirect3DTexture9> texture;
  
  HANDLE sharedHandle = 0;
  device()->CreateTexture(aSize.width, aSize.height, 1,
                          D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT, getter_AddRefs(texture), &sharedHandle);

  nsRefPtr<gfxD2DSurface> surface =
    new gfxD2DSurface(sharedHandle, aFormat == gfxASurface::ImageFormatRGB24 ?
      gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA);

  if (!surface || surface->CairoStatus()) {
    return LayerManager::CreateOptimalSurface(aSize, aFormat);
  }

  surface->SetData(&gKeyD3D9Texture,
                   texture.forget().get(),
                   ReleaseTexture);

  return surface.forget();
#else
  return LayerManager::CreateOptimalSurface(aSize, aFormat);
#endif
}

void
LayerManagerD3D9::Render()
{
  if (!mSwapChain->PrepareForRendering()) {
    return;
  }
  deviceManager()->SetupRenderState();

  SetupPipeline();
  nsIntRect rect;
  mWidget->GetClientBounds(rect);

  device()->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 0, 0);

  device()->BeginScene();

  if (mRoot) {
    const nsIntRect *clipRect = mRoot->GetClipRect();
    RECT r;
    if (clipRect) {
      r.left = (LONG)clipRect->x;
      r.top = (LONG)clipRect->y;
      r.right = (LONG)(clipRect->x + clipRect->width);
      r.bottom = (LONG)(clipRect->y + clipRect->height);
    } else {
      r.left = r.top = 0;
      r.right = rect.width;
      r.bottom = rect.height;
    }
    device()->SetScissorRect(&r);

    static_cast<LayerD3D9*>(mRoot->ImplData())->RenderLayer();
  }

  device()->EndScene();

  if (!mTarget) {
    const nsIntRect *r;
    for (nsIntRegionRectIterator iter(mClippingRegion);
         (r = iter.Next()) != nsnull;) {
      mSwapChain->Present(*r);
    }
  } else {
    PaintToTarget();
  }
}

void
LayerManagerD3D9::SetupPipeline()
{
  nsIntRect rect;
  mWidget->GetClientBounds(rect);

  float viewMatrix[4][4];
  



  memset(&viewMatrix, 0, sizeof(viewMatrix));
  viewMatrix[0][0] = 2.0f / rect.width;
  viewMatrix[1][1] = -2.0f / rect.height;
  viewMatrix[2][2] = 1.0f;
  viewMatrix[3][0] = -1.0f;
  viewMatrix[3][1] = 1.0f;
  viewMatrix[3][3] = 1.0f;

  HRESULT hr = device()->SetVertexShaderConstantF(8, &viewMatrix[0][0], 4);

  if (FAILED(hr)) {
    NS_WARNING("Failed to set projection shader constant!");
  }

  hr = device()->SetVertexShaderConstantF(13, ShaderConstantRect(0, 0, 1.0f, 1.0f), 1);

  if (FAILED(hr)) {
    NS_WARNING("Failed to set texCoords shader constant!");
  }
}

void
LayerManagerD3D9::PaintToTarget()
{
  nsRefPtr<IDirect3DSurface9> backBuff;
  nsRefPtr<IDirect3DSurface9> destSurf;
  device()->GetRenderTarget(0, getter_AddRefs(backBuff));

  D3DSURFACE_DESC desc;
  backBuff->GetDesc(&desc);

  device()->CreateOffscreenPlainSurface(desc.Width, desc.Height,
                                       D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
                                       getter_AddRefs(destSurf), NULL);

  device()->GetRenderTargetData(backBuff, destSurf);

  D3DLOCKED_RECT rect;
  destSurf->LockRect(&rect, NULL, D3DLOCK_READONLY);

  nsRefPtr<gfxImageSurface> imageSurface =
    new gfxImageSurface((unsigned char*)rect.pBits,
                        gfxIntSize(desc.Width, desc.Height),
                        rect.Pitch,
                        gfxASurface::ImageFormatARGB32);

  mTarget->SetSource(imageSurface);
  mTarget->SetOperator(gfxContext::OPERATOR_OVER);
  mTarget->Paint();
  destSurf->UnlockRect();
}

LayerD3D9::LayerD3D9(LayerManagerD3D9 *aManager)
  : mD3DManager(aManager)
{
}

} 
} 
