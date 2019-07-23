






































#ifndef EMBEDWINDOW_H
#define EMBEDWINDOW_H

#include <nsString.h>
#include <nsIWebBrowserChrome.h>
#include <nsIWebBrowserChromeFocus.h>
#include <nsIEmbeddingSiteWindow.h>
#include <nsITooltipListener.h>
#include <nsIContextMenuListener.h>
#include <nsISupports.h>
#include <nsIWebBrowser.h>
#include <nsIBaseWindow.h>
#include <nsIInterfaceRequestor.h>
#include <nsCOMPtr.h>
#include "nsString.h"

class QGeckoEmbed;
class MozTipLabel;

class EmbedWindow : public nsIWebBrowserChrome,
                    public nsIWebBrowserChromeFocus,
                    public nsIEmbeddingSiteWindow,
                    public nsITooltipListener,
                    public nsIContextMenuListener,
                    public nsIInterfaceRequestor
{
public:
    EmbedWindow();
    ~EmbedWindow();
    void Init(QGeckoEmbed *aOwner);

    nsresult CreateWindow    (void);
    void     ReleaseChildren (void);

    NS_DECL_ISUPPORTS

    NS_DECL_NSIWEBBROWSERCHROME

    NS_DECL_NSIWEBBROWSERCHROMEFOCUS

    NS_DECL_NSIEMBEDDINGSITEWINDOW

    NS_DECL_NSITOOLTIPLISTENER

    NS_DECL_NSICONTEXTMENULISTENER

    NS_DECL_NSIINTERFACEREQUESTOR

    nsString                 mTitle;
    nsString                 mJSStatus;
    nsString                 mLinkMessage;

    nsCOMPtr<nsIBaseWindow>  mBaseWindow; 

private:
    QGeckoEmbed              *mOwner;
    nsCOMPtr<nsIWebBrowser>  mWebBrowser; 
    PRBool                   mVisibility;
    PRBool                   mIsModal;
    MozTipLabel *tooltip;
};

#endif
