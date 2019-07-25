




































#ifndef GFXXLIBNATIVERENDER_H_
#define GFXXLIBNATIVERENDER_H_

#include "gfxColor.h"
#include "nsAutoPtr.h"
#include "nsRect.h"
#include <X11/Xlib.h>

class gfxASurface;
class gfxXlibSurface;
class gfxContext;







class THEBES_API gfxXlibNativeRenderer {
public:
    













    virtual nsresult DrawWithXlib(gfxXlibSurface* surface,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, PRUint32 numClipRects) = 0;
  
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

    bool DrawOntoTempSurface(gfxXlibSurface *tempXlibSurface,
                               nsIntPoint offset);

};

#endif 
