




































#ifndef GFX_OS2_SURFACE_H
#define GFX_OS2_SURFACE_H

#include "gfxASurface.h"

#define INCL_GPIBITMAPS
#include <os2.h>
#include <cairo-os2.h>

class THEBES_API gfxOS2Surface : public gfxASurface {

public:
    
    gfxOS2Surface(const gfxIntSize& aSize,
                  gfxASurface::gfxImageFormat aImageFormat);
    
    gfxOS2Surface(HWND aWnd);
    
    gfxOS2Surface(HDC aDC, const gfxIntSize& aSize);
    virtual ~gfxOS2Surface();

    

    
    
    
    void Refresh(RECTL *aRect, HPS aPS);

    
    int Resize(const gfxIntSize& aSize);

    HPS GetPS();
    gfxIntSize GetSize() { return mSize; }

private:
    HWND mWnd; 
    HDC mDC; 
    HPS mPS; 
    HBITMAP mBitmap; 
    gfxIntSize mSize; 
};

#endif 
