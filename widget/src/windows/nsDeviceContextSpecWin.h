




































#ifndef nsDeviceContextSpecWin_h___
#define nsDeviceContextSpecWin_h___

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsIPrintOptions.h" 
#include "nsIPrintSettings.h"
#include "nsISupportsPrimitives.h"
#include <windows.h>

class nsIWidget;

class nsDeviceContextSpecWin : public nsIDeviceContextSpec
{
public:
  nsDeviceContextSpecWin();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
  NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                           PRUnichar*  aPrintToFileName,
                           PRInt32     aStartPage, 
                           PRInt32     aEndPage) { return NS_OK; }
  NS_IMETHOD EndDocument() { return NS_OK; }
  NS_IMETHOD BeginPage() { return NS_OK; }
  NS_IMETHOD EndPage() { return NS_OK; }

  NS_IMETHOD Init(nsIWidget* aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);

  void GetDriverName(PRUnichar *&aDriverName) const   { aDriverName = mDriverName;     }
  void GetDeviceName(PRUnichar *&aDeviceName) const   { aDeviceName = mDeviceName;     }

  
  
  
  
  void GetDevMode(LPDEVMODEW &aDevMode);

  
  nsresult GetDataFromPrinter(const PRUnichar * aName, nsIPrintSettings* aPS = nsnull);

  static nsresult SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, 
                                              LPDEVMODEW         aDevMode);

protected:

  void SetDeviceName(const PRUnichar* aDeviceName);
  void SetDriverName(const PRUnichar* aDriverName);
  void SetDevMode(LPDEVMODEW aDevMode);

  void SetupPaperInfoFromSettings();

  virtual ~nsDeviceContextSpecWin();

  PRUnichar*      mDriverName;
  PRUnichar*      mDeviceName;
  LPDEVMODEW mDevMode;

  nsCOMPtr<nsIPrintSettings> mPrintSettings;
};





class nsPrinterEnumeratorWin : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorWin();
  ~nsPrinterEnumeratorWin();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR
};

#endif
