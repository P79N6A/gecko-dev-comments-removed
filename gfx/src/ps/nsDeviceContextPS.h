






































#ifndef nsDeviceContextPS_h___
#define nsDeviceContextPS_h___

#include "nsDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIWidget.h"
#include "nsIView.h"
#include "nsIRenderingContext.h"
#include "nsVoidArray.h"
#include "nsIDeviceContextPS.h"
#include "nsFontMetricsPS.h"
#include "nsIPrintJobPS.h"

class nsPostScriptObj;
class nsDeviceContextWin;       

class nsDeviceContextPS : public DeviceContextImpl,
                          public nsIDeviceContextPS
{
public:
  nsDeviceContextPS();

  NS_DECL_ISUPPORTS_INHERITED

  




  NS_IMETHOD  InitDeviceContextPS(nsIDeviceContext *aCreatingDeviceContext,nsIDeviceContext *aPrinterContext); 

  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aView,aContext));}
  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aWidget,aContext));}
  
  NS_IMETHOD  CreateRenderingContext(nsIDrawingSurface* aSurface, nsIRenderingContext *&aContext) {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD  CreateRenderingContextInstance(nsIRenderingContext *&aContext);

  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);
  NS_IMETHOD  PrepareNativeWidget(nsIWidget* aWidget, void ** aOut);

  NS_IMETHOD  CheckFontExistence(const nsString& aFontName);
  NS_IMETHOD  GetDepth(PRUint32& aDepth);

  NS_IMETHOD  GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD  GetClientRect(nsRect &aRect);
  NS_IMETHOD  GetRect(nsRect &aRect);

  NS_IMETHOD  GetDeviceContextFor(nsIDeviceContextSpec *aDevice,nsIDeviceContext *&aContext);
  NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;
  NS_IMETHOD  BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD  EndDocument(void);
  NS_IMETHOD  AbortDocument(void);
  NS_IMETHOD  BeginPage(void);
  NS_IMETHOD  EndPage(void);
  NS_IMETHOD  CreateFontCache();
  
  NS_IMETHOD  SetSpec(nsIDeviceContextSpec *aSpec);

  nsPostScriptObj*    GetPrintContext() { return mPSObj; }
  nsHashtable*        GetPSFontGeneratorList() { return mPSFontGeneratorList; }
  PRBool               mFTPEnable;

protected:
  virtual     ~nsDeviceContextPS();
  
  nsIDrawingSurface*       mSurface;
  PRUint32               mDepth;
  nsCOMPtr<nsIDeviceContextSpec>  mSpec;
  nsCOMPtr<nsIDeviceContext>      mParentDeviceContext;
  nsIPrintJobPS         *mPrintJob;
  nsPostScriptObj       *mPSObj;
  nsHashtable           *mPSFontGeneratorList;
};

#endif 
