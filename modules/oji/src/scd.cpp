








































#include "nsJVMManager.h"

NS_IMPL_AGGREGATED(nsSymantecDebugManager)

nsSymantecDebugManager::nsSymantecDebugManager(nsISupports* outer, nsJVMManager* jvmMgr)
    : fJVMMgr(jvmMgr)
{
    NS_INIT_AGGREGATED(outer);
}

nsSymantecDebugManager::~nsSymantecDebugManager()
{
}

NS_INTERFACE_MAP_BEGIN_AGGREGATED(nsSymantecDebugManager)
    NS_INTERFACE_MAP_ENTRY(nsISymantecDebugManager)
NS_INTERFACE_MAP_END

NS_METHOD
nsSymantecDebugManager::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr, 
                               nsJVMManager* jvmMgr)
{
    if (!aInstancePtr)
        return NS_ERROR_INVALID_POINTER;
    NS_ENSURE_PROPER_AGGREGATION(outer, aIID);

    nsSymantecDebugManager* dbgr = new nsSymantecDebugManager(outer, jvmMgr);
    if (dbgr == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    nsISupports* inner = dbgr->InnerObject();
    nsresult rv = inner->QueryInterface(aIID, aInstancePtr);
    if (NS_FAILED(rv)) {
       delete dbgr;
    }
    return rv;
}

#if defined(XP_WIN) && defined(_WIN32)
extern "C" HWND FindNavigatorHiddenWindow(void);
#endif

NS_METHOD
nsSymantecDebugManager::SetDebugAgentPassword(PRInt32 pwd)
{
#if defined(XP_WIN) && defined(_WIN32)
    HWND win = NULL;
    



    HANDLE sem;
    long err;

    
    err = SetWindowLong(win, 0, pwd);	
    if (err == 0) {


        
    }
    sem = OpenSemaphore(SEMAPHORE_MODIFY_STATE, FALSE, "Netscape-Symantec Debugger");
    if (sem) {
        ReleaseSemaphore(sem, 1, NULL);
        CloseHandle(sem);
    }
    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}


