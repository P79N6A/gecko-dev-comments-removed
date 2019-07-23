






































#ifndef _BROWSERIMPL_H
#define _BROWSERIMPL_H

#include "IBrowserFrameGlue.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIContextMenuListener.h"
#include "nsIDOMNode.h"
#include "nsISHistoryListener.h"



class CBrowserImpl : 
	 public nsIInterfaceRequestor,
	 public nsIWebBrowserChrome,
     public nsIWebBrowserChromeFocus,
	 public nsIEmbeddingSiteWindow,
	 public nsIWebProgressListener,
	 public nsIContextMenuListener,
	 public nsSupportsWeakReference,
	 public nsISHistoryListener,		
	 public nsIStreamListener,			
	 public nsITooltipListener,   		
	 public nsIURIContentListener		
{
public:
    CBrowserImpl();
    ~CBrowserImpl();
    NS_METHOD Init(PBROWSERFRAMEGLUE pBrowserFrameGlue,
                   nsIWebBrowser* aWebBrowser);

	
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSICONTEXTMENULISTENER
	NS_DECL_NSISHISTORYLISTENER		 
	NS_DECL_NSISTREAMLISTENER  		 
	NS_DECL_NSIREQUESTOBSERVER		 
	NS_DECL_NSITOOLTIPLISTENER		 
	NS_DECL_NSIURICONTENTLISTENER


protected:

    PBROWSERFRAMEGLUE  m_pBrowserFrameGlue;

    nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsISupports> mLoadCookie;						
	nsCOMPtr<nsIURIContentListener> mParentContentListener;	
	PRUint32 mChromeMask;
};

#endif 
