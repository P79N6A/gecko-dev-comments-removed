




































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

  


  bool Create(HDC adc, PRUint32 aWidth, PRUint32 aHeight, bool aTransparent);

  


  bool Attach(Handle aHandle, PRUint32 aWidth, PRUint32 aHeight,
              bool aTransparent);

  



  void Flush() { ::GdiFlush(); }

  HDC GetHDC() { return mSharedDIB.GetHDC(); }

  nsresult ShareToProcess(base::ProcessHandle aChildProcess, Handle* aChildHandle) {
    return mSharedDIB.ShareToProcess(aChildProcess, aChildHandle);
  }

  static bool IsSharedDIBSurface(gfxASurface* aSurface);

private:
  SharedDIBWin mSharedDIB;

  void InitSurface(PRUint32 aWidth, PRUint32 aHeight, bool aTransparent);
};

} 
} 

#endif 
