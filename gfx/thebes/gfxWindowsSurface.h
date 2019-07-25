




































#ifndef GFX_WINDOWSSURFACE_H
#define GFX_WINDOWSSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include <windows.h>

class gfxContext;

class THEBES_API gfxWindowsSurface : public gfxASurface {
public:
    enum {
        FLAG_TAKE_DC = (1 << 0),
        FLAG_FOR_PRINTING = (1 << 1),
        FLAG_IS_TRANSPARENT = (1 << 2)
    };

    gfxWindowsSurface(HWND wnd, PRUint32 flags = 0);
    gfxWindowsSurface(HDC dc, PRUint32 flags = 0);

    
    gfxWindowsSurface(const gfxIntSize& size,
                      gfxImageFormat imageFormat = ImageFormatRGB24);

    
    gfxWindowsSurface(HDC dc,
                      const gfxIntSize& size,
                      gfxImageFormat imageFormat = ImageFormatRGB24);

    gfxWindowsSurface(cairo_surface_t *csurf);

    virtual already_AddRefed<gfxASurface> CreateSimilarSurface(gfxContentType aType,
                                                               const gfxIntSize& aSize);

    void InitWithDC(PRUint32 flags);

    virtual ~gfxWindowsSurface();

    HDC GetDC() { return mDC; }

    HDC GetDCWithClip(gfxContext *);

    already_AddRefed<gfxImageSurface> GetAsImageSurface();

    already_AddRefed<gfxWindowsSurface> OptimizeToDDB(HDC dc,
                                                      const gfxIntSize& size,
                                                      gfxImageFormat format);

    nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    nsresult EndPrinting();
    nsresult AbortPrinting();
    nsresult BeginPage();
    nsresult EndPage();

    virtual PRInt32 GetDefaultContextFlags() const;

    void MovePixels(const nsIntRect& aSourceRect,
                    const nsIntPoint& aDestTopLeft)
    {
        FastMovePixels(aSourceRect, aDestTopLeft);
    }

private:
    PRPackedBool mOwnsDC;
    PRPackedBool mForPrinting;

    HDC mDC;
    HWND mWnd;
};

#endif
