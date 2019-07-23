




































#ifndef nsImageWin_h___
#define nsImageWin_h___

#include <windows.h>
#include "nsIImage.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"


#define GFX_MS_REMOVE_DBB          60000


#if !defined(AC_SRC_OVER)
#define AC_SRC_OVER                 0x00
#define AC_SRC_ALPHA                0x01
#pragma pack(1)
typedef struct {
    BYTE   BlendOp;
    BYTE   BlendFlags;
    BYTE   SourceConstantAlpha;
    BYTE   AlphaFormat;
}BLENDFUNCTION;
#pragma pack()
#endif

typedef BOOL (WINAPI *ALPHABLENDPROC)(
  HDC hdcDest,
  int nXOriginDest,
  int nYOriginDest,
  int nWidthDest,
  int hHeightDest,
  HDC hdcSrc,
  int nXOriginSrc,
  int nYOriginSrc,
  int nWidthSrc,
  int nHeightSrc,
  BLENDFUNCTION blendFunction);


#define VER_OSMAJOR_WINNT31        3
#define VER_OSMAJOR_WIN9598MENT    4
#define VER_OSMAJOR_WIN2KXP        5

class nsImageWin : public nsIImage{
public:
  nsImageWin();
  ~nsImageWin();

  NS_DECL_ISUPPORTS

  


  virtual PRInt32     GetBytesPix()       { return mNumBytesPixel; }
  virtual PRInt32     GetHeight()         { return mBHead->biHeight; }
  virtual PRBool      GetIsRowOrderTopToBottom() { return PR_FALSE; }
  virtual PRInt32     GetWidth()          { return mBHead->biWidth; }
  virtual PRUint8*    GetBits() ;
  virtual PRInt32     GetLineStride()     { return mRowBytes; }

  virtual PRBool      GetHasAlphaMask()   { return mAlphaBits != nsnull; }

  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, PRInt32 aX, PRInt32 aY,
                           PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD          Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, PRInt32 aSX, PRInt32 aSY,
                           PRInt32 aSWidth, PRInt32 aSHeight,
                           PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);
  NS_IMETHOD          DrawToImage(nsIImage* aDstImage, nscoord aDX, nscoord aDY,
                                  nscoord aDWidth, nscoord aDHeight);
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

   



  PRIntn      GetSizeHeader(){return sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * mNumPaletteColors;}

  




  PRIntn      GetSizeImage(){ return mSizeImage; }

  




  PRInt32  CalcBytesSpan(PRUint32  aWidth);

  




  virtual PRInt8 GetAlphaDepth() {return(mAlphaDepth);}

  




  void* GetBitInfo() {return mBHead;}

  NS_IMETHOD   LockImagePixels(PRBool aMaskPixels);
  NS_IMETHOD   UnlockImagePixels(PRBool aMaskPixels);

  
  
  static PRInt32 gPlatform;
  static PRInt32 gOsMajorVersion;

  

  NS_IMETHOD CreateDDB();
  

  NS_IMETHOD RemoveDDB();

private:
  



  void CleanUpDIB();

  



  void CleanUpDDB();

  void CreateImageWithAlphaBits(HDC TheHDC);

  void DrawComposited24(unsigned char *aBits,
                        PRUint8 *aImageRGB, PRUint32 aStrideRGB,
                        PRUint8 *aImageAlpha, PRUint32 aStrideAlpha,
                        int aWidth, int aHeight);
  nsresult DrawComposited(HDC TheHDC, int aDX, int aDY, int aDWidth, int aDHeight,
                          int aSX, int aSY, int aSWidth, int aSHeight,
                          int aOrigDWidth, int aOrigDHeight);
  static PRBool CanAlphaBlend(void);

  



  nsresult ConvertDDBtoDIB();


  













  nsresult PrintDDB(nsIDrawingSurface* aSurface,
                    PRInt32 aDX, PRInt32 aDY,
                    PRInt32 aDWidth, PRInt32 aDHeight,
                    PRInt32 aSX, PRInt32 aSY,
                    PRInt32 aSWidth, PRInt32 aSHeight,
                    PRUint32 aROP);


  



  PRBool ProgressiveDoubleBlit(nsIDeviceContext *aContext,
                               nsIDrawingSurface* aSurface,
                               PRInt32 aSXOffset, PRInt32 aSYOffset,
                               nsRect aDestRect);

  

















  void BlitImage(HDC aDstDC, HDC aDstMaskDC, PRInt32 aDstX, PRInt32 aDstY,
                 PRInt32 aWidth, PRInt32 aHeight,
                 HDC aSrcDC, HDC aSrcMaskDC, PRInt32 aSrcX, PRInt32 aSrcY,
                 PRBool aUseAlphaBlend);
  
  







  PRUint8 PaletteMatch(PRUint8 r, PRUint8 g, PRUint8 b);

  PRPackedBool        mInitialized;
  PRPackedBool        mWantsOptimization;
  PRPackedBool        mIsOptimized;       
  PRPackedBool        mIsLocked;          
  PRPackedBool        mDIBTemp;           
  PRPackedBool        mImagePreMultiplied;
  PRInt8              mNumBytesPixel;     
  PRInt16             mNumPaletteColors;  
  PRInt32             mSizeImage;         
  PRInt32             mRowBytes;          
  PRUint8*            mImageBits;         
  nsColorMap*         mColorMap;          

  PRInt32             mDecodedX1;         
  PRInt32             mDecodedY1;         
  PRInt32             mDecodedX2; 
  PRInt32             mDecodedY2; 

  
  PRUint8             *mAlphaBits;        
  PRInt8              mAlphaDepth;        
  PRInt32             mARowBytes;         
  PRInt8              mImageCache;        
  HBITMAP             mHBitmap;           
  LPBITMAPINFOHEADER  mBHead;             

  static ALPHABLENDPROC gAlphaBlend;      

  nsCOMPtr<nsITimer>  mTimer;             
  static void TimerCallBack(nsITimer *aTimer, void *aClosure);
};

#endif
