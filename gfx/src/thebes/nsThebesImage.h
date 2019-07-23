





































#ifndef _NSTHEBESIMAGE_H_
#define _NSTHEBESIMAGE_H_

#include "nsIImage.h"

#include "gfxColor.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPattern.h"
#if defined(XP_WIN)
#include "gfxWindowsSurface.h"
#elif defined(XP_MACOSX)
#include "gfxQuartzImageSurface.h"
#endif

class nsThebesImage : public nsIImage
{
public:
    nsThebesImage();
    ~nsThebesImage();

    NS_DECL_ISUPPORTS

    virtual nsresult Init(PRInt32 aWidth, PRInt32 aHeight,
                          PRInt32 aDepth, nsMaskRequirements aMaskRequirements);
    virtual PRInt32 GetBytesPix();
    virtual PRBool GetIsRowOrderTopToBottom();
    virtual PRInt32 GetWidth();
    virtual PRInt32 GetHeight();
    virtual PRUint8 *GetBits();
    virtual PRInt32 GetLineStride();
    virtual PRBool GetHasAlphaMask();
    virtual PRUint8 *GetAlphaBits();
    virtual PRInt32 GetAlphaLineStride();
    virtual PRBool GetIsImageComplete();
    virtual nsresult ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsIntRect *aUpdateRect);
    virtual nsresult Optimize(nsIDeviceContext* aContext);
    virtual nsColorMap *GetColorMap();

    virtual void Draw(gfxContext*        aContext,
                      gfxPattern::GraphicsFilter aFilter,
                      const gfxMatrix&   aUserSpaceToImageSpace,
                      const gfxRect&     aFill,
                      const nsIntMargin& aPadding,
                      const nsIntRect&   aSubimage);

    virtual PRInt8 GetAlphaDepth();
    virtual void* GetBitInfo();
    NS_IMETHOD LockImagePixels(PRBool aMaskPixels);
    NS_IMETHOD UnlockImagePixels(PRBool aMaskPixels);

    NS_IMETHOD GetSurface(gfxASurface **aSurface) {
        *aSurface = ThebesSurface();
        NS_ADDREF(*aSurface);
        return NS_OK;
    }

    NS_IMETHOD GetPattern(gfxPattern **aPattern) {
        if (mSinglePixel)
            *aPattern = new gfxPattern(mSinglePixelColor);
        else
            *aPattern = new gfxPattern(ThebesSurface());
        NS_ADDREF(*aPattern);
        return NS_OK;
    }

    gfxASurface* ThebesSurface() {
        if (mOptSurface)
            return mOptSurface;
#if defined(XP_WIN) && !defined(WINCE)
        if (mWinSurface)
            return mWinSurface;
#elif defined(XP_MACOSX)
        if (mQuartzSurface)
            return mQuartzSurface;
#endif
        return mImageSurface;
    }

    void SetHasNoAlpha();

    NS_IMETHOD Extract(const nsIntRect& aSubimage,
                       nsIImage** aResult NS_OUTPARAM);

protected:
    static PRBool AllowedImageSize(PRInt32 aWidth, PRInt32 aHeight) {
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
        return PR_TRUE;
    }

    gfxImageSurface::gfxImageFormat mFormat;
    PRInt32 mWidth;
    PRInt32 mHeight;
    PRInt32 mStride;
    nsIntRect mDecoded;
    PRPackedBool mImageComplete;
    PRPackedBool mSinglePixel;
    PRPackedBool mFormatChanged;
    PRPackedBool mNeverUseDeviceSurface;
#if defined(XP_WIN) && !defined(WINCE)
    PRPackedBool mIsDDBSurface;
#endif

    gfxRGBA mSinglePixelColor;

    nsRefPtr<gfxImageSurface> mImageSurface;
    nsRefPtr<gfxASurface> mOptSurface;
#if defined(XP_WIN) && !defined(WINCE)
    nsRefPtr<gfxWindowsSurface> mWinSurface;
#elif defined(XP_MACOSX)
    nsRefPtr<gfxQuartzImageSurface> mQuartzSurface;
#endif

    PRUint8 mAlphaDepth;

    
    
    
    
    static PRBool ShouldUseImageSurfaces();
};

#endif 
