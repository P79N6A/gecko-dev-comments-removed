





































#include <PMApplication.h>

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsPrintOptionsX.h"
#include "nsPrintSettingsX.h"

#include "nsCRT.h"
#include "plbase64.h"
#include "prmem.h"




nsPrintOptionsX::nsPrintOptionsX()
{
}



nsPrintOptionsX::~nsPrintOptionsX()
{
}




nsresult nsPrintOptionsX::_CreatePrintSettings(nsIPrintSettings **_retval)
{
  nsresult rv;
  *_retval = nsnull;

  nsPrintSettingsX* printSettings = new nsPrintSettingsX; 
  NS_ENSURE_TRUE(printSettings, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = printSettings); 

  rv = printSettings->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(*_retval);
    return rv;
  }

  (void)InitPrintSettingsFromPrefs(*_retval, PR_FALSE,
                                   nsIPrintSettings::kInitSaveAll);
  return rv;
}



NS_IMETHODIMP
nsPrintOptionsX::ShowPrintSetupDialog(nsIPrintSettings *aThePrintSettings)
{
  return NS_ERROR_NOT_IMPLEMENTED;
} 


NS_IMETHODIMP
nsPrintOptionsX::GetNativeData(PRInt16 aDataType, void * *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  return NS_ERROR_NOT_IMPLEMENTED;
}

#pragma mark -

nsresult
nsPrintOptionsX::ReadPrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, PRUint32 aFlags)
{
  nsresult rv;
  
  rv = nsPrintOptions::ReadPrefs(aPS, aPrinterName, aFlags);
  NS_ASSERTION(NS_SUCCEEDED(rv), "nsPrintOptions::ReadPrefs() failed");
  
  nsCOMPtr<nsIPrintSettingsX> printSettingsX(do_QueryInterface(aPS));
  if (!printSettingsX)
    return NS_ERROR_NO_INTERFACE;
  rv = printSettingsX->ReadPageFormatFromPrefs();
  NS_ASSERTION(NS_SUCCEEDED(rv), "nsIPrintSettingsX::ReadPageFormatFromPrefs() failed");
  
  return NS_OK;
}

nsresult
nsPrintOptionsX::WritePrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, PRUint32 aFlags)
{
  nsresult rv;
  
  rv = nsPrintOptions::WritePrefs(aPS, aPrinterName, aFlags);
  NS_ASSERTION(NS_SUCCEEDED(rv), "nsPrintOptions::WritePrefs() failed");
  
  nsCOMPtr<nsIPrintSettingsX> printSettingsX(do_QueryInterface(aPS));
  if (!printSettingsX)
    return NS_ERROR_NO_INTERFACE;
  rv = printSettingsX->WritePageFormatToPrefs();
  NS_ASSERTION(NS_SUCCEEDED(rv), "nsIPrintSettingsX::WritePageFormatToPrefs() failed");
  
  return NS_OK;
}
