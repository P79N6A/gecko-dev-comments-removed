









































#ifndef nsILiveConnectManager_h___
#define nsILiveConnectManager_h___

#ifndef nsISupports_h___
#include "nsISupports.h"
#endif
#ifndef JNI_H
#include "jni.h"
#endif


#define NS_ILIVECONNECTMANAGER_IID \
{ 0xd20c8081, 0xcbcb, 0x11d2, { 0xa5, 0xa0, 0xe8, 0x84, 0xae, 0xd9, 0xc9, 0xfc } }


#define NS_LIVECONNECTMANAGER_CID \
{ 0xd20c8083, 0xcbcb, 0x11d2, { 0xa5, 0xa0, 0xe8, 0x84, 0xae, 0xd9, 0xc9, 0xfc } }

struct JSRuntime;
struct JSContext;
struct JSObject;

class nsILiveConnectManager : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILIVECONNECTMANAGER_IID)
	
	


	NS_IMETHOD
    StartupLiveConnect(JSRuntime* runtime, PRBool& outStarted) = 0;
    
	


	NS_IMETHOD
    ShutdownLiveConnect(JSRuntime* runtime, PRBool& outShutdown) = 0;

	


	NS_IMETHOD
    IsLiveConnectEnabled(PRBool& outEnabled) = 0;
    
    


	NS_IMETHOD
    InitLiveConnectClasses(JSContext* context, JSObject* globalObject) = 0;
    
    


     NS_IMETHOD
     WrapJavaObject(JSContext* context, jobject javaObject, JSObject* *outJSObject) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILiveConnectManager,
                              NS_ILIVECONNECTMANAGER_IID)



#endif 
