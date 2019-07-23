




































#ifndef GFXQTNATIVERENDER_H_
#define GFXQTNATIVERENDER_H_

#include "gfxColor.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxXlibSurface.h"

class QRect;







class THEBES_API gfxQtNativeRenderer {
public:
    







    virtual nsresult NativeDraw(gfxXlibSurface *xsurf,
            Colormap colormap, short offsetX,
            short offsetY, QRect * clipRects, PRUint32 numClipRects) = 0;
  
    enum {
        
        
        
        
        DRAW_IS_OPAQUE = 0x01,
        
        
        DRAW_SUPPORTS_OFFSET = 0x02,
        
        DRAW_SUPPORTS_CLIP_RECT = 0x04,
        
        
        DRAW_SUPPORTS_CLIP_LIST = 0x08,
        
        
        DRAW_SUPPORTS_NONDEFAULT_VISUAL = 0x10,
        
        
        
        DRAW_SUPPORTS_ALTERNATE_SCREEN = 0x20
    };

    struct DrawOutput {
        nsRefPtr<gfxASurface> mSurface;
        PRPackedBool mUniformAlpha;
        PRPackedBool mUniformColor;
        gfxRGBA      mColor;
    };

    









    nsresult Draw(gfxContext* ctx, int width, int height,
                  PRUint32 flags, DrawOutput* output);
};

#endif 
