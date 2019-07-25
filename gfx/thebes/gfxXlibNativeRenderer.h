




#ifndef GFXXLIBNATIVERENDER_H_
#define GFXXLIBNATIVERENDER_H_

#include "gfxColor.h"
#include "nsAutoPtr.h"
#include "nsRect.h"
#include <X11/Xlib.h>
#if MOZ_WIDGET_GTK == 3
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"
#endif

class gfxASurface;
class gfxXlibSurface;
class gfxContext;







class THEBES_API gfxXlibNativeRenderer {
public:
    














#if defined(MOZ_WIDGET_GTK2)
    virtual nsresult DrawWithXlib(gfxXlibSurface* surface,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, PRUint32 numClipRects) = 0;
#else
    virtual nsresult DrawWithXlib(cairo_t *cr,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, PRUint32 numClipRects) = 0;
#endif  
 

 
    enum {
        
        
        
        
        DRAW_IS_OPAQUE = 0x01,
        
        DRAW_SUPPORTS_CLIP_RECT = 0x04,
        
        
        DRAW_SUPPORTS_CLIP_LIST = 0x08,
        
        
        
        DRAW_SUPPORTS_ALTERNATE_VISUAL = 0x10,
        
        
        
        DRAW_SUPPORTS_ALTERNATE_SCREEN = 0x20
    };

    struct DrawOutput {
        nsRefPtr<gfxASurface> mSurface;
        bool mUniformAlpha;
        bool mUniformColor;
        gfxRGBA      mColor;
    };

    











    void Draw(gfxContext* ctx, nsIntSize size,
              PRUint32 flags, Screen *screen, Visual *visual,
              DrawOutput* result);

private:
    bool DrawDirect(gfxContext *ctx, nsIntSize bounds,
                      PRUint32 flags, Screen *screen, Visual *visual);

#if defined(MOZ_WIDGET_GTK2)
    bool DrawOntoTempSurface(gfxXlibSurface *tempXlibSurface,
                               nsIntPoint offset);
#else
    PRBool DrawOntoTempSurface(cairo_t *cr,
                               nsIntPoint offset);
#endif
};

#endif 
