












































#pragma once

#include "nsIPlugin.h"
#include "nsIJVMPlugin.h"
#include "nsIThreadManager.h"
#include "nsIPluginInstance.h"
#include "nsIJVMPluginInstance.h"
#include "nsIEventHandler.h"
#include "nsIPluginStreamListener.h"
#include "SupportsMixin.h"

class MRJPlugin;
class MRJPluginInstance;
class MRJSession;
class MRJContext;
class MRJConsole;

class nsIJVMManager;

class MRJPlugin :	public nsIPlugin, public nsIJVMPlugin,
					public nsIRunnable, public SupportsMixin {
public:
	MRJPlugin();
	virtual ~MRJPlugin();
	
    static nsresult GetService(const nsCID &aCID, const nsIID& aIID, void* *aService);
    static nsresult GetService(const char* aContractID, const nsIID& aIID, void* *aService);

	static const char* PLUGIN_VERSION;
	
	
	void operator delete(void* ptr) {}

	
	NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
	NS_IMETHOD_(nsrefcnt) AddRef(void) { return addRef(); }
	NS_IMETHOD_(nsrefcnt) Release(void) { return release(); }
	
	

	
	
	
	
	
	
	
	
	
	
	

	
	
	NS_IMETHOD
	CreateInstance(nsISupports *aOuter, const nsIID& aIID, void **aResult);

	NS_IMETHOD
	LockFactory(PRBool aLock) { return NS_ERROR_NOT_IMPLEMENTED; }

	
	
	




    NS_IMETHOD CreatePluginInstance(nsISupports *aOuter, REFNSIID aIID, 
                                    const char* aPluginMIMEType,
                                    void **aResult);

    






    NS_IMETHOD
    Initialize(void);

    







    NS_IMETHOD
    Shutdown(void);

    











    NS_IMETHOD
    GetMIMEDescription(const char* *result);

    








    NS_IMETHOD
    GetValue(nsPluginVariable variable, void *value);

    
    NS_IMETHOD
    SetValue(nsPluginVariable variable, void *value);

	

    
    
    
    
    
    NS_IMETHOD
    StartupJVM(void);

    
    
    
    
    NS_IMETHOD
    ShutdownJVM(PRBool fullShutdown);

    
    
    NS_IMETHOD
    AddToClassPath(const char* dirPath);

    
    
    NS_IMETHOD
    RemoveFromClassPath(const char* dirPath)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
    }

    
    NS_IMETHOD
    GetClassPath(const char* *result);

    NS_IMETHOD
    GetJavaWrapper(JNIEnv* env, jint jsobj, jobject *jobj);

    NS_IMETHOD
    GetJavaVM(JavaVM* *result);

	

    
    
    NS_IMETHOD_(nsrefcnt)
    GetJNIEnv(JNIEnv* *result);

    
    
    NS_IMETHOD_(nsrefcnt)
    ReleaseJNIEnv(JNIEnv* env);

	






	NS_IMETHOD
	CreateSecureEnv(JNIEnv* proxyEnv, nsISecureEnv* *outSecureEnv);

	



	NS_IMETHOD
	SpendTime(PRUint32 timeMillis);
	
	


	NS_IMETHOD
	Run();
	
	

	MRJSession* getSession();
	nsIJVMManager* getManager();
	nsIThreadManager* getThreadManager();

	MRJPluginInstance* getPluginInstance(jobject applet);
    MRJPluginInstance* getPluginInstance(JNIEnv* jenv);
    
    Boolean inPluginThread();
	
	NS_IMETHOD
	UnwrapJavaWrapper(JNIEnv* jenv, jobject jobj, jint* obj);

private:
	nsIJVMManager* mManager;
	nsIThreadManager* mThreadManager;
	MRJSession* mSession;
    MRJConsole* mConsole;
    nsPluginThread *mPluginThreadID;
	Boolean mIsEnabled;
	
	
	static const InterfaceInfo sInterfaces[];
	static const UInt32 kInterfaceCount;
};

class MRJPluginInstance :	public nsIPluginInstance, public nsIJVMPluginInstance,
							public nsIEventHandler, public nsIPluginStreamListener,
							private SupportsMixin {
public:
	MRJPluginInstance(MRJPlugin* plugin);
	virtual ~MRJPluginInstance();

	
	DECL_SUPPORTS_MIXIN

    
    NS_IMETHOD
    HandleEvent(nsPluginEvent* event, PRBool* handled);

    






    NS_IMETHOD
    Initialize(nsIPluginInstancePeer* peer);

    








    NS_IMETHOD
    GetPeer(nsIPluginInstancePeer* *result);

    







    NS_IMETHOD
    Start(void);

    







	NS_IMETHOD
	Stop(void);

    







	NS_IMETHOD
	Destroy(void);

    







	NS_IMETHOD
	SetWindow(nsPluginWindow* window);

    








    NS_IMETHOD
    NewStream(nsIPluginStreamListener** listener)
	{
		*listener = this;
		AddRef();
		return NS_OK;
	}

    







    NS_IMETHOD
    Print(nsPluginPrint* platformPrint);

    






    NS_IMETHOD
    GetValue(nsPluginInstanceVariable variable, void *value);

	

    
    
    NS_IMETHOD
    GetJavaObject(jobject *result);

    NS_IMETHOD
    GetText(const char* *result)
    {
        *result = NULL;
    	return NS_OK;
    }

	
	
    






    NS_IMETHOD
    OnStartBinding(nsIPluginStreamInfo* pluginInfo)
    {
    	return NS_OK;
    }

    









    NS_IMETHOD
    OnDataAvailable(nsIPluginStreamInfo* pluginInfo, nsIInputStream* input, PRUint32 length);

    NS_IMETHOD
    OnFileAvailable(nsIPluginStreamInfo* pluginInfo, const char* fileName)
    {
		return NS_ERROR_NOT_IMPLEMENTED;
	}
	
    










    NS_IMETHOD
    OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status)
    {
    	return NS_OK;
    }

	


    NS_IMETHOD
    GetStreamType(nsPluginStreamType *result)
    {
    	*result = nsPluginStreamType_Normal;
    	return NS_OK;
    }

    
    static MRJPluginInstance* getInstances(void);
    MRJPluginInstance* getNextInstance(void);
    
    MRJContext* getContext(void);
    MRJSession* getSession(void);

private:
	void pushInstance(void);
	void popInstance(void);
	void inspectInstance(Boolean isUpdateEvt);

private:
    nsIPluginInstancePeer* mPeer;
    nsIWindowlessPluginInstancePeer* mWindowlessPeer;
    MRJPlugin* mPlugin;
    MRJSession* mSession;
    MRJContext* mContext;
    jobject mApplet;
    nsPluginWindow* mPluginWindow;
    
    
    MRJPluginInstance* mNext;

	
	static const InterfaceInfo sInterfaces[];
	static const UInt32 kInterfaceCount;
};
