










































#if !defined(AFX_NSIEDITSESSION_H__4C854323_B0B8_11D6_9BE0_00C04FA02BE6__INCLUDED_)
#define AFX_NSIEDITSESSION_H__4C854323_B0B8_11D6_9BE0_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



#include "BrowserView.h"
#include "BrowserImpl.h"
#include "StdAfx.h"
#include "Tests.h"




class CnsIEditSession
{

public:
	CnsIEditSession(nsIWebBrowser *mWebBrowser);

	nsCOMPtr<nsIWebBrowser> qaWebBrowser;

public:


public:


	
	
	


public:
	virtual ~CnsIEditSession();

	
	nsIEditingSession * GetEditSessionObject();
	void InitTest(PRInt16);
	void MakeWinEditTest(PRBool, PRInt16);
	void WinIsEditTest(PRBool, PRInt16);
	void SetEditorWinTest(PRInt16);
	void GetEditorWinTest(PRInt16);
	void TearEditorWinTest(PRInt16);
	void OnStartTests(UINT nMenuID);
	void RunAllTests(void);

	nsCOMPtr<nsIEditingSession> editingSession;
	nsCOMPtr<nsIDOMWindow> domWindow;
	
protected:

};






#endif
