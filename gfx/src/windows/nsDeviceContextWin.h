




































#ifndef nsDeviceContextWin_h___
#define nsDeviceContextWin_h___

#include "nsDeviceContext.h"
#include "nsIScreenManager.h"
#include <windows.h>

class nsIScreen;


class nsDeviceContextWin : public DeviceContextImpl
{
public:
  nsDeviceContextWin();

  NS_IMETHOD  Init(nsNativeWidget aWidget);

  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);

  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

  NS_IMETHOD  GetCanonicalPixelScale(float &aScale) const;
  NS_IMETHOD  SetCanonicalPixelScale(float aScale);

  NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;

  
  
  
  
  NS_IMETHOD  GetDrawingSurface(nsIRenderingContext &aContext, nsIDrawingSurface* &aSurface);

  NS_IMETHOD  CheckFontExistence(const nsString& aFontName);

  NS_IMETHOD  GetDepth(PRUint32& aDepth);

  NS_IMETHOD  GetPaletteInfo(nsPaletteInfo&);

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

  
  static char* GetACPString(const nsAString& aStr);

friend class nsNativeThemeWin;

protected:
  virtual ~nsDeviceContextWin();
  void CommonInit(HDC aDC);
  nsresult Init(nsNativeDeviceContext aContext, nsIDeviceContext *aOrigContext);
  void FindScreen ( nsIScreen** outScreen ) ;
  void ComputeClientRectUsingScreen ( nsRect* outRect ) ;
  void ComputeFullAreaUsingScreen ( nsRect* outRect ) ;
  nsresult GetSysFontInfo(HDC aHDC, nsSystemFontID anID, nsFont* aFont) const;

  nsresult CopyLogFontToNSFont(HDC* aHDC, const LOGFONT* ptrLogFont, nsFont* aFont,
                               PRBool aIsWide = PR_FALSE) const;
  
  PRBool mCachedClientRect;
  PRBool mCachedFullRect;

  nsIDrawingSurface*      mSurface;
  PRUint32              mDepth;  
  nsPaletteInfo         mPaletteInfo;
  float                 mPixelScale;
  PRInt32               mWidth;
  PRInt32               mHeight;
  nsRect                mClientRect;
  nsIDeviceContextSpec  *mSpec;

  nsCOMPtr<nsIScreenManager> mScreenManager;    

public:
  HDC                   mDC;
};

#endif 
