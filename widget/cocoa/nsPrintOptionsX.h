




#ifndef nsPrintOptionsX_h_
#define nsPrintOptionsX_h_

#include "nsPrintOptionsImpl.h"

namespace mozilla
{
namespace embedding
{
  class PrintData;
} 
} 

class nsPrintOptionsX : public nsPrintOptions
{
public:
             nsPrintOptionsX();
  virtual    ~nsPrintOptionsX();

  NS_IMETHODIMP SerializeToPrintData(nsIPrintSettings* aSettings,
                                     nsIWebBrowserPrint* aWBP,
                                     mozilla::embedding::PrintData* data);
  NS_IMETHODIMP DeserializeToPrintSettings(const mozilla::embedding::PrintData& data,
                                           nsIPrintSettings* settings);

protected:
  nsresult   _CreatePrintSettings(nsIPrintSettings **_retval);
  nsresult   ReadPrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, uint32_t aFlags);
  nsresult   WritePrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, uint32_t aFlags);
};

#endif 
