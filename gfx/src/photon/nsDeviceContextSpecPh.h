




































#ifndef nsDeviceContextSpecPh_h___
#define nsDeviceContextSpecPh_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h" 
#include <Pt.h>

class nsDeviceContextSpecPh : public nsIDeviceContextSpec
{
public:
  nsDeviceContextSpecPh();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIWidget* aWidget, nsIPrintSettings* aPrintSettings, PRBool aQuiet);
  PpPrintContext_t *GetPrintContext();

protected:
  virtual ~nsDeviceContextSpecPh();
  PpPrintContext_t *mPC;
};




class nsPrinterEnumeratorPh : public nsIPrinterEnumerator
{
public:
  	nsPrinterEnumeratorPh();
  	~nsPrinterEnumeratorPh();
	NS_DECL_ISUPPORTS
	NS_DECL_NSIPRINTERENUMERATOR

	private:
		
		nsresult DoEnumeratePrinters(PRBool aDoExtended, PRUint32* aCount, PRUnichar*** aResult);
};


#endif
