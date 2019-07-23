






















































#include "stdafx.h"
#include "BrowserImpl.h"
#include "TestEmbed.h"
#include "BrowserFrm.h"
#include "winEmbedFileLocProvider.h"
#include "ProfileMgr.h"
#include "BrowserView.h"
#include "nsIWindowWatcher.h"
#include "nsIComponentRegistrar.h"
#include "plstr.h"
#include "Preferences.h"
#include <io.h>
#include <fcntl.h>

#include "QAUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "PromptService.h"
#define kComponentsLibname "testEmbedComponents.dll"
#define NS_PROMPTSERVICE_CID \
 {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);

BEGIN_MESSAGE_MAP(CTestEmbedApp, CWinApp)
	
	ON_COMMAND(ID_NEW_BROWSER, OnNewBrowser)
	ON_COMMAND(ID_MANAGE_PROFILES, OnManageProfiles)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_EDIT_PREFERENCES, OnEditPreferences)
	
	
	
END_MESSAGE_MAP()

CTestEmbedApp::CTestEmbedApp() :
    m_ProfileMgr(NULL)
{
    mRefCnt = 1; 

    m_strHomePage = "";

    m_iStartupPage = 0; 

}

CTestEmbedApp theApp;

BOOL CTestEmbedApp::IsCmdLineSwitch(const char *pSwitch, BOOL bRemove)
{
    
    
    char *pFound = PL_strcasestr(m_lpCmdLine, pSwitch);
    if(pFound == NULL ||
        
        
        
        ( (pFound != m_lpCmdLine) &&
          *(pFound-1) != ' ' ) ) 
    {
        return(FALSE);
    }

    if (bRemove) 
    {
        
        char *pTravEnd = pFound + strlen(pSwitch);
        char *pTraverse = pFound;

        *pTraverse = *pTravEnd;
        while(*pTraverse != '\0')   
        {
            pTraverse++;
            pTravEnd++;
            *pTraverse = *pTravEnd;
        }
    }

    return(TRUE);
}

void CTestEmbedApp::ParseCmdLine()
{
    
    if(IsCmdLineSwitch("-console"))
        ShowDebugConsole();
}













nsresult CTestEmbedApp::OverrideComponents()
{
    nsresult rv = NS_OK;

    
    
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
                nsCOMPtr<nsIComponentRegistrar> registrar;
                NS_GetComponentRegistrar(getter_AddRefs(registrar));
                if (registrar)
                    registrar->RegisterFactory(kPromptServiceCID,
                                               "Prompt Service",
                                               "@mozilla.org/embedcomp/prompt-service;1",
                                               promptFactory);
            }
        } else
          ::FreeLibrary(overlib);
    }

    return rv;
}

void CTestEmbedApp::ShowDebugConsole()
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








BOOL CTestEmbedApp::InitInstance()
{
	QAOutput("****************************************************\r\n");

    ParseCmdLine();

	Enable3dControls();

	
	
	
	
	
	
	
	

	char curDir[_MAX_PATH+1];
	::GetCurrentDirectory(_MAX_PATH, curDir);
	nsresult rv;
	nsCOMPtr<nsILocalFile> mreAppDir;
	rv = NS_NewNativeLocalFile(nsDependentCString(curDir), TRUE, getter_AddRefs(mreAppDir));
	NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create mreAppDir localfile");

	
	
	

    winEmbedFileLocProvider *provider = new winEmbedFileLocProvider("TestEmbed");
    if(!provider)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    rv = NS_InitEmbedding(mreAppDir, provider);
    if(NS_FAILED(rv))
    {
		QAOutput("TestEmbed didn't start up.");
        ASSERT(FALSE);
        return FALSE;
    }
	else
		QAOutput("TestEmbed started up.");

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
        rv = NS_TermEmbedding();
		RvTestResult(rv, "TestEmbed shutdown");
		return FALSE;
	}

	rv = InitializePrefs();
	if (NS_FAILED(rv))
	{
        ASSERT(FALSE);
        NS_TermEmbedding();
		return FALSE;
	}

    if(!CreateHiddenWindow())
	{
        ASSERT(FALSE);
        rv = NS_TermEmbedding();
		RvTestResult(rv, "TestEmbed shutdown");
		return FALSE;
	}


	
	OnNewBrowser();

	return TRUE;
}

CBrowserFrame* CTestEmbedApp::CreateNewBrowserFrame(PRUint32 chromeMask,
												   PRInt32 x, PRInt32 y,
												   PRInt32 cx, PRInt32 cy,
												   PRBool bShowWindow)
{
	
	CRect winSize(x, y, cx, cy);

	
	if(x == -1 && y == -1 && cx == -1 && cy == -1)
		winSize = CFrameWnd::rectDefault;

	
	CString strTitle;
	strTitle.LoadString(IDR_MAINFRAME);

	
	CBrowserFrame* pFrame = new CBrowserFrame(chromeMask);
	if (!pFrame->Create(NULL, strTitle, WS_OVERLAPPEDWINDOW, 
					winSize, NULL, MAKEINTRESOURCE(IDR_MAINFRAME), 0L, NULL))
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

void CTestEmbedApp::OnNewBrowser()
{
	CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame();

	
	if(pBrowserFrame && (GetStartupPageMode() == 1))
		pBrowserFrame->m_wndBrowserView.LoadHomePage();
}










void CTestEmbedApp::RemoveFrameFromList(CBrowserFrame* pFrm, BOOL bCloseAppOnLastFrame)
{
	POSITION pos = m_FrameWndLst.Find(pFrm);
	m_FrameWndLst.RemoveAt(pos);

	
	
	
	
	
	
	
	if(m_FrameWndLst.GetCount() == 0 && bCloseAppOnLastFrame)
		m_pMainWnd->PostMessage(WM_QUIT);
}

int CTestEmbedApp::ExitInstance()
{
	
	
	

	CBrowserFrame* pBrowserFrame = NULL;
	nsresult rv;

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

    delete m_ProfileMgr;

	rv = NS_TermEmbedding();
	if (NS_FAILED(rv))
		QAOutput("TestEmbed didn't shut down.");
	else
		QAOutput("TestEmbed shut down.");

	return 1;
}

BOOL CTestEmbedApp::OnIdle(LONG lCount)
{
	CWinApp::OnIdle(lCount);

	return FALSE;
}

void CTestEmbedApp::OnManageProfiles()
{
    m_ProfileMgr->DoManageProfilesDialog(PR_FALSE);
}

void CTestEmbedApp::OnEditPreferences()
{
    CPreferences prefs(_T("Preferences"));
    
    prefs.m_startupPage.m_iStartupPage = m_iStartupPage;
    prefs.m_startupPage.m_strHomePage = m_strHomePage;   

    if(prefs.DoModal() == IDOK)
    {
        
        m_iStartupPage = prefs.m_startupPage.m_iStartupPage;
        m_strHomePage = prefs.m_startupPage.m_strHomePage;

        
        nsresult rv;

		nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID,&rv));

        if (NS_SUCCEEDED(rv)) 
        {	
            prefs->SetIntPref("browser.startup.page", m_iStartupPage);
            rv = prefs->SetCharPref("browser.startup.homepage", m_strHomePage);
            if (NS_SUCCEEDED(rv))
                rv = prefs->SavePrefFile(nsnull);
        }
        else
		    NS_ASSERTION(PR_FALSE, "Could not get preferences service");
    }
}

BOOL CTestEmbedApp::InitializeProfiles()
{
    m_ProfileMgr = new CProfileMgr;
    if (!m_ProfileMgr)
        return FALSE;

	nsresult rv;

    nsCOMPtr<nsIObserverService>observerService(do_GetService("@mozilla.org/observer-service;1",&rv));
	if (NS_SUCCEEDED(rv)) 
	{	  
		observerService->AddObserver(this, "profile-approve-change", PR_TRUE);
		observerService->AddObserver(this, "profile-change-teardown", PR_TRUE);
		observerService->AddObserver(this, "profile-after-change", PR_TRUE);
	}

    m_ProfileMgr->StartUp();

	return TRUE;
}






BOOL CTestEmbedApp::CreateHiddenWindow()
{
	CFrameWnd *hiddenWnd = new CFrameWnd;
	if(!hiddenWnd)
		return FALSE;

    RECT bounds = { -10010, -10010, -10000, -10000 };
    hiddenWnd->Create(NULL, "main", WS_DISABLED, bounds, NULL, NULL, 0, NULL);
    m_pMainWnd = hiddenWnd;

	return TRUE;
}

nsresult CTestEmbedApp::InitializePrefs()
{
   nsresult rv;

   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID,&rv));

   if (NS_SUCCEEDED(rv)) {	  

		
		
		
		PRBool inited;
		rv = prefs->GetBoolPref("mfcbrowser.prefs_inited", &inited);
		if (NS_FAILED(rv) || !inited)
		{
            m_iStartupPage = 1;
            m_strHomePage = "http://www.mozilla.org/projects/embedding";

            prefs->SetIntPref("browser.startup.page", m_iStartupPage);
            prefs->SetCharPref("browser.startup.homepage", m_strHomePage);
            prefs->SetIntPref("font.size.variable.x-western", 16);
            prefs->SetIntPref("font.size.fixed.x-western", 13);
            rv = prefs->SetBoolPref("mfcbrowser.prefs_inited", PR_TRUE);
            if (NS_SUCCEEDED(rv))
                rv = prefs->SavePrefFile(nsnull);
        }
        else
        {
            
            prefs->GetIntPref("browser.startup.page", &m_iStartupPage);

            CString strBuf;
            char *pBuf = strBuf.GetBuffer(_MAX_PATH);
            prefs->CopyCharPref("browser.startup.homepage", &pBuf);
            strBuf.ReleaseBuffer(-1);
            if(pBuf)
                m_strHomePage = pBuf;
        }       
	}
	else
		NS_ASSERTION(PR_FALSE, "Could not get preferences service");
		
    return rv;
}








nsresult CTestEmbedApp::InitializeWindowCreator()
{
  
  nsCOMPtr<nsIWindowCreator> windowCreator(static_cast<nsIWindowCreator *>(this));
  if (windowCreator) {
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (wwatch) {
      wwatch->SetWindowCreator(windowCreator);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}





NS_IMPL_THREADSAFE_ISUPPORTS3(CTestEmbedApp, nsIObserver, nsIWindowCreator, nsISupportsWeakReference)







NS_IMETHODIMP CTestEmbedApp::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;
    
    if (nsCRT::strcmp(aTopic, "profile-approve-change") == 0)
    {
        
        int result = MessageBox(NULL, "Do you want to close all windows in order to switch the profile?", "Confirm", MB_YESNO | MB_ICONQUESTION);
        if (result != IDYES)
        {
            nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
            NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
            status->VetoChange();
        }
    }
    else if (nsCRT::strcmp(aTopic, "profile-change-teardown") == 0)
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
    else if (nsCRT::strcmp(aTopic, "profile-after-change") == 0)
    {
        InitializePrefs(); 
        
        
        
        if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("switch").get()))      
            OnNewBrowser();
    }
    return rv;
}




NS_IMETHODIMP CTestEmbedApp::CreateChromeWindow(nsIWebBrowserChrome *parent,
                                               PRUint32 chromeFlags,
                                               nsIWebBrowserChrome **_retval)
{
  
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0;

  CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame(chromeFlags);
  if(pBrowserFrame) {
    *_retval = static_cast<nsIWebBrowserChrome *>(pBrowserFrame->GetBrowserImpl());
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


void CTestEmbedApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
