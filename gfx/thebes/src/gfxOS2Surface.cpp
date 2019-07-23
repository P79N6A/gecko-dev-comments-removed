




































#include "gfxOS2Surface.h"

#include <stdio.h>





gfxOS2Surface::gfxOS2Surface(HPS aPS, const gfxIntSize& aSize)
    : mOwnsPS(PR_FALSE), mPS(aPS), mSize(aSize)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface::gfxOS2Surface(HPS, ...)\n");
#endif

    cairo_surface_t *surf = cairo_os2_surface_create(mPS, mSize.width, mSize.height);
#ifdef DEBUG_thebes_2
    printf("  type(%#x)=%d (own=%d, h/w=%d/%d)\n", (unsigned int)surf,
           cairo_surface_get_type(surf), mOwnsPS, mSize.width, mSize.height);
#endif
    
    
    Init(surf);
}

gfxOS2Surface::gfxOS2Surface(HWND aWnd)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface::gfxOS2Surface(HWND)\n");
#endif

    if (!mPS) {
        
        mOwnsPS = PR_TRUE;
        mPS = WinGetPS(aWnd);
    } else {
        mOwnsPS = PR_FALSE;
    }

    RECTL rectl;
    WinQueryWindowRect(aWnd, &rectl);
    mSize.width = rectl.xRight - rectl.xLeft;
    mSize.height = rectl.yTop - rectl.yBottom;
    if (mSize.width == 0) mSize.width = 10;   
    if (mSize.height == 0) mSize.height = 10; 
    cairo_surface_t *surf = cairo_os2_surface_create(mPS, mSize.width, mSize.height);
#ifdef DEBUG_thebes_2
    printf("  type(%#x)=%d (own=%d, h/w=%d/%d)\n", (unsigned int)surf,
           cairo_surface_get_type(surf), mOwnsPS, mSize.width, mSize.height);
#endif
    cairo_os2_surface_set_hwnd(surf, aWnd); 
    
    
    Init(surf);
}

gfxOS2Surface::~gfxOS2Surface()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Surface::~gfxOS2Surface()\n");
#endif

    if (mOwnsPS)
        WinReleasePS(mPS);
}
