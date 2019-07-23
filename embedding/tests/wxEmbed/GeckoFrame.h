





























#ifndef GECKOFRAME_H
#define GECKOFRAME_H

#include "nsIWebBrowser.h"

#include "GeckoWindow.h"
#include "GeckoContainer.h"

class GeckoFrame :
    public wxFrame,
    public GeckoContainerUI
{
protected:
    GeckoWindow            *mGeckoWnd;
    nsCOMPtr<nsIWebBrowser> mWebBrowser;
public:
    GeckoFrame();

    DECLARE_EVENT_TABLE()
    void OnActivate(wxActivateEvent &event);
    void OnEditCut(wxCommandEvent &event);
    void OnUpdateEditCut(wxUpdateUIEvent &event);
    void OnEditCopy(wxCommandEvent &event);
    void OnUpdateEditCopy(wxUpdateUIEvent &event);
    void OnEditPaste(wxCommandEvent &event);
    void OnUpdateEditPaste(wxUpdateUIEvent &event);
    void OnEditSelectAll(wxCommandEvent &event);

    
    
    
    bool SetupDefaultGeckoWindow();
    
    
    bool SetupGeckoWindow(GeckoWindow *aGeckoWindow, GeckoContainerUI *aUI, nsIWebBrowser **aWebBrowser) const;

    
    void SetFocus();
};

#endif