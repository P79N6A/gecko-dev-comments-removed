





































#ifndef __CBrowserChrome__
#define __CBrowserChrome__

#pragma once


#include "nsCOMPtr.h"


#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIContextMenuListener2.h"
#include "nsITooltipListener.h"
#include "nsWeakReference.h"

#include <LListener.h>


#include "nsIWebBrowser.h"


class CBrowserShell;

class CBrowserChrome : public nsIWebBrowserChrome,
                       public nsIWebBrowserChromeFocus,
                       public nsIEmbeddingSiteWindow2,
                       public nsIInterfaceRequestor,
                       public nsIContextMenuListener2,
                       public nsITooltipListener,
                       public nsSupportsWeakReference,
                       public LListener
{
    friend class CBrowserShell;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIEMBEDDINGSITEWINDOW2
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICONTEXTMENULISTENER2
    NS_DECL_NSITOOLTIPLISTENER

    
	virtual void	    ListenToMessage(MessageT inMessage,
                                        void* ioParam);
                                        
    
    static LWindow*     GetLWindowForDOMWindow(nsIDOMWindow* aDOMWindow);
  
protected:    
                        CBrowserChrome(CBrowserShell* aShell,
                                       UInt32 aChromeFlags,
                                       Boolean aIsMainContent);
    virtual             ~CBrowserChrome();

    void                SetBrowserShell(CBrowserShell* aShell);
    
    
protected:
    CBrowserShell       *mBrowserShell;
    LWindow             *mBrowserWindow;

    PRUint32            mChromeFlags;
    Boolean             mIsMainContent;
    Boolean             mSizeToContent;
    Boolean             mInModalLoop;
    Boolean             mWindowVisible;
    Boolean             mInitialLoadComplete;
};

#endif 
