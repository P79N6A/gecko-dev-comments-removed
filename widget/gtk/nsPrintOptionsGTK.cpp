



#include "nsPrintOptionsGTK.h"
#include "nsPrintSettingsGTK.h"

using namespace mozilla::embedding;





nsPrintOptionsGTK::nsPrintOptionsGTK()
{

}





nsPrintOptionsGTK::~nsPrintOptionsGTK()
{
}

NS_IMETHODIMP
nsPrintOptionsGTK::SerializeToPrintData(nsIPrintSettings* aSettings,
                                        nsIWebBrowserPrint* aWBP,
                                        PrintData* data)
{
  nsresult rv = nsPrintOptions::SerializeToPrintData(aSettings, aWBP, data);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsPrintOptionsGTK::DeserializeToPrintSettings(const PrintData& data,
                                              nsIPrintSettings* settings)
{
  nsresult rv = nsPrintOptions::DeserializeToPrintSettings(data, settings);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult nsPrintOptionsGTK::_CreatePrintSettings(nsIPrintSettings **_retval)
{
  *_retval = nullptr;
  nsPrintSettingsGTK* printSettings = new nsPrintSettingsGTK(); 
  NS_ENSURE_TRUE(printSettings, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = printSettings); 

  return NS_OK;
}


