




































#include "stdafx.h"
#include <string.h>
#include <string>
#include <objidl.h>

#include <shlobj.h>


#include <commdlg.h>

#include "MozillaControl.h"
#include "MozillaBrowser.h"
#include "IEHtmlDocument.h"
#include "PropertyDlg.h"
#include "PageSetupDlg.h"
#include "PromptService.h"
#include "HelperAppDlg.h"
#include "WindowCreator.h"

#include "nsNetUtil.h"
#include "nsCWebBrowser.h"
#include "nsIAtom.h"
#include "nsILocalFile.h"
#include "nsIWebBrowserPersist.h"
#include "nsIClipboardCommands.h"
#include "nsIProfile.h"
#include "nsIWidget.h"
#include "nsIWebBrowserFocus.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIComponentRegistrar.h"

#ifdef NS_PRINTING
#include "nsIPrintOptions.h"
#include "nsIWebBrowserPrint.h"
#endif

#include "nsIDOMWindow.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMNSDocument.h"

#include "nsEmbedAPI.h"
#include "nsEmbedCID.h"

#define HACK_NON_REENTRANCY
#ifdef HACK_NON_REENTRANCY
static HANDLE s_hHackedNonReentrancy = NULL;
#endif

#define NS_PROMPTSERVICE_CID \
  {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}

#define NS_HELPERAPPLAUNCHERDIALOG_CID \
    {0xf68578eb, 0x6ec2, 0x4169, {0xae, 0x19, 0x8c, 0x62, 0x43, 0xf0, 0xab, 0xe1}}

static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);
static NS_DEFINE_CID(kHelperAppLauncherDialogCID, NS_HELPERAPPLAUNCHERDIALOG_CID);

#ifdef NS_PRINTING
class PrintListener : public nsIWebProgressListener
{
    PRBool mComplete;
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER

    PrintListener();
    virtual ~PrintListener();

    void WaitForComplete();
};
#endif

class SimpleDirectoryProvider :
    public nsIDirectoryServiceProvider
{
public:
    SimpleDirectoryProvider();
    BOOL IsValid() const;


    NS_DECL_ISUPPORTS
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER

protected:
    virtual ~SimpleDirectoryProvider();

    nsCOMPtr<nsILocalFile> mApplicationRegistryDir;
    nsCOMPtr<nsILocalFile> mApplicationRegistryFile;
    nsCOMPtr<nsILocalFile> mUserProfileDir;
};




static const char kDesignModeScheme[] = "data";
static const OLECHAR kDesignModeURL[] =
    L"data:text/html,<html><body bgcolor=\"#00FF00\"><p>Mozilla Control</p></body></html>";



static const TCHAR kBrowserHelperObjectRegKey[] =
    _T("Software\\Mozilla\\ActiveX Control\\Browser Helper Objects");





GUID CGID_IWebBrowser_Moz =
    { 0xED016940L, 0xBD5B, 0x11cf, {0xBA, 0x4E, 0x00, 0xC0, 0x4F, 0xD7, 0x08, 0x16} };

GUID CGID_MSHTML_Moz =
    { 0xED016940L, 0xBD5B, 0x11cf, {0xBA, 0x4E, 0x00, 0xC0, 0x4F, 0xD7, 0x08, 0x16} };





nsTArray<CMozillaBrowser*> CMozillaBrowser::sBrowserList;




CMozillaBrowser::CMozillaBrowser()
{
    NG_TRACE_METHOD(CMozillaBrowser::CMozillaBrowser);

    
    m_bWindowOnly = TRUE;
    m_bWndLess = FALSE;

    
    mWebBrowserAsWin = nsnull;
    mValidBrowserFlag = FALSE;

    
    mWebBrowserContainer = NULL;

    
    mEditModeFlag = FALSE;

    
    mHaveDropTargetFlag = FALSE;

     
     mIERootDocument = NULL;

    
    mBrowserHelperList = NULL;
    mBrowserHelperListCount = 0;

    
    mProfileName.Assign(NS_LITERAL_STRING("MozillaControl"));

    
    Initialize();
}





CMozillaBrowser::~CMozillaBrowser()
{
    NG_TRACE_METHOD(CMozillaBrowser::~CMozillaBrowser);
    
    
    Terminate();
}






static inline BOOL _IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

STDMETHODIMP CMozillaBrowser::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IWebBrowser,
        &IID_IWebBrowser2,
        &IID_IWebBrowserApp
    };
    for (int i = 0; i < (sizeof(arr) / sizeof(arr[0])); i++)
    {
        if (_IsEqualGUID(*arr[i], riid))
            return S_OK;
    }
    return S_FALSE;
}




void CMozillaBrowser::ShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    POINT pt;
    GetCursorPos(&pt);

    
    

    CIPtr(IDocHostUIHandler) spIDocHostUIHandler = m_spClientSite;
    if (spIDocHostUIHandler)
    {
        enum IE4MenuContexts
        {
            ctxMenuDefault = 0,
            ctxMenuImage,
            ctxMenuControl,
            ctxMenuTable,
            ctxMenuDebug,
            ctxMenu1DSelect,
            ctxMenuAnchor,
            ctxMenuImgDynSrc
        };

        DWORD dwID = ctxMenuDefault;
        if (aContextFlags & nsIContextMenuListener::CONTEXT_DOCUMENT)
        {
            dwID = ctxMenuDefault;
        }
        else if (aContextFlags & nsIContextMenuListener::CONTEXT_LINK)
        {
            dwID = ctxMenuAnchor;
        }
        else if (aContextFlags & nsIContextMenuListener::CONTEXT_IMAGE)
        {
            dwID = ctxMenuImage;
        }
        else if (aContextFlags & nsIContextMenuListener::CONTEXT_TEXT)
        {
            dwID = ctxMenu1DSelect;
        }
        else
        {
            dwID = ctxMenuDefault;
        }

        HRESULT hr = spIDocHostUIHandler->ShowContextMenu(dwID, &pt, NULL, NULL);
        if (hr == S_OK)
        {
            
            return;
        }
    }

    LPTSTR pszMenuResource = NULL;
    if (aContextFlags & nsIContextMenuListener::CONTEXT_DOCUMENT)
    {
        pszMenuResource = MAKEINTRESOURCE(IDR_POPUP_DOCUMENT);
    }
    else if (aContextFlags & nsIContextMenuListener::CONTEXT_LINK)
    {
        pszMenuResource = MAKEINTRESOURCE(IDR_POPUP_LINK);
    }
    else if (aContextFlags & nsIContextMenuListener::CONTEXT_IMAGE)
    {
        pszMenuResource = MAKEINTRESOURCE(IDR_POPUP_IMAGE);
    }
    else if (aContextFlags & nsIContextMenuListener::CONTEXT_TEXT)
    {
        pszMenuResource = MAKEINTRESOURCE(IDR_POPUP_TEXT);
    }
    else
    {
        pszMenuResource = MAKEINTRESOURCE(IDR_POPUP_DOCUMENT);
    }

    if (pszMenuResource)
    {
        HMENU hMenu = LoadMenu(_Module.m_hInstResource, pszMenuResource);
        HMENU hPopupMenu = GetSubMenu(hMenu, 0);
        mContextNode = do_QueryInterface(aNode);
        UINT nCmd = TrackPopupMenu(hPopupMenu, TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
        DestroyMenu(hMenu);
        if (nCmd != 0)
        {
            SendMessage(WM_COMMAND, nCmd);
        }
        mContextNode = nsnull;
    }
}





void CMozillaBrowser::ShowURIPropertyDlg(const nsAString &aURI, const nsAString &aContentType)
{
    CPropertyDlg dlg;
    CPPageDlg linkDlg;
    dlg.AddPage(&linkDlg);

    if (!aURI.IsEmpty())
    {
        linkDlg.mType = aContentType;
        linkDlg.mURL = aURI;
    }

    dlg.DoModal();
}







int CMozillaBrowser::MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption, UINT nType)
{
    
    CIPtr(IDocHostShowUI) spIDocHostShowUI = m_spClientSite;
    if (spIDocHostShowUI)
    {
        USES_CONVERSION;
        LRESULT lResult = 0;
        HRESULT hr = spIDocHostShowUI->ShowMessage(m_hWnd,
                        T2OLE(lpszText), T2OLE(lpszCaption), nType, NULL, 0, &lResult);
        if (hr == S_OK)
        {
            return lResult;
        }
    }

    
    return CWindow::MessageBox(lpszText, lpszCaption, nType);
}





HRESULT CMozillaBrowser::SetStartupErrorMessage(UINT nStringID)
{
    TCHAR szMsg[1024];
    ::LoadString(_Module.m_hInstResource, nStringID, szMsg, sizeof(szMsg) / sizeof(szMsg[0]));
    mStartupErrorMessage = szMsg;
    return S_OK;
}




void CMozillaBrowser::NextDlgControl()
{
    HWND hwndParent = GetParent();
    if (::IsWindow(hwndParent))
    {
      ::PostMessage(hwndParent, WM_NEXTDLGCTL, 0, 0);
    }
}





void CMozillaBrowser::PrevDlgControl()
{
    HWND hwndParent = GetParent();
    if (::IsWindow(hwndParent))
    {
      ::PostMessage(hwndParent, WM_NEXTDLGCTL, 1, 0);
    }
}







LRESULT CMozillaBrowser::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnCreate);

    
    CreateBrowser();

    

    
    mBrowserReadyState = READYSTATE_COMPLETE;
    FireOnChanged(DISPID_READYSTATE);

    
    LoadBrowserHelpers();

    
    BOOL bUserMode = FALSE;
    if (SUCCEEDED(GetAmbientUserMode(bUserMode)))
    {
        if (!bUserMode)
        {
            
            nsCOMPtr<nsIIOService> ios = do_GetIOService();
            if (ios)
            {
                
                
                nsCOMPtr<nsIProtocolHandler> ph;
                nsCAutoString phScheme;
                ios->GetProtocolHandler(kDesignModeScheme, getter_AddRefs(ph));
                if (ph &&
                    NS_SUCCEEDED(ph->GetScheme(phScheme)) &&
                    phScheme.Equals(NS_LITERAL_CSTRING(kDesignModeScheme)))
                {
                    Navigate(const_cast<BSTR>(kDesignModeURL), NULL, NULL, NULL, NULL);
                }
            }
        }
        else
        {
            if (mInitialSrc.Length() > 0)
            {
                Navigate(mInitialSrc, NULL, NULL, NULL, NULL);
            }
        }
    }

    
    SetWindowLong(GWL_STYLE, GetWindowLong(GWL_STYLE) | WS_CLIPCHILDREN | WS_TABSTOP);

    return 0;
}



LRESULT CMozillaBrowser::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnDestroy);

    
    UnloadBrowserHelpers();

    
    DestroyBrowser();

    return 0;
}



LRESULT CMozillaBrowser::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnSize);

    RECT rc;
    rc.top = 0;
    rc.left = 0;
    rc.right = LOWORD(lParam);
    rc.bottom = HIWORD(lParam);
    
    AdjustWindowRectEx(&rc, GetWindowLong(GWL_STYLE), FALSE, GetWindowLong(GWL_EXSTYLE));

    rc.right -= rc.left;
    rc.bottom -= rc.top;
    rc.left = 0;
    rc.top = 0;

    
    if (mWebBrowserAsWin)
    {
        mWebBrowserAsWin->SetPosition(rc.left, rc.top);
        mWebBrowserAsWin->SetSize(rc.right - rc.left, rc.bottom - rc.top, PR_TRUE);
    }
    return 0;
}


LRESULT CMozillaBrowser::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATLTRACE(_T("CMozillaBrowser::OnSetFocus()\n"));
    nsCOMPtr<nsIWebBrowserFocus> browserAsFocus = do_QueryInterface(mWebBrowser);
    if (browserAsFocus)
    {
        browserAsFocus->Activate();
    }
    CComQIPtr<IOleControlSite> controlSite = m_spClientSite;
    if (controlSite)
    {
        controlSite->OnFocus(TRUE);
    }
    return 0;
}


LRESULT CMozillaBrowser::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATLTRACE(_T("CMozillaBrowser::OnKillFocus()\n"));
    nsCOMPtr<nsIWebBrowserFocus> browserAsFocus = do_QueryInterface(mWebBrowser);
    if (browserAsFocus)
    {
        browserAsFocus->Deactivate();
    }
    CComQIPtr<IOleControlSite> controlSite = m_spClientSite;
    if (controlSite)
    {
        controlSite->OnFocus(FALSE);
    }
    return 0;
}


LRESULT CMozillaBrowser::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return MA_ACTIVATE;
}


LRESULT CMozillaBrowser::OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return DLGC_WANTALLKEYS; 
}


HRESULT CMozillaBrowser::OnDraw(ATL_DRAWINFO& di)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnDraw);

    if (!BrowserIsValid())
    {
        RECT& rc = *(RECT*)di.prcBounds;
        DrawText(di.hdcDraw, mStartupErrorMessage.c_str(), -1, &rc, DT_TOP | DT_LEFT | DT_WORDBREAK);
    }

    return S_OK;
}


LRESULT CMozillaBrowser::OnPageSetup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnPageSetup);


#ifdef NS_PRINTING
    nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
    nsCOMPtr<nsIPrintSettings> printSettings;
    if(!print ||
        NS_FAILED(print->GetGlobalPrintSettings(getter_AddRefs(printSettings))))
    {
        return 0;
    }

    
    CPageSetupDlg dlg(printSettings);
    dlg.DoModal();
#endif

    return 0;
}


LRESULT CMozillaBrowser::OnPrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnPrint);
    if (!BrowserIsValid())
    {
        return 0;
    }
    PrintDocument(TRUE);
    return 0;
}

LRESULT CMozillaBrowser::OnSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnSaveAs);

    OPENFILENAME SaveFileName;

    char szFile[_MAX_PATH];
    char szFileTitle[256];

    
    
    

    memset(&SaveFileName, 0, sizeof(SaveFileName));
    SaveFileName.lStructSize = sizeof(SaveFileName);
    SaveFileName.hwndOwner = m_hWnd;
    SaveFileName.lpstrFilter = "Web Page, HTML Only (*.htm;*.html)\0*.htm;*.html\0Text File (*.txt)\0*.txt\0"; 
    SaveFileName.nFilterIndex = 1; 
    SaveFileName.lpstrFile = szFile; 
    SaveFileName.nMaxFile = sizeof(szFile); 
    SaveFileName.lpstrFileTitle = szFileTitle;
    SaveFileName.nMaxFileTitle = sizeof(szFileTitle); 
    SaveFileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT; 
    SaveFileName.lpstrDefExt = "htm"; 

    
    char szTmp[_MAX_FNAME] = "untitled";
    BSTR pageName = NULL;
    get_LocationName(&pageName); 
    if (pageName)
    {
        USES_CONVERSION;
        strncpy(szTmp, OLE2A(pageName), sizeof(szTmp) - 1);
        SysFreeString(pageName);
        szTmp[sizeof(szTmp) - 1] = '\0';
    }
    
    
    
    
    int j = 0;
    for (int i=0; szTmp[i]!='\0'; i++)
    {        
        switch(szTmp[i])
        {
        case '\\':
        case '*':
        case '|':
        case ':':
        case '"':
        case '>':
        case '<':
        case '?':
            break;
        case '.':
            if (szTmp[i+1] != '\0')
            {
                szFile[j] = '_';
                j++;
            }
            break;
        case '/':
            szFile[j] = '-';
            j++;
            break;
        default:
            szFile[j] = szTmp[i];
            j++;
        }
    }
    szFile[j] = '\0';

    HRESULT hr = S_OK;
    if (GetSaveFileName(&SaveFileName))
    {
        nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
        USES_CONVERSION;

        char szDataFile[_MAX_PATH];
        char szDataPath[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];

        _splitpath(szFile, drive, dir, fname, ext);
        sprintf(szDataFile, "%s_files", fname);
        _makepath(szDataPath, drive, dir, szDataFile, "");

        nsCOMPtr<nsILocalFile> file;
        NS_NewNativeLocalFile(nsDependentCString(T2A(szFile)), TRUE, getter_AddRefs(file));

        nsCOMPtr<nsILocalFile> dataPath;
        NS_NewNativeLocalFile(nsDependentCString(szDataPath), TRUE, getter_AddRefs(dataPath));

        persist->SaveDocument(nsnull, file, dataPath, nsnull, 0, 0);
    }

    return hr;
}

LRESULT CMozillaBrowser::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnProperties);
    MessageBox(_T("No Properties Yet!"), _T("Control Message"), MB_OK);
    
    return 0;
}

LRESULT CMozillaBrowser::OnCut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnCut);
    nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWebBrowser));
    if (clipboard)
    {
        clipboard->CutSelection();
    }
    return 0;
}

LRESULT CMozillaBrowser::OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnCopy);
    nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWebBrowser));
    if (clipboard)
    {
        clipboard->CopySelection();
    }
    return 0;
}

LRESULT CMozillaBrowser::OnPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnPaste);
    nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWebBrowser));
    if (clipboard)
    {
        clipboard->Paste();
    }
    return 0;
}

LRESULT CMozillaBrowser::OnSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnSelectAll);
    nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWebBrowser));
    if (clipboard)
    {
        clipboard->SelectAll();
    }
    return 0;
}


LRESULT CMozillaBrowser::OnViewSource(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnViewSource);

    if (!mWebBrowser)
    {
        
        NG_ASSERT(0);
        return 0;
    }

    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
    if (!webNav)
    {
        
        NG_ASSERT(0);
        return 0;
    }

    nsCOMPtr<nsIURI> uri;
    webNav->GetCurrentURI(getter_AddRefs(uri));
    if (!uri)
    {
        
        NG_ASSERT(0);
        return 0;
    }

    
    nsCAutoString aURI;
    uri->GetSpec(aURI);

    NS_ConvertUTF8toUTF16 strURI(aURI);
    strURI.Insert(NS_LITERAL_STRING("view-source:"), 0);

    
    CIPtr(IDispatch) spDispNew;
    VARIANT_BOOL bCancel = VARIANT_FALSE;
    Fire_NewWindow2(&spDispNew, &bCancel);

    
    if ((bCancel == VARIANT_FALSE) && spDispNew)
    {
        CIPtr(IWebBrowser2) spOther = spDispNew;;
        if (spOther)
        {
            
            CComBSTR bstrURL(strURI.get());
            CComVariant vURL(bstrURL);
            VARIANT vNull;
            vNull.vt = VT_NULL;
            spOther->Navigate2(&vURL, &vNull, &vNull, &vNull, &vNull);            
        }
    }

    return 0;
}






LRESULT CMozillaBrowser::OnDocumentBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    GoBack();
    return 0;
}


LRESULT CMozillaBrowser::OnDocumentForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    GoForward();
    return 0;
}


LRESULT CMozillaBrowser::OnDocumentPrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return OnPrint(wNotifyCode, wID, hWndCtl, bHandled);
}


LRESULT CMozillaBrowser::OnDocumentRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    Refresh();
    return 0;
}


LRESULT CMozillaBrowser::OnDocumentProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    nsCOMPtr<nsIDOMDocument> ownerDoc;
    if (mContextNode)
    {
        mContextNode->GetOwnerDocument(getter_AddRefs(ownerDoc));
    }

    
    nsAutoString uri;
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(ownerDoc);
    if (htmlDoc)
    {
        htmlDoc->GetURL(uri);
    }
    nsAutoString contentType;
    nsCOMPtr<nsIDOMNSDocument> doc = do_QueryInterface(ownerDoc);
    if (doc)
    {
        doc->GetContentType(contentType);
    }

    ShowURIPropertyDlg(uri, contentType);

    return 0;
}






LRESULT CMozillaBrowser::OnLinkOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    nsAutoString uri;

    nsCOMPtr<nsIDOMHTMLAnchorElement> anchorElement = do_QueryInterface(mContextNode);
    if (anchorElement)
    {
        anchorElement->GetHref(uri);
    }

    if (!uri.IsEmpty())
    {
        CComBSTR bstrURI(uri.get());
        CComVariant vFlags(0);
        Navigate(bstrURI, &vFlags, NULL, NULL, NULL);
    }

    return 0;
}


LRESULT CMozillaBrowser::OnLinkOpenInNewWindow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    nsAutoString uri;

    nsCOMPtr<nsIDOMHTMLAnchorElement> anchorElement = do_QueryInterface(mContextNode);
    if (anchorElement)
    {
        anchorElement->GetHref(uri);
    }

    if (!uri.IsEmpty())
    {
        CComBSTR bstrURI(uri.get());
        CComVariant vFlags(navOpenInNewWindow);
        Navigate(bstrURI, &vFlags, NULL, NULL, NULL);
    }

    return 0;
}


LRESULT CMozillaBrowser::OnLinkCopyShortcut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    nsAutoString uri;

    nsCOMPtr<nsIDOMHTMLAnchorElement> anchorElement = do_QueryInterface(mContextNode);
    if (anchorElement)
    {
        anchorElement->GetHref(uri);
    }

    if (!uri.IsEmpty() && OpenClipboard())
    {
        EmptyClipboard();

        NS_ConvertUTF16toUTF8 curi(uri);
        const char *stringText;
        PRUint32 stringLen = NS_CStringGetData(curi,
                                               &stringText);

        
        HGLOBAL hmemText = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                       stringLen + 1);
        char *pszText = (char *) GlobalLock(hmemText);
        strncpy(pszText, stringText, stringLen);
        pszText[stringLen] = '\0';
        GlobalUnlock(hmemText);
        SetClipboardData(CF_TEXT, hmemText);

        
        const UINT cfShellURL = RegisterClipboardFormat(CFSTR_SHELLURL);
        HGLOBAL hmemURL = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                      stringLen + 1);
        char *pszURL = (char *) GlobalLock(hmemURL);
        strncpy(pszText, stringText, stringLen);
        pszText[stringLen] = '\0';
        GlobalUnlock(hmemURL);
        SetClipboardData(cfShellURL, hmemURL);

        
        
        
        

        CloseClipboard();
    }
    return 0;
}


LRESULT CMozillaBrowser::OnLinkProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    nsAutoString uri;
    nsAutoString type;

    nsCOMPtr<nsIDOMHTMLAnchorElement> anchorElement = do_QueryInterface(mContextNode);
    if (anchorElement)
    {
        anchorElement->GetHref(uri);
        anchorElement->GetType(type); 
    }

    ShowURIPropertyDlg(uri, type);

    return 0;
}





HRESULT CMozillaBrowser::Initialize()
{
#ifdef HACK_NON_REENTRANCY
    
    
    
    TCHAR szHackEvent[255];
    _stprintf(szHackEvent, _T("MozCtlEvent%d"), (int) GetCurrentProcessId());
    s_hHackedNonReentrancy = OpenEvent(EVENT_ALL_ACCESS, FALSE, szHackEvent);
    if (s_hHackedNonReentrancy == NULL)
    {
        s_hHackedNonReentrancy = CreateEvent(NULL, FALSE, FALSE, szHackEvent);
#endif


    TCHAR szMozCtlPath[MAX_PATH];
    memset(szMozCtlPath, 0, sizeof(szMozCtlPath));
    GetModuleFileName(_Module.m_hInst, szMozCtlPath, sizeof(szMozCtlPath) / sizeof(szMozCtlPath[0]));

    TCHAR szTmpDrive[_MAX_DRIVE];
    TCHAR szTmpDir[_MAX_DIR];
    TCHAR szTmpFname[_MAX_FNAME];
    TCHAR szTmpExt[_MAX_EXT];
    TCHAR szBinDirPath[MAX_PATH];

    _tsplitpath(szMozCtlPath, szTmpDrive, szTmpDir, szTmpFname, szTmpExt);
    memset(szBinDirPath, 0, sizeof(szBinDirPath));
    _tmakepath(szBinDirPath, szTmpDrive, szTmpDir, NULL, NULL);
    if (_tcslen(szBinDirPath) == 0)
    {
        return E_FAIL;
    }

    
    
    

    nsCOMPtr<nsIDirectoryServiceProvider> directoryProvider;
    SimpleDirectoryProvider *pDirectoryProvider = new SimpleDirectoryProvider;
    if (pDirectoryProvider->IsValid())
        directoryProvider = do_QueryInterface(pDirectoryProvider);

    
    nsresult rv;
    nsCOMPtr<nsILocalFile> binDir;
    USES_CONVERSION;
    NS_NewNativeLocalFile(nsDependentCString(T2A(szBinDirPath)), TRUE, getter_AddRefs(binDir));
    rv = NS_InitEmbedding(binDir, directoryProvider);

    
    mPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
    {
        NG_ASSERT(0);
        NG_TRACE_ALWAYS(_T("Could not create preference object rv=%08x\n"), (int) rv);
        SetStartupErrorMessage(IDS_CANNOTCREATEPREFS);
        return E_FAIL;
    }

    
    static BOOL bRegisterComponents = FALSE;
    if (!bRegisterComponents)
    {
        
        
        nsCOMPtr<nsIFactory> promptFactory;
        rv = NS_NewPromptServiceFactory(getter_AddRefs(promptFactory));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIComponentRegistrar> registrar;
        rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
        if (NS_FAILED(rv)) return rv;

        rv = registrar->RegisterFactory(kPromptServiceCID,
            "Prompt Service",
            NS_PROMPTSERVICE_CONTRACTID,
            promptFactory);
        if (NS_FAILED(rv)) return rv;

        
        nsCOMPtr<nsIFactory> helperAppDlgFactory;
        rv = NS_NewHelperAppLauncherDlgFactory(getter_AddRefs(helperAppDlgFactory));
        if (NS_FAILED(rv)) return rv;

        rv = registrar->RegisterFactory(kHelperAppLauncherDialogCID,
            "Helper App Launcher Dialog",
            "@mozilla.org/helperapplauncherdialog;1",
            helperAppDlgFactory);
        if (NS_FAILED(rv)) return rv;

        
        CWindowCreator *creator = new CWindowCreator();
        nsCOMPtr<nsIWindowCreator> windowCreator;
        windowCreator = static_cast<nsIWindowCreator *>(creator);

        
        nsCOMPtr<nsIWindowWatcher> watcher =
            do_GetService(NS_WINDOWWATCHER_CONTRACTID);
        if (watcher)
            watcher->SetWindowCreator(windowCreator);
    }

    
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
    {
        return E_FAIL;
    }

    
    PRBool profileExists = PR_FALSE;
    rv = profileService->ProfileExists(mProfileName.get(), &profileExists);
    if (NS_FAILED(rv))
    {
        return E_FAIL;
    }
    else if (!profileExists)
    {
        rv = profileService->CreateNewProfile(mProfileName.get(), nsnull, nsnull, PR_FALSE);
        if (NS_FAILED(rv))
        {
            return E_FAIL;
        }
    }

    rv = profileService->SetCurrentProfile(mProfileName.get());
    if (NS_FAILED(rv))
    {
        return E_FAIL;
    }

#ifdef HACK_NON_REENTRANCY
    }
#endif

    return S_OK;
}


HRESULT CMozillaBrowser::Terminate()
{
#ifdef HACK_NON_REENTRANCY
    if (0)
    {
#endif

    mPrefBranch = nsnull;
    NS_TermEmbedding();

#ifdef HACK_NON_REENTRANCY
    }
#endif

    return S_OK;
}



HRESULT CMozillaBrowser::CreateBrowser() 
{    
    NG_TRACE_METHOD(CMozillaBrowser::CreateBrowser);
    
    if (mWebBrowser != nsnull)
    {
        NG_ASSERT(0);
        NG_TRACE_ALWAYS(_T("CreateBrowser() called more than once!"));
        return SetErrorInfo(E_UNEXPECTED);
    }

    RECT rcLocation;
    GetClientRect(&rcLocation);
    if (IsRectEmpty(&rcLocation))
    {
        rcLocation.bottom++;
        rcLocation.top++;
    }

    nsresult rv;

    
    mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
    {
        NG_ASSERT(0);
        NG_TRACE_ALWAYS(_T("Could not create webbrowser object rv=%08x\n"), (int) rv);
        SetStartupErrorMessage(IDS_CANNOTCREATEPREFS);
        return rv;
    }

    
    nsCOMPtr<nsIWebBrowserSetup> webBrowserAsSetup(do_QueryInterface(mWebBrowser));

    
    const PRBool kAllowPlugins = PR_TRUE;
    webBrowserAsSetup->SetProperty(nsIWebBrowserSetup::SETUP_ALLOW_PLUGINS, kAllowPlugins);

    
    const PRBool kHostChrome = PR_FALSE; 
    webBrowserAsSetup->SetProperty(nsIWebBrowserSetup::SETUP_IS_CHROME_WRAPPER, PR_FALSE);

    
    mWebBrowserAsWin = do_QueryInterface(mWebBrowser);
    rv = mWebBrowserAsWin->InitWindow(nsNativeWidget(m_hWnd), nsnull,
        0, 0, rcLocation.right - rcLocation.left, rcLocation.bottom - rcLocation.top);
    rv = mWebBrowserAsWin->Create();

    
    mWebBrowserContainer = new CWebBrowserContainer(this);
    if (mWebBrowserContainer == NULL)
    {
        NG_ASSERT(0);
        NG_TRACE_ALWAYS(_T("Could not create webbrowsercontainer - out of memory\n"));
        return NS_ERROR_OUT_OF_MEMORY;
    }
    mWebBrowserContainer->AddRef();

    
    mWebBrowser->SetContainerWindow(static_cast<nsIWebBrowserChrome*>(mWebBrowserContainer));
    mWebBrowser->SetParentURIContentListener(mWebBrowserContainer);

    
    nsCOMPtr<nsIWeakReference> listener(
        do_GetWeakReference(static_cast<nsIWebProgressListener*>(mWebBrowserContainer)));
    mWebBrowser->AddWebBrowserListener(listener, NS_GET_IID(nsIWebProgressListener));

    
    mWebBrowserAsWin->SetVisibility(PR_TRUE);

    
    nsCOMPtr<nsIWebBrowserFocus> browserAsFocus = do_QueryInterface(mWebBrowser);
    browserAsFocus->Activate();

    
    mEditingSession = do_GetInterface(mWebBrowser);
    mCommandManager = do_GetInterface(mWebBrowser);

    
    sBrowserList.AppendElement(this);

    mValidBrowserFlag = TRUE;

    return S_OK;
}


HRESULT CMozillaBrowser::DestroyBrowser()
{
    

    mValidBrowserFlag = FALSE;

    
    sBrowserList.RemoveElement(this);

     
     if (mIERootDocument != NULL)
     {
         mIERootDocument->Release();
         mIERootDocument = NULL;
     }

    
    if (mWebBrowserAsWin)
    {
        mWebBrowserAsWin->Destroy();
        mWebBrowserAsWin = nsnull;
    }

    if (mWebBrowserContainer)
    {
        mWebBrowserContainer->Release();
        mWebBrowserContainer = NULL;
    }

    mEditingSession = nsnull;
    mCommandManager = nsnull;
    mWebBrowser = nsnull;

    return S_OK;
}



HRESULT CMozillaBrowser::SetEditorMode(BOOL bEnabled)
{
    NG_TRACE_METHOD(CMozillaBrowser::SetEditorMode);

    if (!mEditingSession || !mCommandManager)
        return E_FAIL;
  
    nsCOMPtr<nsIDOMWindow> domWindow;
    nsresult rv = GetDOMWindow(getter_AddRefs(domWindow));
    if (NS_FAILED(rv))
        return E_FAIL;

    rv = mEditingSession->MakeWindowEditable(domWindow, "html", PR_FALSE,
                                             PR_TRUE, PR_FALSE);
 
    return S_OK;
}


HRESULT CMozillaBrowser::OnEditorCommand(DWORD nCmdID)
{
    NG_TRACE_METHOD(CMozillaBrowser::OnEditorCommand);

    nsCOMPtr<nsIDOMWindow> domWindow;
    GetDOMWindow(getter_AddRefs(domWindow));

    const char *styleCommand = nsnull;
    nsICommandParams *commandParams = nsnull;

    switch (nCmdID)
    {
    case IDM_BOLD:
        styleCommand = "cmd_bold";
        break;
    case IDM_ITALIC:
        styleCommand = "cmd_italic";
        break;
    case IDM_UNDERLINE:
        styleCommand = "cmd_underline";
        break;
    
    
    
    default:
        
        break;
    }

    return mCommandManager ?
        mCommandManager->DoCommand(styleCommand, commandParams, domWindow) :
        NS_ERROR_FAILURE;
}



HRESULT CMozillaBrowser::GetDOMDocument(nsIDOMDocument **pDocument)
{
    NG_TRACE_METHOD(CMozillaBrowser::GetDOMDocument);

    HRESULT hr = E_FAIL;

    
    if (pDocument == NULL)
    {
        NG_ASSERT(0);
        return E_INVALIDARG;
    }

    *pDocument = nsnull;

    if (!BrowserIsValid())
    {
        NG_ASSERT(0);
        return E_UNEXPECTED;
    }

    
    nsCOMPtr<nsIDOMWindow> window;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(window));
    if (window)
    {
        if (NS_SUCCEEDED(window->GetDocument(pDocument)) && *pDocument)
        {
            hr = S_OK;
        }
    }

    return hr;
}



HRESULT CMozillaBrowser::LoadBrowserHelpers()
{
    NG_TRACE_METHOD(CMozillaBrowser::LoadBrowserHelpers);

    UnloadBrowserHelpers();

    
    

    CRegKey cKey;
    if (cKey.Open(HKEY_LOCAL_MACHINE, kBrowserHelperObjectRegKey, KEY_ENUMERATE_SUB_KEYS) != ERROR_SUCCESS)
    {
        NG_TRACE(_T("No browser helper key found\n"));
        return S_FALSE;
    }

    
    ULONG nHelperKeys = 0;
    LONG nResult = ERROR_SUCCESS;
    while (nResult == ERROR_SUCCESS)
    {
        TCHAR szCLSID[50];
        DWORD dwCLSID = sizeof(szCLSID) / sizeof(TCHAR);
        FILETIME cLastWrite;
        
        
        nResult = RegEnumKeyEx(cKey, nHelperKeys, szCLSID, &dwCLSID, NULL, NULL, NULL, &cLastWrite);
        if (nResult != ERROR_SUCCESS)
        {
            break;
        }
        nHelperKeys++;
    }
    if (nHelperKeys == 0)
    {
         NG_TRACE(_T("No browser helper objects found\n"));
        return S_FALSE;
    }

    
    CLSID *pClassList = new CLSID[nHelperKeys];
    DWORD nHelpers = 0;
    DWORD nKey = 0;
    nResult = ERROR_SUCCESS;
    while (nResult == ERROR_SUCCESS)
    {
        TCHAR szCLSID[50];
        DWORD dwCLSID = sizeof(szCLSID) / sizeof(TCHAR);
        FILETIME cLastWrite;
        
        
        nResult = RegEnumKeyEx(cKey, nKey++, szCLSID, &dwCLSID, NULL, NULL, NULL, &cLastWrite);
        if (nResult != ERROR_SUCCESS)
        {
            break;
        }

        NG_TRACE(_T("Reading helper object entry \"%s\"\n"), szCLSID);

        
        USES_CONVERSION;
        CLSID clsid;
        if (CLSIDFromString(T2OLE(szCLSID), &clsid) != NOERROR)
        {
            continue;
        }

        pClassList[nHelpers++] = clsid;
    }

    mBrowserHelperList = new CComUnkPtr[nHelpers];

    
    for (ULONG i = 0; i < nHelpers; i++)
    {
        CLSID clsid = pClassList[i];
        HRESULT hr;
        CComQIPtr<IObjectWithSite, &IID_IObjectWithSite> cpObjectWithSite;

        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IObjectWithSite, (LPVOID *) &cpObjectWithSite);
        if (FAILED(hr))
        {
            NG_TRACE(_T("Registered browser helper object cannot be created\n"));;
        }
        else
        {
            
            cpObjectWithSite->SetSite((IWebBrowser2 *) this);

            
            CComUnkPtr cpUnk = cpObjectWithSite;
            mBrowserHelperList[mBrowserHelperListCount++] = cpUnk;
        }
    }

    delete []pClassList;
        
    return S_OK;
}


HRESULT CMozillaBrowser::UnloadBrowserHelpers()
{
    NG_TRACE_METHOD(CMozillaBrowser::UnloadBrowserHelpers);

    if (mBrowserHelperList == NULL)
    {
        return S_OK;
    }

    
    for (ULONG i = 0; i < mBrowserHelperListCount; i++)
    {
        CComUnkPtr cpUnk = mBrowserHelperList[i];
        if (cpUnk)
        {
            CComQIPtr<IObjectWithSite, &IID_IObjectWithSite> cpObjectWithSite = cpUnk;
            if (cpObjectWithSite)
            {
                cpObjectWithSite->SetSite(NULL);
            }
        }
    }

    
    mBrowserHelperListCount = 0;
    delete []mBrowserHelperList;
    mBrowserHelperList = NULL;

    return S_OK;
}


HRESULT CMozillaBrowser::PrintDocument(BOOL promptUser)
{
#ifdef NS_PRINTING
    
    nsCOMPtr<nsIWebBrowserPrint> browserAsPrint = do_GetInterface(mWebBrowser);
    NS_ASSERTION(browserAsPrint, "No nsIWebBrowserPrint!");

    PRBool oldPrintSilent = PR_FALSE;
    nsCOMPtr<nsIPrintSettings> printSettings;
    browserAsPrint->GetGlobalPrintSettings(getter_AddRefs(printSettings));
    if (printSettings) 
    {
        printSettings->GetPrintSilent(&oldPrintSilent);
        printSettings->SetPrintSilent(promptUser ? PR_FALSE : PR_TRUE);
    }

    
    PRBool oldShowPrintProgress = FALSE;
    const char *kShowPrintProgressPref = "print.show_print_progress";
    mPrefBranch->GetBoolPref(kShowPrintProgressPref, &oldShowPrintProgress);
    mPrefBranch->SetBoolPref(kShowPrintProgressPref, PR_FALSE);

    
    PrintListener *listener = new PrintListener;
    nsCOMPtr<nsIWebProgressListener> printListener = do_QueryInterface(listener);
    nsresult rv = browserAsPrint->Print(printSettings, printListener);
    if (NS_SUCCEEDED(rv))
    {
        listener->WaitForComplete();
    }

    
    if (printSettings)
    {
        printSettings->SetPrintSilent(oldPrintSilent);
    }
    mPrefBranch->SetBoolPref(kShowPrintProgressPref, oldShowPrintProgress);
#endif
    return S_OK;
}







HRESULT CMozillaBrowser::InPlaceActivate(LONG iVerb, const RECT* prcPosRect)
{
    NG_TRACE_METHOD(CMozillaBrowser::InPlaceActivate);

    HRESULT hr;

    if (m_spClientSite == NULL)
        return S_OK;

    CComPtr<IOleInPlaceObject> pIPO;
    ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
    _ASSERTE(pIPO != NULL);
    if (prcPosRect != NULL)
        pIPO->SetObjectRects(prcPosRect, prcPosRect);

    if (!m_bNegotiatedWnd)
    {
        if (!m_bWindowOnly)
            
            hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_spInPlaceSite);

        if (m_spInPlaceSite)
        {
            m_bInPlaceSiteEx = TRUE;
            m_bWndLess = SUCCEEDED(m_spInPlaceSite->CanWindowlessActivate());
            m_bWasOnceWindowless = TRUE;
        }
        else
        {
            m_spClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&m_spInPlaceSite);
            if (m_spInPlaceSite)
                m_bInPlaceSiteEx = TRUE;
            else
                hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_spInPlaceSite);
        }
    }

    _ASSERTE(m_spInPlaceSite);
    if (!m_spInPlaceSite)
        return E_FAIL;

    m_bNegotiatedWnd = TRUE;

    if (!m_bInPlaceActive)
    {

        BOOL bNoRedraw = FALSE;
        if (m_bWndLess)
            m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, ACTIVATE_WINDOWLESS);
        else
        {
            if (m_bInPlaceSiteEx)
                m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, 0);
            else
            {
                HRESULT hr = m_spInPlaceSite->CanInPlaceActivate();
                if (FAILED(hr))
                    return hr;
                m_spInPlaceSite->OnInPlaceActivate();
            }
        }
    }

    m_bInPlaceActive = TRUE;

    
    
    
    OLEINPLACEFRAMEINFO frameInfo;
    RECT rcPos, rcClip;
    CComPtr<IOleInPlaceFrame> spInPlaceFrame;
    CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
    frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    HWND hwndParent;
    if (m_spInPlaceSite->GetWindow(&hwndParent) == S_OK)
    {
        m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
            &spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);

        if (!m_bWndLess)
        {
            if (m_hWndCD)
            {
                ::ShowWindow(m_hWndCD, SW_SHOW);
                ::SetFocus(m_hWndCD);
            }
            else
            {
                HWND h = CreateControlWindow(hwndParent, rcPos);
                _ASSERTE(h == m_hWndCD);
            }
        }

        pIPO->SetObjectRects(&rcPos, &rcClip);
    }

    CComPtr<IOleInPlaceActiveObject> spActiveObject;
    ControlQueryInterface(IID_IOleInPlaceActiveObject, (void**)&spActiveObject);

    
    if (DoesVerbUIActivate(iVerb))
    {
        if (!m_bUIActive)
        {
            m_bUIActive = TRUE;
            hr = m_spInPlaceSite->OnUIActivate();
            if (FAILED(hr))
                return hr;

            SetControlFocus(TRUE);
            
            
            if (spActiveObject)
            {
                if (spInPlaceFrame)
                    spInPlaceFrame->SetActiveObject(spActiveObject, NULL);
                if (spInPlaceUIWindow)
                    spInPlaceUIWindow->SetActiveObject(spActiveObject, NULL);
            }








        }
    }

    m_spClientSite->ShowObject();

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CMozillaBrowser::GetClientSite(IOleClientSite **ppClientSite)
{
    NG_TRACE_METHOD(CMozillaBrowser::GetClientSite);

    NG_ASSERT(ppClientSite);

    
    

    HRESULT hRes = E_POINTER;
    if (ppClientSite != NULL)
    {
        *ppClientSite = NULL;
        if (m_spClientSite == NULL)
        {
            return S_OK;
        }
    }

    return IOleObjectImpl<CMozillaBrowser>::GetClientSite(ppClientSite);
}






HRESULT STDMETHODCALLTYPE CMozillaBrowser::GetWebBrowser( void __RPC_FAR *__RPC_FAR *aBrowser)
{
    if (!NgIsValidAddress(aBrowser, sizeof(void *)))
    {
        NG_ASSERT(0);
        return SetErrorInfo(E_INVALIDARG);
    }

    *aBrowser = NULL;
    if (mWebBrowser)
    {
        nsIWebBrowser *browser = mWebBrowser.get();
        NS_ADDREF(browser);
        *aBrowser = (void *) browser;
    }
    return S_OK;
}




nsresult CMozillaBrowser::GetWebNavigation(nsIWebNavigation **aWebNav)
{
    NS_ENSURE_ARG_POINTER(aWebNav);
    if (!mWebBrowser) return NS_ERROR_FAILURE;
    return mWebBrowser->QueryInterface(NS_GET_IID(nsIWebNavigation), (void **) aWebNav);
}

nsresult CMozillaBrowser::GetDOMWindow(nsIDOMWindow **aDOMWindow)
{
    NS_ENSURE_ARG_POINTER(aDOMWindow);
    if (!mWebBrowser) return NS_ERROR_FAILURE;
    return mWebBrowser->GetContentDOMWindow(aDOMWindow);
}

nsresult CMozillaBrowser::GetPrefs(nsIPrefBranch **aPrefBranch)
{
    if (mPrefBranch)
        *aPrefBranch = mPrefBranch;
    NS_IF_ADDREF(*aPrefBranch);
    return (*aPrefBranch) ? NS_OK : NS_ERROR_FAILURE;
}

PRBool CMozillaBrowser::BrowserIsValid()
{
    NG_TRACE_METHOD(CMozillaBrowser::BrowserIsValid);
    return mValidBrowserFlag ? PR_TRUE : PR_FALSE;
}



HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Parent(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
    NG_TRACE_METHOD(CMozillaBrowser::get_Parent);
    ENSURE_BROWSER_IS_VALID();

    if (!NgIsValidAddress(ppDisp, sizeof(IDispatch *)))
    {
        NG_ASSERT(0);
        return SetErrorInfo(E_INVALIDARG);
    }

    
    HRESULT hr = E_FAIL;
    if (m_spClientSite)
    {
        hr = m_spClientSite->QueryInterface(IID_IDispatch, (void **) ppDisp);
    }

    return (SUCCEEDED(hr)) ? S_OK : E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_Document(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
    NG_TRACE_METHOD(CMozillaBrowser::get_Document);
    ENSURE_BROWSER_IS_VALID();

    if (!NgIsValidAddress(ppDisp, sizeof(IDispatch *)))
    {
        NG_ASSERT(0);
        return SetErrorInfo(E_UNEXPECTED);
    }

    *ppDisp = NULL;

    
    nsIDOMDocument *pIDOMDocument = nsnull;
    if (FAILED(GetDOMDocument(&pIDOMDocument)) || pIDOMDocument == nsnull)
    {
        return S_OK; 
    }

    if (mIERootDocument == NULL)
     {    
        nsCOMPtr <nsIDOMHTMLDocument> pIDOMHTMLDocument =
                do_QueryInterface(pIDOMDocument);

        if (!pIDOMDocument)
        {
            return SetErrorInfo(E_FAIL, L"get_Document: not HTML");
        }

         CIEHtmlDocumentInstance::CreateInstance(&mIERootDocument);
          if (mIERootDocument == NULL)
         {
            return SetErrorInfo(E_OUTOFMEMORY, L"get_Document: can't create IERootDocument");
         }
         
        
         mIERootDocument->AddRef();
 
         
         
         mIERootDocument->SetParent(this);
        mIERootDocument->SetDOMNode(pIDOMDocument);
        mIERootDocument->SetNative(pIDOMHTMLDocument);
    }

    mIERootDocument->QueryInterface(IID_IDispatch, (void **) ppDisp);
    pIDOMDocument->Release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMozillaBrowser::get_RegisterAsDropTarget(VARIANT_BOOL __RPC_FAR *pbRegister)
{
    NG_TRACE_METHOD(CMozillaBrowser::get_RegisterAsDropTarget);
    ENSURE_BROWSER_IS_VALID();

    if (pbRegister == NULL)
    {
        NG_ASSERT(0);
        return SetErrorInfo(E_INVALIDARG);
    }

    *pbRegister = mHaveDropTargetFlag ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}



static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    ::RevokeDragDrop(hwnd);
    return TRUE;
}
 

HRESULT STDMETHODCALLTYPE CMozillaBrowser::put_RegisterAsDropTarget(VARIANT_BOOL bRegister)
{
    NG_TRACE_METHOD(CMozillaBrowser::put_RegisterAsDropTarget);
    ENSURE_BROWSER_IS_VALID();

    
    if (bRegister == VARIANT_TRUE)
    {
        if (!mHaveDropTargetFlag)
        {
            CDropTargetInstance *pDropTarget = NULL;
            CDropTargetInstance::CreateInstance(&pDropTarget);
            if (pDropTarget)
            {
                pDropTarget->AddRef();
                pDropTarget->SetOwner(this);


                
                CIPtr(IDropTarget) spDropTarget;
                CIPtr(IDocHostUIHandler) spDocHostUIHandler = m_spClientSite;
                if (spDocHostUIHandler)
                {
                    
                    if (spDocHostUIHandler->GetDropTarget(pDropTarget, &spDropTarget) == S_OK)
                    {
                        mHaveDropTargetFlag = TRUE;
                        ::RegisterDragDrop(m_hWnd, spDropTarget);
                        
                    }
                }
                else
                
                {
                    mHaveDropTargetFlag = TRUE;
                    ::RegisterDragDrop(m_hWnd, pDropTarget);
                    
                }
                pDropTarget->Release();
            }
            
            
            ::EnumChildWindows(m_hWnd, EnumChildProc, (LPARAM) this);
        }
    }
    else
    {
        if (mHaveDropTargetFlag)
        {
            mHaveDropTargetFlag = FALSE;
            ::RevokeDragDrop(m_hWnd);
        }
    }

    return S_OK;
}





HRESULT _stdcall CMozillaBrowser::PrintHandler(CMozillaBrowser *pThis, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    BOOL promptUser = (nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER) ? FALSE : TRUE;
    pThis->PrintDocument(promptUser);
    return S_OK;
}


HRESULT _stdcall CMozillaBrowser::EditModeHandler(CMozillaBrowser *pThis, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    BOOL bEditorMode = (nCmdID == IDM_EDITMODE) ? TRUE : FALSE;
    pThis->SetEditorMode(bEditorMode);
    return S_OK;
}


HRESULT _stdcall CMozillaBrowser::EditCommandHandler(CMozillaBrowser *pThis, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    pThis->OnEditorCommand(nCmdID);
    return S_OK;
}





SimpleDirectoryProvider::SimpleDirectoryProvider()
{
    nsCOMPtr<nsILocalFile> appDataDir;

    
    

    CComPtr<IMalloc> shellMalloc;
    SHGetMalloc(&shellMalloc);
    if (shellMalloc)
    {
        LPITEMIDLIST pitemidList = NULL;
        SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pitemidList);
        if (pitemidList)
        {
            TCHAR szBuffer[MAX_PATH + 1];
            if (SUCCEEDED(SHGetPathFromIDList(pitemidList, szBuffer)))
            {
                szBuffer[MAX_PATH] = TCHAR('\0');
                USES_CONVERSION;
                NS_NewNativeLocalFile(nsDependentCString(T2A(szBuffer)), TRUE, getter_AddRefs(appDataDir));
            }
            shellMalloc->Free(pitemidList);
        }
    }
    if (!appDataDir)
    {
        return;
    }

    
    
    
    

    nsresult rv;

    
    PRBool exists;
    rv = appDataDir->Exists(&exists);
    if (NS_FAILED(rv) || !exists) return;

    
    rv = appDataDir->AppendRelativePath(NS_LITERAL_STRING("MozillaControl"));
    if (NS_FAILED(rv)) return;
    rv = appDataDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = appDataDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
    if (NS_FAILED(rv)) return;

    
    nsCOMPtr<nsIFile> appDataRegAsFile;
    rv = appDataDir->Clone(getter_AddRefs(appDataRegAsFile));
    if (NS_FAILED(rv)) return;
    nsCOMPtr<nsILocalFile> appDataRegistry = do_QueryInterface(appDataRegAsFile, &rv);
    if (NS_FAILED(rv)) return;
    appDataRegistry->AppendRelativePath(NS_LITERAL_STRING("registry.dat"));

    
    nsCOMPtr<nsIFile> profileDirAsFile;
    rv = appDataDir->Clone(getter_AddRefs(profileDirAsFile));
    if (NS_FAILED(rv)) return;
    nsCOMPtr<nsILocalFile> profileDir = do_QueryInterface(profileDirAsFile, &rv);
    if (NS_FAILED(rv)) return;
    profileDir->AppendRelativePath(NS_LITERAL_STRING("profiles"));
    rv = profileDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = profileDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
    if (NS_FAILED(rv)) return;

    
    mApplicationRegistryDir = appDataDir;
    mApplicationRegistryFile = appDataRegistry;
    mUserProfileDir = profileDir;
}

SimpleDirectoryProvider::~SimpleDirectoryProvider()
{
}

BOOL
SimpleDirectoryProvider::IsValid() const
{
    return (mApplicationRegistryDir && mApplicationRegistryFile && mUserProfileDir) ?
        TRUE : FALSE;
}

NS_IMPL_ISUPPORTS1(SimpleDirectoryProvider, nsIDirectoryServiceProvider)





NS_IMETHODIMP SimpleDirectoryProvider::GetFile(const char *prop, PRBool *persistent, nsIFile **_retval)
{
    NS_ENSURE_ARG_POINTER(prop);
    NS_ENSURE_ARG_POINTER(persistent);
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = nsnull;
    *persistent = PR_TRUE;

    
    
    
    nsCOMPtr<nsILocalFile> localFile;
    nsresult rv = NS_ERROR_FAILURE;

    if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) == 0)
    {
        localFile = mApplicationRegistryDir;
    }
    else if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_FILE) == 0)
    {
        localFile = mApplicationRegistryFile;
    }
    else if (strcmp(prop, NS_APP_USER_PROFILES_ROOT_DIR) == 0)
    {
        localFile = mUserProfileDir;
    }
    
    if (localFile)
        return CallQueryInterface(localFile, _retval);
        
    return rv;
}






#ifdef NS_PRINTING

NS_IMPL_ISUPPORTS1(PrintListener, nsIWebProgressListener)

PrintListener::PrintListener() : mComplete(PR_FALSE)
{
    
}

PrintListener::~PrintListener()
{
    
}

void PrintListener::WaitForComplete()
{
    MSG msg;
    HANDLE hFakeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    while (!mComplete)
    {
        
        while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (!::GetMessage(&msg, NULL, 0, 0))
            {
                ::CloseHandle(hFakeEvent);
                return;
            }

            PRBool wasHandled = PR_FALSE;
            ::NS_HandleEmbeddingEvent(msg, wasHandled);
            if (wasHandled)
                continue;

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        if (mComplete)
            break;
        
        
        ::MsgWaitForMultipleObjects(1, &hFakeEvent, FALSE, 500, QS_ALLEVENTS);
    }

    ::CloseHandle(hFakeEvent);
}


NS_IMETHODIMP PrintListener::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
    if (aStateFlags & nsIWebProgressListener::STATE_STOP &&
        aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT)
    {
        mComplete = PR_TRUE;
    }
    return NS_OK;
}


NS_IMETHODIMP PrintListener::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
    return NS_OK;
}


NS_IMETHODIMP PrintListener::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
    return NS_OK;
}


NS_IMETHODIMP PrintListener::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
    return NS_OK;
}


NS_IMETHODIMP PrintListener::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
    return NS_OK;
}
#endif
