































#include <stdio.h>


#define INCL_WIN
#define INCL_DOS
#include <os2.h>


#include "nsEmbedAPI.h"
#include "nsWeakReference.h"
#include "nsIClipboardCommands.h"
#include "nsXPIDLString.h"
#include "nsIWebBrowserPersist.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWindowWatcher.h"
#include "nsIProfile.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsIProfileChangeStatus.h"
#include "nsIURI.h"
#include "plstr.h"
#include "nsIInterfaceRequestor.h"
#include "nsCRT.h"


#include "os2Embed.h"
#include "WebBrowserChrome.h"
#include "WindowCreator.h"
#include "resource.h"
#include "nsStaticComponents.h"


#include "nsIPrintSettings.h"
#include "nsIWebBrowserPrint.h"

#define MAX_LOADSTRING 100


#ifndef _BUILD_STATIC_BIN
nsStaticModuleInfo const *const kPStaticModules = nsnull;
PRUint32 const kStaticModuleCount = 0;
#endif

const CHAR *szWindowClass = "OS2EMBED";


static void             MyRegisterClass();
static MRESULT EXPENTRY BrowserWndProc(HWND, ULONG, MPARAM, MPARAM);
static MRESULT EXPENTRY BrowserDlgProc(HWND hwndDlg, ULONG uMsg, MPARAM wParam, MPARAM lParam);

static nsresult InitializeWindowCreator();
static nsresult OpenWebPage(const char * url);
static nsresult ResizeEmbedding(nsIWebBrowserChrome* chrome);


static BOOL ChooseNewProfile(BOOL bShowForMultipleProfilesOnly, const char *szDefaultProfile);
static MRESULT EXPENTRY ChooseProfileDlgProc(HWND, ULONG, MPARAM, MPARAM);


static UINT gDialogCount = 0;
static BOOL gProfileSwitch = FALSE;
static HMODULE ghInstanceResources = NULL;
static char gFirstURL[1024];


static const CHAR *gDefaultURLs[] = 
{
    ("http://www.mozilla.org/"),
    ("http://www.netscape.com/"),
    ("http://browsertest.web.aol.com/tests/javascript/javascpt/index.htm"),
    ("http://127.0.0.1/"),
    ("http://www.yahoo.com/"),
    ("http://www.travelocity.com/"),
    ("http://www.disney.com/"),
    ("http://www.go.com/"),
    ("http://www.google.com/"),
    ("http://www.ebay.com/"),
    ("http://www.shockwave.com/"),
    ("http://www.slashdot.org/"),
    ("http://www.quicken.com/"),
    ("http://www.hotmail.com/"),
    ("http://www.cnn.com/"),
    ("http://www.javasoft.com/")
};

class ProfileChangeObserver : public nsIObserver,
                              public nsSupportsWeakReference

{
public:
	 ProfileChangeObserver();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};


int main(int argc, char *argv[])
{
    printf("You are embedded, man!\n\n");

    
    char *szFirstURL = "http://www.mozilla.org/projects/embedding";
    char *szDefaultProfile = nsnull;
    int argn;
    for (argn = 1; argn < argc; argn++)
    {
        if (stricmp("-P", argv[argn]) == 0)
        {
            if (argn + 1 < argc)
            {
                szDefaultProfile = argv[++argn];
            }
        }
        else
        {
            szFirstURL = argv[argn];
        }
    }
    strncpy(gFirstURL, szFirstURL, sizeof(gFirstURL) - 1);

    
    CHAR szTitle[MAX_LOADSTRING];
    WinLoadString((HAB)0, ghInstanceResources, IDS_APP_TITLE, MAX_LOADSTRING, szTitle);
    MyRegisterClass();

    
    NS_InitEmbedding(nsnull, nsnull, kPStaticModules, kStaticModuleCount);

    
    if (!ChooseNewProfile(TRUE, szDefaultProfile))
    {
        NS_TermEmbedding();
        return 1;
    }
    MPARAM rv;
    {    
    	
        nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1"));

        ProfileChangeObserver *observer = new ProfileChangeObserver;
        observer->AddRef();
        observerService->AddObserver(NS_STATIC_CAST(nsIObserver *, observer), "profile-approve-change", PR_TRUE);
        observerService->AddObserver(NS_STATIC_CAST(nsIObserver *, observer), "profile-change-teardown", PR_TRUE);
        observerService->AddObserver(NS_STATIC_CAST(nsIObserver *, observer), "profile-after-change", PR_TRUE);

        InitializeWindowCreator();

        
        OpenWebPage(gFirstURL);

        
        
        
        PRBool runCondition = PR_TRUE;

        rv = (MPARAM)RunEventLoop(runCondition);

        observer->Release();
    }
    
    NS_TermEmbedding();

    return (int)rv;
}





NS_IMPL_THREADSAFE_ISUPPORTS2(ProfileChangeObserver, nsIObserver, nsISupportsWeakReference)

ProfileChangeObserver::ProfileChangeObserver()
{
}





NS_IMETHODIMP ProfileChangeObserver::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;

    if (nsCRT::strcmp(aTopic, "profile-approve-change") == 0)
    {
		

        
        int result = ::WinMessageBox(HWND_DESKTOP, NULL, "Do you want to close all windows in order to switch the profile?", "Confirm", 101, MB_YESNO | MB_ICONQUESTION);
        if (result != MBID_YES)
        {
            nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
            NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
            status->VetoChange();
        }
    }
    else if (nsCRT::strcmp(aTopic, "profile-change-teardown") == 0)
    {
		

		
		gDialogCount++;
    }
    else if (nsCRT::strcmp(aTopic, "profile-after-change") == 0)
    {
		
		gDialogCount--;
        if (gDialogCount == 0)
        {
            
            OpenWebPage(gFirstURL);
        }
        else
        {
		    
            
            
            gProfileSwitch = TRUE;
        }
    }

    return rv;
}




nsresult InitializeWindowCreator()
{
    
    WindowCreator *creatorCallback = new WindowCreator();
    if (creatorCallback)
    {
        nsCOMPtr<nsIWindowCreator> windowCreator(NS_STATIC_CAST(nsIWindowCreator *, creatorCallback));
        if (windowCreator)
        {
            nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
            if (wwatch)
            {
                wwatch->SetWindowCreator(windowCreator);
                return NS_OK;
            }
        }
    }
    return NS_ERROR_FAILURE;
}









nsresult OpenWebPage(const char *url)
{
    nsresult  rv;

    
    
    

    nsCOMPtr<nsIWebBrowserChrome> chrome;
    rv = CreateBrowserWindow(nsIWebBrowserChrome::CHROME_ALL,
           nsnull, getter_AddRefs(chrome));
    if (NS_SUCCEEDED(rv))
    {
        
        nsCOMPtr<nsIWebBrowser> newBrowser;
        chrome->GetWebBrowser(getter_AddRefs(newBrowser));
        nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(newBrowser));
        return webNav->LoadURI(NS_ConvertASCIItoUTF16(url).get(),
                               nsIWebNavigation::LOAD_FLAGS_NONE,
                               nsnull,
                               nsnull,
                               nsnull);
    }

    return rv;
}   







HWND GetBrowserFromChrome(nsIWebBrowserChrome *aChrome)
{
    if (!aChrome)
    {
        return NULL;
    }
    nsCOMPtr<nsIEmbeddingSiteWindow> baseWindow = do_QueryInterface(aChrome);
    HWND hwnd = NULL;
    baseWindow->GetSiteWindow((void **) & hwnd);
    return hwnd;
}








HWND GetBrowserDlgFromChrome(nsIWebBrowserChrome *aChrome)
{
    return WinQueryWindow(GetBrowserFromChrome(aChrome), QW_PARENT);
}







void SaveWebPage(HWND hDlg, nsIWebBrowser *aWebBrowser)
{
    
    nsCOMPtr<nsIBaseWindow> webBrowserAsWin = do_QueryInterface(aWebBrowser);
    nsXPIDLString windowTitle;
    webBrowserAsWin->GetTitle(getter_Copies(windowTitle));
    nsCString fileName; fileName.AssignWithConversion(windowTitle);

	
    fileName.CompressWhitespace();     
    fileName.StripChars("\\*|:\"><?"); 
    fileName.ReplaceChar('.', L'_');   
    fileName.ReplaceChar('/', L'-');   
    fileName.ReplaceChar(' ', L'_');   

    
    FILEDLG saveFileNameInfo;
    memset(&saveFileNameInfo, 0, sizeof(saveFileNameInfo));
    saveFileNameInfo.cbSize = sizeof(saveFileNameInfo);
    PL_strncpyz(saveFileNameInfo.szFullFile, fileName.get(), CCHMAXPATH);
    strcat(saveFileNameInfo.szFullFile, ".html");

    PSZ *apszTypeList = (PSZ *)malloc(3 * sizeof(PSZ) + 1);
    apszTypeList[0] = "Web Page, HTML Only (*.htm;*.html)";
    apszTypeList[1] = "Web Page, Complete (*.htm;*.html)";
    apszTypeList[2] = "Text File (*.txt)";
    apszTypeList[3] = 0;
    saveFileNameInfo.papszITypeList = (PAPSZ)apszTypeList; 
    saveFileNameInfo.pszTitle = NULL; 
    saveFileNameInfo.fl = FDS_SAVEAS_DIALOG | FDS_CENTER | FDS_ENABLEFILELB; 
    saveFileNameInfo.pszIType = apszTypeList[0];

    WinFileDlg(HWND_DESKTOP, hDlg, &saveFileNameInfo);
    if (saveFileNameInfo.lReturn == DID_OK)  
    {
        char *pszDataPath = NULL;
        static char szDataFile[_MAX_PATH];
        char szDataPath[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];

        _splitpath(saveFileNameInfo.szFullFile, drive, dir, fname, ext);
        
        if (strcmp(ext, "") == 0) {
          if ((saveFileNameInfo.sEAType == 2) && (stricmp(ext, ".txt") != 0)) {
            strcat(saveFileNameInfo.szFullFile, ".txt");
            strcpy(ext, ".txt");
          } else 
            if ((stricmp(ext, ".html") != 0) && (stricmp(ext, ".htm") != 0)) {
              strcat(saveFileNameInfo.szFullFile, ".html");
              strcpy(ext, ".html");
          }
        }

        
        
        if (saveFileNameInfo.sEAType == 1) 
        {
            sprintf(szDataFile, "%s_files", fname);
            _makepath(szDataPath, drive, dir, szDataFile, "");

            pszDataPath = szDataPath;
       }

        
        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(aWebBrowser));

        nsCOMPtr<nsILocalFile> file;
        NS_NewNativeLocalFile(nsDependentCString(saveFileNameInfo.szFullFile), TRUE, getter_AddRefs(file));

        nsCOMPtr<nsILocalFile> dataPath;
        if (pszDataPath)
        {
            NS_NewNativeLocalFile(nsDependentCString(pszDataPath), TRUE, getter_AddRefs(dataPath));
        }

        persist->SaveDocument(nsnull, file, dataPath, nsnull, 0, 0);
    }
    if (saveFileNameInfo.papszFQFilename) 
       WinFreeFileDlgList(saveFileNameInfo.papszFQFilename);
    for (int i = 0; i < 3; i++)
       free(saveFileNameInfo.papszITypeList[i]);
    free(saveFileNameInfo.papszITypeList);
}







nsresult ResizeEmbedding(nsIWebBrowserChrome* chrome)
{
    if (!chrome)
        return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIEmbeddingSiteWindow> embeddingSite = do_QueryInterface(chrome);
    HWND hWnd;
	embeddingSite->GetSiteWindow((void **) & hWnd);
    
    if (!hWnd)
        return NS_ERROR_NULL_POINTER;
    
    RECTL rect;
    WinQueryWindowRect(hWnd, &rect);
    
    
    nsCOMPtr<nsIWebBrowser> webBrowser;
    chrome->GetWebBrowser(getter_AddRefs(webBrowser));
    nsCOMPtr<nsIBaseWindow> webBrowserAsWin = do_QueryInterface(webBrowser);
    if (webBrowserAsWin)
    {
        webBrowserAsWin->SetPositionAndSize(rect.xLeft, 
                                            rect.yBottom, 
                                            rect.xRight - rect.xLeft,
                                            rect.yTop - rect.yBottom,
                                            PR_TRUE);
        webBrowserAsWin->SetVisibility(PR_TRUE);
	}

    return NS_OK;
}










void MyRegisterClass()
{
    WinRegisterClass((HAB)0, szWindowClass, BrowserWndProc, CS_SIZEREDRAW, sizeof(ULONG));
}







void UpdateUI(nsIWebBrowserChrome *aChrome)
{
    HWND hwndDlg = GetBrowserDlgFromChrome(aChrome);
    nsCOMPtr<nsIWebBrowser> webBrowser;
    nsCOMPtr<nsIWebNavigation> webNavigation;
    aChrome->GetWebBrowser(getter_AddRefs(webBrowser));
    webNavigation = do_QueryInterface(webBrowser);

    PRBool canGoBack = PR_FALSE;
    PRBool canGoForward = PR_FALSE;
    if (webNavigation)
    {
        webNavigation->GetCanGoBack(&canGoBack);
        webNavigation->GetCanGoForward(&canGoForward);
    }

    PRBool canCutSelection = PR_FALSE;
    PRBool canCopySelection = PR_FALSE;
    PRBool canPaste = PR_FALSE;

    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(webBrowser);
    if (clipCmds)
    {
        clipCmds->CanCutSelection(&canCutSelection);
        clipCmds->CanCopySelection(&canCopySelection);
        clipCmds->CanPaste(&canPaste);
    }

    HWND hmenu = WinWindowFromID(hwndDlg, FID_MENU);
    if (hmenu)
    {
        MENUITEM goMenu, editMenu;
        WinSendMsg(hmenu, MM_QUERYITEM, MPFROM2SHORT(MOZ_Go, TRUE), (MPARAM)&goMenu);
        WinEnableMenuItem(goMenu.hwndSubMenu, MOZ_GoBack, canGoBack);
        WinEnableMenuItem(goMenu.hwndSubMenu, MOZ_GoForward, canGoForward);

        WinSendMsg(hmenu, MM_QUERYITEM, MPFROM2SHORT(MOZ_Edit, TRUE), (MPARAM)&editMenu);
        WinEnableMenuItem(editMenu.hwndSubMenu, MOZ_Cut, canCutSelection);
        WinEnableMenuItem(editMenu.hwndSubMenu, MOZ_Copy, canCopySelection);
        WinEnableMenuItem(editMenu.hwndSubMenu, MOZ_Paste, canPaste);
    }

    HWND button;
    button = WinWindowFromID(hwndDlg, IDC_BACK);
    if (button)
      WinEnableWindow(button, canGoBack);
    button = WinWindowFromID(hwndDlg, IDC_FORWARD);
    if (button)
      WinEnableWindow(button, canGoForward);
}











MRESULT EXPENTRY BrowserDlgProc(HWND hwndDlg, ULONG uMsg, MPARAM wParam, MPARAM lParam)
{
    if (uMsg == WM_COMMAND && SHORT1FROMMP(wParam) == MOZ_SwitchProfile)
    {
       ChooseNewProfile(FALSE, NULL);
       return (MRESULT)FALSE;
    }

    
    HWND hwndBrowser = WinWindowFromID(hwndDlg, IDC_BROWSER);
    nsIWebBrowserChrome *chrome = nsnull ;
    if (hwndBrowser)
    {
        chrome = (nsIWebBrowserChrome *) WinQueryWindowULong(hwndBrowser, QWL_USER);
    }
    nsCOMPtr<nsIWebBrowser> webBrowser;
    nsCOMPtr<nsIWebNavigation> webNavigation;
    nsCOMPtr<nsIWebBrowserPrint> webBrowserPrint;
    if (chrome)
    {
        chrome->GetWebBrowser(getter_AddRefs(webBrowser));
        webNavigation = do_QueryInterface(webBrowser);
        webBrowserPrint = do_GetInterface(webBrowser);
    }

    
    switch (uMsg)
    {
    case WM_INITDLG:
        return (MRESULT)TRUE;

    case WM_INITMENU:
        UpdateUI(chrome);
        return (MRESULT)TRUE;

    case WM_SYSCOMMAND:
        if (SHORT1FROMMP(wParam) == SC_CLOSE)
        {
            WebBrowserChromeUI::Destroy(chrome);
            return (MRESULT)TRUE;
        }
        break;

    case WM_DESTROY:
	    return (MRESULT)TRUE;

    case WM_COMMAND:
        if (!webBrowser)
        {
            return (MRESULT)TRUE;
        }

        
        switch (SHORT1FROMMP(wParam))
		{
        case IDC_ADDRESS:
            if (SHORT1FROMMP(wParam) == CBN_EFCHANGE || SHORT1FROMMP(wParam) == CBN_LBSELECT)
            {
                
                WinEnableWindow(WinWindowFromID(hwndDlg, IDC_GO), TRUE);
            }
            break;

        case IDC_GO:
            {
                char szURL[2048];
                memset(szURL, 0, sizeof(szURL));
                WinQueryDlgItemText(hwndDlg, IDC_ADDRESS, sizeof(szURL) / sizeof(szURL[0]) - 1, szURL);
                webNavigation->LoadURI(
                    NS_ConvertASCIItoUTF16(szURL).get(),
                    nsIWebNavigation::LOAD_FLAGS_NONE,
                    nsnull,
                    nsnull,
                    nsnull);
            }
            break;

        case IDC_STOP:
            webNavigation->Stop(nsIWebNavigation::STOP_ALL);
            UpdateUI(chrome);
            break;

        case IDC_RELOAD:
            webNavigation->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
            break;

        case IDM_EXIT:
            WinPostMsg(hwndDlg, WM_SYSCOMMAND, MPFROMSHORT(SC_CLOSE), 0);
            break;

        

        case MOZ_NewBrowser:
            OpenWebPage(gFirstURL);
            break;

        case MOZ_Save:
            SaveWebPage(hwndDlg, webBrowser);
            break;

        case MOZ_Print:
            {
                
                
                
                
              if (webBrowserPrint)
              {
                  nsCOMPtr<nsIPrintSettings> printSettings;
                  webBrowserPrint->GetGlobalPrintSettings(getter_AddRefs(printSettings));
                  NS_ASSERTION(printSettings, "You can't PrintPreview without a PrintSettings!");
                  if (printSettings) 
                  {
                      printSettings->SetPrintSilent(PR_TRUE);
                      webBrowserPrint->Print(printSettings, (nsIWebProgressListener*)nsnull);
                  }
              }
            }
            break;

        

        case MOZ_Cut:
            {
                nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(webBrowser);
                clipCmds->CutSelection();
            }
            break;

        case MOZ_Copy:
            {
                nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(webBrowser);
                clipCmds->CopySelection();
            }
            break;

        case MOZ_Paste:
            {
                nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(webBrowser);
                clipCmds->Paste();
            }
            break;

        case MOZ_SelectAll:
            {
                nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(webBrowser);
                clipCmds->SelectAll();
            }
            break;

        case MOZ_SelectNone:
            {
                nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(webBrowser);
                clipCmds->SelectNone();
            }
            break;

        
        case IDC_BACK:
        case MOZ_GoBack:
            webNavigation->GoBack();
            UpdateUI(chrome);
            break;

        case IDC_FORWARD:
        case MOZ_GoForward:
            webNavigation->GoForward();
            UpdateUI(chrome);
            break;

        
        case MOZ_About:
            {
                char szAboutTitle[MAX_LOADSTRING];
                char szAbout[MAX_LOADSTRING];
                WinLoadString((HAB)0, ghInstanceResources, IDS_ABOUT_TITLE, MAX_LOADSTRING, szAboutTitle);
                WinLoadString((HAB)0, ghInstanceResources, IDS_ABOUT, MAX_LOADSTRING, szAbout);
                WinMessageBox(HWND_DESKTOP, NULL, szAbout, szAboutTitle, 0, MB_OK | MB_APPLMODAL);
            }
            break;
		}

	    return (MRESULT)TRUE;

    case WM_ACTIVATE:
        {
            nsCOMPtr<nsIWebBrowserFocus> focus(do_GetInterface(webBrowser));
            if(focus)
            {
                switch (SHORT1FROMMP(wParam))
                {
                case TRUE: 
                    focus->Activate();
                    break;
                case FALSE: 
                    focus->Deactivate();
                    break;
                default:
                    break;
                }
            }
        }
        break;

    case WM_ADJUSTWINDOWPOS: 
        {
            PSWP swp = (PSWP)wParam;
            if (swp->fl & (SWP_SIZE)) {
               UINT newDlgWidth = swp->cx;
               UINT newDlgHeight = swp->cy;

               
               
               
               
               int progressWidth, statusWidth, addressWidth, goWidth, forwardWidth, reloadWidth, stopWidth, backWidth, staticWidth;
               int statusHeight, backHeight, buttonHeight, addressHeight, comboboxHeight;

               HWND hwndStatus = WinWindowFromID(hwndDlg, IDC_STATUS);
               if (hwndStatus) {
                 RECTL rcStatus;
                 WinQueryWindowRect(hwndStatus, &rcStatus);
                 statusHeight = rcStatus.yTop - rcStatus.yBottom;
               } else
                 statusHeight = 0;
   
               HWND hwndProgress = WinWindowFromID(hwndDlg, IDC_PROGRESS);
               if (hwndProgress) {
                 RECTL rcProgress;
                 WinQueryWindowRect(hwndProgress, &rcProgress);
                 progressWidth = rcProgress.xRight - rcProgress.xLeft;
               } else
                 progressWidth = 0;
               statusWidth = newDlgWidth - progressWidth;
   
               HWND hwndBack = WinWindowFromID(hwndDlg, IDC_BACK);
               if (hwndBack) {
                 RECTL rcBack;
                 WinQueryWindowRect(hwndBack, &rcBack);
                 backHeight = rcBack.yTop - rcBack.yBottom;
                 backWidth = rcBack.xRight - rcBack.xLeft;
               } else {
                 backHeight = 0;
                 backWidth = 0;
               }
               buttonHeight = newDlgHeight - backHeight - 50;

               HWND hwndForward = WinWindowFromID(hwndDlg, IDC_FORWARD);
               if (hwndForward) {
                 RECTL rcForward;
                 WinQueryWindowRect(hwndForward, &rcForward);
                 forwardWidth = rcForward.xRight - rcForward.xLeft;
               } else
                 forwardWidth = 0;

               HWND hwndReload = WinWindowFromID(hwndDlg, IDC_RELOAD);
               if (hwndReload) {
                 RECTL rcReload;
                 WinQueryWindowRect(hwndReload, &rcReload);
                 reloadWidth = rcReload.xRight - rcReload.xLeft;
               } else
                 reloadWidth = 0;

               HWND hwndStop = WinWindowFromID(hwndDlg, IDC_STOP);
               if (hwndStop) {
                 RECTL rcStop;
                 WinQueryWindowRect(hwndStop, &rcStop);
                 stopWidth = rcStop.xRight - rcStop.xLeft;
               } else
                 stopWidth = 0;

               HWND hwndStatic = WinWindowFromID(hwndDlg, IDC_ADDRESSLABEL);
               if (hwndStatic) {
                 RECTL rcStatic;
                 WinQueryWindowRect(hwndStatic, &rcStatic);
                 staticWidth = rcStatic.xRight - rcStatic.xLeft;
               } else
                 staticWidth = 0;

               HWND hwndGo = WinWindowFromID(hwndDlg, IDC_GO);
               if (hwndGo) {
                 RECTL rcGo;
                 WinQueryWindowRect(hwndGo, &rcGo);
                 goWidth = rcGo.xRight - rcGo.xLeft;
               } else
                 goWidth = 0;

               HWND hwndAddress = WinWindowFromID(hwndDlg, IDC_ADDRESS);
               if (hwndAddress) {
                 RECTL rcAddress;
                 WinQueryWindowRect(hwndAddress, &rcAddress);
                 addressHeight = rcAddress.yTop - rcAddress.yBottom;
                 comboboxHeight = buttonHeight + backHeight - addressHeight;
               } else {
                 addressHeight = 0;
                 comboboxHeight = 0;
               }
               addressWidth = newDlgWidth - goWidth - backWidth - forwardWidth - reloadWidth - stopWidth - staticWidth - 15;

               if (hwndStatus)
                 WinSetWindowPos(hwndStatus,
                                 HWND_TOP,
                                 0, 0, 
                                 statusWidth,
                                 statusHeight,
                                 SWP_MOVE | SWP_SIZE | SWP_SHOW);
               if (hwndProgress)
                 WinSetWindowPos(hwndProgress,
                                 HWND_TOP,
                                 statusWidth, 0, 
                                 0, 0,
                                 SWP_MOVE | SWP_SHOW);
               if (hwndBack)
                 WinSetWindowPos(hwndBack,
                                 HWND_TOP,
                                 2, buttonHeight,
                                 0, 0,
                                 SWP_MOVE | SWP_SHOW);
               if (hwndForward)
                 WinSetWindowPos(hwndForward,
                                 HWND_TOP,
                                 2 + backWidth, buttonHeight, 
                                 0, 0,
                                 SWP_MOVE | SWP_SHOW);
               if (hwndReload)
                 WinSetWindowPos(hwndReload,
                                 HWND_TOP,
                                 4 + backWidth + forwardWidth, buttonHeight,
                                 0, 0,
                                 SWP_MOVE | SWP_SHOW);
               if (hwndStop)
                 WinSetWindowPos(hwndStop,
                                 HWND_TOP,
                                 5 + backWidth + forwardWidth + reloadWidth, buttonHeight, 
                                 0, 0,
                                 SWP_MOVE | SWP_SHOW);
               if (hwndStatic)
                 WinSetWindowPos(hwndStatic,
                                 HWND_TOP,
                                 9 + backWidth + forwardWidth + reloadWidth + stopWidth, buttonHeight + 3,
                                 0, 0,
                                 SWP_MOVE | SWP_SHOW);
               if (hwndAddress)
                 WinSetWindowPos(hwndAddress,
                                 HWND_TOP,
                                 12 + backWidth + forwardWidth + reloadWidth + stopWidth + staticWidth, comboboxHeight,
                                 addressWidth, addressHeight,
                                 SWP_MOVE | SWP_SIZE | SWP_SHOW);
               if (hwndGo)
                 WinSetWindowPos(hwndGo,
                                 HWND_TOP,
                                 13 + backWidth + forwardWidth + reloadWidth + stopWidth + staticWidth + addressWidth, buttonHeight,
                                 0, 0,
                                 SWP_MOVE | SWP_SHOW);

               
               
               WinSetWindowPos(hwndBrowser,
                               HWND_TOP,
                               2, statusHeight,
                               newDlgWidth - 4,
                               newDlgHeight - backHeight - statusHeight - 52,
                               SWP_MOVE | SWP_SIZE | SWP_SHOW);
            }
        }
        return (MRESULT)TRUE;
    }
    return WinDefDlgProc(hwndDlg, uMsg, wParam, lParam);
}







MRESULT EXPENTRY BrowserWndProc(HWND hWnd, ULONG message, MPARAM wParam, MPARAM lParam)
{
    nsIWebBrowserChrome *chrome = (nsIWebBrowserChrome *) WinQueryWindowULong(hWnd, QWL_USER);
	switch (message) 
	{
    case WM_SIZE:
        
        ResizeEmbedding(chrome);
        return (MRESULT)0;
    case WM_ERASEBACKGROUND:
        
        return (MRESULT)1;
    }
    return WinDefWindowProc(hWnd, message, wParam, lParam);
}















BOOL ChooseNewProfile(BOOL bShowForMultipleProfilesOnly, const char *szDefaultProfile)
{
    nsresult rv;
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
    {
        return FALSE;
    }

    if (szDefaultProfile)
    {
        
        nsAutoString newProfileName; newProfileName.AssignWithConversion(szDefaultProfile);
        rv = profileService->CreateNewProfile(newProfileName.get(), nsnull, nsnull, PR_FALSE);
        if (NS_FAILED(rv)) return FALSE;
        rv = profileService->SetCurrentProfile(newProfileName.get());
        if (NS_FAILED(rv)) return FALSE;
        return TRUE;
    }

    PRInt32 profileCount = 0;
    rv = profileService->GetProfileCount(&profileCount);
    if (profileCount == 0)
    {
        
        NS_NAMED_LITERAL_STRING(newProfileName, "os2Embed");
        rv = profileService->CreateNewProfile(newProfileName.get(), nsnull, nsnull, PR_FALSE);
        if (NS_FAILED(rv)) return FALSE;
        rv = profileService->SetCurrentProfile(newProfileName.get());
        if (NS_FAILED(rv)) return FALSE;
        return TRUE;
    }
    else if (profileCount == 1 && bShowForMultipleProfilesOnly)
    {
        
        
        
        nsXPIDLString   currProfileName;
        rv = profileService->GetCurrentProfile(getter_Copies(currProfileName));
        if (NS_FAILED(rv)) return FALSE;
        rv = profileService->SetCurrentProfile(currProfileName);
        if (NS_FAILED(rv)) return FALSE;
        return TRUE;
    }

    INT nResult;
    nResult = WinDlgBox(HWND_DESKTOP, HWND_DESKTOP, (PFNWP)ChooseProfileDlgProc, NULL, IDD_CHOOSEPROFILE, (PVOID)ghInstanceResources);
    return (nResult == DID_OK) ? TRUE : FALSE;
}







MRESULT EXPENTRY ChooseProfileDlgProc(HWND hDlg, ULONG message, MPARAM wParam, MPARAM lParam)
{
    nsresult rv;
	switch (message)
	{
	case WM_INITDLG:
        {
            WinSetActiveWindow(HWND_DESKTOP, hDlg);
            HWND hwndProfileList = WinWindowFromID(hDlg, IDC_PROFILELIST);

            nsCOMPtr<nsIProfile> profileService = 
                     do_GetService(NS_PROFILE_CONTRACTID, &rv);

            
            PRUint32 listLen = 0;
            PRUnichar **profileList = nsnull;
            rv = profileService->GetProfileList(&listLen, &profileList);
            for (PRUint32 index = 0; index < listLen; index++)
            {
#ifdef UNICODE
                WinSendMsg(hwndProfileList, LM_INSERTITEM, (MPARAM)LIT_END, (MPARAM) profileList[index]);
#else
                nsCAutoString profile; profile.AssignWithConversion(profileList[index]);
                WinSendMsg(hwndProfileList, LM_INSERTITEM, (MPARAM)LIT_END, (MPARAM) profile.get());
#endif
            }

            

            
#ifdef UNICODE
            nsXPIDLString currProfile;
            profileService->GetCurrentProfile(getter_Copies(currProfile));
#else
            nsXPIDLString currProfileUnicode;
            profileService->GetCurrentProfile(getter_Copies(currProfileUnicode));
            nsCAutoString currProfile; currProfile.AssignWithConversion(currProfileUnicode);
#endif

            
            LONG currentProfileIndex = LIT_ERROR;
            currentProfileIndex = (LONG)WinSendMsg(hwndProfileList, LM_SEARCHSTRING, MPFROM2SHORT(LSS_CASESENSITIVE, LIT_FIRST), (MPARAM) currProfile.get());
            if (currentProfileIndex != LIT_ERROR)
            {
                WinSendMsg(hwndProfileList, LM_SELECTITEM, (MPARAM)currentProfileIndex, (MPARAM)TRUE);
            }
        
		return (MRESULT)TRUE;
        }
     case WM_COMMAND:
        if (SHORT1FROMMP(wParam) == DID_OK)
        {
            HWND hwndProfileList = WinWindowFromID(hDlg, IDC_PROFILELIST);

            
            LONG currentProfileIndex = (LONG)WinSendMsg(hwndProfileList, LM_QUERYSELECTION, (MPARAM)LIT_FIRST, (MPARAM)0);
            if (currentProfileIndex != LIT_ERROR)
            {
                nsCOMPtr<nsIProfile> profileService = 
                         do_GetService(NS_PROFILE_CONTRACTID, &rv);
                
                INT profileNameLen = (INT)WinSendMsg(hwndProfileList, LM_QUERYITEMTEXTLENGTH, (MPARAM)currentProfileIndex, 0);
                char *profileName = new char[profileNameLen + 1];
                WinSendMsg(hwndProfileList, LM_QUERYITEMTEXT, MPFROM2SHORT(currentProfileIndex, profileNameLen + 1) ,(MPARAM)profileName);
                nsAutoString newProfile; newProfile.AssignWithConversion(profileName);
                rv = profileService->SetCurrentProfile(newProfile.get());
            }
	        WinDismissDlg(hDlg, DID_OK);
        }
		else if (SHORT1FROMMP(wParam) == DID_CANCEL)
		{
	        WinDismissDlg(hDlg, SHORT1FROMMP(wParam));
		}
        return (MRESULT)TRUE;
	}

    return WinDefDlgProc(hDlg, message, wParam, lParam);
}

















nativeWindow WebBrowserChromeUI::CreateNativeWindow(nsIWebBrowserChrome* chrome)
{
  
  HWND hwndDialog;
  PRUint32 chromeFlags;

  chrome->GetChromeFlags(&chromeFlags);
  if ((chromeFlags & nsIWebBrowserChrome::CHROME_ALL) == nsIWebBrowserChrome::CHROME_ALL)
    hwndDialog = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, BrowserDlgProc, ghInstanceResources, IDD_BROWSER, NULL);
  else
    hwndDialog = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, BrowserDlgProc, ghInstanceResources, IDD_BROWSER_NC, NULL);
  if (!hwndDialog)
    return NULL;

  
  if (chromeFlags & nsIWebBrowserChrome::CHROME_MENUBAR) {
    HWND hmenuDlg = WinLoadMenu(hwndDialog, 0, IDC_OS2EMBED);
    WinSendMsg(hwndDialog, WM_UPDATEFRAME, MPFROMLONG(FCF_MENU), 0);
  } else
    WinSendMsg(hwndDialog, WM_UPDATEFRAME, 0, 0);

  
  HWND hwndAddress = WinWindowFromID(hwndDialog, IDC_ADDRESS);
  if (hwndAddress) {
    for (int i = 0; i < sizeof(gDefaultURLs) / sizeof(gDefaultURLs[0]); i++)
    {
      WinSendMsg(hwndAddress, LM_INSERTITEM, (MPARAM)LIT_SORTASCENDING, (MPARAM)gDefaultURLs[i]);
    }
  }

  
  HWND hwndBrowser = WinWindowFromID(hwndDialog, IDC_BROWSER);
  WinSetWindowULong(hwndBrowser, QWL_USER, (ULONG)chrome);  
  WinSetWindowULong(hwndBrowser, QWL_STYLE, WinQueryWindowULong(hwndBrowser, QWL_STYLE) | WS_CLIPCHILDREN);

  
  WinPostMsg(hwndDialog, WM_ACTIVATE, MPFROMSHORT(TRUE), (MPARAM)0);

  gDialogCount++;

  return (void *)hwndBrowser;
}







void WebBrowserChromeUI::Destroy(nsIWebBrowserChrome* chrome)
{
  nsCOMPtr<nsIWebBrowser> webBrowser;
  nsCOMPtr<nsIWebNavigation> webNavigation;

  chrome->GetWebBrowser(getter_AddRefs(webBrowser));
  webNavigation = do_QueryInterface(webBrowser);
  if (webNavigation)
    webNavigation->Stop(nsIWebNavigation::STOP_ALL);

  chrome->ExitModalEventLoop(NS_OK);

  HWND hwndDlg = GetBrowserDlgFromChrome(chrome);
  if (hwndDlg == NULL)
    return;

  

  
  nsCOMPtr<nsIWebBrowser> browser = nsnull;
  chrome->GetWebBrowser(getter_AddRefs(browser));
  nsCOMPtr<nsIBaseWindow> browserAsWin = do_QueryInterface(browser);
  if (browserAsWin)
    browserAsWin->Destroy();

      
  chrome->SetWebBrowser(nsnull);
  NS_RELEASE(chrome);
}





void WebBrowserChromeUI::Destroyed(nsIWebBrowserChrome* chrome)
{
    HWND hwndDlg = GetBrowserDlgFromChrome(chrome);
	if (hwndDlg == NULL)
	{
		return;
	}

    
    HWND hwndBrowser = WinWindowFromID(hwndDlg, IDC_BROWSER);
    WinSetWindowULong(hwndBrowser, QWL_USER, nsnull);
    WinDestroyWindow(hwndBrowser);
    WinDestroyWindow(hwndDlg);

    --gDialogCount;
    if (gDialogCount == 0)
    {
        if (gProfileSwitch)
        {
            gProfileSwitch = FALSE;
            OpenWebPage(gFirstURL);
        }
        else
        {
            
            WinPostQueueMsg(0, WM_QUIT, 0, 0);
        }
    }
}





void WebBrowserChromeUI::SetFocus(nsIWebBrowserChrome *chrome)
{
    HWND hwndDlg = GetBrowserDlgFromChrome(chrome);
	if (hwndDlg == NULL)
	{
		return;
	}
    
    HWND hwndBrowser = WinWindowFromID(hwndDlg, IDC_BROWSER);
    ::WinSetFocus(HWND_DESKTOP, hwndBrowser);
}






void WebBrowserChromeUI::UpdateStatusBarText(nsIWebBrowserChrome *aChrome, const PRUnichar* aStatusText)
{
    HWND hwndDlg = GetBrowserDlgFromChrome(aChrome);
    nsCString status; 
    if (aStatusText)
        status.AssignWithConversion(aStatusText);
    WinSetDlgItemText(hwndDlg, IDC_STATUS, status.get());
}







void WebBrowserChromeUI::UpdateCurrentURI(nsIWebBrowserChrome *aChrome)
{
    nsCOMPtr<nsIWebBrowser> webBrowser;
    nsCOMPtr<nsIWebNavigation> webNavigation;
    aChrome->GetWebBrowser(getter_AddRefs(webBrowser));
    webNavigation = do_QueryInterface(webBrowser);

    nsCOMPtr<nsIURI> currentURI;
    webNavigation->GetCurrentURI(getter_AddRefs(currentURI));
    if (currentURI)
    {
        nsCAutoString uriString;
        currentURI->GetAsciiSpec(uriString);
        HWND hwndDlg = GetBrowserDlgFromChrome(aChrome);
        WinSetDlgItemText(hwndDlg, IDC_ADDRESS, uriString.get());
    }
}







void WebBrowserChromeUI::UpdateBusyState(nsIWebBrowserChrome *aChrome, PRBool aBusy)
{
    HWND hwndDlg = GetBrowserDlgFromChrome(aChrome);
    HWND button;
    button = WinWindowFromID(hwndDlg, IDC_STOP);
    if (button)
        WinEnableWindow(button, aBusy);
    button = WinWindowFromID(hwndDlg, IDC_GO);
    if (button)
        WinEnableWindow(button, !aBusy);
    UpdateUI(aChrome);
}







void WebBrowserChromeUI::UpdateProgress(nsIWebBrowserChrome *aChrome, PRInt32 aCurrent, PRInt32 aMax)
{
    HWND hwndDlg = GetBrowserDlgFromChrome(aChrome);
    HWND hwndProgress = WinWindowFromID(hwndDlg, IDC_PROGRESS);
    if (aCurrent < 0)
    {
        aCurrent = 0;
    }
    if (aCurrent > aMax)
    {
        aMax = aCurrent + 20; 
    }
    if (hwndProgress)
    {
        ULONG value = (double)aCurrent/(double)aMax*(double)100;
        WinSendMsg(hwndProgress, SLM_SETSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE), (MPARAM)(value == 100? value-1: value));
    }
}






void WebBrowserChromeUI::ShowContextMenu(nsIWebBrowserChrome *aChrome, PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    
}






void WebBrowserChromeUI::ShowTooltip(nsIWebBrowserChrome *aChrome, PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText)
{
    
}






void WebBrowserChromeUI::HideTooltip(nsIWebBrowserChrome *aChrome)
{
    
}

void WebBrowserChromeUI::ShowWindow(nsIWebBrowserChrome *aChrome, PRBool aShow)
{
  HWND win = GetBrowserDlgFromChrome(aChrome);
  ::WinShowWindow(win, aShow);
}

void WebBrowserChromeUI::SizeTo(nsIWebBrowserChrome *aChrome, PRInt32 aWidth, PRInt32 aHeight)
{
  HWND hchrome = GetBrowserDlgFromChrome(aChrome);
  HWND hbrowser = GetBrowserFromChrome(aChrome);
  RECTL chromeRect, browserRect;

  ::WinQueryWindowRect(hchrome,  &chromeRect);
  ::WinQueryWindowRect(hbrowser, &browserRect);

  PRInt32 decoration_x = (browserRect.xLeft - chromeRect.xLeft) + 
                         (chromeRect.xRight - browserRect.xRight);
  PRInt32 decoration_y = (chromeRect.yTop - browserRect.yTop) + 
                         (browserRect.yBottom - chromeRect.yBottom);

  ::WinSetWindowPos(hchrome, HWND_TOP, WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)/2 - (aWidth+decoration_x)/2,
                    WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)/2 - (aHeight+decoration_y)/2, 
                    aWidth+decoration_x, aHeight+decoration_y, SWP_SIZE | SWP_MOVE | SWP_SHOW);
}






void WebBrowserChromeUI::GetResourceStringById(PRInt32 aID, char ** aReturn)
{
    char resBuf[MAX_LOADSTRING];
    int retval = WinLoadString((HAB)0, ghInstanceResources, aID, sizeof(resBuf), (PSZ)resBuf );
    if (retval != 0)
    {
        int resLen = strlen(resBuf);
        *aReturn = (char *)calloc(resLen+1, sizeof(char *));
        if (!*aReturn) return;
            PL_strncpy(*aReturn, (char *) resBuf, resLen);
    }
    return;
}



nsresult CreateBrowserWindow(PRUint32 aChromeFlags,
                             nsIWebBrowserChrome *aParent,
                             nsIWebBrowserChrome **aNewWindow)
{
  WebBrowserChrome * chrome = new WebBrowserChrome();
  if (!chrome)
    return NS_ERROR_FAILURE;

  
  
  CallQueryInterface(NS_STATIC_CAST(nsIWebBrowserChrome*, chrome), aNewWindow);
  
  
  NS_ADDREF(*aNewWindow);

  chrome->SetChromeFlags(aChromeFlags);
  chrome->SetParent(aParent);

  
  nsCOMPtr<nsIWebBrowser> newBrowser;
  chrome->CreateBrowser(-1, -1, -1, -1, getter_AddRefs(newBrowser));
  if (!newBrowser)
    return NS_ERROR_FAILURE;

  
  ResizeEmbedding(NS_STATIC_CAST(nsIWebBrowserChrome*, chrome));

  
  nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1"));
  if (observerService)
    observerService->AddObserver(NS_STATIC_CAST(nsIObserver *, chrome),
                                 "profile-change-teardown", PR_TRUE);

  
  
  if (!(aChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
    WebBrowserChromeUI::ShowWindow(*aNewWindow, PR_TRUE);

  return NS_OK;
}

void EnableChromeWindow(nsIWebBrowserChrome *aWindow,
                        PRBool aEnabled)
{
  HWND hwnd = GetBrowserDlgFromChrome(aWindow);
  ::WinEnableWindow(hwnd, aEnabled ? TRUE : FALSE);
}

PRUint32 RunEventLoop(PRBool &aRunCondition)
{
  QMSG msg;
  HEV hFakeEvent = 0; 
  ::DosCreateEventSem(NULL, &hFakeEvent, 0L, FALSE);

  while (aRunCondition ) {
    
    while (::WinPeekMsg((HAB)0, &msg, NULL, 0, 0, PM_NOREMOVE)) {
      if (!::WinGetMsg((HAB)0, &msg, NULL, 0, 0)) {
        
        aRunCondition = PR_FALSE;
        break;
      }

      PRBool wasHandled = PR_FALSE;
      ::NS_HandleEmbeddingEvent(msg, wasHandled);
      if (wasHandled)
        continue;

      ::WinDispatchMsg((HAB)0, &msg);
    }

    
    ::DosWaitEventSem(hFakeEvent, 100);
  }
  ::DosCloseEventSem(hFakeEvent);
  return LONGFROMMP(msg.mp1);
}
