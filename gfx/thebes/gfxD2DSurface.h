




#ifndef GFX_D2DSURFACE_H
#define GFX_D2DSURFACE_H

#include "gfxASurface.h"
#include "nsPoint.h"

#include <windows.h>

struct ID3D10Texture2D;
struct nsIntRect;

class gfxD2DSurface : public gfxASurface {
public:

    gfxD2DSurface(HWND wnd,
                  gfxContentType aContent);

    gfxD2DSurface(const gfxIntSize& size,
                  gfxImageFormat imageFormat = gfxImageFormatRGB24);

    gfxD2DSurface(HANDLE handle, gfxContentType aContent);

    gfxD2DSurface(ID3D10Texture2D *texture, gfxContentType aContent);

    gfxD2DSurface(cairo_surface_t *csurf);

    void MovePixels(const nsIntRect& aSourceRect,
                    const nsIntPoint& aDestTopLeft)
    {
        FastMovePixels(aSourceRect, aDestTopLeft);
    }

    virtual ~gfxD2DSurface();

    void Present();
    void Scroll(const nsIntPoint &aDelta, const nsIntRect &aClip);

    virtual const gfxIntSize GetSize() const;

    ID3D10Texture2D *GetTexture();

    HDC GetDC(bool aRetainContents);
    void ReleaseDC(const nsIntRect *aUpdatedRect);
};

#endif 
