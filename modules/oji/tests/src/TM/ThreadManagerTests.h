



































#ifdef XP_WIN
#include <windows.h>
#endif
#include "ojiapitests.h"

#include <nsIThreadManager.h>
#include <nsIJVMManager.h>

static NS_DEFINE_IID(kIThreadManagerIID, NS_ITHREADMANAGER_IID);
static NS_DEFINE_IID(kJVMManagerCID, NS_JVMMANAGER_CID);

extern nsresult GetThreadManager(nsIThreadManager** tm);

class BaseDummyThread : public nsIRunnable {
protected:
	nsIThreadManager *tm;
public:
	nsresult rc;	
	NS_METHOD QueryInterface(const nsIID & uuid, void * *result) { return NS_OK; }
	NS_METHOD_(nsrefcnt) AddRef(void) { return NS_OK; }
	NS_METHOD_(nsrefcnt) Release(void) { return NS_OK; }
	NS_IMETHOD Run() = 0;
};
