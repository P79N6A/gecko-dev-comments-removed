




#include "gfxBlur.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"

#include "mozilla/gfx/Blur.h"
#include "mozilla/gfx/2D.h"

using namespace mozilla::gfx;

gfxAlphaBoxBlur::gfxAlphaBoxBlur()
 : mBlur(nullptr)
{
}

gfxAlphaBoxBlur::~gfxAlphaBoxBlur()
{
  mContext = nullptr;
  delete mBlur;
}

gfxContext*
gfxAlphaBoxBlur::Init(const gfxRect& aRect,
                      const gfxIntSize& aSpreadRadius,
                      const gfxIntSize& aBlurRadius,
                      const gfxRect* aDirtyRect,
                      const gfxRect* aSkipRect)
{
    mozilla::gfx::Rect rect(Float(aRect.x), Float(aRect.y),
                            Float(aRect.width), Float(aRect.height));
    IntSize spreadRadius(aSpreadRadius.width, aSpreadRadius.height);
    IntSize blurRadius(aBlurRadius.width, aBlurRadius.height);
    nsAutoPtr<mozilla::gfx::Rect> dirtyRect;
    if (aDirtyRect) {
      dirtyRect = new mozilla::gfx::Rect(Float(aDirtyRect->x),
                                         Float(aDirtyRect->y),
                                         Float(aDirtyRect->width),
                                         Float(aDirtyRect->height));
    }
    nsAutoPtr<mozilla::gfx::Rect> skipRect;
    if (aSkipRect) {
      skipRect = new mozilla::gfx::Rect(Float(aSkipRect->x),
                                        Float(aSkipRect->y),
                                        Float(aSkipRect->width),
                                        Float(aSkipRect->height));
    }

    mBlur = new AlphaBoxBlur(rect, spreadRadius, blurRadius, dirtyRect, skipRect);
    int32_t blurDataSize = mBlur->GetSurfaceAllocationSize();
    if (blurDataSize <= 0)
        return nullptr;

    IntSize size = mBlur->GetSize();

    
    
    mData = new unsigned char[blurDataSize];
    memset(mData, 0, blurDataSize);

    RefPtr<DrawTarget> dt =
        gfxPlatform::GetPlatform()->CreateDrawTargetForData(mData, size,
                                                            mBlur->GetStride(),
                                                            FORMAT_A8);
    if (!dt) {
        nsRefPtr<gfxImageSurface> image =
            new gfxImageSurface(mData,
                                gfxIntSize(size.width, size.height),
                                mBlur->GetStride(),
                                gfxImageFormatA8);
        dt = Factory::CreateDrawTargetForCairoSurface(image->CairoSurface(), size);
        if (!dt) {
            return nullptr;
        }
    }

    IntRect irect = mBlur->GetRect();
    gfxPoint topleft(irect.TopLeft().x, irect.TopLeft().y);

    mContext = new gfxContext(dt);
    mContext->Translate(-topleft);

    return mContext;
}

void
gfxAlphaBoxBlur::Paint(gfxContext* aDestinationCtx)
{
    if (!mContext)
        return;

    mBlur->Blur(mData);

    mozilla::gfx::Rect* dirtyRect = mBlur->GetDirtyRect();

    DrawTarget *dest = aDestinationCtx->GetDrawTarget();
    if (!dest) {
      NS_ERROR("Blurring not supported for Thebes contexts!");
      return;
    }

    RefPtr<SourceSurface> mask = dest->CreateSourceSurfaceFromData(mData,
                                                                   mBlur->GetSize(),
                                                                   mBlur->GetStride(),
                                                                   FORMAT_A8);
    if (!mask) {
      NS_ERROR("Failed to create mask!");
      return;
    }

    nsRefPtr<gfxPattern> thebesPat = aDestinationCtx->GetPattern();
    Pattern* pat = thebesPat->GetPattern(dest, nullptr);

    Matrix oldTransform = dest->GetTransform();
    Matrix newTransform = oldTransform;
    newTransform.Translate(mBlur->GetRect().x, mBlur->GetRect().y);

    
    
    if (dirtyRect) {
        dest->PushClipRect(*dirtyRect);
    }

    dest->SetTransform(newTransform);
    dest->MaskSurface(*pat, mask, Point(0, 0));
    dest->SetTransform(oldTransform);

    if (dirtyRect) {
        dest->PopClip();
    }
}

gfxIntSize gfxAlphaBoxBlur::CalculateBlurRadius(const gfxPoint& aStd)
{
    mozilla::gfx::Point std(Float(aStd.x), Float(aStd.y));
    IntSize size = AlphaBoxBlur::CalculateBlurRadius(std);
    return gfxIntSize(size.width, size.height);
}

 void
gfxAlphaBoxBlur::BlurRectangle(gfxContext *aDestinationCtx,
                               const gfxRect& aRect,
                               gfxCornerSizes* aCornerRadii,
                               const gfxPoint& aBlurStdDev,
                               const gfxRGBA& aShadowColor,
                               const gfxRect& aDirtyRect,
                               const gfxRect& aSkipRect)
{
  gfxIntSize blurRadius = CalculateBlurRadius(aBlurStdDev);

  
  gfxAlphaBoxBlur blur;
  gfxContext *dest = blur.Init(aRect, gfxIntSize(), blurRadius, &aDirtyRect, &aSkipRect);

  if (!dest) {
    return;
  }

  gfxRect shadowGfxRect = aRect;
  shadowGfxRect.Round();

  dest->NewPath();
  if (aCornerRadii) {
    dest->RoundedRectangle(shadowGfxRect, *aCornerRadii);
  } else {
    dest->Rectangle(shadowGfxRect);
  }
  dest->Fill();

  aDestinationCtx->SetColor(aShadowColor);
  blur.Paint(aDestinationCtx);
}
