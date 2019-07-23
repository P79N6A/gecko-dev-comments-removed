



































#include <ojiapitests.h> 

#include <LiveConnectManagerTests.h>

nsresult GetLiveConnectManager(nsILiveConnectManager** lcMngr) {
	*lcMngr = nsnull;
	nsresult rv = NS_OK;
	rv = CallGetService(kJVMManagerCID, lcMngr);
	if (rv != NS_OK || !lcMngr) {
		fprintf(stderr, "ERROR: Can't get LiveConnect manager !\n");
		return NS_ERROR_FAILURE;
	}
	return NS_OK;
} 
