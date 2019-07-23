





































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

  
  
  nsresult Create(HDC aHdc, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth);

  
  
  
  nsresult Attach(Handle aHandle, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth);

  
  nsresult Close();

  
  HDC GetHDC() { return mSharedHdc; }

private:
  HDC                 mSharedHdc;
  HBITMAP             mSharedBmp;
  HGDIOBJ             mOldObj;

  PRUint32 SetupBitmapHeader(PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth, BITMAPINFOHEADER *aHeader);
  nsresult SetupSurface(HDC aHdc, BITMAPINFOHEADER *aHdr);
};

} 
} 

#endif
