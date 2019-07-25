



































#include "nsCOMPtr.h"
#include "nsPrintOptionsWin.h"
#include "nsPrintSettingsWin.h"

#include "nsGfxCIID.h"
#include "nsIServiceManager.h"
const char kPrinterEnumeratorContractID[] = "@mozilla.org/gfx/printerenumerator;1";





nsPrintOptionsWin::nsPrintOptionsWin()
{

}





nsPrintOptionsWin::~nsPrintOptionsWin()
{
}


nsresult nsPrintOptionsWin::_CreatePrintSettings(nsIPrintSettings **_retval)
{
  *_retval = nsnull;
  nsPrintSettingsWin* printSettings = new nsPrintSettingsWin(); 
  NS_ENSURE_TRUE(printSettings, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = printSettings); 

  return NS_OK;
}

