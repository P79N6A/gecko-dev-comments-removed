




































#ifndef nsDeviceContextSpecWin_h___
#define nsDeviceContextSpecWin_h___

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsIPrintOptions.h" 
#include "nsIPrintSettings.h"
#include "nsIWidget.h"
#include "nsISupportsPrimitives.h"
#include <windows.h>

class nsDeviceContextSpecWin : public nsIDeviceContextSpec
#ifndef MOZ_CAIRO_GFX
  , public nsISupportsVoid
#endif
{
public:
  nsDeviceContextSpecWin();

  NS_DECL_ISUPPORTS

#ifdef MOZ_CAIRO_GFX
  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
  NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                           PRUnichar*  aPrintToFileName,
                           PRInt32     aStartPage, 
                           PRInt32     aEndPage) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD EndDocument() { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD BeginPage() { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD EndPage() { return NS_ERROR_NOT_IMPLEMENTED; }
#else
  
  NS_IMETHOD GetType(PRUint16 *aType) { *aType = nsISupportsPrimitive::TYPE_VOID; return NS_OK; }
  NS_IMETHOD GetData(void * *aData);
  NS_IMETHOD SetData(void * aData) { return NS_ERROR_FAILURE; }
  NS_IMETHOD ToString(char **_retval) { return NS_ERROR_FAILURE; }
#endif

  NS_IMETHOD Init(nsIWidget* aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);

  void GetDriverName(char *&aDriverName) const   { aDriverName = mDriverName;     }
  void GetDeviceName(char *&aDeviceName) const   { aDeviceName = mDeviceName;     }

  
  
  
  
  void GetDevMode(LPDEVMODE &aDevMode);

  
  nsresult GetDataFromPrinter(const PRUnichar * aName, nsIPrintSettings* aPS = nsnull);

  static nsresult SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, 
                                              LPDEVMODE         aDevMode);

protected:

  void SetDeviceName(char* aDeviceName);
  void SetDriverName(char* aDriverName);
  void SetDevMode(LPDEVMODE aDevMode);

  void SetupPaperInfoFromSettings();

  virtual ~nsDeviceContextSpecWin();

  char*     mDriverName;
  char*     mDeviceName;
  LPDEVMODE mDevMode;

  nsCOMPtr<nsIPrintSettings> mPrintSettings;
};





class nsPrinterEnumeratorWin : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorWin();
  ~nsPrinterEnumeratorWin();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR

private:
  
  nsresult DoEnumeratePrinters(PRBool aDoExtended, PRUint32* aCount, PRUnichar*** aResult);
};

#endif
