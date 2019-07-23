













































#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"      


#define JS7_ISDEC(c)    (((c) >= '0') && ((c) <= '9'))
#define JS7_UNDEC(c)    ((c) - '0')






static jsval
try_convert_to_jsint(JSContext *cx, jsval idval)
{
    const jschar *cp;
    JSString *jsstr;
    
    jsstr = JS_ValueToString(cx, idval);
    if (!jsstr)
        return idval;

    cp = JS_GetStringChars(jsstr);
    if (JS7_ISDEC(*cp)) {
        jsuint index = JS7_UNDEC(*cp++);
        jsuint oldIndex = 0;
        jsuint c = 0;
        if (index != 0) {
            while (JS7_ISDEC(*cp)) {
                oldIndex = index;
                c = JS7_UNDEC(*cp);
                index = 10*index + c;
                cp++;
            }
        }
        if (*cp == 0 &&
            (oldIndex < (JSVAL_INT_MAX / 10) ||
            (oldIndex == (JSVAL_INT_MAX / 10) && c < (JSVAL_INT_MAX % 10)))) {
            return INT_TO_JSVAL(index);
        }
    }
    return idval;
}


static JSBool
access_java_array_element(JSContext *cx,
                          JNIEnv *jEnv,
                          JSObject *obj,
                          jsid id,
                          jsval *vp,
                          JSBool do_assignment)
{
    jsval idval;
    jarray java_array;
    JavaClassDescriptor *class_descriptor;
    JavaObjectWrapper *java_wrapper;
    jsize array_length, index;
    JavaSignature *array_component_signature;
    
    
    
    java_wrapper = JS_GetPrivate(cx, obj);
    if (!java_wrapper) {
        const char *property_name;
        if (JS_IdToValue(cx, id, &idval) && JSVAL_IS_STRING(idval) &&
            (property_name = JS_GetStringBytes(JSVAL_TO_STRING(idval))) != NULL) {
            if (!strcmp(property_name, "constructor")) {
                if (vp)
                    *vp = JSVAL_VOID;
                return JS_TRUE;
            }
        }
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_BAD_OP_JARRAY);
        return JS_FALSE;
    }
    class_descriptor = java_wrapper->class_descriptor;
    java_array = java_wrapper->java_obj;
    
    JS_ASSERT(class_descriptor->type == JAVA_SIGNATURE_ARRAY);

    JS_IdToValue(cx, id, &idval);

    if (!JSVAL_IS_INT(idval))
        idval = try_convert_to_jsint(cx, idval);

    if (!JSVAL_IS_INT(idval)) {
        




        if (JSVAL_IS_STRING(idval)) {
            const char *member_name;
            
            member_name = JS_GetStringBytes(JSVAL_TO_STRING(idval));
            
            if (do_assignment) {
                JSVersion version = JS_GetVersion(cx);

                if (!JSVERSION_IS_ECMA(version)) {
 
                    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                        JSJMSG_CANT_WRITE_JARRAY, member_name);
                    return JS_FALSE;
                } else {
                    if (vp)
                        *vp = JSVAL_VOID;
                    return JS_TRUE;
                }
            } else {
                if (!strcmp(member_name, "length")) {
                    array_length = jsj_GetJavaArrayLength(cx, jEnv, java_array);
                    if (array_length < 0)
                        return JS_FALSE;
                    if (vp)
                        *vp = INT_TO_JSVAL(array_length);
                    return JS_TRUE;
                }
                
                
                return JavaObject_getPropertyById(cx, obj, id, vp);
            }
        }

        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                            JSJMSG_BAD_INDEX_EXPR);
        return JS_FALSE;
    }
    
    index = JSVAL_TO_INT(idval);

#if 0
    array_length = jsj_GetJavaArrayLength(cx, jEnv, java_array);
    if (array_length < 0)
        return JS_FALSE;

    
    if (index < 0 || index >= array_length) {
        char numBuf[12];
        sprintf(numBuf, "%d", index);
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                            JSJMSG_BAD_JARRAY_INDEX, numBuf);
        return JS_FALSE;
    }
#endif

    array_component_signature = class_descriptor->array_component_signature;

    if (!vp)
        return JS_TRUE;

    if (do_assignment) {
        return jsj_SetJavaArrayElement(cx, jEnv, java_array, index,
                                       array_component_signature, *vp);
    } else {
        return jsj_GetJavaArrayElement(cx, jEnv, java_array, index,
                                       array_component_signature, vp);
    }
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_getPropertyById(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    JSBool result;

    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;
    result = access_java_array_element(cx, jEnv, obj, id, vp, JS_FALSE);
    jsj_ExitJava(jsj_env);
    return result;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_setPropertyById(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    JSBool result;
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;
    result = access_java_array_element(cx, jEnv, obj, id, vp, JS_TRUE);
    jsj_ExitJava(jsj_env);
    return result;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                         JSObject **objp, JSProperty **propp)
{
    JNIEnv *jEnv;
    JSErrorReporter old_reporter;
    JSJavaThreadState *jsj_env;

    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    old_reporter = JS_SetErrorReporter(cx, NULL);
    if (access_java_array_element(cx, jEnv, obj, id, NULL, JS_FALSE)) {
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
JavaArray_defineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                         JSPropertyOp getter, JSPropertyOp setter,
                         uintN attrs, JSProperty **propp)
{
    jsval *vp = &value;
    if (propp)
        return JS_FALSE;
    if (attrs & ~(JSPROP_PERMANENT|JSPROP_ENUMERATE))
        return JS_FALSE;

    return JavaArray_setPropertyById(cx, obj, id, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_getAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    
    *attrsp = JSPROP_PERMANENT|JSPROP_ENUMERATE;
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_setAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    
    if (*attrsp != (JSPROP_PERMANENT|JSPROP_ENUMERATE)) {
        JS_ASSERT(0);
        return JS_FALSE;
    }

    
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_deleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSVersion version = JS_GetVersion(cx);

    *vp = JSVAL_FALSE;
    
    if (!JSVERSION_IS_ECMA(version)) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                            JSJMSG_JARRAY_PROP_DELETE);
        return JS_FALSE;
    } else {
        

        return JS_TRUE;
    }
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_defaultValue(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    
    return JavaObject_convert(cx, obj, JSTYPE_STRING, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_newEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                       jsval *statep, jsid *idp)
{
    JavaObjectWrapper *java_wrapper;
    JSJavaThreadState *jsj_env;
    JNIEnv *jEnv;
    jsize array_length, index;
    JSBool ok = JS_TRUE;

    java_wrapper = JS_GetPrivate(cx, obj);
    
    if (!java_wrapper) {
        *statep = JSVAL_NULL;
        if (idp)
            *idp = INT_TO_JSVAL(0);
        return JS_TRUE;
    }
        
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    array_length = jsj_GetJavaArrayLength(cx, jEnv, java_wrapper->java_obj);
    if (array_length < 0) {
        jsj_ExitJava(jsj_env);
        return JS_FALSE;
    }

    switch(enum_op) {
    case JSENUMERATE_INIT:
        *statep = INT_TO_JSVAL(0);

        if (idp)
            *idp = INT_TO_JSVAL(array_length);
        break;
        
    case JSENUMERATE_NEXT:
        index = JSVAL_TO_INT(*statep);
        if (index < array_length) {
            JS_ValueToId(cx, INT_TO_JSVAL(index), idp);
            index++;
            *statep = INT_TO_JSVAL(index);
            break;
        }

        

    case JSENUMERATE_DESTROY:
        *statep = JSVAL_NULL;
        break;

    default:
        JS_ASSERT(0);
        ok = JS_FALSE;
        break;
    }

    jsj_ExitJava(jsj_env);
    return ok;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaArray_checkAccess(JSContext *cx, JSObject *obj, jsid id,
                      JSAccessMode mode, jsval *vp, uintN *attrsp)
{
    switch (mode) {
    case JSACC_WATCH:
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                            JSJMSG_JARRAY_PROP_WATCH);
        return JS_FALSE;

    case JSACC_IMPORT:
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                            JSJMSG_JARRAY_PROP_EXPORT);
        return JS_FALSE;

    default:
        return JS_TRUE;
    }
}

JSObjectOps JavaArray_ops = {
    
    jsj_wrapper_newObjectMap,       
    jsj_wrapper_destroyObjectMap,   
    JavaArray_lookupProperty,
    JavaArray_defineProperty,
    JavaArray_getPropertyById,      
    JavaArray_setPropertyById,      
    JavaArray_getAttributes,
    JavaArray_setAttributes,
    JavaArray_deleteProperty,
    JavaArray_defaultValue,
    JavaArray_newEnumerate,
    JavaArray_checkAccess,

    
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
JavaArray_getObjectOps(JSContext *cx, JSClass *clazz)
{
    return &JavaArray_ops;
}

JSClass JavaArray_class = {
    "JavaArray", JSCLASS_HAS_PRIVATE,
    NULL, NULL, NULL, NULL,
    NULL, NULL, JavaObject_convert, JavaObject_finalize,

    
    JavaArray_getObjectOps,
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
jsj_init_JavaArray(JSContext *cx, JSObject *global_obj)
{
    if (!JS_InitClass(cx, global_obj, 
        0, &JavaArray_class, 0, 0,
        0, 0, 0, 0))
        return JS_FALSE;
    
    return JS_TRUE;
}

