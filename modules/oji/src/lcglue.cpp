





































#include "prthread.h"
#include "nsJVMManager.h"
#include "nsIPluginInstancePeer2.h"
#include "ProxyJNI.h"
#include "lcglue.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsISecurityContext.h"
#include "nsCSecurityContext.h"
#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"
#include "nsDOMJSUtils.h"

static NS_DEFINE_CID(kJVMManagerCID, NS_JVMMANAGER_CID);

extern "C" int XP_PROGRESS_STARTING_JAVA;
extern "C" int XP_PROGRESS_STARTING_JAVA_DONE;
extern "C" int XP_JAVA_NO_CLASSES;
extern "C" int XP_JAVA_GENERAL_FAILURE;
extern "C" int XP_JAVA_STARTUP_FAILED;
extern "C" int XP_JAVA_DEBUGGER_FAILED;




template <class T>
class ThreadLocalStorage {
public:
	ThreadLocalStorage(PRThreadPrivateDTOR dtor) : mIndex(0), mValid(PR_FALSE)
	{
		mValid = (PR_NewThreadPrivateIndex(&mIndex, dtor) == PR_SUCCESS);
	}
	
	void set(T value)
	{
		if (mValid) PR_SetThreadPrivate(mIndex, value);
	}
	
	T get()
	{
		return (T) (mValid ? PR_GetThreadPrivate(mIndex) : 0);
	}

private:
	PRUintn mIndex;
	PRBool mValid;
};


static void PR_CALLBACK detach_JVMContext(void* storage)
{
	JVMContext* context = reinterpret_cast<JVMContext*>(storage);
	
	JNIEnv* proxyEnv = context->proxyEnv;
	if (proxyEnv != NULL) {
		DeleteProxyJNI(proxyEnv);
		context->proxyEnv = NULL;
	}
	
	delete context;
}

JVMContext* GetJVMContext()
{
	
	static ThreadLocalStorage<JVMContext*> localContext((PRThreadPrivateDTOR)&detach_JVMContext);
	JVMContext* context = localContext.get();
	if (context == NULL) {
		context = new JVMContext;
		context->proxyEnv = NULL;
		context->jsj_env = NULL;
		localContext.set(context);
	}
	return context;
}





JS_BEGIN_EXTERN_C

#include "jscntxt.h"

static JSContext*
map_jsj_thread_to_js_context_impl(JSJavaThreadState *jsj_env, void* java_applet_obj, JNIEnv *env, char **errp)
{
	
	
	
	JSContext* context = NULL;
	if (java_applet_obj != NULL) {
		nsIPluginInstance* pluginInstance = reinterpret_cast<nsIPluginInstance*>(java_applet_obj);
	        nsIPluginInstancePeer* pluginPeer = NULL;
		if (pluginInstance->GetPeer(&pluginPeer) == NS_OK) {
			nsIPluginInstancePeer2* pluginPeer2 = NULL;
			if (pluginPeer->QueryInterface(NS_GET_IID(nsIPluginInstancePeer2), (void**) &pluginPeer2) == NS_OK) {
				pluginPeer2->GetJSContext(&context);
				NS_RELEASE(pluginPeer2);
			}
			NS_RELEASE(pluginPeer);
		}
	}
	return context;
}







static JSJavaThreadState*
map_js_context_to_jsj_thread_impl(JSContext *cx, char **errp)
{
	*errp = NULL;

    
    
    

	JVMContext* context = GetJVMContext();
	JSJavaThreadState* jsj_env = context->jsj_env;
	if (jsj_env != NULL)
		return jsj_env;

	JSJavaVM* js_jvm = NULL;
	nsresult rv;
	nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
	if (NS_FAILED(rv)) return NULL;
	nsJVMManager* pJVMMgr = (nsJVMManager*) managerService.get();  
	if (pJVMMgr != NULL) {
		js_jvm = pJVMMgr->GetJSJavaVM();
		if (js_jvm == NULL) {
			*errp = strdup("Failed to attach to a Java VM.");
			return NULL;
		}
	}

	jsj_env = JSJ_AttachCurrentThreadToJava(js_jvm, NULL, NULL);
	context->jsj_env = jsj_env;

	return jsj_env;
}










static JSObject*
map_java_object_to_js_object_impl(JNIEnv *env, void *pluginInstancePtr, char* *errp)
{
	JSObject        *window = NULL;
	PRBool           mayscript = PR_FALSE;
	PRBool           jvmMochaPrefsEnabled = PR_TRUE;
	nsresult         err = NS_OK;

	*errp = NULL;

	if (pluginInstancePtr == NULL) {
		env->ThrowNew(env->FindClass("java/lang/NullPointerException"), "plugin instance is NULL");
		return NULL;
	}

	
	
	
	if (!jvmMochaPrefsEnabled) {
		*errp = strdup("JSObject.getWindow() failed: java preference is disabled");
		return NULL;
	}

	


	nsIPluginInstance* pluginInstance = reinterpret_cast<nsIPluginInstance*>(pluginInstancePtr);
	nsIPluginInstancePeer* pluginPeer;
	if (pluginInstance->GetPeer(&pluginPeer) == NS_OK) {
		nsIJVMPluginTagInfo* tagInfo;
		if (pluginPeer->QueryInterface(NS_GET_IID(nsIJVMPluginTagInfo), (void**) &tagInfo) == NS_OK) {
			err = tagInfo->GetMayScript(&mayscript);
			
			NS_RELEASE(tagInfo);
		}
		if ( !mayscript ) {
			*errp = strdup("JSObject.getWindow() requires mayscript attribute on this Applet");
		} else {
			nsIPluginInstancePeer2* pluginPeer2 = nsnull;
			if (pluginPeer->QueryInterface(NS_GET_IID(nsIPluginInstancePeer2),
			                              (void**) &pluginPeer2) == NS_OK) {
				err = pluginPeer2->GetJSWindow(&window);
				NS_RELEASE(pluginPeer2);
			}
		}
		NS_RELEASE(pluginPeer);
	}

	
	
	return window;
}

static JSPrincipals*
get_JSPrincipals_from_java_caller_impl(JNIEnv *pJNIEnv, JSContext *pJSContext, void  **ppNSIPrincipalArrayIN, int numPrincipals, void *pNSISecurityContext)
{
    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> secMan = 
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return NULL;

    nsCOMPtr<nsIPrincipal> principal;
    rv = secMan->GetPrincipalFromContext(pJSContext,
                                         getter_AddRefs(principal));
    if (NS_FAILED(rv))
        return NULL;

    JSPrincipals* jsprincipals = NULL;
    principal->GetJSPrincipals(pJSContext, &jsprincipals);
    return jsprincipals;
}

static jobject
get_java_wrapper_impl(JNIEnv *pJNIEnv, lcjsobject a_jsobject)
{
    nsresult       err    = NS_OK;
    jobject  pJSObjectWrapper = NULL;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &err);
    if (NS_FAILED(err)) return NULL;
    nsJVMManager* pJVMMgr = (nsJVMManager *)managerService.get();  
    if (pJVMMgr != NULL) {
      nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
      if (pJVMPI != NULL) {
         err = pJVMPI->GetJavaWrapper(pJNIEnv, a_jsobject, &pJSObjectWrapper);
      }
    }
    if ( err != NS_OK )
    {
       return NULL;
    }
    return pJSObjectWrapper;
}

static lcjsobject
unwrap_java_wrapper_impl(JNIEnv *pJNIEnv, jobject java_wrapper)
{
    lcjsobject obj = 0;
    nsresult       err    = NS_OK;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &err);
    if (NS_FAILED(err)) return 0;
    nsJVMManager* pJVMMgr = (nsJVMManager *)managerService.get();  
    if (pJVMMgr != NULL) {
      nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
      if (pJVMPI != NULL) {
         err = pJVMPI->UnwrapJavaWrapper(pJNIEnv, java_wrapper, &obj);
      }
    }
    if ( err != NS_OK )
    {
       return 0;
    }
    return obj;
}

static JSBool
enter_js_from_java_impl(JNIEnv *jEnv, char **errp,
                        void **pNSIPrincipaArray, int numPrincipals, 
                        void *pNSISecurityContext,
                        void *java_applet_obj)
{
	return PR_TRUE;
}

static void
exit_js_impl(JNIEnv *jEnv, JSContext *cx)
{
    
    if (cx)
    {
        nsIScriptContext *scriptContext = GetScriptContextFromJSContext(cx);

        if (scriptContext)
        {
            scriptContext->ScriptEvaluated(PR_TRUE);
        }
    }
    return;
}

static PRBool
create_java_vm_impl(SystemJavaVM* *jvm, JNIEnv* *initialEnv, void* initargs)
{
    
    nsCOMPtr<nsIJVMManager> serv = do_GetService(kJVMManagerCID);
    if (!serv)
        return PR_FALSE;
    *initialEnv = JVM_GetJNIEnv();
    if (!*initialEnv)
        return PR_FALSE;
    
    
    *jvm = reinterpret_cast<SystemJavaVM*>(serv.get());
    return PR_TRUE;
}

static PRBool
destroy_java_vm_impl(SystemJavaVM* jvm, JNIEnv* initialEnv)
{
    JVM_ReleaseJNIEnv(initialEnv);
    
    return PR_TRUE;
}

static JNIEnv*
attach_current_thread_impl(SystemJavaVM* jvm)
{
    return JVM_GetJNIEnv();
}

static PRBool
detach_current_thread_impl(SystemJavaVM* jvm, JNIEnv* env)
{
    JVM_ReleaseJNIEnv(env);
    return PR_TRUE;
}

static SystemJavaVM*
get_java_vm_impl(JNIEnv* env)
{
    
    nsresult rv;
    nsCOMPtr<nsIJVMManager> managerService = do_GetService(kJVMManagerCID, &rv);
    if (NS_FAILED(rv)) return NULL;
    SystemJavaVM* jvm = reinterpret_cast<SystemJavaVM*>(managerService.get());  
    return jvm;
}

JS_END_EXTERN_C

static JSJCallbacks jsj_callbacks = {
    map_jsj_thread_to_js_context_impl,
    map_js_context_to_jsj_thread_impl,
    map_java_object_to_js_object_impl,
    get_JSPrincipals_from_java_caller_impl,
    enter_js_from_java_impl,
    exit_js_impl,
    NULL,       
    get_java_wrapper_impl,
    unwrap_java_wrapper_impl,
    create_java_vm_impl,
    destroy_java_vm_impl,
    attach_current_thread_impl,
    detach_current_thread_impl,
    get_java_vm_impl
};

void
JVM_InitLCGlue(void)
{
    JSJ_Init(&jsj_callbacks);
}





















