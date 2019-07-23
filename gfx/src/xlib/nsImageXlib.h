





































#ifndef nsImageXlib_h__
#define nsImageXlib_h__

#include "nsIImage.h"
#include "nsPoint.h"
#include "nsGCCache.h"
#include "nsRegion.h"
#include "xlibrgb.h"

class nsImageXlib : public nsIImage {
public:
  nsImageXlib();
  virtual ~nsImageXlib();

  NS_DECL_ISUPPORTS

  virtual PRInt32     GetBytesPix()       { return mNumBytesPixel; }
  virtual PRInt32     GetHeight();
  virtual PRInt32     GetWidth();
  virtual PRUint8*    GetBits();
  virtual void*       GetBitInfo();
  virtual PRBool      GetIsRowOrderTopToBottom() { return PR_TRUE; }
  virtual PRInt32     GetLineStride();

  virtual nsColorMap* GetColorMap();

  NS_IMETHOD Draw(nsIRenderingContext &aContext,
                  nsIDrawingSurface* aSurface,
                  PRInt32 aX, PRInt32 aY,
                  PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD Draw(nsIRenderingContext &aContext,
                  nsIDrawingSurface* aSurface,
                  PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                  PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

  NS_IMETHOD DrawToImage(nsIImage* aDstImage, nscoord aDX, nscoord aDY,
                         nscoord aDWidth, nscoord aDHeight);

  NS_IMETHOD DrawTile(nsIRenderingContext &aContext,
                      nsIDrawingSurface* aSurface,
                      PRInt32 aSXOffset, PRInt32 aSYOffset,
                      PRInt32 aPadX, PRInt32 aPadY,
                      const nsRect &aTileRect);

  void UpdateCachedImage();
  virtual void ImageUpdated(nsIDeviceContext *aContext,
                            PRUint8 aFlags, nsRect *aUpdateRect);
  virtual PRBool      GetIsImageComplete();
  virtual nsresult    Init(PRInt32 aWidth, PRInt32 aHeight,
                           PRInt32 aDepth,
                           nsMaskRequirements aMaskRequirements);

  virtual nsresult    Optimize(nsIDeviceContext* aContext);

  virtual PRBool      GetHasAlphaMask()     { return mAlphaBits != nsnull; }     
  virtual PRUint8*    GetAlphaBits();
  virtual PRInt32     GetAlphaLineStride();
  




  virtual PRInt8 GetAlphaDepth() {return(mAlphaDepth);}  

  NS_IMETHOD   LockImagePixels(PRBool aMaskPixels);
  NS_IMETHOD   UnlockImagePixels(PRBool aMaskPixels);

private:
  


  void ComputeMetrics() {
    mRowBytes = (mWidth * mDepth) >> 5;

    if (((PRUint32)mWidth * mDepth) & 0x1F)
      mRowBytes++;
    mRowBytes <<= 2;

    mSizeImage = mRowBytes * mHeight;
  };
  void ComputePaletteSize(PRIntn nBitCount);

private:
  NS_IMETHODIMP DrawScaled(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                           PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                           PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

  static unsigned scaled6[1<<6];
  static unsigned scaled5[1<<5];

  void DrawComposited32(PRBool isLSB, PRBool flipBytes,
                        PRUint8 *imageOrigin, PRUint32 imageStride,
                        PRUint8 *alphaOrigin, PRUint32 alphaStride,
                        unsigned width, unsigned height,
                        XImage *ximage, unsigned char *readData);
  void DrawComposited24(PRBool isLSB, PRBool flipBytes,
                        PRUint8 *imageOrigin, PRUint32 imageStride,
                        PRUint8 *alphaOrigin, PRUint32 alphaStride,
                        unsigned width, unsigned height,
                        XImage *ximage, unsigned char *readData);
  void DrawComposited16(PRBool isLSB, PRBool flipBytes,
                        PRUint8 *imageOrigin, PRUint32 imageStride,
                        PRUint8 *alphaOrigin, PRUint32 alphaStride,
                        unsigned width, unsigned height,
                        XImage *ximage, unsigned char *readData);
  void DrawCompositedGeneral(PRBool isLSB, PRBool flipBytes,
                             PRUint8 *imageOrigin, PRUint32 imageStride,
                             PRUint8 *alphaOrigin, PRUint32 alphaStride,
                             unsigned width, unsigned height,
                             XImage *ximage, unsigned char *readData);
  inline void DrawComposited(nsIRenderingContext &aContext,
                             nsIDrawingSurface* aSurface,
                             PRInt32 aSX, PRInt32 aSY,
                             PRInt32 aSWidth, PRInt32 aSHeight,
                             PRInt32 aDX, PRInt32 aDY,
                             PRInt32 aDWidth, PRInt32 aDHeight);

  inline void TilePixmap(Pixmap src, Pixmap dest, PRInt32 aSXOffset, PRInt32 aSYOffset,
                         const nsRect &destRect, const nsRect &clipRect, PRBool useClip);
  inline void CreateAlphaBitmap(PRInt32 aWidth, PRInt32 aHeight);
  inline void CreateOffscreenPixmap(PRInt32 aWidth, PRInt32 aHeight);
  inline void DrawImageOffscreen(PRInt32 aSX, PRInt32 aSY,
                                 PRInt32 aWidth, PRInt32 aHeight);
  inline void SetupGCForAlpha(GC aGC, PRInt32 aX, PRInt32 aY);

  
  PRUint8      *mImageBits;
  PRUint8      *mAlphaBits;
  Pixmap        mImagePixmap;
  Pixmap        mAlphaPixmap;

  PRInt32       mWidth;
  PRInt32       mHeight;
  PRInt32       mDepth;       
  PRInt32       mRowBytes;
  GC            mGC;

  PRInt32       mSizeImage;
  PRInt8        mNumBytesPixel;

  PRInt32       mDecodedX1;       
  PRInt32       mDecodedY1;       
  PRInt32       mDecodedX2;
  PRInt32       mDecodedY2;

  nsRegion      mUpdateRegion;

  static XlibRgbHandle *mXlibRgbHandle;
  static Display       *mDisplay;

  
  PRInt8        mAlphaDepth;        
  PRInt16       mAlphaRowBytes;     
  PRPackedBool  mAlphaValid;
  PRPackedBool  mIsSpacer;
  PRPackedBool  mPendingUpdate;

  PRUint8       mFlags;             
};

#endif 
