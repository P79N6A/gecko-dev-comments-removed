





































 









#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"
#include "jsjava.h"

#include "jscntxt.h"        


#include "netscape_javascript_JSObject.h"   




struct CapturedJSError {
    char *              message;
    JSErrorReport       report;         
    jthrowable          java_exception; 
    CapturedJSError *   next;           
};




#ifdef PRESERVE_JSOBJECT_IDENTITY















static JSHashTable *js_obj_reflections = NULL;

#ifdef JSJ_THREADSAFE




static PRMonitor *js_obj_reflections_monitor = NULL;
#endif  
#endif  

JSBool
jsj_init_js_obj_reflections_table()
{
#ifdef PRESERVE_JSOBJECT_IDENTITY
    if(js_obj_reflections != NULL)
    {
      return JS_TRUE;
    }
    js_obj_reflections = JS_NewHashTable(128, NULL, JS_CompareValues,
                                         JS_CompareValues, NULL, NULL);
    if (!js_obj_reflections)
        return JS_FALSE;

#ifdef JSJ_THREADSAFE
    js_obj_reflections_monitor = PR_NewMonitor();
    if (!js_obj_reflections_monitor) {
        JS_HashTableDestroy(js_obj_reflections);
        return JS_FALSE;
    }
#endif  
#endif  

    return JS_TRUE;
}










#ifdef PRESERVE_JSOBJECT_IDENTITY

jobject
jsj_WrapJSObject(JSContext *cx, JNIEnv *jEnv, JSObject *js_obj)
{
    jobject java_wrapper_obj;
    JSHashEntry *he, **hep;

    java_wrapper_obj = NULL;

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(js_obj_reflections_monitor);
#endif

    

    hep = JS_HashTableRawLookup(js_obj_reflections, (JSHashNumber)js_obj, js_obj);

    






    he = *hep;
    if (he) {
        java_wrapper_obj = (jobject)he->value;
        JS_ASSERT(java_wrapper_obj);
        if (java_wrapper_obj)
            goto done;
    }

    

#ifndef OJI
    java_wrapper_obj =
        (*jEnv)->NewObject(jEnv, njJSObject, njJSObject_JSObject, (jint)js_obj);
#else
    if (JSJ_callbacks && JSJ_callbacks->get_java_wrapper != NULL) {
        java_wrapper_obj = JSJ_callbacks->get_java_wrapper(jEnv, (jint)handle);
    }
#endif 

    if (!java_wrapper_obj) {
        jsj_UnexpectedJavaError(cx, jEnv, "Couldn't create new instance of "
                                          "netscape.javascript.JSObject");
        goto done;
    }

    
    he = JS_HashTableRawAdd(js_obj_reflections, hep, (JSHashNumber)js_obj,
                            js_obj, java_wrapper_obj);
    if (he) {
        

        JS_AddNamedRoot(cx, (void*)&he->key, "&he->key");
    } else {
        JS_ReportOutOfMemory(cx);
        
        java_wrapper_obj = NULL;
    }
    
    



    
    
    

done:
#ifdef JSJ_THREADSAFE
        PR_ExitMonitor(js_obj_reflections_monitor);
#endif
        
    return java_wrapper_obj;
}






JSBool
jsj_remove_js_obj_reflection_from_hashtable(JSContext *cx, JSObject *js_obj)
{
    JSHashEntry *he, **hep;
    JSBool success = JS_FALSE;

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(js_obj_reflections_monitor);
#endif

    
    hep = JS_HashTableRawLookup(js_obj_reflections, (JSHashNumber)js_obj, js_obj);
    he = *hep;

    JS_ASSERT(he);
    if (he) {
        

        success = JS_RemoveRoot(cx, (void *)&he->key);

        JS_HashTableRawRemove(js_obj_reflections, hep, he);
    }

#ifdef JSJ_THREADSAFE
    PR_ExitMonitor(js_obj_reflections_monitor);
#endif

    return success;
}

#else 






jobject
jsj_WrapJSObject(JSContext *cx, JNIEnv *jEnv, JSObject *js_obj)
{
    jobject java_wrapper_obj;
    JSObjectHandle *handle;

    

    handle = (JSObjectHandle*)JS_malloc(cx, sizeof(JSObjectHandle));
    if (!handle)
        return NULL;
    handle->js_obj = js_obj;
    handle->rt = JS_GetRuntime(cx);

    

#ifndef OJI
    java_wrapper_obj =
        (*jEnv)->NewObject(jEnv, njJSObject, njJSObject_JSObject, (lcjsobject)handle);
#else
    if (JSJ_callbacks && JSJ_callbacks->get_java_wrapper != NULL) {
        java_wrapper_obj = JSJ_callbacks->get_java_wrapper(jEnv, (lcjsobject)handle);
    } else  {
        java_wrapper_obj = NULL;
    }
#endif 
    if (!java_wrapper_obj) {
        jsj_UnexpectedJavaError(cx, jEnv, "Couldn't create new instance of "
                                          "netscape.javascript.JSObject");
        JS_free(cx, handle);
        goto done;
    }
 
    JS_AddNamedRoot(cx, &handle->js_obj, "&handle->js_obj");

done:
        
    return java_wrapper_obj;
}

JSObject *
jsj_UnwrapJSObjectWrapper(JNIEnv *jEnv, jobject java_wrapper_obj)
{
    JSObjectHandle *handle;

#ifndef OJI
#if JS_BYTES_PER_LONG == 8 || JS_BYTES_PER_WORD == 8
    handle = (JSObjectHandle*)((*jEnv)->GetLongField(jEnv, java_wrapper_obj, njJSObject_long_internal));
#else
    handle = (JSObjectHandle*)((*jEnv)->GetIntField(jEnv, java_wrapper_obj, njJSObject_internal));
#endif
#else
    







    if (JSJ_callbacks && JSJ_callbacks->unwrap_java_wrapper != NULL) {
        handle = (JSObjectHandle*)JSJ_callbacks->unwrap_java_wrapper(jEnv, java_wrapper_obj);
    }
    else {
        jclass   cid = (*jEnv)->GetObjectClass(jEnv, java_wrapper_obj);
#if JS_BYTES_PER_LONG == 8 || JS_BYTES_PER_WORD == 8
        jfieldID fid = (*jEnv)->GetFieldID(jEnv, cid, "nativeJSObject", "J");
        handle = (JSObjectHandle*)((*jEnv)->GetLongField(jEnv, java_wrapper_obj, fid));
#else
        jfieldID fid = (*jEnv)->GetFieldID(jEnv, cid, "nativeJSObject", "I");
        handle = (JSObjectHandle*)((*jEnv)->GetIntField(jEnv, java_wrapper_obj, fid));
#endif
    }
#endif
    
	
    if (!handle)
        return NULL;

    return handle->js_obj;
	
}

#endif  







static CapturedJSError *
destroy_saved_js_error(JNIEnv *jEnv, CapturedJSError *error)
{
    CapturedJSError *next_error;
    if (!error)
        return NULL;
    next_error = error->next;

    if (error->java_exception)
        (*jEnv)->DeleteGlobalRef(jEnv, error->java_exception);
    if (error->message)
        free((char*)error->message);
    if (error->report.filename)
        free((char*)error->report.filename);
    if (error->report.linebuf)
        free((char*)error->report.linebuf);
    free(error);

    return next_error;
}











JS_STATIC_DLL_CALLBACK(void)
capture_js_error_reports_for_java(JSContext *cx, const char *message,
                                  JSErrorReport *report)
{
    CapturedJSError *new_error;
    JSJavaThreadState *jsj_env;
    jthrowable java_exception, tmp_exception;
    JNIEnv *jEnv;

    


    if (report && (report->flags & (JSREPORT_WARNING | JSREPORT_EXCEPTION)))
        return;

    
    new_error = malloc(sizeof(CapturedJSError));
    if (!new_error)
        goto out_of_memory;
    memset(new_error, 0, sizeof(CapturedJSError));

    
    if (message) {
        new_error->message = strdup(message);
        if (!new_error->message) {
            free(new_error);
            return;
        }
    }
    if (report) {
        new_error->report.lineno = report->lineno;

        if (report->filename) {
            new_error->report.filename = strdup(report->filename);
            if (!new_error->report.filename)
                goto out_of_memory;
        }

        if (report->linebuf) {
            new_error->report.linebuf = strdup(report->linebuf);
            if (!new_error->report.linebuf)
                goto out_of_memory;
            new_error->report.tokenptr =
                new_error->report.linebuf + (report->tokenptr - report->linebuf);
        }
    }

    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jsj_env)
        goto abort;

    
    java_exception = (*jEnv)->ExceptionOccurred(jEnv);
    if (java_exception) {
        (*jEnv)->ExceptionClear(jEnv);
        tmp_exception = java_exception;     
        java_exception = (*jEnv)->NewGlobalRef(jEnv, java_exception);
        new_error->java_exception = java_exception;
        (*jEnv)->DeleteLocalRef(jEnv, tmp_exception);
    }

    
    new_error->next = jsj_env->pending_js_errors;
    jsj_env->pending_js_errors = new_error;
    jsj_ExitJava(jsj_env);
    return;

abort:
out_of_memory:
    
    JS_ASSERT(0);
    destroy_saved_js_error(jEnv, new_error);
    return;
}

void
jsj_ClearPendingJSErrors(JSJavaThreadState *jsj_env)
{
    while (jsj_env->pending_js_errors)
        jsj_env->pending_js_errors = destroy_saved_js_error(jsj_env->jEnv, jsj_env->pending_js_errors);
}







static void
throw_any_pending_js_error_as_a_java_exception(JSJavaThreadState *jsj_env)
{
    CapturedJSError *error;
    JNIEnv *jEnv;
    jstring message_jstr, linebuf_jstr, filename_jstr;
    jint index, lineno;
    JSErrorReport *report;
    JSContext *cx;
    jsval pending_exception; 
    jobject java_obj;
    int dummy_cost;
    JSBool is_local_refp;
    JSType primitive_type;
    jthrowable java_exception;

    message_jstr = linebuf_jstr = filename_jstr = java_exception = NULL;

    
    jEnv = jsj_env->jEnv;

    cx = jsj_env->cx;
    
    if (cx&&JS_IsExceptionPending(cx)) {
        if (!JS_GetPendingException(cx, &pending_exception))
            goto out_of_memory;

        
        primitive_type = JS_TypeOfValue(cx, pending_exception);
        
        
 
        if (!jsj_ConvertJSValueToJavaObject(cx, jEnv, 
                                            pending_exception, 
                                            jsj_get_jlObject_descriptor(cx, jEnv),
                                            &dummy_cost, &java_obj, 
                                            &is_local_refp))
            goto done;
        
        java_exception = (*jEnv)->NewObject(jEnv, njJSException, 
                                            njJSException_JSException_wrap,
                                            primitive_type, java_obj);
        if (is_local_refp)
            (*jEnv)->DeleteLocalRef(jEnv, java_obj);
        if (!java_exception) 
            goto out_of_memory;
        
        
        if ((*jEnv)->Throw(jEnv, java_exception) < 0) {
            JS_ASSERT(0);
            jsj_LogError("Couldn't throw JSException\n");
            goto done;
        }    
        JS_ClearPendingException(cx);
        return;
    }
    if (!jsj_env->pending_js_errors) {
#ifdef DEBUG
        

        if ((*jEnv)->ExceptionOccurred(jEnv)) {
            
            JS_ASSERT(0);
            (*jEnv)->ExceptionClear(jEnv);
        }
#endif
        return;
    }

    
    

    error = jsj_env->pending_js_errors;
    while (error->next)
        error = error->next;
    
    



    if (error->java_exception) {
        (*jEnv)->Throw(jEnv, error->java_exception);
        goto done;
    }

    


    
    message_jstr = NULL;
    if (error->message) {
        message_jstr = (*jEnv)->NewStringUTF(jEnv, error->message);
        if (!message_jstr)
            goto out_of_memory;
    }

    report = &error->report;

    filename_jstr = NULL;
    if (report->filename) {
        filename_jstr = (*jEnv)->NewStringUTF(jEnv, report->filename);
        if (!filename_jstr)
            goto out_of_memory;
    }

    linebuf_jstr = NULL;
    if (report->linebuf) {
        linebuf_jstr = (*jEnv)->NewStringUTF(jEnv, report->linebuf);
        if (!linebuf_jstr)
            goto out_of_memory;
    }

    lineno = report->lineno;
    index = report->linebuf ? report->tokenptr - report->linebuf : 0;

    
    java_exception = (*jEnv)->NewObject(jEnv, njJSException, njJSException_JSException,
                                        message_jstr, filename_jstr, lineno, linebuf_jstr, index);
    if (!java_exception)
        goto out_of_memory;

    
    if ((*jEnv)->Throw(jEnv, java_exception) < 0) {
        JS_ASSERT(0);
        jsj_UnexpectedJavaError(cx, jEnv, "Couldn't throw JSException\n");
    }
    goto done;

out_of_memory:
    
    JS_ASSERT(0);
    jsj_LogError("Out of memory while attempting to throw JSException\n");

done:
    jsj_ClearPendingJSErrors(jsj_env);
    



    if (message_jstr)
        (*jEnv)->DeleteLocalRef(jEnv, message_jstr);
    if (filename_jstr)
        (*jEnv)->DeleteLocalRef(jEnv, filename_jstr);
    if (linebuf_jstr)
        (*jEnv)->DeleteLocalRef(jEnv, linebuf_jstr);
    if (java_exception)
        (*jEnv)->DeleteLocalRef(jEnv, java_exception);
}









JSBool
jsj_ReportUncaughtJSException(JSContext *cx, JNIEnv *jEnv, jthrowable java_exception)
{
    JSBool success;
    JSErrorReport report;
    const char *linebuf, *filename, *message, *tokenptr;
    jint lineno, token_index;
    jstring linebuf_jstr, filename_jstr, message_jstr;

    
    memset(&report, 0, sizeof(JSErrorReport));
    success = JS_FALSE;
    filename_jstr = linebuf_jstr = message_jstr = NULL;
    filename = message = linebuf = tokenptr = NULL;

    lineno = (*jEnv)->GetIntField(jEnv, java_exception, njJSException_lineno);
    report.lineno = lineno;

    filename_jstr = (*jEnv)->GetObjectField(jEnv, java_exception, njJSException_filename);
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        jsj_UnexpectedJavaError(cx, jEnv, "Unable to access filename field of a JSException");
        goto done;
    }
    if (filename_jstr)
        filename = (*jEnv)->GetStringUTFChars(jEnv, filename_jstr, 0);
    report.filename = filename;

    linebuf_jstr = (*jEnv)->GetObjectField(jEnv, java_exception, njJSException_source);
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        jsj_UnexpectedJavaError(cx, jEnv, "Unable to access source field of a JSException");
        goto done;
    }
    if (linebuf_jstr)
        linebuf = (*jEnv)->GetStringUTFChars(jEnv, linebuf_jstr, 0);
    report.linebuf = linebuf;

    token_index = (*jEnv)->GetIntField(jEnv, java_exception, njJSException_lineno);
    report.tokenptr = linebuf + token_index;

    message_jstr = (*jEnv)->CallObjectMethod(jEnv, java_exception, jlThrowable_getMessage);
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        jsj_UnexpectedJavaError(cx, jEnv, "Unable to access message of a JSException");
        goto done;
    }
    if (message_jstr)
        message = (*jEnv)->GetStringUTFChars(jEnv, message_jstr, 0);

    js_ReportErrorAgain(cx, message, &report);

    success = JS_TRUE;

done:

    if (filename_jstr && filename)
        (*jEnv)->ReleaseStringUTFChars(jEnv, filename_jstr, filename);
    if (linebuf_jstr && linebuf)
        (*jEnv)->ReleaseStringUTFChars(jEnv, linebuf_jstr, linebuf);
    if (message_jstr && message)
        (*jEnv)->ReleaseStringUTFChars(jEnv, message_jstr, message);
    return success;
}















JSJavaThreadState *
jsj_enter_js(JNIEnv *jEnv, void* applet_obj, jobject java_wrapper_obj,
             JSContext **cxp, JSObject **js_objp, JSErrorReporter *old_error_reporterp,
             void **pNSIPrincipaArray, int numPrincipals, void *pNSISecurityContext)
{
    JSContext *cx;
    char *err_msg;
    JSObject *js_obj;
    JSJavaThreadState *jsj_env;

    cx = NULL;
    err_msg = NULL;

    
    if (JSJ_callbacks && JSJ_callbacks->enter_js_from_java) {
#ifdef OJI
        if (!JSJ_callbacks->enter_js_from_java(jEnv, &err_msg, pNSIPrincipaArray, numPrincipals, pNSISecurityContext,applet_obj))
#else
        if (!JSJ_callbacks->enter_js_from_java(jEnv, &err_msg))
#endif
            goto entry_failure;
    }

    
    if (js_objp) {

#ifdef PRESERVE_JSOBJECT_IDENTITY
#if JS_BYTES_PER_LONG == 8
        js_obj = (JSObject *)((*jEnv)->GetLongField(jEnv, java_wrapper_obj, njJSObject_long_internal));
#else
        js_obj = (JSObject *)((*jEnv)->GetIntField(jEnv, java_wrapper_obj, njJSObject_internal));
#endif
#else   
        js_obj = jsj_UnwrapJSObjectWrapper(jEnv, java_wrapper_obj);
#endif  

        JS_ASSERT(js_obj);
        if (!js_obj)
            goto error;
        *js_objp = js_obj;
    }

    
    jsj_env = jsj_MapJavaThreadToJSJavaThreadState(jEnv, &err_msg);
    if (!jsj_env)
        goto error;

    
    cx = jsj_env->cx;
    if (!cx) {
        


        if (JSJ_callbacks && JSJ_callbacks->map_jsj_thread_to_js_context) {
#ifdef OJI
            cx = JSJ_callbacks->map_jsj_thread_to_js_context(jsj_env,
                                                             applet_obj,
                                                             jEnv, &err_msg);
#else
            cx = JSJ_callbacks->map_jsj_thread_to_js_context(jsj_env,
                                                             jEnv, &err_msg);
#endif
            if (!cx)
                goto error;
        } else {
            err_msg = JS_smprintf("Unable to find/create JavaScript execution "
                                  "context for JNI thread 0x%08x", jEnv);
            goto error;
        }
    }
    *cxp = cx;

    



    *old_error_reporterp =
        JS_SetErrorReporter(cx, capture_js_error_reports_for_java);

#ifdef JSJ_THREADSAFE
    JS_BeginRequest(cx);
#endif

    return jsj_env;

error:
    JS_ASSERT(!cx);
    
    if (JSJ_callbacks && JSJ_callbacks->exit_js)
        JSJ_callbacks->exit_js(jEnv, cx);

entry_failure:
    JS_ASSERT(!cx);
    if (err_msg) {
        jsj_LogError(err_msg);
        JS_smprintf_free(err_msg);
    }

    return NULL;
}




JSBool
jsj_exit_js(JSContext *cx, JSJavaThreadState *jsj_env, JSErrorReporter original_reporter)
{
    JNIEnv *jEnv;

#ifdef JSJ_THREADSAFE
    JS_EndRequest(cx);
#endif

    
    JS_SetErrorReporter(cx, original_reporter);

    jEnv = jsj_env->jEnv;

#ifdef DEBUG
    
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        JS_ASSERT(0);
        jsj_LogError("Unhandled Java exception detected");
        return JS_FALSE;
    }
#endif

    




    throw_any_pending_js_error_as_a_java_exception(jsj_env);

    
    if (JSJ_callbacks && JSJ_callbacks->exit_js)
        JSJ_callbacks->exit_js(jEnv, cx);

    return JS_TRUE;
}



JavaClassDescriptor *
jsj_get_jlObject_descriptor(JSContext *cx, JNIEnv *jEnv)
{
    
    static JavaClassDescriptor *jlObject_descriptor = NULL;

    if (!jlObject_descriptor)
        jlObject_descriptor = jsj_GetJavaClassDescriptor(cx, jEnv, jlObject);
    return jlObject_descriptor;
}








JNIEXPORT void JNICALL
Java_netscape_javascript_JSObject_initClass(JNIEnv *jEnv, jclass java_class)
{
    jsj_init_js_obj_reflections_table();
}






JNIEXPORT jobject JNICALL
Java_netscape_javascript_JSObject_getMember(JNIEnv *jEnv,
                                            jobject java_wrapper_obj,
                                            jstring property_name_jstr)
{
    JSContext *cx = NULL;
    JSObject *js_obj;
    jsval js_val;
    int dummy_cost;
    JSBool dummy_bool;
    const jchar *property_name_ucs2;
    jsize property_name_len;
    JSErrorReporter saved_reporter;
    jobject member;
    jboolean is_copy;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return NULL;

    property_name_ucs2 = NULL;
    if (!property_name_jstr) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                             JSJMSG_NULL_MEMBER_NAME);
        member = NULL;
        goto done;
    }

    
    property_name_ucs2 = (*jEnv)->GetStringChars(jEnv, property_name_jstr, &is_copy);
    if (!property_name_ucs2) {
        JS_ASSERT(0);
        goto done;
    }
    property_name_len = (*jEnv)->GetStringLength(jEnv, property_name_jstr);
    
    if (!JS_GetUCProperty(cx, js_obj, property_name_ucs2, property_name_len, &js_val))
        goto done;

    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, 
                                   jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &member, &dummy_bool);

done:
    if (property_name_ucs2)
        (*jEnv)->ReleaseStringChars(jEnv, property_name_jstr, property_name_ucs2);
    if (!jsj_exit_js(cx, jsj_env, saved_reporter))
        return NULL;
    
    return member;
}






JNIEXPORT jobject JNICALL
Java_netscape_javascript_JSObject_getSlot(JNIEnv *jEnv,
                                          jobject java_wrapper_obj,
                                          jint slot)
{
    JSContext *cx = NULL;
    JSObject *js_obj;
    jsval js_val;
    int dummy_cost;
    JSBool dummy_bool;
    JSErrorReporter saved_reporter;
    jobject member;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return NULL;
    

    if (!JS_GetElement(cx, js_obj, slot, &js_val))
        goto done;
    if (!jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                        &dummy_cost, &member, &dummy_bool))
        goto done;

done:
    if (!jsj_exit_js(cx, jsj_env, saved_reporter))
        return NULL;
    
    return member;
}






JNIEXPORT void JNICALL
Java_netscape_javascript_JSObject_setMember(JNIEnv *jEnv,
                                            jobject java_wrapper_obj,
                                            jstring property_name_jstr,
                                            jobject java_obj)
{
    JSContext *cx = NULL;
    JSObject *js_obj;
    jsval js_val;
    const jchar *property_name_ucs2;
    jsize property_name_len;
    JSErrorReporter saved_reporter;
    jboolean is_copy;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return;
    
    property_name_ucs2 = NULL;
    if (!property_name_jstr) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                            JSJMSG_NULL_MEMBER_NAME);
        goto done;
    }

    
    property_name_ucs2 = (*jEnv)->GetStringChars(jEnv, property_name_jstr, &is_copy);
    if (!property_name_ucs2) {
        JS_ASSERT(0);
        goto done;
    }
    property_name_len = (*jEnv)->GetStringLength(jEnv, property_name_jstr);
    
    if (!jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_obj, &js_val))
        goto done;

    JS_SetUCProperty(cx, js_obj, property_name_ucs2, property_name_len, &js_val);

done:
    if (property_name_ucs2)
        (*jEnv)->ReleaseStringChars(jEnv, property_name_jstr, property_name_ucs2);
    jsj_exit_js(cx, jsj_env, saved_reporter);
}






JNIEXPORT void JNICALL
Java_netscape_javascript_JSObject_setSlot(JNIEnv *jEnv,
                                          jobject java_wrapper_obj,
                                          jint slot,
                                          jobject java_obj)
{
    JSContext *cx = NULL;
    JSObject *js_obj;
    jsval js_val;
    JSErrorReporter saved_reporter;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return;
    
    if (!jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_obj, &js_val))
        goto done;
    JS_SetElement(cx, js_obj, slot, &js_val);

done:
    jsj_exit_js(cx, jsj_env, saved_reporter);
}






JNIEXPORT void JNICALL
Java_netscape_javascript_JSObject_removeMember(JNIEnv *jEnv,
                                               jobject java_wrapper_obj,
                                               jstring property_name_jstr)
{
    JSContext *cx = NULL;
    JSObject *js_obj;
    jsval js_val;
    const jchar *property_name_ucs2;
    jsize property_name_len;
    JSErrorReporter saved_reporter;
    jboolean is_copy;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return;
    
    if (!property_name_jstr) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                            JSJMSG_NULL_MEMBER_NAME);
        goto done;
    }
    
    property_name_ucs2 = (*jEnv)->GetStringChars(jEnv, property_name_jstr, &is_copy);
    if (!property_name_ucs2) {
        JS_ASSERT(0);
        goto done;
    }
    property_name_len = (*jEnv)->GetStringLength(jEnv, property_name_jstr);
    
    JS_DeleteUCProperty2(cx, js_obj, property_name_ucs2, property_name_len, &js_val);

    (*jEnv)->ReleaseStringChars(jEnv, property_name_jstr, property_name_ucs2);

done:
    jsj_exit_js(cx, jsj_env, saved_reporter);
    return;
}






JNIEXPORT jobject JNICALL
Java_netscape_javascript_JSObject_call(JNIEnv *jEnv, jobject java_wrapper_obj,
                                       jstring function_name_jstr, jobjectArray java_args)
{
    int i, argc, arg_num;
    jsval *argv;
    JSContext *cx = NULL;
    JSObject *js_obj;
    jsval js_val, function_val;
    int dummy_cost;
    JSBool dummy_bool;
    const jchar *function_name_ucs2;
    jsize function_name_len;
    JSErrorReporter saved_reporter;
    jboolean is_copy;
    jobject result;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return NULL;
    
    function_name_ucs2 = NULL;
    result = NULL;
    if (!function_name_jstr) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                                    JSJMSG_NULL_FUNCTION_NAME);
        goto done;
    }

    
    function_name_ucs2 = (*jEnv)->GetStringChars(jEnv, function_name_jstr, &is_copy);
    if (!function_name_ucs2) {
        JS_ASSERT(0);
        goto done;
    }
    function_name_len = (*jEnv)->GetStringLength(jEnv, function_name_jstr);
    
    
    if (java_args) {
        argc = (*jEnv)->GetArrayLength(jEnv, java_args);
        argv = (jsval*)JS_malloc(cx, argc * sizeof(jsval));
    } else {
        argc = 0;
        argv = 0;
    }

    
    for (arg_num = 0; arg_num < argc; arg_num++) {
        jobject arg = (*jEnv)->GetObjectArrayElement(jEnv, java_args, arg_num);

        if (!jsj_ConvertJavaObjectToJSValue(cx, jEnv, arg, &argv[arg_num]))
            goto cleanup_argv;
        JS_AddNamedRoot(cx, &argv[arg_num], "&argv[arg_num]");
    }

    if (!JS_GetUCProperty(cx, js_obj, function_name_ucs2, function_name_len,
                          &function_val))
        goto cleanup_argv;

    if (!JS_CallFunctionValue(cx, js_obj, function_val, argc, argv, &js_val))
        goto cleanup_argv;

    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &result, &dummy_bool);

cleanup_argv:
    if (argv) {
        for (i = 0; i < arg_num; i++)
            JS_RemoveRoot(cx, &argv[i]);
        JS_free(cx, argv);
    }

done:
    if (function_name_ucs2)
        (*jEnv)->ReleaseStringChars(jEnv, function_name_jstr, function_name_ucs2);
    if (!jsj_exit_js(cx, jsj_env, saved_reporter))
        return NULL;
    
    return result;
}






JNIEXPORT jobject JNICALL
Java_netscape_javascript_JSObject_eval(JNIEnv *jEnv,
                                       jobject java_wrapper_obj,
                                       jstring eval_jstr)
{
    const char *codebase;
    JSPrincipals *principals;
    JSContext *cx = NULL;
    JSBool eval_succeeded;
    JSObject *js_obj;
    jsval js_val;
    int dummy_cost;
    JSBool dummy_bool;
    const jchar *eval_ucs2;
    jsize eval_len;
    JSErrorReporter saved_reporter;
    jboolean is_copy;
    jobject result;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return NULL;
    
    result = NULL;
    eval_ucs2 = NULL;
    if (!eval_jstr) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_NULL_EVAL_ARG);
        goto done;
    }

    
    eval_ucs2 = (*jEnv)->GetStringChars(jEnv, eval_jstr, &is_copy);
    if (!eval_ucs2) {
        JS_ASSERT(0);
        goto done;
    }
    eval_len = (*jEnv)->GetStringLength(jEnv, eval_jstr);
    
    
    principals = NULL;
    if (JSJ_callbacks && JSJ_callbacks->get_JSPrincipals_from_java_caller)
        principals = JSJ_callbacks->get_JSPrincipals_from_java_caller(jEnv, cx, NULL, 0, NULL);
    codebase = principals ? principals->codebase : NULL;

    
    eval_succeeded = JS_EvaluateUCScriptForPrincipals(cx, js_obj, principals,
                                                      eval_ucs2, eval_len,
                                                      codebase, 0, &js_val);
    if (!eval_succeeded)
        goto done;

    
    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &result, &dummy_bool);

done:
    if (eval_ucs2)
        (*jEnv)->ReleaseStringChars(jEnv, eval_jstr, eval_ucs2);
    if (!jsj_exit_js(cx, jsj_env, saved_reporter))
        return NULL;
    
    return result;
}






JNIEXPORT jstring JNICALL
Java_netscape_javascript_JSObject_toString(JNIEnv *jEnv,
                                           jobject java_wrapper_obj)
{
    jstring result;
    JSContext *cx = NULL;
    JSObject *js_obj;
    JSString *jsstr;
    JSErrorReporter saved_reporter;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, NULL, java_wrapper_obj, &cx, &js_obj, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return NULL;
    
    result = NULL;
    jsstr = JS_ValueToString(cx, OBJECT_TO_JSVAL(js_obj));
    if (jsstr)
        result = jsj_ConvertJSStringToJavaString(cx, jEnv, jsstr);
    if (!result)
        result = (*jEnv)->NewStringUTF(jEnv, "*JavaObject*");

    if (!jsj_exit_js(cx, jsj_env, saved_reporter))
        return NULL;
    
    return result;
}






JNIEXPORT jobject JNICALL
Java_netscape_javascript_JSObject_getWindow(JNIEnv *jEnv,
                                            jclass js_object_class,
                                            jobject java_applet_obj)
{
    char *err_msg;
    JSContext *cx = NULL;
    JSObject *js_obj = NULL;
    jsval js_val;
    int dummy_cost;
    JSBool dummy_bool;
    JSErrorReporter saved_reporter;
    jobject java_obj;
    JSJavaThreadState *jsj_env;
    
    jsj_env = jsj_enter_js(jEnv, java_applet_obj, NULL, &cx, NULL, &saved_reporter, NULL, 0, NULL);
    if (!jsj_env)
        return NULL;
    
    err_msg = NULL;
    java_obj = NULL;
    if (JSJ_callbacks && JSJ_callbacks->map_java_object_to_js_object)
        js_obj = JSJ_callbacks->map_java_object_to_js_object(jEnv, java_applet_obj, &err_msg);
    if (!js_obj) {
        if (err_msg) {
            JS_ReportError(cx, err_msg);
            free(err_msg);
        }
        goto done;
    }
    js_val = OBJECT_TO_JSVAL(js_obj);
    jsj_ConvertJSValueToJavaObject(cx, jEnv, js_val, jsj_get_jlObject_descriptor(cx, jEnv),
                                   &dummy_cost, &java_obj, &dummy_bool);
done:
    if (!jsj_exit_js(cx, jsj_env, saved_reporter))
        return NULL;
    
    return java_obj;
}






JNIEXPORT void JNICALL
Java_netscape_javascript_JSObject_finalize(JNIEnv *jEnv, jobject java_wrapper_obj)
{
    JSBool success;
    JSObjectHandle *handle;

    success = JS_FALSE;

#if JS_BYTES_PER_LONG == 8 || JS_BYTES_PER_WORD == 8
    handle = (JSObjectHandle *)((*jEnv)->GetLongField(jEnv, java_wrapper_obj, njJSObject_long_internal));
#else    
    handle = (JSObjectHandle *)((*jEnv)->GetIntField(jEnv, java_wrapper_obj, njJSObject_internal));
#endif
    JS_ASSERT(handle);
    if (!handle)
        return;

    success = JS_RemoveRootRT(handle->rt, &handle->js_obj);
    free(handle);

    JS_ASSERT(success);
}







JNIEXPORT jboolean JNICALL
Java_netscape_javascript_JSObject_equals(JNIEnv *jEnv,
                                         jobject java_wrapper_obj,
                                         jobject comparison_obj)
{
#ifdef PRESERVE_JSOBJECT_IDENTITY
#    error "Missing code should be added here"
#else
    JSObject *js_obj1, *js_obj2;

    
    if (!comparison_obj)
        return 0;
    if (!(*jEnv)->IsInstanceOf(jEnv, comparison_obj, njJSObject))
        return 0;

    js_obj1 = jsj_UnwrapJSObjectWrapper(jEnv, java_wrapper_obj);
    js_obj2 = jsj_UnwrapJSObjectWrapper(jEnv, comparison_obj);

    return (js_obj1 == js_obj2);
#endif  
}

