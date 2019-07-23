





































#ifndef nsDeviceContextXp_h___
#define nsDeviceContextXp_h___

#include "nsDeviceContextX.h"
#include "nsRenderingContextXp.h"
#include "nsIDeviceContextXPrint.h"
#include "nsXPrintContext.h"

class nsDeviceContextXp : public nsDeviceContextX,
                          public nsIDeviceContextXp
{
public:
  nsDeviceContextXp();

  NS_DECL_ISUPPORTS_INHERITED

  




  NS_IMETHOD  InitDeviceContextXP(nsIDeviceContext *aCreatingDeviceContext,nsIDeviceContext *aPrinterContext); 

  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aView,aContext));}
  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aWidget,aContext));}
#ifdef NOT_NOW
  



  NS_IMETHOD  CreateRenderingContext(nsIDrawingSurface* aSurface, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aSurface, aContext));}
#else
  NS_IMETHOD  CreateRenderingContext(nsIDrawingSurface* aSurface, nsIRenderingContext *&aContext) {return NS_ERROR_NOT_IMPLEMENTED;}
#endif 
  NS_IMETHOD  CreateRenderingContextInstance(nsIRenderingContext *&aContext);
  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

  NS_IMETHOD         CheckFontExistence(const nsString& aFontName);

  NS_IMETHOD         GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD         GetRect(nsRect &aRect);
  NS_IMETHOD         GetClientRect(nsRect &aRect);

  NS_IMETHOD         GetDeviceContextFor(nsIDeviceContextSpec *aDevice,nsIDeviceContext *&aContext);
  NS_IMETHOD         GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;
  NS_IMETHOD         BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD         EndDocument(void);
  NS_IMETHOD         AbortDocument(void);
  NS_IMETHOD         BeginPage(void);
  NS_IMETHOD         EndPage(void);

  NS_IMETHOD         SetSpec(nsIDeviceContextSpec *aSpec);
  NS_IMETHOD         GetXlibRgbHandle(XlibRgbHandle *&aHandle)
                     { return mPrintContext->GetXlibRgbHandle(aHandle); }
  XlibRgbHandle *GetXlibRgbHandle() { XlibRgbHandle *h; mPrintContext->GetXlibRgbHandle(h); return h; } 
  NS_IMETHOD         GetDepth(PRUint32 &depth) 
                     { depth = xxlib_rgb_get_depth(GetXlibRgbHandle()); return NS_OK; }
  NS_IMETHOD         GetPrintContext(nsXPrintContext*& aContext);

  NS_IMETHOD         CreateFontCache();

  virtual void GetFontMetricsContext(nsFontMetricsXlibContext *&aContext) { aContext = mFontMetricsContext; };
  virtual void GetRCContext(nsRenderingContextXlibContext *&aContext) { aContext = mRCContext; };
 
protected:
  virtual         ~nsDeviceContextXp();
  void             DestroyXPContext();

  nsCOMPtr<nsXPrintContext>      mPrintContext;  
  nsCOMPtr<nsIDeviceContextSpec> mSpec;
  nsCOMPtr<nsIDeviceContext>     mParentDeviceContext;
  nsFontMetricsXlibContext      *mFontMetricsContext;
  nsRenderingContextXlibContext *mRCContext;
};

#endif 
