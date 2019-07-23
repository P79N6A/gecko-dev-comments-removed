




































#include "CSecureEnv.h"
#include "nsISecurityContext.h"

#include "MRJPlugin.h"
#include "MRJSession.h"
#include "nsIThreadManager.h"
#include "nsIJVMManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"

#include "MRJMonitor.h"
#include "NativeMonitor.h"
#include "JavaMessageQueue.h"

#define PROXY_JNI_CALLS 1
#define USE_LIVECONNECT_PROXY 1
#define LOCAL_REFS_ARE_GLOBAL USE_LIVECONNECT_PROXY

inline jobject ToGlobalRef(JNIEnv* env, jobject localRef)
{
    jobject globalRef = env->NewGlobalRef(localRef);
    
    return globalRef;
}

JavaMessageQueue::JavaMessageQueue(Monitor* monitor)
	:	mFirst(NULL), mLast(NULL), mMonitor(monitor)
{
}

void JavaMessageQueue::putMessage(JavaMessage* msg)
{
	if (mFirst == NULL) {
		mFirst = mLast = msg;
	} else {
		mLast->setNext(msg);
		mLast = msg;
	}
	msg->setNext(NULL);
}

JavaMessage* JavaMessageQueue::getMessage()
{
	JavaMessage* msg = mFirst;
	if (msg != NULL) {
		mFirst = mFirst->getNext();
		if (mFirst == NULL) mLast = NULL;
	}
	return msg;
}

void JavaMessageQueue::enter()
{
	mMonitor->enter();
}

void JavaMessageQueue::exit()
{
	mMonitor->exit();
}

void JavaMessageQueue::wait()
{
	mMonitor->wait();
}

void JavaMessageQueue::wait(long long millis)
{
	mMonitor->wait(millis);
}

void JavaMessageQueue::notify()
{
	mMonitor->notify();
}




static void netscape_oji_JNIThread_run(JNIEnv* env, jobject self)
{
	CSecureEnv* secureEnv = NULL;
	jmethodID yieldMethod = NULL;
	jmethodID sleepMethod = NULL;
	
	jclass clazz = env->GetObjectClass(self);
	if (clazz != NULL) {
		
		jfieldID fSecureEnvField = env->GetFieldID(clazz, "fSecureEnv", "I");
		if (fSecureEnvField != NULL) {
			secureEnv = (CSecureEnv*) env->GetIntField(self, fSecureEnvField);
		}
		yieldMethod = env->GetStaticMethodID(clazz, "yield", "()V");
		sleepMethod = env->GetStaticMethodID(clazz, "sleep", "(J)V");
	}
	
	
	if (secureEnv != NULL) {
		jboolean isRunning = true;
		MRJSession* session = secureEnv->getSession();
		MRJMonitor requestMonitor(session, self);
		MRJMonitor replyMonitor(session);
		
		JavaMessageQueue requests(&requestMonitor), replies(&replyMonitor);
		secureEnv->initialize(env, &isRunning, &requests, &replies);
		
		
		requests.enter();
		
		while (isRunning) {
			
			
			
			
			JavaMessage* msg = requests.getMessage();
			if (msg != NULL) {
				msg->execute(env);
				secureEnv->savePendingException(env);
				replies.putMessage(msg);
				replies.notify();
			} else {
				
				
				
				requests.wait();
			}
		}
		
		requests.exit();
	}
}

static jclass netscape_oji_JNIRunnable;
static jmethodID netscape_oji_JNIRunnable_constructorID;
static jfieldID netscape_oji_JNIRunnable_mJavaMessageID;

static void netscape_oji_JNIRunnable_run(JNIEnv* env, jobject self)
{
    JavaMessage* msg = (JavaMessage*) env->GetIntField(self, netscape_oji_JNIRunnable_mJavaMessageID);
    if (msg) {
        msg->execute(env);
        
    }
}

static bool check_exceptions(JNIEnv* env)
{
    jthrowable exc = env->ExceptionOccurred();
	if (exc) {
	    env->ExceptionDescribe();
	    env->ExceptionClear();
	    env->DeleteLocalRef(exc);
	    return true;
	}
	return false;
}





static void CreateJNIThread(CSecureEnv* secureEnv)
{
	nsIThreadManager* manager = secureEnv->getThreadManager();
	MRJSession* session = secureEnv->getSession();
	JNIEnv* env = session->getCurrentEnv();
	
    check_exceptions(env);

	jclass JNIThreadClass = env->FindClass("netscape.oji.JNIThread");
	if (JNIThreadClass != NULL) {
		JNINativeMethod method = { "run", "()V", &netscape_oji_JNIThread_run };
		env->RegisterNatives(JNIThreadClass, &method, 1);
		jmethodID constructorID = env->GetMethodID(JNIThreadClass, "<init>", "(I)V");
		if (constructorID != NULL) {
			jobject javaThread = env->NewObject(JNIThreadClass, constructorID, secureEnv);
			for (;;) {
				
				if (check_exceptions(env))
				    break;
				
				session->idle();
				
				if (secureEnv->isInitialized())
					break;
				
				manager->Sleep();
			}
		}
		env->DeleteLocalRef(JNIThreadClass);
	}
}

static jobject CreateJNIRunnable(JNIEnv* env, JavaMessage* msg)
{
    
    if (!netscape_oji_JNIRunnable) {
    	jclass clazz = env->FindClass("netscape.oji.JNIRunnable");
    	if (!clazz) return NULL;
		JNINativeMethod method = { "run", "()V", &netscape_oji_JNIRunnable_run };
		env->RegisterNatives(clazz, &method, 1);
		netscape_oji_JNIRunnable_constructorID = env->GetMethodID(clazz, "<init>", "(I)V");
        netscape_oji_JNIRunnable_mJavaMessageID = env->GetFieldID(clazz, "mJavaMessage", "I");
		netscape_oji_JNIRunnable = (jclass) env->NewGlobalRef(clazz);
		env->DeleteLocalRef(clazz);
    }
    check_exceptions(env);
    return env->NewObject(netscape_oji_JNIRunnable, netscape_oji_JNIRunnable_constructorID, msg);
}

static jclass GetLiveConnectProxy(JNIEnv* env, nsIPrincipal* codebasePrincipal)
{
    jclass liveConnectProxy = NULL;
    jclass netscape_oji_ProxyClassLoaderFactory = env->FindClass("netscape/oji/ProxyClassLoaderFactory");
    if (netscape_oji_ProxyClassLoaderFactory) {
        jmethodID createClassLoaderID = env->GetStaticMethodID(netscape_oji_ProxyClassLoaderFactory,
                                                               "createClassLoader", 
                                                               "(Ljava/lang/String;)Ljava/lang/ClassLoader;");
        if (createClassLoaderID) {
            jstring codebaseUTF = NULL;
            char* codebase;
            nsresult rv = codebasePrincipal->GetOrigin(&codebase);
            if (NS_SUCCEEDED(rv)) {
                codebaseUTF = env->NewStringUTF(codebase);
                delete[] codebase;
            }
            if (codebaseUTF) {
                jobject classLoader = env->CallStaticObjectMethod(netscape_oji_ProxyClassLoaderFactory,
                                                                  createClassLoaderID, codebaseUTF);
                if (classLoader) {
                    jclass clazz = env->GetObjectClass(classLoader);
                    jmethodID loadClassID = env->GetMethodID(clazz, "loadClass",
                                                             "(Ljava/lang/String;)Ljava/lang/Class;");
                    if (loadClassID) {
                        jstring className = env->NewStringUTF("netscape.oji.LiveConnectProxy");
                        if (className) {
                            liveConnectProxy = (jclass) env->CallObjectMethod(classLoader, loadClassID, className);
                            env->DeleteLocalRef(className);
                        }
                    }
                    env->DeleteLocalRef(clazz);
                }
                env->DeleteLocalRef(codebaseUTF);
            }
        }
        env->DeleteLocalRef(netscape_oji_ProxyClassLoaderFactory);
    }
    check_exceptions(env);
    return liveConnectProxy;
}




class CreateNativeThreadMessage : public NativeMessage {
	nsresult* mResult;
	PRUint32* mThreadID;
	CSecureEnv* mSecureEnv;
public:
	CreateNativeThreadMessage(nsresult* outResult, PRUint32* outThreadID, CSecureEnv* secureEnv)
		:	mResult(outResult), mThreadID(outThreadID), mSecureEnv(secureEnv)
	{
	}

	virtual void execute()
	{
		nsIThreadManager* manager = mSecureEnv->getThreadManager();
		*mResult = manager->CreateThread(mThreadID, mSecureEnv);
	}
};




static void CreateNativeThread(CSecureEnv* secureEnv)
{
	nsresult result;
	PRUint32 threadID;
	MRJSession* session = secureEnv->getSession();
	
	
	
	CreateNativeThreadMessage message(&result, &threadID, secureEnv);
	session->sendMessage(&message);

	if (session->onMainThread()) {
		
		nsIThreadManager* manager = secureEnv->getThreadManager();
		while (!secureEnv->isInitialized()) {
			manager->Sleep();
		}
	} else {
		
		JNIEnv* env = session->getCurrentEnv();
		jclass threadClass = env->FindClass("java/lang/Thread");
		if (threadClass != NULL) {
			jmethodID sleepMethod = env->GetStaticMethodID(threadClass, "sleep", "(J)V");
			if (sleepMethod != NULL) {
				while (!secureEnv->isInitialized())
					env->CallStaticVoidMethod(threadClass, sleepMethod, jlong(1024));
			}
			env->DeleteLocalRef(threadClass);
		}
	}
}




NS_IMETHODIMP CSecureEnv::Run()
{
	jboolean isRunning = true;
	NativeMonitor requestMonitor(mSession, mThreadManager);
	MRJMonitor replyMonitor(mSession);
	JavaMessageQueue requests(&requestMonitor), replies(&replyMonitor);
	
	
	
	nsIJVMManager* manager = mPlugin->getManager();
	manager->CreateProxyJNI(this, &mProxyEnv);
	
	mIsRunning = &isRunning;
	mNativeQueue = &requests;
	mJavaQueue = &replies;
	
	
	requests.enter();
	
	while (isRunning) {
		
		
		
		
		JavaMessage* msg = requests.getMessage();
		if (msg != NULL) {
			msg->execute(mProxyEnv);
			replies.putMessage(msg);
			replies.notify();
		} else {
			
			
			
			requests.wait();
		}
	}
	
	requests.exit();

	return NS_OK;
}




void CSecureEnv::sendMessageToJava(JavaMessage* msg)
{
#if USE_LIVECONNECT_PROXY
    JNIEnv* env = mSession->getCurrentEnv();
    
    
    
    nsresult rv = NS_OK;
    if (!mScriptSecurityManager) {
        rv = MRJPlugin::GetService("@mozilla.org/scriptsecuritymanager;1",
                                   NS_GET_IID(nsIScriptSecurityManager),
                                   (void**)&mScriptSecurityManager);
    }
    
    
    
    
    
    
    
    if (NS_SUCCEEDED(rv)) {
        nsIPrincipal* scriptPrincipal;
        rv = mScriptSecurityManager->GetSubjectPrincipal(&scriptPrincipal);
        if (NS_SUCCEEDED(rv)) {
            if (scriptPrincipal != mScriptPrincipal) {
                
                NS_IF_RELEASE(mScriptPrincipal);
                mScriptPrincipal = scriptPrincipal;
                if (mLiveConnectProxy) {
                    env->DeleteGlobalRef(mLiveConnectProxy);
                    mLiveConnectProxy = NULL;
                }
            }
            if (!mLiveConnectProxy && scriptPrincipal) {
                jclass liveConnectProxy = GetLiveConnectProxy(env, scriptPrincipal);
                if (liveConnectProxy) {
                    mLiveConnectProxy = (jclass) env->NewGlobalRef(liveConnectProxy);
                    env->DeleteLocalRef(liveConnectProxy);
                }
            }
        }
    }
    if (mLiveConnectProxy) {
        jobject runnable = CreateJNIRunnable(env, msg);
        if (runnable) {
            jmethodID runID = env->GetStaticMethodID(mLiveConnectProxy, "run", "(Ljava/lang/Runnable;)V");
            if (runID) {
                env->CallStaticVoidMethod(mLiveConnectProxy, runID, runnable);
                savePendingException(env);
            }
            env->DeleteLocalRef(runnable);
        }
    } else {
        msg->execute(env);
        savePendingException(env);
    }
#else
	messageLoop(mProxyEnv, msg, mJavaQueue, mNativeQueue, true);
#endif
}




void CSecureEnv::sendMessageFromJava(JNIEnv* javaEnv, JavaMessage* msg, Boolean busyWaiting)
{
	messageLoop(javaEnv, msg, mNativeQueue, mJavaQueue, busyWaiting);
}




const InterfaceInfo CSecureEnv::sInterfaces[] = {
	{ NS_ISECUREENV_IID, INTERFACE_OFFSET(CSecureEnv, nsISecureEnv) },
	{ NS_IRUNNABLE_IID, INTERFACE_OFFSET(CSecureEnv, nsIRunnable) },
};
const UInt32 CSecureEnv::kInterfaceCount = sizeof(sInterfaces) / sizeof(InterfaceInfo);

CSecureEnv::CSecureEnv(MRJPlugin* plugin, JNIEnv* proxyEnv, JNIEnv* javaEnv)
	:	SupportsMixin(this, sInterfaces, kInterfaceCount),
		mPlugin(plugin), mProxyEnv(proxyEnv), mJavaEnv(javaEnv),
		mSession(plugin->getSession()), mThreadManager(plugin->getThreadManager()),
		mIsRunning(NULL), mJavaQueue(NULL), mNativeQueue(NULL),
		mPendingException(NULL),
		mScriptSecurityManager(NULL), mScriptPrincipal(NULL), mLiveConnectProxy(NULL)
{
	
	if (mJavaEnv != NULL)
        CreateNativeThread(this);
	else
	    CreateJNIThread(this);
}


CSecureEnv::~CSecureEnv()  
{
	
	if (mIsRunning != NULL) {
		*mIsRunning = false;
		mJavaQueue->notify();
	}

    JNIEnv* env = mSession->getCurrentEnv();
    if (mPendingException) {
        env->DeleteGlobalRef(mPendingException);
        mPendingException = NULL;
    }
    
    if (mLiveConnectProxy) {
        env->DeleteGlobalRef(mLiveConnectProxy);
        mLiveConnectProxy = NULL;
    }
    
    NS_IF_RELEASE(mScriptPrincipal);
    NS_IF_RELEASE(mScriptSecurityManager);
}

void CSecureEnv::initialize(JNIEnv* javaEnv, jboolean* isRunning, JavaMessageQueue* javaQueue, JavaMessageQueue* nativeQueue)
{
	mJavaEnv = javaEnv;
	mIsRunning = isRunning;
	mJavaQueue = javaQueue;
	mNativeQueue = nativeQueue;
}













NS_METHOD
CSecureEnv::Create(MRJPlugin* plugin, JNIEnv* proxyEnv, const nsIID& aIID, void* *aInstancePtr)
{
	CSecureEnv* secureEnv = new CSecureEnv(plugin, proxyEnv);
	if (secureEnv == NULL)
	    return NS_ERROR_OUT_OF_MEMORY;
	NS_ADDREF(secureEnv);
	nsresult rv = secureEnv->QueryInterface(aIID, aInstancePtr);
	NS_RELEASE(secureEnv);
	return rv;
}














class NewObjectMessage : public JavaMessage {
	jclass clazz;
	jmethodID methodID;
	jvalue* args;
	jobject* result;

public:
	NewObjectMessage(jclass clazz,  jmethodID methodID, jvalue *args, jobject* result)
	{
		this->clazz = clazz;
		this->methodID = methodID;
		this->args = args;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
		*result = env->NewObjectA(clazz, methodID, args);
#if LOCAL_REFS_ARE_GLOBAL
        *result = ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::NewObject(  jclass clazz, 
                                      jmethodID methodID, 
                                      jvalue *args, 
                                     jobject* result,
                                      nsISecurityContext* ctx)
{
    if (clazz == NULL || methodID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    
    NewObjectMessage msg(clazz, methodID, args, result);
    sendMessageToJava(&msg);
#else
	*result = mJavaEnv->NewObjectA(clazz, methodID, args);
#endif
	
	return NS_OK;
}














class CallMethodMessage : public JavaMessage {
	jni_type return_type;
	jobject obj;
	jmethodID methodID;
	jvalue* args;
	jvalue* result;

public:
	CallMethodMessage(jni_type return_type, jobject obj, jmethodID methodID, jvalue *args, jvalue* result)
	{
		this->return_type = return_type;
		this->obj = obj;
		this->methodID = methodID;
		this->args = args;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
		switch (return_type) {
		case jobject_type:
			result->l = env->CallObjectMethodA(obj, methodID, args);
#if LOCAL_REFS_ARE_GLOBAL
            result->l = ToGlobalRef(env, result->l);
#endif
			break;
		case jboolean_type:
			result->z = env->CallBooleanMethodA(obj, methodID, args);
			break;
		case jbyte_type:
			result->b = env->CallByteMethodA(obj, methodID, args);
			break;
		case jchar_type:
			result->c = env->CallCharMethodA(obj, methodID, args);
			break;
		case jshort_type:
			result->s = env->CallShortMethodA(obj, methodID, args);
			break;
		case jint_type:
			result->i = env->CallIntMethodA(obj, methodID, args);
			break;
		case jlong_type:
			result->j = env->CallLongMethodA(obj, methodID, args);
			break;
		case jfloat_type:
			result->f = env->CallFloatMethodA(obj, methodID, args);
			break;
		case jdouble_type:
			result->d = env->CallDoubleMethodA(obj, methodID, args);
			break;
		case jvoid_type:
			env->CallVoidMethodA(obj, methodID, args);
			break;
		}
	}
};

NS_IMETHODIMP CSecureEnv::CallMethod(  jni_type return_type,
                                        jobject obj, 
                                        jmethodID methodID, 
                                        jvalue *args, 
                                       jvalue* result,
                                        nsISecurityContext* ctx)
{
    if (obj == NULL || methodID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	
	
	CallMethodMessage msg(return_type, obj, methodID, args, result);
	sendMessageToJava(&msg);
#else
    JNIEnv* env = mJavaEnv;
	switch (return_type) {
	case jobject_type:
		result->l = env->CallObjectMethodA(obj, methodID, args);
		break;
	case jboolean_type:
		result->z = env->CallBooleanMethodA(obj, methodID, args);
		break;
	case jbyte_type:
		result->b = env->CallByteMethodA(obj, methodID, args);
		break;
	case jchar_type:
		result->c = env->CallCharMethodA(obj, methodID, args);
		break;
	case jshort_type:
		result->s = env->CallShortMethodA(obj, methodID, args);
		break;
	case jint_type:
		result->i = env->CallIntMethodA(obj, methodID, args);
		break;
	case jlong_type:
		result->j = env->CallLongMethodA(obj, methodID, args);
		break;
	case jfloat_type:
		result->f = env->CallFloatMethodA(obj, methodID, args);
		break;
	case jdouble_type:
		result->d = env->CallDoubleMethodA(obj, methodID, args);
		break;
	case jvoid_type:
		env->CallVoidMethodA(obj, methodID, args);
		break;
	}
#endif
	
	return NS_OK;
}














class CallNonvirtualMethodMessage : public JavaMessage {
	jni_type type;
	jobject obj;
	jclass clazz;
	jmethodID methodID;
	jvalue* args;
	jvalue* result;

public:
	CallNonvirtualMethodMessage(jni_type type, jobject obj, jclass clazz, jmethodID methodID, jvalue *args, jvalue* result)
	{
		this->type = type;
		this->obj = obj;
		this->clazz = clazz;
		this->methodID = methodID;
		this->args = args;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
		switch (type) {
		case jobject_type:
			result->l = env->CallNonvirtualObjectMethodA(obj, clazz, methodID, args);
#if LOCAL_REFS_ARE_GLOBAL
            result->l = ToGlobalRef(env, result->l);
#endif
			break;
		case jboolean_type:
			result->z = env->CallNonvirtualBooleanMethodA(obj, clazz, methodID, args);
			break;
		case jbyte_type:
			result->b = env->CallNonvirtualByteMethodA(obj, clazz, methodID, args);
			break;
		case jchar_type:
			result->c = env->CallNonvirtualCharMethodA(obj, clazz, methodID, args);
			break;
		case jshort_type:
			result->s = env->CallNonvirtualShortMethodA(obj, clazz, methodID, args);
			break;
		case jint_type:
			result->i = env->CallNonvirtualIntMethodA(obj, clazz, methodID, args);
			break;
		case jlong_type:
			result->j = env->CallNonvirtualLongMethodA(obj, clazz, methodID, args);
			break;
		case jfloat_type:
			result->f = env->CallNonvirtualFloatMethodA(obj, clazz, methodID, args);
			break;
		case jdouble_type:
			result->d = env->CallNonvirtualDoubleMethodA(obj, clazz, methodID, args);
			break;
		case jvoid_type:
			env->CallNonvirtualVoidMethodA(obj, clazz, methodID, args);
			break;
		}
	}
};

NS_IMETHODIMP CSecureEnv::CallNonvirtualMethod(  jni_type type,
                                                  jobject obj, 
                                                  jclass clazz,
                                                  jmethodID methodID,
                                                  jvalue *args, 
                                                 jvalue* result,
                                                  nsISecurityContext* ctx)
{
    if (obj == NULL || clazz == NULL || methodID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	
	
	CallNonvirtualMethodMessage msg(type, obj, clazz, methodID, args, result);
	sendMessageToJava(&msg);
#else
    #error "Implement me!"
#endif

	return NS_OK;
}













class GetFieldMessage : public JavaMessage {
	jni_type type;
	jobject obj;
	jfieldID fieldID;
	jvalue* result;
public:
	GetFieldMessage(jni_type type, jobject obj, jfieldID fieldID, jvalue* result)
	{
		this->type = type;
		this->obj = obj;
		this->fieldID = fieldID;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
		switch (type) {
		case jobject_type:
			result->l = env->GetObjectField(obj, fieldID);
#if LOCAL_REFS_ARE_GLOBAL
            result->l = ToGlobalRef(env, result->l);
#endif
			break;
		case jboolean_type:
			result->z = env->GetBooleanField(obj, fieldID);
			break;
		case jbyte_type:
			result->b = env->GetByteField(obj, fieldID);
			break;
		case jchar_type:
			result->c = env->GetCharField(obj, fieldID);
			break;
		case jshort_type:
			result->s = env->GetShortField(obj, fieldID);
			break;
		case jint_type:
			result->i = env->GetIntField(obj, fieldID);
			break;
		case jlong_type:
			result->j = env->GetLongField(obj, fieldID);
			break;
		case jfloat_type:
			result->f = env->GetFloatField(obj, fieldID);
			break;
		case jdouble_type:
			result->d = env->GetDoubleField(obj, fieldID);
			break;
		}
	}
};

NS_IMETHODIMP CSecureEnv::GetField(  jni_type type,
                                     jobject obj, 
                                     jfieldID fieldID,
                                    jvalue* result,
                                     nsISecurityContext* ctx)
{
    if (mJavaEnv == NULL || obj == NULL || fieldID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    
    GetFieldMessage msg(type, obj, fieldID, result);
    sendMessageToJava(&msg);
#else    
	JNIEnv* env = mJavaEnv;
	switch (type) {
	case jobject_type:
		result->l = env->GetObjectField(obj, fieldID);
		break;
	case jboolean_type:
		result->z = env->GetBooleanField(obj, fieldID);
		break;
	case jbyte_type:
		result->b = env->GetByteField(obj, fieldID);
		break;
	case jchar_type:
		result->c = env->GetCharField(obj, fieldID);
		break;
	case jshort_type:
		result->s = env->GetShortField(obj, fieldID);
		break;
	case jint_type:
		result->i = env->GetIntField(obj, fieldID);
		break;
	case jlong_type:
		result->j = env->GetLongField(obj, fieldID);
		break;
	case jfloat_type:
		result->f = env->GetFloatField(obj, fieldID);
		break;
	case jdouble_type:
		result->d = env->GetDoubleField(obj, fieldID);
		break;
	}
#endif

	return NS_OK;
}














class SetFieldMessage : public JavaMessage {
	jni_type type;
	jobject obj;
	jfieldID fieldID;
	jvalue val;
public:
	SetFieldMessage(jni_type type, jobject obj, jfieldID fieldID, jvalue val)
	{
		this->type = type;
		this->obj = obj;
		this->fieldID = fieldID;
		this->val = val;
	}
	
	virtual void execute(JNIEnv* env)
	{
    	switch (type) {
    	case jobject_type:
    		env->SetObjectField(obj, fieldID, val.l);
    		break;
    	case jboolean_type:
    		env->SetBooleanField(obj, fieldID, val.z);
    		break;
    	case jbyte_type:
    		env->SetByteField(obj, fieldID, val.b);
    		break;
    	case jchar_type:
    		env->SetCharField(obj, fieldID, val.c);
    		break;
    	case jshort_type:
    		env->SetShortField(obj, fieldID, val.s);
    		break;
    	case jint_type:
    		env->SetIntField(obj, fieldID, val.i);
    		break;
    	case jlong_type:
    		env->SetLongField(obj, fieldID, val.j);
    		break;
    	case jfloat_type:
    		env->SetFloatField(obj, fieldID, val.f);
    		break;
    	case jdouble_type:
    		env->SetDoubleField(obj, fieldID, val.d);
    		break;
    	}
	}
};

NS_IMETHODIMP CSecureEnv::SetField( jni_type type,
                                     jobject obj, 
                                     jfieldID fieldID,
                                     jvalue val,
                                     nsISecurityContext* ctx)
{
    if (mJavaEnv == NULL || obj == NULL || fieldID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    
    SetFieldMessage msg(type, obj, fieldID, val);
    sendMessageToJava(&msg);
#else
	JNIEnv* env = mJavaEnv;
	switch (type) {
	case jobject_type:
		env->SetObjectField(obj, fieldID, val.l);
		break;
	case jboolean_type:
		env->SetBooleanField(obj, fieldID, val.z);
		break;
	case jbyte_type:
		env->SetByteField(obj, fieldID, val.b);
		break;
	case jchar_type:
		env->SetCharField(obj, fieldID, val.c);
		break;
	case jshort_type:
		env->SetShortField(obj, fieldID, val.s);
		break;
	case jint_type:
		env->SetIntField(obj, fieldID, val.i);
		break;
	case jlong_type:
		env->SetLongField(obj, fieldID, val.j);
		break;
	case jfloat_type:
		env->SetFloatField(obj, fieldID, val.f);
		break;
	case jdouble_type:
		env->SetDoubleField(obj, fieldID, val.d);
		break;
	}
#endif

	return NS_OK;
}















class CallStaticMethodMessage : public JavaMessage {
	jni_type type;
	jclass clazz;
	jmethodID methodID;
	jvalue* args;
	jvalue* result;
public:
	CallStaticMethodMessage(jni_type type, jclass clazz, jmethodID methodID, jvalue *args, jvalue* result)
	{
		this->type = type;
		this->clazz = clazz;
		this->methodID = methodID;
		this->args = args;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
		switch (type) {
		case jobject_type:
			result->l = env->CallStaticObjectMethodA(clazz, methodID, args);
#if LOCAL_REFS_ARE_GLOBAL
            result->l = ToGlobalRef(env, result->l);
#endif
			break;
		case jboolean_type:
			result->z = env->CallStaticBooleanMethodA(clazz, methodID, args);
			break;
		case jbyte_type:
			result->b = env->CallStaticByteMethodA(clazz, methodID, args);
			break;
		case jchar_type:
			result->c = env->CallStaticCharMethodA(clazz, methodID, args);
			break;
		case jshort_type:
			result->s = env->CallStaticShortMethodA(clazz, methodID, args);
			break;
		case jint_type:
			result->i = env->CallStaticIntMethodA(clazz, methodID, args);
			break;
		case jlong_type:
			result->j = env->CallStaticLongMethodA(clazz, methodID, args);
			break;
		case jfloat_type:
			result->f = env->CallStaticFloatMethodA(clazz, methodID, args);
			break;
		case jdouble_type:
			result->d = env->CallStaticDoubleMethodA(clazz, methodID, args);
			break;
		case jvoid_type:
			env->CallStaticVoidMethodA(clazz, methodID, args);
			break;
		}
	}
};

NS_IMETHODIMP CSecureEnv::CallStaticMethod(  jni_type type,
                                              jclass clazz,
                                              jmethodID methodID,
                                              jvalue *args, 
                                             jvalue* result,
                                              nsISecurityContext* ctx)
{
    if (clazz == NULL || methodID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	
	
	CallStaticMethodMessage msg(type, clazz, methodID, args, result);
	sendMessageToJava(&msg);
#else
    #error "Implement me!"
#endif

	return NS_OK;
}













class GetStaticFieldMessage : public JavaMessage {
	jni_type type;
	jclass clazz;
	jfieldID fieldID;
	jvalue* result;
public:
	GetStaticFieldMessage(jni_type type, jclass clazz, jfieldID fieldID, jvalue* result)
	{
		this->type = type;
		this->clazz = clazz;
		this->fieldID = fieldID;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
		switch (type) {
		case jobject_type:
			result->l = env->GetStaticObjectField(clazz, fieldID);
#if LOCAL_REFS_ARE_GLOBAL
            result->l = ToGlobalRef(env, result->l);
#endif
			break;
		case jboolean_type:
			result->z = env->GetStaticBooleanField(clazz, fieldID);
			break;
		case jbyte_type:
			result->b = env->GetStaticByteField(clazz, fieldID);
			break;
		case jchar_type:
			result->c = env->GetStaticCharField(clazz, fieldID);
			break;
		case jshort_type:
			result->s = env->GetStaticShortField(clazz, fieldID);
			break;
		case jint_type:
			result->i = env->GetStaticIntField(clazz, fieldID);
			break;
		case jlong_type:
			result->j = env->GetStaticLongField(clazz, fieldID);
			break;
		case jfloat_type:
			result->f = env->GetStaticFloatField(clazz, fieldID);
			break;
		case jdouble_type:
			result->d = env->GetStaticDoubleField(clazz, fieldID);
			break;
		}
	}
};

NS_IMETHODIMP CSecureEnv::GetStaticField(  jni_type type,
                                            jclass clazz, 
                                            jfieldID fieldID,
                                           jvalue* result,
                                            nsISecurityContext* ctx)
{
    if (mJavaEnv == NULL || clazz == NULL || fieldID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    
	GetStaticFieldMessage msg(type, clazz, fieldID, result);
	sendMessageToJava(&msg);
#else
    
	
	JNIEnv* env = mJavaEnv;
	switch (type) {
	case jobject_type:
		result->l = env->GetStaticObjectField(clazz, fieldID);
		break;
	case jboolean_type:
		result->z = env->GetStaticBooleanField(clazz, fieldID);
		break;
	case jbyte_type:
		result->b = env->GetStaticByteField(clazz, fieldID);
		break;
	case jchar_type:
		result->c = env->GetStaticCharField(clazz, fieldID);
		break;
	case jshort_type:
		result->s = env->GetStaticShortField(clazz, fieldID);
		break;
	case jint_type:
		result->i = env->GetStaticIntField(clazz, fieldID);
		break;
	case jlong_type:
		result->j = env->GetStaticLongField(clazz, fieldID);
		break;
	case jfloat_type:
		result->f = env->GetStaticFloatField(clazz, fieldID);
		break;
	case jdouble_type:
		result->d = env->GetStaticDoubleField(clazz, fieldID);
		break;
	}
#endif

	return NS_OK;
}













class SetStaticFieldMessage : public JavaMessage {
	jni_type type;
	jclass clazz;
	jfieldID fieldID;
	jvalue val;
public:
	SetStaticFieldMessage(jni_type type, jclass clazz, jfieldID fieldID, jvalue val)
	{
		this->type = type;
		this->clazz = clazz;
		this->fieldID = fieldID;
		this->val = val;
	}
	
	virtual void execute(JNIEnv* env)
	{
    	switch (type) {
    	case jobject_type:
    		env->SetStaticObjectField(clazz, fieldID, val.l);
    		break;
    	case jboolean_type:
    		env->SetStaticBooleanField(clazz, fieldID, val.z);
    		break;
    	case jbyte_type:
    		env->SetStaticByteField(clazz, fieldID, val.b);
    		break;
    	case jchar_type:
    		env->SetStaticCharField(clazz, fieldID, val.c);
    		break;
    	case jshort_type:
    		env->SetStaticShortField(clazz, fieldID, val.s);
    		break;
    	case jint_type:
    		env->SetStaticIntField(clazz, fieldID, val.i);
    		break;
    	case jlong_type:
    		env->SetStaticLongField(clazz, fieldID, val.j);
    		break;
    	case jfloat_type:
    		env->SetStaticFloatField(clazz, fieldID, val.f);
    		break;
    	case jdouble_type:
    		env->SetStaticDoubleField(clazz, fieldID, val.d);
    		break;
    	}
	}
};

NS_IMETHODIMP CSecureEnv::SetStaticField(  jni_type type,
                                           jclass clazz, 
                                           jfieldID fieldID,
                                           jvalue val,
                                           nsISecurityContext* ctx)
{
    if (mJavaEnv == NULL || clazz == NULL || fieldID == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	
	SetStaticFieldMessage msg(type, clazz, fieldID, val);
	sendMessageToJava(&msg);
#else
	JNIEnv* env = mJavaEnv;
	switch (type) {
	case jobject_type:
		env->SetStaticObjectField(clazz, fieldID, val.l);
		break;
	case jboolean_type:
		env->SetStaticBooleanField(clazz, fieldID, val.z);
		break;
	case jbyte_type:
		env->SetStaticByteField(clazz, fieldID, val.b);
		break;
	case jchar_type:
		env->SetStaticCharField(clazz, fieldID, val.c);
		break;
	case jshort_type:
		env->SetStaticShortField(clazz, fieldID, val.s);
		break;
	case jint_type:
		env->SetStaticIntField(clazz, fieldID, val.i);
		break;
	case jlong_type:
		env->SetStaticLongField(clazz, fieldID, val.j);
		break;
	case jfloat_type:
		env->SetStaticFloatField(clazz, fieldID, val.f);
		break;
	case jdouble_type:
		env->SetStaticDoubleField(clazz, fieldID, val.d);
		break;
	}
#endif

	return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetVersion( jint* version) 
{
    if (mJavaEnv == NULL || version == NULL)
        return NS_ERROR_NULL_POINTER;
    
 	JNIEnv* env = mJavaEnv;
	*version = env->GetVersion();

    return NS_OK;
}




class DefineClassMessage : public JavaMessage {
	const char* name;
	jobject loader;
	const jbyte *buf;
	jsize len;
	jclass* clazz;
public:
	DefineClassMessage(const char* name, jobject loader, const jbyte *buf, jsize len, jclass* clazz)
	{
		this->name = name;
		this->loader = loader;
		this->buf = buf;
		this->len = len;
		this->clazz = clazz;
	}
	
	virtual void execute(JNIEnv* env)
	{
    	*clazz = env->DefineClass(name, loader, buf, len);
	}
};

NS_IMETHODIMP CSecureEnv::DefineClass(  const char* name,
                                         jobject loader,
                                         const jbyte *buf,
                                         jsize len,
                                        jclass* clazz) 
{
    if (mJavaEnv == NULL || clazz == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    DefineClassMessage msg(name, loader, buf, len, clazz);
    sendMessageToJava(&msg);
#else
    *clazz = mJavaEnv->DefineClass(name, loader, buf, len);
#endif

    return NS_OK;
}




class FindClassMessage : public JavaMessage {
	const char* name;
	jclass* result;
public:
	FindClassMessage(const char* name, jclass* result)
	{
		this->name = name;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->FindClass(name);
#if LOCAL_REFS_ARE_GLOBAL
        *result = (jclass) ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::FindClass(  const char* name, 
                                      jclass* clazz) 
{
    if (clazz == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	FindClassMessage msg(name, clazz);
	sendMessageToJava(&msg);
#else
	*clazz = mJavaEnv->FindClass(name);
#endif

    return NS_OK;
}




class GetSuperclassMessage : public JavaMessage {
	jclass sub;
	jclass* super;
public:
	GetSuperclassMessage(jclass sub, jclass* super)
	{
		this->sub = sub;
		this->super = super;
	}

	virtual void execute(JNIEnv* env)
	{
    	*super = env->GetSuperclass(sub);
	}
};

NS_IMETHODIMP CSecureEnv::GetSuperclass(  jclass sub,
                                          jclass* super) 
{
    if (mJavaEnv == NULL || super == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	GetSuperclassMessage msg(sub, super);
	sendMessageToJava(&msg);
#else
	*super = mJavaEnv->GetSuperclass(sub);
#endif

    return NS_OK;
}


class IsAssignableFromMessage : public JavaMessage {
    jclass sub;
    jclass super;
    jboolean* result;
public:
	IsAssignableFromMessage(jclass sub, jclass super, jboolean* result)
	{
		this->sub = sub;
		this->super = super;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
        *result = env->IsAssignableFrom(sub, super);
	}
};

NS_IMETHODIMP CSecureEnv::IsAssignableFrom(  jclass sub,
                                              jclass super,
                                             jboolean* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
	IsAssignableFromMessage msg(sub, super, result);
	sendMessageToJava(&msg);
#else
	
	*result = mJavaEnv->IsAssignableFrom(sub, super);
#endif

    return NS_OK;
}


class ThrowMessage : public JavaMessage {
    jthrowable obj;
    jint* result;
public:
	ThrowMessage(jthrowable obj, jint* result)
	{
		this->obj = obj;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
        *result = env->Throw(obj);
	}
};

NS_IMETHODIMP CSecureEnv::Throw(  jthrowable obj,
                                  jint* result) 
{
	if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
	ThrowMessage msg(obj, result);
	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->Throw(obj);
#endif

    return NS_OK;
}

class ThrowNewMessage : public JavaMessage {
    jclass clazz;
    const char* message;
    jint* result;
public:
	ThrowNewMessage(jclass clazz, const char* message, jint* result)
	{
		this->clazz = clazz;
		this->message = message;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
        *result = env->ThrowNew(clazz, message);
	}
};

NS_IMETHODIMP CSecureEnv::ThrowNew(  jclass clazz,
                                      const char *message,
                                     jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
	ThrowNewMessage msg(clazz, message, result);
	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->ThrowNew(clazz, msg);
#endif

    return NS_OK;
}


class ExceptionOccurredMessage : public JavaMessage {
    CSecureEnv* secureEnv;
    jthrowable* result;
public:
	ExceptionOccurredMessage(CSecureEnv* secureEnv, jthrowable* result)
	{
	    this->secureEnv = secureEnv;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
        *result = secureEnv->getPendingException(env);
#if LOCAL_REFS_ARE_GLOBAL
        *result = (jthrowable) ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::ExceptionOccurred( jthrowable* result)
{
	if (mJavaEnv == NULL || result == NULL)
		return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
	ExceptionOccurredMessage msg(this, result);
	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->ExceptionOccurred();
#endif

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::ExceptionDescribe(void)
{
	if (mJavaEnv == NULL)
		return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
	class ExceptionDescribeMessage : public JavaMessage {
    public:
    	virtual void execute(JNIEnv* env) { env->ExceptionDescribe(); }
    } msg;
	sendMessageToJava(&msg);
#else
    mJavaEnv->ExceptionDescribe();
#endif

    return NS_OK;
}

class ExceptionClearMessage : public JavaMessage {
    CSecureEnv* secureEnv;
public:
    ExceptionClearMessage(CSecureEnv* secureEnv)
    {
        this->secureEnv = secureEnv;
    }
	virtual void execute(JNIEnv* env) { secureEnv->clearPendingException(env); }
};

NS_IMETHODIMP CSecureEnv::ExceptionClear(void)
{
#if PROXY_JNI_CALLS
    ExceptionClearMessage msg(this); 
	sendMessageToJava(&msg);
#else
    mJavaEnv->ExceptionClear();
#endif

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::FatalError( const char* msg)
{
    mJavaEnv->FatalError(msg);
    return NS_OK;
}






class NewGlobalRefMessage : public JavaMessage {
	jobject localRef;
	jobject* result;
public:
	NewGlobalRefMessage(jobject localRef, jobject* result)
	{
		this->localRef = localRef;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->NewGlobalRef(localRef);
	}
};

NS_IMETHODIMP CSecureEnv::NewGlobalRef(  jobject localRef, 
                                         jobject* result)
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    NewGlobalRefMessage msg(localRef, result);
 	sendMessageToJava(&msg);
#else
	*result = mJavaEnv->NewGlobalRef(localRef);
#endif

    return NS_OK;
}





NS_IMETHODIMP CSecureEnv::DeleteGlobalRef( jobject globalRef) 
{
    mJavaEnv->DeleteGlobalRef(globalRef);
    return NS_OK;
}




class DeleteLocalRefMessage : public JavaMessage {
	jobject localRef;
public:
	DeleteLocalRefMessage(jobject localRef)
	{
		this->localRef = localRef;
	}

	virtual void execute(JNIEnv* env)
	{
#if LOCAL_REFS_ARE_GLOBAL
		env->DeleteGlobalRef(localRef);
#else
		env->DeleteLocalRef(localRef);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::DeleteLocalRef( jobject localRef)
{
#if PROXY_JNI_CALLS
    DeleteLocalRefMessage msg(localRef);
 	sendMessageToJava(&msg);
#else
    mJavaEnv->DeleteLocalRef(localRef);
#endif
    return NS_OK;
}

class IsSameObjectMessage : public JavaMessage {
	jobject obj1;
	jobject obj2;
	jboolean* result;
public:
	IsSameObjectMessage(jobject obj1, jobject obj2, jboolean* result)
	{
		this->obj1 = obj1;
		this->obj2 = obj2;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->IsSameObject(obj1, obj2);
	}
};

NS_IMETHODIMP CSecureEnv::IsSameObject(  jobject obj1,
                                          jobject obj2,
                                         jboolean* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
    IsSameObjectMessage msg(obj1, obj2, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->IsSameObject(obj1, obj2);
#endif

    return NS_OK;
}


class AllocObjectMessage : public JavaMessage {
	jclass clazz;
	jobject* result;
public:
	AllocObjectMessage(jclass clazz, jobject* result)
	{
		this->clazz = clazz;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->AllocObject(clazz);
#if LOCAL_REFS_ARE_GLOBAL
        *result = ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::AllocObject(  jclass clazz,
                                       jobject* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
    AllocObjectMessage msg(clazz, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->AllocObject(clazz);
#endif

    return NS_OK;
}

class GetObjectClassMessage : public JavaMessage {
    jobject obj;
	jclass* result;
public:
	GetObjectClassMessage(jobject obj, jclass* result)
	{
		this->obj = obj;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->GetObjectClass(obj);
#if LOCAL_REFS_ARE_GLOBAL
        *result = (jclass) ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::GetObjectClass(  jobject obj,
                                           jclass* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
    GetObjectClassMessage msg(obj, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetObjectClass(obj);
#endif

    return NS_OK;
}


class IsInstanceOfMessage : public JavaMessage {
    jobject obj;
    jclass clazz;
	jboolean* result;
public:
	IsInstanceOfMessage(jobject obj, jclass clazz, jboolean* result)
	{
		this->obj = obj;
		this->clazz = clazz;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->IsInstanceOf(obj, clazz);
	}
};

NS_IMETHODIMP CSecureEnv::IsInstanceOf(  jobject obj,
                                          jclass clazz,
                                         jboolean* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
    IsInstanceOfMessage msg(obj, clazz, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->IsInstanceOf(obj, clazz);
#endif

    return NS_OK;
}

class GetMethodIDMessage : public JavaMessage {
    jclass clazz;
    const char* name;
    const char* sig;
    jboolean isStatic;
	jmethodID* result;
public:
	GetMethodIDMessage(jclass clazz, const char* name, const char* sig,
	                   jboolean isStatic, jmethodID* result)
	{
		this->clazz = clazz;
		this->name = name;
		this->sig = sig;
		this->isStatic = isStatic;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = (isStatic ? env->GetStaticMethodID(clazz, name, sig)
		                    : env->GetMethodID(clazz, name, sig));
	}
};

class GetFieldIDMessage : public JavaMessage {
    jclass clazz;
    const char* name;
    const char* sig;
    jboolean isStatic;
	jfieldID* result;
public:
	GetFieldIDMessage(jclass clazz, const char* name, const char* sig,
	                   jboolean isStatic, jfieldID* result)
	{
		this->clazz = clazz;
		this->name = name;
		this->sig = sig;
		this->isStatic = isStatic;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = (isStatic ? env->GetStaticFieldID(clazz, name, sig)
		                    : env->GetFieldID(clazz, name, sig));
	}
};

NS_IMETHODIMP CSecureEnv::GetMethodID(  jclass clazz, 
                                         const char* name,
                                         const char* sig,
                                        jmethodID* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    GetMethodIDMessage msg(clazz, name, sig, JNI_FALSE, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetMethodID(clazz, name, sig);
#endif

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetFieldID(  jclass clazz, 
                                        const char* name,
                                        const char* sig,
                                       jfieldID* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
    GetFieldIDMessage msg(clazz, name, sig, JNI_FALSE, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetFieldID(clazz, name, sig);
#endif

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetStaticMethodID(  jclass clazz, 
                                               const char* name,
                                               const char* sig,
                                              jmethodID* result)
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
    GetMethodIDMessage msg(clazz, name, sig, JNI_TRUE, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetStaticMethodID(clazz, name, sig);
#endif

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetStaticFieldID(  jclass clazz, 
                                              const char* name,
                                              const char* sig,
                                             jfieldID* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS
    GetFieldIDMessage msg(clazz, name, sig, JNI_TRUE, result);
 	sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetStaticFieldID(clazz, name, sig);
#endif

    return NS_OK;
}





class NewStringMessage : public JavaMessage {
	const jchar* unicode;
	jsize len;
	jstring* result;
public:
	NewStringMessage(const jchar* unicode, jsize len, jstring* result)
	{
		this->unicode = unicode;
		this->len = len;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
    	*result = env->NewString(unicode, len);
#if LOCAL_REFS_ARE_GLOBAL
        *result = (jstring) ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::NewString(  const jchar* unicode,
                                       jsize len,
                                      jstring* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
    NewStringMessage msg(unicode, len, result);
    sendMessageToJava(&msg);
#else
	*result = mJavaEnv->NewString(unicode, len);
#endif

    return NS_OK;
}

class GetStringLengthMessage : public JavaMessage {
	jstring str;
	jsize* result;
public:
	GetStringLengthMessage(jstring str, jsize* result)
	{
		this->str = str;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
    	*result = env->GetStringLength(str);
	}
};

NS_IMETHODIMP CSecureEnv::GetStringLength(  jstring str,
                                            jsize* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
    GetStringLengthMessage msg(str, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetStringLength(str);
#endif

    return NS_OK;
}

class GetStringCharsMessage : public JavaMessage {
	jstring str;
	jboolean* isCopy;
	const jchar** result;
public:
	GetStringCharsMessage(jstring str, jboolean* isCopy, const jchar** result)
	{
		this->str = str;
		this->isCopy = isCopy;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
    	*result = env->GetStringChars(str, isCopy);
	}
};

NS_IMETHODIMP CSecureEnv::GetStringChars(  jstring str,
                                            jboolean *isCopy,
                                           const jchar** result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
    GetStringCharsMessage msg(str, isCopy, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetStringChars(str, isCopy);
#endif

    return NS_OK;
}


class ReleaseStringCharsMessage : public JavaMessage {
	jstring str;
	const jchar* chars;
public:
	ReleaseStringCharsMessage(jstring str, const jchar* chars)
	{
		this->str = str;
		this->chars = chars;
	}

	virtual void execute(JNIEnv* env)
	{
    	env->ReleaseStringChars(str, chars);
	}
};

NS_IMETHODIMP CSecureEnv::ReleaseStringChars(  jstring str,
                                                const jchar *chars) 
{
    if (mJavaEnv == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
    ReleaseStringCharsMessage msg(str, chars);
    sendMessageToJava(&msg);
#else
    mJavaEnv->ReleaseStringChars(str, chars);
#endif

    return NS_OK;
}

class NewStringUTFMessage : public JavaMessage {
	const char *utf;
	jstring* result;
public:
	NewStringUTFMessage(const char *utf, jstring* result)
	{
		this->utf = utf;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
    	*result = env->NewStringUTF(utf);
#if LOCAL_REFS_ARE_GLOBAL
        *result = (jstring) ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::NewStringUTF(  const char *utf,
                                         jstring* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
	NewStringUTFMessage msg(utf, result);
    sendMessageToJava(&msg);
#else
	*result = mJavaEnv->NewStringUTF(utf);
#endif

    return NS_OK;
}


class GetStringUTFLengthMessage : public JavaMessage {
	jstring str;
	jsize* result;
public:
	GetStringUTFLengthMessage(jstring str, jsize* result)
	{
		this->str = str;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
    	*result = env->GetStringUTFLength(str);
	}
};

NS_IMETHODIMP CSecureEnv::GetStringUTFLength(  jstring str,
                                               jsize* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
	GetStringUTFLengthMessage msg(str, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetStringUTFLength(str);
#endif

    return NS_OK;
}

    
class GetStringUTFCharsMessage : public JavaMessage {
	jstring str;
	jboolean* isCopy;
	const char** result;
public:
	GetStringUTFCharsMessage(jstring str, jboolean* isCopy, const char** result)
	{
		this->str = str;
		this->isCopy = isCopy;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
    	*result = env->GetStringUTFChars(str, isCopy);
	}
};

NS_IMETHODIMP CSecureEnv::GetStringUTFChars(  jstring str,
                                               jboolean *isCopy,
                                              const char** result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
	GetStringUTFCharsMessage msg(str, isCopy, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetStringUTFChars(str, isCopy);
#endif

    return NS_OK;
}


class ReleaseStringUTFCharsMessage : public JavaMessage {
	jstring str;
	const char* chars;
public:
	ReleaseStringUTFCharsMessage(jstring str, const char* chars)
	{
		this->str = str;
		this->chars = chars;
	}

	virtual void execute(JNIEnv* env)
	{
    	env->ReleaseStringUTFChars(str, chars);
	}
};

NS_IMETHODIMP CSecureEnv::ReleaseStringUTFChars(  jstring str,
                                                   const char *chars) 
{
    if (mJavaEnv == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
	ReleaseStringUTFCharsMessage msg(str, chars);
    sendMessageToJava(&msg);
#else
    mJavaEnv->ReleaseStringUTFChars(str, chars);
#endif

    return NS_OK;
}


class GetArrayLengthMessage : public JavaMessage {
	jarray array;
	jsize* result;
public:
	GetArrayLengthMessage(jarray array, jsize* result)
	{
		this->array = array;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
    	*result = env->GetArrayLength(array);
	}
};

NS_IMETHODIMP CSecureEnv::GetArrayLength(  jarray array,
                                          jsize* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
#if PROXY_JNI_CALLS    
	GetArrayLengthMessage msg(array, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetArrayLength(array);
#endif

    return NS_OK;
}

class NewObjectArrayMessage : public JavaMessage {
    jsize len;
	jclass clazz;
	jobject init;
	jobjectArray* result;
public:
	NewObjectArrayMessage(jsize len, jclass clazz, jobject init, jobjectArray* result)
	{
	    this->len = len;
		this->clazz = clazz;
		this->init = init;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->NewObjectArray(len, clazz, init);
#if LOCAL_REFS_ARE_GLOBAL
        *result = (jobjectArray) ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::NewObjectArray(  jsize len,
										   jclass clazz,
					                       jobject init,
					                      jobjectArray* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
	NewObjectArrayMessage msg(len, clazz, init, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->NewObjectArray(len, clazz, init);
#endif

    return NS_OK;
}

class GetObjectArrayElementMessage : public JavaMessage {
	jobjectArray array;
	jsize index;
	jobject* result;
public:
	GetObjectArrayElementMessage(jobjectArray array, jsize index, jobject* result)
	{
		this->array = array;
		this->index = index;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
        *result = env->GetObjectArrayElement(array, index);
#if LOCAL_REFS_ARE_GLOBAL
        *result = ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::GetObjectArrayElement(  jobjectArray array,
                                                   jsize index,
                                                  jobject* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
	GetObjectArrayElementMessage msg(array, index, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->GetObjectArrayElement(array, index);
#endif

    return NS_OK;
}


class SetObjectArrayElementMessage : public JavaMessage {
	jobjectArray array;
	jsize index;
	jobject val;
public:
	SetObjectArrayElementMessage(jobjectArray array, jsize index, jobject val)
	{
		this->array = array;
		this->index = index;
		this->val = val;
	}
	
	virtual void execute(JNIEnv* env)
	{
        env->SetObjectArrayElement(array, index, val);
	}
};

NS_IMETHODIMP CSecureEnv::SetObjectArrayElement(  jobjectArray array,
                                                   jsize index,
                                                   jobject val) 
{
    if (mJavaEnv == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
	SetObjectArrayElementMessage msg(array, index, val);
    sendMessageToJava(&msg);
#else
    mJavaEnv->SetObjectArrayElement(array, index, val);
#endif

    return NS_OK;
}

class NewArrayMessage : public JavaMessage {
	jni_type element_type;
	jsize len;
	jarray* result;
public:
	NewArrayMessage(jni_type element_type, jsize len, jarray* result)
	{
		this->element_type = element_type;
		this->len = len;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
        switch (element_type) {
        case jboolean_type:
            *result = env->NewBooleanArray(len);
            break;
        case jbyte_type:
            *result = env->NewByteArray(len);
            break;
        case jchar_type:
            *result = env->NewCharArray(len);
            break;
        case jshort_type:
            *result = env->NewShortArray(len);
            break;
        case jint_type:
            *result = env->NewIntArray(len);
            break;
        case jlong_type:
            *result = env->NewLongArray(len);
            break;
        case jfloat_type:
            *result = env->NewFloatArray(len);
            break;
        case jdouble_type:
            *result = env->NewDoubleArray(len);
            break;
        default:
            *result = NULL;
        }
#if LOCAL_REFS_ARE_GLOBAL
        *result = (jarray) ToGlobalRef(env, *result);
#endif
	}
};

NS_IMETHODIMP CSecureEnv::NewArray( jni_type element_type,
                        			  jsize len,
                        			 jarray* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS    
	NewArrayMessage msg(element_type, len, result);
    sendMessageToJava(&msg);
#else
    JNIEnv* env = mJavaEnv;
    switch (element_type) {
    case jboolean_type:
        *result = env->NewBooleanArray(len);
        break;
    case jbyte_type:
        *result = env->NewByteArray(len);
        break;
    case jchar_type:
        *result = env->NewCharArray(len);
        break;
    case jshort_type:
        *result = env->NewShortArray(len);
        break;
    case jint_type:
        *result = env->NewIntArray(len);
        break;
    case jlong_type:
        *result = env->NewLongArray(len);
        break;
    case jfloat_type:
        *result = env->NewFloatArray(len);
        break;
    case jdouble_type:
        *result = env->NewDoubleArray(len);
        break;
    default:
        *result = NULL;
    }
#endif

    return NS_OK;
}

class GetArrayElementsMessage : public JavaMessage {
	jni_type element_type;
	jarray array;
	jboolean* isCopy;
	void* result;
public:
	GetArrayElementsMessage(jni_type element_type, jarray array, jboolean* isCopy, void* result)
	{
		this->element_type = element_type;
		this->array = array;
		this->isCopy = isCopy;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
        switch (element_type) {
    	case jboolean_type:
    	    *(jboolean**) result = env->GetBooleanArrayElements((jbooleanArray)array, isCopy);
    	    break;
    	case jbyte_type:
    	    *(jbyte**) result = env->GetByteArrayElements((jbyteArray)array, isCopy);
    	    break;
    	case jchar_type:
    	    *(jchar**) result = env->GetCharArrayElements((jcharArray)array, isCopy);
    	    break;
    	case jshort_type:
    	    * (jshort**) result = env->GetShortArrayElements((jshortArray)array, isCopy);
    	    break;
    	case jint_type:
    	    * (jint**) result = env->GetIntArrayElements((jintArray)array, isCopy);
    	    break;
    	case jlong_type:
    	    * (jlong**) result = env->GetLongArrayElements((jlongArray)array, isCopy);
    	    break;
    	case jfloat_type:
    	    * (jfloat**) result = env->GetFloatArrayElements((jfloatArray)array, isCopy);
    	    break;
    	case jdouble_type:
    	    * (jdouble**) result = env->GetDoubleArrayElements((jdoubleArray)array, isCopy);
    	    break;
    	default:
    	    *(void**)result = NULL;
    	}
	}
};

NS_IMETHODIMP CSecureEnv::GetArrayElements(  jni_type element_type,
                                             jarray array,
                                             jboolean *isCopy,
                                            void* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    GetArrayElementsMessage msg(element_type, array, isCopy, result);
    sendMessageToJava(&msg);
#else
    JNIEnv* env = mJavaEnv;
    switch (element_type) {
	case jboolean_type:
	    *(jboolean**) result = env->GetBooleanArrayElements((jbooleanArray)array, isCopy);
	    break;
	case jbyte_type:
	    *(jbyte**) result = env->GetByteArrayElements((jbyteArray)array, isCopy);
	    break;
	case jchar_type:
	    *(jchar**) result = env->GetCharArrayElements((jcharArray)array, isCopy);
	    break;
	case jshort_type:
	    * (jshort**) result = env->GetShortArrayElements((jshortArray)array, isCopy);
	    break;
	case jint_type:
	    * (jint**) result = env->GetIntArrayElements((jintArray)array, isCopy);
	    break;
	case jlong_type:
	    * (jlong**) result = env->GetLongArrayElements((jlongArray)array, isCopy);
	    break;
	case jfloat_type:
	    * (jfloat**) result = env->GetFloatArrayElements((jfloatArray)array, isCopy);
	    break;
	case jdouble_type:
	    * (jdouble**) result = env->GetDoubleArrayElements((jdoubleArray)array, isCopy);
	    break;
	default:
	    *(void**)result = NULL;
	}
#endif

    return NS_OK;
}


class ReleaseArrayElementsMessage : public JavaMessage {
	jni_type element_type;
	jarray array;
	void* elems;
	jint mode;
public:
	ReleaseArrayElementsMessage(jni_type element_type, jarray array, void* elems, jint mode)
	{
		this->element_type = element_type;
		this->array = array;
		this->elems = elems;
		this->mode = mode;
	}

	virtual void execute(JNIEnv* env)
	{
    	switch (element_type) {
    	case jboolean_type:
    		env->ReleaseBooleanArrayElements((jbooleanArray)array, (jboolean*)elems, mode);
    		break;
    	case jbyte_type:
    		env->ReleaseByteArrayElements((jbyteArray)array, (jbyte*)elems, mode);
    		break;
    	case jchar_type:
    		env->ReleaseCharArrayElements((jcharArray)array, (jchar*)elems, mode);
    		break;
    	case jshort_type:
    		env->ReleaseShortArrayElements((jshortArray)array, (jshort*)elems, mode);
    		break;
    	case jint_type:
    		env->ReleaseIntArrayElements((jintArray)array, (jint*)elems, mode);
    		break;
    	case jlong_type:
    		env->ReleaseLongArrayElements((jlongArray)array, (jlong*)elems, mode);
    		break;
    	case jfloat_type:
    		env->ReleaseFloatArrayElements((jfloatArray)array, (jfloat*)elems, mode);
    		break;
    	case jdouble_type:
    		env->ReleaseDoubleArrayElements((jdoubleArray)array, (jdouble*)elems, mode);
    		break;
    	}
	}
};

NS_IMETHODIMP CSecureEnv::ReleaseArrayElements( jni_type element_type,
                                                 jarray array,
                                                 void *elems,
                                                 jint mode) 
{
	if (mJavaEnv == NULL)
		return NS_ERROR_NULL_POINTER;
	
#if PROXY_JNI_CALLS
    ReleaseArrayElementsMessage msg(element_type, array, elems, mode);
    sendMessageToJava(&msg);
#else
	JNIEnv* env = mJavaEnv;
	switch (element_type) {
	case jboolean_type:
		env->ReleaseBooleanArrayElements((jbooleanArray)array, (jboolean*)elems, mode);
		break;
	case jbyte_type:
		env->ReleaseByteArrayElements((jbyteArray)array, (jbyte*)elems, mode);
		break;
	case jchar_type:
		env->ReleaseCharArrayElements((jcharArray)array, (jchar*)elems, mode);
		break;
	case jshort_type:
		env->ReleaseShortArrayElements((jshortArray)array, (jshort*)elems, mode);
		break;
	case jint_type:
		env->ReleaseIntArrayElements((jintArray)array, (jint*)elems, mode);
		break;
	case jlong_type:
		env->ReleaseLongArrayElements((jlongArray)array, (jlong*)elems, mode);
		break;
	case jfloat_type:
		env->ReleaseFloatArrayElements((jfloatArray)array, (jfloat*)elems, mode);
		break;
	case jdouble_type:
		env->ReleaseDoubleArrayElements((jdoubleArray)array, (jdouble*)elems, mode);
		break;
	}
#endif

    return NS_OK;
}

class GetArrayRegionMessage : public JavaMessage {
	jni_type element_type;
	jarray array;
	jsize start;
	jsize len;
	void* buf;
public:
	GetArrayRegionMessage(jni_type element_type, jarray array, jsize start, jsize len, void* buf)
	{
		this->element_type = element_type;
		this->array = array;
		this->start = start;
		this->len = len;
		this->buf = buf;
	}

	virtual void execute(JNIEnv* env)
	{
        switch (element_type) {
        case jboolean_type:
            env->GetBooleanArrayRegion((jbooleanArray)array, start, len, (jboolean*)buf);
            break;
        case jbyte_type:
            env->GetByteArrayRegion((jbyteArray)array, start, len, (jbyte*)buf);
            break;
        case jchar_type:
            env->GetCharArrayRegion((jcharArray)array, start, len, (jchar*)buf);
            break;
        case jshort_type:
            env->GetShortArrayRegion((jshortArray)array, start, len, (jshort*)buf);
            break;
        case jint_type:
            env->GetIntArrayRegion((jintArray)array, start, len, (jint*)buf);
            break;
        case jlong_type:
            env->GetLongArrayRegion((jlongArray)array, start, len, (jlong*)buf);
            break;
        case jfloat_type:
            env->GetFloatArrayRegion((jfloatArray)array, start, len, (jfloat*)buf);
            break;
        case jdouble_type:
            env->GetDoubleArrayRegion((jdoubleArray)array, start, len, (jdouble*)buf);
            break;
        }
	}
};

NS_IMETHODIMP CSecureEnv::GetArrayRegion(  jni_type element_type,
                                           jarray array,
                                           jsize start,
                                           jsize len,
                                          void* buf)
{
    if (mJavaEnv == NULL || buf == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    GetArrayRegionMessage msg(element_type, array, start, len, buf);
    sendMessageToJava(&msg);
#else
    JNIEnv* env = mJavaEnv;
    switch (element_type) {
    case jboolean_type:
        env->GetBooleanArrayRegion((jbooleanArray)array, start, len, (jboolean*)buf);
        break;
    case jbyte_type:
        env->GetByteArrayRegion((jbyteArray)array, start, len, (jbyte*)buf);
        break;
    case jchar_type:
        env->GetCharArrayRegion((jcharArray)array, start, len, (jchar*)buf);
        break;
    case jshort_type:
        env->GetShortArrayRegion((jshortArray)array, start, len, (jshort*)buf);
        break;
    case jint_type:
        env->GetIntArrayRegion((jintArray)array, start, len, (jint*)buf);
        break;
    case jlong_type:
        env->GetLongArrayRegion((jlongArray)array, start, len, (jlong*)buf);
        break;
    case jfloat_type:
        env->GetFloatArrayRegion((jfloatArray)array, start, len, (jfloat*)buf);
        break;
    case jdouble_type:
        env->GetDoubleArrayRegion((jdoubleArray)array, start, len, (jdouble*)buf);
        break;
    default:
        return NS_ERROR_FAILURE;
    }
#endif

    return NS_OK;
}


class SetArrayRegionMessage : public JavaMessage {
	jni_type element_type;
	jarray array;
	jsize start;
	jsize len;
	void* buf;
public:
	SetArrayRegionMessage(jni_type element_type, jarray array, jsize start, jsize len, void* buf)
	{
		this->element_type = element_type;
		this->array = array;
		this->start = start;
		this->len = len;
		this->buf = buf;
	}

	virtual void execute(JNIEnv* env)
	{
        switch (element_type) {
        case jboolean_type:
            env->SetBooleanArrayRegion((jbooleanArray)array, start, len, (jboolean*)buf);
            break;
        case jbyte_type:
            env->SetByteArrayRegion((jbyteArray)array, start, len, (jbyte*)buf);
            break;
        case jchar_type:
            env->SetCharArrayRegion((jcharArray)array, start, len, (jchar*)buf);
            break;
        case jshort_type:
            env->SetShortArrayRegion((jshortArray)array, start, len, (jshort*)buf);
            break;
        case jint_type:
            env->SetIntArrayRegion((jintArray)array, start, len, (jint*)buf);
            break;
        case jlong_type:
            env->SetLongArrayRegion((jlongArray)array, start, len, (jlong*)buf);
            break;
        case jfloat_type:
            env->SetFloatArrayRegion((jfloatArray)array, start, len, (jfloat*)buf);
            break;
        case jdouble_type:
            env->SetDoubleArrayRegion((jdoubleArray)array, start, len, (jdouble*)buf);
            break;
        }
	}
};

NS_IMETHODIMP CSecureEnv::SetArrayRegion(  jni_type element_type,
                                           jarray array,
                                           jsize start,
                                           jsize len,
                                           void* buf)
{
    if (mJavaEnv == NULL || buf == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    SetArrayRegionMessage msg(element_type, array, start, len, buf);
    sendMessageToJava(&msg);
#else
    JNIEnv* env = mJavaEnv;
    switch (type) {
    case jboolean_type:
        env->SetBooleanArrayRegion((jbooleanArray)array, start, len, (jboolean*)buf);
        break;
    case jbyte_type:
        env->SetByteArrayRegion((jbyteArray)array, start, len, (jbyte*)buf);
        break;
    case jchar_type:
        env->SetCharArrayRegion((jcharArray)array, start, len, (jchar*)buf);
        break;
    case jshort_type:
        env->SetShortArrayRegion((jshortArray)array, start, len, (jshort*)buf);
        break;
    case jint_type:
        env->SetIntArrayRegion((jintArray)array, start, len, (jint*)buf);
        break;
    case jlong_type:
        env->SetLongArrayRegion((jlongArray)array, start, len, (jlong*)buf);
        break;
    case jfloat_type:
        env->SetFloatArrayRegion((jfloatArray)array, start, len, (jfloat*)buf);
        break;
    case jdouble_type:
        env->SetDoubleArrayRegion((jdoubleArray)array, start, len, (jdouble*)buf);
        break;
    default:
        return NS_ERROR_FAILURE;
    }
#endif

    return NS_OK;
}


class RegisterNativesMessage : public JavaMessage {
	jclass clazz;
	const JNINativeMethod *methods;
	jint nMethods;
	jint* result;
public:
	RegisterNativesMessage(jclass clazz, const JNINativeMethod *methods, jint nMethods, jint* result)
	{
		this->clazz = clazz;
		this->methods = methods;
		this->nMethods = nMethods;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
    	*result = env->RegisterNatives(clazz, methods, nMethods);
	}
};

NS_IMETHODIMP CSecureEnv::RegisterNatives(  jclass clazz,
                                             const JNINativeMethod *methods,
                                             jint nMethods,
                                            jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    RegisterNativesMessage msg(clazz, methods, nMethods, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->RegisterNatives(clazz, methods, nMethods);
#endif

    return NS_OK;
}


class UnregisterNativesMessage : public JavaMessage {
	jclass clazz;
	jint* result;
public:
	UnregisterNativesMessage(jclass clazz, jint* result)
	{
		this->clazz = clazz;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
    	*result = env->UnregisterNatives(clazz);
	}
};

NS_IMETHODIMP CSecureEnv::UnregisterNatives(  jclass clazz,
                                              jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
    UnregisterNativesMessage msg(clazz, result);
    sendMessageToJava(&msg);
#else
    *result = mJavaEnv->UnregisterNatives(clazz);
#endif

    return NS_OK;
}


class MonitorEnterMessage : public JavaMessage {
	jobject obj;
	jint* result;
public:
	MonitorEnterMessage(jobject obj, jint* result)
	{
		this->obj = obj;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->MonitorEnter(obj);
	}
};

NS_IMETHODIMP CSecureEnv::MonitorEnter(  jobject obj,
                                         jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	MonitorEnterMessage msg(obj, result);
	sendMessageToJava(&msg);
#else
	*result = mJavaEnv->MonitorEnter(obj);
#endif

    return NS_OK;
}

class MonitorExitMessage : public JavaMessage {
	jobject obj;
	jint* result;
public:
	MonitorExitMessage(jobject obj, jint* result)
	{
		this->obj = obj;
		this->result = result;
	}

	virtual void execute(JNIEnv* env)
	{
		*result = env->MonitorExit(obj);
	}
};
 
NS_IMETHODIMP CSecureEnv::MonitorExit(  jobject obj,
                                        jint* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

#if PROXY_JNI_CALLS
	MonitorExitMessage msg(obj, result);
	sendMessageToJava(&msg);
#else
	*result = mJavaEnv->MonitorExit(obj);
#endif

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::GetJavaVM(  JavaVM **vm,
                                      jint* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

    *result = mJavaEnv->GetJavaVM(vm);

    return NS_OK;
}





void CSecureEnv::messageLoop(JNIEnv* env, JavaMessage* msg, JavaMessageQueue* sendQueue, JavaMessageQueue* receiveQueue, Boolean busyWaiting)
{
	
	sendQueue->putMessage(msg);
	sendQueue->notify();
	JavaMessage* replyMsg = receiveQueue->getMessage();
	for (;;) {
		
		if (replyMsg != NULL) {
			if (replyMsg == msg)
				break;
			
			replyMsg->execute(env);
			sendQueue->putMessage(replyMsg);
			sendQueue->notify();
			
			replyMsg = receiveQueue->getMessage();
			if (replyMsg != NULL)
				continue;
		}
		
		if (busyWaiting) {
			receiveQueue->wait(1024);
			replyMsg = receiveQueue->getMessage();
			if (replyMsg != NULL)
				continue;
			
			mThreadManager->Sleep();
			
		} else {
			
			receiveQueue->wait();
		}
		replyMsg = receiveQueue->getMessage();
	}
}

void CSecureEnv::savePendingException(JNIEnv* env)
{
    
    jthrowable pendingException = env->ExceptionOccurred();
    if (pendingException) {
#if USE_LIVECONNECT_PROXY    
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
        
        if (mPendingException)
            env->DeleteGlobalRef(mPendingException);

        mPendingException = (jthrowable) env->NewGlobalRef(pendingException);
        env->DeleteLocalRef(pendingException);
    }
}

jthrowable CSecureEnv::getPendingException(JNIEnv* env)
{
    if (mPendingException)
        return (jthrowable) env->NewLocalRef(mPendingException);
    return NULL;
}

void CSecureEnv::clearPendingException(JNIEnv* env)
{
    if (mPendingException) {
        env->DeleteGlobalRef(mPendingException);
        mPendingException = NULL;
    }
}

CSecureEnv* GetSecureJNI(JNIEnv* env, jobject thread)
{
	CSecureEnv* secureJNI = NULL;
	
	jclass threadClass = env->GetObjectClass(thread);
	if (threadClass != NULL) {
		jfieldID fSecureEnvField = env->GetFieldID(threadClass, "fSecureEnv", "I");
		if (fSecureEnvField != NULL) {
			secureJNI = (CSecureEnv*) env->GetIntField(thread, fSecureEnvField);
		} else {
			env->ExceptionClear();
		}
		env->DeleteLocalRef(threadClass);
	}
	
	return secureJNI;
}
