




































#ifndef nsJVMManager_h___
#define nsJVMManager_h___

#include "jvmmgr.h"
#include "prtypes.h"
#include "nscore.h"
#include "jni.h"
#include "jsdbgapi.h"
#include "nsError.h"

#include "nsjvm.h"
#include "nsAgg.h"
#include "jsjava.h"
#include "nsVoidArray.h"
#include "nsILiveConnectManager.h"

#include "nsIObserver.h"

class nsSymantecDebugManager;
class nsIWebBrowserChrome;







struct nsJVMManager : public nsIJVMManager, public nsIJVMThreadManager,
                      public nsILiveConnectManager, public nsIObserver {
public:

    NS_DECL_AGGREGATED

    NS_DECL_NSIJVMMANAGER    

    NS_DECL_NSIOBSERVER

    
    
	


	NS_IMETHOD
	GetCurrentThread(PRThread* *threadID);

	




	NS_IMETHOD
	Sleep(PRUint32 milli = 0);

	



	NS_IMETHOD
	EnterMonitor(void* address);
	
	


	NS_IMETHOD
	ExitMonitor(void* address);
	
	



	NS_IMETHOD
	Wait(void* address, PRUint32 milli = 0);

	


	NS_IMETHOD
	Notify(void* address);

	


	NS_IMETHOD
	NotifyAll(void* address);
	
	


	NS_IMETHOD
	CreateThread(PRThread **thread, nsIRunnable* runnable);
	
	





	NS_IMETHOD
	PostEvent(PRThread *thread, nsIRunnable* runnable, PRBool async);

	

	


	NS_IMETHOD
    StartupLiveConnect(JSRuntime* runtime, PRBool& outStarted)
    {
    	outStarted = MaybeStartupLiveConnect();
    	return NS_OK;
    }
    
	


	NS_IMETHOD
    ShutdownLiveConnect(JSRuntime* runtime, PRBool& outShutdown)
    {
    	outShutdown = MaybeShutdownLiveConnect();
    	return NS_OK;
    }

	


	NS_IMETHOD
    IsLiveConnectEnabled(PRBool& outEnabled)
    {
    	outEnabled = IsLiveConnectEnabled();
    	return NS_OK;
    }

    


	NS_IMETHOD
    InitLiveConnectClasses(JSContext* context, JSObject* globalObject);

    


	NS_IMETHOD
	WrapJavaObject(JSContext* context, jobject javaObject, JSObject* *outJSObject);

    

    
    NS_IMETHOD
    GetClasspathAdditions(const char* *result);

    nsIJVMPlugin* GetJVMPlugin(void) { return fJVM; }

    

    nsJVMStatus StartupJVM(void);
    nsJVMStatus ShutdownJVM(PRBool fullShutdown = PR_FALSE);
    nsJVMStatus GetJVMStatus(void);
    void SetJVMEnabled(PRBool enabled);

#if 0    
    void        ReportJVMError(nsresult err);
    const char* GetJavaErrorString(JNIEnv* env);
#endif

    nsresult    AddToClassPath(const char* dirPath);
    PRBool      MaybeStartupLiveConnect(void);
    PRBool      MaybeShutdownLiveConnect(void);
    PRBool      IsLiveConnectEnabled(void);
    JSJavaVM*   GetJSJavaVM(void) { return fJSJavaVM; }


    nsJVMManager(nsISupports* outer);
    virtual ~nsJVMManager(void);

protected:    

    






    nsresult    GetChrome(nsIWebBrowserChrome **theChrome);
    const char* GetJavaErrorString(JRIEnv* env);

    nsIJVMPlugin*       fJVM;
    nsJVMStatus         fStatus;
    nsISupports*        fDebugManager;
    JSJavaVM *          fJSJavaVM;  
    nsVoidArray*        fClassPathAdditions;
    char*               fClassPathAdditionsString;
    PRBool              fStartupMessagePosted;
};





class nsSymantecDebugManager : public nsISymantecDebugManager {
public:

    NS_DECL_AGGREGATED

    NS_IMETHOD
    SetDebugAgentPassword(PRInt32 pwd);

    static NS_METHOD
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr,
           nsJVMManager* nsJVMManager);

protected:
    nsSymantecDebugManager(nsISupports* outer, nsJVMManager* nsJVMManager);
    virtual ~nsSymantecDebugManager(void);

    nsJVMManager* fJVMMgr;

};



#endif 
