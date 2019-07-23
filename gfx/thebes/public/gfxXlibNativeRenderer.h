




































#ifndef GFXXLIBNATIVERENDER_H_
#define GFXXLIBNATIVERENDER_H_

#include "gfxColor.h"
#include <X11/Xlib.h>

class gfxASurface;
class gfxContext;







class THEBES_API gfxXlibNativeRenderer {
public:
    







    virtual nsresult NativeDraw(Display* dpy, Drawable drawable, Visual* visual,
                                short offsetX, short offsetY,
                                XRectangle* clipRects, PRUint32 numClipRects) = 0;
  
    enum {
        
        
        
        
        DRAW_IS_OPAQUE = 0x01,
        
        
        DRAW_SUPPORTS_OFFSET = 0x02,
        
        DRAW_SUPPORTS_CLIP_RECT = 0x04,
        
        
        DRAW_SUPPORTS_CLIP_LIST = 0x08,
        
        
        DRAW_SUPPORTS_NONDEFAULT_VISUAL = 0x08,
        
        
        DRAW_SUPPORTS_ALTERNATE_DISPLAY = 0x10
    };

    struct DrawOutput {
        nsRefPtr<gfxASurface> mSurface;
        PRPackedBool mUniformAlpha;
        PRPackedBool mUniformColor;
        gfxRGBA      mColor;
    };

    









    nsresult Draw(Display* dpy, gfxContext* ctx, int width, int height,
                  PRUint32 flags, DrawOutput* output);
};

#endif 
