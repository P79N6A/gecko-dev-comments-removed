





















































#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"        
#include "jsjava.h"             






JSJHashNumber JS_DLL_CALLBACK
jsj_HashJavaObject(const void *key, void* env)
{
    JSHashNumber hash_code;
    jobject java_obj;
    JNIEnv *jEnv;

    java_obj = (jobject)key;
    jEnv = (JNIEnv*) env;
    JS_ASSERT(!(*jEnv)->ExceptionOccurred(jEnv));
    hash_code = (*jEnv)->CallStaticIntMethod(jEnv, jlSystem,
                                             jlSystem_identityHashCode, java_obj);

#ifdef DEBUG
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        (*jEnv)->ExceptionDescribe(jEnv);
        JS_ASSERT(0);
    }
#endif

    return hash_code;
}








intN JS_DLL_CALLBACK
jsj_JavaObjectComparator(const void *v1, const void *v2, void *arg)
{
    jobject java_obj1, java_obj2;
    JNIEnv *jEnv;

    jEnv = (JNIEnv*)arg;
    java_obj1 = (jobject)v1;
    java_obj2 = (jobject)v2;

    if (java_obj1 == java_obj2)
        return 1;
    if (jEnv)
        return (*jEnv)->IsSameObject(jEnv, java_obj1, java_obj2);
    return 0;
}







const char *
jsj_DupJavaStringUTF(JSContext *cx, JNIEnv *jEnv, jstring jstr)
{
    const char *str, *retval;

    str = (*jEnv)->GetStringUTFChars(jEnv, jstr, 0);
    if (!str) {
        jsj_UnexpectedJavaError(cx, jEnv, "Can't get UTF8 characters from "
                                          "Java string");
        return NULL;
    }
    retval = JS_strdup(cx, str);
    (*jEnv)->ReleaseStringUTFChars(jEnv, jstr, str);
    return retval;
}

JSBool
JavaStringToId(JSContext *cx, JNIEnv *jEnv, jstring jstr, jsid *idp)
{
    const jschar *ucs2;
    JSString *jsstr;
    jsize ucs2_len;
    jsval val;
    
    ucs2 = (*jEnv)->GetStringChars(jEnv, jstr, 0);
    if (!ucs2) {
        jsj_UnexpectedJavaError(cx, jEnv, "Couldn't obtain Unicode characters"
                                "from Java string");
        return JS_FALSE;
    }
    
    ucs2_len = (*jEnv)->GetStringLength(jEnv, jstr);
    jsstr = JS_InternUCStringN(cx, ucs2, ucs2_len);
    (*jEnv)->ReleaseStringChars(jEnv, jstr, ucs2);
    if (!jsstr)
        return JS_FALSE;

    val = STRING_TO_JSVAL(jsstr);
    JS_ValueToId(cx, STRING_TO_JSVAL(jsstr), idp);
    return JS_TRUE;
}











const char *
jsj_GetJavaErrorMessage(JNIEnv *jEnv)
{
    const char *java_error_msg;
    char *error_msg = NULL;
    jthrowable exception;
    jstring java_exception_jstring;

    exception = (*jEnv)->ExceptionOccurred(jEnv);
    if (exception && jlThrowable_toString) {
        java_exception_jstring =
            (*jEnv)->CallObjectMethod(jEnv, exception, jlThrowable_toString);
        if (!java_exception_jstring)
            goto done;
        java_error_msg = (*jEnv)->GetStringUTFChars(jEnv, java_exception_jstring, NULL);
        if (java_error_msg) {
            error_msg = strdup((char*)java_error_msg);
            (*jEnv)->ReleaseStringUTFChars(jEnv, java_exception_jstring, java_error_msg);
        }
        (*jEnv)->DeleteLocalRef(jEnv, java_exception_jstring);

#ifdef DEBUG
        
#endif
    }
done:
    if (exception)
        (*jEnv)->DeleteLocalRef(jEnv, exception);
    return error_msg;
}    








static const char *
get_java_stack_trace(JSContext *cx, JNIEnv *jEnv, jthrowable java_exception)
{
    const char *backtrace;
    jstring backtrace_jstr;

    backtrace = NULL;
    if (java_exception && njJSUtil_getStackTrace) {
        backtrace_jstr = (*jEnv)->CallStaticObjectMethod(jEnv, njJSUtil,
                                                         njJSUtil_getStackTrace,
                                                         java_exception);
        if (!backtrace_jstr) {
            jsj_UnexpectedJavaError(cx, jEnv, "Unable to get exception stack trace");
            return NULL;
        }
        backtrace = jsj_DupJavaStringUTF(cx, jEnv, backtrace_jstr);
        (*jEnv)->DeleteLocalRef(jEnv, backtrace_jstr);
    }
    return backtrace;
} 


#define REPORT_JAVA_EXCEPTION_STACK_TRACE







static void
vreport_java_error(JSContext *cx, JNIEnv *jEnv, const char *format, va_list ap)
{
    jobject java_obj;
    jclass java_class;
    JavaClassDescriptor *class_descriptor;
    jthrowable java_exception;
    JSType wrapped_exception_type;
    jsval js_exception;
       
    java_obj = NULL;
    class_descriptor = NULL;

    
    java_exception = (*jEnv)->ExceptionOccurred(jEnv);
    if (!java_exception) {
        JSString *err_jsstr;
        char *err = JS_vsmprintf(format, ap);
        if (!err)
            return;
        err_jsstr = JS_NewString(cx, err, strlen(err));
        if (!err_jsstr)
            return;
        JS_SetPendingException(cx, STRING_TO_JSVAL(err_jsstr));
        return;
    }

    
    (*jEnv)->ExceptionClear(jEnv);
    
    
    if (njJSException && 
        (*jEnv)->IsInstanceOf(jEnv, java_exception, njJSException)) {
        
        wrapped_exception_type = 
            (*jEnv)->GetIntField(jEnv, java_exception,
            njJSException_wrappedExceptionType);
        
        
        if ((int)wrapped_exception_type != JSTYPE_EMPTY) {
            java_obj = 
                (*jEnv)->GetObjectField(jEnv, java_exception, 
                njJSException_wrappedException);
            
            if ((java_obj == NULL) && 
                (wrapped_exception_type == JSTYPE_OBJECT)) {
                js_exception = JSVAL_NULL;
            } else { 
                java_class = (*jEnv)->GetObjectClass(jEnv, java_obj); 
                class_descriptor = jsj_GetJavaClassDescriptor(cx, jEnv, java_class);
                
                (*jEnv)->DeleteLocalRef(jEnv, java_class);  
                
                
                switch(wrapped_exception_type) {
                case JSTYPE_NUMBER:
                    if (!jsj_ConvertJavaObjectToJSNumber(cx, jEnv,
                        class_descriptor,
                        java_obj, 
                        &js_exception))
                        goto error;
                    break;
                case JSTYPE_BOOLEAN:
                    if (!jsj_ConvertJavaObjectToJSBoolean(cx, jEnv,
                        class_descriptor,
                        java_obj, 
                        &js_exception))
                        goto error;
                    break;
                case JSTYPE_STRING:
                    if (!jsj_ConvertJavaObjectToJSString(cx, jEnv,
                        class_descriptor,
                        java_obj, 
                        &js_exception))
                        goto error;
                    break;
                case JSTYPE_VOID:
                    js_exception = JSVAL_VOID;
                    break;
                case JSTYPE_OBJECT:
                case JSTYPE_FUNCTION:
                default:
                    if ((*jEnv)->IsInstanceOf(jEnv, java_obj, njJSObject)) {
                        js_exception = OBJECT_TO_JSVAL(jsj_UnwrapJSObjectWrapper(jEnv, java_obj));
                        if (!js_exception)
                            goto error;                        
                    } else {
                        if (!jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_obj, 
                            &js_exception)) 
                            goto error;
                    }
                }
            }
        }
        
    } else {
        if (!JSJ_ConvertJavaObjectToJSValue(cx, java_exception,
            &js_exception)) {
            goto error;
        }
    }
    
    
    JS_SetPendingException(cx, js_exception);                        
    goto done;

error:
    
    JS_ASSERT(0);
    jsj_LogError("Out of memory while attempting to throw JSException\n");
    
done:
    if (class_descriptor)
        jsj_ReleaseJavaClassDescriptor(cx, jEnv, class_descriptor);
    if (java_obj)
        (*jEnv)->DeleteLocalRef(jEnv, java_obj);
    if (java_exception)
        (*jEnv)->DeleteLocalRef(jEnv, java_exception);
}

void
jsj_ReportJavaError(JSContext *cx, JNIEnv *env, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vreport_java_error(cx, env, format, ap);
    va_end(ap);
}





void
jsj_UnexpectedJavaError(JSContext *cx, JNIEnv *env, const char *format, ...)
{
    va_list ap;
    const char *format2;

    va_start(ap, format);
    format2 = JS_smprintf("internal error: %s", format);
    if (format2) {
        vreport_java_error(cx, env, format2, ap);
        JS_smprintf_free((void*)format2);
    }
    va_end(ap);
}







void
jsj_LogError(const char *error_msg)
{
    if (JSJ_callbacks && JSJ_callbacks->error_print)
        JSJ_callbacks->error_print(error_msg);
    else
        fputs(error_msg, stderr);
}













#define JSJ_HAS_DFLT_MSG_STRINGS 1

JSErrorFormatString jsj_ErrorFormatString[JSJ_Err_Limit] = {
#if JSJ_HAS_DFLT_MSG_STRINGS
#define MSG_DEF(name, number, count, format) \
    { format, count } ,
#else
#define MSG_DEF(name, number, count, format) \
    { NULL, count } ,
#endif
#include "jsj.msg"
#undef MSG_DEF
};

const JSErrorFormatString *
jsj_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSJ_Err_Limit))
            return &jsj_ErrorFormatString[errorNumber];
        else
            return NULL;
}

jsize
jsj_GetJavaArrayLength(JSContext *cx, JNIEnv *jEnv, jarray java_array)
{
    jsize array_length = (*jEnv)->GetArrayLength(jEnv, java_array);
    jthrowable java_exception = (*jEnv)->ExceptionOccurred(jEnv);
    if (java_exception) {
        jsj_UnexpectedJavaError(cx, jEnv, "Couldn't obtain array length");
        (*jEnv)->DeleteLocalRef(jEnv, java_exception);
        return -1;
    }
    return array_length;
}

static JSJavaThreadState *the_java_jsj_env = NULL;

JSJavaThreadState *
jsj_EnterJava(JSContext *cx, JNIEnv **envp)
{
    JSJavaThreadState *jsj_env;
    char *err_msg;

    JS_ASSERT(envp);
    *envp = NULL;
    err_msg = NULL;

    jsj_env = the_java_jsj_env;
    if (jsj_env == NULL && JSJ_callbacks && JSJ_callbacks->map_js_context_to_jsj_thread)
        jsj_env = JSJ_callbacks->map_js_context_to_jsj_thread(cx, &err_msg);
    if (!jsj_env) {
        if (err_msg) {
            JS_ReportError(cx, err_msg);
            free(err_msg);
        }
        return NULL;
    }

    
    if ((jsj_env->recursion_depth > 0) && (jsj_env->cx != cx))
        return NULL;

    jsj_env->recursion_depth++;

    
    if (!jsj_env->cx)
        jsj_env->cx = cx;

    *envp = jsj_env->jEnv;
    return jsj_env;
}

extern void
jsj_ExitJava(JSJavaThreadState *jsj_env)
{
    jsj_JSIsCallingApplet = JS_FALSE;
    if (jsj_env) {
        JS_ASSERT(jsj_env->recursion_depth > 0);
        if (--jsj_env->recursion_depth == 0)
            jsj_env->cx = NULL;
    }
}









JSJavaThreadState *
jsj_SetJavaJSJEnv(JSJavaThreadState* java_jsj_env)
{
    JSJavaThreadState *old_jsj_env = the_java_jsj_env;
    the_java_jsj_env = java_jsj_env;
    return old_jsj_env;
}
