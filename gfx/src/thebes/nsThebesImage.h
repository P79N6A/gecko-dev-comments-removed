





































#ifndef _NSTHEBESIMAGE_H_
#define _NSTHEBESIMAGE_H_

#include "nsIImage.h"

#include "gfxColor.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#ifdef XP_WIN
#include "gfxWindowsSurface.h"
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
    virtual void ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect);
    virtual nsresult Optimize(nsIDeviceContext* aContext);
    virtual nsColorMap *GetColorMap();

    NS_IMETHOD Draw(nsIRenderingContext &aContext,
                    const gfxRect &aSourceRect,
                    const gfxRect &aDestRect);
    NS_IMETHOD DrawTile(nsIRenderingContext &aContext,
                        nsIDrawingSurface *aSurface,
                        PRInt32 aSXOffset, PRInt32 aSYOffset,
                        PRInt32 aPadX, PRInt32 aPadY,
                        const nsRect &aTileRect);
    NS_IMETHOD DrawToImage(nsIImage* aDstImage,
                           PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

    nsresult ThebesDrawTile(gfxContext *thebesContext,
                            nsIDeviceContext* dx,
                            const gfxPoint& aOffset,
                            const gfxRect& aTileRect,
                            const PRInt32 aXPadding,
                            const PRInt32 aYPadding);

    virtual PRInt8 GetAlphaDepth();
    virtual void* GetBitInfo();
    NS_IMETHOD LockImagePixels(PRBool aMaskPixels);
    NS_IMETHOD UnlockImagePixels(PRBool aMaskPixels);

    NS_IMETHOD GetSurface(gfxASurface **aSurface) {
        *aSurface = ThebesSurface();
        NS_ADDREF(*aSurface);
        return NS_OK;
    }

    gfxASurface* ThebesSurface() {
        if (mOptSurface)
            return mOptSurface;

        return mImageSurface;
    }

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
    nsRect mDecoded;
    PRPackedBool mImageComplete;
    PRPackedBool mSinglePixel;

    gfxRGBA mSinglePixelColor;

    nsRefPtr<gfxImageSurface> mImageSurface;
    nsRefPtr<gfxASurface> mOptSurface;
#ifdef XP_WIN
    nsRefPtr<gfxWindowsSurface> mWinSurface;
#endif

    PRUint8 mAlphaDepth;

    
    
    
    
    static PRBool ShouldUseImageSurfaces();
};

#endif 
