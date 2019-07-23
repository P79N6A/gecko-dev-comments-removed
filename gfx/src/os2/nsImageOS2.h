












































#ifndef _nsImageOS2_h_
#define _nsImageOS2_h_

#include "nsIImage.h"
#include "nsRect.h"

struct nsDrawingSurfaceOS2;

class nsImageOS2 : public nsIImage{
public:
  nsImageOS2();
  ~nsImageOS2();

  NS_DECL_ISUPPORTS

  


  virtual PRInt32     GetBytesPix()       { return mInfo ? (mInfo->cBitCount <= 8 ? 1 : mInfo->cBitCount / 8) : 0; }
  virtual PRInt32     GetHeight()         { return mInfo ? mInfo->cy : 0; }
  virtual PRBool      GetIsRowOrderTopToBottom() { return PR_FALSE; }
  virtual PRInt32     GetWidth()          { return mInfo ? mInfo->cx : 0; }
  virtual PRUint8*    GetBits()           { return mImageBits; }
  virtual PRInt32     GetLineStride()     { return mRowBytes; }

  virtual PRBool      GetHasAlphaMask()   { return mAlphaBits != nsnull; }

  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                      PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);
  virtual nsColorMap* GetColorMap() {return mColorMap;}
  virtual void        ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect);
  virtual PRBool      GetIsImageComplete();
  virtual nsresult    Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements);
  virtual nsresult    Optimize(nsIDeviceContext* aContext);
  virtual PRUint8*    GetAlphaBits()      { return mAlphaBits; }
  virtual PRInt32     GetAlphaLineStride(){ return mARowBytes; }

  







  NS_IMETHOD DrawTile(nsIRenderingContext &aContext,
                      nsIDrawingSurface* aSurface,
                      PRInt32 aSXOffset, PRInt32 aSYOffset,
		      PRInt32 aPadX, PRInt32 aPadY,
                      const nsRect &aTileRect);

  NS_IMETHOD DrawToImage(nsIImage* aDstImage, nscoord aDX, nscoord aDY,
                         nscoord aDWidth, nscoord aDHeight);

  



#if 0 
  PRIntn      GetSizeHeader(){return sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * mNumPaletteColors;}
#endif

  




#if 0 
  PRIntn      GetSizeImage(){ return mSizeImage; }
#endif

  




#if 0 
  PRInt32  CalcBytesSpan(PRUint32  aWidth);
#endif

  




  virtual PRInt8 GetAlphaDepth() {return(mAlphaDepth);}

  




  void* GetBitInfo()  { return mInfo; }

  NS_IMETHOD   LockImagePixels(PRBool aMaskPixels);
  NS_IMETHOD   UnlockImagePixels(PRBool aMaskPixels);


 private:
  




  void CleanUp(PRBool aCleanUpAll);

  void DrawComposited24(unsigned char *aBits,
                        PRUint8 *aImageRGB, PRUint32 aStrideRGB,
                        PRUint8 *aImageAlpha, PRUint32 aStrideAlpha,
                        int aWidth, int aHeight);

#ifdef OS2TODO
  






  nsresult ConvertDDBtoDIB(PRInt32 aWidth,PRInt32 aHeight);


  








  nsresult PrintDDB(nsIDrawingSurface* aSurface,PRInt32 aX,PRInt32 aY,PRInt32 aWidth,PRInt32 aHeight);


  







  PRUint8 PaletteMatch(PRUint8 r, PRUint8 g, PRUint8 b);
#endif
  BITMAPINFO2*        mInfo;
  PRUint32            mDeviceDepth;
  PRInt32             mRowBytes;          
  PRUint8*            mImageBits;         
  PRBool              mIsOptimized;       
  nsColorMap*         mColorMap;          

  nsRect              mDecodedRect;       
    
  
  PRUint8             *mAlphaBits;        
  PRInt8              mAlphaDepth;        
  PRInt16             mARowBytes;         

  static PRUint8      gBlenderLookup [65536];    
  static PRBool       gBlenderReady;
  static void         BuildBlenderLookup (void);
  static PRUint8      FAST_BLEND (PRUint8 Source, PRUint8 Dest, PRUint8 Alpha) { return gBlenderLookup [(Alpha << 8) + Source] + 
                                                                                        gBlenderLookup [((255 - Alpha) << 8) + Dest]; }

  NS_IMETHODIMP UpdateImageBits( HPS mPS );
  void NS2PM_ININ( const nsRect &in, RECTL &rcl);
  void CreateBitmaps( nsDrawingSurfaceOS2 *surf);

  void     BuildTile (HPS hpsTile, PRUint8* pImageBits, PBITMAPINFO2 pBitmapInfo, nscoord aTileWidth, nscoord aTileHeight, float scale);
};

#endif
