






































#include "extern.h"
#include "dialogs.h"
#include "extra.h"
#include "xpistub.h"
#include "xpi.h"
#include "xperr.h"
#include "logging.h"
#include "ifuncns.h"

#define BDIR_RIGHT 1
#define BDIR_LEFT  2

typedef HRESULT (_cdecl *XpiInit)(const char *, const char *aLogName, pfnXPIProgress);
typedef HRESULT (_cdecl *XpiInstall)(const char *, const char *, long);
typedef void    (_cdecl *XpiExit)(void);
typedef BOOL    (WINAPI *SetDllPathProc)(const char*);

#if _MSC_VER >= 1400
typedef HANDLE  (WINAPI *FnCreateActCtxA)(PCACTCTXA pActCtx);
typedef BOOL    (WINAPI *FnActivateActCtx)(HANDLE hActCtx, ULONG_PTR* lpCookie);
typedef BOOL    (WINAPI *FnDeactivateActCtx)(DWORD dwFlags, ULONG_PTR ulCookie);
typedef VOID    (WINAPI *FnReleaseActCtx)(HANDLE hActCtx);

ACTCTXA actctx;
HANDLE hActCtx = INVALID_HANDLE_VALUE;
ULONG_PTR ulpActivationCookie;

static FnCreateActCtxA    pfnCreateActCtxA    = NULL;
static FnActivateActCtx   pfnActivateActCtx   = NULL;
static FnDeactivateActCtx pfnDeactivateActCtx = NULL;
static FnReleaseActCtx    pfnReleaseActCtx    = NULL;
#endif

static XpiInit          pfnXpiInit;
static XpiInstall       pfnXpiInstall;
static XpiExit          pfnXpiExit;
static SetDllPathProc   pfnSetDllPath = NULL;

static long             lFileCounter;
static long             lBarberCounter;
static BOOL             bBarberBar;
static DWORD            dwBarberDirection;
static DWORD            dwCurrentArchive;
static DWORD            dwTotalArchives;
char                    szStrProcessingFile[MAX_BUF];
char                    szStrCopyingFile[MAX_BUF];
char                    szStrInstalling[MAX_BUF];
static char             gSavedCwd[MAX_BUF];

static void UpdateGaugeFileProgressBar(unsigned value);
static void UpdateGaugeArchiveProgressBar(unsigned value);
static void UpdateGaugeFileBarber(void);

struct ExtractFilesDlgInfo
{
	HWND	hWndDlg;
	int		nMaxFileBars;	    
	int		nMaxArchiveBars;	
	int		nFileBars;		    
	int		nArchiveBars;		  
} dlgInfo;

HRESULT InitializeXPIStub(char *xpinstallPath)
{
  char szBuf[MAX_BUF];
  char szXPIStubFile[MAX_BUF];
  char szEGetProcAddress[MAX_BUF];
  HANDLE hKernel;

  hXPIStubInst = NULL;
  GetCurrentDirectory(sizeof(gSavedCwd), gSavedCwd);

  if(!GetPrivateProfileString("Messages", "ERROR_GETPROCADDRESS", "", szEGetProcAddress, sizeof(szEGetProcAddress), szFileIniInstall))
    return(1);

  
  SetCurrentDirectory(xpinstallPath);

  
  
  if ((hKernel = LoadLibrary("kernel32.dll")) != NULL)
  {
    pfnSetDllPath = (SetDllPathProc)GetProcAddress(hKernel, "SetDllDirectoryA");
    if (pfnSetDllPath)
      pfnSetDllPath(xpinstallPath);
  }

  
  lstrcpy(szXPIStubFile, xpinstallPath);
  AppendBackSlash(szXPIStubFile, sizeof(szXPIStubFile));
  lstrcat(szXPIStubFile, "xpistub.dll");

  if(FileExists(szXPIStubFile) == FALSE)
    return(2);

#if _MSC_VER >= 1400
  





  if (hKernel != NULL &&
      (pfnCreateActCtxA = (FnCreateActCtxA)GetProcAddress(hKernel, "CreateActCtxA")) != NULL &&
      (pfnActivateActCtx = (FnActivateActCtx)GetProcAddress(hKernel, "ActivateActCtx")) != NULL &&
      (pfnDeactivateActCtx = (FnDeactivateActCtx)GetProcAddress(hKernel, "DeactivateActCtx")) != NULL &&
      (pfnReleaseActCtx = (FnReleaseActCtx)GetProcAddress(hKernel, "ReleaseActCtx")) != NULL)
  {
    memset(&actctx, 0, sizeof(actctx));
    actctx.cbSize = sizeof(actctx);
    actctx.lpSource = (LPCSTR)szXPIStubFile;
    actctx.lpResourceName = MAKEINTRESOURCE(17);
    actctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;

    hActCtx = pfnCreateActCtxA(&actctx);

    if (hActCtx == INVALID_HANDLE_VALUE)
      return(2); 

    if (!pfnActivateActCtx(hActCtx, &ulpActivationCookie))
    {
      pfnReleaseActCtx(hActCtx);
      return(2);
    }
  }
#endif

  
  if((hXPIStubInst = LoadLibraryEx(szXPIStubFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)) == NULL)
  {
    wsprintf(szBuf, szEDllLoad, szXPIStubFile);
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  if(((FARPROC)pfnXpiInit = GetProcAddress(hXPIStubInst, "XPI_Init")) == NULL)
  {
    wsprintf(szBuf, szEGetProcAddress, "XPI_Init");
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  if(((FARPROC)pfnXpiInstall = GetProcAddress(hXPIStubInst, "XPI_Install")) == NULL)
  {
    wsprintf(szBuf, szEGetProcAddress, "XPI_Install");
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }
  if(((FARPROC)pfnXpiExit = GetProcAddress(hXPIStubInst, "XPI_Exit")) == NULL)
  {
    wsprintf(szBuf, szEGetProcAddress, "XPI_Exit");
    PrintError(szBuf, ERROR_CODE_SHOW);
    return(1);
  }

  return(0);
}

HRESULT DeInitializeXPIStub()
{
  pfnXpiInit    = NULL;
  pfnXpiInstall = NULL;
  pfnXpiExit    = NULL;

#if _MSC_VER >= 1400
  if (pfnDeactivateActCtx)
  {
    pfnDeactivateActCtx(0, ulpActivationCookie);
    pfnReleaseActCtx(hActCtx);
  }

  pfnCreateActCtxA    = NULL;
  pfnActivateActCtx   = NULL;
  pfnDeactivateActCtx = NULL;
  pfnReleaseActCtx    = NULL;
#endif

  if(hXPIStubInst)
    FreeLibrary(hXPIStubInst);

  chdir(szSetupDir);
  if (pfnSetDllPath)
    pfnSetDllPath(NULL);

  SetCurrentDirectory(gSavedCwd);
  return(0);
}

void GetTotalArchivesToInstall(void)
{
  DWORD     dwIndex0;
  siC       *siCObject = NULL;

  dwIndex0        = 0;
  dwTotalArchives = 0;
  siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  while(siCObject)
  {
    if((siCObject->dwAttributes & SIC_SELECTED) && !(siCObject->dwAttributes & SIC_LAUNCHAPP))
      ++dwTotalArchives;

    ++dwIndex0;
    siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  }
}

char *GetErrorString(DWORD dwError, char *szErrorString, DWORD dwErrorStringSize)
{
  int  i = 0;
  char szErrorNumber[MAX_BUF];

  ZeroMemory(szErrorString, dwErrorStringSize);
  itoa(dwError, szErrorNumber, 10);

  
  while(TRUE)
  {
    if(*XpErrorList[i] == '\0')
      break;

    if(lstrcmpi(szErrorNumber, XpErrorList[i]) == 0)
    {
      if(*XpErrorList[i + 1] != '\0')
        lstrcpy(szErrorString, XpErrorList[i + 1]);

      break;
    }

    ++i;
  }

  return(szErrorString);
}




void InvalidateBarberBarArea()
{
  HWND	hWndGauge;
  RECT	rect;

  
  hWndGauge = GetDlgItem(dlgInfo.hWndDlg, IDC_GAUGE_FILE);
  
  GetClientRect(hWndGauge, &rect);
  
  InvalidateRect(hWndGauge, &rect, FALSE);
  
  UpdateWindow(dlgInfo.hWndDlg);
}

HRESULT SmartUpdateJars()
{
  DWORD     dwIndex0;
  siC       *siCObject = NULL;
  HRESULT   hrResult;
  char      szBuf[MAX_BUF];
  char      szEXpiInstall[MAX_BUF];
  char      szArchive[MAX_BUF];
  char      szMsgSmartUpdateStart[MAX_BUF];
  char      szDlgExtractingTitle[MAX_BUF];
  char      xpinstallPath[MAX_BUF];
  char      xpiArgs[MAX_BUF];

  if(!GetPrivateProfileString("Messages", "MSG_SMARTUPDATE_START", "", szMsgSmartUpdateStart, sizeof(szMsgSmartUpdateStart), szFileIniInstall))
    return(1);
  if(!GetPrivateProfileString("Messages", "DLG_EXTRACTING_TITLE", "", szDlgExtractingTitle, sizeof(szDlgExtractingTitle), szFileIniInstall))
    return(1);
  if(!GetPrivateProfileString("Messages", "STR_PROCESSINGFILE", "", szStrProcessingFile, sizeof(szStrProcessingFile), szFileIniInstall))
    exit(1);
  if(!GetPrivateProfileString("Messages", "STR_INSTALLING", "", szStrInstalling, sizeof(szStrInstalling), szFileIniInstall))
    exit(1);
  if(!GetPrivateProfileString("Messages", "STR_COPYINGFILE", "", szStrCopyingFile, sizeof(szStrCopyingFile), szFileIniInstall))
    exit(1);

  ShowMessage(szMsgSmartUpdateStart, TRUE);
  GetXpinstallPath(xpinstallPath, sizeof(xpinstallPath));
  if(InitializeXPIStub(xpinstallPath) == WIZ_OK)
  {
    LogISXPInstall(W_START);
    lstrcpy(szBuf, sgProduct.szPath);
    if(*sgProduct.szSubPath != '\0')
    {
      AppendBackSlash(szBuf, sizeof(szBuf));
      lstrcat(szBuf, sgProduct.szSubPath);
    }
    hrResult = pfnXpiInit(szBuf, FILE_INSTALL_LOG, cbXPIProgress);

    ShowMessage(NULL, FALSE);
    InitProgressDlg();
    GetTotalArchivesToInstall();
    SetWindowText(dlgInfo.hWndDlg, szDlgExtractingTitle);

    dwIndex0          = 0;
    dwCurrentArchive  = 0;
    dwTotalArchives   = (dwTotalArchives * 2) + 1;
    bBarberBar        = FALSE;
    siCObject         = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
    while(siCObject)
    {
      if(siCObject->dwAttributes & SIC_SELECTED)
        
         ProcessFileOps(T_PRE_ARCHIVE, siCObject->szReferenceName);

      
      if((siCObject->dwAttributes & SIC_SELECTED)   &&
        !(siCObject->dwAttributes & SIC_LAUNCHAPP) &&
        !(siCObject->dwAttributes & SIC_DOWNLOAD_ONLY))
      {
        lFileCounter      = 0;
        lBarberCounter    = 0;
        dwBarberDirection = BDIR_RIGHT;
			  dlgInfo.nFileBars = 0;
        UpdateGaugeFileProgressBar(0);

        lstrcpy(szArchive, sgProduct.szAlternateArchiveSearchPath);
        AppendBackSlash(szArchive, sizeof(szArchive));
        lstrcat(szArchive, siCObject->szArchiveName);
        if((*sgProduct.szAlternateArchiveSearchPath == '\0') || (!FileExists(szArchive)))
        {
          lstrcpy(szArchive, szSetupDir);
          AppendBackSlash(szArchive, sizeof(szArchive));
          lstrcat(szArchive, siCObject->szArchiveName);
          if(!FileExists(szArchive))
          {
            lstrcpy(szArchive, szTempDir);
            AppendBackSlash(szArchive, sizeof(szArchive));
            lstrcat(szArchive, siCObject->szArchiveName);
            if(!FileExists(szArchive))
            {
              char szEFileNotFound[MAX_BUF];

              if(GetPrivateProfileString("Messages", "ERROR_FILE_NOT_FOUND", "", szEFileNotFound, sizeof(szEFileNotFound), szFileIniInstall))
              {
                wsprintf(szBuf, szEFileNotFound, szArchive);
                PrintError(szBuf, ERROR_CODE_HIDE);
              }
              return(1);
            }
          }
        }

        if(dwCurrentArchive == 0)
        {
          ++dwCurrentArchive;
          UpdateGaugeArchiveProgressBar((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));
          UpdateGREAppInstallerProgress((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));
        }

        wsprintf(szBuf, szStrInstalling, siCObject->szDescriptionShort);
        SetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS0, szBuf);
        LogISXPInstallComponent(siCObject->szDescriptionShort);

        





        *xpiArgs = '\0';
        if(lstrcmpi(siCObject->szArchiveName, "gre.xpi") == 0)
          MozCopyStr(sgProduct.szRegPath, xpiArgs, sizeof(xpiArgs));
        else if((lstrcmpi(siCObject->szArchiveName, "browser.xpi") == 0) &&
                (sgProduct.greType == GRE_LOCAL))
          

          MozCopyStr("-greLocal", xpiArgs, sizeof(xpiArgs));

        hrResult = pfnXpiInstall(szArchive, xpiArgs, 0xFFFF);
        if(hrResult == E_REBOOT)
          bReboot = TRUE;
        else if((hrResult != WIZ_OK) &&
               !(siCObject->dwAttributes & SIC_IGNORE_XPINSTALL_ERROR))
        {
          LogMSXPInstallStatus(siCObject->szArchiveName, hrResult);
          LogISXPInstallComponentResult(hrResult);
          if(GetPrivateProfileString("Messages", "ERROR_XPI_INSTALL", "", szEXpiInstall, sizeof(szEXpiInstall), szFileIniInstall))
          {
            char szErrorString[MAX_BUF];

            GetErrorString(hrResult, szErrorString, sizeof(szErrorString));
            wsprintf(szBuf, "%s - %s: %d %s", szEXpiInstall, siCObject->szDescriptionShort, hrResult, szErrorString);
            PrintError(szBuf, ERROR_CODE_HIDE);
          }

          
          break;
        }

        ++dwCurrentArchive;
        UpdateGaugeArchiveProgressBar((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));
        UpdateGREAppInstallerProgress((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));

        ProcessWindowsMessages();
        LogISXPInstallComponentResult(hrResult);

        if((hrResult != WIZ_OK) &&
          (siCObject->dwAttributes & SIC_IGNORE_XPINSTALL_ERROR))
          



          hrResult = WIZ_OK;
      }

      if(siCObject->dwAttributes & SIC_SELECTED)
        
         ProcessFileOps(T_POST_ARCHIVE, siCObject->szReferenceName);

      ++dwIndex0;
      siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
    } 

    SaveWindowPosition(dlgInfo.hWndDlg);
    
    UpdateGREAppInstallerProgress(100);
    LogMSXPInstallStatus(NULL, hrResult);
    pfnXpiExit();
    DeInitProgressDlg();
  }

  DeInitializeXPIStub();
  LogISXPInstall(W_END);

  return(hrResult);
}

void cbXPIStart(const char *URL, const char *UIName)
{
}

void cbXPIProgress(const char* msg, PRInt32 val, PRInt32 max)
{
  char szFilename[MAX_BUF];
  char szStrProcessingFileBuf[MAX_BUF];
  char szStrCopyingFileBuf[MAX_BUF];

  if(sgProduct.mode != SILENT)
  {
    ParsePath((char *)msg, szFilename, sizeof(szFilename), FALSE, PP_FILENAME_ONLY);

    if(max == 0)
    {
      wsprintf(szStrProcessingFileBuf, szStrProcessingFile, szFilename);
      SetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS3, szStrProcessingFileBuf);
      bBarberBar = TRUE;
      UpdateGaugeFileBarber();
    }
    else
    {
      if(bBarberBar == TRUE)
      {
        dlgInfo.nFileBars = 0;
        ++dwCurrentArchive;
        UpdateGaugeArchiveProgressBar((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));
        UpdateGREAppInstallerProgress((unsigned)(((double)(dwCurrentArchive)/(double)dwTotalArchives)*(double)100));

        InvalidateBarberBarArea();
        bBarberBar = FALSE;
      }

      wsprintf(szStrCopyingFileBuf, szStrCopyingFile, szFilename);
      SetDlgItemText(dlgInfo.hWndDlg, IDC_STATUS3, szStrCopyingFileBuf);
      UpdateGaugeFileProgressBar((unsigned)(((double)(val+1)/(double)max)*(double)100));
    }
  }

  ProcessWindowsMessages();
}

void cbXPIFinal(const char *URL, PRInt32 finalStatus)
{
}







LRESULT CALLBACK
ProgressDlgProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
  {
		case WM_INITDIALOG:
      DisableSystemMenuItems(hWndDlg, TRUE);
      SendDlgItemMessage (hWndDlg, IDC_STATUS0, WM_SETFONT, (WPARAM)sgInstallGui.definedFont, 0L); 
      SendDlgItemMessage (hWndDlg, IDC_GAUGE_ARCHIVE, WM_SETFONT, (WPARAM)sgInstallGui.definedFont, 0L); 
      SendDlgItemMessage (hWndDlg, IDC_STATUS3, WM_SETFONT, (WPARAM)sgInstallGui.definedFont, 0L); 
      SendDlgItemMessage (hWndDlg, IDC_GAUGE_FILE, WM_SETFONT, (WPARAM)sgInstallGui.definedFont, 0L); 
			RepositionWindow(hWndDlg, BANNER_IMAGE_INSTALLING);
      ClosePreviousDialog();
			return FALSE;

		case WM_COMMAND:
			return TRUE;
	}

	return FALSE;  
}



static void
UpdateGaugeFileBarber()
{
	int	  nBars;
	HWND	hWndGauge;
	RECT	rect;

  if(sgProduct.mode != SILENT)
  {
	  hWndGauge = GetDlgItem(dlgInfo.hWndDlg, IDC_GAUGE_FILE);
    if(dwBarberDirection == BDIR_RIGHT)
    {
      
      
      
      if(lBarberCounter < 121)
        ++lBarberCounter;
      else
        dwBarberDirection = BDIR_LEFT;
    }
    else if(dwBarberDirection == BDIR_LEFT)
    {
      if(lBarberCounter > 0)
        --lBarberCounter;
      else
        dwBarberDirection = BDIR_RIGHT;
    }

    
    nBars = (dlgInfo.nMaxFileBars * lBarberCounter / 100);

    
    dlgInfo.nFileBars = nBars;

    
    GetClientRect(hWndGauge, &rect);
    InvalidateRect(hWndGauge, &rect, FALSE);

    
    
    
    UpdateWindow(dlgInfo.hWndDlg);
  }
}



static void
UpdateGaugeFileProgressBar(unsigned value)
{
	int	nBars;

  if(sgProduct.mode != SILENT)
  {
    
    nBars = dlgInfo.nMaxFileBars * value / 100;

    
    if((nBars > dlgInfo.nFileBars) || (dlgInfo.nFileBars == 0))
    {
      HWND	hWndGauge = GetDlgItem(dlgInfo.hWndDlg, IDC_GAUGE_FILE);
      RECT	rect;

      
      dlgInfo.nFileBars = nBars;

      
      GetClientRect(hWndGauge, &rect);
      InvalidateRect(hWndGauge, &rect, FALSE);
    
      
      
      
      UpdateWindow(dlgInfo.hWndDlg);
    }
  }
}



static void
UpdateGaugeArchiveProgressBar(unsigned value)
{
	int	nBars;

  if(sgProduct.mode != SILENT)
  {
    
    nBars = dlgInfo.nMaxArchiveBars * value / 100;

    
    if (nBars > dlgInfo.nArchiveBars)
    {
      HWND	hWndGauge = GetDlgItem(dlgInfo.hWndDlg, IDC_GAUGE_ARCHIVE);
      RECT	rect;

      
      dlgInfo.nArchiveBars = nBars;

      
      GetClientRect(hWndGauge, &rect);
      InvalidateRect(hWndGauge, &rect, FALSE);
    
      
      
      
      UpdateWindow(dlgInfo.hWndDlg);
    }
  }
}


static void
DrawGaugeBorder(HWND hWnd)
{
	HDC		hDC = GetWindowDC(hWnd);
	RECT	rect;
	int		cx, cy;
	HPEN	hShadowPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
	HGDIOBJ	hOldPen;

	GetWindowRect(hWnd, &rect);
	cx = rect.right - rect.left;
	cy = rect.bottom - rect.top;

	
	hOldPen = SelectObject(hDC, (HGDIOBJ)hShadowPen);
	MoveToEx(hDC, 0, cy - 1, NULL);
	LineTo(hDC, 0, 0);
	LineTo(hDC, cx - 1, 0);

	
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	MoveToEx(hDC, 0, cy - 1, NULL);
	LineTo(hDC, cx - 1, cy - 1);
	LineTo(hDC, cx - 1, 0);

	SelectObject(hDC, hOldPen);
	DeleteObject(hShadowPen);
	ReleaseDC(hWnd, hDC);
}


static void
DrawProgressBar(HWND hWnd, int nBars)
{
  int         i;
	PAINTSTRUCT	ps;
	HDC         hDC;
	RECT        rect;
	HBRUSH      hBrush;

  hDC = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rect);
  if(nBars <= 0)
  {
    
    hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
    FillRect(hDC, &rect, hBrush);
  }
  else
  {
  	
    hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
	  rect.left     = rect.top = BAR_MARGIN;
	  rect.bottom  -= BAR_MARGIN;
	  rect.right    = rect.left + BAR_WIDTH;

	  for(i = 0; i < nBars; i++)
    {
		  RECT	dest;

		  if(IntersectRect(&dest, &ps.rcPaint, &rect))
			  FillRect(hDC, &rect, hBrush);

      OffsetRect(&rect, BAR_WIDTH + BAR_SPACING, 0);
	  }
  }

	DeleteObject(hBrush);
	EndPaint(hWnd, &ps);
}


static void
DrawBarberBar(HWND hWnd, int nBars)
{
  int         i;
	PAINTSTRUCT	ps;
	HDC         hDC;
	RECT        rect;
	HBRUSH      hBrush      = NULL;
	HBRUSH      hBrushClear = NULL;

  hDC = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rect);
  if(nBars <= 0)
  {
    
    hBrushClear = CreateSolidBrush(GetSysColor(COLOR_MENU));
    FillRect(hDC, &rect, hBrushClear);
  }
  else
  {
  	
    hBrushClear   = CreateSolidBrush(GetSysColor(COLOR_MENU));
    hBrush        = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
	  rect.left     = rect.top = BAR_MARGIN;
	  rect.bottom  -= BAR_MARGIN;
	  rect.right    = rect.left + BAR_WIDTH;

	  for(i = 0; i < (nBars + 1); i++)
    {
		  RECT	dest;

		  if(IntersectRect(&dest, &ps.rcPaint, &rect))
      {
        if((i >= (nBars - 15)) && (i < nBars))
			    FillRect(hDC, &rect, hBrush);
        else
			    FillRect(hDC, &rect, hBrushClear);
      }

      OffsetRect(&rect, BAR_WIDTH + BAR_SPACING, 0);
	  }
  }

  if(hBrushClear)
    DeleteObject(hBrushClear);

  if(hBrush)
    DeleteObject(hBrush);

	EndPaint(hWnd, &ps);
}


static void
SizeToFitGauge(HWND hWnd, int nMaxBars)
{
	RECT	rect;
	int		cx;

	
	GetWindowRect(hWnd, &rect);

	
	cx = 2 * GetSystemMetrics(SM_CXBORDER) + 2 * BAR_MARGIN +
		nMaxBars * BAR_WIDTH + (nMaxBars - 1) * BAR_SPACING;

	SetWindowPos(hWnd, NULL, -1, -1, cx, rect.bottom - rect.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}


LRESULT CALLBACK
GaugeFileWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DWORD	dwStyle;
	RECT	rect;

	switch(msg)
  {
		case WM_NCCREATE:
			dwStyle = GetWindowLong(hWnd, GWL_STYLE);
			SetWindowLong(hWnd, GWL_STYLE, dwStyle | WS_BORDER);
			return(TRUE);

		case WM_CREATE:
			
			GetClientRect(hWnd, &rect);
			dlgInfo.nFileBars = 0;
			dlgInfo.nMaxFileBars = (rect.right - rect.left - 2 * BAR_MARGIN + BAR_SPACING) / (BAR_WIDTH + BAR_SPACING);

			
			SizeToFitGauge(hWnd, dlgInfo.nMaxFileBars);
			return(FALSE);

		case WM_NCPAINT:
			DrawGaugeBorder(hWnd);
			return(FALSE);

		case WM_PAINT:
      if(bBarberBar == TRUE)
			  DrawBarberBar(hWnd, dlgInfo.nFileBars);
      else
			  DrawProgressBar(hWnd, dlgInfo.nFileBars);

			return(FALSE);
	}

	return(DefWindowProc(hWnd, msg, wParam, lParam));
}


LRESULT CALLBACK
GaugeArchiveWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DWORD	dwStyle;
	RECT	rect;

	switch(msg)
  {
		case WM_NCCREATE:
			dwStyle = GetWindowLong(hWnd, GWL_STYLE);
			SetWindowLong(hWnd, GWL_STYLE, dwStyle | WS_BORDER);
			return(TRUE);

		case WM_CREATE:
			
			GetClientRect(hWnd, &rect);
			dlgInfo.nArchiveBars = 0;
			dlgInfo.nMaxArchiveBars = (rect.right - rect.left - 2 * BAR_MARGIN + BAR_SPACING) / (BAR_WIDTH + BAR_SPACING);

			
			SizeToFitGauge(hWnd, dlgInfo.nMaxArchiveBars);
			return(FALSE);

		case WM_NCPAINT:
			DrawGaugeBorder(hWnd);
			return(FALSE);

		case WM_PAINT:
			DrawProgressBar(hWnd, dlgInfo.nArchiveBars);
			return(FALSE);
	}

	return(DefWindowProc(hWnd, msg, wParam, lParam));
}

void InitProgressDlg()
{
	WNDCLASS	wc;

  if(sgProduct.mode != SILENT)
  {
    memset(&wc, 0, sizeof(wc));
    wc.style          = CS_GLOBALCLASS;
    wc.hInstance      = hInst;
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpfnWndProc    = (WNDPROC)GaugeFileWndProc;
    wc.lpszClassName  = "GaugeFile";
    RegisterClass(&wc);

    wc.lpfnWndProc    = (WNDPROC)GaugeArchiveWndProc;
    wc.lpszClassName  = "GaugeArchive";
    RegisterClass(&wc);

    
    dlgInfo.hWndDlg = CreateDialog(hSetupRscInst, MAKEINTRESOURCE(DLG_EXTRACTING), hWndMain, (WNDPROC)ProgressDlgProc);
    UpdateWindow(dlgInfo.hWndDlg);
  }
}

void DeInitProgressDlg()
{
  if(sgProduct.mode != SILENT)
  {
    SaveWindowPosition(dlgInfo.hWndDlg);
    DestroyWindow(dlgInfo.hWndDlg);
    UnregisterClass("GaugeFile", hInst);
    UnregisterClass("GaugeArchive", hInst);
  }
}
