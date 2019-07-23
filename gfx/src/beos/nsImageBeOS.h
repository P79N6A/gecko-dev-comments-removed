





































#ifndef nsImageBeOS_h___
#define nsImageBeOS_h___

#include "nsIImage.h"
#include "nsPoint.h"
class BBitmap;
class BView;
class nsImageBeOS : public nsIImage
{
public:
	nsImageBeOS();
	virtual ~nsImageBeOS();
	
	NS_DECL_ISUPPORTS

	
	virtual PRInt32 GetBytesPix() { return mNumBytesPixel; } 
	virtual PRInt32 GetHeight() { return mHeight; }
	virtual PRInt32 GetWidth() { return mWidth; }
	virtual PRUint8 *GetBits() { return mImageBits; }
	virtual void *GetBitInfo() { return nsnull; }
	virtual PRBool GetIsRowOrderTopToBottom() { return PR_TRUE; }
	virtual PRInt32 GetLineStride() { return mRowBytes; }
	
	virtual nsColorMap *GetColorMap() { return nsnull; }

	
	NS_IMETHOD Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
		PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
	NS_IMETHOD Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
		PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
		PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

	NS_IMETHOD DrawToImage(nsIImage *aDstImage, nscoord aDX, nscoord aDY,
		nscoord aDWidth, nscoord aDHeight);
 	
	NS_IMETHOD DrawTile(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, 
		PRInt32 aSXOffset, PRInt32 aSYOffset, PRInt32 aPadX, PRInt32 aPadY,
		const nsRect &aTileRect); 
	
	virtual void ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags,
		nsRect *aUpdateRect);
  virtual PRBool GetIsImageComplete();
	virtual nsresult Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
		nsMaskRequirements aMaskRequirements);

	virtual PRBool IsOptimized() { return mOptimized; }
	virtual nsresult Optimize(nsIDeviceContext *aContext);
	
	virtual PRBool GetHasAlphaMask() { return mAlphaBits != nsnull; }
	virtual PRUint8 *GetAlphaBits() { return mAlphaBits; }
	virtual PRInt32 GetAlphaLineStride() { return mAlphaRowBytes; }
	
	virtual PRInt8 GetAlphaDepth() { return mAlphaDepth; }
	
	NS_IMETHOD LockImagePixels(PRBool aMaskPixels);
	NS_IMETHOD UnlockImagePixels(PRBool aMaskPixels);

private:
	void ComputePaletteSize(PRIntn nBitCount);

protected:
	void CreateImage(nsIDrawingSurface* aSurface);
	nsresult BuildImage(nsIDrawingSurface* aDrawingSurface);
	
private:
	BBitmap *mImage;
	BBitmap *mTileBitmap;
	PRUint8 *mImageBits;
	PRInt32 mWidth;
	PRInt32 mHeight;
	PRInt32 mDepth;
	PRInt32 mRowBytes;
	PRInt32 mSizeImage;
	
	
	PRInt32 mDecodedX1;
	PRInt32 mDecodedY1;
	PRInt32 mDecodedX2;
	PRInt32 mDecodedY2;
	
	
	PRUint8 *mAlphaBits;
	PRInt16 mAlphaRowBytes;
	PRInt8 mAlphaDepth;
	
	
	PRUint8 mFlags;
	PRInt8 mNumBytesPixel;
	PRBool mImageCurrent;
	PRBool mOptimized;
};

#endif
