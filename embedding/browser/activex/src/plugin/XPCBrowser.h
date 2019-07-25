





































#ifndef IEBrowser_H
#define IEBrowser_H

#include <docobj.h>
#include <ExDisp.h>

#include "nsCOMPtr.h"
#include "nsIWebNavigation.h"

#include "LegacyPlugin.h"

#include "IWebBrowserImpl.h"

class IEBrowser :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IWebBrowserImpl<IEBrowser, &CLSID_NULL>
{
public:
BEGIN_COM_MAP(IEBrowser)
    COM_INTERFACE_ENTRY(IWebBrowser)
    COM_INTERFACE_ENTRY(IWebBrowser2)
    COM_INTERFACE_ENTRY(IWebBrowserApp)
END_COM_MAP()

    PluginInstanceData *mData;
    nsCOMPtr<nsIWebNavigation> mWebNavigation;
    nsCOMPtr<nsIDOMWindow>     mDOMWindow;

    IEBrowser();
    HRESULT Init(PluginInstanceData *pData);

protected:
    virtual ~IEBrowser();

public:

    
    virtual nsresult GetWebNavigation(nsIWebNavigation **aWebNav);
    
    virtual nsresult GetDOMWindow(nsIDOMWindow **aDOMWindow);
    
    virtual nsresult GetPrefs(nsIPrefBranch **aPrefBranch);
    
    virtual PRBool BrowserIsValid();
};


#endif
