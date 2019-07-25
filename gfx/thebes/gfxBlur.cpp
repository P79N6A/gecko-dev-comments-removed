




































#include "gfxBlur.h"

#include "mozilla/gfx/Blur.h"

using namespace mozilla::gfx;

gfxAlphaBoxBlur::gfxAlphaBoxBlur()
 : mBlur(nsnull)
{
}

gfxAlphaBoxBlur::~gfxAlphaBoxBlur()
{
  delete mBlur;
}

gfxContext*
gfxAlphaBoxBlur::Init(const gfxRect& aRect,
                      const gfxIntSize& aSpreadRadius,
                      const gfxIntSize& aBlurRadius,
                      const gfxRect* aDirtyRect,
                      const gfxRect* aSkipRect)
{
    Rect rect(aRect.x, aRect.y, aRect.width, aRect.height);
    IntSize spreadRadius(aSpreadRadius.width, aSpreadRadius.height);
    IntSize blurRadius(aBlurRadius.width, aBlurRadius.height);
    nsAutoPtr<Rect> dirtyRect;
    if (aDirtyRect) {
      dirtyRect = new Rect(aDirtyRect->x, aDirtyRect->y, aDirtyRect->width, aDirtyRect->height);
    }
    nsAutoPtr<Rect> skipRect;
    if (aSkipRect) {
      skipRect = new Rect(aSkipRect->x, aSkipRect->y, aSkipRect->width, aSkipRect->height);
    }

    mBlur = new AlphaBoxBlur(rect, spreadRadius, blurRadius, dirtyRect, skipRect);

    unsigned char* data = mBlur->GetData();
    if (!data)
      return nsnull;

    IntSize size = mBlur->GetSize();
    
    
    mImageSurface = new gfxImageSurface(data, gfxIntSize(size.width, size.height),
                                        mBlur->GetStride(),
                                        gfxASurface::ImageFormatA8);
    if (mImageSurface->CairoStatus())
        return nsnull;

    IntRect irect = mBlur->GetRect();
    gfxPoint topleft(irect.TopLeft().x, irect.TopLeft().y);

    
    
    
    mImageSurface->SetDeviceOffset(-topleft);

    mContext = new gfxContext(mImageSurface);

    return mContext;
}

void
gfxAlphaBoxBlur::Paint(gfxContext* aDestinationCtx, const gfxPoint& offset)
{
    if (!mContext)
        return;

    mBlur->Blur();

    Rect* dirtyrect = mBlur->GetDirtyRect();

    
    
    if (dirtyrect) {
        aDestinationCtx->Save();
        aDestinationCtx->NewPath();
        gfxRect dirty(dirtyrect->x, dirtyrect->y, dirtyrect->width, dirtyrect->height);
        aDestinationCtx->Rectangle(dirty);
        aDestinationCtx->Clip();
        aDestinationCtx->Mask(mImageSurface, offset);
        aDestinationCtx->Restore();
    } else {
        aDestinationCtx->Mask(mImageSurface, offset);
    }
}

gfxIntSize gfxAlphaBoxBlur::CalculateBlurRadius(const gfxPoint& aStd)
{
    Point std(aStd.x, aStd.y);
    IntSize size = AlphaBoxBlur::CalculateBlurRadius(std);
    return gfxIntSize(size.width, size.height);
}
