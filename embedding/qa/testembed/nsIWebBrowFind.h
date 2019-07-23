









































#ifndef _NSIWEBBROWFIND_H
#define _NSIWEBBROWFIND_H

#if _MSC_VER > 1000
	#pragma once
#endif

#include "BrowserView.h"
#include "BrowserImpl.h"
#include "StdAfx.h"
#include "UrlDialog.h"
#include "QaFindDlg.h"





class CBrowserImpl;
class CUrlDialog;

class CNsIWebBrowFind
{
public:
	CNsIWebBrowFind(nsIWebBrowser *mWebBrowser,
				   CBrowserImpl *mpBrowserImpl);
	virtual ~CNsIWebBrowFind();

	
	
	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	CBrowserImpl	*qaBrowserImpl;

	
	nsIWebBrowserFind * GetWebBrowFindObject();
	void SetSearchStringTest(PRInt16);
	void GetSearchStringTest(PRInt16);
	void FindNextTest(PRBool, PRInt16);
	void SetFindBackwardsTest(PRBool, PRInt16);
	void GetFindBackwardsTest(PRBool, PRInt16);
	void SetWrapFindTest(PRBool, PRInt16);
	void GetWrapFindTest(PRBool, PRInt16);
	void SetEntireWordTest(PRBool, PRInt16);
	void GetEntireWordTest(PRBool, PRInt16);
	void SetMatchCase(PRBool, PRInt16);
	void GetMatchCase(PRBool, PRInt16);
	void SetSearchFrames(PRBool, PRInt16);
	void GetSearchFrames(PRBool, PRInt16);

	void OnStartTests(UINT nMenuID);
	void RunAllTests();

	

	
	
	
	public:
	

private:
	

	CQaFindDlg myDialog;

protected:
	

	
};

#endif
