


































#ifndef GFX_D2DSURFACE_H
#define GFX_D2DSURFACE_H

#include "gfxASurface.h"
#include "nsPoint.h"
#include "nsRect.h"

#include <windows.h>

struct ID3D10Texture2D;

class THEBES_API gfxD2DSurface : public gfxASurface {
public:

    gfxD2DSurface(HWND wnd,
                  gfxContentType aContent);

    gfxD2DSurface(const gfxIntSize& size,
                  gfxImageFormat imageFormat = ImageFormatRGB24);

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

    ID3D10Texture2D *GetTexture();

    HDC GetDC(PRBool aRetainContents);
    void ReleaseDC(const nsIntRect *aUpdatedRect);
};

#endif 
