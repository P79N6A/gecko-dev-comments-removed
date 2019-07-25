




































#include "nsPrintOptionsAndroid.h"

#include "nsPrintSettingsImpl.h"

class nsPrintSettingsAndroid : public nsPrintSettings {
public:
  nsPrintSettingsAndroid()
  {
    
    SetOutputFormat(nsIPrintSettings::kOutputFormatPDF);
    SetPrinterName(NS_LITERAL_STRING("PDF printer").get());
    
  }
};

nsPrintOptionsAndroid::nsPrintOptionsAndroid()
{
}

nsPrintOptionsAndroid::~nsPrintOptionsAndroid()
{
}

NS_IMETHODIMP nsPrintOptionsAndroid::CreatePrintSettings(nsIPrintSettings **_retval)
{
  nsPrintSettings * printSettings = new nsPrintSettingsAndroid();
  NS_ENSURE_TRUE(printSettings, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*_retval = printSettings);
  (void)InitPrintSettingsFromPrefs(*_retval, false,
                                   nsIPrintSettings::kInitSaveAll);
  return NS_OK;
}
