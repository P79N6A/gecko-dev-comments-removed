




#ifndef GFXGDKNATIVERENDER_H_
#define GFXGDKNATIVERENDER_H_

#include <gdk/gdk.h>
#include "nsSize.h"
#ifdef MOZ_X11
#include "gfxXlibNativeRenderer.h"
#endif

class gfxASurface;
class gfxContext;







class THEBES_API gfxGdkNativeRenderer
#ifdef MOZ_X11
    : private gfxXlibNativeRenderer
#endif
{
public:
    








#if (MOZ_WIDGET_GTK == 2)
    virtual nsresult DrawWithGDK(GdkDrawable * drawable, gint offsetX, 
            gint offsetY, GdkRectangle * clipRects, PRUint32 numClipRects) = 0;
#endif

    enum {
        
        
        
        
        DRAW_IS_OPAQUE =
#ifdef MOZ_X11
            gfxXlibNativeRenderer::DRAW_IS_OPAQUE
#else
            0x1
#endif
        
        
        , DRAW_SUPPORTS_CLIP_RECT =
#ifdef MOZ_X11
            gfxXlibNativeRenderer::DRAW_SUPPORTS_CLIP_RECT
#else
            0x2
#endif
    };

    





#if (MOZ_WIDGET_GTK == 2)
    void Draw(gfxContext* ctx, nsIntSize size,
              PRUint32 flags, GdkColormap* colormap);
#endif

private:
#ifdef MOZ_X11
    
    virtual nsresult DrawWithXlib(gfxXlibSurface* surface,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, PRUint32 numClipRects);

#if (MOZ_WIDGET_GTK == 2)
    GdkColormap *mColormap;
#endif
#endif
};

#endif 
