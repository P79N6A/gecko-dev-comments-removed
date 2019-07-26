




#ifndef GFX_WINDOWSSURFACE_H
#define GFX_WINDOWSSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"


#include <windows.h>

struct IDirect3DSurface9;


#undef LoadImage

class gfxContext;

class gfxWindowsSurface : public gfxASurface {
public:
    enum {
        FLAG_TAKE_DC = (1 << 0),
        FLAG_FOR_PRINTING = (1 << 1),
        FLAG_IS_TRANSPARENT = (1 << 2)
    };

    gfxWindowsSurface(HWND wnd, uint32_t flags = 0);
    gfxWindowsSurface(HDC dc, uint32_t flags = 0);

    
    gfxWindowsSurface(IDirect3DSurface9 *surface, uint32_t flags = 0);

    
    gfxWindowsSurface(const gfxIntSize& size,
                      gfxImageFormat imageFormat = gfxImageFormatRGB24);

    
    gfxWindowsSurface(HDC dc,
                      const gfxIntSize& size,
                      gfxImageFormat imageFormat = gfxImageFormatRGB24);

    gfxWindowsSurface(cairo_surface_t *csurf);

    virtual already_AddRefed<gfxASurface> CreateSimilarSurface(gfxContentType aType,
                                                               const gfxIntSize& aSize);

    void InitWithDC(uint32_t flags);

    virtual ~gfxWindowsSurface();

    HDC GetDC();

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

    virtual int32_t GetDefaultContextFlags() const;

    const gfxIntSize GetSize() const;

    void MovePixels(const nsIntRect& aSourceRect,
                    const nsIntPoint& aDestTopLeft)
    {
        FastMovePixels(aSourceRect, aDestTopLeft);
    }

    
    
    virtual gfxMemoryLocation GetMemoryLocation() const;

private:
    void MakeInvalid(gfxIntSize& size);

    bool mOwnsDC;
    bool mForPrinting;

    HDC mDC;
    HWND mWnd;
};

#endif
