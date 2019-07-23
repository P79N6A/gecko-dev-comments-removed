









































#if !defined(AFX_NSIWEBPROG_H__8024AB8F_6DB3_11D6_9BA5_00C04FA02BE6__INCLUDED_)
#define AFX_NSIWEBPROG_H__8024AB8F_6DB3_11D6_9BA5_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



#include "BrowserView.h"
#include "BrowserImpl.h"
#include "StdAfx.h"
#include "Tests.h"




class CBrowserImpl;

class CnsiWebProg
{
public:
	
	CnsiWebProg(nsIWebBrowser *mWebBrowser,
				   CBrowserImpl *mpBrowserImpl);

	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	CBrowserImpl	*qaBrowserImpl;
	nsIWebProgress * GetWebProgObject();
	void AddWebProgLstnr(PRUint32, PRInt16);
	void RemoveWebProgLstnr(PRInt16);
	void GetTheDOMWindow(PRInt16);
	void GetIsLoadingDocTest(PRInt16);

	void ConvertWPFlagToString(PRUint32, nsCAutoString&);
	void StoreWebProgFlag(PRUint32);
	void RetrieveWebProgFlag(void);

	void OnStartTests(UINT nMenuID);
	void RunAllTests(void);

	PRUint32 theStoredFlag;

public:



public:



public:
	virtual ~CnsiWebProg();

	
protected:

private:
	nsCOMPtr<nsIWebProgress> qaWebProgress;

};






#endif 
