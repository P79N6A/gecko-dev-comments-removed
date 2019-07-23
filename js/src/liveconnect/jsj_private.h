





















































 

#ifndef _JSJAVA_PVT_H
#define _JSJAVA_PVT_H

#include "jstypes.h"

#   include "jsprf.h"
#   include "jsutil.h"
#   include "jshash.h"

#include "jsj_hash.h"        
#include "jni.h"             
#include "jsapi.h"           
#include "jsjava.h"          





#if defined(OJI) && !defined(JSJ_INHIBIT_THREADSAFE)
#    define JSJ_INHIBIT_THREADSAFE
#endif


#ifndef JSJ_THREADSAFE
#    if defined(JS_THREADSAFE) && !defined(JSJ_INHIBIT_THREADSAFE)
#        define JSJ_THREADSAFE
#    endif
#endif






#ifdef __cplusplus
extern "C" {
#endif




typedef struct JavaMemberDescriptor JavaMemberDescriptor;
typedef struct JavaMethodSpec JavaMethodSpec;
typedef struct JavaClassDescriptor JavaClassDescriptor;
typedef struct JavaClassDescriptor JavaSignature;
typedef struct CapturedJSError CapturedJSError;






typedef enum {
    JAVA_SIGNATURE_UNKNOWN,
    JAVA_SIGNATURE_VOID,

    
    JAVA_SIGNATURE_BOOLEAN,
    JAVA_SIGNATURE_CHAR,
    JAVA_SIGNATURE_BYTE,
    JAVA_SIGNATURE_SHORT,
    JAVA_SIGNATURE_INT,
    JAVA_SIGNATURE_LONG,
    JAVA_SIGNATURE_FLOAT,
    JAVA_SIGNATURE_DOUBLE,

    
    JAVA_SIGNATURE_ARRAY,              
    JAVA_SIGNATURE_OBJECT,             

    JAVA_SIGNATURE_JAVA_LANG_BOOLEAN,  
    JAVA_SIGNATURE_JAVA_LANG_CLASS,    
    JAVA_SIGNATURE_JAVA_LANG_DOUBLE,   
    JAVA_SIGNATURE_NETSCAPE_JAVASCRIPT_JSOBJECT,  
    JAVA_SIGNATURE_JAVA_LANG_OBJECT,   
    JAVA_SIGNATURE_JAVA_LANG_STRING,   

    JAVA_SIGNATURE_LIMIT
} JavaSignatureChar;

#define IS_REFERENCE_TYPE(sig) ((int)(sig) >= (int)JAVA_SIGNATURE_ARRAY)
#define IS_OBJECT_TYPE(sig)    ((int)(sig) >= (int)JAVA_SIGNATURE_OBJECT)
#define IS_PRIMITIVE_TYPE(sig)                                               \
    (((int)(sig) >= (int)JAVA_SIGNATURE_BOOLEAN) && !IS_REFERENCE_TYPE(sig))                                    \


#define JSTYPE_EMPTY -1



typedef struct JavaMethodSignature {
    jsize                   num_args;               
    JavaSignature **        arg_signatures;         
    JavaSignature *         return_val_signature;   
} JavaMethodSignature;


typedef struct JavaFieldSpec {
    jfieldID                fieldID;    
    JavaSignature *         signature;  
    int                     modifiers;  
    const char *            name;       
} JavaFieldSpec;



struct JavaMethodSpec {
    jmethodID               methodID;   
    JavaMethodSignature     signature;
    const char *            name;       
    JavaMethodSpec *        next;       
    JSBool		    is_alias;	
};








struct JavaMemberDescriptor {
    const char *            name;       
    jsid                    id;         
    JavaFieldSpec *         field;      
    JavaMethodSpec *        methods;    
    JavaMemberDescriptor *  next;       
    JSObject *              invoke_func_obj; 
};


typedef enum {
    REFLECT_NO,
    REFLECT_IN_PROGRESS,
    REFLECT_COMPLETE
} ReflectStatus;


struct JavaClassDescriptor {
    const char *            name;       
    JavaSignatureChar       type;       
    jclass                  java_class; 
    int                     num_instance_members;
    int                     num_static_members;
    volatile ReflectStatus  instance_members_reflected;
    JavaMemberDescriptor *  instance_members;
    volatile ReflectStatus  static_members_reflected;
    JavaMemberDescriptor *  static_members;
    JavaMemberDescriptor *  constructors;
    int                     modifiers;  

    int                     ref_count;  
    JavaSignature *         array_component_signature; 
};

typedef struct JavaObjectWrapper JavaObjectWrapper;


struct JavaObjectWrapper {
    jobject                 java_obj;           
    JavaClassDescriptor *   class_descriptor;   
    union {
        JSJHashNumber       hash_code;          
        JavaObjectWrapper * next;               
    } u;
};




#define ACC_PUBLIC          0x0001      /* visible to everyone */
#define ACC_STATIC          0x0008      /* instance variable is static */
#define ACC_FINAL           0x0010      /* no further subclassing,overriding */
#define ACC_INTERFACE       0x0200      /* class is an interface */
#define ACC_ABSTRACT	    0x0400      /* no definition provided */



struct JSJavaVM {



    void *              init_args;              
    SystemJavaVM *      java_vm;
    JNIEnv *            main_thread_env;        
    JSPackedBool        jsj_created_java_vm;
    JSPackedBool        jsj_inited_java_vm;
    int                 num_attached_threads;
    JSJavaVM *          next;                   
};


struct JSJavaThreadState {
    const char *        name;           
    JSJavaVM *          jsjava_vm;      
    JNIEnv *            jEnv;           
    CapturedJSError *   pending_js_errors; 
    JSContext *         cx;             
    int			recursion_depth;
    JSJavaThreadState * next;           
};

struct JavaToJSSavedState {
	JSErrorReporter error_reporter;
	JSJavaThreadState* java_jsj_env;
};
typedef struct JavaToJSSavedState JavaToJSSavedState;




struct JSObjectHandle {
    JSObject *js_obj;
    JSRuntime *rt;
};
typedef struct JSObjectHandle JSObjectHandle;




extern JSJCallbacks *JSJ_callbacks;


extern JSClass JavaObject_class;
extern JSClass JavaArray_class;
extern JSClass JavaClass_class;
extern JSClass JavaMember_class;






extern jclass jlObject;                        
extern jclass jlrConstructor;                  
extern jclass jlrArray;                        
extern jclass jlThrowable;                     
extern jclass jlSystem;                        
extern jclass jlClass;                         
extern jclass jlBoolean;                       
extern jclass jlDouble;                        
extern jclass jlString;                        
extern jclass jaApplet;                        
extern jclass njJSObject;                      
extern jclass njJSException;                   
extern jclass njJSUtil;                        

extern jmethodID jlClass_getMethods;           
extern jmethodID jlClass_getConstructors;      
extern jmethodID jlClass_getFields;            
extern jmethodID jlClass_getName;              
extern jmethodID jlClass_getComponentType;     
extern jmethodID jlClass_getModifiers;         
extern jmethodID jlClass_isArray;              

extern jmethodID jlrMethod_getName;            
extern jmethodID jlrMethod_getParameterTypes;  
extern jmethodID jlrMethod_getReturnType;      
extern jmethodID jlrMethod_getModifiers;       

extern jmethodID jlrConstructor_getParameterTypes; 
extern jmethodID jlrConstructor_getModifiers;  

extern jmethodID jlrField_getName;             
extern jmethodID jlrField_getType;             
extern jmethodID jlrField_getModifiers;        

extern jmethodID jlrArray_newInstance;         

extern jmethodID jlThrowable_getMessage;       
extern jmethodID jlThrowable_toString;         

extern jmethodID jlBoolean_Boolean;            
extern jmethodID jlBoolean_booleanValue;       
extern jmethodID jlDouble_Double;              
extern jmethodID jlDouble_doubleValue;         

extern jmethodID jlSystem_identityHashCode;    

extern jobject jlVoid_TYPE;                    

extern jmethodID njJSException_JSException;    
extern jmethodID njJSException_JSException_wrap;
extern jmethodID njJSObject_JSObject;          
extern jmethodID njJSUtil_getStackTrace;       
extern jfieldID njJSObject_internal;           
extern jfieldID njJSObject_long_internal;      
extern jfieldID njJSException_lineno;          
extern jfieldID njJSException_tokenIndex;      
extern jfieldID njJSException_source;          
extern jfieldID njJSException_filename;        
extern jfieldID njJSException_wrappedExceptionType;        
extern jfieldID njJSException_wrappedException;        





extern JSBool jsj_JSIsCallingApplet;


extern JSBool
jsj_ComputeJavaClassSignature(JSContext *cx,
                              JavaSignature *signature,
                              jclass java_class);

extern const char *
jsj_ConvertJavaSignatureToString(JSContext *cx, JavaSignature *signature);
extern const char *
jsj_ConvertJavaSignatureToHRString(JSContext *cx,
                                   JavaSignature *signature);
extern JavaMethodSignature *
jsj_InitJavaMethodSignature(JSContext *cx, JNIEnv *jEnv, jobject method,
                            JavaMethodSignature *method_signature);

extern const char *
jsj_ConvertJavaMethodSignatureToString(JSContext *cx,
                                       JavaMethodSignature *method_signature);
extern const char *
jsj_ConvertJavaMethodSignatureToHRString(JSContext *cx,
                                         const char *method_name,
                                         JavaMethodSignature *method_signature);
extern void
jsj_PurgeJavaMethodSignature(JSContext *cx, JNIEnv *jEnv, JavaMethodSignature *signature);

extern JSBool
jsj_ConvertJSValueToJavaValue(JSContext *cx, JNIEnv *jEnv, jsval v, JavaSignature *signature,
			      int *cost, jvalue *java_value, JSBool *is_local_refp);
extern JSBool
jsj_ConvertJSValueToJavaObject(JSContext *cx, JNIEnv *jEnv, jsval v, JavaSignature *signature,
			       int *cost, jobject *java_value, JSBool *is_local_refp);
extern jstring
jsj_ConvertJSStringToJavaString(JSContext *cx, JNIEnv *jEnv, JSString *js_str);

extern JSBool
jsj_ConvertJavaValueToJSValue(JSContext *cx, JNIEnv *jEnv, JavaSignature *signature,
                              jvalue *java_value, jsval *vp);
extern JSBool
jsj_ConvertJavaObjectToJSValue(JSContext *cx, JNIEnv *jEnv,
                               jobject java_obj, jsval *vp);

extern JSString *
jsj_ConvertJavaStringToJSString(JSContext *cx, JNIEnv *jEnv, jstring java_str);

extern JSBool
jsj_ConvertJavaObjectToJSString(JSContext *cx, JNIEnv *jEnv,
                                JavaClassDescriptor *class_descriptor,
                                jobject java_obj, jsval *vp);
extern JSBool
jsj_ConvertJavaObjectToJSNumber(JSContext *cx, JNIEnv *jEnv,
                                JavaClassDescriptor *class_descriptor,
                                jobject java_obj, jsval *vp);
extern JSBool
jsj_ConvertJavaObjectToJSBoolean(JSContext *cx, JNIEnv *jEnv,
                                 JavaClassDescriptor *class_descriptor,
                                 jobject java_obj, jsval *vp);
extern JSJavaThreadState *
jsj_enter_js(JNIEnv *jEnv, void* java_applet_obj, jobject java_wrapper_obj,
         JSContext **cxp, JSObject **js_objp, JSErrorReporter *old_error_reporterp, 
         void **pNSIPrincipaArray, int numPrincipals, void *pNSISecurityContext);

extern JSBool
jsj_exit_js(JSContext *cx, JSJavaThreadState *jsj_env, JSErrorReporter old_error_reporterp);

extern JavaClassDescriptor *
jsj_get_jlObject_descriptor(JSContext *cx, JNIEnv *jEnv);

extern JSBool
jsj_remove_js_obj_reflection_from_hashtable(JSContext *cx, JSObject *js_obj);

extern JSBool
jsj_init_js_obj_reflections_table(void);


extern JSBool
jsj_init_JavaPackage(JSContext *, JSObject *,
                     JavaPackageDef *predefined_packages);


extern JSBool
jsj_init_JavaClass(JSContext *cx, JSObject *global_obj);

extern void
jsj_DiscardJavaClassReflections(JNIEnv *jEnv);

extern const char *
jsj_GetJavaClassName(JSContext *cx, JNIEnv *jEnv, jclass java_class);

extern JavaClassDescriptor *
jsj_GetJavaClassDescriptor(JSContext *cx, JNIEnv *jEnv, jclass java_class);

extern void
jsj_ReleaseJavaClassDescriptor(JSContext *cx, JNIEnv *jEnv, JavaClassDescriptor *class_descriptor);

extern JSObject *
jsj_define_JavaClass(JSContext *cx, JNIEnv *jEnv, JSObject *obj,
                     const char *unqualified_class_name,
                     jclass jclazz);

extern JavaMemberDescriptor *
jsj_GetJavaMemberDescriptor(JSContext *cx,
                            JNIEnv *jEnv,
                            JavaClassDescriptor *class_descriptor,
                            jstring member_name);

extern JavaMemberDescriptor *
jsj_LookupJavaMemberDescriptorById(JSContext *cx, JNIEnv *jEnv,
                                   JavaClassDescriptor *class_descriptor,
                                   jsid id);

extern JavaMemberDescriptor *
jsj_LookupJavaStaticMemberDescriptorById(JSContext *cx, JNIEnv *jEnv,
                                         JavaClassDescriptor *class_descriptor,
                                         jsid id);

extern JavaMemberDescriptor *
jsj_GetJavaStaticMemberDescriptor(JSContext *cx, JNIEnv *jEnv,
                                  JavaClassDescriptor *class_descriptor,
                                  jstring member_name);

extern JavaMemberDescriptor *
jsj_GetJavaClassConstructors(JSContext *cx,
                             JavaClassDescriptor *class_descriptor);

extern JavaMemberDescriptor *
jsj_LookupJavaClassConstructors(JSContext *cx, JNIEnv *jEnv,
                                JavaClassDescriptor *class_descriptor);

extern JavaMemberDescriptor *
jsj_GetClassInstanceMembers(JSContext *cx, JNIEnv *jEnv,
                            JavaClassDescriptor *class_descriptor);

extern JavaMemberDescriptor *
jsj_GetClassStaticMembers(JSContext *cx, JNIEnv *jEnv,
                          JavaClassDescriptor *class_descriptor);

extern JSBool
jsj_InitJavaClassReflectionsTable(void);



extern JSBool
jsj_GetJavaFieldValue(JSContext *cx, JNIEnv *jEnv, JavaFieldSpec *field_spec,
                      jobject java_obj, jsval *vp);
extern JSBool
jsj_SetJavaFieldValue(JSContext *cx, JNIEnv *jEnv, JavaFieldSpec *field_spec,
                      jobject java_obj, jsval js_val);
extern JSBool 
jsj_ReflectJavaFields(JSContext *cx, JNIEnv *jEnv,
                      JavaClassDescriptor *class_descriptor,
                      JSBool reflect_only_static_fields);
extern void
jsj_DestroyFieldSpec(JSContext *cx, JNIEnv *jEnv, JavaFieldSpec *field);


JS_EXTERN_API(JSBool)
jsj_JavaInstanceMethodWrapper(JSContext *cx, JSObject *obj,
                              uintN argc, jsval *argv, jsval *vp);
JS_EXTERN_API(JSBool)
jsj_JavaStaticMethodWrapper(JSContext *cx, JSObject *obj,
                            uintN argc, jsval *argv, jsval *vp);
JS_EXTERN_API(JSBool)
jsj_JavaConstructorWrapper(JSContext *cx, JSObject *obj,
                           uintN argc, jsval *argv, jsval *vp);
extern JSBool 
jsj_ReflectJavaMethods(JSContext *cx, JNIEnv *jEnv,
                       JavaClassDescriptor *class_descriptor,
                       JSBool reflect_only_static_methods);

extern JavaMemberDescriptor *
jsj_ResolveExplicitMethod(JSContext *cx, JNIEnv *jEnv,
			  JavaClassDescriptor *class_descriptor, 
			  jsid method_name_id,
			  JSBool is_static);
extern void
jsj_DestroyMethodSpec(JSContext *cx, JNIEnv *jEnv, JavaMethodSpec *method_spec);


extern JSBool
jsj_init_JavaMember(JSContext *, JSObject *);

extern JSBool
jsj_ReflectJavaMethodsAndFields(JSContext *cx, JavaClassDescriptor *class_descriptor,
                                JSBool reflect_only_statics);

extern JSObject *
jsj_CreateJavaMember(JSContext *cx, jsval method_val, jsval field_val);


extern JSBool
jsj_init_JavaObject(JSContext *, JSObject *);

extern JSBool
jsj_InitJavaObjReflectionsTable(void);

extern JSObject *
jsj_WrapJavaObject(JSContext *cx, JNIEnv *jEnv, jobject java_obj, jclass java_class);

extern void
jsj_DiscardJavaObjReflections(JNIEnv *jEnv);

extern JSBool JS_DLL_CALLBACK
JavaObject_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

JS_EXTERN_API(void)
JavaObject_finalize(JSContext *cx, JSObject *obj);

extern JSBool
JavaObject_resolve(JSContext *cx, JSObject *obj, jsval id);

extern JSBool
JavaObject_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_EXTERN_API(JSBool)
JavaObject_getPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp);


extern JSBool
jsj_init_JavaArray(JSContext *cx, JSObject *global_obj);

extern JSBool
jsj_GetJavaArrayElement(JSContext *cx, JNIEnv *jEnv, jarray java_array,
                        jsize index, JavaSignature *array_component_signature,
                        jsval *vp);
   
extern JSBool     
jsj_SetJavaArrayElement(JSContext *cx, JNIEnv *jEnv, jarray java_array,
                        jsize index, JavaSignature *array_component_signature,
                        jsval js_val);

                        
extern jobject
jsj_WrapJSObject(JSContext *cx, JNIEnv *jEnv, JSObject *js_obj);

extern JSObject *
jsj_UnwrapJSObjectWrapper(JNIEnv *jEnv, jobject java_wrapper_obj);

extern void
jsj_ClearPendingJSErrors(JSJavaThreadState *jsj_env);

extern JSBool
jsj_ReportUncaughtJSException(JSContext *cx, JNIEnv *jEnv, jthrowable java_exception);


extern void
jsj_ReportJavaError(JSContext *cx, JNIEnv *env, const char *format, ...);

extern void
jsj_UnexpectedJavaError(JSContext *cx, JNIEnv *env, const char *format, ...);

extern const char *
jsj_GetJavaErrorMessage(JNIEnv *env);

extern void
jsj_LogError(const char *error_msg);

extern const JSErrorFormatString * JS_DLL_CALLBACK
jsj_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);

JSJHashNumber JS_DLL_CALLBACK
jsj_HashJavaObject(const void *key, void* env);

intN JS_DLL_CALLBACK
jsj_JavaObjectComparator(const void *v1, const void *v2, void *arg);

extern JSJavaThreadState *
jsj_MapJavaThreadToJSJavaThreadState(JNIEnv *jEnv, char **errp);

extern void
jsj_MakeJNIClassname(char *jClassName);

extern const char *
jsj_ClassNameOfJavaObject(JSContext *cx, JNIEnv *jEnv, jobject java_object);

extern jsize
jsj_GetJavaArrayLength(JSContext *cx, JNIEnv *jEnv, jarray java_array);

extern JSBool
JavaStringToId(JSContext *cx, JNIEnv *jEnv, jstring jstr, jsid *idp);

extern const char *
jsj_DupJavaStringUTF(JSContext *cx, JNIEnv *jEnv, jstring jstr);

extern JSJavaThreadState *
jsj_EnterJava(JSContext *cx, JNIEnv **envp);

extern void
jsj_ExitJava(JSJavaThreadState *jsj_env);

extern JSObjectMap * JS_DLL_CALLBACK
jsj_wrapper_newObjectMap(JSContext *cx, jsrefcount nrefs, JSObjectOps *ops,
                         JSClass *clasp, JSObject *obj);

extern void JS_DLL_CALLBACK
jsj_wrapper_destroyObjectMap(JSContext *cx, JSObjectMap *map);

extern jsval JS_DLL_CALLBACK
jsj_wrapper_getRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot);

extern JSBool JS_DLL_CALLBACK
jsj_wrapper_setRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot, jsval v);

#ifdef DEBUG
#define DEBUG_LOG(args) printf args
#endif

#define JS_FREE_IF(cx, x)                                                   \
    JS_BEGIN_MACRO                                                          \
        if (x)                                                              \
            JS_free(cx, x);                                                 \
    JS_END_MACRO


enum JSJErrNum {
#define MSG_DEF(name, number, format, count) \
    name = number,
#include "jsj.msg"
#undef MSG_DEF
    JSJ_Err_Limit
#undef MSGDEF
};

#ifdef __cplusplus
} 

#endif

#endif   
