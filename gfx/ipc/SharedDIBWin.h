




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

  
  
  nsresult Create(HDC aHdc, uint32_t aWidth, uint32_t aHeight,
                  bool aTransparent);

  
  
  
  nsresult Attach(Handle aHandle, uint32_t aWidth, uint32_t aHeight,
                  bool aTransparent);

  
  nsresult Close();

  
  HDC GetHDC() { return mSharedHdc; }

  
  void* GetBits() { return mBitmapBits; }

private:
  HDC                 mSharedHdc;
  HBITMAP             mSharedBmp;
  HGDIOBJ             mOldObj;
  void*               mBitmapBits;

  uint32_t SetupBitmapHeader(uint32_t aWidth, uint32_t aHeight,
                             bool aTransparent, BITMAPV4HEADER *aHeader);
  nsresult SetupSurface(HDC aHdc, BITMAPV4HEADER *aHdr);
};

} 
} 

#endif
