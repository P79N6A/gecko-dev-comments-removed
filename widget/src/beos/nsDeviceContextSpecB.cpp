




































 
#include "nsIWidget.h"
#include "nsDeviceContextSpecB.h"
 
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "prenv.h"  
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsStringEnumerator.h"
#include "nsCRT.h"







class GlobalPrinters {
public:
  static GlobalPrinters* GetInstance()   { return &mGlobalPrinters; }
  ~GlobalPrinters()                      { FreeGlobalPrinters(); }

  void      FreeGlobalPrinters();
  nsresult  InitializeGlobalPrinters();

  PRBool    PrintersAreAllocated()       { return mGlobalPrinterList != nsnull; }
  PRInt32   GetNumPrinters()             { return mGlobalNumPrinters; }
  nsString* GetStringAt(PRInt32 aInx)    { return mGlobalPrinterList->StringAt(aInx); }

protected:
  GlobalPrinters() {}

  static GlobalPrinters mGlobalPrinters;
  static nsStringArray* mGlobalPrinterList;
  static int            mGlobalNumPrinters;

};


GlobalPrinters GlobalPrinters::mGlobalPrinters;
nsStringArray* GlobalPrinters::mGlobalPrinterList = nsnull;
int            GlobalPrinters::mGlobalNumPrinters = 0;





nsDeviceContextSpecBeOS :: nsDeviceContextSpecBeOS()
{
}





nsDeviceContextSpecBeOS :: ~nsDeviceContextSpecBeOS()
{
} 
 
static NS_DEFINE_IID(kIDeviceContextSpecIID, NS_IDEVICE_CONTEXT_SPEC_IID); 
 
#if 0 
NS_IMPL_ISUPPORTS1(nsDeviceContextSpecBeOS, nsIDeviceContextSpec)
#endif 
 
NS_IMETHODIMP nsDeviceContextSpecBeOS :: QueryInterface(REFNSIID aIID, void** aInstancePtr) 
{ 
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(kIDeviceContextSpecIID)) 
  { 
    nsIDeviceContextSpec* tmp = this; 
    *aInstancePtr = (void*) tmp; 
    NS_ADDREF_THIS(); 
    return NS_OK; 
  } 
 
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

  if (aIID.Equals(kISupportsIID)) 
  { 
    nsIDeviceContextSpec* tmp = this; 
    nsISupports* tmp2 = tmp; 
    *aInstancePtr = (void*) tmp2; 
    NS_ADDREF_THIS(); 
    return NS_OK; 
  } 
 
  *aInstancePtr = nsnull;
  return NS_ERROR_NO_INTERFACE;
} 
 
NS_IMPL_ADDREF(nsDeviceContextSpecBeOS)
NS_IMPL_RELEASE(nsDeviceContextSpecBeOS)






NS_IMETHODIMP nsDeviceContextSpecBeOS::Init(nsIWidget *aWidget,
                                            nsIPrintSettings* aPS,
                                            PRBool aIsPrintPreview)
{
  nsresult rv = NS_ERROR_FAILURE;
  NS_ASSERTION(nsnull != aPS, "No print settings.");

  mPrintSettings = aPS;
  
  
  if (aPS != nsnull) {
    PRBool isOn;
    aPS->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &isOn);
    nsCOMPtr<nsIPrefBranch> pPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      (void) pPrefs->SetBoolPref("print.selection_radio_enabled", isOn);
    }
  }

  rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();

  return rv;
}
 




NS_IMETHODIMP nsDeviceContextSpecBeOS :: ClosePrintManager()
{
  return NS_OK;
}  


nsPrinterEnumeratorBeOS::nsPrinterEnumeratorBeOS()
{
}

NS_IMPL_ISUPPORTS1(nsPrinterEnumeratorBeOS, nsIPrinterEnumerator)

NS_IMETHODIMP nsPrinterEnumeratorBeOS::GetPrinterNameList(nsIStringEnumerator **aPrinterNameList)
{
  NS_ENSURE_ARG_POINTER(aPrinterNameList);
  *aPrinterNameList = nsnull;
  
  nsresult rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }

  PRInt32 numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();
  nsStringArray *printers = new nsStringArray(numPrinters);
  if (!printers) {
    GlobalPrinters::GetInstance()->FreeGlobalPrinters();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  int count = 0;
  while( count < numPrinters )
  {
    printers->AppendString(*GlobalPrinters::GetInstance()->GetStringAt(count++));
  }
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();

  return NS_NewAdoptingStringEnumerator(aPrinterNameList, printers);
}


NS_IMETHODIMP nsPrinterEnumeratorBeOS::GetDefaultPrinterName(PRUnichar * *aDefaultPrinterName)
{
  NS_ENSURE_ARG_POINTER(aDefaultPrinterName);
  *aDefaultPrinterName = nsnull;
  return NS_OK;
}


NS_IMETHODIMP nsPrinterEnumeratorBeOS::InitPrintSettingsFromPrinter(const PRUnichar *aPrinterName, nsIPrintSettings *aPrintSettings)
{
    return NS_OK;
}

NS_IMETHODIMP nsPrinterEnumeratorBeOS::DisplayPropertiesDlg(const PRUnichar *aPrinter, nsIPrintSettings *aPrintSettings)
{
  return NS_OK;
}


nsresult GlobalPrinters::InitializeGlobalPrinters ()
{
  if (PrintersAreAllocated())
    return NS_OK;

  mGlobalNumPrinters = 0;

#ifdef USE_POSTSCRIPT
  mGlobalPrinterList = new nsStringArray();
  if (!mGlobalPrinterList) 
    return NS_ERROR_OUT_OF_MEMORY;

  
  mGlobalPrinterList->AppendString(
    nsString(NS_ConvertASCIItoUTF16(NS_POSTSCRIPT_DRIVER_NAME "default")));
  mGlobalNumPrinters++;

  
  char *printerList = nsnull;
  
  
  printerList = PR_GetEnv("MOZILLA_PRINTER_LIST");
  
  if (!printerList) {
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> pPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID,
                                                   &rv);
    if (NS_SUCCEEDED(rv)) {
      (void) pPrefs->GetCharPref("print.printer_list", &printerList);
    }
  }  

  if (printerList) {
    char *tok_lasts;
    char *name;
    
    
    printerList = strdup(printerList);
    if (!printerList)
      return NS_ERROR_OUT_OF_MEMORY;    

    for( name = PL_strtok_r(printerList, " ", &tok_lasts) ; 
         name != nsnull ; 
         name = PL_strtok_r(nsnull, " ", &tok_lasts) )
    {
      mGlobalPrinterList->AppendString(
        nsString(NS_ConvertASCIItoUTF16(NS_POSTSCRIPT_DRIVER_NAME)) + 
        nsString(NS_ConvertASCIItoUTF16(name)));
      mGlobalNumPrinters++;      
    }

    NS_Free(printerList);
  }
#endif 
      
  if (mGlobalNumPrinters == 0)
    return NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE; 

  return NS_OK;
}


void GlobalPrinters::FreeGlobalPrinters()
{
  delete mGlobalPrinterList;
  mGlobalPrinterList = nsnull;
  mGlobalNumPrinters = 0;
}




