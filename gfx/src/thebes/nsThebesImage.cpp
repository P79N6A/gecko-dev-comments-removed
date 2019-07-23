





































#include "nsThebesImage.h"
#include "nsThebesRenderingContext.h"

#include "gfxContext.h"
#include "gfxPattern.h"

#include "gfxPlatform.h"

#include "prenv.h"

static PRBool gDisableOptimize = PR_FALSE;

NS_IMPL_ISUPPORTS1(nsThebesImage, nsIImage)

nsThebesImage::nsThebesImage()
    : mFormat(gfxImageSurface::ImageFormatRGB24),
      mWidth(0),
      mHeight(0),
      mDecoded(0,0,0,0),
      mImageComplete(PR_FALSE),
      mSinglePixel(PR_FALSE),
      mAlphaDepth(0)
{
    static PRBool hasCheckedOptimize = PR_FALSE;
    if (!hasCheckedOptimize) {
        if (PR_GetEnv("MOZ_DISABLE_IMAGE_OPTIMIZE")) {
            gDisableOptimize = PR_TRUE;
        }
        hasCheckedOptimize = PR_TRUE;
    }
}

nsresult
nsThebesImage::Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements)
{
    mWidth = aWidth;
    mHeight = aHeight;

    
    if (!AllowedImageSize(aWidth, aHeight))
        return NS_ERROR_FAILURE;

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

    if (!mImageSurface) {
        mWinSurface = nsnull;
        mImageSurface = new gfxImageSurface(gfxIntSize(mWidth, mHeight), format);
    }
#else
    mImageSurface = new gfxImageSurface(gfxIntSize(mWidth, mHeight), format);
#endif

    if (!mImageSurface || mImageSurface->CairoStatus()) {
        mImageSurface = nsnull;
        
        return NS_ERROR_OUT_OF_MEMORY;
    }

    mStride = mImageSurface->Stride();

    return NS_OK;
}

nsThebesImage::~nsThebesImage()
{
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

void
nsThebesImage::ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect)
{
    mDecoded.UnionRect(mDecoded, *aUpdateRect);
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

    if (mWidth == 1 && mHeight == 1) {
        
        if (mFormat == gfxImageSurface::ImageFormatARGB32 ||
            mFormat == gfxImageSurface::ImageFormatRGB24)
        {
            PRUint32 pixel = *((PRUint32 *) mImageSurface->Data());

            mSinglePixelColor = gfxRGBA
                (pixel,
                 (mFormat == gfxImageSurface::ImageFormatRGB24 ?
                  gfxRGBA::PACKED_XRGB :
                  gfxRGBA::PACKED_ARGB_PREMULTIPLIED));

            mSinglePixel = PR_TRUE;

            return NS_OK;
        }

        
        
    }

    
    
    if (ShouldUseImageSurfaces())
        return NS_OK;

#ifdef XP_WIN
    
    
    
    if (mWinSurface) {
        
        
        
        
        if (mWidth <= 1024 && mHeight <= 1024) {
            nsRefPtr<gfxWindowsSurface> wsurf = mWinSurface->OptimizeToDDB(nsnull, gfxIntSize(mWidth, mHeight), mFormat);
            if (wsurf) {
                mOptSurface = wsurf;
            }
        }

        if (!mOptSurface) {
            
            mOptSurface = mWinSurface;
        }
    } else {
        mOptSurface = gfxPlatform::GetPlatform()->OptimizeImage(mImageSurface);
    }

    mWinSurface = nsnull;
#else
    mOptSurface = gfxPlatform::GetPlatform()->OptimizeImage(mImageSurface);
#endif

    mImageSurface = nsnull;

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
        nsRefPtr<gfxContext> context = new gfxContext(mImageSurface);
        if (!context) {
            mImageSurface = nsnull;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        context->SetOperator(gfxContext::OPERATOR_SOURCE);
        if (mSinglePixel)
            context->SetColor(mSinglePixelColor);
        else
            context->SetSource(mOptSurface);
        context->Paint();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsThebesImage::UnlockImagePixels(PRBool aMaskPixels)
{
    if (aMaskPixels)
        return NS_ERROR_NOT_IMPLEMENTED;
    if (mImageSurface && mOptSurface) {
        
        mImageSurface = nsnull;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsThebesImage::Draw(nsIRenderingContext &aContext,
                    const gfxRect &aSourceRect,
                    const gfxRect &aDestRect)
{
    if (NS_UNLIKELY(aDestRect.IsEmpty())) {
        NS_ERROR("nsThebesImage::Draw zero dest size - please fix caller.");
        return NS_OK;
    }

    nsThebesRenderingContext *thebesRC = NS_STATIC_CAST(nsThebesRenderingContext*, &aContext);
    gfxContext *ctx = thebesRC->Thebes();

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

        
        ctx->SetColor(mSinglePixelColor);
        ctx->NewPath();
        ctx->Rectangle(aDestRect, PR_TRUE);
        ctx->Fill();
        return NS_OK;
    }

    gfxFloat xscale = aDestRect.size.width / aSourceRect.size.width;
    gfxFloat yscale = aDestRect.size.height / aSourceRect.size.height;

    gfxRect srcRect(aSourceRect);
    gfxRect destRect(aDestRect);

    if (!GetIsImageComplete()) {
      srcRect = srcRect.Intersect(gfxRect(mDecoded.x, mDecoded.y,
                                          mDecoded.width, mDecoded.height));

      
      if (NS_UNLIKELY(srcRect.size.width == 0 || srcRect.size.height == 0))
          return NS_OK;

      destRect.pos.x += (srcRect.pos.x - aSourceRect.pos.x)*xscale;
      destRect.pos.y += (srcRect.pos.y - aSourceRect.pos.y)*yscale;

      
      destRect.size.width  = (srcRect.size.width)*xscale + 1 - xscale;
      destRect.size.height = (srcRect.size.height)*yscale + 1 - yscale;
    }

    
    if (!AllowedImageSize(destRect.size.width + 1, destRect.size.height + 1))
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxPattern> pat;

    



    if (aDestRect.pos.x * (1.0 / xscale) >= 32768.0 ||
        aDestRect.pos.y * (1.0 / yscale) >= 32768.0)
    {
        gfxIntSize dim(NS_lroundf(destRect.size.width),
                       NS_lroundf(destRect.size.height));
        nsRefPtr<gfxASurface> temp =
            gfxPlatform::GetPlatform()->CreateOffscreenSurface (dim,  mFormat);
        nsRefPtr<gfxContext> tempctx = new gfxContext(temp);

        nsRefPtr<gfxPattern> srcpat = new gfxPattern(ThebesSurface());
        gfxMatrix mat;
        mat.Translate(srcRect.pos);
        mat.Scale(1.0 / xscale, 1.0 / yscale);
        srcpat->SetMatrix(mat);

        tempctx->SetPattern(srcpat);
        tempctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        tempctx->NewPath();
        tempctx->Rectangle(gfxRect(0.0, 0.0, dim.width, dim.height));
        tempctx->Fill();

        pat = new gfxPattern(temp);

        srcRect.pos.x = 0.0;
        srcRect.pos.y = 0.0;
        srcRect.size.width = dim.width;
        srcRect.size.height = dim.height;

        xscale = 1.0;
        yscale = 1.0;
    }

    if (!pat) {
        pat = new gfxPattern(ThebesSurface());
    }

    gfxMatrix mat;
    mat.Translate(srcRect.pos);
    mat.Scale(1.0/xscale, 1.0/yscale);

    


    mat.Translate(-destRect.pos);

    pat->SetMatrix(mat);

    
#ifndef XP_MACOSX
    if (xscale > 1.0 || yscale > 1.0) {
        
        
        
        
        
        
        
        
        pat->SetFilter(0);
    }
#endif

    ctx->NewPath();
    ctx->SetPattern(pat);
    ctx->Rectangle(destRect);
    ctx->Fill();

    return NS_OK;
}

nsresult
nsThebesImage::ThebesDrawTile(gfxContext *thebesContext,
                              nsIDeviceContext* dx,
                              const gfxPoint& offset,
                              const gfxRect& targetRect,
                              const PRInt32 xPadding,
                              const PRInt32 yPadding)
{
    NS_ASSERTION(xPadding >= 0 && yPadding >= 0, "negative padding");

    if (targetRect.size.width <= 0.0 || targetRect.size.height <= 0.0)
        return NS_OK;

    
    if (mSinglePixel && mSinglePixelColor.a == 0.0)
        return NS_OK;

    
    nsRefPtr<gfxPattern> pat;

    gfxMatrix savedCTM(thebesContext->CurrentMatrix());
    PRBool doSnap = !(savedCTM.HasNonTranslation());
    PRBool hasPadding = ((xPadding != 0) || (yPadding != 0));

    
    
    
    
    if (doSnap) {
        gfxMatrix roundedCTM(savedCTM);
        roundedCTM.x0 = ::floor(roundedCTM.x0 + 0.5);
        roundedCTM.y0 = ::floor(roundedCTM.y0 + 0.5);
        thebesContext->SetMatrix(roundedCTM);
    }

    nsRefPtr<gfxASurface> tmpSurfaceGrip;

    if (mSinglePixel && !hasPadding) {
        thebesContext->SetColor(mSinglePixelColor);
    } else {
        nsRefPtr<gfxASurface> surface;
        PRInt32 width, height;

        if (hasPadding) {
            

            width = mWidth + xPadding;
            height = mHeight + yPadding;

            
            if (!AllowedImageSize(width, height))
                return NS_ERROR_FAILURE;

            surface = new gfxImageSurface(gfxIntSize(width, height),
                                          gfxASurface::ImageFormatARGB32);
            if (!surface || surface->CairoStatus()) {
                thebesContext->SetMatrix(savedCTM);
                return NS_ERROR_OUT_OF_MEMORY;
            }

            tmpSurfaceGrip = surface;

            nsRefPtr<gfxContext> tmpContext = new gfxContext(surface);
            if (mSinglePixel) {
                tmpContext->SetColor(mSinglePixelColor);
            } else {
                tmpContext->SetSource(ThebesSurface());
            }
            tmpContext->SetOperator(gfxContext::OPERATOR_SOURCE);
            tmpContext->Rectangle(gfxRect(0, 0, mWidth, mHeight));
            tmpContext->Fill();
        } else {
            width = mWidth;
            height = mHeight;
            surface = ThebesSurface();
        }

        gfxMatrix patMat;
        gfxPoint p0;

        if (offset.x > width || offset.y > height) {
            p0.x = - floor(fmod(offset.x, gfxFloat(width)) + 0.5);
            p0.y = - floor(fmod(offset.y, gfxFloat(height)) + 0.5);
        } else {
            p0.x = - floor(offset.x + 0.5);
            p0.y = - floor(offset.y + 0.5);
        }
        
        
        
        gfxFloat scale = gfxFloat(nsIDeviceContext::AppUnitsPerCSSPixel()) / 
                         gfxFloat(dx->AppUnitsPerDevPixel());
        patMat.Scale(1.0 / scale, 1.0 / scale);
        patMat.Translate(p0);

        pat = new gfxPattern(surface);
        pat->SetExtend(gfxPattern::EXTEND_REPEAT);
        pat->SetMatrix(patMat);

#ifndef XP_MACOSX
        if (scale > 1.0) {
            
            
            pat->SetFilter(0);
        }
#endif

        thebesContext->SetPattern(pat);
    }

    thebesContext->NewPath();
    thebesContext->Rectangle(targetRect, doSnap);
    thebesContext->Fill();

    thebesContext->SetColor(gfxRGBA(0,0,0,0));
    if (doSnap)
        thebesContext->SetMatrix(savedCTM);

    return NS_OK;
}


NS_IMETHODIMP
nsThebesImage::DrawToImage(nsIImage* aDstImage, PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
    nsThebesImage *dstThebesImage = NS_STATIC_CAST(nsThebesImage*, aDstImage);

    nsRefPtr<gfxContext> dst = new gfxContext(dstThebesImage->ThebesSurface());

    dst->NewPath();
    
    
    
    dst->Translate(gfxPoint(aDX, aDY));
    dst->Rectangle(gfxRect(0, 0, aDWidth, aDHeight), PR_TRUE);
    dst->Scale(double(aDWidth)/mWidth, double(aDHeight)/mHeight);

    dst->SetSource(ThebesSurface());
    dst->Paint();

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
