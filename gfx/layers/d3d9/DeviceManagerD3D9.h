




#ifndef GFX_DEVICEMANAGERD3D9_H
#define GFX_DEVICEMANAGERD3D9_H

#include "gfxTypes.h"
#include "nsAutoPtr.h"
#include "d3d9.h"
#include "nsTArray.h"
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/gfx/Rect.h"

namespace mozilla {
namespace layers {

class DeviceManagerD3D9;
class Nv3DVUtils;
class Layer;
class TextureSourceD3D9;


const int CBmLayerTransform = 0;
const int CBmProjection = 4;
const int CBvRenderTargetOffset = 8;
const int CBvTextureCoords = 9;
const int CBvLayerQuad = 10;

const int CBfLayerOpacity = 0;
const int CBvColor = 0;

enum DeviceManagerState {
  
  DeviceOK,
  
  DeviceFail,
  
  
  DeviceMustRecreate,
};









struct ShaderConstantRect
{
  float mX, mY, mWidth, mHeight;

  
  
  ShaderConstantRect(float aX, float aY, float aWidth, float aHeight)
    : mX(aX), mY(aY), mWidth(aWidth), mHeight(aHeight)
  { }

  ShaderConstantRect(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight)
    : mX((float)aX), mY((float)aY)
    , mWidth((float)aWidth), mHeight((float)aHeight)
  { }

  ShaderConstantRect(int32_t aX, int32_t aY, float aWidth, float aHeight)
    : mX((float)aX), mY((float)aY), mWidth(aWidth), mHeight(aHeight)
  { }

  
  operator float* () { return &mX; }
};





class SwapChainD3D9 final
{
  NS_INLINE_DECL_REFCOUNTING(SwapChainD3D9)
public:

  









  DeviceManagerState PrepareForRendering();

  already_AddRefed<IDirect3DSurface9> GetBackBuffer();

  



  void Present(const gfx::IntRect &aRect);
  void Present();

private:
  friend class DeviceManagerD3D9;

  SwapChainD3D9(DeviceManagerD3D9 *aDeviceManager);

  
  ~SwapChainD3D9();
  
  bool Init(HWND hWnd);

  



  void Reset();

  nsRefPtr<IDirect3DSwapChain9> mSwapChain;
  nsRefPtr<DeviceManagerD3D9> mDeviceManager;
  HWND mWnd;
};






class DeviceManagerD3D9 final
{
public:
  DeviceManagerD3D9();
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DeviceManagerD3D9)

  






  bool Init();

  


  void SetupRenderState();

  


  already_AddRefed<SwapChainD3D9> CreateSwapChain(HWND hWnd);

  IDirect3DDevice9 *device() { return mDevice; }

  bool IsD3D9Ex() { return mDeviceEx; }

  bool HasDynamicTextures() { return mHasDynamicTextures; }

  enum ShaderMode {
    RGBLAYER,
    RGBALAYER,
    COMPONENTLAYERPASS1,
    COMPONENTLAYERPASS2,
    YCBCRLAYER,
    SOLIDCOLORLAYER
  };

  
  uint32_t SetShaderMode(ShaderMode aMode, MaskType aMaskType);

  

 
  Nv3DVUtils *GetNv3DVUtils()  { return mNv3DVUtils; }

  


  bool DeviceWasRemoved() { return mDeviceWasRemoved; }

  uint32_t GetDeviceResetCount() { return mDeviceResetCount; }

  int32_t GetMaxTextureSize() { return mMaxTextureSize; }

  
  void RemoveTextureListHead(TextureSourceD3D9* aHost);

  




  TemporaryRef<IDirect3DTexture9> CreateTexture(const gfx::IntSize &aSize,
                                                _D3DFORMAT aFormat,
                                                D3DPOOL aPool,
                                                TextureSourceD3D9* aTextureHostIDirect3DTexture9);
#ifdef DEBUG
  
  
  bool IsInTextureHostList(TextureSourceD3D9* aFind);
#endif

  




  DeviceManagerState VerifyReadyForRendering();

  static uint32_t sMaskQuadRegister;

private:
  friend class SwapChainD3D9;

  ~DeviceManagerD3D9();
  void DestroyDevice();

  



  bool CreateVertexBuffer();

  


  void ReleaseTextureResources();
  


  void RegisterTextureHost(TextureSourceD3D9* aHost);

  
  nsTArray<SwapChainD3D9*> mSwapChains;

  
  nsRefPtr<IDirect3DDevice9> mDevice;

  
  nsRefPtr<IDirect3DDevice9Ex> mDeviceEx;

  
  nsRefPtr<IDirect3D9> mD3D9;

  
  nsRefPtr<IDirect3D9Ex> mD3D9Ex;

  
  nsRefPtr<IDirect3DVertexShader9> mLayerVS;

  
  nsRefPtr<IDirect3DPixelShader9> mRGBPS;

  
  nsRefPtr<IDirect3DPixelShader9> mRGBAPS;

  
  nsRefPtr<IDirect3DPixelShader9> mComponentPass1PS;

  
  nsRefPtr<IDirect3DPixelShader9> mComponentPass2PS;

  
  nsRefPtr<IDirect3DPixelShader9> mYCbCrPS;

  
  nsRefPtr<IDirect3DPixelShader9> mSolidColorPS;

  
  nsRefPtr<IDirect3DVertexShader9> mLayerVSMask;
  nsRefPtr<IDirect3DVertexShader9> mLayerVSMask3D;
  nsRefPtr<IDirect3DPixelShader9> mRGBPSMask;
  nsRefPtr<IDirect3DPixelShader9> mRGBAPSMask;
  nsRefPtr<IDirect3DPixelShader9> mRGBAPSMask3D;
  nsRefPtr<IDirect3DPixelShader9> mComponentPass1PSMask;
  nsRefPtr<IDirect3DPixelShader9> mComponentPass2PSMask;
  nsRefPtr<IDirect3DPixelShader9> mYCbCrPSMask;
  nsRefPtr<IDirect3DPixelShader9> mSolidColorPSMask;

  
  nsRefPtr<IDirect3DVertexBuffer9> mVB;

  
  nsRefPtr<IDirect3DVertexDeclaration9> mVD;

  





  TextureSourceD3D9* mTextureHostList;

  


  HWND mFocusWnd;

  
  HMONITOR mDeviceMonitor;

  uint32_t mDeviceResetCount;

  uint32_t mMaxTextureSize;

  



  D3DTEXTUREADDRESS mTextureAddressingMode;

  
  bool mHasDynamicTextures;

  
  bool mDeviceWasRemoved;

   
  nsAutoPtr<Nv3DVUtils> mNv3DVUtils; 

  


  bool VerifyCaps();
};

} 
} 

#endif 
