


































#ifndef GFX_D2DSURFACE_H
#define GFX_D2DSURFACE_H

#include "gfxASurface.h"
#include "nsPoint.h"
#include "nsRect.h"

#include <windows.h>

class THEBES_API gfxD2DSurface : public gfxASurface {
public:

    gfxD2DSurface(HWND wnd,
                  gfxContentType aContent);

    gfxD2DSurface(const gfxIntSize& size,
                  gfxImageFormat imageFormat = ImageFormatRGB24);

    gfxD2DSurface(HANDLE handle, gfxContentType aContent);

    gfxD2DSurface(cairo_surface_t *csurf);

    virtual ~gfxD2DSurface();


    void Present();
    void Scroll(const nsIntPoint &aDelta, const nsIntRect &aClip);

    HDC GetDC(PRBool aRetainContents);
    void ReleaseDC(const nsIntRect *aUpdatedRect);
};

#endif 
