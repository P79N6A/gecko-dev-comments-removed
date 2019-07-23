





































 






 
#ifndef _JSJAVA_H
#define _JSJAVA_H

#ifndef jstypes_h___
#include "jstypes.h"
#endif

JS_BEGIN_EXTERN_C

#include "jni.h"             
#include "jsapi.h"           

#if JS_BYTES_PER_LONG == 8 || JS_BYTES_PER_WORD == 8
typedef jlong lcjsobject;
#else
typedef jint lcjsobject;
#endif





typedef struct JSJavaVM JSJavaVM;


typedef struct JSJavaThreadState JSJavaThreadState;







#ifdef OJI
typedef struct SystemJavaVM SystemJavaVM;
#else
typedef JavaVM SystemJavaVM;
#endif










typedef struct JSJCallbacks {

    











    JSContext *         (*map_jsj_thread_to_js_context)(JSJavaThreadState *jsj_env,
#ifdef OJI
                                                        void *java_applet_obj,
#endif
                                                        JNIEnv *jEnv,
                                                        char **errp);

    



    JSJavaThreadState * (*map_js_context_to_jsj_thread)(JSContext *cx,
                                                        char **errp);
                                                        
    




    JSObject *          (*map_java_object_to_js_object)(JNIEnv *jEnv, void *pJavaObject,
                                                        char **errp);
        
    

    JSPrincipals *      (*get_JSPrincipals_from_java_caller)(JNIEnv *jEnv, JSContext *pJSContext, void **pNSIPrincipaArray, int numPrincipals, void *pNSISecurityContext);
    
    





#ifdef OJI
    JSBool              (*enter_js_from_java)(JNIEnv *jEnv, char **errp,  void **pNSIPrincipaArray, int numPrincipals, void *pNSISecurityContext,void* applet_obj);
#else
    JSBool              (*enter_js_from_java)(JNIEnv *jEnv, char **errp);
#endif
    void                (*exit_js)(JNIEnv *jEnv, JSContext *cx);

    




    void                (*error_print)(const char *error_msg);

    


    jobject             (*get_java_wrapper)(JNIEnv *jEnv, lcjsobject jsobj);

    

    lcjsobject          (*unwrap_java_wrapper)(JNIEnv *jEnv, jobject java_wrapper);

    
    JSBool              (*create_java_vm)(SystemJavaVM* *jvm, JNIEnv* *initialEnv, void* initargs);
    JSBool              (*destroy_java_vm)(SystemJavaVM* jvm, JNIEnv* initialEnv);
    JNIEnv*             (*attach_current_thread)(SystemJavaVM* jvm);
    JSBool              (*detach_current_thread)(SystemJavaVM* jvm, JNIEnv* env);
    SystemJavaVM*       (*get_java_vm)(JNIEnv* env);

    
    void *              reserved[10];
} JSJCallbacks;









#define PKG_SYSTEM      1








#define PKG_USER        2


typedef struct JavaPackageDef {
    const char *        name;   
    const char *        path;   
    int                 flags;  
    int                 access;             
} JavaPackageDef;













JS_EXPORT_API(JSBool)
JSJ_SimpleInit(JSContext *cx, JSObject *global_obj,
               SystemJavaVM *java_vm, const char *classpath);


JS_EXPORT_API(void)
JSJ_SimpleShutdown(void);




















JS_EXPORT_API(void)
JSJ_Init(JSJCallbacks *callbacks);






JS_EXPORT_API(JSJavaVM *)
JSJ_ConnectToJavaVM(SystemJavaVM *java_vm, void* initargs);








JS_EXPORT_API(JSBool)
JSJ_InitJSContext(JSContext *cx, JSObject *global_obj,
                  JavaPackageDef *predefined_packages);
   







JS_EXPORT_API(JSJavaThreadState *)
JSJ_AttachCurrentThreadToJava(JSJavaVM *jsjava_vm, const char *thread_name,
                              JNIEnv **java_envp);


JS_EXPORT_API(JSBool)
JSJ_DetachCurrentThreadFromJava(JSJavaThreadState *jsj_env);














JS_EXPORT_API(JSContext *)
JSJ_SetDefaultJSContextForJavaThread(JSContext *cx, JSJavaThreadState *jsj_env);





JS_EXPORT_API(void)
JSJ_DisconnectFromJavaVM(JSJavaVM *);





JS_EXPORT_API(JSBool)
JSJ_ConvertJavaObjectToJSValue(JSContext *cx, jobject java_obj, jsval *vp);

JS_EXPORT_API(JSBool)
JSJ_ConvertJSValueToJavaObject(JSContext *cx, jsval js_val, jobject *vp);

JS_EXPORT_API(JSBool)
JSJ_IsJSCallApplet();

JS_END_EXTERN_C

#endif  
