





































#include "nsThebesImage.h"
#include "nsThebesRenderingContext.h"

#include "gfxContext.h"
#include "gfxPattern.h"

#include "gfxPlatform.h"

#include "prenv.h"

static PRBool gDisableOptimize = PR_FALSE;

#ifdef XP_WIN
static PRUint32 gTotalDDBs = 0;
static PRUint32 gTotalDDBSize = 0;

#define kMaxDDBSize (64*1024*1024)

#define kMaxSingleDDBSize (4*1024*1024)
#endif

NS_IMPL_ISUPPORTS1(nsThebesImage, nsIImage)

nsThebesImage::nsThebesImage()
    : mFormat(gfxImageSurface::ImageFormatRGB24),
      mWidth(0),
      mHeight(0),
      mDecoded(0,0,0,0),
      mImageComplete(PR_FALSE),
      mSinglePixel(PR_FALSE),
      mFormatChanged(PR_FALSE),
      mNeverUseDeviceSurface(PR_FALSE),
      mSinglePixelColor(0),
      mAlphaDepth(0)
{
    static PRBool hasCheckedOptimize = PR_FALSE;
    if (!hasCheckedOptimize) {
        if (PR_GetEnv("MOZ_DISABLE_IMAGE_OPTIMIZE")) {
            gDisableOptimize = PR_TRUE;
        }
        hasCheckedOptimize = PR_TRUE;
    }

#ifdef XP_WIN
    mIsDDBSurface = PR_FALSE;
#endif
}

nsresult
nsThebesImage::Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements)
{
    mWidth = aWidth;
    mHeight = aHeight;

    
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

    gfxImageSurface::gfxImageFormat format;
    switch(aMaskRequirements)
    {
        case nsMaskRequirements_kNeeds1Bit:
            format = gfxImageSurface::ImageFormatARGB32;
            mAlphaDepth = 1;
            break;
        case nsMaskRequirements_kNeeds8Bit:
            format = gfxImageSurface::ImageFormatARGB32;
            mAlphaDepth = 8;
            break;
        default:
            format = gfxImageSurface::ImageFormatRGB24;
            mAlphaDepth = 0;
            break;
    }

    mFormat = format;

    
    
    
#ifdef XP_WIN
    if (!mNeverUseDeviceSurface && !ShouldUseImageSurfaces()) {
        mWinSurface = new gfxWindowsSurface(gfxIntSize(mWidth, mHeight), format);
        if (mWinSurface && mWinSurface->CairoStatus() == 0) {
            
            mImageSurface = mWinSurface->GetImageSurface();
        } else {
            mWinSurface = nsnull;
        }
    }
#endif

    
    
    
    
    if (!mImageSurface)
        mImageSurface = new gfxImageSurface(gfxIntSize(mWidth, mHeight), format);

    if (!mImageSurface || mImageSurface->CairoStatus()) {
        mImageSurface = nsnull;
        
        return NS_ERROR_OUT_OF_MEMORY;
    }

#ifdef XP_MACOSX
    if (!mNeverUseDeviceSurface && !ShouldUseImageSurfaces()) {
        mQuartzSurface = new gfxQuartzImageSurface(mImageSurface);
    }
#endif

    mStride = mImageSurface->Stride();

    return NS_OK;
}

nsThebesImage::~nsThebesImage()
{
#ifdef XP_WIN
    if (mIsDDBSurface) {
        gTotalDDBs--;
        gTotalDDBSize -= mWidth*mHeight*4;
    }
#endif
}

PRInt32
nsThebesImage::GetBytesPix()
{
    return 4;
}

PRBool
nsThebesImage::GetIsRowOrderTopToBottom()
{
    return PR_TRUE;
}

PRInt32
nsThebesImage::GetWidth()
{
    return mWidth;
}

PRInt32
nsThebesImage::GetHeight()
{
    return mHeight;
}

PRUint8 *
nsThebesImage::GetBits()
{
    if (mImageSurface)
        return mImageSurface->Data();
    return nsnull;
}

PRInt32
nsThebesImage::GetLineStride()
{
    return mStride;
}

PRBool
nsThebesImage::GetHasAlphaMask()
{
    return mAlphaDepth > 0;
}

PRUint8 *
nsThebesImage::GetAlphaBits()
{
    return nsnull;
}

PRInt32
nsThebesImage::GetAlphaLineStride()
{
    return (mAlphaDepth > 0) ? mStride : 0;
}

nsresult
nsThebesImage::ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsIntRect *aUpdateRect)
{
    
    nsCOMPtr<nsIMemory> mem;
    NS_GetMemoryManager(getter_AddRefs(mem));
    if (!mem)
        return NS_ERROR_UNEXPECTED;

    PRBool lowMemory;
    mem->IsLowMemory(&lowMemory);
    if (lowMemory)
        return NS_ERROR_OUT_OF_MEMORY;

    mDecoded.UnionRect(mDecoded, *aUpdateRect);

    
    
    nsIntRect boundsRect(0, 0, mWidth, mHeight);
    mDecoded.IntersectRect(mDecoded, boundsRect);

#ifdef XP_MACOSX
    if (mQuartzSurface)
        mQuartzSurface->Flush();
#endif
    return NS_OK;
}

PRBool
nsThebesImage::GetIsImageComplete()
{
    if (!mImageComplete)
        mImageComplete = (mDecoded == nsIntRect(0, 0, mWidth, mHeight));
    return mImageComplete;
}

nsresult
nsThebesImage::Optimize(nsIDeviceContext* aContext)
{
    if (gDisableOptimize)
        return NS_OK;

    if (mOptSurface || mSinglePixel)
        return NS_OK;

    

    
    if (mStride == mWidth * 4) {
        PRUint32 *imgData = (PRUint32*) mImageSurface->Data();
        PRUint32 firstPixel = * (PRUint32*) imgData;
        PRUint32 pixelCount = mWidth * mHeight + 1;

        while (--pixelCount && *imgData++ == firstPixel)
            ;

        if (pixelCount == 0) {
            
            if (mFormat == gfxImageSurface::ImageFormatARGB32 ||
                mFormat == gfxImageSurface::ImageFormatRGB24)
            {
                mSinglePixelColor = gfxRGBA
                    (firstPixel,
                     (mFormat == gfxImageSurface::ImageFormatRGB24 ?
                      gfxRGBA::PACKED_XRGB :
                      gfxRGBA::PACKED_ARGB_PREMULTIPLIED));

                mSinglePixel = PR_TRUE;

                

                mImageSurface = nsnull;
                mOptSurface = nsnull;
#ifdef XP_WIN
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

#ifdef XP_WIN
    
    
    
    if (mWinSurface) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        
        PRUint32 ddbSize = mWidth * mHeight * 4;
        if (ddbSize <= kMaxSingleDDBSize &&
            ddbSize + gTotalDDBSize <= kMaxDDBSize)
        {
            nsRefPtr<gfxWindowsSurface> wsurf = mWinSurface->OptimizeToDDB(nsnull, gfxIntSize(mWidth, mHeight), mFormat);
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
#ifdef XP_WIN
        mWinSurface = nsnull;
#endif
#ifdef XP_MACOSX
        mQuartzSurface = nsnull;
#endif
    }

    return NS_OK;
}

nsColorMap *
nsThebesImage::GetColorMap()
{
    return NULL;
}

PRInt8
nsThebesImage::GetAlphaDepth()
{
    return mAlphaDepth;
}

void *
nsThebesImage::GetBitInfo()
{
    return NULL;
}

NS_IMETHODIMP
nsThebesImage::LockImagePixels(PRBool aMaskPixels)
{
    if (aMaskPixels)
        return NS_ERROR_NOT_IMPLEMENTED;
    if ((mOptSurface || mSinglePixel) && !mImageSurface) {
        
        mImageSurface = new gfxImageSurface(gfxIntSize(mWidth, mHeight),
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

#ifdef XP_WIN
        mWinSurface = nsnull;
#endif
#ifdef XP_MACOSX
        mQuartzSurface = nsnull;
#endif
    }

    return NS_OK;
}

NS_IMETHODIMP
nsThebesImage::UnlockImagePixels(PRBool aMaskPixels)
{
    if (aMaskPixels)
        return NS_ERROR_NOT_IMPLEMENTED;
    mOptSurface = nsnull;
#ifdef XP_MACOSX
    if (mQuartzSurface)
        mQuartzSurface->Flush();
#endif
    return NS_OK;
}

static PRBool
IsSafeImageTransformComponent(gfxFloat aValue)
{
    return aValue >= -32768 && aValue <= 32767;
}

void
nsThebesImage::Draw(gfxContext*        aContext,
                    gfxPattern::GraphicsFilter aFilter,
                    const gfxMatrix&   aUserSpaceToImageSpace,
                    const gfxRect&     aFill,
                    const nsIntMargin& aPadding,
                    const nsIntRect&   aSubimage)
{
    NS_ASSERTION(!aFill.IsEmpty(), "zero dest size --- fix caller");
    NS_ASSERTION(!aSubimage.IsEmpty(), "zero source size --- fix caller");

    PRBool doPadding = aPadding != nsIntMargin(0,0,0,0);
    PRBool doPartialDecode = !GetIsImageComplete();
    gfxContext::GraphicsOperator op = aContext->CurrentOperator();

    if (mSinglePixel && !doPadding && !doPartialDecode) {
        
        
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
    gfxRect imageRect(0, 0, mWidth + aPadding.LeftRight(), mHeight + aPadding.TopBottom());
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
            userSpaceToImageSpace.Multiply(
                gfxMatrix().Translate(-gfxPoint(aPadding.left, aPadding.top)));
            sourceRect = sourceRect - gfxPoint(aPadding.left, aPadding.top);
            imageRect = gfxRect(0, 0, mWidth, mHeight);
        } else {
            
            gfxIntSize size(PRInt32(imageRect.Width()),
                            PRInt32(imageRect.Height()));
            
            
            format = gfxASurface::ImageFormatARGB32;
            surface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(size,
                format);
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
    
    

    if (!AllowedImageSize(fill.size.width + 1, fill.size.height + 1)) {
        NS_WARNING("Destination area too large, bailing out");
        return;
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
            
            
            
            
            
            
            gfxRect imageSpaceClipExtents =
              userSpaceToImageSpace.TransformBounds(userSpaceClipExtents);
            
            
            imageSpaceClipExtents.Outset(1.0);

            gfxRect needed =
              imageSpaceClipExtents.Intersect(sourceRect).Intersect(subimage);
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
        case gfxASurface::SurfaceTypeXcb: {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
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

nsresult
nsThebesImage::Extract(const nsIntRect& aRegion,
                       nsIImage** aResult)
{
    nsRefPtr<nsThebesImage> subImage(new nsThebesImage());
    if (!subImage)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    
    
    
    
    
    
    
    subImage->mNeverUseDeviceSurface = PR_TRUE;

    
    nsMaskRequirements maskReq;
    switch (mAlphaDepth) {
    case 0: maskReq = nsMaskRequirements_kNoMask; break;
    case 1: maskReq = nsMaskRequirements_kNeeds1Bit; break;
    case 8: maskReq = nsMaskRequirements_kNeeds8Bit; break;
    default:
        NS_NOTREACHED("impossible alpha depth");
        maskReq = nsMaskRequirements_kNeeds8Bit; 
    }

    nsresult rv = subImage->Init(aRegion.width, aRegion.height,
                                 8 , maskReq);
    NS_ENSURE_SUCCESS(rv, rv);

    { 
        gfxContext ctx(subImage->ThebesSurface());
        ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
        if (mSinglePixel) {
            ctx.SetDeviceColor(mSinglePixelColor);
        } else {
            
            
            
            
            ctx.SetSource(this->ThebesSurface(),
                          gfxPoint(-aRegion.x, -aRegion.y));
        }
        ctx.Rectangle(gfxRect(0, 0, aRegion.width, aRegion.height));
        ctx.Fill();
    }

    nsIntRect filled(0, 0, aRegion.width, aRegion.height);
    subImage->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &filled);
    subImage->Optimize(nsnull);

    NS_ADDREF(*aResult = subImage);
    return NS_OK;
}

PRBool
nsThebesImage::ShouldUseImageSurfaces()
{
#if defined(WINCE)
    
    
    return PR_TRUE;

#elif defined(XP_WIN)
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







void
nsThebesImage::SetHasNoAlpha()
{
    if (mFormat == gfxASurface::ImageFormatARGB32) {
        mFormat = gfxASurface::ImageFormatRGB24;
        mFormatChanged = PR_TRUE;
        mAlphaDepth = 0;
    }
}
