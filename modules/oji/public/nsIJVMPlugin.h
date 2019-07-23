














































#ifndef nsIJVMPlugin_h___
#define nsIJVMPlugin_h___

#include "nsISupports.h"
#include "nsIPrincipal.h"
#include "jni.h"

class nsISecureEnv;




#define NS_JVM_MIME_TYPE        "application/x-java-vm" // XXX "application/java" ?


#define NS_IJVMPLUGIN_IID                            \
{ /* da6f3bc0-a1bc-11d1-85b1-00805f0e4dfe */         \
    0xda6f3bc0,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}






class nsIJVMPlugin : public nsISupports {
public:
	
	
	NS_IMETHOD
	AddToClassPath(const char* dirPath) = 0;

	
	
	NS_IMETHOD
	RemoveFromClassPath(const char* dirPath) = 0;

	
	NS_IMETHOD
	GetClassPath(const char* *result) = 0;

	NS_IMETHOD
#if PR_BYTES_PER_LONG == 8
	GetJavaWrapper(JNIEnv* jenv, jlong obj, jobject *jobj) = 0;
#else
	GetJavaWrapper(JNIEnv* jenv, jint obj, jobject *jobj) = 0;
#endif

	






	NS_IMETHOD
	CreateSecureEnv(JNIEnv* proxyEnv, nsISecureEnv* *outSecureEnv) = 0;

	



	NS_IMETHOD
	SpendTime(PRUint32 timeMillis) = 0;

	NS_IMETHOD
#if PR_BYTES_PER_LONG == 8 || PR_BYTES_PER_WORD == 8
	UnwrapJavaWrapper(JNIEnv* jenv, jobject jobj, jlong* obj) = 0;
#else
	UnwrapJavaWrapper(JNIEnv* jenv, jobject jobj, jint* obj) = 0;
#endif

 	NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJVMPLUGIN_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJVMPlugin, NS_IJVMPLUGIN_IID)



#endif
