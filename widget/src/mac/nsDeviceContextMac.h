




































#ifndef nsDeviceContextMac_h___
#define nsDeviceContextMac_h___

#include "nsDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIWidget.h"
#include "nsIView.h"
#include "nsIRenderingContext.h"
#include "nsIFontEnumerator.h"
#include <Types.h>
#include <Quickdraw.h>

#include "nsIScreen.h"
#include "nsIScreenManager.h"


class nsDeviceContextMac : public DeviceContextImpl
{
public:
  nsDeviceContextMac();

  NS_IMETHOD  Init(nsNativeWidget aNativeWidget);  

  

  using DeviceContextImpl::CreateRenderingContext;
  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
  NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

  NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;

  NS_IMETHOD 	CheckFontExistence(const nsString& aFontName);
  NS_IMETHOD 	GetDepth(PRUint32& aDepth);

  NS_IMETHOD 	GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD  GetRect(nsRect &aRect);
  NS_IMETHOD  GetClientRect(nsRect &aRect);

  NS_IMETHOD 	GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                 nsIDeviceContext *&aContext);

  NS_IMETHOD 	BeginDocument(PRUnichar * aTitle, 
                              PRUnichar*  aPrintToFileName,
                              PRInt32     aStartPage, 
                              PRInt32     aEndPage);
  NS_IMETHOD 	EndDocument(void);
  NS_IMETHOD 	AbortDocument(void);

  NS_IMETHOD 	BeginPage(void);
  NS_IMETHOD 	EndPage(void);
  PRBool	IsPrinter() 	{if (nsnull != mSpec){return (PR_TRUE);}else{return (PR_FALSE);} };



protected:
  virtual 	~nsDeviceContextMac();
  
	virtual nsresult CreateFontAliasTable();

  void FindScreenForSurface ( nsIScreen** outScreen ) ;

  Rect									mPageRect;
  nsCOMPtr<nsIDeviceContextSpec> mSpec;
  GrafPtr								mOldPort;
  nsCOMPtr<nsIScreenManager> mScreenManager;
  nsCOMPtr<nsIScreen> mPrimaryScreen;         
  
public:
  
  static void InitFontInfoList();
  static nsHashtable* gFontInfoList;
public:
  static bool GetMacFontNumber(const nsString& aFontName, short &fontNum);

private:
	static PRUint32		mPixelsPerInch;
	static PRUint32   sNumberOfScreens;       
public:
	static PRUint32		GetScreenResolution();
};


class nsFontEnumeratorMac : public nsIFontEnumerator
{
public:
  nsFontEnumeratorMac();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR
};

#endif
