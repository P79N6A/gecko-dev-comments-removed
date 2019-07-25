




































#ifndef GFXQTNATIVERENDER_H_
#define GFXQTNATIVERENDER_H_

#include "gfxColor.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxXlibSurface.h"
#include "nsRect.h"

class QRect;







class THEBES_API gfxQtNativeRenderer {
public:
    







    virtual nsresult DrawWithXlib(gfxXlibSurface *xsurf,
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

    









    nsresult Draw(gfxContext* ctx, nsIntSize size,
                  PRUint32 flags, Screen* screen, Visual* visual,
                  DrawOutput* output);
};

#endif 
