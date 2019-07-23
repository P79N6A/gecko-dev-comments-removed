






















































#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"        
#include "jscntxt.h"            

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    char *name;
    JSString *str;

    JavaClassDescriptor *class_descriptor;
    
    class_descriptor = JS_GetPrivate(cx, obj);
    if (!class_descriptor)
        return JS_FALSE;

    switch(type) {


    case JSTYPE_STRING:
        
        if (!class_descriptor->name)
            break;
        name = JS_smprintf("[JavaClass %s]", class_descriptor->name);
        if (!name) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }

        str = JS_NewString(cx, name, strlen(name));
        if (!str) {
            JS_smprintf_free(name);
            

            return JS_FALSE;
        }

        *vp = STRING_TO_JSVAL(str);
        return JS_TRUE;

    default:
      break;
    }
    return JS_TRUE;
}

static JSBool
lookup_static_member_by_id(JSContext *cx, JNIEnv *jEnv, JSObject *obj,
                           JavaClassDescriptor **class_descriptorp,
                           jsid id, JavaMemberDescriptor **memberp)
{
    jsval idval;
    JavaMemberDescriptor *member_descriptor;
    const char *member_name;
    JavaClassDescriptor *class_descriptor;

    class_descriptor = JS_GetPrivate(cx, obj);
    if (!class_descriptor) {
        *class_descriptorp = NULL;
        *memberp = NULL;
        return JS_TRUE;
    }
    
    if (class_descriptorp)
        *class_descriptorp = class_descriptor;
    
    member_descriptor = jsj_LookupJavaStaticMemberDescriptorById(cx, jEnv, class_descriptor, id);
    if (!member_descriptor) {
        JS_IdToValue(cx, id, &idval);
        if (!JSVAL_IS_STRING(idval)) {
            JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                            JSJMSG_BAD_JCLASS_EXPR);
            return JS_FALSE;
        }

        member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval));
        
        



        member_descriptor =
            jsj_ResolveExplicitMethod(cx, jEnv, class_descriptor, id, JS_TRUE);
        if (member_descriptor)
            goto done;

        
        if (!strcmp(member_name, "prototype")) {
            *memberp = NULL;
            return JS_TRUE;
        }

        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                        JSJMSG_MISSING_NAME,
                       class_descriptor->name, member_name);
        return JS_FALSE;
    }

done:
    if (memberp)
        *memberp = member_descriptor;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_getPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    jsval idval;
    jclass java_class;
    const char *member_name;
    JavaClassDescriptor *class_descriptor;
    JavaMemberDescriptor *member_descriptor;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    JSBool result;

    
    
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    if (!lookup_static_member_by_id(cx, jEnv, obj, &class_descriptor, id, &member_descriptor)) {
	jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }
    if (!member_descriptor) {
        *vp = JSVAL_VOID;
	jsj_ExitJava(jsj_env);
        return JS_TRUE;
    }

    java_class = class_descriptor->java_class;

    if (member_descriptor->field) {
        if (!member_descriptor->methods) {
            result = jsj_GetJavaFieldValue(cx, jEnv, member_descriptor->field, java_class, vp);
	    jsj_ExitJava(jsj_env);
	    return result;
        } else {
            JS_ASSERT(0);
        }
    } else {
        JSFunction *function;
        
        
        if (member_descriptor->methods->is_alias) {
            


            JS_IdToValue(cx, id, &idval);
            member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval));
        } else {
            


            member_name = member_descriptor->name;
        }
        function = JS_NewFunction(cx, jsj_JavaStaticMethodWrapper, 0,
                                  JSFUN_BOUND_METHOD, obj, member_name);
        if (!function) {
	    jsj_ExitJava(jsj_env);
            return JS_FALSE;
	}

        *vp = OBJECT_TO_JSVAL(JS_GetFunctionObject(function));
    }

    jsj_ExitJava(jsj_env);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_setPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    jclass java_class;
    const char *member_name;
    JavaClassDescriptor *class_descriptor;
    JavaMemberDescriptor *member_descriptor;
    jsval idval;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    JSBool result;

    

    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;
    
    if (!lookup_static_member_by_id(cx, jEnv, obj, &class_descriptor, id, &member_descriptor)) {
	jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }

    

    if (!member_descriptor->field)
        goto no_such_field;

    
    if (member_descriptor->field->modifiers & ACC_FINAL) {
	jsj_ExitJava(jsj_env);
        return JS_TRUE;
    }

    java_class = class_descriptor->java_class;
    result = jsj_SetJavaFieldValue(cx, jEnv, member_descriptor->field, java_class, *vp);
    jsj_ExitJava(jsj_env);
    return result;

no_such_field:
    JS_IdToValue(cx, id, &idval);
    member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval));
    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                   JSJMSG_MISSING_STATIC,
                   member_name, class_descriptor->name);
    jsj_ExitJava(jsj_env);
    return JS_FALSE;
}




JS_STATIC_DLL_CALLBACK(void)
JavaClass_finalize(JSContext *cx, JSObject *obj)
{
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;

    JavaClassDescriptor *class_descriptor = JS_GetPrivate(cx, obj);
    if (!class_descriptor)
        return;
    
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return;

    
    jsj_ReleaseJavaClassDescriptor(cx, jEnv, class_descriptor);
    jsj_ExitJava(jsj_env);
}


JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                         JSObject **objp, JSProperty **propp)
{
    JNIEnv *jEnv;
    JSErrorReporter old_reporter;
    JSJavaThreadState *jsj_env;

    
    
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    old_reporter = JS_SetErrorReporter(cx, NULL);
    if (lookup_static_member_by_id(cx, jEnv, obj, NULL, id, NULL)) {
        *objp = obj;
        *propp = (JSProperty*)1;
    } else {
        *objp = NULL;
        *propp = NULL;
    }

    JS_SetErrorReporter(cx, old_reporter);
    jsj_ExitJava(jsj_env);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_defineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                         JSPropertyOp getter, JSPropertyOp setter,
                         uintN attrs, JSProperty **propp)
{
    JavaClassDescriptor *class_descriptor;
    
    class_descriptor = JS_GetPrivate(cx, obj);

    
    if (!class_descriptor)
	return JS_TRUE;

    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                         JSJMSG_JCLASS_PROP_DEFINE);
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_getAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    
    *attrsp = JSPROP_PERMANENT|JSPROP_ENUMERATE;
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_setAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    
    if (*attrsp != (JSPROP_PERMANENT|JSPROP_ENUMERATE)) {
        JS_ASSERT(0);
        return JS_FALSE;
    }

    
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_deleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSVersion version = JS_GetVersion(cx);
    
    *vp = JSVAL_FALSE;

    if (!JSVERSION_IS_ECMA(version)) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                            JSJMSG_JCLASS_PROP_DELETE);
        return JS_FALSE;
    } else {
        

        return JS_TRUE;
    }
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_defaultValue(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    
    return JavaClass_convert(cx, obj, JSTYPE_STRING, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_newEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                       jsval *statep, jsid *idp)
{
    JavaMemberDescriptor *member_descriptor;
    JavaClassDescriptor *class_descriptor;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    
    class_descriptor = JS_GetPrivate(cx, obj);

    
    if (!class_descriptor) {
        *statep = JSVAL_NULL;
        if (idp)
            *idp = INT_TO_JSVAL(0);
        return JS_TRUE;
    }
    
    switch(enum_op) {
    case JSENUMERATE_INIT:
        
        jsj_env = jsj_EnterJava(cx, &jEnv);
        if (!jEnv)
            return JS_FALSE;
        member_descriptor = jsj_GetClassStaticMembers(cx, jEnv, class_descriptor);
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
JavaClass_checkAccess(JSContext *cx, JSObject *obj, jsid id,
                      JSAccessMode mode, jsval *vp, uintN *attrsp)
{
    switch (mode) {
    case JSACC_WATCH:
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                            JSJMSG_JCLASS_PROP_WATCH);
        return JS_FALSE;

    case JSACC_IMPORT:
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                            JSJMSG_JCLASS_PROP_EXPORT);
        return JS_FALSE;

    default:
        return JS_TRUE;
    }
}





JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_hasInstance(JSContext *cx, JSObject *obj, jsval candidate_jsval,
                      JSBool *has_instancep)
{
    JavaClassDescriptor *class_descriptor;
    JavaObjectWrapper *java_wrapper;
    JSClass *js_class;
    JSBool has_instance;
    JSObject *candidate_obj;
    jclass java_class;
    jobject java_obj;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    
    has_instance = JS_FALSE;
    class_descriptor = JS_GetPrivate(cx, obj);
    if (!class_descriptor) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                    JSJMSG_BAD_OP_JCLASS);
        return JS_FALSE;
    }

    



    if (!JSVAL_IS_OBJECT(candidate_jsval))
        goto done;
    candidate_obj = JSVAL_TO_OBJECT(candidate_jsval);
#ifdef JS_THREADSAFE
    js_class = JS_GetClass(cx, candidate_obj);
#else
    js_class = JS_GetClass(candidate_obj);
#endif
    if ((js_class != &JavaObject_class) && (js_class != &JavaArray_class))
        goto done;

    java_class = class_descriptor->java_class;
    java_wrapper = JS_GetPrivate(cx, candidate_obj);
    if (!java_wrapper) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_BAD_OP_PROTO);
        return JS_FALSE;
    }
    java_obj = java_wrapper->java_obj;
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    has_instance = (*jEnv)->IsInstanceOf(jEnv, java_obj, java_class);
    jsj_ExitJava(jsj_env);

done:
    *has_instancep = has_instance;
    return JS_TRUE;
}

JSObjectOps JavaClass_ops = {
    
    jsj_wrapper_newObjectMap,       
    jsj_wrapper_destroyObjectMap,   
    JavaClass_lookupProperty,
    JavaClass_defineProperty,
    JavaClass_getPropertyById,      
    JavaClass_setPropertyById,      
    JavaClass_getAttributes,
    JavaClass_setAttributes,
    JavaClass_deleteProperty,
    JavaClass_defaultValue,
    JavaClass_newEnumerate,
    JavaClass_checkAccess,

    
    NULL,                           
    NULL,                           
    jsj_JavaConstructorWrapper,     
    jsj_JavaConstructorWrapper,     
    NULL,                           
    JavaClass_hasInstance,          
    NULL,                           
    NULL,                           
    NULL,                           
    NULL,                           
    jsj_wrapper_getRequiredSlot,    
    jsj_wrapper_setRequiredSlot     
};

JS_STATIC_DLL_CALLBACK(JSObjectOps *)
JavaClass_getObjectOps(JSContext *cx, JSClass *clazz)
{
    return &JavaClass_ops;
}

JSClass JavaClass_class = {
    "JavaClass", JSCLASS_HAS_PRIVATE,
    NULL, NULL, NULL, NULL,
    NULL, NULL, JavaClass_convert, JavaClass_finalize,

    
    JavaClass_getObjectOps,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
};

static JSObject *
jsj_new_JavaClass(JSContext *cx, JNIEnv *jEnv, JSObject* parent_obj,
                  JavaClassDescriptor *class_descriptor)
{
    JSObject *JavaClass_obj;

    JavaClass_obj = JS_NewObject(cx, &JavaClass_class, 0, parent_obj);
    if (!JavaClass_obj)
        return NULL;

    JS_SetPrivate(cx, JavaClass_obj, (void *)class_descriptor);

#ifdef DEBUG
    
#endif

    return JavaClass_obj;
}

JSObject *
jsj_define_JavaClass(JSContext *cx, JNIEnv *jEnv, JSObject* parent_obj,
                     const char *simple_class_name,
                     jclass java_class)
{
    JavaClassDescriptor *class_descriptor;
    JSObject *JavaClass_obj;

    class_descriptor = jsj_GetJavaClassDescriptor(cx, jEnv, java_class);
    if (!class_descriptor)
        return NULL;

    JavaClass_obj = jsj_new_JavaClass(cx, jEnv, parent_obj, class_descriptor);
    if (!JavaClass_obj)
        return NULL;

    if (!JS_DefineProperty(cx, parent_obj, simple_class_name,
                           OBJECT_TO_JSVAL(JavaClass_obj), 0, 0,
                           JSPROP_PERMANENT|JSPROP_READONLY|JSPROP_ENUMERATE))
        return NULL;
    return JavaClass_obj;
}










JS_STATIC_DLL_CALLBACK(JSBool)
getClass(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *obj_arg, *JavaClass_obj;
    JavaObjectWrapper *java_wrapper;
    JavaClassDescriptor *class_descriptor;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;

    if (argc != 1 ||
    !JSVAL_IS_OBJECT(argv[0]) ||
    !(obj_arg = JSVAL_TO_OBJECT(argv[0])) ||
    (!JS_InstanceOf(cx, obj_arg, &JavaObject_class, 0) &&
         !JS_InstanceOf(cx, obj_arg, &JavaArray_class, 0))) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_NEED_JOBJECT_ARG);
        return JS_FALSE;
    }

    java_wrapper = JS_GetPrivate(cx, obj_arg);
    if (!java_wrapper) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_PROTO_GETCLASS);
        return JS_FALSE;
    }

    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;
    
    class_descriptor = java_wrapper->class_descriptor;

    JavaClass_obj = jsj_new_JavaClass(cx, jEnv, NULL, class_descriptor);
    if (!JavaClass_obj) {
	jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }

    *rval = OBJECT_TO_JSVAL(JavaClass_obj);
    jsj_ExitJava(jsj_env);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaClass_construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *obj_arg, *JavaClass_obj;
    JavaObjectWrapper *java_wrapper;
    JavaClassDescriptor *class_descriptor;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;

    if (argc != 1 ||
	!JSVAL_IS_OBJECT(argv[0]) ||
	!(obj_arg = JSVAL_TO_OBJECT(argv[0])) ||
	!JS_InstanceOf(cx, obj_arg, &JavaObject_class, 0) ||
	((java_wrapper = JS_GetPrivate(cx, obj_arg)) == NULL)) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                             JSJMSG_NEED_JCLASS_ARG);
        return JS_FALSE;
    }

    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    class_descriptor = java_wrapper->class_descriptor;
    if (!(*jEnv)->IsSameObject(jEnv, class_descriptor->java_class, jlClass)) {
	JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                             JSJMSG_NEED_JCLASS_ARG);
	jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }

    class_descriptor = jsj_GetJavaClassDescriptor(cx, jEnv, java_wrapper->java_obj);
    JavaClass_obj = jsj_new_JavaClass(cx, jEnv, NULL, class_descriptor);
    if (!JavaClass_obj) {
	jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }

    *rval = OBJECT_TO_JSVAL(JavaClass_obj);
    jsj_ExitJava(jsj_env);
    return JS_TRUE;
}

extern JS_IMPORT_DATA(JSObjectOps) js_ObjectOps;

JSBool
jsj_init_JavaClass(JSContext *cx, JSObject *global_obj)
{
    
    if (!JS_InitClass(cx, global_obj, 0, &JavaClass_class, JavaClass_construct, 0, 0, 0, 0, 0))
        return JS_FALSE;

    if (!JS_DefineFunction(cx, global_obj, "getClass", getClass, 0,
                           JSPROP_READONLY))
        return JS_FALSE;

    return jsj_InitJavaClassReflectionsTable();
}

