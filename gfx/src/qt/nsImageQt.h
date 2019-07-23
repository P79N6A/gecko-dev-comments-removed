







































#ifndef nsImageQt_h___
#define nsImageQt_h___

#include "nsIImage.h"
#include "nsRect.h"

#include <qpixmap.h>

#undef Bool

class nsImageQt : public nsIImage
{
public:
    nsImageQt();
    virtual ~nsImageQt();

    NS_DECL_ISUPPORTS

    


    virtual nsresult    Init(PRInt32 aWidth, PRInt32 aHeight,
                             PRInt32 aDepth, nsMaskRequirements aMaskRequirements);

    virtual PRInt32     GetBytesPix();
    virtual PRBool      GetIsRowOrderTopToBottom() {return PR_TRUE;}
    virtual PRInt32     GetHeight();
    virtual PRInt32     GetWidth();
    virtual PRUint8     *GetBits();
    virtual PRInt32     GetLineStride();
    virtual PRBool      GetHasAlphaMask() {return mAlphaBits != nsnull;}
    virtual PRUint8     *GetAlphaBits();
    virtual PRInt32     GetAlphaLineStride();

    virtual void ImageUpdated(nsIDeviceContext *aContext,
                              PRUint8 aFlags,nsRect *aUpdateRect);
    virtual PRBool      GetIsImageComplete();

    virtual nsresult    Optimize(nsIDeviceContext *aContext);

    virtual nsColorMap *GetColorMap();

    NS_IMETHOD Draw(nsIRenderingContext &aContext,
                    nsIDrawingSurface* aSurface,
                    PRInt32 aX, PRInt32 aY,
                    PRInt32 aWidth, PRInt32 aHeight);

    NS_IMETHOD Draw(nsIRenderingContext &aContext,
                    nsIDrawingSurface *aSurface,
                    PRInt32 aSX, PRInt32 aSY,
                    PRInt32 aSWidth, PRInt32 aSHeight,
                    PRInt32 aDX, PRInt32 aDY,
                    PRInt32 aDWidth, PRInt32 aDHeight);

    NS_IMETHOD DrawTile(nsIRenderingContext &aContext,
                        nsIDrawingSurface* aSurface,
                        PRInt32 aSXOffset, PRInt32 aSYOffset,
                        PRInt32 aPadX, PRInt32 aPadY,
                        const nsRect &aTileRect);

    NS_IMETHOD DrawToImage(nsIImage *aDstImage, PRInt32 aDX, PRInt32 aDY,
                           PRInt32 aDWidth, PRInt32 aDHeight);

    virtual PRInt8 GetAlphaDepth() { return(mAlphaDepth); }

    virtual void        *GetBitInfo();

    NS_IMETHOD   LockImagePixels(PRBool aMaskPixels);

    NS_IMETHOD   UnlockImagePixels(PRBool aMaskPixels);

private:
    



    void updatePixmap();

private:
    PRInt32   mWidth;
    PRInt32   mHeight;
    PRInt32   mDepth;
    PRInt32   mRowBytes;
    PRUint8   *mImageBits;

    PRInt8    mAlphaDepth;    
    PRInt16   mAlphaRowBytes; 
    PRUint8   *mAlphaBits;


    PRInt8    mNumBytesPixel;

    nsRect    mDecodedRect;   

    QPixmap pixmap;
    PRBool pixmapDirty;
};

#endif
