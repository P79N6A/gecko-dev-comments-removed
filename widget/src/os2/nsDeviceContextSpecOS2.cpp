







































#include <stdlib.h>
#include "nsDeviceContextSpecOS2.h"

#include "nsReadableUtils.h"
#include "nsISupportsArray.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "prenv.h" 

#include "nsPrintfCString.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtils.h"
#include "nsStringFwd.h"
#include "nsStringEnumerator.h"

#include "nsOS2Uni.h"

#include "nsILocalFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFileStreams.h"
#include "gfxPDFSurface.h"
#include "gfxOS2Surface.h"

PRINTDLG nsDeviceContextSpecOS2::PrnDlg;







class GlobalPrinters {
public:
  static GlobalPrinters* GetInstance()   { return &mGlobalPrinters; }
  ~GlobalPrinters()                      { FreeGlobalPrinters(); }

  void      FreeGlobalPrinters();
  nsresult  InitializeGlobalPrinters();

  PRBool    PrintersAreAllocated()       { return mGlobalPrinterList != nsnull; }
  PRInt32   GetNumPrinters()             { return mGlobalNumPrinters; }
  nsString* GetStringAt(PRInt32 aInx)    { return mGlobalPrinterList->StringAt(aInx); }
  void      GetDefaultPrinterName(PRUnichar*& aDefaultPrinterName);

protected:
  GlobalPrinters() {}

  static GlobalPrinters mGlobalPrinters;
  static nsStringArray* mGlobalPrinterList;
  static ULONG          mGlobalNumPrinters;

};


GlobalPrinters GlobalPrinters::mGlobalPrinters;
nsStringArray* GlobalPrinters::mGlobalPrinterList = nsnull;
ULONG          GlobalPrinters::mGlobalNumPrinters = 0;


nsDeviceContextSpecOS2::nsDeviceContextSpecOS2()
  : mQueue(nsnull)
{
}

nsDeviceContextSpecOS2::~nsDeviceContextSpecOS2()
{
  if (mQueue)
    PrnClosePrinter(mQueue);
}

NS_IMPL_ISUPPORTS1(nsDeviceContextSpecOS2, nsIDeviceContextSpec)

void SetupDevModeFromSettings(ULONG printer, nsIPrintSettings* aPrintSettings)
{
  if (aPrintSettings) {
    int bufferSize = 3 * sizeof(DJP_ITEM);
    PBYTE pDJP_Buffer = new BYTE[bufferSize];
    memset(pDJP_Buffer, 0, bufferSize);
    PDJP_ITEM pDJP = (PDJP_ITEM) pDJP_Buffer;

    HDC hdc = nsDeviceContextSpecOS2::PrnDlg.GetDCHandle(printer);
    char* driver = nsDeviceContextSpecOS2::PrnDlg.GetDriverType(printer);

    
    PRInt32 orientation;
    aPrintSettings->GetOrientation(&orientation);
    if (!strcmp(driver, "LASERJET"))
      pDJP->lType = DJP_ALL;
    else
      pDJP->lType = DJP_CURRENT;
    pDJP->cb = sizeof(DJP_ITEM);
    pDJP->ulNumReturned = 1;
    pDJP->ulProperty = DJP_SJ_ORIENTATION;
    pDJP->ulValue = orientation == nsIPrintSettings::kPortraitOrientation?DJP_ORI_PORTRAIT:DJP_ORI_LANDSCAPE;
    pDJP++;

    
    PRInt32 copies;
    aPrintSettings->GetNumCopies(&copies);
    pDJP->cb = sizeof(DJP_ITEM);
    pDJP->lType = DJP_CURRENT;
    pDJP->ulNumReturned = 1;
    pDJP->ulProperty = DJP_SJ_COPIES;
    pDJP->ulValue = copies;
    pDJP++;

    pDJP->cb = sizeof(DJP_ITEM);
    pDJP->lType = DJP_NONE;
    pDJP->ulNumReturned = 1;
    pDJP->ulProperty = 0;
    pDJP->ulValue = 0;

    LONG driverSize = nsDeviceContextSpecOS2::PrnDlg.GetPrintDriverSize(printer);
    GreEscape (hdc, DEVESC_SETJOBPROPERTIES, bufferSize, pDJP_Buffer, 
               &driverSize, PBYTE(nsDeviceContextSpecOS2::PrnDlg.GetPrintDriver(printer)));

    delete [] pDJP_Buffer;
    DevCloseDC(hdc);
  }
}

nsresult nsDeviceContextSpecOS2::SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, ULONG printer)
{
  if (aPrintSettings == nsnull)
    return NS_ERROR_FAILURE;

  int bufferSize = 3 * sizeof(DJP_ITEM);
  PBYTE pDJP_Buffer = new BYTE[bufferSize];
  memset(pDJP_Buffer, 0, bufferSize);
  PDJP_ITEM pDJP = (PDJP_ITEM) pDJP_Buffer;

  HDC hdc = nsDeviceContextSpecOS2::PrnDlg.GetDCHandle(printer);

  
  pDJP->lType = DJP_CURRENT;
  pDJP->cb = sizeof(DJP_ITEM);
  pDJP->ulNumReturned = 1;
  pDJP->ulProperty = DJP_SJ_COPIES;
  pDJP->ulValue = 1;
  pDJP++;

  
  pDJP->lType = DJP_CURRENT;
  pDJP->cb = sizeof(DJP_ITEM);
  pDJP->ulNumReturned = 1;
  pDJP->ulProperty = DJP_SJ_ORIENTATION;
  pDJP->ulValue = 1;
  pDJP++;

  pDJP->lType = DJP_NONE;
  pDJP->cb = sizeof(DJP_ITEM);
  pDJP->ulNumReturned = 1;
  pDJP->ulProperty = 0;
  pDJP->ulValue = 0;

  LONG driverSize = nsDeviceContextSpecOS2::PrnDlg.GetPrintDriverSize(printer);
  LONG rc = GreEscape(hdc, DEVESC_QUERYJOBPROPERTIES, bufferSize, pDJP_Buffer, 
                      &driverSize, PBYTE(nsDeviceContextSpecOS2::PrnDlg.GetPrintDriver(printer)));

  pDJP = (PDJP_ITEM) pDJP_Buffer;
  if ((rc == DEV_OK) || (rc == DEV_WARNING)) { 
    while (pDJP->lType != DJP_NONE) {
      if ((pDJP->ulProperty == DJP_SJ_ORIENTATION) && (pDJP->lType > 0)){
        if ((pDJP->ulValue == DJP_ORI_PORTRAIT) || (pDJP->ulValue == DJP_ORI_REV_PORTRAIT))
          aPrintSettings->SetOrientation(nsIPrintSettings::kPortraitOrientation);
        else
         aPrintSettings->SetOrientation(nsIPrintSettings::kLandscapeOrientation);
      }
      if ((pDJP->ulProperty == DJP_SJ_COPIES) && (pDJP->lType > 0)){
        aPrintSettings->SetNumCopies(PRInt32(pDJP->ulValue));
      }
      pDJP = DJP_NEXT_STRUCTP(pDJP);
    }
  }
  
  delete [] pDJP_Buffer;
  DevCloseDC(hdc);  
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecOS2::Init(nsIWidget *aWidget,
                                           nsIPrintSettings* aPS,
                                           PRBool aIsPrintPreview)
{
  nsresult rv = NS_ERROR_FAILURE;

  mPrintSettings = aPS;
  NS_ASSERTION(aPS, "Must have a PrintSettings!");

  rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }
 
  if (aPS) {
    PRBool     tofile         = PR_FALSE;
    PRInt32    copies         = 1;
    PRUnichar *printer        = nsnull;
    PRUnichar *printfile      = nsnull;

    mPrintSettings->GetPrinterName(&printer);
    mPrintSettings->GetToFileName(&printfile);
    mPrintSettings->GetPrintToFile(&tofile);
    mPrintSettings->GetNumCopies(&copies);

    if ((copies == 0)  ||  (copies > 999)) {
       GlobalPrinters::GetInstance()->FreeGlobalPrinters();
       return NS_ERROR_FAILURE;
    }

    if (printfile != nsnull) {
      
      strcpy(mPrData.path,    NS_ConvertUTF16toUTF8(printfile).get());
    }
    if (printer != nsnull) 
      strcpy(mPrData.printer, NS_ConvertUTF16toUTF8(printer).get());  

    if (aIsPrintPreview) 
      mPrData.destination = printPreview; 
    else if (tofile)  
      mPrData.destination = printToFile;
    else  
      mPrData.destination = printToPrinter;
    mPrData.copies = copies;

    rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
    if (NS_FAILED(rv))
      return rv;

    const nsAFlatString& printerUCS2 = NS_ConvertUTF8toUTF16(mPrData.printer);
    ULONG numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();
    if (numPrinters) {
       for(ULONG i = 0; (i < numPrinters) && !mQueue; i++) {
          if ((GlobalPrinters::GetInstance()->GetStringAt(i)->Equals(printerUCS2, nsCaseInsensitiveStringComparator()))) {
             SetupDevModeFromSettings(i, aPS);
             mQueue = PrnDlg.SetPrinterQueue(i);
          }
       }
    }

    if (printfile != nsnull) 
      nsMemory::Free(printfile);
  
    if (printer != nsnull) 
      nsMemory::Free(printer);
  }

  GlobalPrinters::GetInstance()->FreeGlobalPrinters();
  return rv;
}


NS_IMETHODIMP nsDeviceContextSpecOS2 :: GetDestination( int &aDestination )     
{
  aDestination = mPrData.destination;
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecOS2 :: GetPrinterName ( char **aPrinter )
{
   *aPrinter = &mPrData.printer[0];
   return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecOS2 :: GetCopies ( int &aCopies )
{
   aCopies = mPrData.copies;
   return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecOS2 :: GetPath ( char **aPath )      
{
  *aPath = &mPrData.path[0];
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecOS2 :: GetUserCancelled( PRBool &aCancel )     
{
  aCancel = mPrData.cancel;
  return NS_OK;
}





NS_IMETHODIMP nsDeviceContextSpecOS2 :: ClosePrintManager()
{
  return NS_OK;
}

nsresult nsDeviceContextSpecOS2::GetPRTQUEUE( PRTQUEUE *&p)
{
   p = mQueue;
   return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecOS2::GetSurfaceForPrinter(gfxASurface **surface)
{
  NS_ASSERTION(mQueue, "Queue can't be NULL here");

  nsRefPtr<gfxASurface> newSurface;

  PRInt16 outputFormat;
  mPrintSettings->GetOutputFormat(&outputFormat);

  if (outputFormat == nsIPrintSettings::kOutputFormatPDF) {
    nsXPIDLString filename;
    mPrintSettings->GetToFileName(getter_Copies(filename));
    nsresult rv;
    if (filename.IsEmpty()) {
      
      nsCOMPtr<nsIFile> pdfLocation;
      rv = NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(pdfLocation));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = pdfLocation->AppendNative(NS_LITERAL_CSTRING("moz_print.pdf"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = pdfLocation->GetPath(filename);
      NS_ENSURE_SUCCESS(rv, rv);
    }
#ifdef debug_thebes_print
    printf("nsDeviceContextSpecOS2::GetSurfaceForPrinter(): print to filename=%s\n",
           NS_LossyConvertUTF16toASCII(filename).get());
#endif

    double width, height;
    mPrintSettings->GetEffectivePageSize(&width, &height);
    
    width /= 20;
    height /= 20;

    nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    rv = file->InitWithPath(filename);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIFileOutputStream> stream = do_CreateInstance("@mozilla.org/network/file-output-stream;1");
    rv = stream->Init(file, -1, -1, 0);
    if (NS_FAILED(rv))
      return rv;

    newSurface = new(std::nothrow) gfxPDFSurface(stream, gfxSize(width, height));
  } else {
    int numCopies = 0;
    int printerDest = 0;
    char *filename = nsnull;

    GetCopies(numCopies);
    GetDestination(printerDest);
    if (!printerDest) {
      GetPath(&filename);
    }
    HDC printdc = PrnOpenDC(mQueue, "Mozilla", numCopies, printerDest, filename);

    double width, height;
    mPrintSettings->GetEffectivePageSize(&width, &height);
#ifdef debug_thebes_print
    printf("nsDeviceContextSpecOS2::GetSurfaceForPrinter(): %fx%ftwips, copies=%d\n",
           width, height, numCopies);
#endif

    
    
    
    double hDPI = 3937., vDPI = 3937.;
    LONG value;
    if (DevQueryCaps(printdc, CAPS_HORIZONTAL_RESOLUTION, 1, &value))
      hDPI = value * 0.0254;
    if (DevQueryCaps(printdc, CAPS_VERTICAL_RESOLUTION, 1, &value))
      vDPI = value * 0.0254;
    width = width * hDPI / 1440;
    height = height * vDPI / 1440;
#ifdef debug_thebes_print
    printf("nsDeviceContextSpecOS2::GetSurfaceForPrinter(): %fx%fpx (res=%fx%f)\n",
           width, height, hDPI, vDPI);
#endif

    
    
    newSurface = new(std::nothrow)
      gfxOS2Surface(printdc, gfxIntSize(int(ceil(width)), int(ceil(height))));
  }
  if (!newSurface) {
    *surface = nsnull;
    return NS_ERROR_FAILURE;
  }
  *surface = newSurface;
  NS_ADDREF(*surface);
  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecOS2::BeginDocument(PRUnichar* aTitle,
                                                    PRUnichar* aPrintToFileName,
                                                    PRInt32 aStartPage,
                                                    PRInt32 aEndPage)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsDeviceContextSpecOS2::EndDocument()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsDeviceContextSpecOS2::BeginPage()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsDeviceContextSpecOS2::EndPage()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsPrinterEnumeratorOS2::nsPrinterEnumeratorOS2()
{
}

NS_IMPL_ISUPPORTS1(nsPrinterEnumeratorOS2, nsIPrinterEnumerator)

NS_IMETHODIMP nsPrinterEnumeratorOS2::GetPrinterNameList(nsIStringEnumerator **aPrinterNameList)
{
  NS_ENSURE_ARG_POINTER(aPrinterNameList);
  *aPrinterNameList = nsnull;

  nsDeviceContextSpecOS2::PrnDlg.RefreshPrintQueue();
  
  nsresult rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }

  ULONG numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();
  nsStringArray *printers = new nsStringArray(numPrinters);
  if (!printers) {
    GlobalPrinters::GetInstance()->FreeGlobalPrinters();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  ULONG count = 0;
  while( count < numPrinters )
  {
    printers->AppendString(*GlobalPrinters::GetInstance()->GetStringAt(count++));
  }
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();

  return NS_NewAdoptingStringEnumerator(aPrinterNameList, printers);
}

NS_IMETHODIMP nsPrinterEnumeratorOS2::GetDefaultPrinterName(PRUnichar * *aDefaultPrinterName)
{
  NS_ENSURE_ARG_POINTER(aDefaultPrinterName);
  GlobalPrinters::GetInstance()->GetDefaultPrinterName(*aDefaultPrinterName);
  return NS_OK;
}


NS_IMETHODIMP nsPrinterEnumeratorOS2::InitPrintSettingsFromPrinter(const PRUnichar *aPrinterName, nsIPrintSettings *aPrintSettings)
{
   NS_ENSURE_ARG_POINTER(aPrinterName);
   NS_ENSURE_ARG_POINTER(aPrintSettings);

   if (!*aPrinterName) 
     return NS_OK;

  if (NS_FAILED(GlobalPrinters::GetInstance()->InitializeGlobalPrinters())) 
    return NS_ERROR_FAILURE;

  ULONG numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();
  for(ULONG i = 0; i < numPrinters; i++) {
    if ((GlobalPrinters::GetInstance()->GetStringAt(i)->Equals(aPrinterName, nsCaseInsensitiveStringComparator()))) 
      nsDeviceContextSpecOS2::SetPrintSettingsFromDevMode(aPrintSettings, i);
  }

  
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();
  aPrintSettings->SetIsInitializedFromPrinter(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP nsPrinterEnumeratorOS2::DisplayPropertiesDlg(const PRUnichar *aPrinter, nsIPrintSettings *aPrintSettings)
{
  nsresult rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) {
    return rv;
  }

  ULONG numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();
  for(ULONG i = 0; i < numPrinters; i++) {
    if ((GlobalPrinters::GetInstance()->GetStringAt(i)->Equals(aPrinter, nsCaseInsensitiveStringComparator()))) {
       SetupDevModeFromSettings(i, aPrintSettings);
       if ( nsDeviceContextSpecOS2::PrnDlg.ShowProperties(i) ) {
          nsDeviceContextSpecOS2::SetPrintSettingsFromDevMode(aPrintSettings, i);
          return NS_OK;
       } else {
          return NS_ERROR_FAILURE;
       }
    }
  }
  return NS_ERROR_FAILURE;
}

nsresult GlobalPrinters::InitializeGlobalPrinters ()
{
  if (PrintersAreAllocated()) 
    return NS_OK;

  mGlobalNumPrinters = 0;
  mGlobalNumPrinters = nsDeviceContextSpecOS2::PrnDlg.GetNumPrinters();
  if (!mGlobalNumPrinters) 
    return NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE; 

  mGlobalPrinterList = new nsStringArray();
  if (!mGlobalPrinterList) 
     return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> pPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  BOOL prefFailed = NS_FAILED(rv); 

  for (ULONG i = 0; i < mGlobalNumPrinters; i++) {
    nsXPIDLCString printer;
    nsDeviceContextSpecOS2::PrnDlg.GetPrinter(i, getter_Copies(printer));

    nsAutoChar16Buffer printerName;
    PRInt32 printerNameLength;
    rv = MultiByteToWideChar(0, printer, strlen(printer),
                             printerName, printerNameLength);
    mGlobalPrinterList->AppendString(nsDependentString(printerName.Elements()));

    
    if (!prefFailed) {
       nsCAutoString printerDescription;
       printerDescription = nsCAutoString(nsDeviceContextSpecOS2::PrnDlg.GetPrintDriver(i)->szDeviceName);
       printerDescription += " (";
       printerDescription += nsCAutoString(nsDeviceContextSpecOS2::PrnDlg.GetDriverType(i));
       printerDescription += ")";
       pPrefs->SetCharPref(nsPrintfCString(256,
                                           "print.printer_%s.printer_description",
                                           printer.get()).get(),
                           printerDescription.get());
    }
  } 
  return NS_OK;
}

void GlobalPrinters::GetDefaultPrinterName(PRUnichar*& aDefaultPrinterName)
{
  aDefaultPrinterName = nsnull;

  nsresult rv = GlobalPrinters::GetInstance()->InitializeGlobalPrinters();
  if (NS_FAILED(rv)) 
     return;

  if (GetNumPrinters() == 0)
     return;

  
  nsXPIDLCString printer;
  nsDeviceContextSpecOS2::PrnDlg.GetPrinter(0, getter_Copies(printer));

  nsAutoChar16Buffer printerName;
  PRInt32 printerNameLength;
  MultiByteToWideChar(0, printer, strlen(printer), printerName,
                      printerNameLength);
  aDefaultPrinterName = ToNewUnicode(nsDependentString(printerName.Elements()));

  GlobalPrinters::GetInstance()->FreeGlobalPrinters();
}

void GlobalPrinters::FreeGlobalPrinters()
{
  delete mGlobalPrinterList;
  mGlobalPrinterList = nsnull;
  mGlobalNumPrinters = 0;
}

