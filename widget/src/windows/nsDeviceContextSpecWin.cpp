




































#include "nsDeviceContextSpecWin.h"
#include "prmem.h"

#ifndef WINCE
#include <winspool.h>
#endif

#include <tchar.h>

#include "nsAutoPtr.h"
#include "nsIWidget.h"

#include "nsTArray.h"
#include "nsIPrintSettingsWin.h"

#include "nsString.h"
#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsStringEnumerator.h"

#include "gfxPDFSurface.h"
#include "gfxWindowsSurface.h"

#include "nsIFileStreams.h"
#include "nsIWindowWatcher.h"
#include "nsIDOMWindow.h"


#include "nsNativeCharsetUtils.h"


#include "nsILocalFile.h"
#include "nsIFile.h"
#include "nsIFilePicker.h"
#include "nsIStringBundle.h"
#define NS_ERROR_GFX_PRINTER_BUNDLE_URL "chrome://global/locale/printing.properties"

#include "prlog.h"
#ifdef PR_LOGGING 
PRLogModuleInfo * kWidgetPrintingLogMod = PR_NewLogModule("printing-widget");
#define PR_PL(_p1)  PR_LOG(kWidgetPrintingLogMod, PR_LOG_DEBUG, _p1)
#else
#define PR_PL(_p1)
#endif







class GlobalPrinters {
public:
  static GlobalPrinters* GetInstance() { return &mGlobalPrinters; }
  ~GlobalPrinters() { FreeGlobalPrinters(); }

  void FreeGlobalPrinters();

  PRBool       PrintersAreAllocated() { return mPrinters != nsnull; }
  LPTSTR       GetItemFromList(PRInt32 aInx) { return mPrinters?mPrinters->ElementAt(aInx):nsnull; }
  nsresult     EnumeratePrinterList();
  void         GetDefaultPrinterName(LPTSTR& aDefaultPrinterName);
  PRInt32      GetNumPrinters() { return mPrinters?mPrinters->Length():0; }

protected:
  GlobalPrinters() {}
  nsresult EnumerateNativePrinters();
  void     ReallocatePrinters();

  static GlobalPrinters    mGlobalPrinters;
  static nsTArray<LPTSTR>* mPrinters;
};


GlobalPrinters    GlobalPrinters::mGlobalPrinters;
nsTArray<LPTSTR>* GlobalPrinters::mPrinters = nsnull;





typedef struct {
  short  mPaperSize; 
  double mWidth;
  double mHeight;
  PRBool mIsInches;
} NativePaperSizes;


const NativePaperSizes kPaperSizes[] = {
  {DMPAPER_LETTER,    8.5,   11.0,  PR_TRUE},
  {DMPAPER_LEGAL,     8.5,   14.0,  PR_TRUE},
  {DMPAPER_A4,        210.0, 297.0, PR_FALSE},
  {DMPAPER_B4,        250.0, 354.0, PR_FALSE}, 
  {DMPAPER_B5,        182.0, 257.0, PR_FALSE},
#ifndef WINCE
  {DMPAPER_TABLOID,   11.0,  17.0,  PR_TRUE},
  {DMPAPER_LEDGER,    17.0,  11.0,  PR_TRUE},
  {DMPAPER_STATEMENT, 5.5,   8.5,   PR_TRUE},
  {DMPAPER_EXECUTIVE, 7.25,  10.5,  PR_TRUE},
  {DMPAPER_A3,        297.0, 420.0, PR_FALSE},
  {DMPAPER_A5,        148.0, 210.0, PR_FALSE},
  {DMPAPER_CSHEET,    17.0,  22.0,  PR_TRUE},  
  {DMPAPER_DSHEET,    22.0,  34.0,  PR_TRUE},  
  {DMPAPER_ESHEET,    34.0,  44.0,  PR_TRUE},  
  {DMPAPER_LETTERSMALL, 8.5, 11.0,  PR_TRUE},  
  {DMPAPER_A4SMALL,   210.0, 297.0, PR_FALSE}, 
  {DMPAPER_FOLIO,     8.5,   13.0,  PR_TRUE},
  {DMPAPER_QUARTO,    215.0, 275.0, PR_FALSE},
  {DMPAPER_10X14,     10.0,  14.0,  PR_TRUE},
  {DMPAPER_11X17,     11.0,  17.0,  PR_TRUE},
  {DMPAPER_NOTE,      8.5,   11.0,  PR_TRUE},  
  {DMPAPER_ENV_9,     3.875, 8.875, PR_TRUE},  
  {DMPAPER_ENV_10,    40.125, 9.5,  PR_TRUE},  
  {DMPAPER_ENV_11,    4.5,   10.375, PR_TRUE},  
  {DMPAPER_ENV_12,    4.75,  11.0,  PR_TRUE},  
  {DMPAPER_ENV_14,    5.0,   11.5,  PR_TRUE},  
  {DMPAPER_ENV_DL,    110.0, 220.0, PR_FALSE}, 
  {DMPAPER_ENV_C5,    162.0, 229.0, PR_FALSE}, 
  {DMPAPER_ENV_C3,    324.0, 458.0, PR_FALSE}, 
  {DMPAPER_ENV_C4,    229.0, 324.0, PR_FALSE}, 
  {DMPAPER_ENV_C6,    114.0, 162.0, PR_FALSE}, 
  {DMPAPER_ENV_C65,   114.0, 229.0, PR_FALSE}, 
  {DMPAPER_ENV_B4,    250.0, 353.0, PR_FALSE}, 
  {DMPAPER_ENV_B5,    176.0, 250.0, PR_FALSE}, 
  {DMPAPER_ENV_B6,    176.0, 125.0, PR_FALSE}, 
  {DMPAPER_ENV_ITALY, 110.0, 230.0, PR_FALSE}, 
  {DMPAPER_ENV_MONARCH,  3.875,  7.5, PR_TRUE},  
  {DMPAPER_ENV_PERSONAL, 3.625,  6.5, PR_TRUE},  
  {DMPAPER_FANFOLD_US,   14.875, 11.0, PR_TRUE},  
  {DMPAPER_FANFOLD_STD_GERMAN, 8.5, 12.0, PR_TRUE},  
  {DMPAPER_FANFOLD_LGL_GERMAN, 8.5, 13.0, PR_TRUE},  
#endif 
};
const PRInt32 kNumPaperSizes = 41;


nsDeviceContextSpecWin::nsDeviceContextSpecWin()
{
  mDriverName    = nsnull;
  mDeviceName    = nsnull;
  mDevMode       = NULL;

}




NS_IMPL_ISUPPORTS1(nsDeviceContextSpecWin, nsIDeviceContextSpec)

nsDeviceContextSpecWin::~nsDeviceContextSpecWin()
{
  SetDeviceName(nsnull);
  SetDriverName(nsnull);
  SetDevMode(NULL);

  nsCOMPtr<nsIPrintSettingsWin> psWin(do_QueryInterface(mPrintSettings));
  if (psWin) {
    psWin->SetDeviceName(nsnull);
    psWin->SetDriverName(nsnull);
    psWin->SetDevMode(NULL);
  }

  
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();
}




static PRUnichar * GetDefaultPrinterNameFromGlobalPrinters()
{
  PRUnichar * printerName;
  LPTSTR lpPrtName;
  GlobalPrinters::GetInstance()->GetDefaultPrinterName(lpPrtName);
  nsAutoString str;
  NS_CopyNativeToUnicode(nsDependentCString((char *)lpPrtName), str);
  printerName = ToNewUnicode(str);
  free(lpPrtName);
  return printerName;
}


static nsresult 
EnumerateNativePrinters(DWORD aWhichPrinters, LPWSTR aPrinterName, PRBool& aIsFound, PRBool& aIsFile)
{
#ifdef WINCE
  aIsFound = PR_FALSE;
#else
  DWORD             dwSizeNeeded = 0;
  DWORD             dwNumItems   = 0;
  LPPRINTER_INFO_2W  lpInfo        = NULL;

  
  if (::EnumPrinters ( aWhichPrinters, NULL, 2, NULL, 0, &dwSizeNeeded, &dwNumItems )) {
    return NS_ERROR_FAILURE;
  }

  
  lpInfo = (LPPRINTER_INFO_2W)HeapAlloc ( GetProcessHeap (), HEAP_ZERO_MEMORY, dwSizeNeeded );
  if ( lpInfo == NULL ) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (::EnumPrinters ( PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)lpInfo, dwSizeNeeded, &dwSizeNeeded, &dwNumItems) == 0 ) {
    ::HeapFree(GetProcessHeap (), 0, lpInfo);
    return NS_OK;
  }


  for (DWORD i = 0; i < dwNumItems; i++ ) {
    if (wcscmp(lpInfo[i].pPrinterName, aPrinterName) == 0) {
      aIsFound = PR_TRUE;
      aIsFile  = wcscmp(lpInfo[i].pPortName, L"FILE:") == 0;
      break;
    }
  }

  ::HeapFree(GetProcessHeap (), 0, lpInfo);
#endif
  return NS_OK;
}


static void 
CheckForPrintToFileWithName(LPWSTR aPrinterName, PRBool& aIsFile)
{
  PRBool isFound = PR_FALSE;
  aIsFile = PR_FALSE;
#ifndef WINCE
  nsresult rv = EnumerateNativePrinters(PRINTER_ENUM_LOCAL, aPrinterName, isFound, aIsFile);
  if (isFound) return;

  rv = EnumerateNativePrinters(PRINTER_ENUM_NETWORK, aPrinterName, isFound, aIsFile);
  if (isFound) return;

  rv = EnumerateNativePrinters(PRINTER_ENUM_SHARED, aPrinterName, isFound, aIsFile);
  if (isFound) return;

  rv = EnumerateNativePrinters(PRINTER_ENUM_REMOTE, aPrinterName, isFound, aIsFile);
  if (isFound) return;
#endif
}

static nsresult 
GetFileNameForPrintSettings(nsIPrintSettings* aPS)
{
  
#ifdef DEBUG_rods
  return NS_OK;
#endif

  nsresult rv;

  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIStringBundle> bundle;
  rv = bundleService->CreateBundle(NS_ERROR_GFX_PRINTER_BUNDLE_URL, getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLString title;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("PrintToFile").get(), getter_Copies(title));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWindowWatcher> wwatch =
    (do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindow> window;
  wwatch->GetActiveWindow(getter_AddRefs(window));

  rv = filePicker->Init(window, title, nsIFilePicker::modeSave);
  NS_ENSURE_SUCCESS(rv, rv);
 
  rv = filePicker->AppendFilters(nsIFilePicker::filterAll);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar* fileName;
  aPS->GetToFileName(&fileName);

  if (fileName) {
    if (*fileName) {
      nsAutoString leafName;
      nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
      if (file) {
        rv = file->InitWithPath(nsDependentString(fileName));
        if (NS_SUCCEEDED(rv)) {
          file->GetLeafName(leafName);
          filePicker->SetDisplayDirectory(file);
        }
      }
      if (!leafName.IsEmpty()) {
        rv = filePicker->SetDefaultString(leafName);
      }
      NS_ENSURE_SUCCESS(rv, rv);
    }
    nsMemory::Free(fileName);
  }

  PRInt16 dialogResult;
  filePicker->Show(&dialogResult);

  if (dialogResult == nsIFilePicker::returnCancel) {
    return NS_ERROR_ABORT;
  }

  nsCOMPtr<nsILocalFile> localFile;
  rv = filePicker->GetFile(getter_AddRefs(localFile));
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (dialogResult == nsIFilePicker::returnReplace) {
    
    PRBool isFile;
    rv = localFile->IsFile(&isFile);
    if (NS_SUCCEEDED(rv) && isFile) {
      rv = localFile->Remove(PR_FALSE );
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsAutoString unicodePath;
  rv = localFile->GetPath(unicodePath);
  NS_ENSURE_SUCCESS(rv,rv);

  if (unicodePath.IsEmpty()) {
    rv = NS_ERROR_ABORT;
  }

  if (NS_SUCCEEDED(rv)) aPS->SetToFileName(unicodePath.get());

  return rv;
}


static nsresult
CheckForPrintToFile(nsIPrintSettings* aPS, LPWSTR aPrinterName, PRUnichar* aUPrinterName)
{
  nsresult rv = NS_OK;

  if (!aPrinterName && !aUPrinterName) return rv;

  PRBool toFile;
  CheckForPrintToFileWithName(aPrinterName?aPrinterName:aUPrinterName, toFile);
  
  
  if (!toFile) {
    nsXPIDLString toFileName;
    aPS->GetToFileName(getter_Copies(toFileName));
    if (toFileName) {
      if (*toFileName) {
        if (toFileName.EqualsLiteral("FILE:")) {
          
          
          return NS_OK; 
        }
      }
    }
  }
  aPS->SetPrintToFile(toFile);
  if (toFile) {
    rv = GetFileNameForPrintSettings(aPS);
  }
  return rv;
}


NS_IMETHODIMP nsDeviceContextSpecWin::Init(nsIWidget* aWidget, 
                                           nsIPrintSettings* aPrintSettings,
                                           PRBool aIsPrintPreview)
{
  mPrintSettings = aPrintSettings;

  nsresult rv = aIsPrintPreview ? NS_ERROR_GFX_PRINTER_PRINTPREVIEW : 
                                  NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE;
  if (aPrintSettings) {
    nsCOMPtr<nsIPrintSettingsWin> psWin(do_QueryInterface(aPrintSettings));
    if (psWin) {
      PRUnichar* deviceName;
      PRUnichar* driverName;
      psWin->GetDeviceName(&deviceName); 
      psWin->GetDriverName(&driverName); 

      LPDEVMODEW devMode;
      psWin->GetDevMode(&devMode);       

      if (deviceName && driverName && devMode) {
        
        
        if (devMode->dmFields & DM_SCALE) {
          double scale = double(devMode->dmScale) / 100.0f;
          if (scale != 1.0) {
            aPrintSettings->SetScaling(scale);
            devMode->dmScale = 100;
          }
        }

        SetDeviceName(deviceName);
        SetDriverName(driverName);
        SetDevMode(devMode);

        if (!aIsPrintPreview) {
          rv = CheckForPrintToFile(mPrintSettings, deviceName, nsnull);
          if (NS_FAILED(rv)) {
            nsCRT::free(deviceName);
            nsCRT::free(driverName);
            return NS_ERROR_FAILURE;
          }
        }

        
        nsCRT::free(deviceName);
        nsCRT::free(driverName);

        return NS_OK;
      } else {
        PR_PL(("***** nsDeviceContextSpecWin::Init - deviceName/driverName/devMode was NULL!\n"));
        if (deviceName) nsCRT::free(deviceName);
        if (driverName) nsCRT::free(driverName);
        if (devMode) ::HeapFree(::GetProcessHeap(), 0, devMode);
      }
    }
  } else {
    PR_PL(("***** nsDeviceContextSpecWin::Init - aPrintSettingswas NULL!\n"));
  }

  LPDEVMODEW pDevMode  = NULL;
  HGLOBAL   hDevNames = NULL;

  
  PRUnichar * printerName;
  mPrintSettings->GetPrinterName(&printerName);

  
  if (!printerName || (printerName && !*printerName)) {
    printerName = GetDefaultPrinterNameFromGlobalPrinters();
  }

  NS_ASSERTION(printerName, "We have to have a printer name");
  if (!printerName || !*printerName) return rv;

  if (!aIsPrintPreview) {
    CheckForPrintToFile(mPrintSettings, nsnull, printerName);
  }
 
  return GetDataFromPrinter(printerName, mPrintSettings);
}



static void CleanAndCopyString(PRUnichar*& aStr, const PRUnichar* aNewStr)
{
  if (aStr != nsnull) {
    if (aNewStr != nsnull && wcslen(aStr) > wcslen(aNewStr)) { 
      wcscpy(aStr, aNewStr);
      return;
    } else {
      PR_Free(aStr);
      aStr = nsnull;
    }
  }

  if (nsnull != aNewStr) {
    aStr = (PRUnichar *)PR_Malloc(sizeof(PRUnichar)*(wcslen(aNewStr) + 1));
    wcscpy(aStr, aNewStr);
  }
}

NS_IMETHODIMP nsDeviceContextSpecWin::GetSurfaceForPrinter(gfxASurface **surface)
{
  NS_ASSERTION(mDevMode, "DevMode can't be NULL here");

  nsRefPtr<gfxASurface> newSurface;

  PRInt16 outputFormat;
  mPrintSettings->GetOutputFormat(&outputFormat);

  if (outputFormat == nsIPrintSettings::kOutputFormatPDF) {
    nsXPIDLString filename;
    mPrintSettings->GetToFileName(getter_Copies(filename));

    double width, height;
    mPrintSettings->GetEffectivePageSize(&width, &height);
    
    width  /= TWIPS_PER_POINT_FLOAT;
    height /= TWIPS_PER_POINT_FLOAT;

    nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    nsresult rv = file->InitWithPath(filename);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIFileOutputStream> stream = do_CreateInstance("@mozilla.org/network/file-output-stream;1");
    rv = stream->Init(file, -1, -1, 0);
    if (NS_FAILED(rv))
      return rv;

    newSurface = new gfxPDFSurface(stream, gfxSize(width, height));
  } else {
    if (mDevMode) {
      HDC dc = ::CreateDCW(mDriverName, mDeviceName, NULL, mDevMode);

      
      newSurface = new gfxWindowsSurface(dc, gfxWindowsSurface::FLAG_TAKE_DC | gfxWindowsSurface::FLAG_FOR_PRINTING);
    }
  }

  if (newSurface) {
    *surface = newSurface;
    NS_ADDREF(*surface);
    return NS_OK;
  }

  *surface = nsnull;
  return NS_ERROR_FAILURE;
}


void nsDeviceContextSpecWin::SetDeviceName(const PRUnichar* aDeviceName)
{
  CleanAndCopyString(mDeviceName, aDeviceName);
}


void nsDeviceContextSpecWin::SetDriverName(const PRUnichar* aDriverName)
{
  CleanAndCopyString(mDriverName, aDriverName);
}


void nsDeviceContextSpecWin::SetDevMode(LPDEVMODEW aDevMode)
{
  if (mDevMode) {
    ::HeapFree(::GetProcessHeap(), 0, mDevMode);
  }

  mDevMode = aDevMode;
}


void 
nsDeviceContextSpecWin::GetDevMode(LPDEVMODEW &aDevMode)
{
  aDevMode = mDevMode;
}



static void 
MapPaperSizeToNativeEnum(LPDEVMODEW aDevMode,
                         PRInt16   aType, 
                         double    aW, 
                         double    aH)
{

#ifdef DEBUG_rods
  BOOL doingOrientation = aDevMode->dmFields & DM_ORIENTATION;
  BOOL doingPaperSize   = aDevMode->dmFields & DM_PAPERSIZE;
  BOOL doingPaperLength = aDevMode->dmFields & DM_PAPERLENGTH;
  BOOL doingPaperWidth  = aDevMode->dmFields & DM_PAPERWIDTH;
#endif

  PRBool foundEnum = PR_FALSE;
  for (PRInt32 i=0;i<kNumPaperSizes;i++) {
    if (kPaperSizes[i].mWidth == aW && kPaperSizes[i].mHeight == aH) {
      aDevMode->dmPaperSize = kPaperSizes[i].mPaperSize;
      aDevMode->dmFields &= ~DM_PAPERLENGTH;
      aDevMode->dmFields &= ~DM_PAPERWIDTH;
      aDevMode->dmFields |= DM_PAPERSIZE;
      return;
    }
  }

  short width  = 0;
  short height = 0;
  if (aType == nsIPrintSettings::kPaperSizeInches) {
    width  = short(NS_TWIPS_TO_MILLIMETERS(NS_INCHES_TO_TWIPS(float(aW))) / 10);
    height = short(NS_TWIPS_TO_MILLIMETERS(NS_INCHES_TO_TWIPS(float(aH))) / 10);

  } else if (aType == nsIPrintSettings::kPaperSizeMillimeters) {
    width  = short(aW / 10.0);
    height = short(aH / 10.0);
  } else {
    return; 
  }

  
  aDevMode->dmPaperSize   = 0;
  aDevMode->dmPaperWidth  = width;
  aDevMode->dmPaperLength = height;

  aDevMode->dmFields |= DM_PAPERSIZE;
  aDevMode->dmFields |= DM_PAPERLENGTH;
  aDevMode->dmFields |= DM_PAPERWIDTH;
}




static void 
SetupDevModeFromSettings(LPDEVMODEW aDevMode, nsIPrintSettings* aPrintSettings)
{
  
  if (aPrintSettings) {
    PRInt16 type;
    aPrintSettings->GetPaperSizeType(&type);
    if (type == nsIPrintSettings::kPaperSizeNativeData) {
      PRInt16 paperEnum;
      aPrintSettings->GetPaperData(&paperEnum);
      aDevMode->dmPaperSize = paperEnum;
      aDevMode->dmFields &= ~DM_PAPERLENGTH;
      aDevMode->dmFields &= ~DM_PAPERWIDTH;
      aDevMode->dmFields |= DM_PAPERSIZE;
    } else {
      PRInt16 unit;
      double width, height;
      aPrintSettings->GetPaperSizeUnit(&unit);
      aPrintSettings->GetPaperWidth(&width);
      aPrintSettings->GetPaperHeight(&height);
      MapPaperSizeToNativeEnum(aDevMode, unit, width, height);
    }

    
    PRInt32 orientation;
    aPrintSettings->GetOrientation(&orientation);
    aDevMode->dmOrientation = orientation == nsIPrintSettings::kPortraitOrientation?DMORIENT_PORTRAIT:DMORIENT_LANDSCAPE;
    aDevMode->dmFields |= DM_ORIENTATION;

    
    PRInt32 copies;
    aPrintSettings->GetNumCopies(&copies);
    aDevMode->dmCopies = copies;
    aDevMode->dmFields |= DM_COPIES;
  }

}

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
static void DisplayLastError()
{
  LPVOID lpMsgBuf;
  DWORD errCode = GetLastError();

  FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
      (LPTSTR) &lpMsgBuf,
      0,
      NULL 
  );

  
  MessageBox( NULL, (const char *)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );
}
#define DISPLAY_LAST_ERROR DisplayLastError();
#else
#define DISPLAY_LAST_ERROR 
#endif



nsresult
nsDeviceContextSpecWin::GetDataFromPrinter(const PRUnichar * aName, nsIPrintSettings* aPS)
{
#ifdef WINCE 
  return NS_ERROR_NOT_IMPLEMENTED;
#else
  nsresult rv = NS_ERROR_FAILURE;

  if (!GlobalPrinters::GetInstance()->PrintersAreAllocated()) {
    rv = GlobalPrinters::GetInstance()->EnumeratePrinterList();
    if (NS_FAILED(rv)) {
      PR_PL(("***** nsDeviceContextSpecWin::GetDataFromPrinter - Couldn't enumerate printers!\n"));
      DISPLAY_LAST_ERROR
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  HANDLE hPrinter = NULL;
  
  BOOL status = ::OpenPrinterW((LPWSTR)(aName),
                              &hPrinter, NULL);
  if (status) {

    LPDEVMODEW   pDevMode;
    DWORD       dwNeeded, dwRet;

    
    dwNeeded = ::DocumentPropertiesW(NULL, hPrinter,
                                    const_cast<wchar_t*>(aName),
                                    NULL, NULL, 0);

    pDevMode = (LPDEVMODEW)::HeapAlloc (::GetProcessHeap(), HEAP_ZERO_MEMORY, dwNeeded);
    if (!pDevMode) return NS_ERROR_FAILURE;

    
    dwRet = DocumentPropertiesW(NULL, hPrinter, 
                               const_cast<wchar_t*>(aName),
                               pDevMode, NULL, DM_OUT_BUFFER);

    if (dwRet == IDOK && aPS) {
      SetupDevModeFromSettings(pDevMode, aPS);
      
      dwRet = ::DocumentPropertiesW(NULL, hPrinter,
                                   const_cast<wchar_t*>(aName),
                                   pDevMode, pDevMode,
                                   DM_IN_BUFFER | DM_OUT_BUFFER);
    }

    if (dwRet != IDOK) {
      ::HeapFree(::GetProcessHeap(), 0, pDevMode);
      ::ClosePrinter(hPrinter);
      PR_PL(("***** nsDeviceContextSpecWin::GetDataFromPrinter - DocumentProperties call failed code: %d/0x%x\n", dwRet, dwRet));
      DISPLAY_LAST_ERROR
      return NS_ERROR_FAILURE;
    }

    SetDevMode(pDevMode); 

    SetDeviceName(aName);

    SetDriverName(L"WINSPOOL");

    ::ClosePrinter(hPrinter);
    rv = NS_OK;
  } else {
    rv = NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND;
    PR_PL(("***** nsDeviceContextSpecWin::GetDataFromPrinter - Couldn't open printer: [%s]\n", NS_ConvertUTF16toUTF8(aName).get()));
    DISPLAY_LAST_ERROR
  }
  return rv;
#endif 
}







void 
nsDeviceContextSpecWin::SetupPaperInfoFromSettings()
{
  LPDEVMODEW devMode;

  GetDevMode(devMode);
  NS_ASSERTION(devMode, "DevMode can't be NULL here");
  if (devMode) {
    SetupDevModeFromSettings(devMode, mPrintSettings);
  }
}



nsresult 
nsDeviceContextSpecWin::SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, 
                                                    LPDEVMODEW         aDevMode)
{
  if (aPrintSettings == nsnull) {
    return NS_ERROR_FAILURE;
  }
  aPrintSettings->SetIsInitializedFromPrinter(PR_TRUE);

  BOOL doingNumCopies   = aDevMode->dmFields & DM_COPIES;
  BOOL doingOrientation = aDevMode->dmFields & DM_ORIENTATION;
  BOOL doingPaperSize   = aDevMode->dmFields & DM_PAPERSIZE;
  BOOL doingPaperLength = aDevMode->dmFields & DM_PAPERLENGTH;
  BOOL doingPaperWidth  = aDevMode->dmFields & DM_PAPERWIDTH;

  if (doingOrientation) {
    PRInt32 orientation  = aDevMode->dmOrientation == DMORIENT_PORTRAIT?
      nsIPrintSettings::kPortraitOrientation:nsIPrintSettings::kLandscapeOrientation;
    aPrintSettings->SetOrientation(orientation);
  }

  
  if (doingNumCopies) {
    aPrintSettings->SetNumCopies(PRInt32(aDevMode->dmCopies));
  }

  if (aDevMode->dmFields & DM_SCALE) {
    double scale = double(aDevMode->dmScale) / 100.0f;
    if (scale != 1.0) {
      aPrintSettings->SetScaling(scale);
      aDevMode->dmScale = 100;
      
      
    }
  }

  if (doingPaperSize) {
    aPrintSettings->SetPaperSizeType(nsIPrintSettings::kPaperSizeNativeData);
    aPrintSettings->SetPaperData(aDevMode->dmPaperSize);
    for (PRInt32 i=0;i<kNumPaperSizes;i++) {
      if (kPaperSizes[i].mPaperSize == aDevMode->dmPaperSize) {
        aPrintSettings->SetPaperSizeUnit(kPaperSizes[i].mIsInches?nsIPrintSettings::kPaperSizeInches:nsIPrintSettings::kPaperSizeMillimeters);
        break;
      }
    }

  } else if (doingPaperLength && doingPaperWidth) {
    PRBool found = PR_FALSE;
    for (PRInt32 i=0;i<kNumPaperSizes;i++) {
      if (kPaperSizes[i].mPaperSize == aDevMode->dmPaperSize) {
        aPrintSettings->SetPaperSizeType(nsIPrintSettings::kPaperSizeDefined);
        aPrintSettings->SetPaperWidth(kPaperSizes[i].mWidth);
        aPrintSettings->SetPaperHeight(kPaperSizes[i].mHeight);
        aPrintSettings->SetPaperSizeUnit(kPaperSizes[i].mIsInches?nsIPrintSettings::kPaperSizeInches:nsIPrintSettings::kPaperSizeMillimeters);
        found = PR_TRUE;
        break;
      }
    }
    if (!found) {
      return NS_ERROR_FAILURE;
    }
  } else {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}




nsPrinterEnumeratorWin::nsPrinterEnumeratorWin()
{
}

nsPrinterEnumeratorWin::~nsPrinterEnumeratorWin()
{
  
  
}

NS_IMPL_ISUPPORTS1(nsPrinterEnumeratorWin, nsIPrinterEnumerator)




NS_IMETHODIMP 
nsPrinterEnumeratorWin::GetDefaultPrinterName(PRUnichar * *aDefaultPrinterName)
{
  NS_ENSURE_ARG_POINTER(aDefaultPrinterName);

  *aDefaultPrinterName = GetDefaultPrinterNameFromGlobalPrinters(); 

  return NS_OK;
}


NS_IMETHODIMP 
nsPrinterEnumeratorWin::InitPrintSettingsFromPrinter(const PRUnichar *aPrinterName, nsIPrintSettings *aPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aPrinterName);
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  if (!*aPrinterName) {
    return NS_OK;
  }

  nsRefPtr<nsDeviceContextSpecWin> devSpecWin = new nsDeviceContextSpecWin();
  if (!devSpecWin) return NS_ERROR_OUT_OF_MEMORY;

  if (NS_FAILED(GlobalPrinters::GetInstance()->EnumeratePrinterList())) {
    return NS_ERROR_FAILURE;
  }

  devSpecWin->GetDataFromPrinter(aPrinterName);

  LPDEVMODEW devmode;
  devSpecWin->GetDevMode(devmode);
  NS_ASSERTION(devmode, "DevMode can't be NULL here");
  if (devmode) {
    aPrintSettings->SetPrinterName(aPrinterName);
    nsDeviceContextSpecWin::SetPrintSettingsFromDevMode(aPrintSettings, devmode);
  }

  
  GlobalPrinters::GetInstance()->FreeGlobalPrinters();
  return NS_OK;
}





NS_IMETHODIMP 
nsPrinterEnumeratorWin::GetPrinterNameList(nsIStringEnumerator **aPrinterNameList)
{
  NS_ENSURE_ARG_POINTER(aPrinterNameList);
  *aPrinterNameList = nsnull;

  nsresult rv = GlobalPrinters::GetInstance()->EnumeratePrinterList();
  if (NS_FAILED(rv)) {
    PR_PL(("***** nsDeviceContextSpecWin::GetPrinterNameList - Couldn't enumerate printers!\n"));
    return rv;
  }

  PRInt32 numPrinters = GlobalPrinters::GetInstance()->GetNumPrinters();
  nsTArray<nsString> *printers = new nsTArray<nsString>(numPrinters);
  if (!printers)
    return NS_ERROR_OUT_OF_MEMORY;

  PRInt32 printerInx = 0;
  while( printerInx < numPrinters ) {
    LPTSTR name = GlobalPrinters::GetInstance()->GetItemFromList(printerInx++);
#ifdef UNICODE
    nsDependentString newName(name);
#else
    nsAutoString newName; 
    NS_CopyNativeToUnicode(nsDependentCString(name), newName);
#endif
    printers->AppendElement(newName);
  }

  return NS_NewAdoptingStringEnumerator(aPrinterNameList, printers);
}



NS_IMETHODIMP nsPrinterEnumeratorWin::DisplayPropertiesDlg(const PRUnichar *aPrinterName, nsIPrintSettings* aPrintSettings)
{
  
  return NS_OK;
}







void 
GlobalPrinters::ReallocatePrinters()
{
  if (PrintersAreAllocated()) {
    FreeGlobalPrinters();
  }
  mPrinters = new nsTArray<LPTSTR>();
  NS_ASSERTION(mPrinters, "Printers Array is NULL!");
}


void 
GlobalPrinters::FreeGlobalPrinters()
{
  if (mPrinters != nsnull) {
    for (int i=0;i<mPrinters->Length();i++) {
      free(mPrinters->ElementAt(i));
    }
    delete mPrinters;
    mPrinters = nsnull;
  }
}


nsresult 
GlobalPrinters::EnumerateNativePrinters()
{
  nsresult rv = NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE;
#ifndef WINCE
  PR_PL(("-----------------------\n"));
  PR_PL(("EnumerateNativePrinters\n"));

  TCHAR szDefaultPrinterName[1024];    
  DWORD status = GetProfileString(TEXT("devices"), 0, TEXT(","),
                                  szDefaultPrinterName,
                                  NS_ARRAY_LENGTH(szDefaultPrinterName));
  if (status > 0) {
    DWORD count = 0;
    LPTSTR sPtr   = (LPTSTR)szDefaultPrinterName;
    LPTSTR ePtr   = (LPTSTR)(szDefaultPrinterName+(status*sizeof(TCHAR)));
    LPTSTR prvPtr = sPtr;
    while (sPtr < ePtr) {
      if (*sPtr == NULL) {
        LPTSTR name = _tcsdup(prvPtr);
        mPrinters->AppendElement(name);
        PR_PL(("Printer Name:    %s\n", prvPtr));
        prvPtr = sPtr+1;
        count++;
      }
      sPtr++;
    }
    rv = NS_OK;
  }
  PR_PL(("-----------------------\n"));
#endif
  return rv;
}



void 
GlobalPrinters::GetDefaultPrinterName(LPTSTR& aDefaultPrinterName)
{
#ifndef WINCE
  aDefaultPrinterName = nsnull;
  TCHAR szDefaultPrinterName[1024];    
  DWORD status = GetProfileString(TEXT("windows"), TEXT("device"), 0,
                                  szDefaultPrinterName,
                                  NS_ARRAY_LENGTH(szDefaultPrinterName));
  if (status > 0) {
    TCHAR comma = (TCHAR)',';
    LPTSTR sPtr = (LPTSTR)szDefaultPrinterName;
    while (*sPtr != comma && *sPtr != NULL) 
      sPtr++;
    if (*sPtr == comma) {
      *sPtr = NULL;
    }
    aDefaultPrinterName = _tcsdup(szDefaultPrinterName);
  } else {
    aDefaultPrinterName = _tcsdup(TEXT(""));
  }

  PR_PL(("DEFAULT PRINTER [%s]\n", aDefaultPrinterName));
#else
  aDefaultPrinterName = TEXT("UNKNOWN");
#endif
}




nsresult 
GlobalPrinters::EnumeratePrinterList()
{
  
  
  ReallocatePrinters();

  
  
  nsresult rv = EnumerateNativePrinters();
  if (NS_FAILED(rv)) return rv;

  
  LPTSTR defPrinterName;
  GetDefaultPrinterName(defPrinterName);

  
  if (defPrinterName != nsnull) {
    for (PRInt32 i=0;i<mPrinters->Length();i++) {
      LPTSTR name = mPrinters->ElementAt(i);
      if (!_tcscmp(name, defPrinterName)) {
        if (i > 0) {
          LPTSTR ptr = mPrinters->ElementAt(0);
          mPrinters->ElementAt(0) = name;
          mPrinters->ElementAt(i) = ptr;
        }
        break;
      }
    }
    free(defPrinterName);
  }

  
  if (!PrintersAreAllocated()) {
    PR_PL(("***** nsDeviceContextSpecWin::EnumeratePrinterList - Printers aren`t allocated\n"));
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

