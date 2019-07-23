




































#include "prlog.h"
#include "nsJVMManager.h"
#include "nsIServiceManager.h"
#include "nsIJVMPrefsWindow.h"
#include "ProxyJNI.h"
#include "lcglue.h"
#include "nsCSecurityContext.h"
#include "nsIJSContextStack.h"

static NS_DEFINE_CID(kJVMManagerCID, NS_JVMMANAGER_CID);
static NS_DEFINE_IID(kIJVMConsoleIID, NS_IJVMCONSOLE_IID);
static NS_DEFINE_IID(kIJVMPrefsWindowIID, NS_IJVMPREFSWINDOW_IID);
static NS_DEFINE_IID(kISymantecDebuggerIID, NS_ISYMANTECDEBUGGER_IID);

PR_BEGIN_EXTERN_C

#ifdef PRE_SERVICE_MANAGER
extern nsPluginManager* thePluginManager;
#endif

PR_IMPLEMENT(void)
JVM_ReleaseJVMMgr(nsJVMManager* mgr)
{
    mgr->Release();
}

static nsIJVMPlugin*
GetRunningJVM(void)
{
    nsIJVMPlugin* jvm = NULL;
    nsresult rv;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
    if (NS_FAILED(rv)) return jvm;
    nsJVMManager* jvmMgr = (nsJVMManager *)managerService.get();  
    if (jvmMgr) {
        nsJVMStatus status = jvmMgr->GetJVMStatus();
        if (status == nsJVMStatus_Enabled)
            status = jvmMgr->StartupJVM();
        if (status == nsJVMStatus_Running) {
            jvm = jvmMgr->GetJVMPlugin();
        }
    }
    return jvm;
}

PR_IMPLEMENT(nsJVMStatus)
JVM_StartupJVM(void)
{
    GetRunningJVM();
    return JVM_GetJVMStatus();
}

PR_IMPLEMENT(nsJVMStatus)
JVM_ShutdownJVM(void)
{
    nsJVMStatus status = nsJVMStatus_Failed;
    nsresult rv;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
    if (NS_FAILED(rv)) return status;
    nsJVMManager* mgr = (nsJVMManager *)managerService.get();  
    if (mgr) {
        status = mgr->ShutdownJVM();
    }
    return status;
}


PR_IMPLEMENT(nsJVMStatus)
JVM_GetJVMStatus(void)
{
    nsresult rv;
    nsJVMStatus status = nsJVMStatus_Disabled;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
    if (NS_FAILED(rv)) return status;
    nsJVMManager* mgr = (nsJVMManager *)managerService.get();  
    if (mgr) {
        status = mgr->GetJVMStatus();
    }
    return status;
}

PR_IMPLEMENT(PRBool)
JVM_AddToClassPath(const char* dirPath)
{
    nsresult err = NS_ERROR_FAILURE;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &err);
    if (NS_FAILED(err)) return PR_FALSE;
    nsJVMManager* mgr = (nsJVMManager *)managerService.get();
    if (mgr) {
        err = mgr->AddToClassPath(dirPath);
    }
    return err == NS_OK;
}





static nsIJVMConsole*
GetConsole(void)
{
    
    
    JNIEnv* env = JVM_GetJNIEnv();
    if (!env)
        return nsnull;
    
    nsIJVMConsole* console = nsnull;
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm)
        jvm->QueryInterface(kIJVMConsoleIID, (void**)&console);
    return console;
}

PR_IMPLEMENT(void)
JVM_ShowConsole(void)
{
    nsIJVMConsole* console = GetConsole();
    if (console) {
        console->Show();
        console->Release();
    }
}

PR_IMPLEMENT(void)
JVM_HideConsole(void)
{
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status == nsJVMStatus_Running) {
        nsIJVMConsole* console = GetConsole();
        if (console) {
            console->Hide();
            console->Release();
        }
    }
}

PR_IMPLEMENT(PRBool)
JVM_IsConsoleVisible(void)
{
    PRBool result = PR_FALSE;
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status == nsJVMStatus_Running) {
        nsIJVMConsole* console = GetConsole();
        if (console) {
            nsresult err = console->IsVisible(&result);
            PR_ASSERT(err != NS_OK ? result == PR_FALSE : PR_TRUE);
            console->Release();
        }
    }
    return result;
}

PR_IMPLEMENT(void)
JVM_PrintToConsole(const char* msg)
{
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status != nsJVMStatus_Running)
        return;
    nsIJVMConsole* console = GetConsole();
    if (console) {
        console->Print(msg);
        console->Release();
    }
}





static nsIJVMPrefsWindow*
GetPrefsWindow(void)
{
    nsIJVMPrefsWindow* prefsWin = NULL;
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        jvm->QueryInterface(kIJVMPrefsWindowIID, (void**)&prefsWin);
        
    }
    return prefsWin;
}

PR_IMPLEMENT(void)
JVM_ShowPrefsWindow(void)
{
    nsIJVMPrefsWindow* prefsWin = GetPrefsWindow();
    if (prefsWin) {
        prefsWin->Show();
        prefsWin->Release();
    }
}

PR_IMPLEMENT(void)
JVM_HidePrefsWindow(void)
{
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status == nsJVMStatus_Running) {
        nsIJVMPrefsWindow* prefsWin = GetPrefsWindow();
        if (prefsWin) {
            prefsWin->Hide();
            prefsWin->Release();
        }
    }
}

PR_IMPLEMENT(PRBool)
JVM_IsPrefsWindowVisible(void)
{
    PRBool result = PR_FALSE;
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status == nsJVMStatus_Running) {
        nsIJVMPrefsWindow* prefsWin = GetPrefsWindow();
        if (prefsWin) {
            nsresult err = prefsWin->IsVisible(&result);
            PR_ASSERT(err != NS_OK ? result == PR_FALSE : PR_TRUE);
            prefsWin->Release();
        }
    }
    return result;
}



PR_IMPLEMENT(void)
JVM_StartDebugger(void)
{
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        nsISymantecDebugger* debugger;
        if (jvm->QueryInterface(kISymantecDebuggerIID, (void**)&debugger) == NS_OK) {
            
            debugger->StartDebugger(nsSymantecDebugPort_SharedMemory);
            debugger->Release();
        }
        
    }
}


PR_IMPLEMENT(JNIEnv*)
JVM_GetJNIEnv(void)
{
	
	JVMContext* context = GetJVMContext();
    JNIEnv* env = context->proxyEnv;
	if (env != NULL)
		return env;

	
    nsIJVMPlugin* jvmPlugin = GetRunningJVM();
	if (jvmPlugin != NULL)
		env = CreateProxyJNI(jvmPlugin);

	
	context->proxyEnv = env;

    return env;
}

PR_IMPLEMENT(void)
JVM_ReleaseJNIEnv(JNIEnv* env)
{
	



}

PR_IMPLEMENT(nsresult)
JVM_SpendTime(PRUint32 timeMillis)
{
#ifdef XP_MAC
	nsresult result = NS_ERROR_NOT_INITIALIZED;
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm != NULL)
		result = jvm->SpendTime(timeMillis);
	return result;
#else
	return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

PR_IMPLEMENT(PRBool)
JVM_MaybeStartupLiveConnect()
{
    PRBool result = PR_FALSE;
    nsresult rv;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
    if (NS_FAILED(rv)) return result;
    nsJVMManager* mgr = (nsJVMManager *)managerService.get();  
    if (mgr) {
        result = mgr->MaybeStartupLiveConnect();
    }
    return result;
}


PR_IMPLEMENT(PRBool)
JVM_MaybeShutdownLiveConnect(void)
{
    PRBool result = PR_FALSE;
    nsresult rv;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
    if (NS_FAILED(rv)) return result;
    nsJVMManager* mgr = (nsJVMManager *)managerService.get(); 
    if (mgr) {
        result = mgr->MaybeShutdownLiveConnect();
    }
    return result;
}

PR_IMPLEMENT(PRBool)
JVM_IsLiveConnectEnabled(void)
{
    PRBool result = PR_FALSE;
    nsresult rv;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
    if (NS_FAILED(rv)) return result;
    nsJVMManager* mgr = (nsJVMManager *)managerService.get();
    if (mgr) {
        result = mgr->IsLiveConnectEnabled();
    }
    return result;
}


PR_IMPLEMENT(nsISecurityContext*) 
JVM_GetJSSecurityContext()
{
    JSContext *cx = nsnull;
    nsCOMPtr<nsIJSContextStack> stack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
    if (stack) stack->Peek(&cx);

    nsCSecurityContext *securityContext = new nsCSecurityContext(cx);
    if (securityContext == nsnull) {
        return nsnull;
    }

    NS_ADDREF(securityContext);
    return securityContext;
}

PR_END_EXTERN_C


