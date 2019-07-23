





































#include <Bitmap.h>

#include "gfxBeOSSurface.h"

gfxBeOSSurface::gfxBeOSSurface(BView* aView) :
    mOwnsView(PR_FALSE), mView(aView), mBitmap(nsnull)
{
    Init(cairo_beos_surface_create(aView));
}

gfxBeOSSurface::gfxBeOSSurface(BView* aView, BBitmap* aBitmap) :
   mOwnsView(PR_FALSE), mView(aView), mBitmap(nsnull)
{
    Init(cairo_beos_surface_create_for_bitmap(aView, aBitmap));
}

gfxBeOSSurface::gfxBeOSSurface(unsigned long width, unsigned long height, color_space space)
{
    BRect bounds(0.0, 0.0, width - 1, height - 1);
    mBitmap = new BBitmap(bounds, space, true);
    mView = new BView(bounds, "Mozilla Bitmap view", B_FOLLOW_ALL_SIDES, 0);
    mBitmap->AddChild(mView);

    mOwnsView = PR_TRUE;
    Init(cairo_beos_surface_create_for_bitmap(mView, mBitmap));
}

gfxBeOSSurface::~gfxBeOSSurface()
{
    if (mOwnsView) {
        mBitmap->RemoveChild(mView);

        delete mView;
        delete mBitmap;
    }
}
