




#ifndef nsPrintOptionsX_h_
#define nsPrintOptionsX_h_

#include "nsPrintOptionsImpl.h"

class nsPrintOptionsX : public nsPrintOptions
{
public:
             nsPrintOptionsX();
  virtual    ~nsPrintOptionsX();
protected:
  nsresult   _CreatePrintSettings(nsIPrintSettings **_retval);
  nsresult   ReadPrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, uint32_t aFlags);
  nsresult   WritePrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, uint32_t aFlags);
};

#endif 
