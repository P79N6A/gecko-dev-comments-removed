





































#ifndef nsDocLoadObserver_h_
#define nsDocLoadObserver_h_

#include "nsIObserver.h"

#include <MacTypes.h>
#include "nsVoidArray.h"
















class nsDocLoadObserver : public nsIObserver
{
public:

	          nsDocLoadObserver();
	virtual 	~nsDocLoadObserver();
	
	NS_DECL_ISUPPORTS
	NS_DECL_NSIOBSERVER

	
	void AddEchoRequester(OSType appSignature);
	void RemoveEchoRequester(OSType appSignature);
	
protected:

	
	
	
	void Register();
	void Unregister();

private:
	
	PRBool mRegistered;

	nsVoidArray		mEchoRequesters;
};

#endif 
