




#ifndef GFXXLIBNATIVERENDER_H_
#define GFXXLIBNATIVERENDER_H_

#include "nsPoint.h"
#include "nsRect.h"
#include <X11/Xlib.h>

namespace mozilla {
namespace gfx {
  class DrawTarget;
}
}

class gfxASurface;
class gfxContext;
struct nsIntRect;
struct nsIntPoint;
struct nsIntSize;
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;







class gfxXlibNativeRenderer {
public:
    












    virtual nsresult DrawWithXlib(cairo_surface_t* surface,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, uint32_t numClipRects) = 0;
  
    enum {
        
        
        
        
        DRAW_IS_OPAQUE = 0x01,
        
        DRAW_SUPPORTS_CLIP_RECT = 0x04,
        
        
        DRAW_SUPPORTS_CLIP_LIST = 0x08,
        
        
        
        DRAW_SUPPORTS_ALTERNATE_VISUAL = 0x10,
        
        
        
        DRAW_SUPPORTS_ALTERNATE_SCREEN = 0x20
    };

    











    void Draw(gfxContext* ctx, nsIntSize size,
              uint32_t flags, Screen *screen, Visual *visual);

private:
    bool DrawDirect(gfxContext *ctx, nsIntSize bounds,
                    uint32_t flags, Screen *screen, Visual *visual);

    bool DrawCairo(cairo_t* cr, nsIntSize size,
                   uint32_t flags, Screen *screen, Visual *visual);

    void DrawFallback(mozilla::gfx::DrawTarget* dt, gfxContext* ctx,
                      gfxASurface* aSurface, nsIntSize& size,
                      nsIntRect& drawingRect, bool canDrawOverBackground,
                      uint32_t flags, Screen* screen, Visual* visual);

    bool DrawOntoTempSurface(cairo_surface_t *tempXlibSurface,
                             nsIntPoint offset);

};

#endif 
