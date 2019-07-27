




#ifndef GFX_D2DSURFACE_H
#define GFX_D2DSURFACE_H

#include "gfxASurface.h"
#include "nsPoint.h"

#include <windows.h>

struct ID3D10Texture2D;

class gfxD2DSurface : public gfxASurface {
public:

    gfxD2DSurface(HWND wnd,
                  gfxContentType aContent);

    gfxD2DSurface(const gfxIntSize& size,
                  gfxImageFormat imageFormat = gfxImageFormat::RGB24);

    gfxD2DSurface(HANDLE handle, gfxContentType aContent);

    gfxD2DSurface(ID3D10Texture2D *texture, gfxContentType aContent);

    gfxD2DSurface(cairo_surface_t *csurf);

    virtual ~gfxD2DSurface();

    void Present();
    void Scroll(const nsIntPoint &aDelta, const mozilla::gfx::IntRect &aClip);

    virtual const mozilla::gfx::IntSize GetSize() const;

    ID3D10Texture2D *GetTexture();

    HDC GetDC(bool aRetainContents);
    void ReleaseDC(const mozilla::gfx::IntRect *aUpdatedRect);
};

#endif 
