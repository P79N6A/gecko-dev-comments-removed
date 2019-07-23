























































#include "stdafx.h"
#include "MfcEmbed.h"
#include "BrowserView.h"
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "Dialogs.h"
#include "CPageSetupPropSheet.h"


#include "nsIIOService.h"
#include "nsIWidget.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "nsXPCOM.h"

static nsresult
NewURI(nsIURI **result, const nsAString &spec)
{
  nsEmbedCString specUtf8;
  NS_UTF16ToCString(spec, NS_CSTRING_ENCODING_UTF8, specUtf8);

  nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/network/io-service;1");
  NS_ENSURE_TRUE(ios, NS_ERROR_UNEXPECTED);

  return ios->NewURI(specUtf8, nsnull, nsnull, result);
}

static void
ReplaceChar(nsAString &str, char oldChar, char newChar)
{
  

  PRUnichar *data = NS_StringCloneData(str);
  PRUnichar *datastring = data;
  for (; *data; ++data)
  {
    if ((char ) *data == oldChar)
      *data = (PRUnichar) newChar;
  }
  NS_StringSetData(str, datastring);
  nsMemory::Free(datastring);
}

static void
StripChars(nsAString &str, const char *chars)
{
  

  PRUint32 len = str.Length();

  PRUnichar *data = NS_StringCloneData(str);
  PRUnichar *datastring = data;
  PRUnichar *dataEnd = data + len;
  for (; *data; ++data)
  {
    if (strchr(chars, (char ) *data))
    {
      
      memmove(data, data + 1, (dataEnd - data) * sizeof(PRUnichar));
      --dataEnd;
    }
  }
  NS_StringSetData(str, datastring);
  nsMemory::Free(datastring);
}

static void
StripChars(nsACString &str, const char *chars)
{
  

  PRUint32 len = str.Length();

  char *data = NS_CStringCloneData(str);
  char *datastring = data;
  char *dataEnd = data + len;
  for (; *data; ++data)
  {
    if (strchr(chars, *data))
    {
      
      memmove(data, data + 1, dataEnd - data);
      --dataEnd;
    }
  }
  NS_CStringSetData(str, datastring);
  nsMemory::Free(datastring);
}

static const char* kWhitespace="\b\t\r\n ";

static void
CompressWhitespace(nsAString &str)
{
  const PRUnichar *p;
  PRInt32 i, len = (PRInt32) NS_StringGetData(str, &p);

  

  for (i=0; i<len; ++i)
  {
    if (!strchr(kWhitespace, (char) p[i]))
      break;
  }

  if (i>0)
  {
    NS_StringCutData(str, 0, i);
    len = (PRInt32) NS_StringGetData(str, &p);
  }

  

  for (i=len-1; i>=0; --i)
  {
    if (!strchr(kWhitespace, (char) p[i]))
      break;
  }

  if (++i < len)
    NS_StringCutData(str, i, len - i);
}

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

    
    ON_COMMAND(IDOK, OnNewUrlEnteredInUrlBar)
    ON_CBN_SELENDOK(ID_URL_BAR, OnUrlSelectedInUrlBar)

    
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
    ON_COMMAND(ID_OPEN_LINK_IN_NEW_WINDOW, OnOpenLinkInNewWindow)
    ON_COMMAND(ID_VIEW_IMAGE, OnViewImageInNewWindow)
    ON_COMMAND(ID_COPY_LINK_LOCATION, OnCopyLinkLocation)
    ON_COMMAND(ID_SAVE_LINK_AS, OnSaveLinkAs)
    ON_COMMAND(ID_SAVE_IMAGE_AS, OnSaveImageAs)
    ON_COMMAND(ID_EDIT_FIND, OnShowFindDlg)
    ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
    ON_COMMAND(ID_FILE_PRINTPREVIEW, OnFilePrintPreview)
    ON_COMMAND(ID_FILE_PRINTSETUP, OnFilePrintSetup)
    ON_REGISTERED_MESSAGE(WM_FINDMSG, OnFindMsg)
    ON_COMMAND(ID_VIEW_FRAME_SOURCE, OnViewFrameSource)
    ON_COMMAND(ID_OPEN_FRAME_IN_NEW_WINDOW, OnOpenFrameInNewWindow)

    
    ON_UPDATE_COMMAND_UI(ID_NAV_BACK, OnUpdateNavBack)
    ON_UPDATE_COMMAND_UI(ID_NAV_FORWARD, OnUpdateNavForward)
    ON_UPDATE_COMMAND_UI(ID_NAV_STOP, OnUpdateNavStop)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdatePaste)
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINTSETUP, OnUpdatePrintSetup)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IMAGE, OnUpdateViewImage)
    
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
    m_bUrlBarClipOp = FALSE;
    m_bCurrentlyPrinting = FALSE;

    m_SecurityState = SECURITY_STATE_INSECURE;

  m_InPrintPreview = FALSE;
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

    
    dsti->SetItemType(((CMfcEmbedApp *)AfxGetApp())->m_bChrome ?
        nsIDocShellTreeItem::typeChromeWrapper :
        nsIDocShellTreeItem::typeContentWrapper);

    
  
    
    
    
    
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

    
    
    nsWeakPtr weakling(
        do_GetWeakReference(NS_STATIC_CAST(nsIWebProgressListener*, mpBrowserImpl)));
    (void)mWebBrowser->AddWebBrowserListener(weakling, 
                                NS_GET_IID(nsIWebProgressListener));

    
    mBaseWindow->SetVisibility(PR_TRUE);

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
    cs.style |= WS_CLIPCHILDREN;
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
    return (strUrl.Find(_T("view-source:"), 0) != -1) ? TRUE : FALSE;
}

BOOL CBrowserView::OpenViewSourceWindow(const char* pUrl)
{
    nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_ASCII, str);
    return OpenViewSourceWindow(str.get());
}

BOOL CBrowserView::OpenViewSourceWindow(const PRUnichar* pUrl)
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

    
    nsEmbedCString uriString;
    rv = currentURI->GetSpec(uriString);
    if(NS_FAILED(rv))
        return;

    
    nsEmbedCString viewSrcUrl("view-source:");
    viewSrcUrl.Append(uriString);

    OpenViewSourceWindow(viewSrcUrl.get());
}

void CBrowserView::OnViewInfo() 
{
    AfxMessageBox(_T("To Be Done..."));
}

void CBrowserView::OnNavBack() 
{
    if(mWebNav)
        mWebNav->GoBack();
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
        mWebNav->GoForward();
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
    CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
    if(pApp)
        pApp->GetHomePage(strHomeURL);

    if(strHomeURL.GetLength() > 0)
        OpenURL(strHomeURL);    
}

void CBrowserView::OnNavReload() 
{
    PRUint32 loadFlags = nsIWebNavigation::LOAD_FLAGS_NONE;
    if (GetKeyState(VK_SHIFT))
        loadFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE;
    if (mWebNav)
        mWebNav->Reload(loadFlags);
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
    TCHAR *lpszFilter =
        _T("HTML Files Only (*.htm;*.html)|*.htm;*.html|")
        _T("All Files (*.*)|*.*||");

    CFileDialog cf(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    lpszFilter, this);
    if(cf.DoModal() == IDOK)
    {
        CString strFullPath = cf.GetPathName(); 
        OpenURL(strFullPath);
    }
}

void CBrowserView::GetBrowserWindowTitle(nsAString& title)
{
    PRUnichar *idlStrTitle = nsnull;
    if(mBaseWindow)
        mBaseWindow->GetTitle(&idlStrTitle);

    title = idlStrTitle;
    nsMemory::Free(idlStrTitle);
}

void CBrowserView::OnFileSaveAs()
{
    nsEmbedString fileName;

    GetBrowserWindowTitle(fileName); 

    

    USES_CONVERSION;
    CompressWhitespace(fileName);     
    StripChars(fileName, "\\*|:\"><?"); 
    ReplaceChar(fileName, '.', L'_');   
    ReplaceChar(fileName, '/', L'-');   

    TCHAR *lpszFilter =
        _T("Web Page, HTML Only (*.htm;*.html)|*.htm;*.html|")
        _T("Web Page, Complete (*.htm;*.html)|*.htm;*.html|")
        _T("Text File (*.txt)|*.txt||");

    CFileDialog cf(FALSE, _T("htm"), W2CT(fileName.get()), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    lpszFilter, this);

    if(cf.DoModal() == IDOK)
    {
        CString strFullPath = cf.GetPathName(); 
        TCHAR *pStrFullPath = strFullPath.GetBuffer(0); 
        
        BOOL bSaveAll = FALSE;        
        CString strDataPath; 
        TCHAR *pStrDataPath = NULL;
        if(cf.m_ofn.nFilterIndex == 2) 
        {
            
            
            

            bSaveAll = TRUE;

            int idxPeriod = strFullPath.ReverseFind('.');
            strDataPath = strFullPath.Mid(0, idxPeriod);
            strDataPath += "_files";

            
            
            
            
            

            pStrDataPath = strDataPath.GetBuffer(0); 
        }

        
        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
        if(persist)
        {
            nsEmbedCString fullPath(T2CA(pStrFullPath));
            nsCOMPtr<nsILocalFile> file;
            NS_NewNativeLocalFile(fullPath, TRUE, getter_AddRefs(file));

            nsCOMPtr<nsILocalFile> data;
            if (pStrDataPath)
            {
                nsEmbedCString dataPath(T2CA(pStrDataPath));
                NS_NewNativeLocalFile(dataPath, TRUE, getter_AddRefs(data));
            }

            if(bSaveAll)
                persist->SaveDocument(nsnull, file, data, nsnull, 0, 0);
            else
                persist->SaveURI(nsnull, nsnull, nsnull, nsnull, nsnull, file);
        }
    }
}

void CBrowserView::OpenURL(const char* pUrl)
{
    nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_ASCII, str);
    OpenURL(str.get());
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
    CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
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

    PRUint32 size = mCtxMenuLinkUrl.Length() + 1;

    HGLOBAL hClipData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size);
    if(! hClipData)
        return;

    char *pszClipData = (char*)::GlobalLock(hClipData);
    if(!pszClipData)
        return;

    memcpy(pszClipData, mCtxMenuLinkUrl.get(), size);

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

void CBrowserView::OnUpdateViewImage(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(mCtxMenuImgSrc.Length() > 0);
}

void CBrowserView::OnSaveLinkAs()
{
    if(! mCtxMenuLinkUrl.Length())
        return;

    
    
    
    
    
    nsresult rv   = NS_OK;
    nsCOMPtr<nsIURI> linkURI;
    rv = NewURI(getter_AddRefs(linkURI), mCtxMenuLinkUrl);
    if (NS_FAILED(rv)) 
        return;

    
    
    nsEmbedCString fileName;
    linkURI->GetPath(fileName);

    
    StripChars(fileName, "\\/");

    

    TCHAR *lpszFilter =
        _T("HTML Files (*.htm;*.html)|*.htm;*.html|")
        _T("Text Files (*.txt)|*.txt|")
        _T("All Files (*.*)|*.*||");

    CString strFilename = fileName.get();

    CFileDialog cf(FALSE, _T("htm"), strFilename, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        lpszFilter, this);
    if(cf.DoModal() == IDOK)
    {
        USES_CONVERSION;
        nsEmbedCString fullPath; fullPath.Assign(T2CA(cf.GetPathName()));
        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
        if(persist)
        {
            nsCOMPtr<nsILocalFile> file;
            NS_NewNativeLocalFile(fullPath, TRUE, getter_AddRefs(file));
            persist->SaveURI(linkURI, nsnull, nsnull, nsnull, nsnull, file);
        }
    }
}

void CBrowserView::OnSaveImageAs()
{
    if(! mCtxMenuImgSrc.Length())
        return;

    
    
    
    
    
    nsresult rv   = NS_OK;
    nsCOMPtr<nsIURI> linkURI;
    rv = NewURI(getter_AddRefs(linkURI), mCtxMenuImgSrc);
    if (NS_FAILED(rv)) 
        return;

    
    
    nsEmbedCString path;
    linkURI->GetPath(path);

    
    nsEmbedCString fileName(path);
    StripChars(fileName, "\\/");

    

    TCHAR *lpszFilter = _T("All Files (*.*)|*.*||");
    CString strFilename = fileName.get();

    CFileDialog cf(FALSE, NULL, strFilename, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        lpszFilter, this);
    if(cf.DoModal() == IDOK)
    {
        USES_CONVERSION;
        nsEmbedCString fullPath; fullPath.Assign(T2CA(cf.GetPathName()));

        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
        if(persist)
        {
            nsCOMPtr<nsILocalFile> file;
            NS_NewNativeLocalFile(fullPath, TRUE, getter_AddRefs(file));
            persist->SaveURI(linkURI, nsnull, nsnull, nsnull, nsnull, file);
        }
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
        PRUnichar *stringBuf = nsnull;
        finder->GetSearchString(&stringBuf);
        csSearchStr = stringBuf;
        nsMemory::Free(stringBuf);

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
        USES_CONVERSION;
        finder->SetSearchString(T2W(dlg->GetFindString().GetBuffer(0)));
        finder->SetMatchCase(dlg->MatchCase() ? PR_TRUE : PR_FALSE);
        finder->SetEntireWord(dlg->MatchWholeWord() ? PR_TRUE : PR_FALSE);
        finder->SetWrapFind(dlg->WrapAround() ? PR_TRUE : PR_FALSE);
        finder->SetFindBackwards(dlg->SearchBackwards() ? PR_TRUE : PR_FALSE);

        PRBool didFind;
        nsresult rv = finder->FindNext(&didFind);
        
        if(!didFind)
        {
            AfxMessageBox(IDS_SRCH_STR_NOT_FOUND);
            dlg->SetFocus();
        }

        return (NS_SUCCEEDED(rv) && didFind);
    }

    return 0;
}

void CBrowserView::OnFilePrint()
{
  nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
    if(print)
    {
    if (!m_PrintSettings) 
    {
      if (NS_FAILED(print->GetGlobalPrintSettings(getter_AddRefs(m_PrintSettings)))) {
        return;
      }
    }
    nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
      if(print)
    {
      print->Print(m_PrintSettings, nsnull);
    }

  }
}

void CBrowserView::OnFilePrintPreview()
{
  nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
    if(print)
    {
    if (!m_PrintSettings) 
    {
      if (NS_FAILED(print->GetGlobalPrintSettings(getter_AddRefs(m_PrintSettings)))) {
        return;
      }
    }
    if (!m_InPrintPreview) 
    {
      if (NS_SUCCEEDED(print->PrintPreview(m_PrintSettings, nsnull, nsnull))) 
      {
        m_InPrintPreview = TRUE;

        CMenu* menu = mpBrowserFrame->GetMenu();
        if (menu) 
        {
          menu->CheckMenuItem( ID_FILE_PRINTPREVIEW, MF_CHECKED );
        }
      }
    } 
    else 
    {
      if (NS_SUCCEEDED(print->ExitPrintPreview())) 
      {
        m_InPrintPreview = FALSE;
        CMenu* menu = mpBrowserFrame->GetMenu();
        if (menu) 
        {
          menu->CheckMenuItem( ID_FILE_PRINTPREVIEW, MF_UNCHECKED );
        }
      }
    }
  }
}

void CBrowserView::OnFilePrintSetup()
{
  nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
    if(print)
  {
    if (!m_PrintSettings) 
    {
      if (NS_FAILED(print->GetGlobalPrintSettings(getter_AddRefs(m_PrintSettings)))) {
        return;
      }
    }
    CPageSetupPropSheet propSheet(_T("Page Setup"), this);

    propSheet.SetPrintSettingsValues(m_PrintSettings);

    int status = propSheet.DoModal();
    if (status == IDOK) 
    {
      propSheet.GetPrintSettingsValues(m_PrintSettings);
    }
  }
}


void CBrowserView::OnUpdateFilePrint(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bCurrentlyPrinting);
}


void CBrowserView::OnUpdatePrintSetup(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bCurrentlyPrinting && !m_InPrintPreview);
}














void CBrowserView::UpdateBusyState(PRBool aBusy)
{
  if (mbDocumentLoading && !aBusy && m_InPrintPreview)
  {
      nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
        if(print)
        {
          PRBool isDoingPP;
          print->GetDoingPrintPreview(&isDoingPP);
          if (!isDoingPP) 
          {
              m_InPrintPreview = FALSE;
              CMenu* menu = mpBrowserFrame->GetMenu();
              if (menu) 
              {
                  menu->CheckMenuItem( ID_FILE_PRINTPREVIEW, MF_UNCHECKED );
              }
          }
      }
  }
    mbDocumentLoading = aBusy;
}

void CBrowserView::SetCtxMenuLinkUrl(nsEmbedString& strLinkUrl)
{
    mCtxMenuLinkUrl = strLinkUrl;
}

void CBrowserView::SetCtxMenuImageSrc(nsEmbedString& strImgSrc)
{
    mCtxMenuImgSrc = strImgSrc;
}

void CBrowserView::SetCurrentFrameURL(nsEmbedString& strCurrentFrameURL)
{
    mCtxMenuCurrentFrameURL = strCurrentFrameURL;
}

void CBrowserView::Activate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
    if(bMinimized)  
        return;

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

void CBrowserView::ShowSecurityInfo()
{
    HWND hParent = mpBrowserFrame->m_hWnd;

    if(m_SecurityState == SECURITY_STATE_INSECURE) {
        CString csMsg;
        csMsg.LoadString(IDS_NOSECURITY_INFO);
        ::MessageBox(hParent, csMsg, _T("MfcEmbed"), MB_OK);
        return;
    }

    ::MessageBox(hParent, _T("To Be Done.........."), _T("MfcEmbed"), MB_OK);
}




BOOL CBrowserView::ViewContentContainsFrames()
{
    nsresult rv = NS_OK;

    
    nsCOMPtr<nsIDOMDocument> domDoc;
    rv = mWebNav->GetDocument(getter_AddRefs(domDoc));
    if(NS_FAILED(rv))
       return FALSE;

    
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(domDoc);
    if (!htmlDoc)
        return FALSE;
   
    
    nsCOMPtr<nsIDOMHTMLElement> body;
    rv = htmlDoc->GetBody(getter_AddRefs(body));
    if(NS_FAILED(rv))
       return FALSE;

    
    nsCOMPtr<nsIDOMHTMLFrameSetElement> frameset = do_QueryInterface(body);

    return (frameset != nsnull);
}

void CBrowserView::OnViewFrameSource()
{
    
    
    nsEmbedString viewSrcUrl;
    viewSrcUrl.Append(L"view-source:");
    viewSrcUrl.Append(mCtxMenuCurrentFrameURL);

    OpenViewSourceWindow(viewSrcUrl.get());
}

void CBrowserView::OnOpenFrameInNewWindow()
{
    if(mCtxMenuCurrentFrameURL.Length())
        OpenURLInNewWindow(mCtxMenuCurrentFrameURL.get());
}
