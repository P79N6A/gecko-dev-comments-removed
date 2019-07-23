




































#ifndef nsImagePh_h___
#define nsImagePh_h___

#include <Pt.h>
#include "nsIImage.h"
#include "nsRect.h"

class nsImagePh : public nsIImage {
public:
  nsImagePh();
  virtual ~nsImagePh();

  NS_DECL_ISUPPORTS

  


  virtual PRInt32     GetBytesPix()       { return mNumBytesPixel; }
  virtual PRInt32     GetHeight() { return mHeight; }
  virtual PRInt32     GetWidth() { return mWidth; }

  virtual PRUint8*    GetBits() {  return mImageBits; }

  virtual void*       GetBitInfo() { return nsnull; }

  virtual PRBool      GetIsRowOrderTopToBottom() { return PR_TRUE; }
  virtual PRInt32     GetLineStride() { return mRowBytes; }

	inline
  NS_IMETHODIMP       SetDecodedRect(PRInt32 x1, PRInt32 y1, PRInt32 x2, PRInt32 y2)
		{
		mDecodedX1 = x1;
		mDecodedY1 = y1;
		mDecodedX2 = x2;
		mDecodedY2 = y2;
		return NS_OK;
		}

  virtual PRInt32     GetDecodedX1() { return mDecodedX1;}
  virtual PRInt32     GetDecodedY1() { return mDecodedY1;}
  virtual PRInt32     GetDecodedX2() { return mDecodedX2;}
  virtual PRInt32     GetDecodedY2() { return mDecodedY2;}

  NS_IMETHOD     SetNaturalWidth(PRInt32 naturalwidth) { mNaturalWidth= naturalwidth; return NS_OK;}
  NS_IMETHOD     SetNaturalHeight(PRInt32 naturalheight) { mNaturalHeight= naturalheight; return NS_OK;}
  virtual PRInt32     GetNaturalWidth() {return mNaturalWidth; }
  virtual PRInt32     GetNaturalHeight() {return mNaturalHeight; }


  virtual nsColorMap* GetColorMap() { return nsnull; }

  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth,  \
  						PRInt32 aSHeight, PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

  NS_IMETHOD DrawToImage(nsIImage* aDstImage, nscoord aDX, nscoord aDY,
             nscoord aDWidth, nscoord aDHeight);

  NS_IMETHOD 		  DrawTile(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                        PRInt32 aSXOffset, PRInt32 aSYOffset, PRInt32 aPadX, PRInt32 aPadY,
												const nsRect &aTileRect);

  virtual void        ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect)
		{
		PRInt32 y = aUpdateRect->YMost();
		PRInt32 x = aUpdateRect->XMost();
		if( y > mDecodedY2 ) mDecodedY2 = y;
		if( x > mDecodedX2 ) mDecodedX2 = x;
		mDirtyFlags = aFlags;
		mPhImage.size.h = mDecodedY2;
		}
  virtual PRBool      GetIsImageComplete();

  virtual nsresult    Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements);


  virtual nsresult    Optimize(nsIDeviceContext* aContext);

  virtual PRBool      GetHasAlphaMask() { return mAlphaBits != nsnull; } 
  virtual PRUint8*    GetAlphaBits() { return mAlphaBits; }
  virtual PRInt32     GetAlphaWidth() { return mAlphaWidth; }
  virtual PRInt32     GetAlphaHeight() { return mAlphaHeight; }
  virtual PRInt32     GetAlphaLineStride() { return mAlphaRowBytes; }
  virtual nsIImage*   DuplicateImage() { return nsnull; }
  
  




  virtual PRInt8 GetAlphaDepth() { return mAlphaDepth; }  
  virtual void  	  MoveAlphaMask(PRInt32 aX, PRInt32 aY) { }

  inline
	NS_IMETHODIMP 	  LockImagePixels(PRBool aMaskPixels) { return NS_OK; }

	inline
  NS_IMETHODIMP 	  UnlockImagePixels(PRBool aMaskPixels) { return NS_OK; }

private:
  void ComputePaletteSize(PRIntn nBitCount);

private:
  PRInt32             mWidth;
  PRInt32             mHeight;
  PRInt32             mDepth;
  PRInt32             mRowBytes;          
  PRUint8*            mImageBits;         

  PRInt8              mNumBytesPixel;     
  PRUint8             mImageFlags;
	PRUint8							mDirtyFlags;

  PRInt32             mDecodedX1;       
  PRInt32             mDecodedY1;       
  PRInt32             mDecodedX2; 
  PRInt32             mDecodedY2;    

  
  PRUint8             *mAlphaBits;        
  PRInt8              mAlphaDepth;        
  PRInt16             mAlphaRowBytes;         
  PRInt16             mAlphaWidth;        
  PRInt16             mAlphaHeight;       
  PhImage_t           mPhImage;
  PhImage_t           *mPhImageZoom;			
	PRInt32							mDecodedY2_when_scaled;

  PRInt32 mNaturalWidth;
  PRInt32 mNaturalHeight;
};
#endif
