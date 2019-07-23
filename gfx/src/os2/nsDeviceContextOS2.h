




































#ifndef nsDeviceContextOS2_h___
#define nsDeviceContextOS2_h___

#include "nsGfxDefs.h"

#include "nsDeviceContext.h"
#include "nsIScreenManager.h"
#include "nsDeviceContextSpecOS2.h"

class nsIScreen;

class nsDeviceContextOS2 : public DeviceContextImpl
{
public:
  nsDeviceContextOS2();

  NS_IMETHOD  Init(nsNativeWidget aWidget);

  using DeviceContextImpl::CreateRenderingContext;
  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);

  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

  NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;

  NS_IMETHOD  CheckFontExistence(const nsString& aFontName);

  NS_IMETHOD  GetDepth(PRUint32& aDepth);

  NS_IMETHOD GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD GetRect(nsRect &aRect);
  NS_IMETHOD GetClientRect(nsRect &aRect);

  NS_IMETHOD GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                 nsIDeviceContext *&aContext);

  NS_IMETHOD PrepareDocument(PRUnichar * aTitle, 
                             PRUnichar*  aPrintToFileName);
  NS_IMETHOD BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD EndDocument(void);
  NS_IMETHOD AbortDocument(void);

  NS_IMETHOD BeginPage(void);
  NS_IMETHOD EndPage(void);

  
  static char* GetACPString(const nsString& aStr);
  nsresult   SetDPI(PRInt32 aPrefDPI);
  int        GetDPI() { return mDpi; };

protected:
  virtual ~nsDeviceContextOS2();
  void CommonInit(HDC aDC);
  nsresult Init(nsNativeDeviceContext aContext, nsIDeviceContext *aOrigContext);
  void FindScreen ( nsIScreen** outScreen ) ;
  void ComputeClientRectUsingScreen ( nsRect* outRect ) ;
  void ComputeFullAreaUsingScreen ( nsRect* outRect ) ;

  PRBool mCachedClientRect;
  PRBool mCachedFullRect;
  PRBool mPrintingStarted;

  nsIDrawingSurface*      mSurface;
  PRUint32              mDepth;  
  PRBool                mIsPaletteDevice;
  PRInt32               mWidth;
  PRInt32               mHeight;
  nsRect                mClientRect;
  nsIDeviceContextSpec  *mSpec;
  PRBool                mSupportsRasterFonts;

  nsCOMPtr<nsIScreenManager> mScreenManager;
  static PRUint32 sNumberOfScreens;
  static nscoord mDpi;

public:
  HDC                   mPrintDC;
  HPS                   mPrintPS;

  enum nsPrintState
  {
     nsPrintState_ePreBeginDoc,
     nsPrintState_eBegunDoc,
     nsPrintState_eBegunFirstPage,
     nsPrintState_eEndedDoc
  } mPrintState;

  BOOL isPrintDC();
  PRBool SupportsRasterFonts();
  PRBool IsPaletteDevice() {return mIsPaletteDevice;};
  nsresult CreateFontAliasTable();
};

int PR_CALLBACK prefChanged(const char *aPref, void *aClosure);

#endif 
