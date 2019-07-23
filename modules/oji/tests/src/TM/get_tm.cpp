



































#include <ThreadManagerTests.h>

#include <nsIServiceManager.h>

nsresult GetThreadManager(nsIThreadManager** thrdMngr) {
	*thrdMngr = nsnull; 
	nsresult rv;
	rv = CallGetService(kJVMManagerCID, thrdMngr);
	if (rv != NS_OK || !thrdMngr) {
		fprintf(stderr, "ERROR: Can't get Thread manager !\n");
		return NS_ERROR_FAILURE;
	}
	return NS_OK;
}
 
