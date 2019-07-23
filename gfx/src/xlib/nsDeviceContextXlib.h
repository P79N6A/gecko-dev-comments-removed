






































#ifndef nsDeviceContextXlib_h__
#define nsDeviceContextXlib_h__

#include "nsDeviceContextX.h"
#include "nsRenderingContextXlib.h"

class nsDeviceContextXlib : public nsDeviceContextX
{
public:
  nsDeviceContextXlib();

  NS_IMETHOD  Init(nsNativeWidget aNativeWidget);

  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext)
    {return (DeviceContextImpl::CreateRenderingContext(aView, aContext)); }
  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext)
    {return (DeviceContextImpl::CreateRenderingContext(aWidget, aContext)); }
  NS_IMETHOD  CreateRenderingContext(nsIDrawingSurface* aSurface, nsIRenderingContext *&aContext) 
    {return (DeviceContextImpl::CreateRenderingContext(aSurface, aContext));}
  NS_IMETHOD  CreateRenderingContextInstance(nsIRenderingContext *&aContext);

  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

  NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;

  NS_IMETHOD CheckFontExistence(const nsString& aFontName);

  NS_IMETHOD GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD GetRect(nsRect &aRect);
  NS_IMETHOD GetClientRect(nsRect &aRect);

  NS_IMETHOD GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                 nsIDeviceContext *&aContext);

  NS_IMETHOD BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD EndDocument(void);
  NS_IMETHOD AbortDocument(void);

  NS_IMETHOD BeginPage(void);
  NS_IMETHOD EndPage(void);

  NS_IMETHOD CreateFontCache();

  NS_IMETHOD GetXlibRgbHandle(XlibRgbHandle *&aHandle) 
             { aHandle =  mXlibRgbHandle; return NS_OK; }
  XlibRgbHandle *GetXlibRgbHandle() { return mXlibRgbHandle; }
  NS_IMETHOD GetDepth( PRUint32 &depth ) { depth = (PRUint32)mDepth; return NS_OK; }
  virtual void GetFontMetricsContext(nsFontMetricsXlibContext *&aContext) { aContext = mFontMetricsContext; };
  virtual void GetRCContext(nsRenderingContextXlibContext *&aContext) { aContext = mRCContext; };

protected:
  virtual ~nsDeviceContextXlib();

private:
  nsresult             CommonInit(void);
  PRBool               mWriteable;
  PRUint32             mNumCells;
  nsIDrawingSurface*     mSurface;
  XlibRgbHandle       *mXlibRgbHandle;
  Display *            mDisplay;
  Screen *             mScreen;
  Visual *             mVisual;
  int                  mDepth;
  static nsFontMetricsXlibContext      *mFontMetricsContext;
  static nsRenderingContextXlibContext *mRCContext;
  static int                            mContextCounter;

  float                mWidthFloat;
  float                mHeightFloat;
  PRInt32              mWidth;
  PRInt32              mHeight;
};

#endif 

