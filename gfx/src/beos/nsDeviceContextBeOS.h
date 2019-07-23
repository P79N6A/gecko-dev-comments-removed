




































#ifndef nsDeviceContextBeOS_h___
#define nsDeviceContextBeOS_h___

#include "nsDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIWidget.h"
#include "nsIView.h"
#include "nsIRenderingContext.h"

#include "nsRenderingContextBeOS.h"

class nsDeviceContextBeOS : public DeviceContextImpl
{
public:
  nsDeviceContextBeOS();
  virtual ~nsDeviceContextBeOS();

  NS_IMETHOD  Init(nsNativeWidget aNativeWidget);

  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aView,aContext));} 
  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aWidget,aContext));} 
 
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

  NS_IMETHOD BeginPage(void);
  NS_IMETHOD EndPage(void);

  NS_IMETHOD GetDepth(PRUint32& aDepth); 
 
  static int prefChanged(const char *aPref, void *aClosure); 
  nsresult   SetDPI(PRInt32 dpi); 

private:
  PRUint32      mDepth;
  PRBool        mWriteable;
  PRUint32      mNumCells;
  BFont         mMenuFont; 
  static nscoord mDpi; 
 
  float         mWidthFloat; 
  float         mHeightFloat; 
  PRInt32       mWidth; 
  PRInt32       mHeight; 
 
  nsresult GetSystemFontInfo(const BFont *font, nsSystemFontID anID, nsFont* aFont) const;
};

#endif 

