





































#ifndef GFX_BEOSSURFACE_H
#define GFX_BEOSSURFACE_H

#include "gfxASurface.h"

#include <cairo-beos.h>

class gfxBeOSSurface : public gfxASurface {
public:
    gfxBeOSSurface(BView* aView);
    gfxBeOSSurface(BView* aView, BBitmap* aBitmap);
    gfxBeOSSurface(unsigned long width, unsigned long height, color_space space = B_RGBA32);
    virtual ~gfxBeOSSurface();

private:
    PRBool mOwnsView;
    BView* mView;
    BBitmap* mBitmap;
};

#endif 
