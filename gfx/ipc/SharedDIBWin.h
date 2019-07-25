





































#ifndef gfx_SharedDIBWin_h__
#define gfx_SharedDIBWin_h__

#include <windows.h>

#include "SharedDIB.h"

namespace mozilla {
namespace gfx {

class SharedDIBWin : public SharedDIB
{
public:
  SharedDIBWin();
  ~SharedDIBWin();

  
  
  nsresult Create(HDC aHdc, PRUint32 aWidth, PRUint32 aHeight,
                  bool aTransparent);

  
  
  
  nsresult Attach(Handle aHandle, PRUint32 aWidth, PRUint32 aHeight,
                  bool aTransparent);

  
  nsresult Close();

  
  HDC GetHDC() { return mSharedHdc; }

  
  void* GetBits() { return mBitmapBits; }

private:
  HDC                 mSharedHdc;
  HBITMAP             mSharedBmp;
  HGDIOBJ             mOldObj;
  void*               mBitmapBits;

  PRUint32 SetupBitmapHeader(PRUint32 aWidth, PRUint32 aHeight,
                             bool aTransparent, BITMAPV4HEADER *aHeader);
  nsresult SetupSurface(HDC aHdc, BITMAPV4HEADER *aHdr);
};

} 
} 

#endif
