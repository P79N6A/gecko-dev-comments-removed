




#ifndef nsDeviceContextSpecWin_h___
#define nsDeviceContextSpecWin_h___

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsIPrintOptions.h" 
#include "nsIPrintSettings.h"
#include "nsISupportsPrimitives.h"
#include <windows.h>
#include "mozilla/Attributes.h"

class nsIWidget;

class nsDeviceContextSpecWin : public nsIDeviceContextSpec
{
public:
  nsDeviceContextSpecWin();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
  NS_IMETHOD BeginDocument(const nsAString& aTitle,
                           char16_t*       aPrintToFileName,
                           int32_t          aStartPage,
                           int32_t          aEndPage) { return NS_OK; }
  NS_IMETHOD EndDocument() { return NS_OK; }
  NS_IMETHOD BeginPage() { return NS_OK; }
  NS_IMETHOD EndPage() { return NS_OK; }

  NS_IMETHOD Init(nsIWidget* aWidget, nsIPrintSettings* aPS, bool aIsPrintPreview);

  void GetDriverName(wchar_t *&aDriverName) const   { aDriverName = mDriverName;     }
  void GetDeviceName(wchar_t *&aDeviceName) const   { aDeviceName = mDeviceName;     }

  
  
  
  
  void GetDevMode(LPDEVMODEW &aDevMode);

  
  nsresult GetDataFromPrinter(char16ptr_t aName, nsIPrintSettings* aPS = nullptr);

  static nsresult SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, 
                                              LPDEVMODEW         aDevMode);

protected:

  void SetDeviceName(char16ptr_t aDeviceName);
  void SetDriverName(char16ptr_t aDriverName);
  void SetDevMode(LPDEVMODEW aDevMode);

  void SetupPaperInfoFromSettings();

  virtual ~nsDeviceContextSpecWin();

  wchar_t*      mDriverName;
  wchar_t*      mDeviceName;
  LPDEVMODEW mDevMode;

  nsCOMPtr<nsIPrintSettings> mPrintSettings;
};





class nsPrinterEnumeratorWin MOZ_FINAL : public nsIPrinterEnumerator
{
  ~nsPrinterEnumeratorWin();

public:
  nsPrinterEnumeratorWin();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR
};

#endif
