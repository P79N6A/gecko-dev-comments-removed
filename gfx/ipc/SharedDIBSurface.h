




#ifndef mozilla_gfx_SharedDIBSurface_h
#define mozilla_gfx_SharedDIBSurface_h

#include "gfxImageSurface.h"
#include "SharedDIBWin.h"

#include <windows.h>

namespace mozilla {
namespace gfx {




class SharedDIBSurface : public gfxImageSurface
{
public:
  typedef base::SharedMemoryHandle Handle;

  SharedDIBSurface() { }
  ~SharedDIBSurface() { }

  


  bool Create(HDC adc, uint32_t aWidth, uint32_t aHeight, bool aTransparent);

  


  bool Attach(Handle aHandle, uint32_t aWidth, uint32_t aHeight,
              bool aTransparent);

  



  void Flush() { ::GdiFlush(); }

  HDC GetHDC() { return mSharedDIB.GetHDC(); }

  nsresult ShareToProcess(base::ProcessId aTargetPid, Handle* aNewHandle) {
    return mSharedDIB.ShareToProcess(aTargetPid, aNewHandle);
  }

  static bool IsSharedDIBSurface(gfxASurface* aSurface);

private:
  SharedDIBWin mSharedDIB;

  void InitSurface(uint32_t aWidth, uint32_t aHeight, bool aTransparent);
};

} 
} 

#endif 
