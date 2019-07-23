







































#include "Tests.h"

#if !defined(AFX_NSICHANNELTESTS_H__33C5EBD3_0178_11D7_9C13_00C04FA02BE6__INCLUDED_)
#define AFX_NSICHANNELTESTS_H__33C5EBD3_0178_11D7_9C13_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 







class CnsIChannelTests
{

public:
	CnsIChannelTests(nsIWebBrowser* mWebBrowser, CBrowserImpl *mpBrowserImpl);


public:


public:


	nsIChannel * GetChannelObject(nsCAutoString);
	nsIURI * GetURIObject(nsCAutoString);
	void SetOriginalURITest(nsIChannel *, nsCAutoString, PRInt16);
	void GetOriginalURITest(nsIChannel *, PRInt16);
	void GetURITest(nsIChannel *, PRInt16);
	void SetOwnerTest(nsIChannel *, PRInt16);
	void GetOwnerTest(nsIChannel *, PRInt16);
	void SetNotificationsTest(nsIChannel *, PRInt16);
	void GetNotificationsTest(nsIChannel *, PRInt16);
	void GetSecurityInfoTest(nsIChannel *, PRInt16);
	void SetContentTypeTest(nsIChannel *, PRInt16);
	void GetContentTypeTest(nsIChannel *, PRInt16);
	void SetContentCharsetTest(nsIChannel *, PRInt16);
	void GetContentCharsetTest(nsIChannel *, PRInt16);
	void SetContentLengthTest(nsIChannel *, PRInt16);
	void GetContentLengthTest(nsIChannel *, PRInt16);
	void OpenTest(nsIChannel *, PRInt16);
	void AsyncOpenTest(nsIChannel *, PRInt16);
	void PostAsyncOpenTests(nsIChannel *, PRInt16);
	void OnStartTests(UINT nMenuID);
	void RunAllTests();
public:
	virtual ~CnsIChannelTests();

	
protected:

private:
	CBrowserImpl *qaBrowserImpl;
	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	nsCOMPtr<nsIChannel> theChannel;
	nsCOMPtr<nsIURI> theURI;
	nsCOMPtr<nsISupports> theSupports;
	nsCOMPtr<nsIInterfaceRequestor> theIRequestor;
	nsCOMPtr<nsIInputStream> theInputStream;
	nsCAutoString theSpec;
};

typedef struct
{
	char		theURL[1024];
	char		contentType[1024];	
} ChannelRow;






#endif 
