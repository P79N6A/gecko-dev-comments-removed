



































#include <JNIEnvTests.h>
#include <nsIServiceManager.h>

nsresult GetJNI(JNIEnv** env) {
	nsIJVMManager *jvmMngr = nsnull;
	nsresult rv = NS_OK;
	*env = nsnull;
	rv = CallGetService(kJVMManagerCID, &jvmMngr);
	if (rv != NS_OK || !jvmMngr) {
		fprintf(stderr, "ERROR: Can't get JVM manager !\n");
		return NS_ERROR_FAILURE;
	}
	if (NS_SUCCEEDED(jvmMngr->GetProxyJNI(env)) && *env)
		return NS_OK;
	fprintf(stderr, "ERROR: Can't get JNI env !\n");
	return NS_ERROR_FAILURE;
}
 
