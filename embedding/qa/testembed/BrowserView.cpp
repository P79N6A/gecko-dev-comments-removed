





























































#include "stdafx.h"
#include "TestEmbed.h"
#include "BrowserView.h"
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "Dialogs.h"
#include "ProfileMgr.h"
#include "ProfilesDlg.h"
#include "UrlDialog.h"
#include "Tests.h"

#include "QaUtils.h"


#include "PrintProgressDialog.h"



#include "nsIWidget.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT WM_FINDMSG = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CBrowserView, CWnd)
	
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_CBN_SELENDOK(ID_URL_BAR, OnUrlSelectedInUrlBar)
	ON_COMMAND(IDOK, OnNewUrlEnteredInUrlBar)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_VIEW_SOURCE, OnViewSource)
	ON_COMMAND(ID_VIEW_INFO, OnViewInfo)
	ON_COMMAND(ID_NAV_BACK, OnNavBack)
	ON_COMMAND(ID_NAV_FORWARD, OnNavForward)
	ON_COMMAND(ID_NAV_HOME, OnNavHome)
	ON_COMMAND(ID_NAV_RELOAD, OnNavReload)
	ON_COMMAND(ID_NAV_STOP, OnNavStop)
	ON_COMMAND(ID_EDIT_CUT, OnCut)
	ON_COMMAND(ID_EDIT_COPY, OnCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnPaste)
    ON_COMMAND(ID_EDIT_UNDO, OnUndoUrlBarEditOp)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAll)
	ON_COMMAND(ID_EDIT_SELECT_NONE, OnSelectNone)
	ON_COMMAND(ID_COPY_LINK_LOCATION, OnCopyLinkLocation)
	ON_COMMAND(ID_OPEN_LINK_IN_NEW_WINDOW, OnOpenLinkInNewWindow)
	ON_COMMAND(ID_VIEW_IMAGE, OnViewImageInNewWindow)
	ON_COMMAND(ID_SAVE_LINK_AS, OnSaveLinkAs)
	ON_COMMAND(ID_SAVE_IMAGE_AS, OnSaveImageAs)
	ON_COMMAND(ID_EDIT_FIND, OnShowFindDlg)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
	ON_REGISTERED_MESSAGE(WM_FINDMSG, OnFindMsg)

	
	ON_UPDATE_COMMAND_UI(ID_NAV_BACK, OnUpdateNavBack)
	ON_UPDATE_COMMAND_UI(ID_NAV_FORWARD, OnUpdateNavForward)
	ON_UPDATE_COMMAND_UI(ID_NAV_STOP, OnUpdateNavStop)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdatePaste)

	
END_MESSAGE_MAP()


CBrowserView::CBrowserView()
{
	mWebBrowser = nsnull;
	mBaseWindow = nsnull;
	mWebNav = nsnull;

	mpBrowserImpl = nsnull;
	mpBrowserFrame = nsnull;
	mpBrowserFrameGlue = nsnull;

	mbDocumentLoading = PR_FALSE;

	m_pFindDlg = NULL;
  m_pPrintProgressDlg = NULL;

  m_bUrlBarClipOp = FALSE;
  m_bCurrentlyPrinting = FALSE;

  char *theUrl = "http://www.aol.com/";
}

CBrowserView::~CBrowserView()
{
}




int CBrowserView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CreateBrowser();

	return 0;
}

void CBrowserView::OnDestroy()
{
	DestroyBrowser();
}



HRESULT CBrowserView::CreateBrowser() 
{	   
	
	nsresult rv;
	mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
	if(NS_FAILED(rv))
		return rv;

	
	
	
	
	rv = NS_OK;
	mWebNav = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;



	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	mpBrowserImpl = new CBrowserImpl();
	if(mpBrowserImpl == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;

	
	
	
	mpBrowserImpl->Init(mpBrowserFrameGlue, mWebBrowser);
	mpBrowserImpl->AddRef();

    mWebBrowser->SetContainerWindow(NS_STATIC_CAST(nsIWebBrowserChrome*, mpBrowserImpl));

	rv = NS_OK;
    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;
    dsti->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

    
  
	
	
	
	
	rv = NS_OK;
	mBaseWindow = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;

	
	
	RECT rcLocation;
	GetClientRect(&rcLocation);
	if(IsRectEmpty(&rcLocation))
	{
		rcLocation.bottom++;
		rcLocation.top++;
	}

	rv = mBaseWindow->InitWindow(nsNativeWidget(m_hWnd), nsnull,
		0, 0, rcLocation.right - rcLocation.left, rcLocation.bottom - rcLocation.top);
	rv = mBaseWindow->Create();

    
	











	
	mBaseWindow->SetVisibility(PR_TRUE);

	qaTests = new CTests(mWebBrowser, mBaseWindow, mWebNav, mpBrowserImpl);
	
	return S_OK;
}

HRESULT CBrowserView::DestroyBrowser() 
{	   
	if(mBaseWindow)
	{
		mBaseWindow->Destroy();
        mBaseWindow = nsnull;
	}

	if(mpBrowserImpl)
	{
		mpBrowserImpl->Release();
		mpBrowserImpl = nsnull;
	}














	return NS_OK;
}

BOOL CBrowserView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), HBRUSH(COLOR_WINDOW+1), NULL);

	return TRUE;
}




void CBrowserView::OnSize( UINT nType, int cx, int cy)
{
    mBaseWindow->SetPositionAndSize(0, 0, cx, cy, PR_TRUE);    
}




void CBrowserView::SetBrowserFrame(CBrowserFrame* pBrowserFrame)
{
	mpBrowserFrame = pBrowserFrame;
}

void CBrowserView::SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue)
{
	mpBrowserFrameGlue = pBrowserFrameGlue;
}





void CBrowserView::OnNewUrlEnteredInUrlBar()
{
	
	CString strUrl;
	mpBrowserFrame->m_wndUrlBar.GetEnteredURL(strUrl);

    if(IsViewSourceUrl(strUrl))
        OpenViewSourceWindow(strUrl.GetBuffer(0));
    else
	    
	    OpenURL(strUrl.GetBuffer(0));	

	
	mpBrowserFrame->m_wndUrlBar.AddURLToList(strUrl);
}



void CBrowserView::OnUrlSelectedInUrlBar()
{
	CString strUrl;
	
	mpBrowserFrame->m_wndUrlBar.GetSelectedURL(strUrl);

    if(IsViewSourceUrl(strUrl))
        OpenViewSourceWindow(strUrl.GetBuffer(0));
    else
    	OpenURL(strUrl.GetBuffer(0));
}

BOOL CBrowserView::IsViewSourceUrl(CString& strUrl)
{
    return (strUrl.Find("view-source:", 0) != -1) ? TRUE : FALSE;
}

BOOL CBrowserView::OpenViewSourceWindow(const char* pUrl)
{
	
	
	
	PRUint32 chromeFlags =  nsIWebBrowserChrome::CHROME_WINDOW_BORDERS |
							nsIWebBrowserChrome::CHROME_TITLEBAR |
							nsIWebBrowserChrome::CHROME_WINDOW_RESIZE;
	CBrowserFrame* pFrm = CreateNewBrowserFrame(chromeFlags);
	if(!pFrm)
		return FALSE;

	
	pFrm->m_wndBrowserView.OpenURL(pUrl);

    pFrm->BringWindowToTop();

    return TRUE;
}

void CBrowserView::OnViewSource() 
{
	if(! mWebNav)
		return;

	
    nsresult rv = NS_OK;
	nsCOMPtr<nsIURI> currentURI;
	rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv) || !currentURI)
		return;

	
	nsXPIDLCString uriString;
	rv = currentURI->GetSpec(uriString);
	if(NS_FAILED(rv))
		return;

    
    nsCAutoString viewSrcUrl;
    viewSrcUrl.Append("view-source:");
    viewSrcUrl.Append(uriString);

    OpenViewSourceWindow(viewSrcUrl.get());
}

void CBrowserView::OnViewInfo() 
{
	AfxMessageBox("To Be Done...");
}

void CBrowserView::OnNavBack() 
{
	if(mWebNav)
	{
		mWebNav->GoBack();
	}
}

void CBrowserView::OnUpdateNavBack(CCmdUI* pCmdUI)
{
	PRBool canGoBack = PR_FALSE;

    if (mWebNav)
        mWebNav->GetCanGoBack(&canGoBack);

	pCmdUI->Enable(canGoBack);
}

void CBrowserView::OnNavForward() 
{
	if(mWebNav)
	{
		mWebNav->GoForward();
	}
}

void CBrowserView::OnUpdateNavForward(CCmdUI* pCmdUI)
{
	PRBool canGoFwd = PR_FALSE;

    if (mWebNav)
        mWebNav->GetCanGoForward(&canGoFwd);

	pCmdUI->Enable(canGoFwd);
}

void CBrowserView::OnNavHome() 
{
    
    CString strHomeURL;
 	CTestEmbedApp *pApp = (CTestEmbedApp *)AfxGetApp();
	if(pApp)
      pApp->GetHomePage(strHomeURL);

    if(strHomeURL.GetLength() > 0)
        OpenURL(strHomeURL);	
}

void CBrowserView::OnNavReload() 
{
	if(mWebNav)
		mWebNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
}

void CBrowserView::OnNavStop() 
{	
	if(mWebNav)
		mWebNav->Stop(nsIWebNavigation::STOP_ALL);
}

void CBrowserView::OnUpdateNavStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(mbDocumentLoading);
}

void CBrowserView::OnCut()
{
    if(m_bUrlBarClipOp)
    {
        
        mpBrowserFrame->CutUrlBarSelToClipboard();
        m_bUrlBarClipOp = FALSE;
    }
    else
    {
	    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	    if(clipCmds)
		    clipCmds->CutSelection();
    }
}

void CBrowserView::OnUpdateCut(CCmdUI* pCmdUI)
{
	PRBool canCutSelection = PR_FALSE;

	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
	if (clipCmds)
        clipCmds->CanCutSelection(&canCutSelection);

    if(!canCutSelection)
    {
        
        
        if(mpBrowserFrame->CanCutUrlBarSelection())
        {
            canCutSelection = TRUE;
            m_bUrlBarClipOp = TRUE;
        }
    }

	pCmdUI->Enable(canCutSelection);
}

void CBrowserView::OnCopy()
{
    if(m_bUrlBarClipOp)
    {
        
        mpBrowserFrame->CopyUrlBarSelToClipboard();
        m_bUrlBarClipOp = FALSE;
    }
    else
    {
        
	    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	    if(clipCmds)
		    clipCmds->CopySelection();
    }
}

void CBrowserView::OnUpdateCopy(CCmdUI* pCmdUI)
{
	PRBool canCopySelection = PR_FALSE;

	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
    if (clipCmds)
        clipCmds->CanCopySelection(&canCopySelection);

    if(!canCopySelection)
    {
        
        
        if(mpBrowserFrame->CanCopyUrlBarSelection())
        {
            canCopySelection = TRUE;
            m_bUrlBarClipOp = TRUE;
        }
    }

	pCmdUI->Enable(canCopySelection);
}

void CBrowserView::OnPaste()
{
    if(m_bUrlBarClipOp)
    {
        mpBrowserFrame->PasteFromClipboardToUrlBar();
        m_bUrlBarClipOp = FALSE;
    }
    else
    {
	    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	    if(clipCmds)
		    clipCmds->Paste();
    }    
}

void CBrowserView::OnUndoUrlBarEditOp()
{
    if(mpBrowserFrame->CanUndoUrlBarEditOp())
        mpBrowserFrame->UndoUrlBarEditOp();
}

void CBrowserView::OnUpdatePaste(CCmdUI* pCmdUI)
{
	PRBool canPaste = PR_FALSE;

	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
    if (clipCmds)
        clipCmds->CanPaste(&canPaste);

    if(!canPaste)
    {
        if(mpBrowserFrame->CanPasteToUrlBar())
        {
            canPaste = TRUE;
            m_bUrlBarClipOp = TRUE;
        }
    }

	pCmdUI->Enable(canPaste);
}

void CBrowserView::OnSelectAll()
{
	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->SelectAll();
}

void CBrowserView::OnSelectNone()
{
	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->SelectNone();
}

void CBrowserView::OnFileOpen()
{
	char *lpszFilter =
        "HTML Files Only (*.htm;*.html)|*.htm;*.html|"
        "All Files (*.*)|*.*||";

	CFileDialog cf(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					lpszFilter, this);
	if(cf.DoModal() == IDOK)
	{
		CString strFullPath = cf.GetPathName(); 
		OpenURL(strFullPath);
	}
}

void CBrowserView::GetBrowserWindowTitle(nsCString& title)
{
	nsXPIDLString idlStrTitle;
	if(mBaseWindow)
		mBaseWindow->GetTitle(getter_Copies(idlStrTitle));

	title.AssignWithConversion(idlStrTitle);

	
    title.CompressWhitespace();     
    title.StripChars("\\*|:\"><?"); 
    title.ReplaceChar('.', L'_');   
    title.ReplaceChar('/', L'-');   
}

void CBrowserView::OnFileSaveAs()
{
	nsCString fileName;

	GetBrowserWindowTitle(fileName); 

	char *lpszFilter =
        "Web Page, HTML Only (*.htm;*.html)|*.htm;*.html|"
        "Web Page, Complete (*.htm;*.html)|*.htm;*.html|" 
        "Text File (*.txt)|*.txt||";

	CFileDialog cf(FALSE, "htm", fileName.get(), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					lpszFilter, this);

	if(cf.DoModal() == IDOK)
	{
		CString strFullPath = cf.GetPathName(); 
		char *pStrFullPath = strFullPath.GetBuffer(0); 
		
		CString strDataPath; 
		char *pStrDataPath = NULL;
		if(cf.m_ofn.nFilterIndex == 2) 
		{
			
			
			

			int idxPeriod = strFullPath.ReverseFind('.');
			strDataPath = strFullPath.Mid(0, idxPeriod);
			strDataPath += "_files";

			
			
			
			
			

			pStrDataPath = strDataPath.GetBuffer(0); 
		}

        
        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));


	}
}

void CBrowserView::OpenURL(const char* pUrl)
{
    OpenURL(NS_ConvertASCIItoUTF16(pUrl).get());
}

void CBrowserView::OpenURL(const PRUnichar* pUrl)
{
    if(mWebNav)
        mWebNav->LoadURI(pUrl,                              
                         nsIWebNavigation::LOAD_FLAGS_NONE, 
                         nsnull,                            
                         nsnull,                            
                         nsnull);                           
}

CBrowserFrame* CBrowserView::CreateNewBrowserFrame(PRUint32 chromeMask, 
									PRInt32 x, PRInt32 y, 
									PRInt32 cx, PRInt32 cy,
									PRBool bShowWindow)
{
	CTestEmbedApp *pApp = (CTestEmbedApp *)AfxGetApp();
	if(!pApp)
		return NULL;

	return pApp->CreateNewBrowserFrame(chromeMask, x, y, cx, cy, bShowWindow);
}

void CBrowserView::OpenURLInNewWindow(const PRUnichar* pUrl)
{
	if(!pUrl)
		return;

	CBrowserFrame* pFrm = CreateNewBrowserFrame();
	if(!pFrm)
		return;

	

	
	
	

	pFrm->m_wndBrowserView.OpenURL(pUrl);
}

void CBrowserView::LoadHomePage()
{
	OnNavHome();
}

void CBrowserView::OnCopyLinkLocation()
{
	if(! mCtxMenuLinkUrl.Length())
		return;

	if (! OpenClipboard())
		return;

	HGLOBAL hClipData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, mCtxMenuLinkUrl.Length() + 1);
	if(! hClipData)
		return;

	char *pszClipData = (char*)::GlobalLock(hClipData);
	if(!pszClipData)
		return;

	nsFixedCString clipDataStr(pszClipData, mCtxMenuLinkUrl.Length() + 1);
	LossyCopyUTF16toASCII(mCtxMenuLinkUrl, clipDataStr);
	NS_ASSERTION(clipDataStr.get() == pszClipData, "buffer too small");

	GlobalUnlock(hClipData);

	EmptyClipboard();
	SetClipboardData(CF_TEXT, hClipData);
	CloseClipboard();
}

void CBrowserView::OnOpenLinkInNewWindow()
{
	if(mCtxMenuLinkUrl.Length())
		OpenURLInNewWindow(mCtxMenuLinkUrl.get());
}

void CBrowserView::OnViewImageInNewWindow()
{
	if(mCtxMenuImgSrc.Length())
		OpenURLInNewWindow(mCtxMenuImgSrc.get());
}

void CBrowserView::OnSaveLinkAs()
{
	if(! mCtxMenuLinkUrl.Length())
		return;

	
	
	
	
	
	nsresult rv   = NS_OK;
	nsCOMPtr<nsIURI> linkURI;
	rv = NS_NewURI(getter_AddRefs(linkURI), mCtxMenuLinkUrl);
	if (NS_FAILED(rv)) 
		return;

	
	
	nsCAutoString fileName;
	linkURI->GetPath(fileName);

	
	fileName.StripChars("\\/");

	

	char *lpszFilter =
        "HTML Files (*.htm;*.html)|*.htm;*.html|"
		"Text Files (*.txt)|*.txt|" 
	    "All Files (*.*)|*.*||";

	const char *pFileName = fileName.Length() ? fileName.get() : NULL;

	CFileDialog cf(FALSE, "htm", pFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		lpszFilter, this);
	if(cf.DoModal() == IDOK)
	{
		CString strFullPath = cf.GetPathName();

        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));


	}
}

void CBrowserView::OnSaveImageAs()
{
	if(! mCtxMenuImgSrc.Length())
		return;

	
	
	
	
	
	nsresult rv   = NS_OK;
	nsCOMPtr<nsIURI> linkURI;
	rv = NS_NewURI(getter_AddRefs(linkURI), mCtxMenuImgSrc);
	if (NS_FAILED(rv)) 
		return;

	
	
	nsCAutoString path;
	linkURI->GetPath(path);

	
	nsCAutoString fileName(path);
	fileName.StripChars("\\/");

	

	char *lpszFilter = "All Files (*.*)|*.*||";
	const char *pFileName = fileName.Length() ? fileName.get() : NULL;

	CFileDialog cf(FALSE, NULL, pFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		lpszFilter, this);
	if(cf.DoModal() == IDOK)
	{
		CString strFullPath = cf.GetPathName();

        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));


	}
}

void CBrowserView::OnShowFindDlg() 
{
	
	
	
	
	if(m_pFindDlg)
	{
		m_pFindDlg->SetFocus();
		return;
	}

	CString csSearchStr;
	PRBool bMatchCase = PR_FALSE;
	PRBool bMatchWholeWord = PR_FALSE;
	PRBool bWrapAround = PR_FALSE;
	PRBool bSearchBackwards = PR_FALSE;

	
	
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
	if(finder)
	{
		nsXPIDLString stringBuf;
		finder->GetSearchString(getter_Copies(stringBuf));
		csSearchStr = stringBuf.get();

		finder->GetMatchCase(&bMatchCase);
		finder->GetEntireWord(&bMatchWholeWord);
		finder->GetWrapFind(&bWrapAround);
		finder->GetFindBackwards(&bSearchBackwards);		
	}

	m_pFindDlg = new CFindDialog(csSearchStr, bMatchCase, bMatchWholeWord,
							bWrapAround, bSearchBackwards, this);
	m_pFindDlg->Create(TRUE, NULL, NULL, 0, this);
}












LRESULT CBrowserView::OnFindMsg(WPARAM wParam, LPARAM lParam)
{
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
	if(!finder)
		return NULL;

	
	CFindDialog* dlg = (CFindDialog *) CFindReplaceDialog::GetNotifier(lParam);
	if(!dlg) 
		return NULL;

	
	if(dlg->IsTerminating())
		return NULL;

	if(dlg->FindNext())
	{
		nsString searchString;
		searchString.AssignWithConversion(dlg->GetFindString().GetBuffer(0));
		finder->SetSearchString(searchString.get());
	
		finder->SetMatchCase(dlg->MatchCase() ? PR_TRUE : PR_FALSE);
		finder->SetEntireWord(dlg->MatchWholeWord() ? PR_TRUE : PR_FALSE);
		finder->SetWrapFind(dlg->WrapAround() ? PR_TRUE : PR_FALSE);
		finder->SetFindBackwards(dlg->SearchBackwards() ? PR_TRUE : PR_FALSE);

		PRBool didFind;
		nsresult rv = finder->FindNext(&didFind);
		
		return (NS_SUCCEEDED(rv) && didFind);
	}

    return 0;
}

void CBrowserView::OnFilePrint()
{
  nsresult rv;
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) 
  {
    prefs->SetBoolPref("print.use_native_print_dialog", PR_TRUE);
    prefs->SetBoolPref("print.show_print_progress", PR_FALSE);
  }
  else
	NS_ASSERTION(PR_FALSE, "Could not get preferences service");

  nsCOMPtr<nsIDOMWindow> domWindow;
	mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
  if(domWindow) {
    nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
	  if(print)
	  {
      CPrintProgressDialog  dlg(mWebBrowser, m_PrintSettings);

	    nsCOMPtr<nsIURI> currentURI;
	    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
      if(NS_SUCCEEDED(rv) || currentURI) 
      {
	    nsCAutoString path;
	    currentURI->GetPath(path);
        dlg.SetURI(path.get());
      }
      m_bCurrentlyPrinting = TRUE;
      dlg.DoModal();
      m_bCurrentlyPrinting = FALSE;
    }
  }
}


void CBrowserView::OnUpdateFilePrint(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_bCurrentlyPrinting);
}














void CBrowserView::UpdateBusyState(PRBool aBusy)
{
	mbDocumentLoading = aBusy;
}

void CBrowserView::SetCtxMenuLinkUrl(nsAutoString& strLinkUrl)
{
	mCtxMenuLinkUrl = strLinkUrl;
}

void CBrowserView::SetCtxMenuImageSrc(nsAutoString& strImgSrc)
{
	mCtxMenuImgSrc = strImgSrc;
}

void CBrowserView::Activate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	nsCOMPtr<nsIWebBrowserFocus> focus(do_GetInterface(mWebBrowser));
	if(!focus)
		return;
    
    switch(nState) {
        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            focus->Activate();
            break;
        case WA_INACTIVE:
            focus->Deactivate();
            break;
        default:
            break;
    }
}

BOOL CBrowserView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   
   

   if ((qaTests != NULL) && qaTests->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
      return TRUE;
   
   

   return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

