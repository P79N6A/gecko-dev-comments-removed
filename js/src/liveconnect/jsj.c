





































 








#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"        
#include "jsjava.h"             

#ifdef JSJ_THREADSAFE
#    include "prmon.h"
#endif

JSBool jsj_JSIsCallingApplet = JS_FALSE;







static void
report_java_initialization_error(JNIEnv *jEnv, const char *js_error_msg)
{
    const char *error_msg, *java_error_msg;

    java_error_msg = NULL;

    if (jEnv) {
        java_error_msg = jsj_GetJavaErrorMessage(jEnv);
        (*jEnv)->ExceptionClear(jEnv);
    }

    if (java_error_msg) { 
        error_msg = JS_smprintf("initialization error: %s (%s)\n",
                                js_error_msg, java_error_msg);
        free((void*)java_error_msg);
    } else {
        error_msg = JS_smprintf("initialization error: %s\n",
                                js_error_msg);
    }

    jsj_LogError(error_msg);
    JS_smprintf_free((char*)error_msg);
}






jclass jlObject;                        
jclass jlrMethod;                       
jclass jlrField;                        
jclass jlrArray;                        
jclass jlVoid;                          
jclass jlrConstructor;                  
jclass jlThrowable;                     
jclass jlSystem;                        
jclass jlClass;                         
jclass jlBoolean;                       
jclass jlDouble;                        
jclass jlString;                        
jclass jaApplet;                        
jclass njJSObject;                      
jclass njJSException;                   
jclass njJSUtil;                        

jmethodID jlClass_getMethods;           
jmethodID jlClass_getConstructors;      
jmethodID jlClass_getFields;            
jmethodID jlClass_getName;              
jmethodID jlClass_getComponentType;     
jmethodID jlClass_getModifiers;         
jmethodID jlClass_isArray;              

jmethodID jlrMethod_getName;            
jmethodID jlrMethod_getParameterTypes;  
jmethodID jlrMethod_getReturnType;      
jmethodID jlrMethod_getModifiers;       

jmethodID jlrConstructor_getParameterTypes; 
jmethodID jlrConstructor_getModifiers;  

jmethodID jlrField_getName;             
jmethodID jlrField_getType;             
jmethodID jlrField_getModifiers;        

jmethodID jlrArray_newInstance;         

jmethodID jlBoolean_Boolean;            
jmethodID jlBoolean_booleanValue;       
jmethodID jlDouble_Double;              
jmethodID jlDouble_doubleValue;         

jmethodID jlThrowable_toString;         
jmethodID jlThrowable_getMessage;       

jmethodID jlSystem_identityHashCode;    

jobject jlVoid_TYPE;                    

jmethodID njJSException_JSException;    
jmethodID njJSException_JSException_wrap;
jmethodID njJSObject_JSObject;          
jmethodID njJSUtil_getStackTrace;       
jfieldID njJSObject_internal;           
jfieldID njJSObject_long_internal;      
jfieldID njJSException_lineno;          
jfieldID njJSException_tokenIndex;      
jfieldID njJSException_source;          
jfieldID njJSException_filename;        
jfieldID njJSException_wrappedExceptionType;        
jfieldID njJSException_wrappedException;        


#define LOAD_CLASS(qualified_name, class)                                    \
    {                                                                        \
        jclass _##class = (*jEnv)->FindClass(jEnv, #qualified_name);         \
        if (_##class == 0) {                                                 \
            (*jEnv)->ExceptionClear(jEnv);                                   \
            report_java_initialization_error(jEnv,                           \
                "Can't load class " #qualified_name);                        \
            return JS_FALSE;                                                 \
        }                                                                    \
        class = (*jEnv)->NewGlobalRef(jEnv, _##class);                       \
        (*jEnv)->DeleteLocalRef(jEnv, _##class);                             \
    }


#define _LOAD_METHOD(qualified_class, method, mvar, signature, class, is_static)\
    if (is_static) {                                                         \
        class##_##mvar =                                                     \
            (*jEnv)->GetStaticMethodID(jEnv, class, #method, signature);     \
    } else {                                                                 \
        class##_##mvar =                                                     \
            (*jEnv)->GetMethodID(jEnv, class, #method, signature);           \
    }                                                                        \
    if (class##_##mvar == 0) {                                               \
            (*jEnv)->ExceptionClear(jEnv);                                   \
        report_java_initialization_error(jEnv,                               \
               "Can't get mid for " #qualified_class "." #method "()");      \
        return JS_FALSE;                                                     \
    }


#define LOAD_METHOD(qualified_class, method, signature, class)               \
    _LOAD_METHOD(qualified_class, method, method, signature, class, JS_FALSE)


#define LOAD_STATIC_METHOD(qualified_class, method, signature, class)        \
    _LOAD_METHOD(qualified_class, method, method, signature, class, JS_TRUE)


#define LOAD_CONSTRUCTOR(qualified_class, method, signature, class)          \
    _LOAD_METHOD(qualified_class,<init>, method, signature, class, JS_FALSE)


#define _LOAD_FIELDID(qualified_class, field, signature, class, is_static)   \
    if (is_static) {                                                         \
        class##_##field = (*jEnv)->GetStaticFieldID(jEnv, class, #field, signature);\
    } else {                                                                 \
        class##_##field = (*jEnv)->GetFieldID(jEnv, class, #field, signature);\
    }                                                                        \
    if (class##_##field == 0) {                                              \
            (*jEnv)->ExceptionClear(jEnv);                                   \
        report_java_initialization_error(jEnv,                               \
                "Can't get fid for " #qualified_class "." #field);           \
        return JS_FALSE;                                                     \
    }


#define LOAD_FIELDID(qualified_class, field, signature, class)               \
    _LOAD_FIELDID(qualified_class, field, signature, class, JS_FALSE)


#define LOAD_FIELD_VAL(qualified_class, field, signature, class, type)       \
    {                                                                        \
        jfieldID field_id;                                                   \
        field_id = (*jEnv)->GetStaticFieldID(jEnv, class, #field, signature);\
        if (field_id == 0) {                                                 \
            report_java_initialization_error(jEnv,                           \
                "Can't get fid for " #qualified_class "." #field);           \
            return JS_FALSE;                                                 \
        }                                                                    \
        class##_##field =                                                    \
            (*jEnv)->GetStatic##type##Field(jEnv, class, field_id);          \
        if (class##_##field == 0) {                                          \
            (*jEnv)->ExceptionClear(jEnv);                                   \
            report_java_initialization_error(jEnv,                           \
                "Can't read static field " #qualified_class "." #field);     \
            return JS_FALSE;                                                 \
        }                                                                    \
    }



#define LOAD_FIELD_OBJ(qualified_class, field, signature, class)             \
    LOAD_FIELD_VAL(qualified_class, field, signature, class, Object);        \
    class##_##field = (*jEnv)->NewGlobalRef(jEnv, class##_##field);





static JSBool
init_java_VM_reflection(JSJavaVM *jsjava_vm, JNIEnv *jEnv)
{
    
    LOAD_CLASS(java/lang/Object,                jlObject);
    LOAD_CLASS(java/lang/Class,                 jlClass);
    LOAD_CLASS(java/lang/reflect/Method,        jlrMethod);
    LOAD_CLASS(java/lang/reflect/Constructor,   jlrConstructor);
    LOAD_CLASS(java/lang/reflect/Field,         jlrField);
    LOAD_CLASS(java/lang/reflect/Array,         jlrArray);
    LOAD_CLASS(java/lang/Throwable,             jlThrowable);
    LOAD_CLASS(java/lang/System,                jlSystem);
    LOAD_CLASS(java/lang/Boolean,               jlBoolean);
    LOAD_CLASS(java/lang/Double,                jlDouble);
    LOAD_CLASS(java/lang/String,                jlString);
    LOAD_CLASS(java/lang/Void,                  jlVoid);

    LOAD_CLASS(java/applet/Applet,              jaApplet);

    LOAD_METHOD(java.lang.Class,            getMethods,         "()[Ljava/lang/reflect/Method;",jlClass);
    LOAD_METHOD(java.lang.Class,            getConstructors,    "()[Ljava/lang/reflect/Constructor;",jlClass);
    LOAD_METHOD(java.lang.Class,            getFields,          "()[Ljava/lang/reflect/Field;", jlClass);
    LOAD_METHOD(java.lang.Class,            getName,            "()Ljava/lang/String;",         jlClass);
    LOAD_METHOD(java.lang.Class,            isArray,            "()Z",                          jlClass);
    LOAD_METHOD(java.lang.Class,            getComponentType,   "()Ljava/lang/Class;",          jlClass);
    LOAD_METHOD(java.lang.Class,            getModifiers,       "()I",                          jlClass);

    LOAD_METHOD(java.lang.reflect.Method,   getName,            "()Ljava/lang/String;",         jlrMethod);
    LOAD_METHOD(java.lang.reflect.Method,   getParameterTypes,  "()[Ljava/lang/Class;",         jlrMethod);
    LOAD_METHOD(java.lang.reflect.Method,   getReturnType,      "()Ljava/lang/Class;",          jlrMethod);
    LOAD_METHOD(java.lang.reflect.Method,   getModifiers,       "()I",                          jlrMethod);

    LOAD_METHOD(java.lang.reflect.Constructor,  getParameterTypes,  "()[Ljava/lang/Class;",     jlrConstructor);
    LOAD_METHOD(java.lang.reflect.Constructor,  getModifiers,       "()I",                      jlrConstructor);
    
    LOAD_METHOD(java.lang.reflect.Field,    getName,            "()Ljava/lang/String;",         jlrField);
    LOAD_METHOD(java.lang.reflect.Field,    getType,            "()Ljava/lang/Class;",          jlrField);
    LOAD_METHOD(java.lang.reflect.Field,    getModifiers,       "()I",                          jlrField);

    LOAD_STATIC_METHOD(java.lang.reflect.Array,
                                            newInstance,        "(Ljava/lang/Class;I)Ljava/lang/Object;",jlrArray);

    LOAD_METHOD(java.lang.Throwable,        toString,           "()Ljava/lang/String;",         jlThrowable);
    LOAD_METHOD(java.lang.Throwable,        getMessage,         "()Ljava/lang/String;",         jlThrowable);

    LOAD_METHOD(java.lang.Double,           doubleValue,        "()D",                          jlDouble);

    LOAD_METHOD(java.lang.Boolean,          booleanValue,       "()Z",                          jlBoolean);
    
    LOAD_STATIC_METHOD(java.lang.System,    identityHashCode,   "(Ljava/lang/Object;)I",        jlSystem);

    LOAD_CONSTRUCTOR(java.lang.Boolean,     Boolean,            "(Z)V",                         jlBoolean);
    LOAD_CONSTRUCTOR(java.lang.Double,      Double,             "(D)V",                         jlDouble);

    LOAD_FIELD_OBJ(java.lang.Void,          TYPE,               "Ljava/lang/Class;",            jlVoid);
  
    return JS_TRUE;
}

#if !defined(OJI) 





#include "netscape_javascript_JSObject.h"


static JSBool
JSObject_RegisterNativeMethods(JNIEnv* jEnv)
{

    static JNINativeMethod nativeMethods[] = {
        {"initClass", "()V", (void*)&Java_netscape_javascript_JSObject_initClass},

#ifndef OJI
        {"getMember", "(Ljava/lang/String;)Ljava/lang/Object;", (void*)&Java_netscape_javascript_JSObject_getMember},
        {"getSlot", "(I)Ljava/lang/Object;", (void*)&Java_netscape_javascript_JSObject_getSlot},
        {"setMember", "(Ljava/lang/String;Ljava/lang/Object;)V", (void*)&Java_netscape_javascript_JSObject_setMember},
        {"setSlot", "(ILjava/lang/Object;)V", (void*)&Java_netscape_javascript_JSObject_setSlot},
        {"removeMember", "(Ljava/lang/String;)V", (void*)&Java_netscape_javascript_JSObject_removeMember},
        {"call", "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;", (void*)&Java_netscape_javascript_JSObject_call},
        {"eval", "(Ljava/lang/String;)Ljava/lang/Object;", (void*)&Java_netscape_javascript_JSObject_eval},
        
        {"toString", "()Ljava/lang/String;", (void*)&Java_netscape_javascript_JSObject_toString},
        {"getWindow", "(Ljava/applet/Applet;)Lnetscape/javascript/JSObject;", (void*)&Java_netscape_javascript_JSObject_getWindow},
        {"finalize", "()V", (void*)&Java_netscape_javascript_JSObject_finalize},
        {"equals", "(Ljava/lang/Object;)Z", (void*)&Java_netscape_javascript_JSObject_equals}
#endif  

    };
    (*jEnv)->RegisterNatives(jEnv, njJSObject, nativeMethods, sizeof(nativeMethods) / sizeof(JNINativeMethod));
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        report_java_initialization_error(jEnv, "Couldn't initialize JSObject native methods.");
        (*jEnv)->ExceptionClear(jEnv);
        return JS_FALSE;
    }
    
    Java_netscape_javascript_JSObject_initClass(jEnv, njJSObject);
    return JS_TRUE;
}

#endif


static JSBool
init_netscape_java_classes(JSJavaVM *jsjava_vm, JNIEnv *jEnv)
{
    LOAD_CLASS(netscape/javascript/JSObject,    njJSObject);
    LOAD_CLASS(netscape/javascript/JSException, njJSException);
    LOAD_CLASS(netscape/javascript/JSUtil,      njJSUtil);

#if !defined(OJI) 
    JSObject_RegisterNativeMethods(jEnv);
#endif

#ifndef OJI
    LOAD_CONSTRUCTOR(netscape.javascript.JSObject,
                                            JSObject,           "(I)V",                         njJSObject);
#endif
    LOAD_CONSTRUCTOR(netscape.javascript.JSException,
                                            JSException,        "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;I)V",
                                                                                                njJSException);

    
    _LOAD_METHOD(netscape.javascript.JSException,<init>,
                 JSException_wrap, "(ILjava/lang/Object;)V",        
                 njJSException, JS_FALSE);

#ifndef OJI
    LOAD_FIELDID(netscape.javascript.JSObject,  
                                            internal,           "I",                            njJSObject);
#endif
    LOAD_FIELDID(netscape.javascript.JSException,  
                                            lineno,             "I",                            njJSException);
    LOAD_FIELDID(netscape.javascript.JSException,  
                                            tokenIndex,         "I",                            njJSException);
    LOAD_FIELDID(netscape.javascript.JSException,  
                                            source,             "Ljava/lang/String;",           njJSException);
    LOAD_FIELDID(netscape.javascript.JSException,  
                                            filename,           "Ljava/lang/String;",           njJSException);
    LOAD_FIELDID(netscape.javascript.JSException, wrappedExceptionType, "I",
                 njJSException);
    LOAD_FIELDID(netscape.javascript.JSException, wrappedException,
                 "Ljava/lang/Object;", njJSException);

    LOAD_STATIC_METHOD(netscape.javascript.JSUtil,
                                            getStackTrace,      "(Ljava/lang/Throwable;)Ljava/lang/String;",
                                                                                                njJSUtil);

    return JS_TRUE;
}

JSJavaVM *jsjava_vm_list = NULL;

static JSJavaThreadState *thread_list = NULL;

#ifdef JSJ_THREADSAFE
static PRMonitor *thread_list_monitor = NULL;
#endif







JSJavaVM *
JSJ_ConnectToJavaVM(SystemJavaVM *java_vm_arg, void* initargs)
{
    SystemJavaVM* java_vm;
    JSJavaVM *jsjava_vm;
    JNIEnv *jEnv;

    JS_ASSERT(JSJ_callbacks);
    JS_ASSERT(JSJ_callbacks->attach_current_thread);
    JS_ASSERT(JSJ_callbacks->detach_current_thread);
    JS_ASSERT(JSJ_callbacks->get_java_vm);

    jsjava_vm = (JSJavaVM*)malloc(sizeof(JSJavaVM));
    if (!jsjava_vm)
        return NULL;
    memset(jsjava_vm, 0, sizeof(JSJavaVM));

    java_vm = java_vm_arg;

    
    if (java_vm) {
        jEnv = JSJ_callbacks->attach_current_thread(java_vm);
        if (jEnv == NULL) {
            jsj_LogError("Failed to attach to Java VM thread\n");
            free(jsjava_vm);
            return NULL;
        }

        jsjava_vm->java_vm = java_vm;
        jsjava_vm->main_thread_env = jEnv;
    } else {
        jsjava_vm->init_args = initargs;
    }
       
#ifdef JSJ_THREADSAFE
    if (jsjava_vm_list == NULL) {
        thread_list_monitor =
            (struct PRMonitor *) PR_NewMonitor();
    }
#endif	

    
    jsjava_vm->next = jsjava_vm_list;
    jsjava_vm_list = jsjava_vm;

    return jsjava_vm;
}


static JSBool
jsj_ConnectToJavaVM(JSJavaVM *jsjava_vm)
{
    if (!jsjava_vm->java_vm) {
        JSBool ok;
        JS_ASSERT(JSJ_callbacks->create_java_vm);
        JS_ASSERT(JSJ_callbacks->destroy_java_vm);

        ok = JSJ_callbacks->create_java_vm(&jsjava_vm->java_vm,
                                           &jsjava_vm->main_thread_env,
                                           jsjava_vm->init_args);
        if (!ok) {
            jsj_LogError("Failed to create Java VM\n");
            return JS_FALSE;
        }

        
        jsjava_vm->jsj_created_java_vm = JS_TRUE;
    }
    
    if (!jsjava_vm->jsj_inited_java_vm) {
        






        init_netscape_java_classes(jsjava_vm, jsjava_vm->main_thread_env);

        

        if (!init_java_VM_reflection(jsjava_vm, jsjava_vm->main_thread_env) || 
            !jsj_InitJavaObjReflectionsTable()) {
            jsj_LogError("LiveConnect was unable to reflect one or more components of the Java runtime.\nGo to http://bugzilla.mozilla.org/show_bug.cgi?id=5369 for details.\n");
            



            return JS_FALSE;
        }
        
        jsjava_vm->jsj_inited_java_vm = JS_TRUE;
    }

    return JS_TRUE;
}

JSJCallbacks *JSJ_callbacks = NULL;


void
JSJ_Init(JSJCallbacks *callbacks)
{
    JS_ASSERT(callbacks);
    JSJ_callbacks = callbacks;
}










JSBool
JSJ_InitJSContext(JSContext *cx, JSObject *global_obj,
                  JavaPackageDef *predefined_packages)
{
    
    if (!jsj_init_JavaObject(cx, global_obj))
        return JS_FALSE;
    


    
    if (!jsj_init_JavaPackage(cx, global_obj, predefined_packages))
        return JS_FALSE;

    if (!jsj_init_JavaClass(cx, global_obj))
        return JS_FALSE;

    if (!jsj_init_JavaArray(cx, global_obj))
        return JS_FALSE;

    if (!jsj_init_JavaMember(cx, global_obj))
        return JS_FALSE;
    
    return JS_TRUE;
}


#define UNLOAD_CLASS(qualified_name, class)                                  \
    if (class) {                                                             \
        (*jEnv)->DeleteGlobalRef(jEnv, class);                               \
        class = NULL;                                                        \
    }







void
JSJ_DisconnectFromJavaVM(JSJavaVM *jsjava_vm)
{
    SystemJavaVM *java_vm;
    JSJavaVM *j, **jp;

    
    java_vm = jsjava_vm->java_vm;
    if (java_vm) {
        JNIEnv *jEnv = jsjava_vm->main_thread_env;

        
        jsj_DiscardJavaObjReflections(jEnv);
        jsj_DiscardJavaClassReflections(jEnv);

        if (jsjava_vm->jsj_created_java_vm) { 
            (void)JSJ_callbacks->destroy_java_vm(java_vm, jEnv);
        } else {
            UNLOAD_CLASS(java/lang/Object,                jlObject);
            UNLOAD_CLASS(java/lang/Class,                 jlClass);
            UNLOAD_CLASS(java/lang/reflect/Method,        jlrMethod);
            UNLOAD_CLASS(java/lang/reflect/Constructor,   jlrConstructor);
            UNLOAD_CLASS(java/lang/reflect/Field,         jlrField);
            UNLOAD_CLASS(java/lang/reflect/Array,         jlrArray);
            UNLOAD_CLASS(java/lang/Throwable,             jlThrowable);
            UNLOAD_CLASS(java/lang/System,                jlSystem);
            UNLOAD_CLASS(java/lang/Boolean,               jlBoolean);
            UNLOAD_CLASS(java/lang/Double,                jlDouble);
            UNLOAD_CLASS(java/lang/String,                jlString);
            UNLOAD_CLASS(java/lang/Void,                  jlVoid);
            UNLOAD_CLASS(java/applet/Applet,              jaApplet);
            UNLOAD_CLASS(netscape/javascript/JSObject,    njJSObject);
            UNLOAD_CLASS(netscape/javascript/JSException, njJSException);
            UNLOAD_CLASS(netscape/javascript/JSUtil,      njJSUtil);
        }
    }

    
    for (jp = &jsjava_vm_list; (j = *jp) != NULL; jp = &j->next) {
        if (j == jsjava_vm) {
            *jp = jsjava_vm->next;
            break;
        }
    }
    JS_ASSERT(j); 

#ifdef JSJ_THREADSAFE
    if (jsjava_vm_list == NULL) {
        PR_DestroyMonitor(thread_list_monitor);
        thread_list_monitor = NULL;
    }
#endif	
    
    free(jsjava_vm);
}

static JSJavaThreadState *
new_jsjava_thread_state(JSJavaVM *jsjava_vm, const char *thread_name, JNIEnv *jEnv)
{
    JSJavaThreadState *jsj_env;

    jsj_env = (JSJavaThreadState *)malloc(sizeof(JSJavaThreadState));
    if (!jsj_env)
        return NULL;
    memset(jsj_env, 0, sizeof(JSJavaThreadState));

    jsj_env->jEnv = jEnv;
    jsj_env->jsjava_vm = jsjava_vm;
    if (thread_name)
        jsj_env->name = strdup(thread_name);

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(thread_list_monitor);
#endif

    jsj_env->next = thread_list;
    thread_list = jsj_env;

#ifdef JSJ_THREADSAFE
    PR_ExitMonitor(thread_list_monitor);
#endif

    return jsj_env;
}

static JSJavaThreadState *
find_jsjava_thread(JNIEnv *jEnv)
{
    JSJavaThreadState *e, **p, *jsj_env;
    jsj_env = NULL;

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(thread_list_monitor);
#endif

    

    for (p = &thread_list; (e = *p) != NULL; p = &(e->next)) {
        if (e->jEnv == jEnv) {
            jsj_env = e;
            break;
        }
    }

    
    if (jsj_env && p != &thread_list) {
        *p = jsj_env->next;
        jsj_env->next = thread_list;
        thread_list = jsj_env;
    }
    
#ifdef JSJ_THREADSAFE
    PR_ExitMonitor(thread_list_monitor);
#endif

    return jsj_env;
}

JS_EXPORT_API(JSJavaThreadState *)
JSJ_AttachCurrentThreadToJava(JSJavaVM *jsjava_vm, const char *name, JNIEnv **java_envp)
{
    JNIEnv *jEnv;
    SystemJavaVM *java_vm;
    JSJavaThreadState *jsj_env;
    
    
    if (!jsj_ConnectToJavaVM(jsjava_vm))
        return NULL;

    
    if (!JSJ_callbacks || !JSJ_callbacks->attach_current_thread)
        return NULL;

    java_vm = jsjava_vm->java_vm;
    if (!(jEnv = JSJ_callbacks->attach_current_thread(java_vm)))
        return NULL;

    if (java_envp)
        *java_envp = jEnv;

    
    jsj_env = find_jsjava_thread(jEnv);
    if (jsj_env)
        return jsj_env;

    
    jsj_env = new_jsjava_thread_state(jsjava_vm, name, jEnv);

    return jsj_env;
}

static JSJavaVM *
map_java_vm_to_jsjava_vm(SystemJavaVM *java_vm)
{
    JSJavaVM *v;
    for (v = jsjava_vm_list; v; v = v->next) {
        if (!jsj_ConnectToJavaVM(v))
            return NULL;
        if (v->java_vm == java_vm)
            return v;
    }
    return NULL;
}











JSJavaThreadState *
jsj_MapJavaThreadToJSJavaThreadState(JNIEnv *jEnv, char **errp)
{
    JSJavaThreadState *jsj_env;
    SystemJavaVM *java_vm;
    JSJavaVM *jsjava_vm;

    
    jsj_env = find_jsjava_thread(jEnv);
    if (jsj_env)
        return jsj_env;

    


    
    if (JSJ_callbacks && JSJ_callbacks->get_java_vm)
        java_vm = JSJ_callbacks->get_java_vm(jEnv);
    else
        return NULL;
    if (java_vm == NULL)
        return NULL;

    
    jsjava_vm = map_java_vm_to_jsjava_vm(java_vm);
    if (!jsjava_vm) {
        *errp = JS_smprintf("Total weirdness:   No JSJavaVM wrapper ever created "
                            "for JavaVM 0x%08x", java_vm);
        return NULL;
    }

    jsj_env = new_jsjava_thread_state(jsjava_vm, NULL, jEnv);
    if (!jsj_env)
        return NULL;

    return jsj_env;
}










JS_EXPORT_API(JSContext *)
JSJ_SetDefaultJSContextForJavaThread(JSContext *cx, JSJavaThreadState *jsj_env)
{
    JSContext *old_context;
    old_context = jsj_env->cx;
    jsj_env->cx = cx;

    
    jsj_env->recursion_depth++;
    return old_context;
}

JS_EXPORT_API(JSBool)
JSJ_DetachCurrentThreadFromJava(JSJavaThreadState *jsj_env)
{
    SystemJavaVM *java_vm;
    JNIEnv* jEnv;
    JSJavaThreadState *e, **p;

    
    java_vm = jsj_env->jsjava_vm->java_vm;
    jEnv = jsj_env->jEnv;

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(thread_list_monitor);
#endif	

    if (!JSJ_callbacks->detach_current_thread(java_vm, jEnv)) {

#ifdef JSJ_THREADSAFE
        PR_ExitMonitor(thread_list_monitor);
#endif	

        return JS_FALSE;
    }

    
    jsj_ClearPendingJSErrors(jsj_env);

    for (p = &thread_list; (e = *p) != NULL; p = &(e->next)) {
        if (e == jsj_env) {
            *p = jsj_env->next;
            break;
        }
    }

    JS_ASSERT(e);

#ifdef JSJ_THREADSAFE
    PR_ExitMonitor(thread_list_monitor);
#endif	

    free(jsj_env);
    return JS_TRUE;
}



JSBool
JSJ_ConvertJavaObjectToJSValue(JSContext *cx, jobject java_obj, jsval *vp)
{
    JNIEnv *jEnv;
    JSBool result;
    JSJavaThreadState *jsj_env;
            
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    result = jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_obj, vp);
    jsj_ExitJava(jsj_env);
    return result;
}


JS_EXPORT_API(JSBool)
JSJ_ConvertJSValueToJavaObject(JSContext *cx, jsval v, jobject *vp)
{
    if (!JSVAL_IS_PRIMITIVE(v)) {
        JSObject *js_obj = JSVAL_TO_OBJECT(v);
        JavaObjectWrapper *java_wrapper = JS_GetPrivate(cx, js_obj);
        *vp = java_wrapper->java_obj;
        return JS_TRUE;
    }
    return JS_FALSE;
}


JS_EXPORT_API(JSBool)
JSJ_IsJSCallApplet()
{
    return jsj_JSIsCallingApplet;
}
