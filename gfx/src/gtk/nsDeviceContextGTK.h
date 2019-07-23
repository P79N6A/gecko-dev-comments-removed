




































#ifndef nsDeviceContextGTK_h___
#define nsDeviceContextGTK_h___

#include "nsDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIWidget.h"
#include "nsIView.h"
#include "nsIRenderingContext.h"

#include "nsRenderingContextGTK.h"
#include "nsIScreenManager.h"

class nsDeviceContextGTK : public DeviceContextImpl
{
public:
  nsDeviceContextGTK();
  virtual ~nsDeviceContextGTK();

  static void Shutdown(); 

  NS_IMETHOD  Init(nsNativeWidget aNativeWidget);

  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aView,aContext));}
  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aWidget,aContext));}
  NS_IMETHOD  CreateRenderingContext(nsIDrawingSurface* aSurface, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aSurface, aContext));}
  NS_IMETHOD  CreateRenderingContextInstance(nsIRenderingContext *&aContext);

  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

  NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;

  NS_IMETHOD CheckFontExistence(const nsString& aFontName);

  NS_IMETHOD GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD GetClientRect(nsRect &aRect);
  NS_IMETHOD GetRect(nsRect &aRect);

  NS_IMETHOD GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                 nsIDeviceContext *&aContext);

  NS_IMETHOD BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD EndDocument(void);
  NS_IMETHOD AbortDocument(void);

  NS_IMETHOD BeginPage(void);
  NS_IMETHOD EndPage(void);

  NS_IMETHOD GetDepth(PRUint32& aDepth);

  NS_IMETHOD ClearCachedSystemFonts();

  static int prefChanged(const char *aPref, void *aClosure);

protected:
  nsresult   SetDPI(PRInt32 aPrefDPI);
  
private:
  PRUint32      mDepth;
  PRBool        mWriteable;
  PRUint32      mNumCells;
  static nscoord mDpi;

  float         mWidthFloat;
  float         mHeightFloat;
  PRInt32       mWidth;
  PRInt32       mHeight;
  GdkWindow    *mDeviceWindow;

  nsCOMPtr<nsIScreenManager> mScreenManager;

  nsresult GetSystemFontInfo(GdkFont* iFont, nsSystemFontID anID, nsFont* aFont) const;
};



#define NS_TO_GDK_RGB(ns) \
  ((ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff))
#define GDK_COLOR_TO_NS_RGB(c) \
  ((nscolor) NS_RGB(c.red, c.green, c.blue))


#endif 

