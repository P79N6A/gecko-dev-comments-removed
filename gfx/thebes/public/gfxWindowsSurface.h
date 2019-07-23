




































#ifndef GFX_WINDOWSSURFACE_H
#define GFX_WINDOWSSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include <windows.h>

class THEBES_API gfxWindowsSurface : public gfxASurface {
public:
    gfxWindowsSurface(HWND wnd);
    gfxWindowsSurface(HDC dc, PRBool deleteDC = PR_FALSE);

    
    gfxWindowsSurface(const gfxIntSize& size,
                      gfxImageFormat imageFormat = ImageFormatRGB24);

    
    gfxWindowsSurface(HDC dc,
                      const gfxIntSize& size,
                      gfxImageFormat imageFormat = ImageFormatRGB24);

    gfxWindowsSurface(cairo_surface_t *csurf);

    virtual ~gfxWindowsSurface();

    HDC GetDC() { return mDC; }

    already_AddRefed<gfxImageSurface> GetImageSurface();

    already_AddRefed<gfxWindowsSurface> OptimizeToDDB(HDC dc,
                                                      const gfxIntSize& size,
                                                      gfxImageFormat format);

    nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    nsresult EndPrinting();
    nsresult AbortPrinting();
    nsresult BeginPage();
    nsresult EndPage();

private:
    PRBool mOwnsDC;
    HDC mDC;
    HWND mWnd;
};

#endif 
