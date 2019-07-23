




































#ifndef nsImageMac_h___
#define nsImageMac_h___

#include "nsIImage.h"
#include "nsIImageMac.h"

class nsImageMac : public nsIImage, public nsIImageMac
{
public:
                      nsImageMac();
  virtual             ~nsImageMac();

  NS_DECL_ISUPPORTS

  


  virtual nsresult    Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements);
  virtual PRInt32     GetBytesPix()         { return mBytesPerPixel; }    
  virtual PRBool      GetIsRowOrderTopToBottom() { return PR_FALSE; }

  virtual PRInt32     GetWidth()            { return mWidth;  }
  virtual PRInt32     GetHeight()           { return mHeight; }

  virtual PRUint8*    GetBits()             { return mImageBits; }
  virtual PRInt32     GetLineStride()       { return mRowBytes; }
  virtual PRBool      GetHasAlphaMask()     { return mAlphaBits != nsnull; }

  virtual PRUint8*    GetAlphaBits()        { return mAlphaBits; }
  virtual PRInt32     GetAlphaLineStride()  { return mAlphaRowBytes; }

  
  
  virtual void        ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags,
                                   nsRect *aUpdateRect);
  virtual PRBool      GetIsImageComplete();

  
  virtual nsresult    Optimize(nsIDeviceContext* aContext);

  virtual nsColorMap* GetColorMap()         { return nsnull; }

  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                              PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);

  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                              PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                              PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

  NS_IMETHOD          DrawTile(nsIRenderingContext &aContext,
                              nsIDrawingSurface* aSurface,
                              PRInt32 aSXOffset, PRInt32 aSYOffset,
                              PRInt32 aPadX, PRInt32 aPadY,
                              const nsRect &aTileRect);


   




  virtual PRInt8      GetAlphaDepth() { return mAlphaDepth; }

  NS_IMETHOD          DrawToImage(nsIImage* aDstImage, nscoord aDX, nscoord aDY,
                                  nscoord aDWidth, nscoord aDHeight);

  virtual void*       GetBitInfo()          { return nsnull; }

  NS_IMETHOD          LockImagePixels(PRBool aMaskPixels);
  NS_IMETHOD          UnlockImagePixels(PRBool aMaskPixels);


  
  
  NS_IMETHOD          ConvertToPICT(PicHandle* outPicture);
  NS_IMETHOD          ConvertFromPICT(PicHandle inPicture);

  NS_IMETHOD          GetCGImageRef(CGImageRef* aCGImageRef);

protected:

  nsresult          SlowTile(nsIRenderingContext &aContext,
                                        nsIDrawingSurface* aSurface,
                                        PRInt32 aSXOffset, PRInt32 aSYOffset,
                                        PRInt32 aPadX, PRInt32 aPadY,
                                        const nsRect &aTileRect);

  nsresult          DrawTileQuickly(nsIRenderingContext &aContext,
                                        nsIDrawingSurface* aSurface,
                                        PRInt32 aSXOffset, PRInt32 aSYOffset,
                                        const nsRect &aTileRect);

  nsresult          DrawTileWithQuartz(nsIDrawingSurface* aSurface,
                                        PRInt32 aSXOffset, PRInt32 aSYOffset,
                                        PRInt32 aPadX, PRInt32 aPadY,
                                        const nsRect &aTileRect);
  
  static PRInt32    CalculateRowBytes(PRUint32 aWidth, PRUint32 aDepth);

  inline static PRInt32 CalculateRowBytesInternal(PRUint32 aWidth,
                                                  PRUint32 aDepth,
                                                  PRBool aAllow2Bytes);

  static PRBool     RenderingToPrinter(nsIRenderingContext &aContext);

  
  nsresult          EnsureCachedImage();

  
  inline PRUint8 GetAlphaBit(PRUint8* rowptr, PRUint32 x) {
    return (rowptr[x >> 3] & (1 << (7 - x & 0x7)));
  }
  inline void SetAlphaBit(PRUint8* rowptr, PRUint32 x) {
    rowptr[x >> 3] |= (1 << (7 - x & 0x7));
  }
  inline void ClearAlphaBit(PRUint8* rowptr, PRUint32 x) {
    rowptr[x >> 3] &= ~(1 << (7 - x & 0x7));
  }

  
  void AdoptImage(CGImageRef aNewImage, PRUint8* aNewBitamp);

private:

  PRUint8*        mImageBits;     
  CGImageRef      mImage;

  PRInt32         mWidth;
  PRInt32         mHeight;

  PRInt32         mRowBytes;
  PRInt32         mBytesPerPixel;

  
  PRUint8*        mAlphaBits;      

  PRInt32         mAlphaRowBytes;   
  PRInt8          mAlphaDepth;      

  PRPackedBool    mPendingUpdate;   
  PRPackedBool    mOptimized;       
                                    

  PRInt32         mDecodedX1;       
  PRInt32         mDecodedY1;       
  PRInt32         mDecodedX2;
  PRInt32         mDecodedY2;
};

#endif
