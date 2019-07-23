





































#ifndef nsPrintOptionsX_h_
#define nsPrintOptionsX_h_

#include "nsPrintOptionsImpl.h"

class nsPrintOptionsX : public nsPrintOptions
{
public:
             nsPrintOptionsX();
  virtual    ~nsPrintOptionsX();
  NS_IMETHOD ShowPrintSetupDialog(nsIPrintSettings *aThePrintSettings);
  NS_IMETHOD GetNativeData(PRInt16 aDataType, void * *_retval);

protected:
  nsresult   _CreatePrintSettings(nsIPrintSettings **_retval);
  nsresult   ReadPrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, PRUint32 aFlags);
  nsresult   WritePrefs(nsIPrintSettings* aPS, const nsAString& aPrinterName, PRUint32 aFlags);
};

#endif 
