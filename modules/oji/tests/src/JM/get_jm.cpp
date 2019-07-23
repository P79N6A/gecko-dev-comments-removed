



































 
#include <JVMManagerTests.h>
#include <nsIServiceManager.h>

nsresult GetJVMManager(nsIJVMManager** jvmMngr) {
	*jvmMngr;
	nsresult rv = CallGetService(kJVMManagerCID, jvmMngr);
	if (rv != NS_OK || !jvmMngr) {
		fprintf(stderr, "ERROR: Can't get JVM manager !\n");
		return NS_ERROR_FAILURE;
	}
	return NS_OK;
}
