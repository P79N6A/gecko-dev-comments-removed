





















#define NOMINMAX 1

#include "plstr.h"
#include <windows.h>
#include <tchar.h>

#include <unknwn.h>
#include <commdlg.h>

#include "nsIWebBrowserPrint.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsIPrintSettings.h"
#include "nsIPrintSettingsWin.h"
#include "nsIPrintOptions.h"

#include "nsRect.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "nsCRT.h"
#include "prenv.h" 

#include <windows.h>
#include <winspool.h> 


#include "nsIStringBundle.h"


#include "nsNativeCharsetUtils.h"


#include <dlgs.h>


static const char* kAsLaidOutOnScreenStr = "As &laid out on the screen";
static const char* kTheSelectedFrameStr  = "The selected &frame";
static const char* kEachFrameSeparately  = "&Each frame separately";






static UINT gFrameSelectedRadioBtn = 0;


static bool gDialogWasExtended     = false;

#define PRINTDLG_PROPERTIES "chrome://global/locale/printdialog.properties"

static HWND gParentWnd = nullptr;




typedef struct {
  short  mPaperSize; 
  double mWidth;
  double mHeight;
  bool mIsInches;
} NativePaperSizes;


const NativePaperSizes kPaperSizes[] = {
  {DMPAPER_LETTER,    8.5,   11.0,  true},
  {DMPAPER_LEGAL,     8.5,   14.0,  true},
  {DMPAPER_A4,        210.0, 297.0, false},
  {DMPAPER_TABLOID,   11.0,  17.0,  true},
  {DMPAPER_LEDGER,    17.0,  11.0,  true},
  {DMPAPER_STATEMENT, 5.5,   8.5,   true},
  {DMPAPER_EXECUTIVE, 7.25,  10.5,  true},
  {DMPAPER_A3,        297.0, 420.0, false},
  {DMPAPER_A5,        148.0, 210.0, false},
  {DMPAPER_CSHEET,    17.0,  22.0,  true},  
  {DMPAPER_DSHEET,    22.0,  34.0,  true},  
  {DMPAPER_ESHEET,    34.0,  44.0,  true},  
  {DMPAPER_LETTERSMALL, 8.5, 11.0,  true},  
  {DMPAPER_A4SMALL,   210.0, 297.0, false}, 
  {DMPAPER_B4,        250.0, 354.0, false}, 
  {DMPAPER_B5,        182.0, 257.0, false},
  {DMPAPER_FOLIO,     8.5,   13.0,  true},
  {DMPAPER_QUARTO,    215.0, 275.0, false},
  {DMPAPER_10X14,     10.0,  14.0,  true},
  {DMPAPER_11X17,     11.0,  17.0,  true},
  {DMPAPER_NOTE,      8.5,   11.0,  true},  
  {DMPAPER_ENV_9,     3.875, 8.875, true},  
  {DMPAPER_ENV_10,    40.125, 9.5,  true},  
  {DMPAPER_ENV_11,    4.5,   10.375, true},  
  {DMPAPER_ENV_12,    4.75,  11.0,  true},  
  {DMPAPER_ENV_14,    5.0,   11.5,  true},  
  {DMPAPER_ENV_DL,    110.0, 220.0, false}, 
  {DMPAPER_ENV_C5,    162.0, 229.0, false}, 
  {DMPAPER_ENV_C3,    324.0, 458.0, false}, 
  {DMPAPER_ENV_C4,    229.0, 324.0, false}, 
  {DMPAPER_ENV_C6,    114.0, 162.0, false}, 
  {DMPAPER_ENV_C65,   114.0, 229.0, false}, 
  {DMPAPER_ENV_B4,    250.0, 353.0, false}, 
  {DMPAPER_ENV_B5,    176.0, 250.0, false}, 
  {DMPAPER_ENV_B6,    176.0, 125.0, false}, 
  {DMPAPER_ENV_ITALY, 110.0, 230.0, false}, 
  {DMPAPER_ENV_MONARCH,  3.875,  7.5, true},  
  {DMPAPER_ENV_PERSONAL, 3.625,  6.5, true},  
  {DMPAPER_FANFOLD_US,   14.875, 11.0, true},  
  {DMPAPER_FANFOLD_STD_GERMAN, 8.5, 12.0, true},  
  {DMPAPER_FANFOLD_LGL_GERMAN, 8.5, 13.0, true},  
};
const int32_t kNumPaperSizes = 41;



static void 
MapPaperSizeToNativeEnum(LPDEVMODEW aDevMode,
                         int16_t   aType, 
                         double    aW, 
                         double    aH)
{

#ifdef DEBUG_rods
  BOOL doingOrientation = aDevMode->dmFields & DM_ORIENTATION;
  BOOL doingPaperSize   = aDevMode->dmFields & DM_PAPERSIZE;
  BOOL doingPaperLength = aDevMode->dmFields & DM_PAPERLENGTH;
  BOOL doingPaperWidth  = aDevMode->dmFields & DM_PAPERWIDTH;
#endif

  const double kThreshold = 0.05;
  for (int32_t i=0;i<kNumPaperSizes;i++) {
    double width  = kPaperSizes[i].mWidth;
    double height = kPaperSizes[i].mHeight;
    if (aW < width+kThreshold && aW > width-kThreshold && 
        aH < height+kThreshold && aH > height-kThreshold) {
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
    int16_t type;
    aPrintSettings->GetPaperSizeType(&type);
    if (type == nsIPrintSettings::kPaperSizeNativeData) {
      int16_t paperEnum;
      aPrintSettings->GetPaperData(&paperEnum);
      aDevMode->dmPaperSize = paperEnum;
      aDevMode->dmFields &= ~DM_PAPERLENGTH;
      aDevMode->dmFields &= ~DM_PAPERWIDTH;
      aDevMode->dmFields |= DM_PAPERSIZE;
    } else {
      int16_t unit;
      double width, height;
      aPrintSettings->GetPaperSizeUnit(&unit);
      aPrintSettings->GetPaperWidth(&width);
      aPrintSettings->GetPaperHeight(&height);
      MapPaperSizeToNativeEnum(aDevMode, unit, width, height);
    }

    
    int32_t orientation;
    aPrintSettings->GetOrientation(&orientation);
    aDevMode->dmOrientation = orientation == nsIPrintSettings::kPortraitOrientation?DMORIENT_PORTRAIT:DMORIENT_LANDSCAPE;
    aDevMode->dmFields |= DM_ORIENTATION;

    
    int32_t copies;
    aPrintSettings->GetNumCopies(&copies);
    aDevMode->dmCopies = copies;
    aDevMode->dmFields |= DM_COPIES;

  }

}



static nsresult 
SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, 
                            LPDEVMODEW         aDevMode)
{
  if (aPrintSettings == nullptr) {
    return NS_ERROR_FAILURE;
  }

  aPrintSettings->SetIsInitializedFromPrinter(true);
  if (aDevMode->dmFields & DM_ORIENTATION) {
    int32_t orientation  = aDevMode->dmOrientation == DMORIENT_PORTRAIT?
                           nsIPrintSettings::kPortraitOrientation:nsIPrintSettings::kLandscapeOrientation;
    aPrintSettings->SetOrientation(orientation);
  }

  
  if (aDevMode->dmFields & DM_COPIES) {
    aPrintSettings->SetNumCopies(int32_t(aDevMode->dmCopies));
  }

  
  
  if (aDevMode->dmFields & DM_SCALE) {
    double origScale = 1.0;
    aPrintSettings->GetScaling(&origScale);
    double scale = double(aDevMode->dmScale) / 100.0f;
    if (origScale == 1.0 || scale != 1.0) {
      aPrintSettings->SetScaling(scale);
    }
    aDevMode->dmScale = 100;
    
    
  }

  if (aDevMode->dmFields & DM_PAPERSIZE) {
    aPrintSettings->SetPaperSizeType(nsIPrintSettings::kPaperSizeNativeData);
    aPrintSettings->SetPaperData(aDevMode->dmPaperSize);
    for (int32_t i=0;i<kNumPaperSizes;i++) {
      if (kPaperSizes[i].mPaperSize == aDevMode->dmPaperSize) {
        aPrintSettings->SetPaperSizeUnit(kPaperSizes[i].mIsInches?nsIPrintSettings::kPaperSizeInches:nsIPrintSettings::kPaperSizeMillimeters);
        break;
      }
    }

  } else if (aDevMode->dmFields & DM_PAPERLENGTH && aDevMode->dmFields & DM_PAPERWIDTH) {
    bool found = false;
    for (int32_t i=0;i<kNumPaperSizes;i++) {
      if (kPaperSizes[i].mPaperSize == aDevMode->dmPaperSize) {
        aPrintSettings->SetPaperSizeType(nsIPrintSettings::kPaperSizeDefined);
        aPrintSettings->SetPaperWidth(kPaperSizes[i].mWidth);
        aPrintSettings->SetPaperHeight(kPaperSizes[i].mHeight);
        aPrintSettings->SetPaperSizeUnit(kPaperSizes[i].mIsInches?nsIPrintSettings::kPaperSizeInches:nsIPrintSettings::kPaperSizeMillimeters);
        found = true;
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



static nsresult
GetLocalizedBundle(const char * aPropFileName, nsIStringBundle** aStrBundle)
{
  NS_ENSURE_ARG_POINTER(aPropFileName);
  NS_ENSURE_ARG_POINTER(aStrBundle);

  nsresult rv;
  nsCOMPtr<nsIStringBundle> bundle;
  

  
  nsCOMPtr<nsIStringBundleService> stringService = 
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv) && stringService) {
    rv = stringService->CreateBundle(aPropFileName, aStrBundle);
  }
  
  return rv;
}



static nsresult
GetLocalizedString(nsIStringBundle* aStrBundle, const char* aKey, nsString& oVal)
{
  NS_ENSURE_ARG_POINTER(aStrBundle);
  NS_ENSURE_ARG_POINTER(aKey);

  
  nsXPIDLString valUni;
  nsAutoString key; 
  key.AssignWithConversion(aKey);
  nsresult rv = aStrBundle->GetStringFromName(key.get(), getter_Copies(valUni));
  if (NS_SUCCEEDED(rv) && valUni) {
    oVal.Assign(valUni);
  } else {
    oVal.Truncate();
  }
  return rv;
}



static void SetTextOnWnd(HWND aControl, const nsString& aStr)
{
  nsAutoCString text;
  if (NS_SUCCEEDED(NS_CopyUnicodeToNative(aStr, text))) {
    ::SetWindowText(aControl, text.get());
  }
}



static void SetText(HWND             aParent, 
                    UINT             aId, 
                    nsIStringBundle* aStrBundle,
                    const char*      aKey) 
{
  HWND wnd = GetDlgItem (aParent, aId);
  if (!wnd) {
    return;
  }
  nsAutoString str;
  nsresult rv = GetLocalizedString(aStrBundle, aKey, str);
  if (NS_SUCCEEDED(rv)) {
    SetTextOnWnd(wnd, str);
  }
}


static void SetRadio(HWND         aParent, 
                     UINT         aId, 
                     bool         aIsSet,
                     bool         isEnabled = true) 
{
  HWND wnd = ::GetDlgItem (aParent, aId);
  if (!wnd) {
    return;
  }
  if (!isEnabled) {
    ::EnableWindow(wnd, FALSE);
    return;
  }
  ::EnableWindow(wnd, TRUE);
  ::SendMessage(wnd, BM_SETCHECK, (WPARAM)aIsSet, (LPARAM)0);
}


static void SetRadioOfGroup(HWND aDlg, int aRadId)
{
  int radioIds[] = {rad4, rad5, rad6};
  int numRads = 3;

  for (int i=0;i<numRads;i++) {
    HWND radWnd = ::GetDlgItem(aDlg, radioIds[i]);
    if (radWnd != nullptr) {
      ::SendMessage(radWnd, BM_SETCHECK, (WPARAM)(radioIds[i] == aRadId), (LPARAM)0);
    }
  }
}


typedef struct {
  const char * mKeyStr;
  long   mKeyId;
} PropKeyInfo;



static PropKeyInfo gAllPropKeys[] = {
    {"printFramesTitleWindows", grp3},
    {"asLaidOutWindows", rad4},
    {"selectedFrameWindows", rad5},
    {"separateFramesWindows", rad6},
    {nullptr, 0}};







static void GetLocalRect(HWND aWnd, RECT& aRect, HWND aParent)
{
  ::GetWindowRect(aWnd, &aRect);

  
  
  ::MapWindowPoints(nullptr, aParent, (LPPOINT)&aRect, 2);
}



static void Show(HWND aWnd, bool bState)
{
  if (aWnd) {
    ::ShowWindow(aWnd, bState?SW_SHOW:SW_HIDE);
  }
}



static HWND CreateControl(LPCTSTR          aType,
                          DWORD            aStyle,
                          HINSTANCE        aHInst, 
                          HWND             aHdlg, 
                          int              aId, 
                          const nsAString& aStr, 
                          const nsIntRect& aRect)
{
  nsAutoCString str;
  if (NS_FAILED(NS_CopyUnicodeToNative(aStr, str)))
    return nullptr;

  HWND hWnd = ::CreateWindow (aType, str.get(),
                              WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | aStyle,
                              aRect.x, aRect.y, aRect.width, aRect.height,
                              (HWND)aHdlg, (HMENU)(intptr_t)aId,
                              aHInst, nullptr);
  if (hWnd == nullptr) return nullptr;

  
  
  HFONT hFont = (HFONT)::SendMessage(aHdlg, WM_GETFONT, (WPARAM)0, (LPARAM)0);
  if (hFont != nullptr) {
    ::SendMessage(hWnd, WM_SETFONT, (WPARAM) hFont, (LPARAM)0);
  }
  return hWnd;
}



static HWND CreateRadioBtn(HINSTANCE        aHInst, 
                           HWND             aHdlg, 
                           int              aId, 
                           const char*      aStr, 
                           const nsIntRect& aRect)
{
  nsString cStr;
  cStr.AssignWithConversion(aStr);
  return CreateControl("BUTTON", BS_RADIOBUTTON, aHInst, aHdlg, aId, cStr, aRect);
}



static HWND CreateGroupBox(HINSTANCE        aHInst, 
                           HWND             aHdlg, 
                           int              aId, 
                           const nsAString& aStr, 
                           const nsIntRect& aRect)
{
  return CreateControl("BUTTON", BS_GROUPBOX, aHInst, aHdlg, aId, aStr, aRect);
}



static void InitializeExtendedDialog(HWND hdlg, int16_t aHowToEnableFrameUI) 
{
  NS_ABORT_IF_FALSE(aHowToEnableFrameUI != nsIPrintSettings::kFrameEnableNone,
                    "should not be called");

  
  nsCOMPtr<nsIStringBundle> strBundle;
  if (NS_SUCCEEDED(GetLocalizedBundle(PRINTDLG_PROPERTIES, getter_AddRefs(strBundle)))) {
    int32_t i = 0;
    while (gAllPropKeys[i].mKeyStr != nullptr) {
      SetText(hdlg, gAllPropKeys[i].mKeyId, strBundle, gAllPropKeys[i].mKeyStr);
      i++;
    }
  }

  
  if (aHowToEnableFrameUI == nsIPrintSettings::kFrameEnableAll) {
    SetRadio(hdlg, rad4, false);  
    SetRadio(hdlg, rad5, true); 
    SetRadio(hdlg, rad6, false);
    
    gFrameSelectedRadioBtn = rad5;

  } else { 
    SetRadio(hdlg, rad4, false);  
    SetRadio(hdlg, rad5, false, false); 
    SetRadio(hdlg, rad6, true);
    
    gFrameSelectedRadioBtn = rad6;
  }
}




static UINT CALLBACK PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) 
{

  if (uiMsg == WM_COMMAND) {
    UINT id = LOWORD(wParam);
    if (id == rad4 || id == rad5 || id == rad6) {
      gFrameSelectedRadioBtn = id;
      SetRadioOfGroup(hdlg, id);
    }

  } else if (uiMsg == WM_INITDIALOG) {
    PRINTDLG * printDlg = (PRINTDLG *)lParam;
    if (printDlg == nullptr) return 0L;

    int16_t howToEnableFrameUI = (int16_t)printDlg->lCustData;
    
    
    if (howToEnableFrameUI == nsIPrintSettings::kFrameEnableNone)
      return TRUE;

    HINSTANCE hInst = (HINSTANCE)::GetWindowLongPtr(hdlg, GWLP_HINSTANCE);
    if (hInst == nullptr) return 0L;

    
    
    HWND wnd = ::GetDlgItem(hdlg, grp1);
    if (wnd == nullptr) return 0L;
    RECT dlgRect;
    GetLocalRect(wnd, dlgRect, hdlg);

    wnd = ::GetDlgItem(hdlg, rad1); 
    if (wnd == nullptr) return 0L;
    RECT rad1Rect;
    GetLocalRect(wnd, rad1Rect, hdlg);

    wnd = ::GetDlgItem(hdlg, rad2); 
    if (wnd == nullptr) return 0L;
    RECT rad2Rect;
    GetLocalRect(wnd, rad2Rect, hdlg);

    wnd = ::GetDlgItem(hdlg, rad3); 
    if (wnd == nullptr) return 0L;
    RECT rad3Rect;
    GetLocalRect(wnd, rad3Rect, hdlg);

    HWND okWnd = ::GetDlgItem(hdlg, IDOK);
    if (okWnd == nullptr) return 0L;
    RECT okRect;
    GetLocalRect(okWnd, okRect, hdlg);

    wnd = ::GetDlgItem(hdlg, grp4); 
    if (wnd == nullptr) return 0L;
    RECT prtRect;
    GetLocalRect(wnd, prtRect, hdlg);


    

    int rbGap     = rad3Rect.top - rad1Rect.bottom;     
    int grpBotGap = dlgRect.bottom - rad2Rect.bottom;   
    int grpGap    = dlgRect.top - prtRect.bottom ;      
    int top       = dlgRect.bottom + grpGap;            
    int radHgt    = rad1Rect.bottom - rad1Rect.top + 1; 
    int y         = top+(rad1Rect.top-dlgRect.top);     
    int rbWidth   = dlgRect.right - rad1Rect.left - 5;  
                                                        
    nsIntRect rect;

    
    
    
    
    
    rect.SetRect(rad1Rect.left, y, rbWidth,radHgt);
    HWND rad4Wnd = CreateRadioBtn(hInst, hdlg, rad4, kAsLaidOutOnScreenStr, rect);
    if (rad4Wnd == nullptr) return 0L;
    y += radHgt + rbGap;

    rect.SetRect(rad1Rect.left, y, rbWidth, radHgt);
    HWND rad5Wnd = CreateRadioBtn(hInst, hdlg, rad5, kTheSelectedFrameStr, rect);
    if (rad5Wnd == nullptr) {
      Show(rad4Wnd, FALSE); 
      return 0L;
    }
    y += radHgt + rbGap;

    rect.SetRect(rad1Rect.left, y, rbWidth, radHgt);
    HWND rad6Wnd = CreateRadioBtn(hInst, hdlg, rad6, kEachFrameSeparately, rect);
    if (rad6Wnd == nullptr) {
      Show(rad4Wnd, FALSE); 
      Show(rad5Wnd, FALSE); 
      return 0L;
    }
    y += radHgt + grpBotGap;

    
    rect.SetRect (dlgRect.left, top, dlgRect.right-dlgRect.left+1, y-top+1);
    HWND grpBoxWnd = CreateGroupBox(hInst, hdlg, grp3, NS_LITERAL_STRING("Print Frame"), rect);
    if (grpBoxWnd == nullptr) {
      Show(rad4Wnd, FALSE); 
      Show(rad5Wnd, FALSE); 
      Show(rad6Wnd, FALSE); 
      return 0L;
    }

    
    
    
    RECT pr, cr; 
    ::GetWindowRect(hdlg, &pr);
    ::GetClientRect(hdlg, &cr);

    int dlgHgt = (cr.bottom - cr.top) + 1;
    int bottomGap = dlgHgt - okRect.bottom;
    pr.bottom += (dlgRect.bottom-dlgRect.top) + grpGap + 1 - (dlgHgt-dlgRect.bottom) + bottomGap;

    ::SetWindowPos(hdlg, nullptr, pr.left, pr.top, pr.right-pr.left+1, pr.bottom-pr.top+1, 
                   SWP_NOMOVE|SWP_NOREDRAW|SWP_NOZORDER);

    
    ::GetClientRect(hdlg, &cr);
    dlgHgt = (cr.bottom - cr.top) + 1;
 
    
    int okHgt = okRect.bottom - okRect.top + 1;
    ::SetWindowPos(okWnd, nullptr, okRect.left, dlgHgt-bottomGap-okHgt, 0, 0, 
                   SWP_NOSIZE|SWP_NOREDRAW|SWP_NOZORDER);

    HWND cancelWnd = ::GetDlgItem(hdlg, IDCANCEL);
    if (cancelWnd == nullptr) return 0L;

    RECT cancelRect;
    GetLocalRect(cancelWnd, cancelRect, hdlg);
    int cancelHgt = cancelRect.bottom - cancelRect.top + 1;
    ::SetWindowPos(cancelWnd, nullptr, cancelRect.left, dlgHgt-bottomGap-cancelHgt, 0, 0, 
                   SWP_NOSIZE|SWP_NOREDRAW|SWP_NOZORDER);

    
    InitializeExtendedDialog(hdlg, howToEnableFrameUI);

    
    gDialogWasExtended = true;
    return TRUE;
  }
  return 0L;
}









static HGLOBAL CreateGlobalDevModeAndInit(const nsXPIDLString& aPrintName, nsIPrintSettings* aPS)
{
  HGLOBAL hGlobalDevMode = nullptr;

  HANDLE hPrinter = nullptr;
  
  LPWSTR printName = const_cast<wchar_t*>(static_cast<const wchar_t*>(aPrintName.get()));
  BOOL status = ::OpenPrinterW(printName, &hPrinter, nullptr);
  if (status) {

    LPDEVMODEW  pNewDevMode;
    DWORD       dwNeeded, dwRet;

    
    dwNeeded = ::DocumentPropertiesW(gParentWnd, hPrinter, printName, nullptr, nullptr, 0);
    if (dwNeeded == 0) {
      return nullptr;
    }

    
    pNewDevMode = (LPDEVMODEW)::HeapAlloc (::GetProcessHeap(), HEAP_ZERO_MEMORY, dwNeeded);
    if (!pNewDevMode) return nullptr;

    hGlobalDevMode = (HGLOBAL)::GlobalAlloc(GHND, dwNeeded);
    if (!hGlobalDevMode) {
      ::HeapFree(::GetProcessHeap(), 0, pNewDevMode);
      return nullptr;
    }

    dwRet = ::DocumentPropertiesW(gParentWnd, hPrinter, printName, pNewDevMode, nullptr, DM_OUT_BUFFER);

    if (dwRet != IDOK) {
      ::HeapFree(::GetProcessHeap(), 0, pNewDevMode);
      ::GlobalFree(hGlobalDevMode);
      ::ClosePrinter(hPrinter);
      return nullptr;
    }

    
    
    LPDEVMODEW devMode = (DEVMODEW *)::GlobalLock(hGlobalDevMode);
    if (devMode) {
      memcpy(devMode, pNewDevMode, dwNeeded);
      
      SetupDevModeFromSettings(devMode, aPS);

      
      dwRet = ::DocumentPropertiesW(gParentWnd, hPrinter, printName, devMode, devMode, DM_IN_BUFFER | DM_OUT_BUFFER);
      if (dwRet != IDOK) {
        ::GlobalUnlock(hGlobalDevMode);
        ::GlobalFree(hGlobalDevMode);
        ::HeapFree(::GetProcessHeap(), 0, pNewDevMode);
        ::ClosePrinter(hPrinter);
        return nullptr;
      }

      ::GlobalUnlock(hGlobalDevMode);
    } else {
      ::GlobalFree(hGlobalDevMode);
      hGlobalDevMode = nullptr;
    }

    ::HeapFree(::GetProcessHeap(), 0, pNewDevMode);

    ::ClosePrinter(hPrinter);

  } else {
    return nullptr;
  }

  return hGlobalDevMode;
}



static void GetDefaultPrinterNameFromGlobalPrinters(nsXPIDLString &printerName)
{
  nsCOMPtr<nsIPrinterEnumerator> prtEnum = do_GetService("@mozilla.org/gfx/printerenumerator;1");
  if (prtEnum) {
    prtEnum->GetDefaultPrinterName(getter_Copies(printerName));
  }
}



static bool ShouldExtendPrintDialog()
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, true);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefs->GetBranch(nullptr, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, true);

  bool result;
  rv = prefBranch->GetBoolPref("print.extend_native_print_dialog", &result);
  NS_ENSURE_SUCCESS(rv, true);
  return result;
}



static nsresult 
ShowNativePrintDialog(HWND              aHWnd,
                      nsIPrintSettings* aPrintSettings)
{
  
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  gDialogWasExtended  = false;

  HGLOBAL hGlobalDevMode = nullptr;
  HGLOBAL hDevNames      = nullptr;

  
  nsXPIDLString printerName;
  aPrintSettings->GetPrinterName(getter_Copies(printerName));

  
  if (printerName.IsEmpty()) {
    GetDefaultPrinterNameFromGlobalPrinters(printerName);
  } else {
    HANDLE hPrinter = nullptr;
    if(!::OpenPrinterW(const_cast<wchar_t*>(static_cast<const wchar_t*>(printerName.get())), &hPrinter, nullptr)) {
      
      GetDefaultPrinterNameFromGlobalPrinters(printerName);
    } else {
      ::ClosePrinter(hPrinter);
    }
  }

  

  uint32_t len = printerName.Length();
  hDevNames = (HGLOBAL)::GlobalAlloc(GHND, sizeof(wchar_t) * (len + 1) + 
                                     sizeof(DEVNAMES));
  if (!hDevNames) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  DEVNAMES* pDevNames = (DEVNAMES*)::GlobalLock(hDevNames);
  if (!pDevNames) {
    ::GlobalFree(hDevNames);
    return NS_ERROR_FAILURE;
  }
  pDevNames->wDriverOffset = sizeof(DEVNAMES)/sizeof(wchar_t);
  pDevNames->wDeviceOffset = sizeof(DEVNAMES)/sizeof(wchar_t);
  pDevNames->wOutputOffset = sizeof(DEVNAMES)/sizeof(wchar_t)+len;
  pDevNames->wDefault      = 0;

  memcpy(pDevNames+1, printerName, (len + 1) * sizeof(wchar_t));
  ::GlobalUnlock(hDevNames);

  
  
  
  
  
  
  hGlobalDevMode = CreateGlobalDevModeAndInit(printerName, aPrintSettings);

  
  PRINTDLGW  prntdlg;
  memset(&prntdlg, 0, sizeof(PRINTDLGW));

  prntdlg.lStructSize = sizeof(prntdlg);
  prntdlg.hwndOwner   = aHWnd;
  prntdlg.hDevMode    = hGlobalDevMode;
  prntdlg.hDevNames   = hDevNames;
  prntdlg.hDC         = nullptr;
  prntdlg.Flags       = PD_ALLPAGES | PD_RETURNIC | 
                        PD_USEDEVMODECOPIESANDCOLLATE | PD_COLLATE;

  
  int16_t howToEnableFrameUI = nsIPrintSettings::kFrameEnableNone;
  bool isOn;
  aPrintSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &isOn);
  if (!isOn) {
    prntdlg.Flags |= PD_NOSELECTION;
  }
  aPrintSettings->GetHowToEnableFrameUI(&howToEnableFrameUI);

  int32_t pg = 1;
  aPrintSettings->GetStartPageRange(&pg);
  prntdlg.nFromPage           = pg;
  
  aPrintSettings->GetEndPageRange(&pg);
  prntdlg.nToPage             = pg;

  prntdlg.nMinPage            = 1;
  prntdlg.nMaxPage            = 0xFFFF;
  prntdlg.nCopies             = 1;
  prntdlg.lpfnSetupHook       = nullptr;
  prntdlg.lpSetupTemplateName = nullptr;
  prntdlg.hPrintTemplate      = nullptr;
  prntdlg.hSetupTemplate      = nullptr;

  prntdlg.hInstance           = nullptr;
  prntdlg.lpPrintTemplateName = nullptr;

  if (!ShouldExtendPrintDialog()) {
    prntdlg.lCustData         = 0;
    prntdlg.lpfnPrintHook     = nullptr;
  } else {
    
    prntdlg.lCustData         = (DWORD)howToEnableFrameUI;
    prntdlg.lpfnPrintHook     = (LPPRINTHOOKPROC)PrintHookProc;
    prntdlg.Flags            |= PD_ENABLEPRINTHOOK;
  }

  BOOL result = ::PrintDlgW(&prntdlg);

  if (TRUE == result) {
    
    NS_ENSURE_TRUE(aPrintSettings && prntdlg.hDevMode, NS_ERROR_FAILURE);

    if (prntdlg.hDevNames == nullptr) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }
    
    DEVNAMES *devnames = (DEVNAMES *)::GlobalLock(prntdlg.hDevNames);
    if (devnames == nullptr) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }

    char16_t* device = &(((char16_t *)devnames)[devnames->wDeviceOffset]);
    char16_t* driver = &(((char16_t *)devnames)[devnames->wDriverOffset]);

    
    
    
    
    
    
    
    
    if (prntdlg.Flags & PD_PRINTTOFILE) {
      char16ptr_t fileName = &(((wchar_t *)devnames)[devnames->wOutputOffset]);
      NS_ASSERTION(wcscmp(fileName, L"FILE:") == 0, "FileName must be `FILE:`");
      aPrintSettings->SetToFileName(fileName);
      aPrintSettings->SetPrintToFile(true);
    } else {
      
      aPrintSettings->SetPrintToFile(false);
      aPrintSettings->SetToFileName(nullptr);
    }

    nsCOMPtr<nsIPrintSettingsWin> psWin(do_QueryInterface(aPrintSettings));
    if (!psWin) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }

    
    psWin->SetDeviceName(device);
    psWin->SetDriverName(driver);

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
    wprintf(L"printer: driver %s, device %s  flags: %d\n", driver, device, prntdlg.Flags);
#endif
    

    aPrintSettings->SetPrinterName(device);

    if (prntdlg.Flags & PD_SELECTION) {
      aPrintSettings->SetPrintRange(nsIPrintSettings::kRangeSelection);

    } else if (prntdlg.Flags & PD_PAGENUMS) {
      aPrintSettings->SetPrintRange(nsIPrintSettings::kRangeSpecifiedPageRange);
      aPrintSettings->SetStartPageRange(prntdlg.nFromPage);
      aPrintSettings->SetEndPageRange(prntdlg.nToPage);

    } else { 
      aPrintSettings->SetPrintRange(nsIPrintSettings::kRangeAllPages);
    }

    if (howToEnableFrameUI != nsIPrintSettings::kFrameEnableNone) {
      
      if (gDialogWasExtended) {
        
        switch (gFrameSelectedRadioBtn) {
          case rad4: 
            aPrintSettings->SetPrintFrameType(nsIPrintSettings::kFramesAsIs);
            break;
          case rad5: 
            aPrintSettings->SetPrintFrameType(nsIPrintSettings::kSelectedFrame);
            break;
          case rad6: 
            aPrintSettings->SetPrintFrameType(nsIPrintSettings::kEachFrameSep);
            break;
        } 
      } else {
        
        
        aPrintSettings->SetPrintFrameType(nsIPrintSettings::kEachFrameSep);
      }
    } else {
      aPrintSettings->SetPrintFrameType(nsIPrintSettings::kNoFrames);
    }
    
    ::GlobalUnlock(prntdlg.hDevNames);

    
    LPDEVMODEW devMode = (LPDEVMODEW)::GlobalLock(prntdlg.hDevMode);
    if (devMode == nullptr) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }
    psWin->SetDevMode(devMode); 
    SetPrintSettingsFromDevMode(aPrintSettings, devMode);
    ::GlobalUnlock(prntdlg.hDevMode);

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
    bool    printSelection = prntdlg.Flags & PD_SELECTION;
    bool    printAllPages  = prntdlg.Flags & PD_ALLPAGES;
    bool    printNumPages  = prntdlg.Flags & PD_PAGENUMS;
    int32_t fromPageNum    = 0;
    int32_t toPageNum      = 0;

    if (printNumPages) {
      fromPageNum = prntdlg.nFromPage;
      toPageNum   = prntdlg.nToPage;
    } 
    if (printSelection) {
      printf("Printing the selection\n");

    } else if (printAllPages) {
      printf("Printing all the pages\n");

    } else {
      printf("Printing from page no. %d to %d\n", fromPageNum, toPageNum);
    }
#endif
    
  } else {
    ::SetFocus(aHWnd);
    aPrintSettings->SetIsCancelled(true);
    if (hGlobalDevMode) ::GlobalFree(hGlobalDevMode);
    return NS_ERROR_ABORT;
  }

  return NS_OK;
}


static void 
PrepareForPrintDialog(nsIWebBrowserPrint* aWebBrowserPrint, nsIPrintSettings* aPS)
{
  NS_ASSERTION(aWebBrowserPrint, "Can't be null");
  NS_ASSERTION(aPS, "Can't be null");

  bool isFramesetDocument;
  bool isFramesetFrameSelected;
  bool isIFrameSelected;
  bool isRangeSelection;

  aWebBrowserPrint->GetIsFramesetDocument(&isFramesetDocument);
  aWebBrowserPrint->GetIsFramesetFrameSelected(&isFramesetFrameSelected);
  aWebBrowserPrint->GetIsIFrameSelected(&isIFrameSelected);
  aWebBrowserPrint->GetIsRangeSelection(&isRangeSelection);

  
  if (isFramesetDocument) {
    if (isFramesetFrameSelected) {
      aPS->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAll);
    } else {
      aPS->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableAsIsAndEach);
    }
  } else {
    aPS->SetHowToEnableFrameUI(nsIPrintSettings::kFrameEnableNone);
  }

  
  aPS->SetPrintOptions(nsIPrintSettings::kEnableSelectionRB, isRangeSelection || isIFrameSelected);

}




nsresult NativeShowPrintDialog(HWND                aHWnd,
                               nsIWebBrowserPrint* aWebBrowserPrint,
                               nsIPrintSettings*   aPrintSettings)
{
  PrepareForPrintDialog(aWebBrowserPrint, aPrintSettings);

  nsresult rv = ShowNativePrintDialog(aHWnd, aPrintSettings);
  if (aHWnd) {
    ::DestroyWindow(aHWnd);
  }

  return rv;
}

