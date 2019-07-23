









































#ifndef CSecureJNI2_h___
#define CSecureJNI_h___

#include "nsISecureEnv.h"
#include "nsIThreadManager.h"
#include "SupportsMixin.h"
#include "nsAgg.h"

class MRJPlugin;
class MRJSession;

class Monitor;
class nsIThreadManager;
class nsIJVMManager;

class JavaMessage;
class JavaMessageQueue;

class CSecureEnv : public nsISecureEnv, public nsIRunnable, private SupportsMixin {
public:

    
    

	
    DECL_SUPPORTS_MIXIN

	static NS_METHOD Create(MRJPlugin* plugin, JNIEnv* proxyEnv, const nsIID& aIID, void* *aInstancePtr);

    
    


    








    NS_IMETHOD NewObject(  jclass clazz, 
                           jmethodID methodID,
                           jvalue *args, 
                          jobject* result,
                           nsISecurityContext* ctx = NULL);
   
    









    NS_IMETHOD CallMethod(  jni_type type,
                            jobject obj, 
                            jmethodID methodID,
                            jvalue *args, 
                           jvalue* result,
                            nsISecurityContext* ctx = NULL);

    










    NS_IMETHOD CallNonvirtualMethod(  jni_type type,
                                      jobject obj, 
                                      jclass clazz,
                                      jmethodID methodID,
                                      jvalue *args, 
                                     jvalue* result,
                                      nsISecurityContext* ctx = NULL);

    








    NS_IMETHOD GetField(  jni_type type,
                          jobject obj, 
                          jfieldID fieldID,
                         jvalue* result,
                          nsISecurityContext* ctx = NULL);

    








    NS_IMETHOD SetField( jni_type type,
                         jobject obj, 
                         jfieldID fieldID,
                         jvalue val,
                         nsISecurityContext* ctx = NULL);

    









    NS_IMETHOD CallStaticMethod(  jni_type type,
                                  jclass clazz,
                                  jmethodID methodID,
                                  jvalue *args, 
                                 jvalue* result,
                                  nsISecurityContext* ctx = NULL);

    








    NS_IMETHOD GetStaticField(  jni_type type,
                                jclass clazz, 
                                jfieldID fieldID, 
                               jvalue* result,
                                nsISecurityContext* ctx = NULL);


    








    NS_IMETHOD SetStaticField( jni_type type,
                               jclass clazz, 
                               jfieldID fieldID,
                               jvalue val,
                               nsISecurityContext* ctx = NULL);


    NS_IMETHOD GetVersion( jint* version);

    NS_IMETHOD DefineClass(  const char* name,
                             jobject loader,
                             const jbyte *buf,
                             jsize len,
                            jclass* clazz);

    NS_IMETHOD FindClass(  const char* name, 
                          jclass* clazz);

    NS_IMETHOD GetSuperclass(  jclass sub,
                              jclass* super);

    NS_IMETHOD IsAssignableFrom(  jclass sub,
                                  jclass super,
                                 jboolean* result);

    NS_IMETHOD Throw(  jthrowable obj,
                      jint* result);

    NS_IMETHOD ThrowNew(  jclass clazz,
                          const char *msg,
                         jint* result);

    NS_IMETHOD ExceptionOccurred( jthrowable* result);

    NS_IMETHOD ExceptionDescribe(void);

    NS_IMETHOD ExceptionClear(void);

    NS_IMETHOD FatalError( const char* msg);

    NS_IMETHOD NewGlobalRef(  jobject lobj, 
                             jobject* result);

    NS_IMETHOD DeleteGlobalRef( jobject gref);

    NS_IMETHOD DeleteLocalRef( jobject obj);

    NS_IMETHOD IsSameObject(  jobject obj1,
                              jobject obj2,
                             jboolean* result);

    NS_IMETHOD AllocObject(  jclass clazz,
                            jobject* result);

    NS_IMETHOD GetObjectClass(  jobject obj,
                               jclass* result);

    NS_IMETHOD IsInstanceOf(  jobject obj,
                              jclass clazz,
                             jboolean* result);

    NS_IMETHOD GetMethodID(  jclass clazz, 
                             const char* name,
                             const char* sig,
                            jmethodID* id);

    NS_IMETHOD GetFieldID(  jclass clazz, 
                            const char* name,
                            const char* sig,
                           jfieldID* id);

    NS_IMETHOD GetStaticMethodID(  jclass clazz, 
                                   const char* name,
                                   const char* sig,
                                  jmethodID* id);

    NS_IMETHOD GetStaticFieldID(  jclass clazz, 
                                  const char* name,
                                  const char* sig,
                                 jfieldID* id);

    NS_IMETHOD NewString(  const jchar* unicode,
                           jsize len,
                          jstring* result);

    NS_IMETHOD GetStringLength(  jstring str,
                                jsize* result);
    
    NS_IMETHOD GetStringChars(  jstring str,
                                jboolean *isCopy,
                               const jchar** result);

    NS_IMETHOD ReleaseStringChars(  jstring str,
                                    const jchar *chars);

    NS_IMETHOD NewStringUTF(  const char *utf,
                             jstring* result);

    NS_IMETHOD GetStringUTFLength(  jstring str,
                                   jsize* result);
    
    NS_IMETHOD GetStringUTFChars(  jstring str,
                                   jboolean *isCopy,
                                  const char** result);

    NS_IMETHOD ReleaseStringUTFChars(  jstring str,
                                       const char *chars);

    NS_IMETHOD GetArrayLength(  jarray array,
                               jsize* result);

    NS_IMETHOD NewObjectArray(  jsize len,
    					  jclass clazz,
                          jobject init,
                         jobjectArray* result);

    NS_IMETHOD GetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                      jobject* result);

    NS_IMETHOD SetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                       jobject val);

    NS_IMETHOD NewArray( jni_type element_type,
                          jsize len,
                         jarray* result);

    NS_IMETHOD GetArrayElements(  jni_type type,
                                  jarray array,
                                  jboolean *isCopy,
                                 void* result);

    NS_IMETHOD ReleaseArrayElements( jni_type type,
                                     jarray array,
                                     void *elems,
                                     jint mode);

    NS_IMETHOD GetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                               void* buf);

    NS_IMETHOD SetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                                void* buf);

    NS_IMETHOD RegisterNatives(  jclass clazz,
                                 const JNINativeMethod *methods,
                                 jint nMethods,
                                jint* result);

    NS_IMETHOD UnregisterNatives(  jclass clazz,
                                  jint* result);

    NS_IMETHOD MonitorEnter(  jobject obj,
                             jint* result);

    NS_IMETHOD MonitorExit(  jobject obj,
                            jint* result);

    NS_IMETHOD GetJavaVM(  JavaVM **vm,
                          jint* result);
                         
    
    
    
    NS_IMETHOD Run();

    CSecureEnv(MRJPlugin* plugin, JNIEnv* proxyEnv, JNIEnv* javaEnv = NULL);
    virtual ~CSecureEnv(void);

	



	void initialize(JNIEnv* javaEnv, jboolean* isRunning, JavaMessageQueue* javaQueue, JavaMessageQueue* nativeQueue);
	
	jboolean isInitialized() { return mJavaQueue != NULL; }

	void setProxyEnv(JNIEnv* proxyEnv) { mProxyEnv = proxyEnv; }
	JNIEnv* getProxyEnv() { return mProxyEnv; }
	
	void setJavaEnv(JNIEnv* javaEnv) { mJavaEnv = javaEnv; }
	JNIEnv* getJavaEnv() { return mJavaEnv; }
	
	MRJSession* getSession() { return mSession; }
	nsIThreadManager* getThreadManager() { return mThreadManager; }
	
	void getMessageQueues(JavaMessageQueue*& javaQueue, JavaMessageQueue*& nativeQueue)
	{
		javaQueue = mJavaQueue;
		nativeQueue = mNativeQueue;
	}
	
	void sendMessageToJava(JavaMessage* msg);
	void sendMessageFromJava(JNIEnv* javaEnv, JavaMessage* msg, Boolean busyWaiting = false);
	void messageLoop(JNIEnv* env, JavaMessage* msgToSend, JavaMessageQueue* sendQueue, JavaMessageQueue* receiveQueue, Boolean busyWaiting = false);
	
protected:

	MRJPlugin*				mPlugin;
    JNIEnv*					mProxyEnv;
    MRJSession*				mSession;
    nsIThreadManager*		mThreadManager;
    
    JNIEnv*     			mJavaEnv;
    jboolean*				mIsRunning;
    JavaMessageQueue*		mJavaQueue;
    JavaMessageQueue*		mNativeQueue;

private:
	
	static const InterfaceInfo sInterfaces[];
	static const UInt32 kInterfaceCount;
};




CSecureEnv* GetSecureJNI(JNIEnv* env, jobject thread);

#endif 
