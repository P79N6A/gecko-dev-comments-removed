




































#ifndef GFX_DDRAWSURFACE_H
#define GFX_DDRAWSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include <windows.h>
#include <ddraw.h>

class THEBES_API gfxDDrawSurface : public gfxASurface {
public:
    gfxDDrawSurface(LPDIRECTDRAW lpdd, const gfxIntSize& size,
                    gfxImageFormat imageFormat = ImageFormatRGB24);

    gfxDDrawSurface(gfxDDrawSurface * psurf, const RECT & rect);

    gfxDDrawSurface(cairo_surface_t *csurf);

    LPDIRECTDRAWSURFACE GetDDSurface();

    virtual ~gfxDDrawSurface();

    nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    nsresult EndPrinting();
    nsresult AbortPrinting();
    nsresult BeginPage();
    nsresult EndPage();

    virtual PRInt32 GetDefaultContextFlags() const;
};

#endif 
