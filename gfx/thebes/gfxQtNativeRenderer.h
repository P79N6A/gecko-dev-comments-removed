




#ifndef GFXQTNATIVERENDER_H_
#define GFXQTNATIVERENDER_H_

#include "gfxColor.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxXlibSurface.h"

class QRect;
struct nsIntRect;







class gfxQtNativeRenderer {
public:
    







    virtual nsresult DrawWithXlib(gfxXlibSurface *xsurf,
            nsIntPoint offset,
            nsIntRect* clipRects, uint32_t numClipRects) = 0;
  
    enum {
        
        
        
        
        DRAW_IS_OPAQUE = 0x01,
        
        DRAW_SUPPORTS_CLIP_RECT = 0x04,
        
        
        DRAW_SUPPORTS_CLIP_LIST = 0x08,
        
        
        DRAW_SUPPORTS_ALTERNATE_VISUAL = 0x10,
        
        
        
        DRAW_SUPPORTS_ALTERNATE_SCREEN = 0x20
    };

    









    nsresult Draw(gfxContext* ctx, nsIntSize size,
                  uint32_t flags, Screen* screen, Visual* visual);
};

#endif 
