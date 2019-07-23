























































#include <stdlib.h>
#include <string.h>

#include "jsobj.h"
#include "jsj_private.h"      
#include "jsj_hash.h"         

#ifdef JSJ_THREADSAFE
#include "prmon.h"
#endif















static JSJHashTable *java_obj_reflections = NULL;

#ifdef JSJ_THREADSAFE
static PRMonitor *java_obj_reflections_monitor = NULL;
static int java_obj_reflections_mutation_count = 0;
#endif

static JSBool installed_GC_callback = JS_FALSE;
static JSGCCallback old_GC_callback = NULL;
static JavaObjectWrapper* deferred_wrappers = NULL;

static JSBool JS_DLL_CALLBACK jsj_GC_callback(JSContext *cx, JSGCStatus status)
{
    if (status == JSGC_END && deferred_wrappers) {
        JNIEnv *jEnv;
        JSJavaThreadState *jsj_env = jsj_EnterJava(cx, &jEnv);
        if (jEnv) {
            JavaObjectWrapper* java_wrapper = deferred_wrappers;
            while (java_wrapper) {
                deferred_wrappers = java_wrapper->u.next;
                if (java_wrapper->java_obj)
                    (*jEnv)->DeleteGlobalRef(jEnv, java_wrapper->java_obj);
                jsj_ReleaseJavaClassDescriptor(cx, jEnv, java_wrapper->class_descriptor);
                JS_free(cx, java_wrapper);
                java_wrapper = deferred_wrappers;
            }
            jsj_ExitJava(jsj_env);
        }
    }
    
    return old_GC_callback ? old_GC_callback(cx, status) : JS_TRUE;
}

JSBool
jsj_InitJavaObjReflectionsTable(void)
{
    JS_ASSERT(!java_obj_reflections);

    java_obj_reflections =
        JSJ_NewHashTable(512, jsj_HashJavaObject, jsj_JavaObjectComparator,
                         NULL, NULL, NULL);
    if (!java_obj_reflections)
        return JS_FALSE;

#ifdef JSJ_THREADSAFE
    java_obj_reflections_monitor = (struct PRMonitor *) PR_NewMonitor();
    if (!java_obj_reflections_monitor) {
        JSJ_HashTableDestroy(java_obj_reflections);
        return JS_FALSE;
    }
#endif

    return JS_TRUE;
}

JSObject *
jsj_WrapJavaObject(JSContext *cx,
                   JNIEnv *jEnv,
                   jobject java_obj,
                   jclass java_class)
{
    JSJHashNumber hash_code;
    JSClass *js_class;
    JSObject *js_wrapper_obj;
    JavaObjectWrapper *java_wrapper;
    JavaClassDescriptor *class_descriptor;
    JSJHashEntry *he, **hep;

#ifdef JSJ_THREADSAFE
    int mutation_count;
#endif

    js_wrapper_obj = NULL;

    hash_code = jsj_HashJavaObject((void*)java_obj, (void*)jEnv);

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(java_obj_reflections_monitor);
#endif

    if (!installed_GC_callback) {
        



        old_GC_callback =  JS_SetGCCallback(cx, jsj_GC_callback);
        installed_GC_callback = JS_TRUE;
    }

    hep = JSJ_HashTableRawLookup(java_obj_reflections,
                                 hash_code, java_obj, (void*)jEnv);
    he = *hep;

#ifdef JSJ_THREADSAFE
    
    mutation_count = java_obj_reflections_mutation_count;

    

    PR_ExitMonitor(java_obj_reflections_monitor);
#endif

    if (he) {
        js_wrapper_obj = (JSObject *)he->value;
        if (js_wrapper_obj)
            return js_wrapper_obj;
    }

    
    class_descriptor = jsj_GetJavaClassDescriptor(cx, jEnv, java_class);
    if (!class_descriptor)
        return NULL;
    if (class_descriptor->type == JAVA_SIGNATURE_ARRAY) {
        js_class = &JavaArray_class;
    } else {
        JS_ASSERT(IS_OBJECT_TYPE(class_descriptor->type));
        js_class = &JavaObject_class;
    }

    
    js_wrapper_obj = JS_NewObject(cx, js_class, NULL, NULL);
    if (!js_wrapper_obj)
        return NULL;

    
    java_wrapper =
        (JavaObjectWrapper *)JS_malloc(cx, sizeof(JavaObjectWrapper));
    if (!java_wrapper) {
        jsj_ReleaseJavaClassDescriptor(cx, jEnv, class_descriptor);
        return NULL;
    }
    JS_SetPrivate(cx, js_wrapper_obj, java_wrapper);
    java_wrapper->class_descriptor = class_descriptor;
    java_wrapper->java_obj = NULL;

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(java_obj_reflections_monitor);

    

    if (mutation_count != java_obj_reflections_mutation_count) {
        hep = JSJ_HashTableRawLookup(java_obj_reflections,
                                     hash_code, java_obj, (void*)jEnv);
        he = *hep;
        if (he) {
            js_wrapper_obj = (JSObject *)he->value;
            if (js_wrapper_obj) {
                PR_ExitMonitor(java_obj_reflections_monitor);
                return js_wrapper_obj;
            }
        }
    }

    java_obj_reflections_mutation_count++;

#endif

    java_obj = (*jEnv)->NewGlobalRef(jEnv, java_obj);
    java_wrapper->java_obj = java_obj;
    if (!java_obj)
        goto out_of_memory;

    
    java_wrapper->u.hash_code = hash_code;

    
    he = JSJ_HashTableRawAdd(java_obj_reflections, hep, hash_code,
                             java_obj, js_wrapper_obj, (void*)jEnv);
#ifdef JSJ_THREADSAFE
    PR_ExitMonitor(java_obj_reflections_monitor);
#endif

    if (!he) {
        (*jEnv)->DeleteGlobalRef(jEnv, java_obj);
        goto out_of_memory;
    }

    return js_wrapper_obj;

out_of_memory:
    
    JS_ReportOutOfMemory(cx);
    return NULL;
}

static void
remove_java_obj_reflection_from_hashtable(jobject java_obj, JSJHashNumber hash_code)
{
    JSJHashEntry *he, **hep;

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(java_obj_reflections_monitor);
#endif

    hep = JSJ_HashTableRawLookup(java_obj_reflections, hash_code,
                                 java_obj, NULL);
    he = *hep;

    JS_ASSERT(he);
    if (he)
        JSJ_HashTableRawRemove(java_obj_reflections, hep, he, NULL);

#ifdef JSJ_THREADSAFE
    java_obj_reflections_mutation_count++;

    PR_ExitMonitor(java_obj_reflections_monitor);
#endif
}

JS_EXPORT_API(void)
JavaObject_finalize(JSContext *cx, JSObject *obj)
{
    JavaObjectWrapper *java_wrapper;
    jobject java_obj;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;

    java_wrapper = JS_GetPrivate(cx, obj);
    if (!java_wrapper)
        return;
    java_obj = java_wrapper->java_obj;

    if (java_obj) {
        remove_java_obj_reflection_from_hashtable(java_obj, java_wrapper->u.hash_code);

        
        java_wrapper->u.next = deferred_wrappers;
        deferred_wrappers = java_wrapper;
    } else {
        jsj_env = jsj_EnterJava(cx, &jEnv);
        if (jEnv) {
            jsj_ReleaseJavaClassDescriptor(cx, jEnv, java_wrapper->class_descriptor);
            JS_free(cx, java_wrapper);
            jsj_ExitJava(jsj_env);
        } else {
            java_wrapper->u.next = deferred_wrappers;
            deferred_wrappers = java_wrapper;
        }
    }
}


static JSIntn
enumerate_remove_java_obj(JSJHashEntry *he, JSIntn i, void *arg)
{
    JSJavaThreadState *jsj_env = (JSJavaThreadState *)arg;
    JNIEnv *jEnv = jsj_env->jEnv;
    jobject java_obj;
    JavaObjectWrapper *java_wrapper;
    JSObject *java_wrapper_obj;

    java_wrapper_obj = (JSObject *)he->value;

    
    java_wrapper = JS_GetPrivate(jsj_env->cx, java_wrapper_obj);
    java_obj = java_wrapper->java_obj;
    (*jEnv)->DeleteGlobalRef(jEnv, java_obj);
    java_wrapper->java_obj = NULL;
    return HT_ENUMERATE_REMOVE;
}




void
jsj_DiscardJavaObjReflections(JNIEnv *jEnv)
{
    JSJavaThreadState *jsj_env;
    char *err_msg;

    
    jsj_env = jsj_MapJavaThreadToJSJavaThreadState(jEnv, &err_msg);
    JS_ASSERT(jsj_env);
    if (!jsj_env) {
        if (err_msg) {
            jsj_LogError(err_msg);
            JS_smprintf_free(err_msg);
        }

        return;
    }

    JS_ASSERT(!err_msg);

    if (java_obj_reflections) {
        JSJ_HashTableEnumerateEntries(java_obj_reflections,
                                      enumerate_remove_java_obj,
                                      (void*)jsj_env);
        JSJ_HashTableDestroy(java_obj_reflections);
        java_obj_reflections = NULL;
    }
}

JSBool JS_DLL_CALLBACK
JavaObject_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    JavaObjectWrapper *java_wrapper;
    JavaClassDescriptor *class_descriptor;
    jobject java_obj;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    JSBool result;

    java_wrapper = JS_GetPrivate(cx, obj);
    if (!java_wrapper) {
        if (type == JSTYPE_OBJECT) {
            *vp = OBJECT_TO_JSVAL(obj);
            return JS_TRUE;
        }

        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                             JSJMSG_BAD_OP_JOBJECT);
        return JS_FALSE;
    }

    java_obj = java_wrapper->java_obj;
    class_descriptor = java_wrapper->class_descriptor;

    switch (type) {
    case JSTYPE_OBJECT:
        *vp = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;

    case JSTYPE_FUNCTION:
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                             JSJMSG_CONVERT_TO_FUNC);
        return JS_FALSE;

    case JSTYPE_VOID:
    case JSTYPE_STRING:
        
        jsj_env = jsj_EnterJava(cx, &jEnv);
        if (!jEnv)
            return JS_FALSE;

        

        result = jsj_ConvertJavaObjectToJSString(cx, jEnv, class_descriptor, java_obj, vp);
        jsj_ExitJava(jsj_env);
        return result;

    case JSTYPE_NUMBER:
        
        jsj_env = jsj_EnterJava(cx, &jEnv);
        if (!jEnv)
            return JS_FALSE;

        
        result = jsj_ConvertJavaObjectToJSNumber(cx, jEnv, class_descriptor, java_obj, vp);
        jsj_ExitJava(jsj_env);
        return result;

    case JSTYPE_BOOLEAN:
        
        jsj_env = jsj_EnterJava(cx, &jEnv);
        if (!jEnv)
            return JS_FALSE;

        
        result = jsj_ConvertJavaObjectToJSBoolean(cx, jEnv, class_descriptor, java_obj, vp);
        jsj_ExitJava(jsj_env);
        return result;

    default:
        JS_ASSERT(0);
        return JS_FALSE;
    }
}







static JSBool
inherit_props_from_JS_natives(JSContext *cx, const char *js_constructor_name,
                              const char *member_name, jsval *vp)
{
    JSObject *global_obj, *constructor_obj, *prototype_obj;
    jsval constructor_val, prototype_val;

    global_obj = JS_GetGlobalObject(cx);
    JS_ASSERT(global_obj);
    if (!global_obj)
        return JS_FALSE;

    JS_GetProperty(cx, global_obj, js_constructor_name, &constructor_val);
    JS_ASSERT(JSVAL_IS_OBJECT(constructor_val));
    constructor_obj = JSVAL_TO_OBJECT(constructor_val);

    JS_GetProperty(cx, constructor_obj, "prototype", &prototype_val);
    JS_ASSERT(JSVAL_IS_OBJECT(prototype_val));
    prototype_obj = JSVAL_TO_OBJECT(prototype_val);

    return JS_GetProperty(cx, prototype_obj, member_name, vp) && *vp != JSVAL_VOID;
}

struct JSJPropertyInfo {
    JSBool wantProp;            
    const char* name;           
    uintN attributes;           
    JSProperty *prop;           

};
typedef struct JSJPropertyInfo JSJPropertyInfo;

static JSBool
lookup_member_by_id(JSContext *cx, JNIEnv *jEnv, JSObject *obj,
                    JavaObjectWrapper **java_wrapperp, jsid id,
                    JavaMemberDescriptor **member_descriptorp,
                    jsval *vp, JSObject **proto_chainp,
                    JSJPropertyInfo *prop_infop)
{
    jsval idval;
    JavaObjectWrapper *java_wrapper;
    JavaMemberDescriptor *member_descriptor;
    const char *member_name;
    JavaClassDescriptor *class_descriptor;
    JSObject *proto_chain;
    JSBool found_in_proto;

    found_in_proto = JS_FALSE;
    member_descriptor = NULL;
    java_wrapper = JS_GetPrivate(cx, obj);

    
    if (!java_wrapper) {
        if (JS_IdToValue(cx, id, &idval) && JSVAL_IS_STRING(idval) &&
            (member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval))) != NULL) {
            if (!strcmp(member_name, "constructor"))
                goto done;
        }
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, JSJMSG_BAD_OP_JOBJECT);
        return JS_FALSE;
    }

    class_descriptor = java_wrapper->class_descriptor;
    JS_ASSERT(IS_REFERENCE_TYPE(class_descriptor->type));

    member_descriptor = jsj_LookupJavaMemberDescriptorById(cx, jEnv, class_descriptor, id);
    if (member_descriptor)
        goto done;

    
    member_descriptor = jsj_LookupJavaStaticMemberDescriptorById(cx, jEnv, class_descriptor, id);
    if (member_descriptor)
        goto done;

    
    JS_IdToValue(cx, id, &idval);
    if (!JSVAL_IS_STRING(idval)) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, JSJMSG_BAD_JOBJECT_EXPR);
        return JS_FALSE;
    }
    member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval));

    










    if (vp) {
        if ((class_descriptor->type == JAVA_SIGNATURE_JAVA_LANG_STRING) &&
            inherit_props_from_JS_natives(cx, "String", member_name, vp))
            goto done;
        if ((class_descriptor->type == JAVA_SIGNATURE_ARRAY) &&
            inherit_props_from_JS_natives(cx, "Array", member_name, vp))
            goto done;
    }

    
    if (!strcmp(member_name, "__proto__")) {
        proto_chain = JS_GetPrototype(cx, obj);
        if (vp)
            *vp = OBJECT_TO_JSVAL(proto_chain);
        goto done;
    }

    





    member_descriptor = jsj_ResolveExplicitMethod(cx, jEnv, class_descriptor, id, JS_FALSE);
    if (member_descriptor)
        goto done;
    member_descriptor = jsj_ResolveExplicitMethod(cx, jEnv, class_descriptor, id, JS_TRUE);
    if (member_descriptor)
        goto done;

    
    if (proto_chainp && prop_infop) {
        
        proto_chain = JS_GetPrototype(cx, obj);

        

        if (proto_chain) {
            if (!OBJ_LOOKUP_PROPERTY(cx, proto_chain, id, proto_chainp,
                                     &prop_infop->prop)) {
                return JS_FALSE;
            }
            if (prop_infop->prop) {
                if (!OBJ_GET_ATTRIBUTES(cx, *proto_chainp, id, prop_infop->prop,
                                        &prop_infop->attributes)) {
                    OBJ_DROP_PROPERTY(cx, *proto_chainp, prop_infop->prop);
                    return JS_FALSE;
                }
                if (!prop_infop->wantProp) {
                    OBJ_DROP_PROPERTY(cx, *proto_chainp, prop_infop->prop);
                    prop_infop->prop = NULL;
                }
                prop_infop->name = member_name;
                found_in_proto = JS_TRUE;
                goto done;
            }
        }
    }

    
    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, JSJMSG_NO_INSTANCE_NAME,
                         class_descriptor->name, member_name);
    return JS_FALSE;

done:
    
    if (java_wrapperp)
        *java_wrapperp = java_wrapper;
    if (member_descriptorp)
        *member_descriptorp = member_descriptor;
    if (proto_chainp && !found_in_proto)
        *proto_chainp = NULL;
    return JS_TRUE;
}

JS_EXPORT_API(JSBool)
JavaObject_getPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    jobject java_obj;
    JavaMemberDescriptor *member_descriptor;
    JavaObjectWrapper *java_wrapper;
    JNIEnv *jEnv;
    JSObject *funobj;
    jsval field_val, method_val;
    JSBool success;
    JSJavaThreadState *jsj_env;
    JSObject *proto_chain;
    JSJPropertyInfo prop_info;

    

    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    if (vp)
        *vp = JSVAL_VOID;
    prop_info.wantProp = JS_FALSE;
    if (!lookup_member_by_id(cx, jEnv, obj, &java_wrapper, id, &member_descriptor, vp,
                             &proto_chain, &prop_info)) {
        jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }

    

    if (!member_descriptor) {
        jsj_ExitJava(jsj_env);
        if (proto_chain)
            return JS_GetProperty(cx, proto_chain, prop_info.name, vp);
        return JS_TRUE;
    }

    java_obj = java_wrapper->java_obj;
    field_val = method_val = JSVAL_VOID;

    if (jaApplet && (*jEnv)->IsInstanceOf(jEnv, java_obj, jaApplet)) {
        jsj_JSIsCallingApplet = JS_TRUE;
    }

    
    if (member_descriptor->field) {
        success = jsj_GetJavaFieldValue(cx, jEnv, member_descriptor->field, java_obj, &field_val);
        if (!success) {
            jsj_ExitJava(jsj_env);
            return JS_FALSE;
        }
    }

    
    if (member_descriptor->methods) {
        

        funobj = JS_CloneFunctionObject(cx, member_descriptor->invoke_func_obj, obj);
        if (!funobj) {
            jsj_ExitJava(jsj_env);
            return JS_FALSE;
        }
        method_val = OBJECT_TO_JSVAL(funobj);
    }

#if TEST_JAVAMEMBER
    
    obj = jsj_CreateJavaMember(cx, method_val, field_val);
    if (!obj) {
        jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }
    *vp = OBJECT_TO_JSVAL(obj);
#else   

    if (member_descriptor->field) {
        if (!member_descriptor->methods) {
            
            *vp = field_val;
        } else {
            



            obj = jsj_CreateJavaMember(cx, method_val, field_val);
            if (!obj) {
                jsj_ExitJava(jsj_env);
                return JS_FALSE;
            }
            *vp = OBJECT_TO_JSVAL(obj);
        }

    } else {
        
        *vp = method_val;
    }

#endif  

    jsj_ExitJava(jsj_env);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_setPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    jobject java_obj;
    const char *member_name;
    JavaObjectWrapper *java_wrapper;
    JavaClassDescriptor *class_descriptor;
    JavaMemberDescriptor *member_descriptor;
    jsval idval;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    JSObject *proto_chain;
    JSJPropertyInfo prop_info;
    JSBool result;

    

    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    prop_info.wantProp = JS_FALSE;
    if (!lookup_member_by_id(cx, jEnv, obj, &java_wrapper, id, &member_descriptor, NULL,
                             &proto_chain, &prop_info)) {
        jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }

    
    if (!member_descriptor) {
        if (proto_chain && (prop_info.attributes & JSPROP_SHARED)) {
            JS_SetProperty(cx, proto_chain, prop_info.name, vp);
        } else {
            JS_IdToValue(cx, id, &idval);
            if (!JSVAL_IS_STRING(idval))
                goto no_such_field;
            member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval));
            if (strcmp(member_name, "__proto__"))
                goto no_such_field;
            if (!JSVAL_IS_OBJECT(*vp)) {
                JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                     JSJMSG_BAD_PROTO_ASSIGNMENT);
                jsj_ExitJava(jsj_env);
                return JS_FALSE;
            }
            JS_SetPrototype(cx, obj, JSVAL_TO_OBJECT(*vp));
        }
        jsj_ExitJava(jsj_env);
        return JS_TRUE;
    }

    

    if (!member_descriptor->field)
        goto no_such_field;

    
    if (member_descriptor->field->modifiers & ACC_FINAL) {
        jsj_ExitJava(jsj_env);
        return JS_TRUE;
    }

    java_obj = java_wrapper->java_obj;

    if (jaApplet && (*jEnv)->IsInstanceOf(jEnv, java_obj, jaApplet)) {
        jsj_JSIsCallingApplet = JS_TRUE;
    }

    result = jsj_SetJavaFieldValue(cx, jEnv, member_descriptor->field, java_obj, *vp);
    jsj_ExitJava(jsj_env);
    return result;

no_such_field:
    JS_IdToValue(cx, id, &idval);
    member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval));
    class_descriptor = java_wrapper->class_descriptor;
    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                         JSJMSG_NO_NAME_IN_CLASS,
                         member_name, class_descriptor->name);
    jsj_ExitJava(jsj_env);
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                         JSObject **objp, JSProperty **propp)
{
    JNIEnv *jEnv;
    JSErrorReporter old_reporter;
    jsval dummy_val;
    JSObject *proto_chain;
    JSJPropertyInfo prop_info;
    JSJavaThreadState *jsj_env;

    

    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    old_reporter = JS_SetErrorReporter(cx, NULL);
    prop_info.wantProp = JS_TRUE;
    if (lookup_member_by_id(cx, jEnv, obj, NULL, id, NULL, &dummy_val,
                            &proto_chain, &prop_info)) {
        
        if (proto_chain) {
            *objp = proto_chain;
            *propp = prop_info.prop;
        } else {
            *objp = obj;
            *propp = (JSProperty*)1;
        }
    } else {
        *objp = NULL;
        *propp = NULL;
    }

    JS_SetErrorReporter(cx, old_reporter);
    jsj_ExitJava(jsj_env);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_defineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                         JSPropertyOp getter, JSPropertyOp setter,
                         uintN attrs, JSProperty **propp)
{
    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                         JSJMSG_JOBJECT_PROP_DEFINE);
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_getAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    
    *attrsp = JSPROP_PERMANENT|JSPROP_ENUMERATE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_setAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    
    if (*attrsp != (JSPROP_PERMANENT|JSPROP_ENUMERATE)) {
        JS_ASSERT(0);
        return JS_FALSE;
    }

    
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_deleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSVersion version = JS_GetVersion(cx);

    *vp = JSVAL_FALSE;

    if (!JSVERSION_IS_ECMA(version)) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                        JSJMSG_JOBJECT_PROP_DELETE);
        return JS_FALSE;
    } else {
        

        return JS_TRUE;
    }
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_defaultValue(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    
    return JavaObject_convert(cx, obj, type, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_newEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                        jsval *statep, jsid *idp)
{
    JavaObjectWrapper *java_wrapper;
    JavaMemberDescriptor *member_descriptor;
    JavaClassDescriptor *class_descriptor;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;

    java_wrapper = JS_GetPrivate(cx, obj);
    
    if (!java_wrapper) {
        *statep = JSVAL_NULL;
        if (idp)
            *idp = INT_TO_JSVAL(0);
        return JS_TRUE;
    }

    class_descriptor = java_wrapper->class_descriptor;

    switch(enum_op) {
    case JSENUMERATE_INIT:

        
        jsj_env = jsj_EnterJava(cx, &jEnv);
        if (!jEnv)
            return JS_FALSE;

        member_descriptor = jsj_GetClassInstanceMembers(cx, jEnv, class_descriptor);
        *statep = PRIVATE_TO_JSVAL(member_descriptor);
        if (idp)
            *idp = INT_TO_JSVAL(class_descriptor->num_instance_members);
        jsj_ExitJava(jsj_env);
        return JS_TRUE;

    case JSENUMERATE_NEXT:
        member_descriptor = JSVAL_TO_PRIVATE(*statep);
        if (member_descriptor) {

            

            while (member_descriptor->methods && member_descriptor->methods->is_alias) {
                member_descriptor = member_descriptor->next;
                if (!member_descriptor) {
                    *statep = JSVAL_NULL;
                    return JS_TRUE;
                }
            }

            *idp = member_descriptor->id;
            *statep = PRIVATE_TO_JSVAL(member_descriptor->next);
            return JS_TRUE;
        }

        

    case JSENUMERATE_DESTROY:
        *statep = JSVAL_NULL;
        return JS_TRUE;

    default:
        JS_ASSERT(0);
        return JS_FALSE;
    }
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaObject_checkAccess(JSContext *cx, JSObject *obj, jsid id,
                      JSAccessMode mode, jsval *vp, uintN *attrsp)
{
    switch (mode) {
    case JSACC_WATCH:
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                             JSJMSG_JOBJECT_PROP_WATCH);
        return JS_FALSE;

    case JSACC_IMPORT:
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                             JSJMSG_JOBJECT_PROP_EXPORT);
        return JS_FALSE;

    default:
        return JS_TRUE;
    }
}

#define JSJ_SLOT_COUNT (JSSLOT_PRIVATE+1)

JSObjectMap * JS_DLL_CALLBACK
jsj_wrapper_newObjectMap(JSContext *cx, jsrefcount nrefs, JSObjectOps *ops,
                         JSClass *clasp, JSObject *obj)
{
    JSObjectMap * map;

    map = (JSObjectMap *) JS_malloc(cx, sizeof(JSObjectMap));
    if (map) {
        map->nrefs = nrefs;
        map->ops = ops;
        map->freeslot = JSJ_SLOT_COUNT;
    }
    return map;
}

void JS_DLL_CALLBACK
jsj_wrapper_destroyObjectMap(JSContext *cx, JSObjectMap *map)
{
    JS_free(cx, map);
}

jsval JS_DLL_CALLBACK
jsj_wrapper_getRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot)
{
    JS_ASSERT(slot < JSJ_SLOT_COUNT);
    JS_ASSERT(obj->map->freeslot == JSJ_SLOT_COUNT);
    return STOBJ_GET_SLOT(obj, slot);
}

JSBool JS_DLL_CALLBACK
jsj_wrapper_setRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot, jsval v)
{
    JS_ASSERT(slot < JSJ_SLOT_COUNT);
    JS_ASSERT(obj->map->freeslot == JSJ_SLOT_COUNT);
    STOBJ_SET_SLOT(obj, slot, v);
    return JS_TRUE;
}

JSObjectOps JavaObject_ops = {
    
    jsj_wrapper_newObjectMap,       
    jsj_wrapper_destroyObjectMap,   
    JavaObject_lookupProperty,
    JavaObject_defineProperty,
    JavaObject_getPropertyById,     
    JavaObject_setPropertyById,     
    JavaObject_getAttributes,
    JavaObject_setAttributes,
    JavaObject_deleteProperty,
    JavaObject_defaultValue,
    JavaObject_newEnumerate,
    JavaObject_checkAccess,

    
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    jsj_wrapper_getRequiredSlot,    
    jsj_wrapper_setRequiredSlot     
};

JS_STATIC_DLL_CALLBACK(JSObjectOps *)
JavaObject_getObjectOps(JSContext *cx, JSClass *clazz)
{
    return &JavaObject_ops;
}

JSClass JavaObject_class = {
    "JavaObject", JSCLASS_HAS_PRIVATE,
    NULL, NULL, NULL, NULL,
    NULL, NULL, JavaObject_convert, JavaObject_finalize,

    
    JavaObject_getObjectOps,
    NULL,                       
    NULL,                       
    NULL,                       
    NULL,                       
    NULL,                       
    NULL,                       
    0,                          
};

extern JS_IMPORT_DATA(JSObjectOps) js_ObjectOps;

JSBool
jsj_init_JavaObject(JSContext *cx, JSObject *global_obj)
{
    return JS_InitClass(cx, global_obj,
                        0, &JavaObject_class, 0, 0,
                        0, 0,
                        0, 0) != 0;
}
