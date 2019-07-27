





#ifndef nsPrintOptionsGTK_h__
#define nsPrintOptionsGTK_h__

#include "nsPrintOptionsImpl.h"  

namespace mozilla
{
namespace embedding
{
  struct PrintData;
} 
} 




class nsPrintOptionsGTK : public nsPrintOptions
{
public:
  nsPrintOptionsGTK();
  virtual ~nsPrintOptionsGTK();

  NS_IMETHODIMP SerializeToPrintData(nsIPrintSettings* aSettings,
                                     nsIWebBrowserPrint* aWBP,
                                     mozilla::embedding::PrintData* data);
  NS_IMETHODIMP DeserializeToPrintSettings(const mozilla::embedding::PrintData& data,
                                           nsIPrintSettings* settings);

  virtual nsresult _CreatePrintSettings(nsIPrintSettings **_retval);

};



#endif 
