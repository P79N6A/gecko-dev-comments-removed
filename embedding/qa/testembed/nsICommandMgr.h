









































#if !defined(AFX_NSICOMMANDMGR_H__574F7B4A_B175_11D6_9BE0_00C04FA02BE6__INCLUDED_)
#define AFX_NSICOMMANDMGR_H__574F7B4A_B175_11D6_9BE0_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



#include "BrowserView.h"
#include "BrowserImpl.h"
#include "StdAfx.h"
#include "Tests.h"




class CnsICommandMgr
{

public:
	CnsICommandMgr(nsIWebBrowser *mWebBrowser);

	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	nsCOMPtr<nsICommandManager> cmdMgrObj;
	nsCOMPtr<nsICommandParams> cmdParamObj;
	
public:
	static nsICommandManager * GetCommandMgrObject(nsIWebBrowser *aWebBrowser, PRInt16);
	static nsICommandManager * GetCommandMgrWithContractIDObject(PRInt16);
	void IsCommandSupportedTest(const char *, PRInt16);
	void IsCommandEnabledTest(const char *, PRInt16);
	void GetCommandStateTest(const char *, PRInt16);
	void DoCommandTest(const char *, const char *, PRInt16);
	void OnStartTests(UINT nMenuID);
	void RunAllTests();

public:


	
	
	


public:
	virtual ~CnsICommandMgr();

	
protected:

};

struct CommandTest
{
	const char *mCmdName;
	const char *mDoCmdState;
	const char *mCmdParamState;
	PRBool	    mBooleanValue;
	PRInt32	    mLongValue;
	double	    mDoubleValue;
	char	   *mStringValue;
	char	   *mCStringValue;
};

extern CommandTest CommandTable[];






#endif
