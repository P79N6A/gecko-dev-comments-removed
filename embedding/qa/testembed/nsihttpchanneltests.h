#if !defined(AFX_NSIHTTPCHANNELTESTS_H__A7985BC6_9A57_453F_BEE4_17A083131427__INCLUDED_)
#define AFX_NSIHTTPCHANNELTESTS_H__A7985BC6_9A57_453F_BEE4_17A083131427__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



#include "Tests.h"




class CnsIHttpChannelTests
{

public:
	CnsIHttpChannelTests(nsIWebBrowser* mWebBrowser, CBrowserImpl *mpBrowserImpl);



	nsIHttpChannel * GetHttpChannelObject(nsCAutoString);
	void OnStartTests(UINT nMenuID);
	void RunAllTests();
	void SetRequestMethodTest(nsIHttpChannel *, const char *, PRInt16);
	void GetRequestMethodTest(nsIHttpChannel *, PRInt16);
	void SetReferrerTest(nsIHttpChannel *, const char *, PRInt16);
	void GetReferrerTest(nsIHttpChannel *, PRInt16);
	void SetRequestHeaderTest(nsIHttpChannel *, const char *, const char *, PRInt16);
	void GetRequestHeaderTest(nsIHttpChannel *, const char *, PRInt16);
	void VisitRequestHeadersTest(nsIHttpChannel *, PRInt16);
	void SetAllowPipeliningTest(nsIHttpChannel *, PRBool, PRInt16);
	void GetAllowPipeliningTest(nsIHttpChannel *, PRInt16);
	void SetRedirectionLimitTest(nsIHttpChannel *, PRUint32, PRInt16);
	void GetRedirectionLimitTest(nsIHttpChannel *, PRInt16);

	
	void CallResponseTests(nsIHttpChannel *, PRInt16);
	void GetResponseStatusTest(nsIHttpChannel *, PRInt16);
	void GetResponseStatusTextTest(nsIHttpChannel *, PRInt16);
	void GetRequestSucceededTest(nsIHttpChannel *, PRInt16);
	void GetResponseHeaderTest(nsIHttpChannel *, const char *, PRInt16);
	void SetResponseHeaderTest(nsIHttpChannel *, const char *, const char *, PRBool, PRInt16);
	void VisitResponseHeaderTest(nsIHttpChannel *, PRInt16);
	void IsNoStoreResponseTest(nsIHttpChannel *, PRInt16);
	void IsNoCacheResponseTest(nsIHttpChannel *, PRInt16);
public:


public:


	
	
	


public:
	virtual ~CnsIHttpChannelTests();

	
protected:

private:
	CBrowserImpl *qaBrowserImpl;
	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	nsCOMPtr<nsIChannel> theChannel;
	nsCOMPtr<nsIURI> theURI;
	nsCOMPtr<nsIHttpChannel> theHttpChannel;
	nsCAutoString theSpec;
};






#endif 
