









































#ifndef _TESTS_H
#define _TESTS_H

#if _MSC_VER > 1000
	#pragma once
#endif

#include "BrowserView.h"
#include "BrowserImpl.h"
#include "StdAfx.h"
#include "UrlDialog.h"




class CBrowserImpl;
class CBrowserView;

class CTests:public CWnd
{
public:
	CTests(nsIWebBrowser* mWebBrowser,
			   nsIBaseWindow* mBaseWindow,
			   nsIWebNavigation* mWebNav,
			   CBrowserImpl *mpBrowserImpl);
	virtual ~CTests();

	

	
	
	
	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	nsCOMPtr<nsIBaseWindow> qaBaseWindow;
	nsCOMPtr<nsIWebNavigation> qaWebNav;
	CBrowserImpl	*qaBrowserImpl;
	CUrlDialog myDialog;

	

	
	
	
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	

private:
	

	
	PRBool exists;
	nsCOMPtr<nsIURI> theURI;
	nsCOMPtr<nsIChannel> theChannel;
	UINT nCommandID;
protected:
	
	afx_msg void OnUpdateNavBack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavForward(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePaste(CCmdUI* pCmdUI);
	afx_msg void OnTestsChangeUrl();
	afx_msg void OnTestsGlobalHistory();
	afx_msg void OnTestsCreateFile();
	afx_msg void OnTestsCreateprofile();
	afx_msg void OnTestsAddTooltipListener();
	afx_msg void OnTestsAddWebProgListener();
	afx_msg void OnTestsAddHistoryListener();
	afx_msg void OnTestsRemovehistorylistener();
	afx_msg void OnTestsAddUriContentListenerByWebBrowser();
	afx_msg void OnTestsAddUriContentListenerByUriLoader();
	afx_msg void OnTestsAddUriContentListenerByOpenUri();
	afx_msg void OnTestsNSNewChannelAndAsyncOpen();
	afx_msg void OnTestsIOServiceNewURI();
	afx_msg void OnTestsProtocolHandlerNewURI();
	afx_msg void OnInterfacesNsifile();
	afx_msg void OnToolsRemoveGHPage();
	afx_msg void OnToolsRemoveAllGH();
	afx_msg void OnToolsViewLogfile();
	afx_msg void OnToolsDeleteLogfile();
	afx_msg void OnToolsTestYourMethod();
	afx_msg void OnToolsTestYourMethod2();
	afx_msg void OnVerifybugs70228();
	afx_msg void OnVerifybugs90195();
	afx_msg void OnVerifybugs169617();
	afx_msg void OnVerifybugs170274();
    afx_msg void OnPasteTest();
    afx_msg void OnCopyTest();
    afx_msg void OnSelectAllTest();
    afx_msg void OnSelectNoneTest();
    afx_msg void OnCutSelectionTest();
    afx_msg void copyLinkLocationTest();
    afx_msg void canCopySelectionTest();
    afx_msg void canCutSelectionTest();
    afx_msg void canPasteTest();
	afx_msg void OnInterfacesNsirequest();
	afx_msg void OnInterfacesNsidomwindow();
	afx_msg void OnInterfacesNsidirectoryservice();
	afx_msg void OnInterfacesNsiselection();
	afx_msg void OnInterfacesNsiprofile();
	afx_msg void OnInterfacesNsishistory();
	afx_msg void OnInterfacesNsiwebnav();
	afx_msg void OnInterfacesNsiclipboardcommands();
	afx_msg void OnInterfacesNsiobserverservice();
	afx_msg void OnInterfacesNsiwebbrowser();
	afx_msg void OnInterfacesNsiwebprogress();
	afx_msg void OnInterfacesNsiwebbrowfind();
	afx_msg void OnInterfacesNsieditingsession();
	afx_msg void OnInterfacesNsicommandmgr();
	afx_msg void OnInterfacesNsicmdparams();
	afx_msg void OnInterfacesRunalltestcases();
	afx_msg void OnInterfacesNsichannel();
	afx_msg void OnInterfacesNsihttpchannel();

	

	DECLARE_MESSAGE_MAP()

	
	void InitWithPathTest(nsILocalFile *);
	void AppendRelativePathTest(nsILocalFile *);
	void FileCreateTest(nsILocalFile *);
	void FileExistsTest(nsILocalFile *);
	void FileCopyTest(nsILocalFile *, nsILocalFile *);
	void FileMoveTest(nsILocalFile *, nsILocalFile *);
};

#endif 

