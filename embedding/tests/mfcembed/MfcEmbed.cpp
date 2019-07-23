














































#include "stdafx.h"
#include "MfcEmbed.h"
#include "nsXPCOM.h"
#include "nsXPCOMGlue.h"
#include "nsMemory.h"
#include "nsIComponentRegistrar.h"
#include "nsIFactory.h"
#include "nsServiceManagerUtils.h"
#include "BrowserFrm.h"
#include "EditorFrm.h"
#include "winEmbedFileLocProvider.h"
#include "BrowserImpl.h"
#include "nsIWindowWatcher.h"
#include "plstr.h"
#include "Preferences.h"
#include <io.h>
#include <fcntl.h>

#ifdef USE_PROFILES
#include "ProfileMgr.h"
#else
#include "nsProfileDirServiceProvider.h"
#endif

#ifdef MOZ_PROFILESHARING
#include "nsIProfileSharingSetup.h"
#endif

#ifdef _BUILD_STATIC_BIN
#include "nsStaticComponent.h"
nsresult PR_CALLBACK
app_getModuleInfo(nsStaticModuleInfo **info, PRUint32 *count);
#endif


#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "PromptService.h"
#define kComponentsLibname _T("mfcEmbedComponents.dll")
#define NS_PROMPTSERVICE_CID \
 {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);


#include "PrintingPromptService.h"
#define NS_PRINTINGPROMPTSERVICE_CID \
 {0xe042570c, 0x62de, 0x4bb6, { 0xa6, 0xe0, 0x79, 0x8e, 0x3c, 0x7, 0xb4, 0xdf}}
static NS_DEFINE_CID(kPrintingPromptServiceCID, NS_PRINTINGPROMPTSERVICE_CID);


#include "HelperAppService.h"
#define NS_HELPERAPPLAUNCHERDIALOG_CID \
    {0xf68578eb, 0x6ec2, 0x4169, {0xae, 0x19, 0x8c, 0x62, 0x43, 0xf0, 0xab, 0xe1}}
static NS_DEFINE_CID(kHelperAppLauncherDialogCID, NS_HELPERAPPLAUNCHERDIALOG_CID);

class CMfcEmbedCommandLine : public CCommandLineInfo
{
public:

    CMfcEmbedCommandLine(CMfcEmbedApp& app) : CCommandLineInfo(),
                                              mApp(app)
    {
    }

    
    
    
    
    virtual void ParseParam(LPCTSTR szParam, BOOL bFlag, BOOL bLast)
    {
        CCommandLineInfo::ParseParam(szParam, bFlag, bLast);
        if (bFlag) {
            
            while (*szParam && *szParam == '-')
                szParam++;

            
            if (mLastFlag.Length() != 0)
                HandleFlag(mLastFlag);
            
            mLastFlag = szParam;

            
            if (bLast)
                HandleFlag(mLastFlag);
            
        } else {
            if (mLastFlag.Length() != 0)
                HandleFlag(mLastFlag, szParam);
                
            mLastFlag.Cut(0, PR_UINT32_MAX);
        }
    }

    
#ifdef _UNICODE
    void HandleFlag(const nsAString& flag, const TCHAR * param = nsnull)
#else
    void HandleFlag(const nsACString& flag, const TCHAR * param = nsnull)
#endif
    {
        if (_tcscmp(flag.BeginReading(), _T("console")) == 0)
            DoConsole();
        else if (_tcscmp(flag.BeginReading(), _T("chrome")) == 0)
            DoChrome();
#ifdef NS_TRACE_MALLOC
        else if (_tcscmp(flag.BeginReading(), _T("trace-malloc")) == 0)
        {
            USES_CONVERSION;
            DoTraceMalloc(flag, T2CA(param));
        }
#endif
        
    }

    void HandleNakedParameter(const char* flag) {
        
    }

    
    void DoConsole() {
        mApp.ShowDebugConsole();
    }

    void DoChrome() {
        mApp.m_bChrome = TRUE;
    }

#ifdef NS_TRACE_MALLOC
    void DoTraceMalloc(const nsACString& flag, const char* param)
    {
        if (!param) {
            NS_WARNING("--trace-malloc needs a filename as a parameter");
            return;
        }

        
        char* argv[] = { "mfcembed", "--trace-malloc",
                         NS_CONST_CAST(char*, param) };
        
        NS_TraceMallocStartupArgs(3, argv);
    }
#endif
    
private:
    
#ifdef _UNICODE
    nsEmbedString mLastFlag;
#else
    nsEmbedCString mLastFlag;
#endif

    CMfcEmbedApp& mApp;
};


BEGIN_MESSAGE_MAP(CMfcEmbedApp, CWinApp)
    
    ON_COMMAND(ID_NEW_BROWSER, OnNewBrowser)
    ON_COMMAND(ID_NEW_EDITORWINDOW, OnNewEditor)
#ifdef USE_PROFILES
    ON_COMMAND(ID_MANAGE_PROFILES, OnManageProfiles)
#endif
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_EDIT_PREFERENCES, OnEditPreferences)
    
    
    
END_MESSAGE_MAP()

CMfcEmbedApp::CMfcEmbedApp()
{
    mRefCnt = 1; 

#ifdef USE_PROFILES
    m_ProfileMgr = NULL;
#endif

    m_strHomePage = "";

    m_iStartupPage = 0; 

    m_bChrome = FALSE;
}

CMfcEmbedApp theApp;













nsresult CMfcEmbedApp::OverrideComponents()
{
    nsCOMPtr<nsIComponentRegistrar> compReg;
    nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(compReg));
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    HMODULE overlib = ::LoadLibrary(kComponentsLibname);
    if (overlib) {
        InitPromptServiceType InitLib;
        MakeFactoryType MakeFactory;
        InitLib = reinterpret_cast<InitPromptServiceType>(::GetProcAddress(overlib, kPromptServiceInitFuncName));
        MakeFactory = reinterpret_cast<MakeFactoryType>(::GetProcAddress(overlib, kPromptServiceFactoryFuncName));

        if (InitLib && MakeFactory) {
            InitLib(overlib);

            nsCOMPtr<nsIFactory> promptFactory;
            rv = MakeFactory(getter_AddRefs(promptFactory));
            if (NS_SUCCEEDED(rv)) {
                compReg->RegisterFactory(kPromptServiceCID,
                                         "Prompt Service",
                                         "@mozilla.org/embedcomp/prompt-service;1",
                                         promptFactory);
            }
        } else
          ::FreeLibrary(overlib);
    }

    
    overlib = ::LoadLibrary(kComponentsLibname);
    if (overlib) {
        InitHelperAppDlgType InitLib;
        MakeFactoryType MakeFactory;
        InitLib = reinterpret_cast<InitHelperAppDlgType>(::GetProcAddress(overlib, kHelperAppDlgInitFuncName));
        MakeFactory = reinterpret_cast<MakeFactoryType>(::GetProcAddress(overlib, kHelperAppDlgFactoryFuncName));

        if (InitLib && MakeFactory) {
            InitLib(overlib);

            nsCOMPtr<nsIFactory> helperAppDlgFactory;
            rv = MakeFactory(getter_AddRefs(helperAppDlgFactory));
            if (NS_SUCCEEDED(rv))
                compReg->RegisterFactory(kHelperAppLauncherDialogCID,
                                         "Helper App Launcher Dialog",
                                         "@mozilla.org/helperapplauncherdialog;1",
                                         helperAppDlgFactory);
        } else
          ::FreeLibrary(overlib);
    }

    
    
    overlib = ::LoadLibrary(kComponentsLibname);
    if (overlib) {
        InitPrintingPromptServiceType InitLib;
        MakeFactoryType MakeFactory;
        InitLib = reinterpret_cast<InitPrintingPromptServiceType>(::GetProcAddress(overlib, kPrintingPromptServiceInitFuncName));
        MakeFactory = reinterpret_cast<MakeFactoryType>(::GetProcAddress(overlib, kPrintingPromptServiceFactoryFuncName));

        if (InitLib && MakeFactory) {
            InitLib(overlib);

            nsCOMPtr<nsIFactory> printingPromptFactory;
            rv = MakeFactory(getter_AddRefs(printingPromptFactory));
            if (NS_SUCCEEDED(rv))
                compReg->RegisterFactory(kPrintingPromptServiceCID,
                                         "Printing Prompt Service",
                                         "@mozilla.org/embedcomp/printingprompt-service;1",
                                         printingPromptFactory);
        } else
          ::FreeLibrary(overlib);
    }
    return rv;
}

void CMfcEmbedApp::ShowDebugConsole()
{
#ifdef _DEBUG
    

    if(! AllocConsole())
        return;

    
    int hCrtOut = _open_osfhandle(
                (long) GetStdHandle(STD_OUTPUT_HANDLE),
                _O_TEXT);
    if(hCrtOut == -1)
        return;

    FILE *hfOut = _fdopen(hCrtOut, "w");
    if(hfOut != NULL)
    {
        
        
        *stdout = *hfOut;
        setvbuf(stdout, NULL, _IONBF, 0); 
    }

    
    int hCrtErr = _open_osfhandle(
                (long) GetStdHandle(STD_ERROR_HANDLE),
                _O_TEXT);
    if(hCrtErr == -1)
        return;

    FILE *hfErr = _fdopen(hCrtErr, "w");
    if(hfErr != NULL)
    {
        
        
        *stderr = *hfErr;
        setvbuf(stderr, NULL, _IONBF, 0); 
    }
#endif
}









BOOL CMfcEmbedApp::InitInstance()
{
#ifdef _BUILD_STATIC_BIN
    
    NSGetStaticModuleInfo = app_getModuleInfo;
#endif

#ifdef XPCOM_GLUE
    if (NS_FAILED(XPCOMGlueStartup(GRE_GetXPCOMPath()))) {
        MessageBox(NULL, "Could not initialize XPCOM. Perhaps the GRE\nis not installed or could not be found?", "MFCEmbed", MB_OK | MB_ICONERROR);
        return FALSE;
    }
#endif

    CMfcEmbedCommandLine cmdLine(*this);
    ParseCommandLine(cmdLine);
    
    Enable3dControls();

    
    
    
    
    
    
    
    

    TCHAR path[_MAX_PATH+1];
    ::GetModuleFileName(0, path, _MAX_PATH);
    TCHAR* lastSlash = _tcsrchr(path, _T('\\'));
    if (!lastSlash) {
        NS_ERROR("No slash in module file name... something is wrong.");
        return FALSE;
    }
    *lastSlash = _T('\0');

    USES_CONVERSION;
    nsresult rv;
    nsCOMPtr<nsILocalFile> mreAppDir;
    rv = NS_NewNativeLocalFile(nsEmbedCString(T2A(path)), TRUE, getter_AddRefs(mreAppDir));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create mreAppDir localfile");

    
    
    

    CString strRes;
    strRes.LoadString(IDS_PROFILES_FOLDER_NAME);
    winEmbedFileLocProvider *provider = new winEmbedFileLocProvider(nsEmbedCString(strRes));
    if(!provider)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    rv = NS_InitEmbedding(mreAppDir, provider);
    if(NS_FAILED(rv))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    rv = OverrideComponents();
    if(NS_FAILED(rv))
    {
        ASSERT(FALSE);
        return FALSE;
    }


    rv = InitializeWindowCreator();
    if (NS_FAILED(rv))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    if(!InitializeProfiles())
    {
        ASSERT(FALSE);
        NS_TermEmbedding();
        return FALSE;
    }


    if(!CreateHiddenWindow())
    {
        ASSERT(FALSE);
        NS_TermEmbedding();
        return FALSE;
    }

    
    OnNewBrowser();

    return TRUE;
}

CBrowserFrame* CMfcEmbedApp::CreateNewBrowserFrame(PRUint32 chromeMask,
                                                   PRInt32 x, PRInt32 y,
                                                   PRInt32 cx, PRInt32 cy,
                                                   PRBool bShowWindow,
                                                   PRBool bIsEditor
                                                   )
{
    UINT resId = bIsEditor ? IDR_EDITOR : IDR_MAINFRAME;

    
    CRect winSize(x, y, cx, cy);

    
    if(x == -1 && y == -1 && cx == -1 && cy == -1)
        winSize = CFrameWnd::rectDefault;

    
    CString strTitle;
    strTitle.LoadString(IDR_MAINFRAME);

    
    CBrowserFrame* pFrame = bIsEditor ? ( new  CEditorFrame(chromeMask) ) :
                        ( new  CBrowserFrame(chromeMask) );
    pFrame->SetEditable(bIsEditor);

    if (!pFrame->Create(NULL, strTitle, WS_OVERLAPPEDWINDOW, 
                    winSize, NULL, MAKEINTRESOURCE(resId), 0L, NULL))
    {
        return NULL;
    }

    
    pFrame->LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));

    
    if(bShowWindow)
    {
        pFrame->ShowWindow(SW_SHOW);
        pFrame->UpdateWindow();
    }

    
    m_FrameWndLst.AddHead(pFrame);

    return pFrame;
}

void CMfcEmbedApp::OnNewBrowser()
{
    CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame();

    
    if(pBrowserFrame && (GetStartupPageMode() == 1))
        pBrowserFrame->m_wndBrowserView.LoadHomePage();
}

void CMfcEmbedApp::OnNewEditor() 
{
    CEditorFrame *pEditorFrame = (CEditorFrame *)CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, 
                                    -1, -1, -1, -1,
                                    PR_TRUE,PR_TRUE);
    if (pEditorFrame)
    {
        pEditorFrame->InitEditor();
        pEditorFrame->m_wndBrowserView.OpenURL("about:blank");
    }
}










void CMfcEmbedApp::RemoveFrameFromList(CBrowserFrame* pFrm, BOOL bCloseAppOnLastFrame)
{
    POSITION pos = m_FrameWndLst.Find(pFrm);
    m_FrameWndLst.RemoveAt(pos);

    
    
    
    
    
    
    
    if(m_FrameWndLst.GetCount() == 0 && bCloseAppOnLastFrame)
        m_pMainWnd->PostMessage(WM_QUIT);
}

int CMfcEmbedApp::ExitInstance()
{
    
    
    

    CBrowserFrame* pBrowserFrame = NULL;

    POSITION pos = m_FrameWndLst.GetHeadPosition();
    while( pos != NULL )
    {
        pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
        if(pBrowserFrame)
        {
            pBrowserFrame->ShowWindow(false);
            pBrowserFrame->DestroyWindow();
        }
    }
    m_FrameWndLst.RemoveAll();

    if (m_pMainWnd)
        m_pMainWnd->DestroyWindow();

#ifdef USE_PROFILES
    delete m_ProfileMgr;
#else
    if (m_ProfileDirServiceProvider)
    {
        m_ProfileDirServiceProvider->Shutdown();
        NS_RELEASE(m_ProfileDirServiceProvider);
    }
#endif

    NS_TermEmbedding();

#ifdef XPCOM_GLUE
    XPCOMGlueShutdown();
#endif

    return 1;
}

BOOL CMfcEmbedApp::OnIdle(LONG lCount)
{
    CWinApp::OnIdle(lCount);

    return FALSE;
}

void CMfcEmbedApp::OnManageProfiles()
{
#ifdef USE_PROFILES
    m_ProfileMgr->DoManageProfilesDialog(PR_FALSE);
#endif
}

void CMfcEmbedApp::OnEditPreferences()
{
    CPreferences prefs(_T("Preferences"));
    
    prefs.m_startupPage.m_iStartupPage = m_iStartupPage;
    prefs.m_startupPage.m_strHomePage = m_strHomePage;   

    if(prefs.DoModal() == IDOK)
    {
        
        m_iStartupPage = prefs.m_startupPage.m_iStartupPage;
        m_strHomePage = prefs.m_startupPage.m_strHomePage;

        
        nsresult rv;
        nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv)) 
        {
            USES_CONVERSION;
            prefs->SetIntPref("browser.startup.page", m_iStartupPage);
            rv = prefs->SetCharPref("browser.startup.homepage", T2CA(m_strHomePage));
            if (NS_SUCCEEDED(rv))
                rv = prefs->SavePrefFile(nsnull);
        }
        else
            NS_ASSERTION(PR_FALSE, "Could not get preferences service");
    }
}

BOOL CMfcEmbedApp::InitializeProfiles()
{

#ifdef MOZ_PROFILESHARING
    
    nsCOMPtr<nsIProfileSharingSetup> sharingSetup =
        do_GetService("@mozilla.org/embedcomp/profile-sharing-setup;1");
    if (sharingSetup)
    {
        USES_CONVERSION;
        CString strRes;
        strRes.LoadString(IDS_PROFILES_NONSHARED_NAME);
        nsEmbedString nonSharedName(T2W(strRes));
        sharingSetup->EnableSharing(nonSharedName);
    }
#endif

    nsCOMPtr<nsIObserverService> observerService = 
             do_GetService("@mozilla.org/observer-service;1");
    if (!observerService)
        return FALSE;

    
    
    observerService->AddObserver(this, "profile-after-change", PR_TRUE);

#ifdef USE_PROFILES
    m_ProfileMgr = new CProfileMgr;
    if (!m_ProfileMgr)
        return FALSE;

    observerService->AddObserver(this, "profile-approve-change", PR_TRUE);
    observerService->AddObserver(this, "profile-change-teardown", PR_TRUE);

    m_ProfileMgr->StartUp();
#else
    nsresult rv;
    nsCOMPtr<nsIFile> appDataDir;
    NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_DIR,
                                getter_AddRefs(appDataDir));
    if (!appDataDir)
        return FALSE;
    nsCOMPtr<nsProfileDirServiceProvider> profProvider;
    NS_NewProfileDirServiceProvider(PR_TRUE, getter_AddRefs(profProvider));
    if (!profProvider)
        return FALSE;
    profProvider->Register();    
    nsCOMPtr<nsILocalFile> localAppDataDir(do_QueryInterface(appDataDir));
    rv = profProvider->SetProfileDir(localAppDataDir);
    if (NS_FAILED(rv))
        return FALSE;
    NS_ADDREF(m_ProfileDirServiceProvider = profProvider);
#endif

    return TRUE;
}






BOOL CMfcEmbedApp::CreateHiddenWindow()
{
    CFrameWnd *hiddenWnd = new CFrameWnd;
    if(!hiddenWnd)
        return FALSE;

    RECT bounds = { -10010, -10010, -10000, -10000 };
    hiddenWnd->Create(NULL, _T("main"), WS_DISABLED, bounds, NULL, NULL, 0, NULL);
    m_pMainWnd = hiddenWnd;

    return TRUE;
}

nsresult CMfcEmbedApp::InitializePrefs()
{
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {      

        
        
        
        
        PRBool inited;
        rv = prefs->GetBoolPref("mfcbrowser.prefs_inited", &inited);
        if (NS_FAILED(rv) || !inited)
        {
            USES_CONVERSION;
            m_iStartupPage = 1;
            m_strHomePage = "http://www.mozilla.org/projects/embedding";

            prefs->SetIntPref("browser.startup.page", m_iStartupPage);
            prefs->SetCharPref("browser.startup.homepage", T2CA(m_strHomePage));
            prefs->SetIntPref("font.size.variable.x-western", 16);
            prefs->SetIntPref("font.size.fixed.x-western", 13);
            rv = prefs->SetBoolPref("mfcbrowser.prefs_inited", PR_TRUE);
            if (NS_SUCCEEDED(rv))
                rv = prefs->SavePrefFile(nsnull);
        }
        else
        {
            

            prefs->GetIntPref("browser.startup.page", &m_iStartupPage);

            char* str = nsnull;
            prefs->GetCharPref("browser.startup.homepage", &str);
            if (str)
            {
                USES_CONVERSION;
                m_strHomePage = A2CT(str);
            }
            else
            {
                m_strHomePage.Empty();
            }
            nsMemory::Free(str);
        }       
    }
    else
        NS_ASSERTION(PR_FALSE, "Could not get preferences service");
        
    return rv;
}








nsresult CMfcEmbedApp::InitializeWindowCreator()
{
  
  nsCOMPtr<nsIWindowCreator> windowCreator(NS_STATIC_CAST(nsIWindowCreator *, this));
  if (windowCreator) {
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (wwatch) {
      wwatch->SetWindowCreator(windowCreator);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}





NS_IMPL_ISUPPORTS3(CMfcEmbedApp, nsIObserver, nsIWindowCreator, nsISupportsWeakReference)







NS_IMETHODIMP CMfcEmbedApp::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;
    
    if (strcmp(aTopic, "profile-approve-change") == 0)
    {
        
        int result = MessageBox(NULL,
            _T("Do you want to close all windows in order to switch the profile?"),
            _T("Confirm"), MB_YESNO | MB_ICONQUESTION);
        if (result != IDYES)
        {
            nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
            NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
            status->VetoChange();
        }
    }
    else if (strcmp(aTopic, "profile-change-teardown") == 0)
    {
        
        
        
        POSITION pos = m_FrameWndLst.GetHeadPosition();
        while( pos != NULL )
        {
            CBrowserFrame *pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
            if(pBrowserFrame)
            {
                pBrowserFrame->ShowWindow(false);

                
                
                RemoveFrameFromList(pBrowserFrame, FALSE);

                pBrowserFrame->DestroyWindow();
            }
        }
    }
    else if (strcmp(aTopic, "profile-after-change") == 0)
    {
        InitializePrefs(); 
        
        
        
        if (!wcscmp(someData, L"switch"))      
            OnNewBrowser();
    }
    return rv;
}




NS_IMETHODIMP CMfcEmbedApp::CreateChromeWindow(nsIWebBrowserChrome *parent,
                                               PRUint32 chromeFlags,
                                               nsIWebBrowserChrome **_retval)
{
  
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0;

  CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame(chromeFlags);
  if(pBrowserFrame) {
    *_retval = NS_STATIC_CAST(nsIWebBrowserChrome *, pBrowserFrame->GetBrowserImpl());
    NS_ADDREF(*_retval);
  }
  return NS_OK;
}



class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    

protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


void CMfcEmbedApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}
