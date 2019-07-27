





#ifndef nsPrintOptionsWin_h__
#define nsPrintOptionsWin_h__

#include "mozilla/embedding/PPrinting.h"
#include "nsPrintOptionsImpl.h"  

class nsIPrintSettings;
class nsIWebBrowserPrint;




class nsPrintOptionsWin : public nsPrintOptions
{
public:
  nsPrintOptionsWin();
  virtual ~nsPrintOptionsWin();

  NS_IMETHODIMP SerializeToPrintData(nsIPrintSettings* aSettings,
                                     nsIWebBrowserPrint* aWBP,
                                     mozilla::embedding::PrintData* data);
  NS_IMETHODIMP DeserializeToPrintSettings(const mozilla::embedding::PrintData& data,
                                           nsIPrintSettings* settings);

  virtual nsresult _CreatePrintSettings(nsIPrintSettings **_retval);
};



#endif 
