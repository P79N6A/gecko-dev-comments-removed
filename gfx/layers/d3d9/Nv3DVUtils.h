




































#ifndef GFX_NV3DVUTILS_H
#define GFX_NV3DVUTILS_H

#include "Layers.h"
#include <windows.h>
#include <d3d9.h>

namespace mozilla {
namespace layers {

#define FIREFOX_3DV_APP_HANDLE    0xECB992B6

enum Nv_Stereo_Mode {
  NV_STEREO_MODE_LEFT_RIGHT = 0,
  NV_STEREO_MODE_RIGHT_LEFT = 1,
  NV_STEREO_MODE_TOP_BOTTOM = 2,
  NV_STEREO_MODE_BOTTOM_TOP = 3,
  NV_STEREO_MODE_LAST       = 4 
};

class INv3DVStreaming : public IUnknown {

public:
  virtual bool Nv3DVInitialize()                  = 0;
  virtual bool Nv3DVRelease()                     = 0;
  virtual bool Nv3DVSetDevice(IUnknown* pDevice)  = 0;
  virtual bool Nv3DVControl(Nv_Stereo_Mode eStereoMode, bool bEnableStereo, DWORD dw3DVAppHandle) = 0;
  virtual bool Nv3DVMetaData(DWORD dwWidth, DWORD dwHeight, HANDLE hSrcLuma, HANDLE hDst) = 0;
};




class Nv3DVUtils {

public:
  Nv3DVUtils();
  ~Nv3DVUtils();

  


  void Initialize();

  



  void UnInitialize();

  



  void SetDeviceInfo(IUnknown *devUnknown);

  



  void SendNv3DVControl(Nv_Stereo_Mode eStereoMode, bool bEnableStereo, DWORD dw3DVAppHandle);

  



  void SendNv3DVMetaData(unsigned int dwWidth, unsigned int dwHeight, HANDLE hSrcLuma, HANDLE hDst);

private:

  
  nsRefPtr<INv3DVStreaming> m3DVStreaming;

};


} 
} 

#endif 
