














































#include "npapi.h"
#include "nsIPluginManager2.h"
#include "nsIServiceManager.h"
#include "nsMemory.h"
#include "nsLiveConnect.h"
#include "nsIEventHandler.h"
#include "nsplugin.h"
#include "nsDebug.h"
#include "nsIJRILiveConnectPlugin.h"
#include "nsIJRILiveConnectPIPeer.h"
#include "nsIPluginInputStream.h"
#include "nsObsoleteModuleLoading.h"

#ifdef XP_MAC
#include "EventFilter.h"
#include <MacWindows.h>
#endif












#pragma mark CPluginManager

class CPluginManager : public nsIPluginManager2, public nsIServiceManager, public nsIMemory {
public:
	
	void* operator new(size_t size) { return ::NPN_MemAlloc(size); }
	void operator delete(void* ptr) { ::NPN_MemFree(ptr); }

    CPluginManager(void);
    virtual ~CPluginManager(void);

    NS_DECL_ISUPPORTS

    
    

    








    NS_IMETHOD
    GetValue(nsPluginManagerVariable variable, void *value);

    









    NS_IMETHOD
    ReloadPlugins(PRBool reloadPages);

    







    NS_IMETHOD
    UserAgent(const char* *resultingAgentString);

    





















    NS_IMETHOD
    GetURL(nsISupports* pluginInst, 
           const char* url, 
           const char* target = NULL,
           nsIPluginStreamListener* streamListener = NULL,
           const char* altHost = NULL,
           const char* referrer = NULL,
           PRBool forceJSEnabled = PR_FALSE);

    





























    NS_IMETHOD
    PostURL(nsISupports* pluginInst,
            const char* url,
            PRUint32 postDataLen, 
            const char* postData,
            PRBool isFile = PR_FALSE,
            const char* target = NULL,
            nsIPluginStreamListener* streamListener = NULL,
            const char* altHost = NULL, 
            const char* referrer = NULL,
            PRBool forceJSEnabled = PR_FALSE,
            PRUint32 postHeadersLength = 0, 
            const char* postHeaders = NULL);
            
    NS_IMETHOD
    RegisterPlugin(REFNSIID aCID,
                   const char* aPluginName,
                   const char* aDescription,
                   const char** aMimeTypes,
                   const char** aMimeDescriptions,
                   const char** aFileExtensions,
                   PRInt32 aCount);

    NS_IMETHOD
    UnregisterPlugin(REFNSIID aCID);

    NS_IMETHOD
    GetURLWithHeaders(nsISupports* pluginInst, 
                      const char* url, 
                      const char* target = NULL,
                      nsIPluginStreamListener* streamListener = NULL,
                      const char* altHost = NULL,
                      const char* referrer = NULL,
                      PRBool forceJSEnabled = PR_FALSE,
                      PRUint32 getHeadersLength = 0, 
                      const char* getHeaders = NULL);
    

    
    

    




    NS_IMETHOD
    BeginWaitCursor(void)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

    




    NS_IMETHOD
    EndWaitCursor(void)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

    






    NS_IMETHOD
    SupportsURLProtocol(const char* protocol, PRBool *result)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

    









    NS_IMETHOD
    NotifyStatusChange(nsIPlugin* plugin, nsresult errorStatus)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}
    
    












    NS_IMETHOD
    FindProxyForURL(const char* url, char* *result)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

    
    
    
    







    NS_IMETHOD
    RegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);
    
    







    NS_IMETHOD
    UnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);

	







    NS_IMETHOD
	AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu, PRInt16 *result);

	






    NS_IMETHOD
	DeallocateMenuID(nsIEventHandler* handler, PRInt16 menuID)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

	







    NS_IMETHOD
    HasAllocatedMenuID(nsIEventHandler* handler, PRInt16 menuID, PRBool *result)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

    
    

    NS_IMETHOD
    GetService(const nsCID& aClass, const nsIID& aIID, void* *result);

    NS_IMETHOD
    GetServiceByContractID(const char *aContractID, const nsIID & aIID, void * *result)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

    NS_IMETHOD
    IsServiceInstantiated(const nsCID & aClass, const nsIID & aIID, PRBool *_retval)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

    NS_IMETHOD
    IsServiceInstantiatedByContractID(const char *aContractID, const nsIID & aIID, PRBool *_retval)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
	}

   





    NS_IMETHOD_(void*)
    Alloc(size_t size);

    






    NS_IMETHOD_(void*)
    Realloc(void* ptr, size_t size);

    




    NS_IMETHOD_(void)
    Free(void* ptr);

    NS_IMETHOD
    IsLowMemory(PRBool *_retval)
    {
    	return NS_ERROR_NOT_IMPLEMENTED;
    }

    


    NS_IMETHOD
    HeapMinimize(PRBool aImmediate);

private:
	nsILiveconnect* mLiveconnect;

	struct RegisteredWindow {
		RegisteredWindow* mNext;
		nsIEventHandler* mHandler;
		nsPluginPlatformWindowRef mWindow;

		RegisteredWindow(RegisteredWindow* next, nsIEventHandler* handler, nsPluginPlatformWindowRef window)
			: mNext(next), mHandler(handler), mWindow(window)
		{
			NS_ADDREF(mHandler);
		}
		
		~RegisteredWindow()
		{
			NS_RELEASE(mHandler);
		}
	};

	static RegisteredWindow* theRegisteredWindows;
	static RegisteredWindow* theActiveWindow;
	
	static RegisteredWindow** GetRegisteredWindow(nsPluginPlatformWindowRef window);
	static RegisteredWindow* FindRegisteredWindow(nsPluginPlatformWindowRef window);

#ifdef XP_MAC
	Boolean mEventFiltersInstalled;

	static Boolean EventFilter(EventRecord* event);
	static Boolean MenuFilter(long menuSelection);
#endif
};








#pragma mark CPluginManagerStream

class CPluginManagerStream : public nsIOutputStream {
public:

    CPluginManagerStream(NPP npp, NPStream* pstr);
    virtual ~CPluginManagerStream(void);

    NS_DECL_ISUPPORTS

    
    
    
    
    
    




   
    NS_IMETHOD
    Write(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount); 

    












    NS_IMETHOD
    Write(nsIInputStream* fromStream, PRUint32 *aWriteCount);

    


    NS_IMETHOD
    Flush(void);

    NS_IMETHOD
    WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval) {
        NS_NOTREACHED("WriteFrom");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval) {
        NS_NOTREACHED("WriteSegments");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    IsNonBlocking(PRBool *aNonBlocking) {
        NS_NOTREACHED("IsNonBlocking");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    
    
    
    
    
    
    NS_IMETHOD
    GetURL(const char*  *result);

    
    NS_IMETHOD
    GetEnd(PRUint32 *result);

    
    NS_IMETHOD
    GetLastModified(PRUint32 *result);

    
    NS_IMETHOD
    GetNotifyData(void*  *result);

    
    NS_IMETHOD Close(void);

protected:

    
    
    NPP npp;

    
    
    NPStream* pstream;

};









#pragma mark CPluginInstancePeer

class CPluginInstancePeer : public nsIPluginInstancePeer, public nsIPluginTagInfo, public nsIJRILiveConnectPluginInstancePeer {
public:

    
    
    
    CPluginInstancePeer(nsIPluginInstance* pluginInstance, NPP npp, nsMIMEType typeString, nsPluginMode type,
        PRUint16 attribute_cnt, const char** attribute_list, const char** values_list);

    virtual ~CPluginInstancePeer(void);

    NS_DECL_ISUPPORTS

    








    NS_IMETHOD
    GetValue(nsPluginInstancePeerVariable variable, void *value);

    
    NS_IMETHOD
    SetValue(nsPluginInstancePeerVariable variable, void *value);

    







    NS_IMETHOD
    GetMIMEType(nsMIMEType *result);

    








    NS_IMETHOD
    GetMode(nsPluginMode *result);

    












    NS_IMETHOD
    NewStream(nsMIMEType type, const char* target, nsIOutputStream* *result);

    








    NS_IMETHOD
    ShowStatus(const char* message);

    






    NS_IMETHOD
    SetWindowSize(PRUint32 width, PRUint32 height);

    
    
    
    
    NS_IMETHOD
    GetAttributes(PRUint16& n, const char* const*& names, const char* const*& values);

    
    
    NS_IMETHOD
    GetAttribute(const char* name, const char* *result);

    









    NS_IMETHOD
    GetDOMElement(nsIDOMElement* *result);

	







	NS_IMETHOD
	GetJavaEnv(JRIEnv* *resultingEnv);

    









    NS_IMETHOD
    GetJavaPeer(jref *resultingJavaPeer);

	nsIPluginInstance* GetInstance(void) { return mInstance; }
	NPP GetNPPInstance(void) { return npp; }
	
	void SetWindow(NPWindow* window) { mWindow = window; }
	NPWindow* GetWindow() { return mWindow; }
	
protected:

    NPP npp;
    
    
    nsIPluginInstance* mInstance;
    NPWindow* mWindow;
    nsMIMEType typeString;
	nsPluginMode type;
	PRUint16 attribute_cnt;
	char** attribute_list;
	char** values_list;
};

#pragma mark CPluginStreamInfo

class CPluginStreamInfo : public nsIPluginStreamInfo {
public:

	CPluginStreamInfo(const char* URL, nsIPluginInputStream* inStr, nsMIMEType type, PRBool seekable)
		 : mURL(URL), mInputStream(inStr), mMimeType(type), mIsSeekable(seekable)
	{
	}

	virtual ~CPluginStreamInfo() {}

    NS_DECL_ISUPPORTS
    
	NS_METHOD
	GetContentType(nsMIMEType* result)
	{
		*result = mMimeType;
		return NS_OK;
	}

	NS_METHOD
	IsSeekable(PRBool* result)
	{
		*result = mIsSeekable;
		return NS_OK;
	}

	NS_METHOD
	GetLength(PRUint32* result)
	{
		return mInputStream->Available(result);
	}

	NS_METHOD
	GetLastModified(PRUint32* result)
	{
		return mInputStream->GetLastModified(result);
	}

	NS_METHOD
	GetURL(const char** result)
	{
		*result = mURL;
		return NS_OK;
	}

	NS_METHOD
	RequestRead(nsByteRange* rangeList)
	{
		return mInputStream->RequestRead(rangeList);
	}

	NS_METHOD
	GetStreamOffset(PRInt32 *result)
	{
		*result = mStreamOffset;
		return NS_OK;
	}


	NS_METHOD
	SetStreamOffset(PRInt32 offset)
	{
		mStreamOffset = offset;
		return NS_OK;
	}

private:
	const char* mURL;
	nsIPluginInputStream* mInputStream;
	nsMIMEType mMimeType;
	PRBool mIsSeekable;
	PRInt32 mStreamOffset;
};

#pragma mark CPluginInputStream

class CPluginInputStream : public nsIPluginInputStream {
public:

    NS_DECL_ISUPPORTS

    
    

    
    NS_IMETHOD
    Close(void);

    





    NS_IMETHOD
    Available(PRUint32 *aLength);

    







   
    NS_IMETHOD
    Read(char* aBuf, PRUint32 aCount, PRUint32 *aReadCount); 

    NS_IMETHOD ReadSegments(nsWriteSegmentFun writer, void * closure, PRUint32 count, PRUint32 *_retval) {
        NS_NOTREACHED("ReadSegments");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD IsNonBlocking(PRBool *aNonBlocking) {
        NS_NOTREACHED("IsNonBlocking");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    
    

    
    NS_IMETHOD
    GetLastModified(PRUint32 *result);

    NS_IMETHOD
    RequestRead(nsByteRange* rangeList);

    
    

    CPluginInputStream(nsIPluginStreamListener* listener);
    virtual ~CPluginInputStream(void);

    void SetStreamInfo(NPP npp, NPStream* stream) {
        mNPP = npp;
        mStream = stream;
    }

    nsIPluginStreamListener* GetListener(void) { return mListener; }
    nsPluginStreamType GetStreamType(void) { return mStreamType; }

    nsresult SetReadBuffer(PRUint32 len, const char* buffer) {
        
        if (mBuffer != NULL) delete[] mBuffer;
        mBuffer = dup(len, buffer);
        mBufferLength = len;
        mAmountRead = 0;
        return NS_OK;
    }
    
    static char* dup(PRUint32 len, const char* buffer) {
    	char* result = new char[len];
    	if (result != NULL)
    	    memcpy(result, buffer, len);
    	return result;
    }

	nsIPluginStreamInfo* CreatePluginStreamInfo(const char* url, nsMIMEType type, PRBool seekable) {
		if (mStreamInfo == NULL) {
			mStreamInfo = new CPluginStreamInfo(url, this, type, seekable);
			NS_IF_ADDREF(mStreamInfo);
		}
		return mStreamInfo;
	}
	
	nsIPluginStreamInfo* GetPluginStreamInfo() {
		return mStreamInfo;
	}

protected:
    const char* mURL;
    nsIPluginStreamListener* mListener;
    nsPluginStreamType mStreamType;
    NPP mNPP;
    NPStream* mStream;
    char* mBuffer;
    PRUint32 mBufferLength;
    PRUint32 mAmountRead;
    nsIPluginStreamInfo* mStreamInfo;
};



#ifdef XP_UNIX
#define TRACE(foo) trace(foo)
#endif

#ifdef XP_MAC
#undef assert
#define assert(cond)
#endif









#if defined(XP_UNIX) || defined(XP_MAC)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <windows.h>
#endif




#pragma mark Globals






#pragma export on
nsIPluginManager* thePluginManager = NULL;
nsIPlugin* thePlugin = NULL;
#pragma export off




#pragma mark IIDs

static NS_DEFINE_CID(kPluginCID, NS_PLUGIN_CID);
static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);
static NS_DEFINE_CID(kMemoryCID, NS_MEMORY_CID);


nsresult fromNPError[] = {
    NS_OK,                          
    NS_ERROR_FAILURE,               
    NS_ERROR_FAILURE,               
    NS_ERROR_NOT_INITIALIZED,       
    NS_ERROR_FACTORY_NOT_LOADED,    
    NS_ERROR_OUT_OF_MEMORY,         
    NS_NOINTERFACE,                 
    NS_ERROR_ILLEGAL_VALUE,         
    NS_NOINTERFACE,                 
    NS_ERROR_ILLEGAL_VALUE,         
    NS_ERROR_ILLEGAL_VALUE,         
    NS_ERROR_ILLEGAL_VALUE,         
    NS_ERROR_FAILURE,               
    NS_ERROR_FAILURE                
};






















#ifdef XP_UNIX
char* NPP_GetMIMEDescription(void)
{
    int freeFac = 0;
    
    if (thePlugin == NULL) {
        freeFac = 1;
        NSGetFactory(thePluginManager, kPluginCID, NULL, NULL, (nsIFactory** )&thePlugin);
    }
    
    const char * ret;
    nsresult err = thePlugin->GetMIMEDescription(&ret);
    if (err) return NULL;
    
    if (freeFac) {
        
        thePlugin->Release();
        thePlugin = NULL;
    }
    
    return (char*)ret;
}










NPError
NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
    return NPERR_GENERIC_ERROR; 
}





NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
    int freeFac = 0;
    
    if (thePlugin == NULL) {
        freeFac = 1;
        if (NSGetFactory(thePluginManager, kPluginCID, NULL, NULL, (nsIFactory** )&thePlugin) != NS_OK)
            return NPERR_GENERIC_ERROR;
    }
    
    nsresult err = thePlugin->GetValue((nsPluginVariable)variable, value);
    if (err) return NPERR_GENERIC_ERROR;
    
    if (freeFac) {
        
        thePlugin->Release();
        thePlugin = NULL;
    }
    
    return NPERR_NO_ERROR;
}
#endif 









NPError
NPP_Initialize(void)
{


    
    
    
    if (thePluginManager == NULL) {
        
        thePluginManager = new CPluginManager();	
        if ( thePluginManager == NULL ) 
            return NPERR_OUT_OF_MEMORY_ERROR;  
        thePluginManager->AddRef();
    }
    NPError error = NPERR_INVALID_PLUGIN_ERROR;  
    
    if (thePlugin == NULL) {
        
        nsresult result = NSGetFactory(thePluginManager, kPluginCID, NULL, NULL, (nsIFactory**)&thePlugin);
        if (result == NS_OK && thePlugin->Initialize() == NS_OK)
        	error = NPERR_NO_ERROR;
      
	}
	
    return error;
}













jref
NPP_GetJavaClass(void)
{
	jref pluginClass = NULL;
	if (thePlugin != NULL) {
		nsIJRILiveConnectPlugin* jriPlugin = NULL;
		if (thePlugin->QueryInterface(NS_GET_IID(nsIJRILiveConnectPlugin), (void**)&jriPlugin) == NS_OK) {
			jriPlugin->GetJavaClass(NPN_GetJavaEnv(), &pluginClass);
			NS_RELEASE(jriPlugin);
		}
	}
	return pluginClass;
}









void
NPP_Shutdown(void)
{


    if (thePlugin)
    {
        thePlugin->Shutdown();
        thePlugin->Release();
        thePlugin = NULL;
    }

    if (thePluginManager)  {
        thePluginManager->Release();
        thePluginManager = NULL;
    }
    
    return;
}










NPError 
NPP_New(NPMIMEType pluginType,
	NPP instance,
	PRUint16 mode,
	int16 argc,
	char* argn[],
	char* argv[],
	NPSavedData* saved)
{

    
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    
    nsIPluginInstance* pluginInstance = NULL;
    thePlugin->CreatePluginInstance(thePluginManager, NS_GET_IID(nsIPluginInstance), pluginType, (void**)&pluginInstance);
    if (pluginInstance == NULL) {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }
    
    
    
    
    
    CPluginInstancePeer* peer = new CPluginInstancePeer(pluginInstance, instance,
    													(nsMIMEType)pluginType, 
						                                (nsPluginMode)mode, (PRUint16)argc,
						                                (const char** )argn, (const char** )argv);
    assert( peer != NULL );
    if (!peer) return NPERR_OUT_OF_MEMORY_ERROR;
    peer->AddRef();
    pluginInstance->Initialize(peer);
    pluginInstance->Start();
    
    instance->pdata = peer;
    peer->Release();

    return NPERR_NO_ERROR;
}









NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{

    
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
    
    CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
    nsIPluginInstance* pluginInstance = peer->GetInstance();
    pluginInstance->Stop();
    pluginInstance->Destroy();
    pluginInstance->Release();
	
    instance->pdata = NULL;
    
    return NPERR_NO_ERROR;
}






NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{

    
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
    if ( peer == NULL)
        return NPERR_INVALID_PLUGIN_ERROR;

	
	peer->SetWindow(window);

    nsIPluginInstance* pluginInstance = peer->GetInstance();
    if( pluginInstance == 0 )
        return NPERR_INVALID_PLUGIN_ERROR;

    return (NPError)pluginInstance->SetWindow((nsPluginWindow* ) window );
}









NPError 
NPP_NewStream(NPP instance,
              NPMIMEType type,
              NPStream *stream, 
              NPBool seekable,
              PRUint16 *stype)
{
    
    
    
   


    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
				
    CPluginInputStream* inStr = (CPluginInputStream*)stream->notifyData;
    if (inStr == NULL)
        return NPERR_GENERIC_ERROR;
    
    nsIPluginStreamInfo* info = inStr->CreatePluginStreamInfo(stream->url, type, seekable);
    nsresult err = inStr->GetListener()->OnStartBinding(info);
    if (err) return err;

    inStr->SetStreamInfo(instance, stream);
    stream->pdata = inStr;
    *stype = inStr->GetStreamType();

    return NPERR_NO_ERROR;
}







int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{


    if (instance == NULL)
        return -1;

    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return -1;
    return NP_MAXREADY;
}







int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{


    if (instance == NULL)
        return -1;
	
    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return -1;
    nsresult err = inStr->SetReadBuffer((PRUint32)len, (const char*)buffer);
    if (err != NS_OK) return -1;
    err = inStr->GetListener()->OnDataAvailable(inStr->GetPluginStreamInfo(), inStr, len);
    if (err != NS_OK) return -1;
    return len;
}









NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{


    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
		
    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return NPERR_GENERIC_ERROR;
    inStr->GetListener()->OnStopBinding(inStr->GetPluginStreamInfo(), (nsPluginReason)reason);
    
    stream->pdata = NULL;
	
    return NPERR_NO_ERROR;
}






void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{


    if (instance == NULL)
        return;
		
    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return;
    (void)inStr->GetListener()->OnFileAvailable(inStr->GetPluginStreamInfo(), fname);
}





void 
NPP_Print(NPP instance, NPPrint* printInfo)
{


    if(printInfo == NULL)   
        return;

    if (instance != NULL)
    {
        CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
        nsIPluginInstance* pluginInstance = peer->GetInstance();
        pluginInstance->Print((nsPluginPrint* ) printInfo );
    }
}






void
NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{


    if (instance != NULL) {
        CPluginInputStream* inStr = (CPluginInputStream*)notifyData;
        (void)inStr->GetListener()->OnStopBinding(inStr->GetPluginStreamInfo(), (nsPluginReason)reason);
        inStr->Release();
    }
}







#ifndef XP_UNIX
int16
NPP_HandleEvent(NPP instance, void* event)
{

    int16 eventHandled = FALSE;
    if (instance == NULL)
        return eventHandled;
	
    NPEvent* npEvent = (NPEvent*) event;
    nsPluginEvent pluginEvent = {
#ifdef XP_MAC
        npEvent, NULL
#else
        npEvent->event, npEvent->wParam, npEvent->lParam
#endif
    };
	
    CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
    nsIPluginInstance* pluginInstance = peer->GetInstance();
    if (pluginInstance) {
        PRBool handled;
        nsresult err = pluginInstance->HandleEvent(&pluginEvent, &handled);
        if (err) return FALSE;
        eventHandled = (handled == PR_TRUE);
    }
	
    return eventHandled;
}
#endif 


















CPluginManager::CPluginManager(void)
{
    mLiveconnect = NULL;

#ifdef XP_MAC
    mEventFiltersInstalled = false;
#endif
}

CPluginManager::~CPluginManager(void) 
{
	NS_IF_RELEASE(mLiveconnect);

#ifdef XP_MAC	
	if (mEventFiltersInstalled)
		RemoveEventFilters();
#endif
}





NS_METHOD
CPluginManager::ReloadPlugins(PRBool reloadPages)
{
    NPN_ReloadPlugins(reloadPages);
    return NS_OK;
}

NS_METHOD
CPluginManager::GetURL(nsISupports* pluginInst, 
                       const char* url, 
                       const char* target,
                       nsIPluginStreamListener* streamListener,
                       const char* altHost,
                       const char* referrer,
                       PRBool forceJSEnabled)
{
    if (altHost != NULL || referrer != NULL || forceJSEnabled != PR_FALSE) {
        return NPERR_INVALID_PARAM;
    }

    nsIPluginInstance* inst = NULL;
    nsresult rslt = pluginInst->QueryInterface(NS_GET_IID(nsIPluginInstance), (void**)&inst);
    if (rslt != NS_OK) return rslt;
	CPluginInstancePeer* instancePeer = NULL;
    rslt = inst->GetPeer((nsIPluginInstancePeer**)&instancePeer);
    if (rslt != NS_OK) {
        inst->Release();
        return rslt;
    }
    NPP npp = instancePeer->GetNPPInstance();

    NPError err;
    if (streamListener) {
        CPluginInputStream* inStr = new CPluginInputStream(streamListener);
        if (inStr == NULL) {
            instancePeer->Release();
            inst->Release();
            return NS_ERROR_OUT_OF_MEMORY;
        }
        inStr->AddRef();
    
        err = NPN_GetURLNotify(npp, url, target, inStr);
    }
    else {
        err = NPN_GetURL(npp, url, target);
    }
    instancePeer->Release();
    inst->Release();
    return fromNPError[err];
}

NS_METHOD
CPluginManager::PostURL(nsISupports* pluginInst,
                        const char* url,
                        PRUint32 postDataLen, 
                        const char* postData,
                        PRBool isFile,
                        const char* target,
                        nsIPluginStreamListener* streamListener,
                        const char* altHost, 
                        const char* referrer,
                        PRBool forceJSEnabled,
                        PRUint32 postHeadersLength, 
                        const char* postHeaders)
{
    if (altHost != NULL || referrer != NULL || forceJSEnabled != PR_FALSE
        || postHeadersLength != 0 || postHeaders != NULL) {
        return NPERR_INVALID_PARAM;
    }

    nsIPluginInstance* inst = NULL;
    nsresult rslt = pluginInst->QueryInterface(NS_GET_IID(nsIPluginInstance), (void**)&inst);
    if (rslt != NS_OK) return rslt;
	CPluginInstancePeer* instancePeer = NULL;
    rslt = inst->GetPeer((nsIPluginInstancePeer**)&instancePeer);
    if (rslt != NS_OK) {
        inst->Release();
        return rslt;
    }
    NPP npp = instancePeer->GetNPPInstance();

    NPError err;
    if (streamListener) {
        CPluginInputStream* inStr = new CPluginInputStream(streamListener);
        if (inStr == NULL) {
            instancePeer->Release();
            inst->Release();
            return NS_ERROR_OUT_OF_MEMORY;
        }
        inStr->AddRef();
    
        err = NPN_PostURLNotify(npp, url, target, postDataLen, postData, isFile, inStr);
    }
    else {
        err = NPN_PostURL(npp, url, target, postDataLen, postData, isFile);
    }
    instancePeer->Release();
    inst->Release();
    return fromNPError[err];
}

NS_IMETHODIMP
CPluginManager::RegisterPlugin(REFNSIID aCID,
                               const char* aPluginName,
                               const char* aDescription,
                               const char** aMimeTypes,
                               const char** aMimeDescriptions,
                               const char** aFileExtensions,
                               PRInt32 aCount)
{
    
    return NS_OK;
}

NS_IMETHODIMP
CPluginManager::UnregisterPlugin(REFNSIID aCID)
{
    
    return NS_OK;
}

NS_IMETHODIMP
CPluginManager::GetURLWithHeaders(nsISupports* pluginInst, 
                                  const char* url, 
                                  const char* target,
                                  nsIPluginStreamListener* streamListener,
                                  const char* altHost,
                                  const char* referrer,
                                  PRBool forceJSEnabled,
                                  PRUint32 getHeadersLength, 
                                  const char* getHeaders)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}










NS_METHOD
CPluginManager::UserAgent(const char* *result)
{
    *result = NPN_UserAgent(NULL);
    return NS_OK;
}


int varMap[] = {
    (int)NPNVxDisplay,                  
    (int)NPNVxtAppContext,              
    (int)NPNVnetscapeWindow,            
    (int)NPPVpluginWindowBool,          
    (int)NPPVpluginTransparentBool,     
    (int)NPPVjavaClass,                 
    (int)NPPVpluginWindowSize,          
    (int)NPPVpluginTimerInterval,       
};





NS_METHOD
CPluginManager::GetValue(nsPluginManagerVariable variable, void *value)
{
#ifdef XP_UNIX
    return fromNPError[NPN_GetValue(NULL, (NPNVariable)varMap[(int)variable], value)];
#else
    return fromNPError[NPERR_GENERIC_ERROR];
#endif 
}





CPluginManager::RegisteredWindow* CPluginManager::theRegisteredWindows = NULL;
CPluginManager::RegisteredWindow* CPluginManager::theActiveWindow = NULL;

CPluginManager::RegisteredWindow** CPluginManager::GetRegisteredWindow(nsPluginPlatformWindowRef window)
{
	RegisteredWindow** link = &theRegisteredWindows;
	RegisteredWindow* registeredWindow = *link;
	while (registeredWindow != NULL) {
		if (registeredWindow->mWindow == window)
			return link;
		link = &registeredWindow->mNext;
		registeredWindow = *link;
	}
	return NULL;
}

CPluginManager::RegisteredWindow* CPluginManager::FindRegisteredWindow(nsPluginPlatformWindowRef window)
{
	RegisteredWindow** link = GetRegisteredWindow(window);
	return (link != NULL ? *link : NULL);
}


NS_METHOD
CPluginManager::RegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
	theRegisteredWindows = new RegisteredWindow(theRegisteredWindows, handler, window);
	
#ifdef XP_MAC
	
	if (!mEventFiltersInstalled) {
		::InstallEventFilters(&EventFilter, &MenuFilter);
		mEventFiltersInstalled = true;
	}

	
	
	SInt16 variant = ::GetWVariant(window);
	if (variant == plainDBox) {
		::ShowHide(window, true);
		::BringToFront(window);
	} else {
		::ShowWindow(window);
		::SelectWindow(window);
	}
#endif

	return NS_OK;
}

NS_METHOD
CPluginManager::UnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
	RegisteredWindow** link = GetRegisteredWindow(window);
	if (link != NULL) {
		RegisteredWindow* registeredWindow = *link;
		if (registeredWindow == theActiveWindow)
			theActiveWindow = NULL;
		*link = registeredWindow->mNext;
		delete registeredWindow;
	}

#ifdef XP_MAC
	::HideWindow(window);

	
	if (theRegisteredWindows == NULL) {
		::RemoveEventFilters();
		mEventFiltersInstalled = false;
	}
#endif

	return NS_OK;
}

#ifdef XP_MAC

static void sendActivateEvent(nsIEventHandler* handler, WindowRef window, Boolean active)
{
	EventRecord event;
	::OSEventAvail(0, &event);
	event.what = activateEvt;
	event.message = UInt32(window);
	if (active)
		event.modifiers |= activeFlag;
	else
		event.modifiers &= ~activeFlag;

	nsPluginEvent pluginEvent = { &event, window };
	PRBool handled = PR_FALSE;

	handler->HandleEvent(&pluginEvent, &handled);
}








Boolean CPluginManager::EventFilter(EventRecord* event)
{
	Boolean filteredEvent = false;

	WindowRef window = WindowRef(event->message);
    nsPluginEvent pluginEvent = { event, window };
    EventRecord simulatedEvent;

    RegisteredWindow* registeredWindow;
	PRBool handled = PR_FALSE;
    
	
	switch (event->what) {
	case nullEvent:
		
		
		window = ::FrontWindow();
		registeredWindow = FindRegisteredWindow(window);
		if (registeredWindow != NULL) {
			simulatedEvent = *event;
			simulatedEvent.what = nsPluginEventType_AdjustCursorEvent;
			pluginEvent.event = &simulatedEvent;
			pluginEvent.window = registeredWindow->mWindow;
			registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
		}
		break;
	case keyDown:
	case keyUp:
	case autoKey:
		
		window = ::FrontWindow();
		registeredWindow = FindRegisteredWindow(window);
		if (registeredWindow != NULL) {
			pluginEvent.window = window;
			registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
			filteredEvent = true;
		}
		break;
	case mouseDown:
		
		short partCode = FindWindow(event->where, &window);
		switch (partCode) {
		case inContent:
		case inDrag:
		case inGrow:
		case inGoAway:
		case inZoomIn:
		case inZoomOut:
		case inCollapseBox:
		case inProxyIcon:
			registeredWindow = FindRegisteredWindow(window);
			if (registeredWindow != NULL) {
				
				if (theActiveWindow == NULL) {
					sendActivateEvent(registeredWindow->mHandler, window, true);
					theActiveWindow = registeredWindow;
				}
				pluginEvent.window = window;
				registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
				filteredEvent = true;
			} else if (theActiveWindow != NULL) {
				
				
				
				window = theActiveWindow->mWindow;
				sendActivateEvent(theActiveWindow->mHandler, window, false);
				::HiliteWindow(window, false);
				theActiveWindow = NULL;
			}
			break;
		}
		break;
	case activateEvt:
		registeredWindow = FindRegisteredWindow(window);
		if (registeredWindow != NULL) {
			registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
			filteredEvent = true;
			theActiveWindow = registeredWindow;
		}
		break;
	case updateEvt:
		registeredWindow = FindRegisteredWindow(window);
		if (registeredWindow != NULL) {
			GrafPtr port; GetPort(&port); SetPort(window); BeginUpdate(window);
				registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
			EndUpdate(window); SetPort(port);
			filteredEvent = true;
		}
		break;
	case osEvt:
		if ((event->message & osEvtMessageMask) == (suspendResumeMessage << 24)) {
			registeredWindow = theActiveWindow;
			if (registeredWindow != NULL) {
				window = registeredWindow->mWindow;
				Boolean active = (event->message & resumeFlag) != 0;
				sendActivateEvent(registeredWindow->mHandler, window, active);
				pluginEvent.window = window;
				registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
				::HiliteWindow(window, active);
			}
		}
		break;
	}
	
	return filteredEvent;
}


enum {
	kBaseMenuID = 20000,
	kBaseSubMenuID = 200
};

static PRInt16 nextMenuID = kBaseMenuID;
static PRInt16 nextSubMenuID = kBaseSubMenuID;

Boolean CPluginManager::MenuFilter(long menuSelection)
{
	if (theActiveWindow != NULL) {
		UInt16 menuID = (menuSelection >> 16);
		if ((menuID >= kBaseMenuID && menuID < nextMenuID) || (menuID >= kBaseSubMenuID && menuID < nextSubMenuID)) {
			EventRecord menuEvent;
			::OSEventAvail(0, &menuEvent);
			menuEvent.what = nsPluginEventType_MenuCommandEvent;
			menuEvent.message = menuSelection;

			WindowRef window = theActiveWindow->mWindow;
	    	nsPluginEvent pluginEvent = { &menuEvent, window };
			PRBool handled = PR_FALSE;
			theActiveWindow->mHandler->HandleEvent(&pluginEvent, &handled);
			
			return handled;
		}
	}
	return false;
}

NS_METHOD
CPluginManager::AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu, PRInt16 *result)
{
	*result = (isSubmenu ? nextSubMenuID++ : nextMenuID++);

	return NS_OK;
}

#else 

NS_METHOD
CPluginManager::AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu, PRInt16 *result)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

#endif 





NS_METHOD
CPluginManager::GetService(const nsCID& aClass, const nsIID& aIID, void* *result)
{
    
    if (aClass.Equals(kPluginManagerCID) || aClass.Equals(kMemoryCID)) {
        return QueryInterface(aIID, result);
    }
    if (!aClass.Equals(nsILiveconnect::GetCID())) {
        return NS_ERROR_SERVICE_NOT_FOUND;
    }
    if (mLiveconnect == NULL) {
        mLiveconnect = new nsLiveconnect;
        if (!mLiveconnect)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(mLiveconnect);
    }
    return mLiveconnect->QueryInterface(aIID, result);
}





NS_METHOD_(void*)
CPluginManager::Alloc(size_t size)
{
	return ::NPN_MemAlloc(size);
}

NS_METHOD_(void*)
CPluginManager::Realloc(void* ptr, size_t size)
{
	if (ptr != NULL) {
		void* new_ptr = Alloc(size);
		if (new_ptr != NULL) {
			::memcpy(new_ptr, ptr, size);
			Free(ptr);
		}
		ptr = new_ptr;
	}
	return ptr;
}

NS_METHOD_(void)
CPluginManager::Free(void* ptr)
{
	if (ptr != NULL) {
		::NPN_MemFree(ptr);
	}
}

NS_METHOD
CPluginManager::HeapMinimize(PRBool aImmediate)
{
#ifdef XP_MAC
	::NPN_MemFlush(1024);
#endif
	return NS_OK;
}





NS_METHOD
CPluginManager::QueryInterface(const nsIID& iid, void** ptr) 
{                                                                        
    if (NULL == ptr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }
    
    if (iid.Equals(NS_GET_IID(nsIPluginManager)) || iid.Equals(NS_GET_IID(nsIPluginManager2))) {
        *ptr = (void*) ((nsIPluginManager2*)this);                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    if (iid.Equals(NS_GET_IID(nsIServiceManager))) {                                                          
        *ptr = (void*) (nsIServiceManager*)this;                                        
        AddRef();                                                            
        return NS_OK;                                                        
    }
    if (iid.Equals(NS_GET_IID(nsIMemory))) {                                                          
        *ptr = (void*) (nsIMemory*)this;                                        
        AddRef();                                                            
        return NS_OK;                                                        
    }
    if (iid.Equals(NS_GET_IID(nsISupports))) {
        *ptr = (void*) this;                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    return NS_NOINTERFACE;                                                 
}

NS_IMPL_ADDREF(CPluginManager)
NS_IMPL_RELEASE(CPluginManager)






CPluginInstancePeer::CPluginInstancePeer(nsIPluginInstance* pluginInstance,
                                         NPP npp,
                                         nsMIMEType typeString, 
                                         nsPluginMode type,
                                         PRUint16 attr_cnt, 
                                         const char** attr_list,
                                         const char** val_list)
    :	mInstance(pluginInstance), mWindow(NULL),
		npp(npp), typeString(typeString), type(type), attribute_cnt(attr_cnt),
		attribute_list(NULL), values_list(NULL)
{
    NS_IF_ADDREF(mInstance);

    attribute_list = (char**) NPN_MemAlloc(attr_cnt * sizeof(const char*));
    values_list = (char**) NPN_MemAlloc(attr_cnt * sizeof(const char*));

    if (attribute_list != NULL && values_list != NULL) {
        for (int i = 0; i < attribute_cnt; i++)   {
            attribute_list[i] = (char*) NPN_MemAlloc(strlen(attr_list[i]) + 1);
            if (attribute_list[i] != NULL)
                strcpy(attribute_list[i], attr_list[i]);

            values_list[i] = (char*) NPN_MemAlloc(strlen(val_list[i]) + 1);
            if (values_list[i] != NULL)
                strcpy(values_list[i], val_list[i]);
        }
    }
}

CPluginInstancePeer::~CPluginInstancePeer(void) 
{
    if (attribute_list != NULL && values_list != NULL) {
        for (int i = 0; i < attribute_cnt; i++)   {
            NPN_MemFree(attribute_list[i]);
            NPN_MemFree(values_list[i]);
        }

        NPN_MemFree(attribute_list);
        NPN_MemFree(values_list);
    }
    
    NS_IF_RELEASE(mInstance);
}






NS_METHOD
CPluginInstancePeer::GetValue(nsPluginInstancePeerVariable variable, void *value)
{
#ifdef XP_UNIX
    return fromNPError[NPN_GetValue(NULL, (NPNVariable)varMap[(int)variable], value)];
#else
    return fromNPError[NPERR_GENERIC_ERROR];
#endif 
}





NS_METHOD
CPluginInstancePeer::SetValue(nsPluginInstancePeerVariable variable, void *value) 
{
#ifdef XP_UNIX
    return fromNPError[NPN_SetValue(NULL, (NPPVariable)varMap[(int)variable], value)];
#else
    return fromNPError[NPERR_GENERIC_ERROR];
#endif 
}






NS_METHOD
CPluginInstancePeer::GetMIMEType(nsMIMEType *result) 
{
    *result = typeString;
    return NS_OK;
}






NS_METHOD
CPluginInstancePeer::GetMode(nsPluginMode *result)
{
    *result = type;
    return NS_OK;
}






NS_METHOD
CPluginInstancePeer::GetAttributes(PRUint16& n, const char* const*& names, const char* const*& values)  
{
    n = attribute_cnt;
    names = attribute_list;
    values = values_list;

    return NS_OK;
}









NS_METHOD
CPluginInstancePeer::GetJavaEnv(JRIEnv* *resultingEnv)
{
	*resultingEnv = NPN_GetJavaEnv();
	return NS_OK;
}











NS_METHOD
CPluginInstancePeer::GetJavaPeer(jref *resultingJavaPeer)
{
	*resultingJavaPeer = NPN_GetJavaPeer(npp);
	return NS_OK;
}

#if defined(XP_MAC)

inline unsigned char toupper(unsigned char c)
{
    return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
}

static int strcasecmp(const char * str1, const char * str2)
{
#if __POWERPC__
	
    const	unsigned char * p1 = (unsigned char *) str1 - 1;
    const	unsigned char * p2 = (unsigned char *) str2 - 1;
    unsigned long		c1, c2;
		
    while (toupper(c1 = *++p1) == toupper(c2 = *++p2))
        if (!c1)
            return(0);

#else
	
    const	unsigned char * p1 = (unsigned char *) str1;
    const	unsigned char * p2 = (unsigned char *) str2;
    unsigned char		c1, c2;
	
    while (toupper(c1 = *p1++) == toupper(c2 = *p2++))
        if (!c1)
            return(0);

#endif
	
    return(toupper(c1) - toupper(c2));
}

#endif 



NS_METHOD
CPluginInstancePeer::GetAttribute(const char* name, const char* *result) 
{
    for (int i=0; i < attribute_cnt; i++)  {
#if defined(XP_UNIX) || defined(XP_MAC)
        if (strcasecmp(name, attribute_list[i]) == 0)
#else
            if (stricmp(name, attribute_list[i]) == 0) 
#endif
            {
                *result = values_list[i];
                return NS_OK;
            }
    }

    return NS_ERROR_FAILURE;
}











NS_METHOD
CPluginInstancePeer::GetDOMElement(nsIDOMElement* *result)
{


	return NS_OK;
}




NS_METHOD
CPluginInstancePeer::NewStream(nsMIMEType type, const char* target, 
                               nsIOutputStream* *result)
{
    assert( npp != NULL );
    
    
    NPStream* ptr = NULL;
    NPError error = NPN_NewStream(npp, (NPMIMEType)type, target, &ptr);
  if (error != NPERR_NO_ERROR)
        return fromNPError[error];
    
    
    
    
    CPluginManagerStream* mstream = new CPluginManagerStream(npp, ptr);
    if (mstream == NULL) 
        return NS_ERROR_OUT_OF_MEMORY;
    mstream->AddRef();
    *result = (nsIOutputStream* )mstream;

    return NS_OK;
}





NS_METHOD
CPluginInstancePeer::ShowStatus(const char* message)
{
    assert( message != NULL );

    NPN_Status(npp, message);
	return NS_OK;
}

NS_METHOD
CPluginInstancePeer::SetWindowSize(PRUint32 width, PRUint32 height)
{
    NPError err;
    NPSize size;
    size.width = width;
    size.height = height;
    err = NPN_SetValue(npp, NPPVpluginWindowSize, &size);
    return fromNPError[err];
}





NS_IMPL_ADDREF(CPluginInstancePeer)
NS_IMPL_RELEASE(CPluginInstancePeer)

NS_METHOD
CPluginInstancePeer::QueryInterface(const nsIID& iid, void** ptr) 
{                                                                        
    if (NULL == ptr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      
  
    if (iid.Equals(NS_GET_IID(nsIPluginInstancePeer))) {
        *ptr = (void*) this;                                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    if (iid.Equals(NS_GET_IID(nsIPluginTagInfo)) || iid.Equals(NS_GET_IID(nsISupports))) {                                      
        *ptr = (void*) ((nsIPluginTagInfo*)this);                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    return NS_NOINTERFACE;                                                 
}






CPluginManagerStream::CPluginManagerStream(NPP npp, NPStream* pstr)
    : npp(npp), pstream(pstr)
{
}

CPluginManagerStream::~CPluginManagerStream(void)
{
    
    NPN_DestroyStream(npp, pstream, NPRES_DONE);
}






NS_METHOD
CPluginManagerStream::Write(const char* buffer, PRUint32 len, PRUint32 *aWriteCount)
{
    assert( npp != NULL );
    assert( pstream != NULL );

    *aWriteCount = NPN_Write(npp, pstream, len, (void* )buffer);
    return *aWriteCount >= 0 ? NS_OK : NS_ERROR_FAILURE;
}

NS_METHOD
CPluginManagerStream::Write(nsIInputStream* fromStream, PRUint32 *aWriteCount)
{
	nsresult rv = fromStream->Available(aWriteCount);
	if (rv == NS_OK) {
		char buffer[1024];
		PRUint32 len = *aWriteCount;
		while (len > 0) {		
			PRUint32 count = (len < sizeof(buffer) ? len : sizeof(buffer));
			rv = fromStream->Read(buffer, count, &count);
			if (rv == NS_OK)
				rv = Write(buffer, count, &count);
			if (rv != NS_OK) {
				*aWriteCount -= len;
				break;
			}
			len -= count;
		}
	}
	return rv;
}

NS_METHOD
CPluginManagerStream::Flush()
{
	return NS_OK;
}





NS_METHOD
CPluginManagerStream::GetURL(const char* *result)
{
    assert( pstream != NULL );

    *result = pstream->url;
	return NS_OK;
}





NS_METHOD
CPluginManagerStream::GetEnd(PRUint32 *result)
{
    assert( pstream != NULL );

    *result = pstream->end;
	return NS_OK;
}





NS_METHOD
CPluginManagerStream::GetLastModified(PRUint32 *result)
{
    assert( pstream != NULL );

    *result = pstream->lastmodified;
	return NS_OK;
}





NS_METHOD
CPluginManagerStream::GetNotifyData(void* *result)
{
    assert( pstream != NULL );

    *result = pstream->notifyData;
	return NS_OK;
}





NS_METHOD
CPluginManagerStream::Close(void)
{
    assert( pstream != NULL );

    return NS_OK;
}






NS_IMPL_ISUPPORTS1(CPluginManagerStream, nsIOutputStream)



NS_IMPL_ISUPPORTS1(CPluginStreamInfo, nsIPluginStreamInfo)

CPluginInputStream::CPluginInputStream(nsIPluginStreamListener* listener)
    : mListener(listener), mStreamType(nsPluginStreamType_Normal),
      mNPP(NULL), mStream(NULL),
      mBuffer(NULL), mBufferLength(0), mAmountRead(0),
      mStreamInfo(NULL)
{
    if (mListener != NULL) {
        mListener->AddRef();
        mListener->GetStreamType(&mStreamType);
    }
}

CPluginInputStream::~CPluginInputStream(void)
{
	NS_IF_RELEASE(mListener);

    delete mBuffer;
    
    NS_IF_RELEASE(mStreamInfo);
}

NS_IMPL_ISUPPORTS1(CPluginInputStream, nsIPluginInputStream)

NS_METHOD
CPluginInputStream::Close(void)
{
    if (mNPP == NULL || mStream == NULL)
        return NS_ERROR_FAILURE;
    NPError err = NPN_DestroyStream(mNPP, mStream, NPRES_USER_BREAK);
    return fromNPError[err];
}

NS_METHOD
CPluginInputStream::Available(PRUint32 *aLength)
{
    *aLength = mStream->end;
    return NS_OK;
}

NS_METHOD
CPluginInputStream::Read(char* aBuf, PRUint32 aCount, PRUint32 *aReadCount)
{
    PRUint32 cnt = PR_MIN(aCount, mBufferLength);
    memcpy(aBuf, mBuffer + mAmountRead, cnt);
    *aReadCount = cnt;
    mAmountRead += cnt;
    mBufferLength -= cnt;
    return NS_OK;
}

NS_METHOD
CPluginInputStream::GetLastModified(PRUint32 *result)
{
    *result = mStream->lastmodified;
    return NS_OK;
}

NS_METHOD
CPluginInputStream::RequestRead(nsByteRange* rangeList)
{
    NPError err = NPN_RequestRead(mStream, (NPByteRange*)rangeList);
    return fromNPError[err];
}








