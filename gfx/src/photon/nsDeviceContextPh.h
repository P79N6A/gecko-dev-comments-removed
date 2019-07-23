




































#ifndef nsDeviceContextPh_h___
#define nsDeviceContextPh_h___

#include "nsDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIWidget.h"
#include "nsIView.h"
#include "nsIRenderingContext.h"
#include "nsIScreenManager.h"

#include "nsRenderingContextPh.h"
#include <Pt.h>

class nsDeviceContextPh : public DeviceContextImpl
{
public:
  nsDeviceContextPh();
  virtual ~nsDeviceContextPh();
  
  NS_IMETHOD  Init(nsNativeWidget aWidget);


  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aView,aContext));}
  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext) {return (DeviceContextImpl::CreateRenderingContext(aWidget,aContext));}

	inline
  NS_IMETHODIMP SupportsNativeWidgets(PRBool &aSupportsWidgets)
		{
		
	  if( nsnull == mDC ) aSupportsWidgets = PR_TRUE;
	  else aSupportsWidgets = PR_FALSE;   
	  return NS_OK;
		}

  NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;

  
  
  
  

	inline
  NS_IMETHODIMP GetDrawingSurface(nsIRenderingContext &aContext, nsIDrawingSurface* &aSurface)
		{
		nsRect aRect;
		GetClientRect( aRect );
		aContext.CreateDrawingSurface(aRect, 0, aSurface);
		return nsnull == aSurface ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
		}

  NS_IMETHOD  CheckFontExistence(const nsString& aFontName);

  NS_IMETHOD  GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  inline NS_IMETHOD  GetClientRect(nsRect &aRect) { return GetRect ( aRect ); }
  NS_IMETHOD GetRect(nsRect &aRect);
 
  NS_IMETHOD  GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                  nsIDeviceContext *&aContext);

	NS_IMETHOD  BeginDocument(PRUnichar *t, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD  EndDocument(void);
  NS_IMETHOD  AbortDocument(void);

  NS_IMETHOD  BeginPage(void);
  NS_IMETHOD  EndPage(void);

	inline
  NS_IMETHODIMP GetDepth(PRUint32& aDepth)
		{
		aDepth = mDepth; 
		return NS_OK;
		}

  static int prefChanged(const char *aPref, void *aClosure);
  nsresult    SetDPI(PRInt32 dpi);

protected:

  nsresult    Init(nsNativeDeviceContext aContext, nsIDeviceContext *aOrigContext);
  nsresult    GetDisplayInfo(PRInt32 &aWidth, PRInt32 &aHeight, PRUint32 &aDepth);
  void        CommonInit(nsNativeDeviceContext aDC);
  void 		GetPrinterRect(int *width, int *height);

  nsIDrawingSurface*      mSurface;
  PRUint32              mDepth;  
  float                 mPixelScale;
  
  float                 mWidthFloat;
  float                 mHeightFloat;
  PRInt32               mWidth;
  PRInt32               mHeight;

  nsIDeviceContextSpec  *mSpec;
  nsNativeDeviceContext mDC;
	PhGC_t								*mGC;

  static nscoord        mDpi;

  PRBool mIsPrintingStart;

private:
	nsCOMPtr<nsIScreenManager> mScreenManager;
	int ReadSystemFonts( ) const;
	void DefaultSystemFonts( ) const;
};

#define	NS_FONT_STYLE_ANTIALIAS				0xf0

#endif 
