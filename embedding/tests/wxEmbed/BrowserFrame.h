





























#ifndef BROWSERFRAME_H
#define BROWSERFRAME_H

#include "GeckoFrame.h"

class BrowserFrame :
    public GeckoFrame
{
protected:
    DECLARE_EVENT_TABLE()

    nsAutoString mContextLinkUrl;

    void OnBrowserUrl(wxCommandEvent &event);
    void OnBrowserGo(wxCommandEvent &event);
    void OnBrowserHome(wxCommandEvent &event);
    void OnBrowserOpenLinkInNewWindow(wxCommandEvent & event);

    void OnBrowserBack(wxCommandEvent &event);
    void OnUpdateBrowserBack(wxUpdateUIEvent &event);

    void OnBrowserForward(wxCommandEvent &event);
    void OnUpdateBrowserForward(wxUpdateUIEvent &event);

    void OnBrowserReload(wxCommandEvent &event);

    void OnBrowserStop(wxCommandEvent &event);
    void OnUpdateBrowserStop(wxUpdateUIEvent &event);

    void OnFileSave(wxCommandEvent &event);
    void OnFilePrint(wxCommandEvent &event);

    void OnViewPageSource(wxCommandEvent &event);
    void OnUpdateViewPageSource(wxUpdateUIEvent &event);

public :
    BrowserFrame(wxWindow* aParent);

    nsresult LoadURI(const char *aURI);
    nsresult LoadURI(const wchar_t *aURI);

    
    virtual nsresult CreateBrowserWindow(PRUint32 aChromeFlags,
         nsIWebBrowserChrome *aParent, nsIWebBrowserChrome **aNewWindow);
    virtual void UpdateStatusBarText(const PRUnichar* aStatusText);
    virtual void UpdateCurrentURI();
    virtual void ShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aContextMenuInfo);
};

#endif