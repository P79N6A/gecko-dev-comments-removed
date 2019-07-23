









































#ifndef _NSIWEBBROW_H
#define _NSIWEBBROW_H

#if _MSC_VER > 1000
	#pragma once
#endif

#include "BrowserView.h"
#include "BrowserImpl.h"
#include "StdAfx.h"





class CBrowserImpl;

class CNsIWebBrowser
{
public:
	CNsIWebBrowser(nsIWebBrowser *mWebBrowser,
				   CBrowserImpl *mpBrowserImpl);
	virtual ~CNsIWebBrowser();

	
	
	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	CBrowserImpl	*qaBrowserImpl;

	
	void WBAddListener(PRInt16);
	void WBRemoveListener(PRInt16);
	void WBGetContainerWindow(PRInt16);
	void WBSetContainerWindow(PRInt16);
	void WBGetURIContentListener(PRInt16);
	void WBSetURIContentListener(PRInt16);
	void WBGetDOMWindow(PRInt16);
	void OnStartTests(UINT nMenuID);
	void RunAllTests();

	
	void WBSSetupProperty(PRInt16);

	

	
	
	
	public:
	

private:
	
	nsCOMPtr<nsIWebBrowserChrome> qaWebBrowserChrome;
	nsCOMPtr<nsIURIContentListener> qaURIContentListener;
	
protected:
	

	
};

#endif
