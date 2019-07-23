




























#include "global.h"

#include "wx/strconv.h"

#include "BrowserFrame.h"

#include "nsIURI.h"

BEGIN_EVENT_TABLE(BrowserFrame, GeckoFrame)

    
    EVT_MENU(XRCID("view_pagesource"),  BrowserFrame::OnViewPageSource)
    EVT_UPDATE_UI(XRCID("view_pagesource"),
                                        BrowserFrame::OnUpdateViewPageSource)

    
    
    EVT_MENU(XRCID("browse_back"),      BrowserFrame::OnBrowserBack)
    EVT_UPDATE_UI(XRCID("browse_back"), BrowserFrame::OnUpdateBrowserBack)
    EVT_MENU(XRCID("browse_fwd"),       BrowserFrame::OnBrowserForward)
    EVT_UPDATE_UI(XRCID("browse_fwd"),  BrowserFrame::OnUpdateBrowserForward)
    EVT_MENU(XRCID("browse_reload"),    BrowserFrame::OnBrowserReload)
    EVT_MENU(XRCID("browse_stop"),      BrowserFrame::OnBrowserStop)
    EVT_UPDATE_UI(XRCID("browse_stop"), BrowserFrame::OnUpdateBrowserStop)
    EVT_MENU(XRCID("browse_home"),      BrowserFrame::OnBrowserHome)
    EVT_BUTTON(XRCID("browser_go"),     BrowserFrame::OnBrowserGo)
    EVT_TEXT_ENTER(XRCID("url"),        BrowserFrame::OnBrowserUrl)
    EVT_MENU(XRCID("browser_open_in_new_window"),
                                        BrowserFrame::OnBrowserOpenLinkInNewWindow)
END_EVENT_TABLE()

BrowserFrame::BrowserFrame(wxWindow* aParent)
{
    wxXmlResource::Get()->LoadFrame(this, aParent, wxT("browser_frame"));

    SetIcon(wxICON(appicon));

    SetName("browser");

    SetupDefaultGeckoWindow();

    CreateStatusBar();
}


nsresult BrowserFrame::LoadURI(const wchar_t *aURI)
{
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        if (webNav)
        {
            return webNav->LoadURI(aURI,
                    nsIWebNavigation::LOAD_FLAGS_NONE,
                    nsnull,
                    nsnull,
                    nsnull);
        }
    }
    return NS_ERROR_FAILURE;
}


nsresult BrowserFrame::LoadURI(const char *aURI)
{
    wxMBConv conv;
    return LoadURI(conv.cWX2WC(aURI));
}






void BrowserFrame::OnFileSave(wxCommandEvent & WXUNUSED(event))
{
}

void BrowserFrame::OnFilePrint(wxCommandEvent & WXUNUSED(event))
{
}

void BrowserFrame::OnViewPageSource(wxCommandEvent &event)
{
    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
    if(!webNav)
        return;

    
    nsresult rv = NS_OK;
    nsCOMPtr<nsIURI> currentURI;
    rv = webNav->GetCurrentURI(getter_AddRefs(currentURI));
    if(NS_FAILED(rv) || !currentURI)
        return;

    
    nsCAutoString uriString;
    rv = currentURI->GetSpec(uriString);
    if(NS_FAILED(rv))
        return;

    
    nsAutoString viewSrcUrl(L"view-source:");
    viewSrcUrl.AppendWithConversion(uriString.get());

    BrowserFrame *frame = new BrowserFrame(NULL);
    if (frame)
    {
        frame->Show(TRUE);
        frame->LoadURI(viewSrcUrl.get());
    }

}

void BrowserFrame::OnUpdateViewPageSource(wxUpdateUIEvent &event)
{
}

void BrowserFrame::OnBrowserGo(wxCommandEvent & WXUNUSED(event))
{
    wxTextCtrl *txtCtrl = (wxTextCtrl *) FindWindowById(XRCID("url"), this);
    wxString url = txtCtrl->GetValue();
    if (!url.IsEmpty())
    {
        LoadURI(url);
    }
}

void BrowserFrame::OnBrowserUrl(wxCommandEvent & event)
{
    OnBrowserGo(event);
}

void BrowserFrame::OnBrowserBack(wxCommandEvent & WXUNUSED(event))
{
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        webNav->GoBack();
    }
}

void BrowserFrame::OnUpdateBrowserBack(wxUpdateUIEvent &event)
{
    PRBool canGoBack = PR_FALSE;
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        webNav->GetCanGoBack(&canGoBack);
    }
    event.Enable(canGoBack ? true : false);
}

void BrowserFrame::OnBrowserForward(wxCommandEvent & WXUNUSED(event))
{
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        webNav->GoForward();
    }
}

void BrowserFrame::OnUpdateBrowserForward(wxUpdateUIEvent &event)
{
    PRBool canGoForward = PR_FALSE;
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        webNav->GetCanGoForward(&canGoForward);
    }
    event.Enable(canGoForward ? true : false);
}

void BrowserFrame::OnBrowserReload(wxCommandEvent & WXUNUSED(event))
{
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        webNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
    }
}

void BrowserFrame::OnBrowserStop(wxCommandEvent & WXUNUSED(event))
{
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        webNav->Stop(nsIWebNavigation::STOP_ALL);
    }
}

void BrowserFrame::OnUpdateBrowserStop(wxUpdateUIEvent &event)
{
    event.Enable(mBusy ? true : false);
}

void BrowserFrame::OnBrowserHome(wxCommandEvent & WXUNUSED(event))
{
    LoadURI("http://www.mozilla.org/projects/embedding/");
}

void BrowserFrame::OnBrowserOpenLinkInNewWindow(wxCommandEvent & WXUNUSED(event))
{
    BrowserFrame* frame = new BrowserFrame(NULL);
    frame->Show(TRUE);
    frame->LoadURI(mContextLinkUrl.get());
}




nsresult BrowserFrame::CreateBrowserWindow(PRUint32 aChromeFlags,
         nsIWebBrowserChrome *aParent, nsIWebBrowserChrome **aNewWindow)
{
    
    BrowserFrame* frame = new BrowserFrame(NULL);
    if (!frame)
        return NS_ERROR_OUT_OF_MEMORY;
    frame->Show(TRUE);
    GeckoContainer *container = frame->mGeckoWnd->GetGeckoContainer();
    return container->QueryInterface(NS_GET_IID(nsIWebBrowserChrome), (void **) aNewWindow);
}

void BrowserFrame::UpdateStatusBarText(const PRUnichar* aStatusText)
{
    SetStatusText(aStatusText);
}

void BrowserFrame::UpdateCurrentURI()
{
    if (mWebBrowser)
    {
        nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
        nsCOMPtr<nsIURI> currentURI;
        webNav->GetCurrentURI(getter_AddRefs(currentURI));
        nsCAutoString spec;
        currentURI->GetSpec(spec);

        wxTextCtrl *txtCtrl = (wxTextCtrl *) FindWindowById(XRCID("url"), this);
        if (txtCtrl)
        {
            txtCtrl->SetValue(spec.get());
        }
    }
}

#include "nsIDOMMouseEvent.h"

void BrowserFrame::ShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aContextMenuInfo)
{
    nsCOMPtr<nsIDOMEvent> event;
    aContextMenuInfo->GetMouseEvent(getter_AddRefs(event));
    if (!event)
    {
        return;
    }

    mContextLinkUrl.SetLength(0);

    nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event);
    if (mouseEvent)
    {
        PRInt32 x, y;
        mouseEvent->GetScreenX(&x);
        mouseEvent->GetScreenY(&y);

        char *menuName = NULL;

        
        
        
        
        
        
        
        
        
        if (aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE)
        {
            if (aContextFlags & nsIContextMenuListener2::CONTEXT_LINK)
                menuName = "context_browser_image"; 
            else if (aContextFlags & nsIContextMenuListener2::CONTEXT_INPUT)
                menuName = "context_browser_image"; 
            else
                menuName = "context_browser_image"; 
        }
        else if (aContextFlags & nsIContextMenuListener2::CONTEXT_LINK)
        {
            menuName = "context_browser_link";

            aContextMenuInfo->GetAssociatedLink(mContextLinkUrl);
        }
        else if (aContextFlags & nsIContextMenuListener2::CONTEXT_INPUT)
        {
            menuName = "context_browser_input";
        }
        else if (aContextFlags & nsIContextMenuListener2::CONTEXT_TEXT)
        {
            menuName = "context_browser_text";
        }
        else if (aContextFlags & nsIContextMenuListener2::CONTEXT_DOCUMENT)
        {
            menuName = "context_browser_document";
        }
        else if (aContextFlags & nsIContextMenuListener2::CONTEXT_BACKGROUND_IMAGE)
        {
            menuName = "context_browser_document";
        }

        if (!menuName)
            return;


#undef LoadMenu
        wxMenu *menu = wxXmlResource::Get()->LoadMenu(menuName);
        if (menu)
        {
            int fX = 0, fY = 0;
            
            
            GetPosition(&fX, &fY);
            PopupMenu(menu, x - fX, y - fY);
        }
    }
}


