




































#ifdef MOZ_REQUIRE_CURRENT_SDK
#undef WINVER
#define WINVER 0x0500
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#endif


















#include "prmem.h"
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




#define GetPrintDlgExQuoted "PrintDlgExA"


static const char* kAsLaidOutOnScreenStr = "As &laid out on the screen";
static const char* kTheSelectedFrameStr  = "The selected &frame";
static const char* kEachFrameSeparately  = "&Each frame separately";






static UINT gFrameSelectedRadioBtn = 0;


static PRPackedBool gDialogWasExtended     = PR_FALSE;

#define PRINTDLG_PROPERTIES "chrome://global/locale/printdialog.properties"

static HWND gParentWnd = NULL;




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
  {DMPAPER_B4,        250.0, 354.0, PR_FALSE}, 
  {DMPAPER_B5,        182.0, 257.0, PR_FALSE},
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
};
const PRInt32 kNumPaperSizes = 41;


static PRBool 
CheckForExtendedDialog()
{
#ifdef MOZ_REQUIRE_CURRENT_SDK
  HMODULE lib = GetModuleHandle("comdlg32.dll");
  if ( lib ) {
    return GetProcAddress(lib, GetPrintDlgExQuoted);
  }
#endif
  return PR_FALSE;
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

  const double kThreshold = 0.05;
  PRBool foundEnum = PR_FALSE;
  for (PRInt32 i=0;i<kNumPaperSizes;i++) {
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



static nsresult 
SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, 
                            LPDEVMODEW         aDevMode)
{
  if (aPrintSettings == nsnull) {
    return NS_ERROR_FAILURE;
  }

  aPrintSettings->SetIsInitializedFromPrinter(PR_TRUE);
  if (aDevMode->dmFields & DM_ORIENTATION) {
    PRInt32 orientation  = aDevMode->dmOrientation == DMORIENT_PORTRAIT?
                           nsIPrintSettings::kPortraitOrientation:nsIPrintSettings::kLandscapeOrientation;
    aPrintSettings->SetOrientation(orientation);
  }

  
  if (aDevMode->dmFields & DM_COPIES) {
    aPrintSettings->SetNumCopies(PRInt32(aDevMode->dmCopies));
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
    for (PRInt32 i=0;i<kNumPaperSizes;i++) {
      if (kPaperSizes[i].mPaperSize == aDevMode->dmPaperSize) {
        aPrintSettings->SetPaperSizeUnit(kPaperSizes[i].mIsInches?nsIPrintSettings::kPaperSizeInches:nsIPrintSettings::kPaperSizeMillimeters);
        break;
      }
    }

  } else if (aDevMode->dmFields & DM_PAPERLENGTH && aDevMode->dmFields & DM_PAPERWIDTH) {
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
  nsCAutoString text;
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
                     PRBool       aIsSet,
                     PRBool       isEnabled = PR_TRUE) 
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
    if (radWnd != NULL) {
      ::SendMessage(radWnd, BM_SETCHECK, (WPARAM)(radioIds[i] == aRadId), (LPARAM)0);
    }
  }
}


typedef struct {
  char * mKeyStr;
  long   mKeyId;
} PropKeyInfo;



static PropKeyInfo gAllPropKeys[] = {
    {"PrintFrames", grp3},
    {"Aslaid", rad4},
    {"selectedframe", rad5},
    {"Eachframe", rad6},
    {NULL, NULL}};







static void GetLocalRect(HWND aWnd, RECT& aRect, HWND aParent)
{
  RECT wr;
  ::GetWindowRect(aParent, &wr);

  RECT cr;
  ::GetClientRect(aParent, &cr);

  ::GetWindowRect(aWnd, &aRect);

  int borderH = (wr.bottom-wr.top+1) - (cr.bottom-cr.top+1);
  int borderW = ((wr.right-wr.left+1) - (cr.right-cr.left+1))/2;
  aRect.top    -= wr.top+borderH-borderW;
  aRect.left   -= wr.left+borderW;
  aRect.right  -= wr.left+borderW;
  aRect.bottom -= wr.top+borderH-borderW;
}



static void Show(HWND aWnd, PRBool bState)
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
  nsCAutoString str;
  if (NS_FAILED(NS_CopyUnicodeToNative(aStr, str)))
    return NULL;

  HWND hWnd = ::CreateWindow (aType, str.get(),
                              WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | aStyle,
                              aRect.x, aRect.y, aRect.width, aRect.height,
                              (HWND)aHdlg, (HMENU)aId,
                              aHInst, NULL);
  if (hWnd == NULL) return NULL;

  
  
  HFONT hFont = (HFONT)::SendMessage(aHdlg, WM_GETFONT, (WPARAM)0, (LPARAM)0);
  if (hFont != NULL) {
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



static void InitializeExtendedDialog(HWND hdlg, PRInt16 aHowToEnableFrameUI) 
{
  
  nsCOMPtr<nsIStringBundle> strBundle;
  if (NS_SUCCEEDED(GetLocalizedBundle(PRINTDLG_PROPERTIES, getter_AddRefs(strBundle)))) {
    PRInt32 i = 0;
    while (gAllPropKeys[i].mKeyStr != NULL) {
      SetText(hdlg, gAllPropKeys[i].mKeyId, strBundle, gAllPropKeys[i].mKeyStr);
      i++;
    }
  }

  
  if (aHowToEnableFrameUI == nsIPrintSettings::kFrameEnableAll) {
    SetRadio(hdlg, rad4, PR_FALSE);  
    SetRadio(hdlg, rad5, PR_TRUE); 
    SetRadio(hdlg, rad6, PR_FALSE);
    
    gFrameSelectedRadioBtn = rad5;

  } else if (aHowToEnableFrameUI == nsIPrintSettings::kFrameEnableAsIsAndEach) {
    SetRadio(hdlg, rad4, PR_FALSE);  
    SetRadio(hdlg, rad5, PR_FALSE, PR_FALSE); 
    SetRadio(hdlg, rad6, PR_TRUE);
    
    gFrameSelectedRadioBtn = rad6;


  } else {  
    
    SetRadio(hdlg, grp3, PR_FALSE, PR_FALSE); 
    
    SetRadio(hdlg, rad4, PR_FALSE, PR_FALSE); 
    SetRadio(hdlg, rad5, PR_FALSE, PR_FALSE); 
    SetRadio(hdlg, rad6, PR_FALSE, PR_FALSE); 
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
    if (printDlg == NULL) return 0L;

    PRInt16 howToEnableFrameUI = (PRInt16)printDlg->lCustData;

    HINSTANCE hInst = (HINSTANCE)::GetWindowLongPtr(hdlg, GWLP_HINSTANCE);
    if (hInst == NULL) return 0L;

    
    
    HWND wnd = ::GetDlgItem(hdlg, grp1);
    if (wnd == NULL) return 0L;
    RECT dlgRect;
    GetLocalRect(wnd, dlgRect, hdlg);

    wnd = ::GetDlgItem(hdlg, rad1); 
    if (wnd == NULL) return 0L;
    RECT rad1Rect;
    GetLocalRect(wnd, rad1Rect, hdlg);

    wnd = ::GetDlgItem(hdlg, rad2); 
    if (wnd == NULL) return 0L;
    RECT rad2Rect;
    GetLocalRect(wnd, rad2Rect, hdlg);

    wnd = ::GetDlgItem(hdlg, rad3); 
    if (wnd == NULL) return 0L;
    RECT rad3Rect;
    GetLocalRect(wnd, rad3Rect, hdlg);

    HWND okWnd = ::GetDlgItem(hdlg, IDOK);
    if (okWnd == NULL) return 0L;
    RECT okRect;
    GetLocalRect(okWnd, okRect, hdlg);

    wnd = ::GetDlgItem(hdlg, grp4); 
    if (wnd == NULL) return 0L;
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
    if (rad4Wnd == NULL) return 0L;
    y += radHgt + rbGap;

    rect.SetRect(rad1Rect.left, y, rbWidth, radHgt);
    HWND rad5Wnd = CreateRadioBtn(hInst, hdlg, rad5, kTheSelectedFrameStr, rect);
    if (rad5Wnd == NULL) {
      Show(rad4Wnd, FALSE); 
      return 0L;
    }
    y += radHgt + rbGap;

    rect.SetRect(rad1Rect.left, y, rbWidth, radHgt);
    HWND rad6Wnd = CreateRadioBtn(hInst, hdlg, rad6, kEachFrameSeparately, rect);
    if (rad6Wnd == NULL) {
      Show(rad4Wnd, FALSE); 
      Show(rad5Wnd, FALSE); 
      return 0L;
    }
    y += radHgt + grpBotGap;

    
    rect.SetRect (dlgRect.left, top, dlgRect.right-dlgRect.left+1, y-top+1);
    HWND grpBoxWnd = CreateGroupBox(hInst, hdlg, grp3, NS_LITERAL_STRING("Print Frame"), rect);
    if (grpBoxWnd == NULL) {
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

    ::SetWindowPos(hdlg, NULL, pr.left, pr.top, pr.right-pr.left+1, pr.bottom-pr.top+1, 
                   SWP_NOMOVE|SWP_NOREDRAW|SWP_NOZORDER);

    
    ::GetClientRect(hdlg, &cr);
    dlgHgt = (cr.bottom - cr.top) + 1;
 
    
    int okHgt = okRect.bottom - okRect.top + 1;
    ::SetWindowPos(okWnd, NULL, okRect.left, dlgHgt-bottomGap-okHgt, 0, 0, 
                   SWP_NOSIZE|SWP_NOREDRAW|SWP_NOZORDER);

    HWND cancelWnd = ::GetDlgItem(hdlg, IDCANCEL);
    if (cancelWnd == NULL) return 0L;

    RECT cancelRect;
    GetLocalRect(cancelWnd, cancelRect, hdlg);
    int cancelHgt = cancelRect.bottom - cancelRect.top + 1;
    ::SetWindowPos(cancelWnd, NULL, cancelRect.left, dlgHgt-bottomGap-cancelHgt, 0, 0, 
                   SWP_NOSIZE|SWP_NOREDRAW|SWP_NOZORDER);

    
    InitializeExtendedDialog(hdlg, howToEnableFrameUI);

    
    gDialogWasExtended = PR_TRUE;
  }
  return 0L;
}









static HGLOBAL CreateGlobalDevModeAndInit(LPCWSTR aPrintName, nsIPrintSettings* aPS)
{
  HGLOBAL hGlobalDevMode = NULL;

  nsresult rv = NS_ERROR_FAILURE;
  HANDLE hPrinter = NULL;
  
  LPWSTR printName = const_cast<wchar_t*>(aPrintName);
  BOOL status = ::OpenPrinterW(printName, &hPrinter, NULL);
  if (status) {

    LPDEVMODEW  pNewDevMode;
    DWORD       dwNeeded, dwRet;

    
    dwNeeded = ::DocumentPropertiesW(gParentWnd, hPrinter, printName, NULL, NULL, 0);
    if (dwNeeded == 0) {
      return NULL;
    }

    
    pNewDevMode = (LPDEVMODEW)::HeapAlloc (::GetProcessHeap(), HEAP_ZERO_MEMORY, dwNeeded);
    if (!pNewDevMode) return NULL;

    hGlobalDevMode = (HGLOBAL)::GlobalAlloc(GHND, dwNeeded);
    if (!hGlobalDevMode) {
      ::HeapFree(::GetProcessHeap(), 0, pNewDevMode);
      return NULL;
    }

    dwRet = ::DocumentPropertiesW(gParentWnd, hPrinter, printName, pNewDevMode, NULL, DM_OUT_BUFFER);

    if (dwRet != IDOK) {
      ::HeapFree(::GetProcessHeap(), 0, pNewDevMode);
      ::GlobalFree(hGlobalDevMode);
      ::ClosePrinter(hPrinter);
      return NULL;
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
         return NULL;
      }

      ::GlobalUnlock(hGlobalDevMode);
    } else {
      ::GlobalFree(hGlobalDevMode);
      hGlobalDevMode = NULL;
    }

    ::HeapFree(::GetProcessHeap(), 0, pNewDevMode);

    ::ClosePrinter(hPrinter);

  } else {
    return NULL;
  }

  return hGlobalDevMode;
}



static PRUnichar * GetDefaultPrinterNameFromGlobalPrinters()
{
  PRUnichar * printerName = nsnull;
  nsCOMPtr<nsIPrinterEnumerator> prtEnum = do_GetService("@mozilla.org/gfx/printerenumerator;1");
  if (prtEnum) {
    prtEnum->GetDefaultPrinterName(&printerName);
  }
  return printerName;
}



static PRBool ShouldExtendPrintDialog()
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, PR_TRUE);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, PR_TRUE);

  PRBool result;
  rv = prefBranch->GetBoolPref("print.extend_native_print_dialog", &result);
  NS_ENSURE_SUCCESS(rv, PR_TRUE);
  return result;
}



static nsresult 
ShowNativePrintDialog(HWND              aHWnd,
                      nsIPrintSettings* aPrintSettings)
{
  
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  nsresult  rv = NS_ERROR_FAILURE;
  gDialogWasExtended  = PR_FALSE;

  HGLOBAL hGlobalDevMode = NULL;
  HGLOBAL hDevNames      = NULL;

  
  PRUnichar * printerName;
  aPrintSettings->GetPrinterName(&printerName);

  
  if (!printerName || (printerName && !*printerName)) {
    printerName = GetDefaultPrinterNameFromGlobalPrinters();
  } else {
    HANDLE hPrinter = NULL;
    if(!::OpenPrinterW(const_cast<wchar_t*>(printerName), &hPrinter, NULL)) {
      
      printerName = GetDefaultPrinterNameFromGlobalPrinters();
    } else {
      ::ClosePrinter(hPrinter);
    }
  }

  NS_ASSERTION(printerName, "We have to have a printer name");
  if (!printerName) return NS_ERROR_FAILURE;

  

  PRUint32 len = wcslen(printerName);
  hDevNames = (HGLOBAL)::GlobalAlloc(GHND, sizeof(wchar_t) * (len + 1) + 
                                     sizeof(DEVNAMES));
  DEVNAMES* pDevNames = (DEVNAMES*)::GlobalLock(hDevNames);
  pDevNames->wDriverOffset = sizeof(DEVNAMES);
  pDevNames->wDeviceOffset = sizeof(DEVNAMES);
  pDevNames->wOutputOffset = sizeof(DEVNAMES)+len+1;
  pDevNames->wDefault      = 0;

  wchar_t* device = &(((wchar_t*)pDevNames)[pDevNames->wDeviceOffset]);
  wcscpy(device, printerName);
  ::GlobalUnlock(hDevNames);

  
  
  
  
  
  
  hGlobalDevMode = CreateGlobalDevModeAndInit(printerName, aPrintSettings);

  
  PRINTDLGW  prntdlg;
  memset(&prntdlg, 0, sizeof(PRINTDLGW));

  prntdlg.lStructSize = sizeof(prntdlg);
  prntdlg.hwndOwner   = aHWnd;
  prntdlg.hDevMode    = hGlobalDevMode;
  prntdlg.hDevNames   = hDevNames;
  prntdlg.hDC         = NULL;
  prntdlg.Flags       = PD_ALLPAGES | PD_RETURNIC | PD_USEDEVMODECOPIESANDCOLLATE;

  
  PRInt16 howToEnableFrameUI = nsIPrintSettings::kFrameEnableNone;
  PRBool isOn;
  aPrintSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &isOn);
  if (!isOn) {
    prntdlg.Flags |= PD_NOSELECTION;
  }
  aPrintSettings->GetHowToEnableFrameUI(&howToEnableFrameUI);

  PRInt32 pg = 1;
  aPrintSettings->GetStartPageRange(&pg);
  prntdlg.nFromPage           = pg;
  
  aPrintSettings->GetEndPageRange(&pg);
  prntdlg.nToPage             = pg;

  prntdlg.nMinPage            = 1;
  prntdlg.nMaxPage            = 0xFFFF;
  prntdlg.nCopies             = 1;
  prntdlg.lpfnSetupHook       = NULL;
  prntdlg.lpSetupTemplateName = NULL;
  prntdlg.hPrintTemplate      = NULL;
  prntdlg.hSetupTemplate      = NULL;

  prntdlg.hInstance           = NULL;
  prntdlg.lpPrintTemplateName = NULL;

  if (!ShouldExtendPrintDialog()) {
    prntdlg.lCustData         = NULL;
    prntdlg.lpfnPrintHook     = NULL;
  } else {
    
    prntdlg.lCustData         = (DWORD)howToEnableFrameUI;
    prntdlg.lpfnPrintHook     = (LPPRINTHOOKPROC)PrintHookProc;
    prntdlg.Flags            |= PD_ENABLEPRINTHOOK;
  }

  BOOL result = ::PrintDlgW(&prntdlg);

  if (TRUE == result) {
    
    NS_ENSURE_TRUE(aPrintSettings && prntdlg.hDevMode, NS_ERROR_FAILURE);

    if (prntdlg.hDevNames == NULL) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }
    
    DEVNAMES *devnames = (DEVNAMES *)::GlobalLock(prntdlg.hDevNames);
    if (devnames == NULL) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }

    wchar_t* device = &(((wchar_t *)devnames)[devnames->wDeviceOffset]);
    wchar_t* driver = &(((wchar_t *)devnames)[devnames->wDriverOffset]);

    
    
    
    
    
    
    
    
    if (prntdlg.Flags & PD_PRINTTOFILE) {
      wchar_t* fileName = &(((wchar_t *)devnames)[devnames->wOutputOffset]);
      NS_ASSERTION(wcscmp(fileName, L"FILE:") == 0, "FileName must be `FILE:`");
      aPrintSettings->SetToFileName(fileName);
      aPrintSettings->SetPrintToFile(PR_TRUE);
    } else {
      
      aPrintSettings->SetPrintToFile(PR_FALSE);
      aPrintSettings->SetToFileName(nsnull);
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
    if (devMode == NULL) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }
    psWin->SetDevMode(devMode); 
    SetPrintSettingsFromDevMode(aPrintSettings, devMode);
    ::GlobalUnlock(prntdlg.hDevMode);

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
    PRBool  printSelection = prntdlg.Flags & PD_SELECTION;
    PRBool  printAllPages  = prntdlg.Flags & PD_ALLPAGES;
    PRBool  printNumPages  = prntdlg.Flags & PD_PAGENUMS;
    PRInt32 fromPageNum    = 0;
    PRInt32 toPageNum      = 0;

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
    aPrintSettings->SetIsCancelled(PR_TRUE);
    if (hGlobalDevMode) ::GlobalFree(hGlobalDevMode);
    return NS_ERROR_ABORT;
  }

  return NS_OK;
}


#ifdef MOZ_REQUIRE_CURRENT_SDK


static BOOL APIENTRY PropSheetCallBack(HWND hdlg, UINT uiMsg, UINT wParam, LONG lParam)
{
  if (uiMsg == WM_COMMAND) {
    UINT id = LOWORD(wParam);
    if (id == rad4 || id == rad5 || id == rad6) {
      gFrameSelectedRadioBtn = id;
      SetRadioOfGroup(hdlg, id);
    }

  } else if (uiMsg == WM_INITDIALOG) {
    

    
    
    PRInt16 howToEnableFrameUI = gFrameSelectedRadioBtn;
    gFrameSelectedRadioBtn     = 0;

    HINSTANCE hInst = (HINSTANCE)::GetWindowLongPtr(hdlg, GWLP_HINSTANCE);
    if (hInst == NULL) return 0L;

    
    
    TEXTMETRIC metrics;
    HFONT hFont = (HFONT)::SendMessage(hdlg, WM_GETFONT, (WPARAM)0, (LPARAM)0);
    HDC localDC = ::GetDC(hdlg);
    ::SelectObject(localDC, (HGDIOBJ)hFont);
    ::GetTextMetrics(localDC, &metrics);
    ::ReleaseDC(hdlg, localDC);

    
     RECT dlgr; 
    ::GetWindowRect(hdlg, &dlgr);

    int horzGap    = 5;                                 
    int vertGap    = 5;                                 
    int rbGap      = metrics.tmHeight / 2;               
    int top        = vertGap*2;                            
    int radHgt     = metrics.tmHeight;                   
    int y          = top;                                
    int x          = horzGap*2;
    int rbWidth    = dlgr.right - dlgr.left - (5*horzGap);  
    int grpWidth   = dlgr.right - dlgr.left - (2*horzGap);  

    nsRect rect;

    
    
    
    
    
    x += horzGap*2;
    y += vertGap + metrics.tmHeight;
    rect.SetRect(x, y, rbWidth,radHgt);
    HWND rad4Wnd = CreateRadioBtn(hInst, hdlg, rad4, kAsLaidOutOnScreenStr, rect);
    if (rad4Wnd == NULL) return 0L;
    y += radHgt + rbGap;

    rect.SetRect(x, y, rbWidth, radHgt);
    HWND rad5Wnd = CreateRadioBtn(hInst, hdlg, rad5, kTheSelectedFrameStr, rect);
    if (rad5Wnd == NULL) {
      Show(rad4Wnd, FALSE); 
      return 0L;
    }
    y += radHgt + rbGap;

    rect.SetRect(x, y, rbWidth, radHgt);
    HWND rad6Wnd = CreateRadioBtn(hInst, hdlg, rad6, kEachFrameSeparately, rect);
    if (rad6Wnd == NULL) {
      Show(rad4Wnd, FALSE); 
      Show(rad5Wnd, FALSE); 
      return 0L;
    }
    y += radHgt + (vertGap*2);

    x -= horzGap*2;
    
    rect.SetRect (x, top, grpWidth, y-top+1);
    HWND grpBoxWnd = CreateGroupBox(hInst, hdlg, grp3, NS_LITERAL_STRING("Print Frame"), rect);
    if (grpBoxWnd == NULL) {
      Show(rad4Wnd, FALSE); 
      Show(rad5Wnd, FALSE); 
      Show(rad6Wnd, FALSE); 
      return 0L;
    }

    
    InitializeExtendedDialog(hdlg, howToEnableFrameUI);

    
    gDialogWasExtended = PR_TRUE;
  }
  return 0L;
}



static HPROPSHEETPAGE ExtendPrintDialog(HWND aHWnd, char* aTitle)
{
  
  HINSTANCE hInst = (HINSTANCE)::GetWindowLongPtr(aHWnd, GWLP_HINSTANCE);
  PROPSHEETPAGE psp;
  memset(&psp, 0, sizeof(PROPSHEETPAGE));
  psp.dwSize      = sizeof(PROPSHEETPAGE);
  psp.dwFlags     = PSP_USETITLE | PSP_PREMATURE;
  psp.hInstance   = hInst;
  psp.pszTemplate = "OPTPROPSHEET";
  psp.pfnDlgProc  = PropSheetCallBack;
  psp.pszTitle    = aTitle?aTitle:"Options";

  HPROPSHEETPAGE newPropSheet = ::CreatePropertySheetPage(&psp);
  return newPropSheet;

}



static nsresult 
ShowNativePrintDialogEx(HWND              aHWnd,
                        nsIPrintSettings* aPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aHWnd);
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  nsresult  rv = NS_ERROR_FAILURE;
  gDialogWasExtended  = PR_FALSE;

  
  
  
  
  
  
  PRUnichar * printerName;
  aPrintSettings->GetPrinterName(&printerName);
  HGLOBAL hGlobalDevMode = NULL;
  if (printerName) {
    NS_ENSURE_SUCCESS(rv, rv);
    hGlobalDevMode = CreateGlobalDevModeAndInit(printerName, aPrintSettings);
  }

  
  PRINTDLGEX  prntdlg;
  memset(&prntdlg, 0, sizeof(PRINTDLGEX));

  prntdlg.lStructSize = sizeof(prntdlg);
  prntdlg.hwndOwner   = aHWnd;
  prntdlg.hDevMode    = hGlobalDevMode;
  prntdlg.Flags       = PD_ALLPAGES | PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE |
                        PD_NOCURRENTPAGE;
  prntdlg.nStartPage  = START_PAGE_GENERAL;

  
  PRInt16 howToEnableFrameUI = nsIPrintSettings::kFrameEnableNone;
  if (aPrintSettings != nsnull) {
    PRBool isOn;
    aPrintSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &isOn);
    if (!isOn) {
      prntdlg.Flags |= PD_NOSELECTION;
    }
    aPrintSettings->GetHowToEnableFrameUI(&howToEnableFrameUI);
  }

  
  
  
  const int kNumPageRanges     = 1;
  LPPRINTPAGERANGE pPageRanges = (LPPRINTPAGERANGE) GlobalAlloc(GPTR, kNumPageRanges * sizeof(PRINTPAGERANGE));
  if (!pPageRanges)
      return E_OUTOFMEMORY;

  prntdlg.nPageRanges    = 0;
  prntdlg.nMaxPageRanges = kNumPageRanges;
  prntdlg.lpPageRanges   = pPageRanges;
  prntdlg.nMinPage       = 1;
  prntdlg.nMaxPage       = 0xFFFF;
  prntdlg.nCopies        = 1;

  if (ShouldExtendPrintDialog()) {
    
    nsCAutoString title;
    nsString optionsStr;
    if (NS_SUCCEEDED(GetLocalizedString(strBundle, "options", optionsStr))) {
      
      NS_CopyUnicodeToNative(optionsStr, title);
    }

    
    
    gFrameSelectedRadioBtn = howToEnableFrameUI;
    HPROPSHEETPAGE psp[1];
    psp[0] = ExtendPrintDialog(aHWnd, title.get());
    prntdlg.nPropertyPages      = 1;
    prntdlg.lphPropertyPages    = psp;
  }

  HRESULT result = ::PrintDlgEx(&prntdlg);

  if (S_OK == result && (prntdlg.dwResultAction == PD_RESULT_PRINT)) {

    
    NS_ENSURE_TRUE(aPrintSettings && prntdlg.hDevMode, NS_ERROR_FAILURE);

    if (prntdlg.hDevNames == NULL) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }
    
    DEVNAMES *devnames = (DEVNAMES *)::GlobalLock(prntdlg.hDevNames);
    if (devnames == NULL) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }

    char* device = &(((char *)devnames)[devnames->wDeviceOffset]);
    char* driver = &(((char *)devnames)[devnames->wDriverOffset]);

    
    
    
    
    
    
    
    
    if (prntdlg.Flags & PD_PRINTTOFILE) {
      char* fileName = &(((char *)devnames)[devnames->wOutputOffset]);
      NS_ASSERTION(strcmp(fileName, "FILE:") == 0, "FileName must be `FILE:`");
      aPrintSettings->SetToFileName(NS_ConvertASCIItoUTF16(fileName).get());
      aPrintSettings->SetPrintToFile(PR_TRUE);
    } else {
      
      aPrintSettings->SetPrintToFile(PR_FALSE);
      aPrintSettings->SetToFileName(nsnull);
    }

    nsCOMPtr<nsIPrintSettingsWin> psWin(do_QueryInterface(aPrintSettings));
    if (!psWin) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }

    
    psWin->SetDeviceName(device);
    psWin->SetDriverName(driver);

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
    printf("printer: driver %s, device %s  flags: %d\n", driver, device, prntdlg.Flags);
#endif
    ::GlobalUnlock(prntdlg.hDevNames);

    
    if (aPrintSettings != nsnull) {

      if (prntdlg.Flags & PD_SELECTION) {
        aPrintSettings->SetPrintRange(nsIPrintSettings::kRangeSelection);

      } else if (prntdlg.Flags & PD_PAGENUMS) {
        aPrintSettings->SetPrintRange(nsIPrintSettings::kRangeSpecifiedPageRange);
        aPrintSettings->SetStartPageRange(pPageRanges->nFromPage);
        aPrintSettings->SetEndPageRange(pPageRanges->nToPage);

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
    }

    
    ::GlobalUnlock(prntdlg.hDevNames);

    
    LPDEVMODEW devMode = (LPDEVMODEW)::GlobalLock(prntdlg.hDevMode);
    if (devMode == NULL) {
      ::GlobalFree(hGlobalDevMode);
      return NS_ERROR_FAILURE;
    }
    psWin->SetDevMode(devMode); 
    SetPrintSettingsFromDevMode(aPrintSettings, devMode);
    ::GlobalUnlock(prntdlg.hDevMode);

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
    PRBool  printSelection = prntdlg.Flags & PD_SELECTION;
    PRBool  printAllPages  = prntdlg.Flags & PD_ALLPAGES;
    PRBool  printNumPages  = prntdlg.Flags & PD_PAGENUMS;
    PRInt32 fromPageNum    = 0;
    PRInt32 toPageNum      = 0;

    if (printNumPages) {
      fromPageNum = pPageRanges->nFromPage;
      toPageNum   = pPageRanges->nToPage;
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
    if (hGlobalDevMode) ::GlobalFree(hGlobalDevMode);
    return NS_ERROR_ABORT;
  }

  ::GlobalFree(pPageRanges);

  return NS_OK;
}
#endif 


static void 
PrepareForPrintDialog(nsIWebBrowserPrint* aWebBrowserPrint, nsIPrintSettings* aPS)
{
  NS_ASSERTION(aWebBrowserPrint, "Can't be null");
  NS_ASSERTION(aPS, "Can't be null");

  PRBool isFramesetDocument;
  PRBool isFramesetFrameSelected;
  PRBool isIFrameSelected;
  PRBool isRangeSelection;

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
  nsresult rv = NS_ERROR_FAILURE;

  PrepareForPrintDialog(aWebBrowserPrint, aPrintSettings);

#ifdef MOZ_REQUIRE_CURRENT_SDK
  if (CheckForExtendedDialog()) {
    rv = ShowNativePrintDialogEx(aHWnd, aPrintSettings);
  } else {
    rv = ShowNativePrintDialog(aHWnd, aPrintSettings);
  }
#else
  rv = ShowNativePrintDialog(aHWnd, aPrintSettings);
#endif

  return rv;
}

