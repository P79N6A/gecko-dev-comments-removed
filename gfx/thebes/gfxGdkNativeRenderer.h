




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
    








#if defined(MOZ_WIDGET_GTK2)
    virtual nsresult DrawWithGDK(GdkDrawable * drawable, gint offsetX, 
            gint offsetY, GdkRectangle * clipRects, PRUint32 numClipRects) = 0;
#else
    virtual nsresult DrawWithGDK(cairo_t * cr, gint offsetX, 
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

    





#if defined(MOZ_WIDGET_GTK2)
    void Draw(gfxContext* ctx, nsIntSize size,
              PRUint32 flags, GdkColormap* colormap);
#else
    void Draw(gfxContext* ctx, nsIntSize size,
              PRUint32 flags, GdkVisual *visual);
#endif

private:
#ifdef MOZ_X11
#if defined(MOZ_WIDGET_GTK2)
    
    virtual nsresult DrawWithXlib(gfxXlibSurface* surface,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, PRUint32 numClipRects);
    GdkColormap *mColormap;
#else
    
    virtual nsresult DrawWithXlib(cairo_t *cr,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, PRUint32 numClipRects);
    GdkVisual *mVisual;
#endif
#endif
};

#endif 
