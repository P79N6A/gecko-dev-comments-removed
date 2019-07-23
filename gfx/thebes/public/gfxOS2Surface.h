




































#ifndef GFX_OS2_SURFACE_H
#define GFX_OS2_SURFACE_H

#include "gfxASurface.h"

#define INCL_GPIBITMAPS
#include <os2.h>
#include <cairo-os2.h>

class THEBES_API gfxOS2Surface : public gfxASurface {

public:
    
    gfxOS2Surface(HPS aPS, const gfxIntSize& aSize);
    
    gfxOS2Surface(const gfxIntSize& aSize,
                  gfxASurface::gfxImageFormat aImageFormat);
    
    gfxOS2Surface(HWND aWnd);
    virtual ~gfxOS2Surface();

    HPS GetPS() { return mPS; }
    gfxIntSize GetSize() { return mSize; }

private:
    PRBool mOwnsPS;
    PRBool mHasWnd; 
    HDC mDC; 
    HPS mPS; 
    HBITMAP mBitmap; 
    gfxIntSize mSize; 
};

#endif 
