





































#include "imgFrame.h"

#include <limits.h>

#include "prmem.h"
#include "prenv.h"

#include "gfxPlatform.h"

static PRBool gDisableOptimize = PR_FALSE;


#include "cairo.h"

#ifdef CAIRO_HAS_DDRAW_SURFACE
#include "gfxDDrawSurface.h"
#endif

#if defined(XP_WIN) || defined(WINCE)
#include "gfxWindowsPlatform.h"
#endif

#if defined(XP_WIN) && !defined(WINCE)


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
#if defined(WINCE)
  
  
  gfxWindowsPlatform::RenderMode rmode = gfxWindowsPlatform::GetPlatform()->GetRenderMode();
  return rmode != gfxWindowsPlatform::RENDER_DDRAW &&
      rmode != gfxWindowsPlatform::RENDER_DDRAW_GL;

#elif defined(USE_WIN_SURFACE)
  static const DWORD kGDIObjectsHighWaterMark = 7000;

  
  
  
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

  
  nsCOMPtr<nsIMemory> mem;
  NS_GetMemoryManager(getter_AddRefs(mem));
  if (!mem)
    return NS_ERROR_UNEXPECTED;

  PRBool lowMemory;
  mem->IsLowMemory(&lowMemory);
  if (lowMemory)
    return NS_ERROR_OUT_OF_MEMORY;

  mOffset.MoveTo(aX, aY);
  mSize.SizeTo(aWidth, aHeight);

  mFormat = aFormat;
  mPaletteDepth = aPaletteDepth;

  if (aPaletteDepth != 0) {
    
    if (aPaletteDepth > 8) {
      NS_ERROR("This Depth is not supported\n");
      return NS_ERROR_FAILURE;
    }

    mPalettedImageData = (PRUint8*)PR_MALLOC(PaletteDataLength() + GetImageDataLength());
    NS_ENSURE_TRUE(mPalettedImageData, NS_ERROR_OUT_OF_MEMORY);
  } else {
    
    
    
#ifdef USE_WIN_SURFACE
    if (!mNeverUseDeviceSurface && !ShouldUseImageSurfaces()) {
      mWinSurface = new gfxWindowsSurface(gfxIntSize(mSize.width, mSize.height), mFormat);
      if (mWinSurface && mWinSurface->CairoStatus() == 0) {
        
        mImageSurface = mWinSurface->GetImageSurface();
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

static PRBool                                                                                                       
IsSafeImageTransformComponent(gfxFloat aValue)
{
  return aValue >= -32768 && aValue <= 32767;
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
  gfxContext::GraphicsOperator op = aContext->CurrentOperator();

  if (mSinglePixel && !doPadding && ImageComplete()) {
    
    
    if (mSinglePixelColor.a == 0.0)
      return;

    if (op == gfxContext::OPERATOR_OVER && mSinglePixelColor.a == 1.0)
      aContext->SetOperator(gfxContext::OPERATOR_SOURCE);

    aContext->SetDeviceColor(mSinglePixelColor);
    aContext->NewPath();
    aContext->Rectangle(aFill);
    aContext->Fill();
    aContext->SetOperator(op);
    aContext->SetDeviceColor(gfxRGBA(0,0,0,0));
    return;
  }

  gfxMatrix userSpaceToImageSpace = aUserSpaceToImageSpace;
  gfxRect sourceRect = userSpaceToImageSpace.Transform(aFill);
  gfxRect imageRect(0, 0, mSize.width + aPadding.LeftRight(), mSize.height + aPadding.TopBottom());
  gfxRect subimage(aSubimage.x, aSubimage.y, aSubimage.width, aSubimage.height);
  gfxRect fill = aFill;
  nsRefPtr<gfxASurface> surface;
  gfxImageSurface::gfxImageFormat format;

  NS_ASSERTION(!sourceRect.Intersect(subimage).IsEmpty(),
               "We must be allowed to sample *some* source pixels!");

  PRBool doTile = !imageRect.Contains(sourceRect);
  if (doPadding || doPartialDecode) {
    gfxRect available = gfxRect(mDecoded.x, mDecoded.y, mDecoded.width, mDecoded.height) +
      gfxPoint(aPadding.left, aPadding.top);

    if (!doTile && !mSinglePixel) {
      
      
      
      sourceRect = sourceRect.Intersect(available);
      gfxMatrix imageSpaceToUserSpace = userSpaceToImageSpace;
      imageSpaceToUserSpace.Invert();
      fill = imageSpaceToUserSpace.Transform(sourceRect);

      surface = ThebesSurface();
      format = mFormat;
      subimage = subimage.Intersect(available) - gfxPoint(aPadding.left, aPadding.top);
      userSpaceToImageSpace.Multiply(gfxMatrix().Translate(-gfxPoint(aPadding.left, aPadding.top)));
      sourceRect = sourceRect - gfxPoint(aPadding.left, aPadding.top);
      imageRect = gfxRect(0, 0, mSize.width, mSize.height);
    } else {
      
      gfxIntSize size(PRInt32(imageRect.Width()), PRInt32(imageRect.Height()));
      
      
      format = gfxASurface::ImageFormatARGB32;
      surface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, format);
      if (!surface || surface->CairoStatus() != 0)
        return;

      
      gfxContext tmpCtx(surface);
      tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
      if (mSinglePixel) {
        tmpCtx.SetDeviceColor(mSinglePixelColor);
      } else {
        tmpCtx.SetSource(ThebesSurface(), gfxPoint(aPadding.left, aPadding.top));
      }
      tmpCtx.Rectangle(available);
      tmpCtx.Fill();
    }
  } else {
    NS_ASSERTION(!mSinglePixel, "This should already have been handled");
    surface = ThebesSurface();
    format = mFormat;
  }
  
  

  
  
  
  gfxFloat deviceX, deviceY;
  nsRefPtr<gfxASurface> currentTarget =
    aContext->CurrentSurface(&deviceX, &deviceY);
  gfxMatrix currentMatrix = aContext->CurrentMatrix();
  gfxMatrix deviceToUser = currentMatrix;
  deviceToUser.Invert();
  deviceToUser.Translate(-gfxPoint(-deviceX, -deviceY));
  gfxMatrix deviceToImage = deviceToUser;
  deviceToImage.Multiply(userSpaceToImageSpace);

  PRBool pushedGroup = PR_FALSE;
  if (currentTarget->GetType() != gfxASurface::SurfaceTypeQuartz) {
    
    
      
    
    if (!IsSafeImageTransformComponent(deviceToImage.xx) ||
        !IsSafeImageTransformComponent(deviceToImage.xy) ||
        !IsSafeImageTransformComponent(deviceToImage.yx) ||
        !IsSafeImageTransformComponent(deviceToImage.yy)) {
      NS_WARNING("Scaling up too much, bailing out");
      return;
    }

    if (!IsSafeImageTransformComponent(deviceToImage.x0) ||
        !IsSafeImageTransformComponent(deviceToImage.y0)) {
      
      
      aContext->Save();

      
      
      
      aContext->IdentityMatrix();
      gfxRect bounds = currentMatrix.TransformBounds(fill);
      bounds.RoundOut();
      aContext->Clip(bounds);
      aContext->SetMatrix(currentMatrix);

      aContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
      aContext->SetOperator(gfxContext::OPERATOR_OVER);
      pushedGroup = PR_TRUE;
    }
    
  }

  nsRefPtr<gfxPattern> pattern = new gfxPattern(surface);
  pattern->SetMatrix(userSpaceToImageSpace);

  
  
  
  
  
  if (!currentMatrix.HasNonIntegerTranslation() &&
      !userSpaceToImageSpace.HasNonIntegerTranslation()) {
    if (doTile) {
      pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
    }
  } else {
    if (doTile || !subimage.Contains(imageRect)) {
      
      
      

      gfxRect userSpaceClipExtents = aContext->GetClipExtents();
      
      
      
      
      
      gfxRect imageSpaceClipExtents = userSpaceToImageSpace.TransformBounds(userSpaceClipExtents);
      
      
      imageSpaceClipExtents.Outset(1.0);

      gfxRect needed = imageSpaceClipExtents.Intersect(sourceRect).Intersect(subimage);
      needed.RoundOut();

      
      
      
      
      if (!needed.IsEmpty()) {
        gfxIntSize size(PRInt32(needed.Width()), PRInt32(needed.Height()));
        nsRefPtr<gfxASurface> temp =
          gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, format);
        if (temp && temp->CairoStatus() == 0) {
          gfxContext tmpCtx(temp);
          tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
          nsRefPtr<gfxPattern> tmpPattern = new gfxPattern(surface);
          if (tmpPattern) {
            tmpPattern->SetExtend(gfxPattern::EXTEND_REPEAT);
            tmpPattern->SetMatrix(gfxMatrix().Translate(needed.pos));
            tmpCtx.SetPattern(tmpPattern);
            tmpCtx.Paint();
            tmpPattern = new gfxPattern(temp);
            if (tmpPattern) {
              pattern.swap(tmpPattern);
              pattern->SetMatrix(
                  gfxMatrix(userSpaceToImageSpace).Multiply(gfxMatrix().Translate(-needed.pos)));
            }
          }
        }
      }
    }

    
    
    
    switch (currentTarget->GetType()) {
      case gfxASurface::SurfaceTypeXlib:
      case gfxASurface::SurfaceTypeXcb:
      {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        PRBool isDownscale =
          deviceToImage.xx >= 1.0 && deviceToImage.yy >= 1.0 &&
          deviceToImage.xy == 0.0 && deviceToImage.yx == 0.0;
        if (!isDownscale) {
          pattern->SetFilter(gfxPattern::FILTER_FAST);
        }
        break;
      }

      case gfxASurface::SurfaceTypeQuartz:
      case gfxASurface::SurfaceTypeQuartzImage:
        
        pattern->SetFilter(aFilter);
        break;

      default:
        
        
        
        pattern->SetExtend(gfxPattern::EXTEND_PAD);
        pattern->SetFilter(aFilter);
        break;
    }
  }

  if ((op == gfxContext::OPERATOR_OVER || pushedGroup) &&
      format == gfxASurface::ImageFormatRGB24) {
    aContext->SetOperator(gfxContext::OPERATOR_SOURCE);
  }

  
  aContext->NewPath();
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  pattern->SetFilter(gfxPattern::FILTER_FAST); 
#endif
  aContext->SetPattern(pattern);
  aContext->Rectangle(fill);
  aContext->Fill();

  aContext->SetOperator(op);
  if (pushedGroup) {
    aContext->PopGroupToSource();
    aContext->Paint();
    aContext->Restore();
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
  
  nsCOMPtr<nsIMemory> mem;
  NS_GetMemoryManager(getter_AddRefs(mem));
  if (!mem)
    return NS_ERROR_UNEXPECTED;

  PRBool lowMemory;
  mem->IsLowMemory(&lowMemory);
  if (lowMemory)
    return NS_ERROR_OUT_OF_MEMORY;

  mDecoded.UnionRect(mDecoded, aUpdateRect);

  
  
  nsIntRect boundsRect(0, 0, mSize.width, mSize.height);
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
  else
    return mSize.width;
}

PRUint32 imgFrame::GetImageDataLength() const
{
  if (mImageSurface)
    return mImageSurface->Stride() * mSize.height;
  else
    return mSize.width * mSize.height;
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
    return NS_OK;

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

  return NS_OK;
}

nsresult imgFrame::UnlockImageData()
{
  if (mPalettedImageData)
    return NS_OK;

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
  return mDecoded == nsIntRect(0, 0, mSize.width, mSize.height);
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
