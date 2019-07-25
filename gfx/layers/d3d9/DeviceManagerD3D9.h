




#ifndef GFX_DEVICEMANAGERD3D9_H
#define GFX_DEVICEMANAGERD3D9_H

#include "gfxTypes.h"
#include "nsRect.h"
#include "nsAutoPtr.h"
#include "d3d9.h"
#include "nsTArray.h"

namespace mozilla {
namespace layers {

class DeviceManagerD3D9;
class LayerD3D9;
class Nv3DVUtils;
class Layer;


const int CBmLayerTransform = 0;
const int CBmProjection = 4;
const int CBvRenderTargetOffset = 8;
const int CBvTextureCoords = 9;
const int CBvLayerQuad = 10;
const int CBfLayerOpacity = 0;





class THEBES_API SwapChainD3D9
{
  NS_INLINE_DECL_REFCOUNTING(SwapChainD3D9)
public:
  ~SwapChainD3D9();

  









  bool PrepareForRendering();

  



  void Present(const nsIntRect &aRect);

private:
  friend class DeviceManagerD3D9;

  SwapChainD3D9(DeviceManagerD3D9 *aDeviceManager);
  
  bool Init(HWND hWnd);

  



  void Reset();

  nsRefPtr<IDirect3DSwapChain9> mSwapChain;
  nsRefPtr<DeviceManagerD3D9> mDeviceManager;
  HWND mWnd;
};






class THEBES_API DeviceManagerD3D9
{
public:
  DeviceManagerD3D9();
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);
protected:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

public:
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

  void SetShaderMode(ShaderMode aMode, Layer* aMask, bool aIs2D);

  

 
  Nv3DVUtils *GetNv3DVUtils()  { return mNv3DVUtils; }

  


  bool DeviceWasRemoved() { return mDeviceWasRemoved; }

  uint32_t GetDeviceResetCount() { return mDeviceResetCount; }

  



  nsTArray<LayerD3D9*> mLayersWithResources;

  int32_t GetMaxTextureSize() { return mMaxTextureSize; }

private:
  friend class SwapChainD3D9;

  ~DeviceManagerD3D9();

  




  bool VerifyReadyForRendering();

  



  bool CreateVertexBuffer();

  
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

  


  HWND mFocusWnd;

  
  HMONITOR mDeviceMonitor;

  uint32_t mDeviceResetCount;

  uint32_t mMaxTextureSize;

  
  bool mHasDynamicTextures;

  
  bool mDeviceWasRemoved;

   
  nsAutoPtr<Nv3DVUtils> mNv3DVUtils; 

  


  bool VerifyCaps();
};

} 
} 

#endif 
