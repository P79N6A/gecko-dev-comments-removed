









































	

#include "CSecureEnv.h"
#include "nsISecurityContext.h"

#include "MRJPlugin.h"
#include "MRJSession.h"
#include "nsIThreadManager.h"
#include "nsIJVMManager.h"

#include "MRJMonitor.h"
#include "NativeMonitor.h"
#include "JavaMessageQueue.h"

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
				replies.putMessage(msg);
				replies.notify();
			} else {
				
				
				
				requests.wait();
			}
		}
		
		requests.exit();
	}
}





static void CreateJNIThread(CSecureEnv* secureEnv)
{
	nsIThreadManager* manager = secureEnv->getThreadManager();
	MRJSession* session = secureEnv->getSession();
	JNIEnv* env = session->getCurrentEnv();
	jclass JNIThreadClass = env->FindClass("netscape/oji/JNIThread");
	if (JNIThreadClass != NULL) {
		JNINativeMethod method = { "run", "()V", &netscape_oji_JNIThread_run };
		env->RegisterNatives(JNIThreadClass, &method, 1);
		jmethodID constructorID = env->GetMethodID(JNIThreadClass, "<init>", "(I)V");
		if (constructorID != NULL) {
			jobject javaThread = env->NewObject(JNIThreadClass, constructorID, secureEnv);
			for (;;) {
				
				session->idle(kDefaultJMTime);
				
				if (secureEnv->isInitialized())
					break;
				
				manager->Sleep();
			}
		}
	}
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
					env->CallStaticVoidMethod(threadClass, sleepMethod, jlong(kDefaultJMTime));
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
	messageLoop(mProxyEnv, msg, mJavaQueue, mNativeQueue, true);
}




void CSecureEnv::sendMessageFromJava(JNIEnv* javaEnv, JavaMessage* msg, Boolean busyWaiting)
{
	messageLoop(javaEnv, msg, mNativeQueue, mJavaQueue, busyWaiting);
}






#if 0
NS_IMPL_AGGREGATED(CSecureEnv)
NS_INTERFACE_MAP_BEGIN_AGGREGATED(CSecureEnv)
    NS_INTERFACE_MAP_ENTRY(nsISecureEnv)
    NS_INTERFACE_MAP_ENTRY(nsIRunnable)
NS_INTERFACE_MAP_END
#endif

const InterfaceInfo CSecureEnv::sInterfaces[] = {
	{ NS_ISECUREENV_IID, INTERFACE_OFFSET(CSecureEnv, nsISecureEnv) },
	{ NS_IRUNNABLE_IID, INTERFACE_OFFSET(CSecureEnv, nsIRunnable) },
};
const UInt32 CSecureEnv::kInterfaceCount = sizeof(sInterfaces) / sizeof(InterfaceInfo);













CSecureEnv::CSecureEnv(MRJPlugin* plugin, JNIEnv* proxyEnv, JNIEnv* javaEnv)
	:	SupportsMixin(this, sInterfaces, kInterfaceCount),
		mPlugin(plugin), mProxyEnv(proxyEnv), mJavaEnv(javaEnv),
		mSession(plugin->getSession()), mThreadManager(plugin->getThreadManager()),
		mIsRunning(NULL), mJavaQueue(NULL), mNativeQueue(NULL)
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
	nsresult result = secureEnv->QueryInterface(aIID, aInstancePtr);
	if (result != NS_OK) {
		delete secureEnv;
		return result;
	}
	return NS_OK;
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

    
    NewObjectMessage msg(clazz, methodID, args, result);
    sendMessageToJava(&msg);
	
	
	return NS_OK;
}














class CallMethodMessage : public JavaMessage {
	jni_type type;
	jobject obj;
	jmethodID methodID;
	jvalue* args;
	jvalue* result;

public:
	CallMethodMessage(jni_type type, jobject obj, jmethodID methodID, jvalue *args, jvalue* result)
	{
		this->type = type;
		this->obj = obj;
		this->methodID = methodID;
		this->args = args;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
		switch (type) {
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
	}
};

NS_IMETHODIMP CSecureEnv::CallMethod(  jni_type type,
                                        jobject obj, 
                                        jmethodID methodID, 
                                        jvalue *args, 
                                       jvalue* result,
                                        nsISecurityContext* ctx)
{
    if (obj == NULL || methodID == NULL)
        return NS_ERROR_NULL_POINTER;

	
	
	CallMethodMessage msg(type, obj, methodID, args, result);
	sendMessageToJava(&msg);
	
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

	
	
	CallNonvirtualMethodMessage msg(type, obj, clazz, methodID, args, result);
	sendMessageToJava(&msg);
	
	return NS_OK;
}













class GetFieldMessage : public JavaMessage {
	jni_type type;
	jobject obj;
	jfieldID fieldID;
	jvalue* args;
	jvalue* result;

public:
	GetFieldMessage(jni_type type, jobject obj, jfieldID fieldID, jvalue *args, jvalue* result)
	{
		this->type = type;
		this->obj = obj;
		this->fieldID = fieldID;
		this->args = args;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
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
	
	return NS_OK;
}













NS_IMETHODIMP CSecureEnv::SetField( jni_type type,
                                     jobject obj, 
                                     jfieldID fieldID,
                                     jvalue val,
                                     nsISecurityContext* ctx)
{
    if (mJavaEnv == NULL || obj == NULL || fieldID == NULL)
        return NS_ERROR_NULL_POINTER;

    
    

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

	
	
	CallStaticMethodMessage msg(type, clazz, methodID, args, result);
	sendMessageToJava(&msg);

	return NS_OK;
}














class GetStaticFieldMessage : public JavaMessage {
	jni_type type;
	jclass clazz;
	jfieldID fieldID;
	jvalue* args;
	jvalue* result;

public:
	GetStaticFieldMessage(jni_type type, jclass clazz, jfieldID fieldID, jvalue* result)
	{
		this->type = type;
		this->clazz = clazz;
		this->fieldID = fieldID;
		this->args = args;
		this->result = result;
	}
	
	virtual void execute(JNIEnv* env)
	{
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
	
	return NS_OK;
}












NS_IMETHODIMP CSecureEnv::SetStaticField(  jni_type type,
                                           jclass clazz, 
                                           jfieldID fieldID,
                                           jvalue val,
                                           nsISecurityContext* ctx)
{
    if (mJavaEnv == NULL || clazz == NULL || fieldID == NULL)
        return NS_ERROR_NULL_POINTER;

	
	

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
	
	return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetVersion( jint* version) 
{
    if (version == NULL)
        return NS_ERROR_NULL_POINTER;
    
 	JNIEnv* env = mSession->getCurrentEnv();
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
    if (clazz == NULL)
        return NS_ERROR_NULL_POINTER;
    
    DefineClassMessage msg(name, loader, buf, len, clazz);
    sendMessageToJava(&msg);

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
	}
};

NS_IMETHODIMP CSecureEnv::FindClass(  const char* name, 
                                      jclass* clazz) 
{
    if (clazz == NULL)
        return NS_ERROR_NULL_POINTER;
    
	
	
	FindClassMessage msg(name, clazz);
	sendMessageToJava(&msg);

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
    
	
	
	*super = mJavaEnv->GetSuperclass(sub);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::IsAssignableFrom(  jclass sub,
                                              jclass super,
                                             jboolean* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
	
	
	*result = mJavaEnv->IsAssignableFrom(sub, super);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::Throw(  jthrowable obj,
                                  jint* result) 
{
	if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
	
    
    *result = mJavaEnv->Throw(obj);

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::ThrowNew(  jclass clazz,
                                      const char *msg,
                                     jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->ThrowNew(clazz, msg);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::ExceptionOccurred( jthrowable* result)
{
	if (mJavaEnv == NULL || result == NULL)
		return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->ExceptionOccurred();

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::ExceptionDescribe(void)
{
	if (mJavaEnv == NULL)
		return NS_ERROR_NULL_POINTER;

    mJavaEnv->ExceptionDescribe();

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::ExceptionClear(void)
{
    mJavaEnv->ExceptionClear();

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::FatalError( const char* msg)
{
    mJavaEnv->FatalError(msg);

    return NS_OK;
}





NS_IMETHODIMP CSecureEnv::NewGlobalRef(  jobject localRef, 
                                         jobject* result)
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
	
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
	} msg(localRef, result);
 	sendMessageToJava(&msg);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::DeleteGlobalRef( jobject globalRef) 
{
	JNIEnv* env = mSession->getCurrentEnv();
    env->DeleteGlobalRef(globalRef);
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
		env->DeleteLocalRef(localRef);
	}
};

NS_IMETHODIMP CSecureEnv::DeleteLocalRef( jobject obj)
{
    mJavaEnv->DeleteLocalRef(obj);
    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::IsSameObject(  jobject obj1,
                                          jobject obj2,
                                         jboolean* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->IsSameObject(obj1, obj2);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::AllocObject(  jclass clazz,
                                        jobject* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->AllocObject(clazz);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetObjectClass(  jobject obj,
                                           jclass* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->GetObjectClass(obj);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::IsInstanceOf(  jobject obj,
                                          jclass clazz,
                                         jboolean* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->IsInstanceOf(obj, clazz);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetMethodID(  jclass clazz, 
                                         const char* name,
                                         const char* sig,
                                        jmethodID* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    
    JNIEnv* env = mSession->getCurrentEnv();
    *result = env->GetMethodID(clazz, name, sig);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetFieldID(  jclass clazz, 
                                        const char* name,
                                        const char* sig,
                                       jfieldID* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    
    JNIEnv* env = mSession->getCurrentEnv();
    *result = env->GetFieldID(clazz, name, sig);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetStaticMethodID(  jclass clazz, 
                                               const char* name,
                                               const char* sig,
                                              jmethodID* result)
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    
    JNIEnv* env = mSession->getCurrentEnv();
    *result = env->GetStaticMethodID(clazz, name, sig);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetStaticFieldID(  jclass clazz, 
                                              const char* name,
                                              const char* sig,
                                             jfieldID* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    
    JNIEnv* env = mSession->getCurrentEnv();
    *result = env->GetStaticFieldID(clazz, name, sig);

    return NS_OK;
}




NS_IMETHODIMP CSecureEnv::NewString(  const jchar* unicode,
                                       jsize len,
                                      jstring* result) 
{
    if (result == NULL)
        return NS_ERROR_NULL_POINTER;
    
	
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
		}
	} msg(unicode, len, result);
    sendMessageToJava(&msg);

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::GetStringLength(  jstring str,
                                            jsize* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->GetStringLength(str);

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::GetStringChars(  jstring str,
                                            jboolean *isCopy,
                                           const jchar** result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->GetStringChars(str, isCopy);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::ReleaseStringChars(  jstring str,
                                                const jchar *chars) 
{
    if (mJavaEnv == NULL)
        return NS_ERROR_NULL_POINTER;
    
    mJavaEnv->ReleaseStringChars(str, chars);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::NewStringUTF(  const char *utf,
                                         jstring* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
	
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
		}
	} msg(utf, result);
    sendMessageToJava(&msg);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetStringUTFLength(  jstring str,
                                               jsize* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->GetStringUTFLength(str);

    return NS_OK;
}

    
NS_IMETHODIMP CSecureEnv::GetStringUTFChars(  jstring str,
                                               jboolean *isCopy,
                                              const char** result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->GetStringUTFChars(str, isCopy);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::ReleaseStringUTFChars(  jstring str,
                                                   const char *chars) 
{
    if (mJavaEnv == NULL)
        return NS_ERROR_NULL_POINTER;
    
    mJavaEnv->ReleaseStringUTFChars(str, chars);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetArrayLength(  jarray array,
                                           jsize* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;
    
    *result = mJavaEnv->GetArrayLength(array);

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::NewObjectArray(  jsize len,
										  jclass clazz,
					                      jobject init,
					                     jobjectArray* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

    *result = mJavaEnv->NewObjectArray(len, clazz, init);

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::GetObjectArrayElement(  jobjectArray array,
                                                   jsize index,
                                                  jobject* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

    *result = mJavaEnv->GetObjectArrayElement(array, index);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::SetObjectArrayElement(  jobjectArray array,
                                                   jsize index,
                                                   jobject val) 
{
    if (mJavaEnv == NULL)
        return NS_ERROR_NULL_POINTER;

    mJavaEnv->SetObjectArrayElement(array, index, val);

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::NewArray( jni_type element_type,
                        			  jsize len,
                        			 jarray* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

    switch (element_type) {
    case jboolean_type:
        result = (jarray*) mJavaEnv->NewBooleanArray(len);
        break;
    case jbyte_type:
        result = (jarray*) mJavaEnv->NewByteArray(len);
        break;
    case jchar_type:
        result = (jarray*) mJavaEnv->NewCharArray(len);
        break;
    case jshort_type:
        result = (jarray*) mJavaEnv->NewShortArray(len);
        break;
    case jint_type:
        result = (jarray*) mJavaEnv->NewIntArray(len);
        break;
    case jlong_type:
        result = (jarray*) mJavaEnv->NewLongArray(len);
        break;
    case jfloat_type:
        result = (jarray*) mJavaEnv->NewFloatArray(len);
        break;
    case jdouble_type:
        result = (jarray*) mJavaEnv->NewDoubleArray(len);
        break;
    default:
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::GetArrayElements(  jni_type type,
                                              jarray array,
                                              jboolean *isCopy,
                                             void* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

    switch (type) {
	case jboolean_type:
	    result = (void*) mJavaEnv->GetBooleanArrayElements((jbooleanArray)array, isCopy);
	    break;
	case jbyte_type:
	    result = (void*) mJavaEnv->GetByteArrayElements((jbyteArray)array, isCopy);
	    break;
	case jchar_type:
	    result = (void*) mJavaEnv->GetCharArrayElements((jcharArray)array, isCopy);
	    break;
	case jshort_type:
	    result = (void*) mJavaEnv->GetShortArrayElements((jshortArray)array, isCopy);
	    break;
	case jint_type:
	    result = (void*) mJavaEnv->GetIntArrayElements((jintArray)array, isCopy);
	    break;
	case jlong_type:
	    result = (void*) mJavaEnv->GetLongArrayElements((jlongArray)array, isCopy);
	    break;
	case jfloat_type:
	    result = (void*) mJavaEnv->GetFloatArrayElements((jfloatArray)array, isCopy);
	    break;
	case jdouble_type:
	    result = (void*) mJavaEnv->GetDoubleArrayElements((jdoubleArray)array, isCopy);
	    break;
	default:
	    return NS_ERROR_FAILURE;
	}

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::ReleaseArrayElements( jni_type type,
                                                 jarray array,
                                                 void *elems,
                                                 jint mode) 
{
	if (mJavaEnv == NULL)
		return NS_ERROR_NULL_POINTER;
	
	switch (type)
	{
	case jboolean_type:
		mJavaEnv->ReleaseBooleanArrayElements((jbooleanArray)array, (jboolean*)elems, mode);
		break;
	case jbyte_type:
		mJavaEnv->ReleaseByteArrayElements((jbyteArray)array, (jbyte*)elems, mode);
		break;
	case jchar_type:
		mJavaEnv->ReleaseCharArrayElements((jcharArray)array, (jchar*)elems, mode);
		break;
	case jshort_type:
		mJavaEnv->ReleaseShortArrayElements((jshortArray)array, (jshort*)elems, mode);
		break;
	case jint_type:
		mJavaEnv->ReleaseIntArrayElements((jintArray)array, (jint*)elems, mode);
		break;
	case jlong_type:
		mJavaEnv->ReleaseLongArrayElements((jlongArray)array, (jlong*)elems, mode);
		break;
	case jfloat_type:
		mJavaEnv->ReleaseFloatArrayElements((jfloatArray)array, (jfloat*)elems, mode);
		break;
	case jdouble_type:
		mJavaEnv->ReleaseDoubleArrayElements((jdoubleArray)array, (jdouble*)elems, mode);
		break;
	default:
		return NS_ERROR_FAILURE;
	}

    return NS_OK;
}

NS_IMETHODIMP CSecureEnv::GetArrayRegion(  jni_type type,
                                            jarray array,
                                            jsize start,
                                            jsize len,
                                           void* buf)
{
    if (mJavaEnv == NULL || buf == NULL)
        return NS_ERROR_NULL_POINTER;

    switch (type) {
    case jboolean_type:
        mJavaEnv->GetBooleanArrayRegion((jbooleanArray)array, start, len, (jboolean*)buf);
        break;
    case jbyte_type:
        mJavaEnv->GetByteArrayRegion((jbyteArray)array, start, len, (jbyte*)buf);
        break;
    case jchar_type:
        mJavaEnv->GetCharArrayRegion((jcharArray)array, start, len, (jchar*)buf);
        break;
    case jshort_type:
        mJavaEnv->GetShortArrayRegion((jshortArray)array, start, len, (jshort*)buf);
        break;
    case jint_type:
        mJavaEnv->GetIntArrayRegion((jintArray)array, start, len, (jint*)buf);
        break;
    case jlong_type:
        mJavaEnv->GetLongArrayRegion((jlongArray)array, start, len, (jlong*)buf);
        break;
    case jfloat_type:
        mJavaEnv->GetFloatArrayRegion((jfloatArray)array, start, len, (jfloat*)buf);
        break;
    case jdouble_type:
        mJavaEnv->GetDoubleArrayRegion((jdoubleArray)array, start, len, (jdouble*)buf);
        break;
    default:
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::SetArrayRegion(  jni_type type,
                                            jarray array,
                                            jsize start,
                                            jsize len,
                                            void* buf) 
{
    if (mJavaEnv == NULL || buf == NULL)
        return NS_ERROR_NULL_POINTER;

    switch (type) {
    case jboolean_type:
        mJavaEnv->SetBooleanArrayRegion((jbooleanArray)array, start, len, (jboolean*)buf);
        break;
    case jbyte_type:
        mJavaEnv->SetByteArrayRegion((jbyteArray)array, start, len, (jbyte*)buf);
        break;
    case jchar_type:
        mJavaEnv->SetCharArrayRegion((jcharArray)array, start, len, (jchar*)buf);
        break;
    case jshort_type:
        mJavaEnv->SetShortArrayRegion((jshortArray)array, start, len, (jshort*)buf);
        break;
    case jint_type:
        mJavaEnv->SetIntArrayRegion((jintArray)array, start, len, (jint*)buf);
        break;
    case jlong_type:
        mJavaEnv->SetLongArrayRegion((jlongArray)array, start, len, (jlong*)buf);
        break;
    case jfloat_type:
        mJavaEnv->SetFloatArrayRegion((jfloatArray)array, start, len, (jfloat*)buf);
        break;
    case jdouble_type:
        mJavaEnv->SetDoubleArrayRegion((jdoubleArray)array, start, len, (jdouble*)buf);
        break;
    default:
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::RegisterNatives(  jclass clazz,
                                             const JNINativeMethod *methods,
                                             jint nMethods,
                                            jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

    *result = mJavaEnv->RegisterNatives(clazz, methods, nMethods);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::UnregisterNatives(  jclass clazz,
                                              jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

    *result = mJavaEnv->UnregisterNatives(clazz);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::MonitorEnter(  jobject obj,
                                         jint* result) 
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

	
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
	} msg(obj, result);
	sendMessageToJava(&msg);

    return NS_OK;
}


NS_IMETHODIMP CSecureEnv::MonitorExit(  jobject obj,
                                        jint* result)
{
    if (mJavaEnv == NULL || result == NULL)
        return NS_ERROR_NULL_POINTER;

	
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
	} msg(obj, result);
	sendMessageToJava(&msg);

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
			receiveQueue->wait(kDefaultJMTime);
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
