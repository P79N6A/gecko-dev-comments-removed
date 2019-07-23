




































#ifndef nsImageGTK_h___
#define nsImageGTK_h___

#include "nsIImage.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gdk/gdk.h>
#include "nsRegion.h"
#ifdef MOZ_WIDGET_GTK2
#include "nsIGdkPixbufImage.h"
#endif

class nsDrawingSurfaceGTK;

class nsImageGTK :
#ifdef MOZ_WIDGET_GTK2
                   public nsIGdkPixbufImage
#else
                   public nsIImage
#endif
{
public:
  nsImageGTK();
  virtual ~nsImageGTK();

  static void Startup();
  static void Shutdown();

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

  virtual PRBool      GetHasAlphaMask()     { return mAlphaBits != nsnull || mAlphaPixmap != nsnull; }
  virtual PRUint8*    GetAlphaBits();
  virtual PRInt32     GetAlphaLineStride();

  




  virtual PRInt8 GetAlphaDepth() { 
    if (mTrueAlphaBits)
      return mTrueAlphaDepth;
    else
      return mAlphaDepth;
  }

  NS_IMETHOD   LockImagePixels(PRBool aMaskPixels);
  NS_IMETHOD   UnlockImagePixels(PRBool aMaskPixels);    

#ifdef MOZ_WIDGET_GTK2
  NS_IMETHOD_(GdkPixbuf*) GetGdkPixbuf();
#endif

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

  static unsigned scaled6[1<<6];
  static unsigned scaled5[1<<5];

  void DrawComposited32(PRBool isLSB, PRBool flipBytes,
                        PRUint8 *imageOrigin, PRUint32 imageStride,
                        PRUint8 *alphaOrigin, PRUint32 alphaStride,
                        unsigned width, unsigned height,
                        XImage *ximage, unsigned char *readData, unsigned char *srcData);
  void DrawComposited24(PRBool isLSB, PRBool flipBytes,
                        PRUint8 *imageOrigin, PRUint32 imageStride,
                        PRUint8 *alphaOrigin, PRUint32 alphaStride,
                        unsigned width, unsigned height,
                        XImage *ximage, unsigned char *readData, unsigned char *srcData);
  void DrawComposited16(PRBool isLSB, PRBool flipBytes,
                        PRUint8 *imageOrigin, PRUint32 imageStride,
                        PRUint8 *alphaOrigin, PRUint32 alphaStride,
                        unsigned width, unsigned height,
                        XImage *ximage, unsigned char *readData, unsigned char *srcData);
  void DrawCompositedGeneral(PRBool isLSB, PRBool flipBytes,
                             PRUint8 *imageOrigin, PRUint32 imageStride,
                             PRUint8 *alphaOrigin, PRUint32 alphaStride,
                             unsigned width, unsigned height,
                             XImage *ximage, unsigned char *readData, unsigned char *srcData);
  inline void DrawComposited(nsIRenderingContext &aContext,
                             nsIDrawingSurface* aSurface,
                             PRInt32 srcWidth, PRInt32 srcHeight,
                             PRInt32 dstWidth, PRInt32 dstHeight,
                             PRInt32 dstOrigX, PRInt32 dstOrigY,
                             PRInt32 aDX, PRInt32 aDY,
                             PRInt32 aDWidth, PRInt32 aDHeight);
  inline void DrawCompositeTile(nsIRenderingContext &aContext,
                                nsIDrawingSurface* aSurface,
                                PRInt32 aSX, PRInt32 aSY,
                                PRInt32 aSWidth, PRInt32 aSHeight,
                                PRInt32 aDX, PRInt32 aDY,
                                PRInt32 aDWidth, PRInt32 aDHeight);

  inline void TilePixmap(GdkPixmap *src, GdkPixmap *dest, PRInt32 aSXOffset, PRInt32 aSYOffset, 
                         const nsRect &destRect, const nsRect &clipRect, PRBool useClip);
  inline void CreateOffscreenPixmap(PRInt32 aWidth, PRInt32 aHeight);
  inline void SetupGCForAlpha(GdkGC *aGC, PRInt32 aX, PRInt32 aY);

  void SlowTile(nsDrawingSurfaceGTK *aSurface, const nsRect &aTileRect,
                PRInt32 aSXOffset, PRInt32 aSYOffset, const nsRect& aRect, PRBool aIsValid);

  PRUint8      *mImageBits;
  GdkPixmap    *mImagePixmap;
  PRUint8      *mTrueAlphaBits;
  PRUint8      *mAlphaBits;
  GdkPixmap    *mAlphaPixmap;
  XImage       *mAlphaXImage;

  PRInt32       mWidth;
  PRInt32       mHeight;
  PRInt32       mRowBytes;
  PRInt32       mSizeImage;

  PRInt32       mDecodedX1;         
  PRInt32       mDecodedY1;         
  PRInt32       mDecodedX2;
  PRInt32       mDecodedY2;

  nsRegion      mUpdateRegion;

  
  PRInt32       mAlphaRowBytes;     
  PRInt32       mTrueAlphaRowBytes; 
  PRInt8        mAlphaDepth;        
  PRInt8        mTrueAlphaDepth;    
  PRPackedBool  mIsSpacer;
  PRPackedBool  mPendingUpdate;

  PRInt8        mNumBytesPixel;
  PRUint8       mFlags;             
  PRInt8        mDepth;             

  PRPackedBool  mOptimized;
};

#endif
