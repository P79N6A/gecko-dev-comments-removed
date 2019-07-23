







































 
#ifndef _XPCONTEXT_H_
#define _XPCONTEXT_H_

#include <X11/Xlib.h>
#include <X11/extensions/Print.h>
#include "nsColor.h"
#include "nsCoord.h"
#include "nsIImage.h"
#include "nsGCCache.h"
#include "nsIDeviceContextSpecXPrint.h"
#include "nsDrawingSurfaceXlib.h"
#include "xlibrgb.h"

class nsDeviceContextXp;

class nsXPrintContext : public nsIDrawingSurfaceXlib
{
public:
  nsXPrintContext();
  virtual ~nsXPrintContext();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags)  { return NS_OK; };
  NS_IMETHOD Unlock(void)  { return NS_OK; };
  
  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
  NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

  NS_IMETHOD Init(nsDeviceContextXp *dc, nsIDeviceContextSpecXp *aSpec);
  NS_IMETHOD BeginPage();
  NS_IMETHOD EndPage();
  NS_IMETHOD RenderEPS(Drawable aDrawable, const nsRect& aRect, const unsigned char *aData, unsigned long aDatalen);
  NS_IMETHOD BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD EndDocument();
  NS_IMETHOD AbortDocument();

  int                     GetHeight() { return mHeight; }
  int                     GetWidth() { return mWidth; }
  NS_IMETHOD GetDrawable(Drawable &aDrawable) { aDrawable = mDrawable; return NS_OK; }
  NS_IMETHOD GetXlibRgbHandle(XlibRgbHandle *&aHandle) { aHandle = mXlibRgbHandle; return NS_OK; }
  NS_IMETHOD GetGC(xGC *&aXGC) { mGC->AddRef(); aXGC = mGC; return NS_OK; }

  virtual Drawable GetDrawable() { return mDrawable; }
  
  void                    SetGC(xGC *aGC) { mGC = aGC; mGC->AddRef(); }

  NS_IMETHOD GetPrintResolution(int &aXres, int &aYres);

  NS_IMETHOD DrawImage(Drawable aDrawable, xGC *gc, nsIImage *aImage,
                PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);

  NS_IMETHOD DrawImage(Drawable aDrawable, xGC *gc, nsIImage *aImage,
                 PRInt32 aX, PRInt32 aY,
                 PRInt32 aWidth, PRInt32 aHeight);

private:
  nsresult DrawImageBitsScaled(Drawable aDrawable,
                xGC *gc, nsIImage *aImage,
                PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight);
                
  nsresult DrawImageBits(Drawable aDrawable, xGC *gc, 
                         PRUint8 *alphaBits, PRInt32  alphaRowBytes, PRUint8 alphaDepth,
                         PRUint8 *image_bits, PRInt32  row_bytes,
                         PRInt32 aX, PRInt32 aY,
                         PRInt32 aWidth, PRInt32 aHeight); 

  XlibRgbHandle *mXlibRgbHandle;
  Display      *mPDisplay;
  Screen       *mScreen;
  Visual       *mVisual;
  Drawable      mDrawable; 
  nsPixelFormat mPixFormat;
  xGC          *mGC;
  int           mXpEventBase, 
                mXpErrorBase; 
  int           mDepth;
  int           mScreenNumber;
  int           mWidth;
  int           mHeight;
  XPContext     mPContext;
  PRBool        mJobStarted;  
  PRBool        mIsGrayscale; 
  PRBool        mIsAPrinter;  
  const char   *mPrintFile;   
  void         *mXpuPrintToFileHandle; 
  long          mPrintXResolution,
                mPrintYResolution;
  nsDeviceContextXp *mContext; 

  static PRUint8 ConvertMaskToCount(unsigned long val);
  static PRUint8 GetShiftForMask(unsigned long val);

  nsresult SetupWindow(int x, int y, int width, int height);
  nsresult SetupPrintContext(nsIDeviceContextSpecXp *aSpec);
  nsresult SetMediumSize(const char *paper_name);
  nsresult SetOrientation(int landscape);
  nsresult SetPlexMode(const char *plexname);
  nsresult SetResolution(const char *resolution_name);
};


#endif 

