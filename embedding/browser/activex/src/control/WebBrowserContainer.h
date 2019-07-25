




































#ifndef WEBBROWSERCONTAINER_H
#define WEBBROWSERCONTAINER_H

#include "nsIContextMenuListener.h"
#include "nsITooltipListener.h"
#include "nsICommandHandler.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIURIContentListener.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsWeakReference.h"




class CWebBrowserContainer :
        public nsIEmbeddingSiteWindow2,
        public nsIWebBrowserChrome,
        public nsIWebProgressListener,
        public nsIRequestObserver,
        public nsIURIContentListener,
        public nsIInterfaceRequestor,
        public nsIContextMenuListener,
        public nsICommandHandler,
        public nsIWebBrowserChromeFocus,
        public nsSupportsWeakReference
{
public:
    CWebBrowserContainer(CMozillaBrowser *pOwner);

    friend CMozillaBrowser;
    friend CWindowCreator;

protected:
    virtual ~CWebBrowserContainer();


protected:
    CMozillaBrowser *mOwner;
    nsCOMPtr<nsIURI> mCurrentURI;
    CProxyDWebBrowserEvents<CMozillaBrowser>  *mEvents1;
    CProxyDWebBrowserEvents2<CMozillaBrowser> *mEvents2;
    nsString mTitle;
    PRPackedBool mVisible;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIEMBEDDINGSITEWINDOW2
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIURICONTENTLISTENER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSICONTEXTMENULISTENER
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSICOMMANDHANDLER
};

#endif

