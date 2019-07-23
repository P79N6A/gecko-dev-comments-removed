





























#include "global.h"

#include "GeckoWindow.h"
#include "nsIWebBrowserFocus.h"

IMPLEMENT_DYNAMIC_CLASS(GeckoWindow, wxPanel)

GeckoWindow::GeckoWindow(void) :
    mGeckoContainer(NULL)
{
}

GeckoWindow::~GeckoWindow()
{
    SetGeckoContainer(NULL);
}

void GeckoWindow::SetGeckoContainer(GeckoContainer *aGeckoContainer)
{
    if (aGeckoContainer != mGeckoContainer)
    {
        NS_IF_RELEASE(mGeckoContainer);
        mGeckoContainer = aGeckoContainer;
        NS_IF_ADDREF(mGeckoContainer);
    }
}

BEGIN_EVENT_TABLE(GeckoWindow, wxPanel)
    EVT_SIZE(      GeckoWindow::OnSize)
    EVT_SET_FOCUS( GeckoWindow::OnSetFocus)
    EVT_KILL_FOCUS(GeckoWindow::OnKillFocus)
END_EVENT_TABLE()

void GeckoWindow::OnSize(wxSizeEvent &event)
{
    if (!mGeckoContainer)
    {
        return;
    }
    
    nsCOMPtr<nsIWebBrowser> webBrowser;
    mGeckoContainer->GetWebBrowser(getter_AddRefs(webBrowser));
    nsCOMPtr<nsIBaseWindow> webBrowserAsWin = do_QueryInterface(webBrowser);
    if (webBrowserAsWin)
    {
        wxSize size = GetClientSize();
        webBrowserAsWin->SetPositionAndSize(
            0, 0, size.GetWidth(), size.GetHeight(), PR_TRUE);
        webBrowserAsWin->SetVisibility(PR_TRUE);
    }
}

void GeckoWindow::OnSetFocus(wxFocusEvent &event)
{
}

void GeckoWindow::OnKillFocus(wxFocusEvent &event)
{
}
