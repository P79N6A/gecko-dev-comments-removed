





































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
  nsresult   ReadPrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, PRUint32 aFlags);
  nsresult   WritePrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, PRUint32 aFlags);
};

#endif 
