










































#include "Tests.h"




class CNsIRequest 
{

public:	

	CNsIRequest(nsIWebBrowser* mWebBrowser,CBrowserImpl *mpBrowserImpl);


public:
	
	
	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	CBrowserImpl *qaBrowserImpl;


public:


	
	
	


public:
	virtual ~CNsIRequest();

public:
	void OnInterfacesNsirequest();
	
	void static IsPendingReqTest(nsIRequest *);
	void static GetStatusReqTest(nsIRequest *);
	void static SuspendReqTest(nsIRequest *);
	void static ResumeReqTest(nsIRequest *);
	void static CancelReqTest(nsIRequest *);
	void static SetLoadGroupTest(nsIRequest *, nsILoadGroup *);
	void static GetLoadGroupTest(nsIRequest *);
    void OnStartTests(UINT nMenuID);
	void RunAllTests(int);
	void RunIndividualTests(UINT nMenuID, int);
	nsIChannel * GetTheChannel(int, nsILoadGroup *);
	
protected:



};


typedef struct
{
	char		theUrl[1024];
	bool		reqPend;
	bool		reqStatus;
	bool		reqSuspend;
	bool		reqResume;
	bool		reqCancel;
	bool		reqSetLoadGroup;
	bool		reqGetLoadGroup;	
} Element;


