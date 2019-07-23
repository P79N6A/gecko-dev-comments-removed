





































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
    if (!ShouldUseImageSurfaces()) {
        mWinSurface = new gfxWindowsSurface(gfxIntSize(mWidth, mHeight), format);
        if (mWinSurface && mWinSurface->CairoStatus() == 0) {
            
            mImageSurface = mWinSurface->GetImageSurface();
        }
    }

    if (!mImageSurface)
        mWinSurface = nsnull;
#endif

    if (!mImageSurface)
        mImageSurface = new gfxImageSurface(gfxIntSize(mWidth, mHeight), format);

    if (!mImageSurface || mImageSurface->CairoStatus()) {
        mImageSurface = nsnull;
        
        return NS_ERROR_OUT_OF_MEMORY;
    }

#ifdef XP_MACOSX
    mQuartzSurface = new gfxQuartzImageSurface(mImageSurface);
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
nsThebesImage::ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect)
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
        mImageComplete = (mDecoded == nsRect(0, 0, mWidth, mHeight));
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

    
    
    if (ShouldUseImageSurfaces())
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


NS_IMETHODIMP
nsThebesImage::Draw(nsIRenderingContext &aContext,
                    const gfxRect &aSourceRect,
                    const gfxRect &aSubimageRect,
                    const gfxRect &aDestRect)
{
    if (NS_UNLIKELY(aDestRect.IsEmpty())) {
        NS_ERROR("nsThebesImage::Draw zero dest size - please fix caller.");
        return NS_OK;
    }

    nsThebesRenderingContext *thebesRC = static_cast<nsThebesRenderingContext*>(&aContext);
    gfxContext *ctx = thebesRC->ThebesContext();

#if 0
    fprintf (stderr, "nsThebesImage::Draw src [%f %f %f %f] dest [%f %f %f %f] trans: [%f %f] dec: [%f %f]\n",
             aSourceRect.pos.x, aSourceRect.pos.y, aSourceRect.size.width, aSourceRect.size.height,
             aDestRect.pos.x, aDestRect.pos.y, aDestRect.size.width, aDestRect.size.height,
             ctx->CurrentMatrix().GetTranslation().x, ctx->CurrentMatrix().GetTranslation().y,
             mDecoded.x, mDecoded.y, mDecoded.width, mDecoded.height);
#endif

    if (mSinglePixel) {
        
        if (mSinglePixelColor.a == 0.0)
            return NS_OK;

        
        gfxContext::GraphicsOperator op = ctx->CurrentOperator();
        if (op == gfxContext::OPERATOR_OVER && mSinglePixelColor.a == 1.0)
            ctx->SetOperator(gfxContext::OPERATOR_SOURCE);

        ctx->SetDeviceColor(mSinglePixelColor);
        ctx->NewPath();
        ctx->Rectangle(aDestRect, PR_TRUE);
        ctx->Fill();
        ctx->SetOperator(op);
        return NS_OK;
    }

    gfxFloat xscale = aDestRect.size.width / aSourceRect.size.width;
    gfxFloat yscale = aDestRect.size.height / aSourceRect.size.height;

    gfxRect srcRect(aSourceRect);
    gfxRect subimageRect(aSubimageRect);
    gfxRect destRect(aDestRect);

    if (!GetIsImageComplete()) {
        gfxRect decoded = gfxRect(mDecoded.x, mDecoded.y,
                                  mDecoded.width, mDecoded.height);
        srcRect = srcRect.Intersect(decoded);
        subimageRect = subimageRect.Intersect(decoded);

        
        if (NS_UNLIKELY(srcRect.size.width == 0 || srcRect.size.height == 0))
            return NS_OK;

        destRect.pos.x += (srcRect.pos.x - aSourceRect.pos.x)*xscale;
        destRect.pos.y += (srcRect.pos.y - aSourceRect.pos.y)*yscale;

        destRect.size.width  = srcRect.size.width * xscale;
        destRect.size.height = srcRect.size.height * yscale;
    }

    
    if (srcRect.IsEmpty() || destRect.IsEmpty())
        return NS_OK;

    
    if (!AllowedImageSize(destRect.size.width + 1, destRect.size.height + 1))
        return NS_ERROR_FAILURE;

    
    
    
    subimageRect.RoundOut();

    nsRefPtr<gfxPattern> pat;
    PRBool ctxHasNonTranslation = ctx->CurrentMatrix().HasNonTranslation();
    if ((xscale == 1.0 && yscale == 1.0 && !ctxHasNonTranslation) ||
        subimageRect == gfxRect(0, 0, mWidth, mHeight))
    {
        
        
        
        
        pat = new gfxPattern(ThebesSurface());
    } else {
        
        
        gfxIntSize size(PRInt32(subimageRect.Width()),
                        PRInt32(subimageRect.Height()));
        nsRefPtr<gfxASurface> temp =
            gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, mFormat);
        if (!temp || temp->CairoStatus() != 0)
            return NS_ERROR_FAILURE;

        gfxContext tempctx(temp);
        tempctx.SetSource(ThebesSurface(), -subimageRect.pos);
        tempctx.SetOperator(gfxContext::OPERATOR_SOURCE);
        tempctx.Paint();

        pat = new gfxPattern(temp);
        srcRect.MoveBy(-subimageRect.pos);
    }

    



    if (aDestRect.pos.x * (1.0 / xscale) >= 32768.0 ||
        aDestRect.pos.y * (1.0 / yscale) >= 32768.0)
    {
        gfxIntSize dim(NS_lroundf(destRect.size.width),
                       NS_lroundf(destRect.size.height));

        
        if (dim.width == 0 || dim.height == 0)
            return NS_OK;

        nsRefPtr<gfxASurface> temp =
            gfxPlatform::GetPlatform()->CreateOffscreenSurface (dim,  mFormat);
        if (!temp || temp->CairoStatus() != 0)
            return NS_ERROR_FAILURE;

        gfxContext tempctx(temp);

        gfxMatrix mat;
        mat.Translate(srcRect.pos);
        mat.Scale(1.0 / xscale, 1.0 / yscale);
        pat->SetMatrix(mat);

        tempctx.SetPattern(pat);
        tempctx.SetOperator(gfxContext::OPERATOR_SOURCE);
        tempctx.NewPath();
        tempctx.Rectangle(gfxRect(0.0, 0.0, dim.width, dim.height));
        tempctx.Fill();

        pat = new gfxPattern(temp);

        srcRect.pos.x = 0.0;
        srcRect.pos.y = 0.0;
        srcRect.size.width = dim.width;
        srcRect.size.height = dim.height;

        xscale = 1.0;
        yscale = 1.0;
    }

    gfxMatrix mat;
    mat.Translate(srcRect.pos);
    mat.Scale(1.0/xscale, 1.0/yscale);

    


    mat.Translate(-destRect.pos);

    pat->SetMatrix(mat);

    nsRefPtr<gfxASurface> target = ctx->CurrentSurface();
    switch (target->GetType()) {
    case gfxASurface::SurfaceTypeXlib:
    case gfxASurface::SurfaceTypeXcb:
        
        
        
        
        
        
        
        
        
        if (xscale > 1.0 || yscale > 1.0 || ctxHasNonTranslation)
            pat->SetFilter(0);
        break;

    case gfxASurface::SurfaceTypeQuartz:
    case gfxASurface::SurfaceTypeQuartzImage:
        
        break;

    default:
        
        
        
        if (xscale != 1.0 || yscale != 1.0 || ctxHasNonTranslation)
            pat->SetExtend(gfxPattern::EXTEND_PAD);
        break;
    }

    gfxContext::GraphicsOperator op = ctx->CurrentOperator();
    if (op == gfxContext::OPERATOR_OVER && mFormat == gfxASurface::ImageFormatRGB24)
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);

    ctx->NewPath();
    ctx->SetPattern(pat);
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    ctx->Rectangle(destRect, PR_TRUE);
#else
    ctx->Rectangle(destRect);
#endif
    ctx->Fill();

    ctx->SetOperator(op);
    ctx->SetDeviceColor(gfxRGBA(0,0,0,0));

    return NS_OK;
}

nsresult
nsThebesImage::ThebesDrawTile(gfxContext *thebesContext,
                              nsIDeviceContext* dx,
                              const gfxPoint& offset,
                              const gfxRect& targetRect,
                              const nsIntRect& aSubimageRect,
                              const PRInt32 xPadding,
                              const PRInt32 yPadding)
{
    NS_ASSERTION(xPadding >= 0 && yPadding >= 0, "negative padding");

    if (targetRect.size.width <= 0.0 || targetRect.size.height <= 0.0)
        return NS_OK;

    
    if (mSinglePixel && mSinglePixelColor.a == 0.0)
        return NS_OK;

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    PRBool doSnap = PR_TRUE;
#else
    PRBool doSnap = !(thebesContext->CurrentMatrix().HasNonTranslation());
#endif
    PRBool hasPadding = ((xPadding != 0) || (yPadding != 0));
    gfxImageSurface::gfxImageFormat format = mFormat;
    
    gfxPoint tmpOffset = offset;

    if (mSinglePixel && !hasPadding) {
        thebesContext->SetDeviceColor(mSinglePixelColor);
    } else {
        nsRefPtr<gfxASurface> surface;
        PRInt32 width, height;

        if (hasPadding) {
            

            width = mWidth + xPadding;
            height = mHeight + yPadding;

            
            if (!AllowedImageSize(width, height))
                return NS_ERROR_FAILURE;

            format = gfxASurface::ImageFormatARGB32;
            surface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(
                    gfxIntSize(width, height), format);
            if (!surface || surface->CairoStatus()) {
                return NS_ERROR_OUT_OF_MEMORY;
            }

            gfxContext tmpContext(surface);
            if (mSinglePixel) {
                tmpContext.SetDeviceColor(mSinglePixelColor);
            } else {
                tmpContext.SetSource(ThebesSurface());
            }
            tmpContext.SetOperator(gfxContext::OPERATOR_SOURCE);
            tmpContext.Rectangle(gfxRect(0, 0, mWidth, mHeight));
            tmpContext.Fill();
        } else {
            width = mWidth;
            height = mHeight;
            surface = ThebesSurface();
        }
        
        
        
        
        gfxFloat scale = gfxFloat(dx->AppUnitsPerDevPixel()) /
                         gfxFloat(nsIDeviceContext::AppUnitsPerCSSPixel());

        if ((aSubimageRect.width < width || aSubimageRect.height < height) &&
            (thebesContext->CurrentMatrix().HasNonTranslation() || scale != 1.0)) {
            
            
            
            
            
            
            PRInt32 padX = aSubimageRect.width < width ? 1 : 0;
            PRInt32 padY = aSubimageRect.height < height ? 1 : 0;
            PRInt32 tileWidth = PR_MIN(aSubimageRect.width, width);
            PRInt32 tileHeight = PR_MIN(aSubimageRect.height, height);
            
            
            
            
            
            
            nsRefPtr<gfxASurface> tmpSurface;
            tmpSurface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(
                    gfxIntSize(tileWidth + 2*padX, tileHeight + 2*padY), format);
            if (!tmpSurface || tmpSurface->CairoStatus()) {
                return NS_ERROR_OUT_OF_MEMORY;
            }

            gfxContext tmpContext(tmpSurface);
            tmpContext.SetOperator(gfxContext::OPERATOR_SOURCE);
            gfxPattern pat(surface);
            pat.SetExtend(gfxPattern::EXTEND_REPEAT);
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            PRInt32 destY = 0;
            for (PRInt32 y = -1; y <= 1; ++y) {
                PRInt32 stripHeight = y == 0 ? tileHeight : padY;
                if (stripHeight == 0)
                    continue;
                PRInt32 srcY = y == 1 ? aSubimageRect.YMost() - padY : aSubimageRect.y;
                
                PRInt32 destX = 0;
                for (PRInt32 x = -1; x <= 1; ++x) {
                    PRInt32 stripWidth = x == 0 ? tileWidth : padX;
                    if (stripWidth == 0)
                        continue;
                    PRInt32 srcX = x == 1 ? aSubimageRect.XMost() - padX : aSubimageRect.x;

                    gfxMatrix patMat;
                    patMat.Translate(gfxPoint(srcX - destX, srcY - destY));
                    pat.SetMatrix(patMat);
                    tmpContext.SetPattern(&pat);
                    tmpContext.Rectangle(gfxRect(destX, destY, stripWidth, stripHeight));
                    tmpContext.Fill();
                    tmpContext.NewPath();
                    
                    destX += stripWidth;
                }
                destY += stripHeight;
            }

            
            
            
            
            tmpOffset += gfxPoint(aSubimageRect.x - padX, aSubimageRect.y - padY)/scale;
            
            surface = tmpSurface;
        }

        gfxMatrix patMat;
        gfxPoint p0;

        p0.x = - floor(tmpOffset.x + 0.5);
        p0.y = - floor(tmpOffset.y + 0.5);
        patMat.Scale(scale, scale);
        patMat.Translate(p0);

        gfxPattern pat(surface);
        pat.SetExtend(gfxPattern::EXTEND_REPEAT);
        pat.SetMatrix(patMat);

#ifndef XP_MACOSX
        if (scale < 1.0) {
            
            
            pat.SetFilter(0);
        }
#endif

        thebesContext->SetPattern(&pat);
    }

    gfxContext::GraphicsOperator op = thebesContext->CurrentOperator();
    if (op == gfxContext::OPERATOR_OVER && format == gfxASurface::ImageFormatRGB24)
        thebesContext->SetOperator(gfxContext::OPERATOR_SOURCE);

    thebesContext->NewPath();
    thebesContext->Rectangle(targetRect, doSnap);
    thebesContext->Fill();

    thebesContext->SetOperator(op);
    thebesContext->SetDeviceColor(gfxRGBA(0,0,0,0));

    return NS_OK;
}

PRBool
nsThebesImage::ShouldUseImageSurfaces()
{
#ifdef XP_WIN
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
    }
}
