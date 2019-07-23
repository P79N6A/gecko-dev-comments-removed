




































#include "gfxOS2Surface.h"

#include <stdio.h>





gfxOS2Surface::gfxOS2Surface(const gfxIntSize& aSize,
                             gfxASurface::gfxImageFormat aImageFormat)
    : mHasWnd(PR_FALSE), mSize(aSize)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface[%#x]::gfxOS2Surface(Size=%dx%d, %d)\n", (unsigned int)this,
           aSize.width, aSize.height, aImageFormat);
#endif
    
    

    
    DEVOPENSTRUC dop = { 0, 0, 0, 0, 0 };
    SIZEL sizel = { 0, 0 }; 
    mDC = DevOpenDC(0, OD_MEMORY, (PSZ)"*", 5, (PDEVOPENDATA)&dop, NULLHANDLE);
    NS_ASSERTION(mDC != DEV_ERROR, "Could not create memory DC");

    mPS = GpiCreatePS(0, mDC, &sizel, PU_PELS | GPIT_MICRO | GPIA_ASSOC);
    NS_ASSERTION(mPS != GPI_ERROR, "Could not create PS on memory DC!");

    
    BITMAPINFOHEADER2 hdr = { 0 };
    hdr.cbFix = sizeof(BITMAPINFOHEADER2);
    hdr.cx = mSize.width;
    hdr.cy = mSize.height;
    hdr.cPlanes = 1;

    
    LONG lBitCount = 0;
    DevQueryCaps(mDC, CAPS_COLOR_BITCOUNT, 1, &lBitCount);
    hdr.cBitCount = (USHORT)lBitCount;

    mBitmap = GpiCreateBitmap(mPS, &hdr, 0, 0, 0);
    NS_ASSERTION(mBitmap != GPI_ERROR, "Could not create bitmap in memory!");
    
    GpiSetBitmap(mPS, mBitmap);

    
    cairo_surface_t *surf = cairo_os2_surface_create(mPS, mSize.width, mSize.height);
#ifdef DEBUG_thebes_2
    printf("  type(%#x)=%d (ID=%#x, h/w=%d/%d)\n", (unsigned int)surf,
           cairo_surface_get_type(surf), (unsigned int)mPS, mSize.width, mSize.height);
#endif
    
    
    

    
    cairo_os2_surface_set_manual_window_refresh(surf, 1);

    Init(surf);
}

gfxOS2Surface::gfxOS2Surface(HWND aWnd)
    : mHasWnd(PR_TRUE), mDC(nsnull), mBitmap(nsnull)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface[%#x]::gfxOS2Surface(HWND=%#x)\n", (unsigned int)this,
           (unsigned int)aWnd);
#endif

    mPS = WinGetPS(aWnd);

    RECTL rectl;
    WinQueryWindowRect(aWnd, &rectl);
    mSize.width = rectl.xRight - rectl.xLeft;
    mSize.height = rectl.yTop - rectl.yBottom;
    if (mSize.width == 0) mSize.width = 1;   
    if (mSize.height == 0) mSize.height = 1; 
    cairo_surface_t *surf = cairo_os2_surface_create(mPS, mSize.width, mSize.height);
#ifdef DEBUG_thebes_2
    printf("  type(%#x)=%d (ID=%#x, h/w=%d/%d)\n", (unsigned int)surf,
           cairo_surface_get_type(surf), (unsigned int)mPS, mSize.width, mSize.height);
#endif
    
    
    

    
    cairo_os2_surface_set_hwnd(surf, aWnd);
    
    cairo_os2_surface_set_manual_window_refresh(surf, 1);

    Init(surf);
}

gfxOS2Surface::~gfxOS2Surface()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface[%#x]::~gfxOS2Surface()\n", (unsigned int)this);
#endif

    
    
    
    
    if (mHasWnd) {
        if (mPS) {
            WinReleasePS(mPS);
        }
    } else {
        if (mBitmap) {
            GpiSetBitmap(mPS, NULL);
            GpiDeleteBitmap(mBitmap);
        }
        if (mPS) {
            GpiDestroyPS(mPS);
        }
        if (mDC) {
            DevCloseDC(mDC);
        }
    }
}

void gfxOS2Surface::Refresh(RECTL *aRect, HPS aPS)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface[%#x]::Refresh(x=%ld,%ld/y=%ld,%ld, HPS=%#x), mPS=%#x\n",
           (unsigned int)this,
           aRect->xLeft, aRect->xRight, aRect->yBottom, aRect->yTop,
           (unsigned int)aPS, (unsigned int)mPS);
#endif
    cairo_os2_surface_refresh_window(CairoSurface(), aPS, aRect);
}

int gfxOS2Surface::Resize(const gfxIntSize& aSize)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface[%#x]::Resize(%dx%d)\n", (unsigned int)this,
           aSize.width, aSize.height);
#endif
    mSize = aSize; 
    
    return cairo_os2_surface_set_size(CairoSurface(), mSize.width, mSize.height, 50);
}
