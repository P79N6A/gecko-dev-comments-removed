





































#include "imgFrame.h"

#include <limits.h>

#include "prmem.h"
#include "prenv.h"

#include "gfxPlatform.h"
#include "gfxUtils.h"

static PRBool gDisableOptimize = PR_FALSE;

#include "cairo.h"

#if defined(XP_WIN)

#include "gfxWindowsPlatform.h"


#define USE_WIN_SURFACE 1

static PRUint32 gTotalDDBs = 0;
static PRUint32 gTotalDDBSize = 0;

#define kMaxDDBSize (64*1024*1024)

#define kMaxSingleDDBSize (4*1024*1024)

#endif


static PRBool AllowedImageSize(PRInt32 aWidth, PRInt32 aHeight)
{
  NS_ASSERTION(aWidth > 0, "invalid image width");
  NS_ASSERTION(aHeight > 0, "invalid image height");

  
  const PRInt32 k64KLimit = 0x0000FFFF;
  if (NS_UNLIKELY(aWidth > k64KLimit || aHeight > k64KLimit )) {
    NS_WARNING("image too big");
    return PR_FALSE;
  }

  
  
  if (NS_UNLIKELY(aHeight == 0)) {
    return PR_FALSE;
  }

  
  PRInt32 tmp = aWidth * aHeight;
  if (NS_UNLIKELY(tmp / aHeight != aWidth)) {
    NS_WARNING("width or height too large");
    return PR_FALSE;
  }
  tmp = tmp * 4;
  if (NS_UNLIKELY(tmp / 4 != aWidth * aHeight)) {
    NS_WARNING("width or height too large");
    return PR_FALSE;
  }
#if defined(XP_MACOSX)
  
  if (NS_UNLIKELY(aHeight > SHRT_MAX)) {
    NS_WARNING("image too big");
    return PR_FALSE;
  }
#endif
  return PR_TRUE;
}



static PRBool ShouldUseImageSurfaces()
{
#if defined(USE_WIN_SURFACE)
  static const DWORD kGDIObjectsHighWaterMark = 7000;

  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
      gfxWindowsPlatform::RENDER_DIRECT2D) {
    return PR_TRUE;
  }

  
  
  
  DWORD count = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
  if (count == 0 ||
      count > kGDIObjectsHighWaterMark)
  {
    
    
    
    return PR_TRUE;
  }
#endif

  return PR_FALSE;
}

imgFrame::imgFrame() :
  mDecoded(0, 0, 0, 0),
  mPalettedImageData(nsnull),
  mSinglePixelColor(0),
  mTimeout(100),
  mDisposalMethod(0), 
  mBlendMethod(1), 
  mSinglePixel(PR_FALSE),
  mNeverUseDeviceSurface(PR_FALSE),
  mFormatChanged(PR_FALSE),
  mCompositingFailed(PR_FALSE)
#ifdef USE_WIN_SURFACE
  , mIsDDBSurface(PR_FALSE)
#endif
  , mLocked(PR_FALSE)
{
  static PRBool hasCheckedOptimize = PR_FALSE;
  if (!hasCheckedOptimize) {
    if (PR_GetEnv("MOZ_DISABLE_IMAGE_OPTIMIZE")) {
      gDisableOptimize = PR_TRUE;
    }
    hasCheckedOptimize = PR_TRUE;
  }
}

imgFrame::~imgFrame()
{
  PR_FREEIF(mPalettedImageData);
#ifdef USE_WIN_SURFACE
  if (mIsDDBSurface) {
      gTotalDDBs--;
      gTotalDDBSize -= mSize.width * mSize.height * 4;
  }
#endif
}

nsresult imgFrame::Init(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, 
                        gfxASurface::gfxImageFormat aFormat, PRInt8 aPaletteDepth )
{
  
  if (!AllowedImageSize(aWidth, aHeight))
    return NS_ERROR_FAILURE;

  mOffset.MoveTo(aX, aY);
  mSize.SizeTo(aWidth, aHeight);

  mFormat = aFormat;
  mPaletteDepth = aPaletteDepth;

  if (aPaletteDepth != 0) {
    
    if (aPaletteDepth > 8) {
      NS_ERROR("This Depth is not supported");
      return NS_ERROR_FAILURE;
    }

    
    mPalettedImageData = (PRUint8*)moz_malloc(PaletteDataLength() + GetImageDataLength());
    NS_ENSURE_TRUE(mPalettedImageData, NS_ERROR_OUT_OF_MEMORY);
  } else {
    
    
    
#ifdef USE_WIN_SURFACE
    if (!mNeverUseDeviceSurface && !ShouldUseImageSurfaces()) {
      mWinSurface = new gfxWindowsSurface(gfxIntSize(mSize.width, mSize.height), mFormat);
      if (mWinSurface && mWinSurface->CairoStatus() == 0) {
        
        mImageSurface = mWinSurface->GetAsImageSurface();
      } else {
        mWinSurface = nsnull;
      }
    }
#endif

    
    
    
    
    if (!mImageSurface)
      mImageSurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height), mFormat);

    if (!mImageSurface || mImageSurface->CairoStatus()) {
      mImageSurface = nsnull;
      
      return NS_ERROR_OUT_OF_MEMORY;
    }

#ifdef XP_MACOSX
    if (!mNeverUseDeviceSurface && !ShouldUseImageSurfaces()) {
      mQuartzSurface = new gfxQuartzImageSurface(mImageSurface);
    }
#endif
  }

  return NS_OK;
}

nsresult imgFrame::Optimize()
{
  if (gDisableOptimize)
    return NS_OK;

  if (mPalettedImageData || mOptSurface || mSinglePixel)
    return NS_OK;

  

  
  if (mImageSurface->Stride() == mSize.width * 4) {
    PRUint32 *imgData = (PRUint32*) mImageSurface->Data();
    PRUint32 firstPixel = * (PRUint32*) imgData;
    PRUint32 pixelCount = mSize.width * mSize.height + 1;

    while (--pixelCount && *imgData++ == firstPixel)
      ;

    if (pixelCount == 0) {
      
      if (mFormat == gfxASurface::ImageFormatARGB32 ||
          mFormat == gfxASurface::ImageFormatRGB24)
      {
        mSinglePixelColor = gfxRGBA
          (firstPixel,
           (mFormat == gfxImageSurface::ImageFormatRGB24 ?
            gfxRGBA::PACKED_XRGB :
            gfxRGBA::PACKED_ARGB_PREMULTIPLIED));

        mSinglePixel = PR_TRUE;

        
        mImageSurface = nsnull;
        mOptSurface = nsnull;
#ifdef USE_WIN_SURFACE
        mWinSurface = nsnull;
#endif
#ifdef XP_MACOSX
        mQuartzSurface = nsnull;
#endif
        return NS_OK;
      }
    }

    
  }

  
  
  if (mNeverUseDeviceSurface || ShouldUseImageSurfaces())
    return NS_OK;

  mOptSurface = nsnull;

#ifdef USE_WIN_SURFACE
  
  
  
  if (mWinSurface) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    PRUint32 ddbSize = mSize.width * mSize.height * 4;
    if (ddbSize <= kMaxSingleDDBSize &&
        ddbSize + gTotalDDBSize <= kMaxDDBSize)
    {
      nsRefPtr<gfxWindowsSurface> wsurf = mWinSurface->OptimizeToDDB(nsnull, gfxIntSize(mSize.width, mSize.height), mFormat);
      if (wsurf) {
        gTotalDDBs++;
        gTotalDDBSize += ddbSize;
        mIsDDBSurface = PR_TRUE;
        mOptSurface = wsurf;
      }
    }
    if (!mOptSurface && !mFormatChanged) {
      
      mOptSurface = mWinSurface;
    }
  }
#endif

#ifdef XP_MACOSX
  if (mQuartzSurface) {
    mQuartzSurface->Flush();
    mOptSurface = mQuartzSurface;
  }
#endif

  if (mOptSurface == nsnull)
    mOptSurface = gfxPlatform::GetPlatform()->OptimizeImage(mImageSurface, mFormat);

  if (mOptSurface) {
    mImageSurface = nsnull;
#ifdef USE_WIN_SURFACE
    mWinSurface = nsnull;
#endif
#ifdef XP_MACOSX
    mQuartzSurface = nsnull;
#endif
  }

  return NS_OK;
}

static void
DoSingleColorFastPath(gfxContext*    aContext,
                      const gfxRGBA& aSinglePixelColor,
                      const gfxRect& aFill)
{
  
  if (aSinglePixelColor.a == 0.0)
    return;

  gfxContext::GraphicsOperator op = aContext->CurrentOperator();
  if (op == gfxContext::OPERATOR_OVER && aSinglePixelColor.a == 1.0) {
    aContext->SetOperator(gfxContext::OPERATOR_SOURCE);
  }

  aContext->SetDeviceColor(aSinglePixelColor);
  aContext->NewPath();
  aContext->Rectangle(aFill);
  aContext->Fill();
  aContext->SetOperator(op);
  aContext->SetDeviceColor(gfxRGBA(0,0,0,0));
}

imgFrame::SurfaceWithFormat
imgFrame::SurfaceForDrawing(PRBool             aDoPadding,
                            PRBool             aDoPartialDecode,
                            PRBool             aDoTile,
                            const nsIntMargin& aPadding,
                            gfxMatrix&         aUserSpaceToImageSpace,
                            gfxRect&           aFill,
                            gfxRect&           aSubimage,
                            gfxRect&           aSourceRect,
                            gfxRect&           aImageRect)
{
  gfxIntSize size(PRInt32(aImageRect.Width()), PRInt32(aImageRect.Height()));
  if (!aDoPadding && !aDoPartialDecode) {
    NS_ASSERTION(!mSinglePixel, "This should already have been handled");
    return SurfaceWithFormat(new gfxSurfaceDrawable(ThebesSurface(), size), mFormat);
  }

  gfxRect available = gfxRect(mDecoded.x, mDecoded.y, mDecoded.width, mDecoded.height);

  if (aDoTile || mSinglePixel) {
    
    
    
    gfxImageSurface::gfxImageFormat format = gfxASurface::ImageFormatARGB32;
    nsRefPtr<gfxASurface> surface =
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, gfxImageSurface::ContentFromFormat(format));
    if (!surface || surface->CairoStatus())
      return SurfaceWithFormat();

    
    gfxContext tmpCtx(surface);
    tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    if (mSinglePixel) {
      tmpCtx.SetDeviceColor(mSinglePixelColor);
    } else {
      tmpCtx.SetSource(ThebesSurface(), gfxPoint(aPadding.left, aPadding.top));
    }
    tmpCtx.Rectangle(available);
    tmpCtx.Fill();
    return SurfaceWithFormat(new gfxSurfaceDrawable(surface, size), format);
  }

  
  
  
  aSourceRect = aSourceRect.Intersect(available);
  gfxMatrix imageSpaceToUserSpace = aUserSpaceToImageSpace;
  imageSpaceToUserSpace.Invert();
  aFill = imageSpaceToUserSpace.Transform(aSourceRect);

  aSubimage = aSubimage.Intersect(available) - gfxPoint(aPadding.left, aPadding.top);
  aUserSpaceToImageSpace.Multiply(gfxMatrix().Translate(-gfxPoint(aPadding.left, aPadding.top)));
  aSourceRect = aSourceRect - gfxPoint(aPadding.left, aPadding.top);
  aImageRect = gfxRect(0, 0, mSize.width, mSize.height);

  gfxIntSize availableSize(mDecoded.width, mDecoded.height);
  return SurfaceWithFormat(new gfxSurfaceDrawable(ThebesSurface(),
                                                  availableSize),
                           mFormat);
}

void imgFrame::Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter,
                    const gfxMatrix &aUserSpaceToImageSpace, const gfxRect& aFill,
                    const nsIntMargin &aPadding, const nsIntRect &aSubimage)
{
  NS_ASSERTION(!aFill.IsEmpty(), "zero dest size --- fix caller");
  NS_ASSERTION(!aSubimage.IsEmpty(), "zero source size --- fix caller");
  NS_ASSERTION(!mPalettedImageData, "Directly drawing a paletted image!");

  PRBool doPadding = aPadding != nsIntMargin(0,0,0,0);
  PRBool doPartialDecode = !ImageComplete();

  if (mSinglePixel && !doPadding && !doPartialDecode) {
    DoSingleColorFastPath(aContext, mSinglePixelColor, aFill);
    return;
  }

  gfxMatrix userSpaceToImageSpace = aUserSpaceToImageSpace;
  gfxRect sourceRect = userSpaceToImageSpace.Transform(aFill);
  gfxRect imageRect(0, 0, mSize.width + aPadding.LeftRight(),
                    mSize.height + aPadding.TopBottom());
  gfxRect subimage(aSubimage.x, aSubimage.y, aSubimage.width, aSubimage.height);
  gfxRect fill = aFill;

  NS_ASSERTION(!sourceRect.Intersect(subimage).IsEmpty(),
               "We must be allowed to sample *some* source pixels!");

  PRBool doTile = !imageRect.Contains(sourceRect);
  SurfaceWithFormat surfaceResult =
    SurfaceForDrawing(doPadding, doPartialDecode, doTile, aPadding,
                      userSpaceToImageSpace, fill, subimage, sourceRect,
                      imageRect);

  if (surfaceResult.IsValid()) {
    gfxUtils::DrawPixelSnapped(aContext, surfaceResult.mDrawable,
                               userSpaceToImageSpace,
                               subimage, sourceRect, imageRect, fill,
                               surfaceResult.mFormat, aFilter);
  }
}

nsresult imgFrame::Extract(const nsIntRect& aRegion, imgFrame** aResult)
{
  nsAutoPtr<imgFrame> subImage(new imgFrame());
  if (!subImage)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  
  
  
  
  
  
  
  subImage->mNeverUseDeviceSurface = PR_TRUE;

  nsresult rv = subImage->Init(0, 0, aRegion.width, aRegion.height, 
                               mFormat, mPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  
  {
    gfxContext ctx(subImage->ThebesSurface());
    ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
    if (mSinglePixel) {
      ctx.SetDeviceColor(mSinglePixelColor);
    } else {
      
      
      
      
      ctx.SetSource(this->ThebesSurface(), gfxPoint(-aRegion.x, -aRegion.y));
    }
    ctx.Rectangle(gfxRect(0, 0, aRegion.width, aRegion.height));
    ctx.Fill();
  }

  nsIntRect filled(0, 0, aRegion.width, aRegion.height);

  rv = subImage->ImageUpdated(filled);
  NS_ENSURE_SUCCESS(rv, rv);

  subImage->Optimize();

  *aResult = subImage.forget();

  return NS_OK;
}

nsresult imgFrame::ImageUpdated(const nsIntRect &aUpdateRect)
{
  mDecoded.UnionRect(mDecoded, aUpdateRect);

  
  
  nsIntRect boundsRect(mOffset, mSize);
  mDecoded.IntersectRect(mDecoded, boundsRect);

#ifdef XP_MACOSX
  if (mQuartzSurface)
    mQuartzSurface->Flush();
#endif
  return NS_OK;
}

PRInt32 imgFrame::GetX() const
{
  return mOffset.x;
}

PRInt32 imgFrame::GetY() const
{
  return mOffset.y;
}

PRInt32 imgFrame::GetWidth() const
{
  return mSize.width;
}

PRInt32 imgFrame::GetHeight() const
{
  return mSize.height;
}

nsIntRect imgFrame::GetRect() const
{
  return nsIntRect(mOffset, mSize);
}

gfxASurface::gfxImageFormat imgFrame::GetFormat() const
{
  return mFormat;
}

PRBool imgFrame::GetNeedsBackground() const
{
  
  return (mFormat == gfxASurface::ImageFormatARGB32 || !ImageComplete());
}

PRUint32 imgFrame::GetImageBytesPerRow() const
{
  if (mImageSurface)
    return mImageSurface->Stride();

  if (mPaletteDepth)
    return mSize.width;

  NS_ERROR("GetImageBytesPerRow called with mImageSurface == null and mPaletteDepth == 0");

  return 0;
}

PRUint32 imgFrame::GetImageDataLength() const
{
  return GetImageBytesPerRow() * mSize.height;
}

void imgFrame::GetImageData(PRUint8 **aData, PRUint32 *length) const
{
  if (mImageSurface)
    *aData = mImageSurface->Data();
  else if (mPalettedImageData)
    *aData = mPalettedImageData + PaletteDataLength();
  else
    *aData = nsnull;

  *length = GetImageDataLength();
}

PRBool imgFrame::GetIsPaletted() const
{
  return mPalettedImageData != nsnull;
}

PRBool imgFrame::GetHasAlpha() const
{
  return mFormat == gfxASurface::ImageFormatARGB32;
}

void imgFrame::GetPaletteData(PRUint32 **aPalette, PRUint32 *length) const
{
  if (!mPalettedImageData) {
    *aPalette = nsnull;
    *length = 0;
  } else {
    *aPalette = (PRUint32 *) mPalettedImageData;
    *length = PaletteDataLength();
  }
}

nsresult imgFrame::LockImageData()
{
  if (mPalettedImageData)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ABORT_IF_FALSE(!mLocked, "Trying to lock already locked image data.");
  if (mLocked) {
    return NS_ERROR_FAILURE;
  }
  mLocked = PR_TRUE;

  if ((mOptSurface || mSinglePixel) && !mImageSurface) {
    
    mImageSurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height),
                                        gfxImageSurface::ImageFormatARGB32);
    if (!mImageSurface || mImageSurface->CairoStatus())
      return NS_ERROR_OUT_OF_MEMORY;

    gfxContext context(mImageSurface);
    context.SetOperator(gfxContext::OPERATOR_SOURCE);
    if (mSinglePixel)
      context.SetDeviceColor(mSinglePixelColor);
    else
      context.SetSource(mOptSurface);
    context.Paint();

    mOptSurface = nsnull;
#ifdef USE_WIN_SURFACE
    mWinSurface = nsnull;
#endif
#ifdef XP_MACOSX
    mQuartzSurface = nsnull;
#endif
  }

  
  
  if (mImageSurface)
    mImageSurface->Flush();

#ifdef USE_WIN_SURFACE
  if (mWinSurface)
    mWinSurface->Flush();
#endif

  return NS_OK;
}

nsresult imgFrame::UnlockImageData()
{
  if (mPalettedImageData)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ABORT_IF_FALSE(mLocked, "Unlocking an unlocked image!");
  if (!mLocked) {
    return NS_ERROR_FAILURE;
  }

  mLocked = PR_FALSE;

  
  if (mImageSurface)
    mImageSurface->MarkDirty();

#ifdef USE_WIN_SURFACE
  if (mWinSurface)
    mWinSurface->MarkDirty();
#endif

#ifdef XP_MACOSX
  
  
  if (mQuartzSurface)
    mQuartzSurface->Flush();
#endif
  return NS_OK;
}

PRInt32 imgFrame::GetTimeout() const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mTimeout >= 0 && mTimeout <= 10)
    return 100;
  else
    return mTimeout;
}

void imgFrame::SetTimeout(PRInt32 aTimeout)
{
  mTimeout = aTimeout;
}

PRInt32 imgFrame::GetFrameDisposalMethod() const
{
  return mDisposalMethod;
}

void imgFrame::SetFrameDisposalMethod(PRInt32 aFrameDisposalMethod)
{
  mDisposalMethod = aFrameDisposalMethod;
}

PRInt32 imgFrame::GetBlendMethod() const
{
  return mBlendMethod;
}

void imgFrame::SetBlendMethod(PRInt32 aBlendMethod)
{
  mBlendMethod = (PRInt8)aBlendMethod;
}

PRBool imgFrame::ImageComplete() const
{
  return mDecoded.IsEqualInterior(nsIntRect(mOffset, mSize));
}







void imgFrame::SetHasNoAlpha()
{
  if (mFormat == gfxASurface::ImageFormatARGB32) {
      mFormat = gfxASurface::ImageFormatRGB24;
      mFormatChanged = PR_TRUE;
  }
}

PRBool imgFrame::GetCompositingFailed() const
{
  return mCompositingFailed;
}

void imgFrame::SetCompositingFailed(PRBool val)
{
  mCompositingFailed = val;
}

PRUint32 imgFrame::EstimateMemoryUsed() const
{
  PRUint32 size = 0;

  if (mSinglePixel) {
    size += sizeof(gfxRGBA);
  }

  if (mPalettedImageData) {
    size += GetImageDataLength() + PaletteDataLength();
  }

#ifdef USE_WIN_SURFACE
  if (mWinSurface) {
    size += mWinSurface->KnownMemoryUsed();
  } else
#endif
#ifdef XP_MACOSX
  if (mQuartzSurface) {
    size += mSize.width * mSize.height * 4;
  } else
#endif
  if (mImageSurface) {
    size += mImageSurface->KnownMemoryUsed();
  }

  if (mOptSurface) {
    size += mOptSurface->KnownMemoryUsed();
  }

  
  if (size == 0) {
    size = mSize.width * mSize.height * 4;
  }

  return size;
}
