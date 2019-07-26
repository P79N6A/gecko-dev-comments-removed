




#pragma once



#include <d3d10_1.h>
#include <dxgi.h>
#include <d3d10misc.h>








static bool D3DFeatureLevelCheck()
{
  HMODULE dxgiModule = LoadLibraryA("dxgi.dll");
  if (!dxgiModule) {
    return false;
  }
  decltype(CreateDXGIFactory1)* createDXGIFactory1 =
    (decltype(CreateDXGIFactory1)*) GetProcAddress(dxgiModule, "CreateDXGIFactory1");
  if (!createDXGIFactory1) {
    FreeLibrary(dxgiModule);
    return false;
  }

  HMODULE d3d10module = LoadLibraryA("d3d10_1.dll");
  if (!d3d10module) {
    FreeLibrary(dxgiModule);
    return false;
  }
  decltype(D3D10CreateDevice1)* createD3DDevice =
    (decltype(D3D10CreateDevice1)*) GetProcAddress(d3d10module,
                                                   "D3D10CreateDevice1");
  if (!createD3DDevice) {
    FreeLibrary(d3d10module);
    FreeLibrary(dxgiModule);
    return false;
  }

  IDXGIFactory1* factory1;
  if (FAILED(createDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory1))) {
    FreeLibrary(d3d10module);
    FreeLibrary(dxgiModule);
    return false;
  }

  IDXGIAdapter1* adapter1;
  if (FAILED(factory1->EnumAdapters1(0, &adapter1))) {
    factory1->Release();
    FreeLibrary(d3d10module);
    FreeLibrary(dxgiModule);
    return false;
  }

  
  ID3D10Device1* device;
  if (FAILED(createD3DDevice(adapter1, D3D10_DRIVER_TYPE_HARDWARE, nullptr,
                             D3D10_CREATE_DEVICE_BGRA_SUPPORT |
                             D3D10_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                             D3D10_FEATURE_LEVEL_10_1,
                             D3D10_1_SDK_VERSION, &device))) {
    
    if (FAILED(createD3DDevice(adapter1, D3D10_DRIVER_TYPE_HARDWARE, nullptr,
                               D3D10_CREATE_DEVICE_BGRA_SUPPORT |
                               D3D10_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                               D3D10_FEATURE_LEVEL_10_0,
                               D3D10_1_SDK_VERSION, &device))) {
      
      if (FAILED(createD3DDevice(adapter1, D3D10_DRIVER_TYPE_HARDWARE, nullptr,
                                 D3D10_CREATE_DEVICE_BGRA_SUPPORT |
                                 D3D10_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                                 D3D10_FEATURE_LEVEL_9_3,
                                 D3D10_1_SDK_VERSION, &device))) {
        adapter1->Release();
        factory1->Release();
        FreeLibrary(d3d10module);
        FreeLibrary(dxgiModule);
        return false;
      }
    }
  }
  device->Release();
  adapter1->Release();
  factory1->Release();
  FreeLibrary(d3d10module);
  FreeLibrary(dxgiModule);
  return true;
}
