




































#include "LayerManagerD3D10.h"
#include "LayerManagerD3D10Effect.h"
#include "gfxWindowsPlatform.h"
#include "gfxD2DSurface.h"
#include "gfxFailure.h"
#include "cairo-win32.h"
#include "dxgi.h"

#include "ContainerLayerD3D10.h"
#include "ThebesLayerD3D10.h"
#include "ColorLayerD3D10.h"
#include "CanvasLayerD3D10.h"
#include "ReadbackLayerD3D10.h"
#include "ImageLayerD3D10.h"

#include "../d3d9/Nv3DVUtils.h"

#include "gfxCrashReporterUtils.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

typedef HRESULT (WINAPI*D3D10CreateEffectFromMemoryFunc)(
    void *pData,
    SIZE_T DataLength,
    UINT FXFlags,
    ID3D10Device *pDevice, 
    ID3D10EffectPool *pEffectPool,
    ID3D10Effect **ppEffect
);

struct Vertex
{
    float position[2];
};


static const GUID sDeviceAttachments = 
{ 0x592bf306, 0xeed, 0x4f76, { 0x9d, 0x3, 0xa0, 0x84, 0x64, 0x50, 0xf4, 0x72 } };

static const GUID sLayerManagerCount = 
{ 0x716aedb1, 0xc9c3, 0x4b4d, { 0x83, 0x32, 0x6f, 0x65, 0xd4, 0x4a, 0xf6, 0xa8 } };

cairo_user_data_key_t gKeyD3D10Texture;

LayerManagerD3D10::LayerManagerD3D10(nsIWidget *aWidget)
  : mWidget(aWidget)
{
}

struct DeviceAttachments
{
  nsRefPtr<ID3D10Effect> mEffect;
  nsRefPtr<ID3D10InputLayout> mInputLayout;
  nsRefPtr<ID3D10Buffer> mVertexBuffer;
  nsRefPtr<ReadbackManagerD3D10> mReadbackManager;
};

LayerManagerD3D10::~LayerManagerD3D10()
{
  if (mDevice) {
    int referenceCount = 0;
    UINT size = sizeof(referenceCount);
    HRESULT hr = mDevice->GetPrivateData(sLayerManagerCount, &size, &referenceCount);
    NS_ASSERTION(SUCCEEDED(hr), "Reference count not found on device.");
    referenceCount--;
    mDevice->SetPrivateData(sLayerManagerCount, sizeof(referenceCount), &referenceCount);

    if (!referenceCount) {
      DeviceAttachments *attachments;
      size = sizeof(attachments);
      mDevice->GetPrivateData(sDeviceAttachments, &size, &attachments);
      
      
      mDevice->SetPrivateData(sDeviceAttachments, 0, NULL);

      delete attachments;
    }
  }

  Destroy();
}

bool
LayerManagerD3D10::Initialize()
{
  ScopedGfxFeatureReporter reporter("D3D10 Layers");

  HRESULT hr;

  
  if (!mNv3DVUtils) {
    mNv3DVUtils = new Nv3DVUtils();
    if (!mNv3DVUtils) {
      NS_WARNING("Could not create a new instance of Nv3DVUtils.\n");
    }
  }

  
  if (mNv3DVUtils) {
    mNv3DVUtils->Initialize();
  }

  mDevice = gfxWindowsPlatform::GetPlatform()->GetD3D10Device();
  if (!mDevice) {
      return false;
  }

  


  if (mNv3DVUtils) {
    IUnknown* devUnknown = NULL;
    if (mDevice) {
      mDevice->QueryInterface(IID_IUnknown, (void **)&devUnknown);
    }
    mNv3DVUtils->SetDeviceInfo(devUnknown);
  }

  int referenceCount = 0;
  UINT size = sizeof(referenceCount);
  
  mDevice->GetPrivateData(sLayerManagerCount, &size, &referenceCount);
  referenceCount++;
  mDevice->SetPrivateData(sLayerManagerCount, sizeof(referenceCount), &referenceCount);

  DeviceAttachments *attachments;
  size = sizeof(DeviceAttachments*);
  if (FAILED(mDevice->GetPrivateData(sDeviceAttachments, &size, &attachments))) {
    attachments = new DeviceAttachments;
    mDevice->SetPrivateData(sDeviceAttachments, sizeof(attachments), &attachments);

    D3D10CreateEffectFromMemoryFunc createEffect = (D3D10CreateEffectFromMemoryFunc)
	GetProcAddress(LoadLibraryA("d3d10_1.dll"), "D3D10CreateEffectFromMemory");

    if (!createEffect) {
      return false;
    }

    hr = createEffect((void*)g_main,
                      sizeof(g_main),
                      D3D10_EFFECT_SINGLE_THREADED,
                      mDevice,
                      NULL,
                      getter_AddRefs(mEffect));
    
    if (FAILED(hr)) {
      return false;
    }

    attachments->mEffect = mEffect;
  
    D3D10_INPUT_ELEMENT_DESC layout[] =
    {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    D3D10_PASS_DESC passDesc;
    mEffect->GetTechniqueByName("RenderRGBLayerPremul")->GetPassByIndex(0)->
      GetDesc(&passDesc);

    hr = mDevice->CreateInputLayout(layout,
                                    sizeof(layout) / sizeof(D3D10_INPUT_ELEMENT_DESC),
                                    passDesc.pIAInputSignature,
                                    passDesc.IAInputSignatureSize,
                                    getter_AddRefs(mInputLayout));
    
    if (FAILED(hr)) {
      return false;
    }

    attachments->mInputLayout = mInputLayout;
  
    Vertex vertices[] = { {0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0} };
    CD3D10_BUFFER_DESC bufferDesc(sizeof(vertices), D3D10_BIND_VERTEX_BUFFER);
    D3D10_SUBRESOURCE_DATA data;
    data.pSysMem = (void*)vertices;

    hr = mDevice->CreateBuffer(&bufferDesc, &data, getter_AddRefs(mVertexBuffer));

    if (FAILED(hr)) {
      return false;
    }

    attachments->mVertexBuffer = mVertexBuffer;
  } else {
    mEffect = attachments->mEffect;
    mVertexBuffer = attachments->mVertexBuffer;
    mInputLayout = attachments->mInputLayout;
  }

  nsRefPtr<IDXGIDevice> dxgiDevice;
  nsRefPtr<IDXGIAdapter> dxgiAdapter;
  nsRefPtr<IDXGIFactory> dxgiFactory;

  mDevice->QueryInterface(dxgiDevice.StartAssignment());
  dxgiDevice->GetAdapter(getter_AddRefs(dxgiAdapter));

  dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.StartAssignment()));

  DXGI_SWAP_CHAIN_DESC swapDesc;
  ::ZeroMemory(&swapDesc, sizeof(swapDesc));

  swapDesc.BufferDesc.Width = 0;
  swapDesc.BufferDesc.Height = 0;
  swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  swapDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapDesc.SampleDesc.Count = 1;
  swapDesc.SampleDesc.Quality = 0;
  swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapDesc.BufferCount = 1;
  
  
  
  
  
  
  if (gfxWindowsPlatform::IsOptimus()) {
    swapDesc.Flags = 0;
  } else {
    swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
  }
  swapDesc.OutputWindow = (HWND)mWidget->GetNativeData(NS_NATIVE_WINDOW);
  swapDesc.Windowed = TRUE;

  




  hr = dxgiFactory->CreateSwapChain(dxgiDevice, &swapDesc, getter_AddRefs(mSwapChain));

  if (FAILED(hr)) {
    return false;
  }

  
  dxgiFactory->MakeWindowAssociation(swapDesc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES);

  reporter.SetSuccessful();
  return true;
}

void
LayerManagerD3D10::Destroy()
{
  if (!IsDestroyed()) {
    if (mRoot) {
      static_cast<LayerD3D10*>(mRoot->ImplData())->LayerManagerDestroyed();
    }
  }
  LayerManager::Destroy();
}

void
LayerManagerD3D10::SetRoot(Layer *aRoot)
{
  mRoot = aRoot;
}

void
LayerManagerD3D10::BeginTransaction()
{
}

void
LayerManagerD3D10::BeginTransactionWithTarget(gfxContext* aTarget)
{
  mTarget = aTarget;
}

bool
LayerManagerD3D10::EndEmptyTransaction()
{
  if (!mRoot)
    return false;

  EndTransaction(nsnull, nsnull);
  return true;
}

void
LayerManagerD3D10::EndTransaction(DrawThebesLayerCallback aCallback,
                                  void* aCallbackData)
{
  if (mRoot) {
    mCurrentCallbackInfo.Callback = aCallback;
    mCurrentCallbackInfo.CallbackData = aCallbackData;

    
    
    mRoot->ComputeEffectiveTransforms(gfx3DMatrix());

    Render();
    mCurrentCallbackInfo.Callback = nsnull;
    mCurrentCallbackInfo.CallbackData = nsnull;
  }

  mTarget = nsnull;
}

already_AddRefed<ThebesLayer>
LayerManagerD3D10::CreateThebesLayer()
{
  nsRefPtr<ThebesLayer> layer = new ThebesLayerD3D10(this);
  return layer.forget();
}

already_AddRefed<ContainerLayer>
LayerManagerD3D10::CreateContainerLayer()
{
  nsRefPtr<ContainerLayer> layer = new ContainerLayerD3D10(this);
  return layer.forget();
}

already_AddRefed<ImageLayer>
LayerManagerD3D10::CreateImageLayer()
{
  nsRefPtr<ImageLayer> layer = new ImageLayerD3D10(this);
  return layer.forget();
}

already_AddRefed<ColorLayer>
LayerManagerD3D10::CreateColorLayer()
{
  nsRefPtr<ColorLayer> layer = new ColorLayerD3D10(this);
  return layer.forget();
}

already_AddRefed<CanvasLayer>
LayerManagerD3D10::CreateCanvasLayer()
{
  nsRefPtr<CanvasLayer> layer = new CanvasLayerD3D10(this);
  return layer.forget();
}

already_AddRefed<ReadbackLayer>
LayerManagerD3D10::CreateReadbackLayer()
{
  nsRefPtr<ReadbackLayer> layer = new ReadbackLayerD3D10(this);
  return layer.forget();
}

already_AddRefed<ImageContainer>
LayerManagerD3D10::CreateImageContainer()
{
  nsRefPtr<ImageContainer> layer = new ImageContainerD3D10(mDevice);
  return layer.forget();
}

static void ReleaseTexture(void *texture)
{
  static_cast<ID3D10Texture2D*>(texture)->Release();
}

already_AddRefed<gfxASurface>
LayerManagerD3D10::CreateOptimalSurface(const gfxIntSize &aSize,
                                   gfxASurface::gfxImageFormat aFormat)
{
  if ((aFormat != gfxASurface::ImageFormatRGB24 &&
       aFormat != gfxASurface::ImageFormatARGB32)) {
    return LayerManager::CreateOptimalSurface(aSize, aFormat);
  }

  nsRefPtr<ID3D10Texture2D> texture;
  
  CD3D10_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, aSize.width, aSize.height, 1, 1);
  desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
  desc.MiscFlags = D3D10_RESOURCE_MISC_GDI_COMPATIBLE;
  
  HRESULT hr = device()->CreateTexture2D(&desc, NULL, getter_AddRefs(texture));

  if (FAILED(hr)) {
    NS_WARNING("Failed to create new texture for CreateOptimalSurface!");
    return LayerManager::CreateOptimalSurface(aSize, aFormat);
  }

  nsRefPtr<gfxD2DSurface> surface =
    new gfxD2DSurface(texture, aFormat == gfxASurface::ImageFormatRGB24 ?
      gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA);

  if (!surface || surface->CairoStatus()) {
    return LayerManager::CreateOptimalSurface(aSize, aFormat);
  }

  surface->SetData(&gKeyD3D10Texture,
                   texture.forget().get(),
                   ReleaseTexture);

  return surface.forget();
}

TemporaryRef<DrawTarget>
LayerManagerD3D10::CreateDrawTarget(const IntSize &aSize,
                                    SurfaceFormat aFormat)
{
  if ((aFormat != FORMAT_B8G8R8A8 &&
       aFormat != FORMAT_B8G8R8X8)) {
    return LayerManager::CreateDrawTarget(aSize, aFormat);
  }

  nsRefPtr<ID3D10Texture2D> texture;
  
  CD3D10_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, aSize.width, aSize.height, 1, 1);
  desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
  desc.MiscFlags = D3D10_RESOURCE_MISC_GDI_COMPATIBLE;
  
  HRESULT hr = device()->CreateTexture2D(&desc, NULL, getter_AddRefs(texture));

  if (FAILED(hr)) {
    NS_WARNING("Failed to create new texture for CreateOptimalSurface!");
    return LayerManager::CreateDrawTarget(aSize, aFormat);
  }

  RefPtr<DrawTarget> surface =
    Factory::CreateDrawTargetForD3D10Texture(texture, aFormat);

  if (!surface) {
    return LayerManager::CreateDrawTarget(aSize, aFormat);
  }
  
  return surface;
}

ReadbackManagerD3D10*
LayerManagerD3D10::readbackManager()
{
  EnsureReadbackManager();
  return mReadbackManager;
}

void
LayerManagerD3D10::SetViewport(const nsIntSize &aViewport)
{
  mViewport = aViewport;

  D3D10_VIEWPORT viewport;
  viewport.MaxDepth = 1.0f;
  viewport.MinDepth = 0;
  viewport.Width = aViewport.width;
  viewport.Height = aViewport.height;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;

  mDevice->RSSetViewports(1, &viewport);

  gfx3DMatrix projection;
  



  projection._11 = 2.0f / aViewport.width;
  projection._22 = -2.0f / aViewport.height;
  projection._33 = 1.0f;
  projection._41 = -1.0f;
  projection._42 = 1.0f;
  projection._44 = 1.0f;

  HRESULT hr = mEffect->GetVariableByName("mProjection")->
    SetRawValue(&projection._11, 0, 64);

  if (FAILED(hr)) {
    NS_WARNING("Failed to set projection matrix.");
  }
}

void
LayerManagerD3D10::SetupPipeline()
{
  VerifyBufferSize();
  UpdateRenderTarget();

  nsIntRect rect;
  mWidget->GetClientBounds(rect);

  HRESULT hr;

  hr = mEffect->GetVariableByName("vTextureCoords")->AsVector()->
    SetFloatVector(ShaderConstantRectD3D10(0, 0, 1.0f, 1.0f));

  if (FAILED(hr)) {
    NS_WARNING("Failed to set Texture Coordinates.");
    return;
  }

  ID3D10RenderTargetView *view = mRTView;
  mDevice->OMSetRenderTargets(1, &view, NULL);
  mDevice->IASetInputLayout(mInputLayout);

  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  ID3D10Buffer *buffer = mVertexBuffer;
  mDevice->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
  mDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  SetViewport(nsIntSize(rect.width, rect.height));
}

void
LayerManagerD3D10::UpdateRenderTarget()
{
  if (mRTView) {
    return;
  }

  HRESULT hr;

  nsRefPtr<ID3D10Texture2D> backBuf;
  
  hr = mSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)backBuf.StartAssignment());
  if (FAILED(hr)) {
    return;
  }
  
  mDevice->CreateRenderTargetView(backBuf, NULL, getter_AddRefs(mRTView));
}

void
LayerManagerD3D10::VerifyBufferSize()
{
  DXGI_SWAP_CHAIN_DESC swapDesc;
  mSwapChain->GetDesc(&swapDesc);

  nsIntRect rect;
  mWidget->GetClientBounds(rect);

  if (swapDesc.BufferDesc.Width == rect.width &&
      swapDesc.BufferDesc.Height == rect.height) {
    return;
  }

  mRTView = nsnull;
  if (gfxWindowsPlatform::IsOptimus()) {
    mSwapChain->ResizeBuffers(1, rect.width, rect.height,
                              DXGI_FORMAT_B8G8R8A8_UNORM,
                              0);
  } else {
    mSwapChain->ResizeBuffers(1, rect.width, rect.height,
                              DXGI_FORMAT_B8G8R8A8_UNORM,
                              DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE);
  }

}

void
LayerManagerD3D10::EnsureReadbackManager()
{
  if (mReadbackManager) {
    return;
  }

  DeviceAttachments *attachments;
  UINT size = sizeof(DeviceAttachments*);
  if (FAILED(mDevice->GetPrivateData(sDeviceAttachments, &size, &attachments))) {
    
    
    mReadbackManager = new ReadbackManagerD3D10();
    gfx::LogFailure(NS_LITERAL_CSTRING("Couldn't get device attachments for device."));
    return;
  }

  if (attachments->mReadbackManager) {
    mReadbackManager = attachments->mReadbackManager;
    return;
  }

  mReadbackManager = new ReadbackManagerD3D10();
  attachments->mReadbackManager = mReadbackManager;
}

void
LayerManagerD3D10::Render()
{
  static_cast<LayerD3D10*>(mRoot->ImplData())->Validate();

  SetupPipeline();

  float black[] = { 0, 0, 0, 0 };
  device()->ClearRenderTargetView(mRTView, black);

  nsIntRect rect;
  mWidget->GetClientBounds(rect);

  const nsIntRect *clipRect = mRoot->GetClipRect();
  D3D10_RECT r;
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
  device()->RSSetScissorRects(1, &r);

  static_cast<LayerD3D10*>(mRoot->ImplData())->RenderLayer();

  if (mTarget) {
    PaintToTarget();
  } else {
    mSwapChain->Present(0, 0);
  }
}

void
LayerManagerD3D10::PaintToTarget()
{
  nsRefPtr<ID3D10Texture2D> backBuf;
  
  mSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)backBuf.StartAssignment());

  D3D10_TEXTURE2D_DESC bbDesc;
  backBuf->GetDesc(&bbDesc);

  CD3D10_TEXTURE2D_DESC softDesc(bbDesc.Format, bbDesc.Width, bbDesc.Height);
  softDesc.MipLevels = 1;
  softDesc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
  softDesc.Usage = D3D10_USAGE_STAGING;
  softDesc.BindFlags = 0;

  nsRefPtr<ID3D10Texture2D> readTexture;

  device()->CreateTexture2D(&softDesc, NULL, getter_AddRefs(readTexture));

  device()->CopyResource(readTexture, backBuf);

  D3D10_MAPPED_TEXTURE2D map;
  readTexture->Map(0, D3D10_MAP_READ, 0, &map);

  nsRefPtr<gfxImageSurface> tmpSurface =
    new gfxImageSurface((unsigned char*)map.pData,
                        gfxIntSize(bbDesc.Width, bbDesc.Height),
                        map.RowPitch,
                        gfxASurface::ImageFormatARGB32);

  mTarget->SetSource(tmpSurface);
  mTarget->SetOperator(gfxContext::OPERATOR_OVER);
  mTarget->Paint();

  readTexture->Unmap(0);
}

void
LayerManagerD3D10::ReportFailure(const nsACString &aMsg, HRESULT aCode)
{
  
  nsCString msg;
  msg.Append(aMsg);
  msg.AppendLiteral(" Error code: ");
  msg.AppendInt(PRUint32(aCode));
  NS_WARNING(msg.BeginReading());

  gfx::LogFailure(msg);
}

LayerD3D10::LayerD3D10(LayerManagerD3D10 *aManager)
  : mD3DManager(aManager)
{
}

}
}
