






































#include "ProxyClassLoader.h"

#include "jsapi.h"
#include "jsjava.h"
#include "prprf.h"

#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSContextStack.h"
#include "nsIPrincipal.h"
#include "nsNetUtil.h"
#include "ProxyJNI.h"
#include "nsCNullSecurityContext.h"









static nsresult getScriptClassLoader(JNIEnv* env, jobject* classloader)
{
    
    nsresult rv;
    nsCOMPtr<nsIJSContextStack> contexts =
        do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
    if (NS_FAILED(rv)) return rv;
    JSContext* cx;
    rv = contexts->Peek(&cx);
    if (NS_FAILED(rv)) return rv;
    
    
    
    JSObject* window = JS_GetGlobalObject(cx);
    if (!window) return NS_ERROR_FAILURE;

    jsval navigator = JSVAL_NULL;
    if (!JS_LookupProperty(cx, window, "navigator", &navigator))
        return NS_ERROR_FAILURE;
    
    jsval javaclasses = JSVAL_NULL;
    if (!JSVAL_IS_PRIMITIVE(navigator)) {
        uintN attrs;
        JSBool found;

        
        
        
        
        JSObject *obj = JSVAL_TO_OBJECT(navigator);
        if (!JS_GetPropertyAttributes(cx, obj, "javaclasses", &attrs, &found))
            return NS_ERROR_FAILURE;
        if ((~attrs & (JSPROP_READONLY | JSPROP_PERMANENT)) == 0 &&
            !JS_GetProperty(cx, obj, "javaclasses", &javaclasses)) {
            return NS_ERROR_FAILURE;
        }

        
        
        if (JSJ_ConvertJSValueToJavaObject(cx, javaclasses, classloader))
            return NS_OK;
    }

    
    
    jclass netscape_oji_ProxyClassLoaderFac =
        env->FindClass("netscape/oji/ProxyClassLoaderFactory");
    if (!netscape_oji_ProxyClassLoaderFac) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }
    jmethodID staticMethodID =
        env->GetStaticMethodID(netscape_oji_ProxyClassLoaderFac,
                               "createClassLoader",
                               "(Ljava/lang/String;)Ljava/lang/ClassLoader;");
    if (!staticMethodID) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }

    
    
    nsCOMPtr<nsIScriptSecurityManager> secMan =
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIPrincipal> principal, sysprincipal;
    rv = secMan->GetPrincipalFromContext(cx, getter_AddRefs(principal));
    if (NS_FAILED(rv)) return rv;

    rv = secMan->GetSystemPrincipal(getter_AddRefs(sysprincipal));
    if (NS_FAILED(rv)) return rv;

    PRBool equals;
    rv = principal->Equals(sysprincipal, &equals);
    
    if (NS_FAILED(rv)) return rv;
    if (equals) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIURI> codebase;
    rv = principal->GetURI(getter_AddRefs(codebase));
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString spec;
    rv = codebase->GetSpec(spec);
    if (NS_FAILED(rv)) return rv;
    
    jstring jspec = env->NewStringUTF(spec.get());
    if (!jspec) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }

    
    
    nsISecurityContext* origContext = nsnull;
    if (NS_FAILED(GetSecurityContext(env, &origContext))) {
        return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsISecurityContext> nullContext(new nsCNullSecurityContext());
    if (!nullContext) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    SetSecurityContext(env, nullContext);
    *classloader = env->CallStaticObjectMethod(netscape_oji_ProxyClassLoaderFac,
                                               staticMethodID, jspec);
    SetSecurityContext(env, origContext);
    if (!*classloader) {
        env->ExceptionClear();
        return NS_ERROR_FAILURE;
    }

    env->DeleteLocalRef(jspec);
    env->DeleteLocalRef(netscape_oji_ProxyClassLoaderFac);
    
    
    if (!JSVAL_IS_PRIMITIVE(navigator) &&
        JSJ_ConvertJavaObjectToJSValue(cx, *classloader, &javaclasses) &&
        !JS_DefineProperty(cx, JSVAL_TO_OBJECT(navigator), "javaclasses",
                           javaclasses, NULL, NULL, JSPROP_ENUMERATE |
                           JSPROP_READONLY | JSPROP_PERMANENT)) {
        return NS_ERROR_FAILURE;
    }
    
    return NS_OK;
}

jclass ProxyFindClass(JNIEnv* env, const char* name)
{
    do {
        
        
        jobject classloader;
        jthrowable jException = env->ExceptionOccurred();
        if (jException != NULL) {
            
            env->ExceptionClear();
            
            env->DeleteLocalRef(jException);
        }
        nsresult rv = getScriptClassLoader(env, &classloader);
        if (NS_FAILED(rv)) break;

        jclass netscape_oji_ProxyClassLoader = env->GetObjectClass(classloader);
        jmethodID loadClassID = env->GetMethodID(netscape_oji_ProxyClassLoader, "loadClass",
                                                 "(Ljava/lang/String;)Ljava/lang/Class;");
        env->DeleteLocalRef(netscape_oji_ProxyClassLoader);
        if (!loadClassID) {
            env->ExceptionClear();
            break;
        }
        jstring jname = env->NewStringUTF(name);
        jvalue jargs[1]; jargs[0].l = jname;
        jclass c = (jclass) env->CallObjectMethodA(classloader, loadClassID, jargs);
        env->DeleteLocalRef(jname);
        return c;
    } while (0);
    return 0;
}
