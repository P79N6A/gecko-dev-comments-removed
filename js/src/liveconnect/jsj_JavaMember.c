




































 






















#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"      


typedef struct JavaMethodOrFieldValue {
    jsval method_val;
    jsval field_val;
} JavaMethodOrFieldValue;

JSObject *
jsj_CreateJavaMember(JSContext *cx, jsval method_val, jsval field_val)
{
    JavaMethodOrFieldValue *member_val;
    JSObject *JavaMember_obj;
    
    member_val = (JavaMethodOrFieldValue *)JS_malloc(cx, sizeof(*member_val));
    if (!member_val)
        return NULL;
    
    JavaMember_obj = JS_NewObject(cx, &JavaMember_class, 0, 0);
    if (!JavaMember_obj) {
        JS_free(cx, member_val);
        return NULL;
    }

    member_val->method_val = method_val;
    member_val->field_val = field_val;
    JS_SetPrivate(cx, JavaMember_obj, (void *)member_val);

    return JavaMember_obj;
}

JS_STATIC_DLL_CALLBACK(void)
JavaMember_finalize(JSContext *cx, JSObject *obj)
{
    JavaMethodOrFieldValue *member_val;

    member_val = JS_GetPrivate(cx, obj);
    if (!member_val)
        return;
    JS_free(cx, member_val);
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaMember_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    JavaMethodOrFieldValue *member_val;
        
    member_val = JS_GetPrivate(cx, obj);
    if (!member_val) {
        if (type == JSTYPE_OBJECT) {
            *vp = OBJECT_TO_JSVAL(obj);
            return JS_TRUE;
        }
        
        JS_ReportError(cx, "illegal operation on JavaObject prototype object");
        return JS_FALSE;
    }

    switch (type) {
    case JSTYPE_VOID:
    case JSTYPE_STRING:
    case JSTYPE_NUMBER:
    case JSTYPE_BOOLEAN:
    case JSTYPE_OBJECT:
        *vp = member_val->field_val;
        return JS_TRUE;

    case JSTYPE_FUNCTION:
        *vp = member_val->method_val;
        return JS_TRUE;

    default:
        JS_ASSERT(0);
        return JS_FALSE;
    }
}






JS_STATIC_DLL_CALLBACK(JSBool)
JavaMember_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_ASSERT(0);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(void)
JavaMember_trace(JSTracer *trc, JSObject *obj)
{
    JavaMethodOrFieldValue *member_val;

    member_val = (JavaMethodOrFieldValue *)JS_GetPrivate(trc->context, obj);
    if (member_val) {
        JS_CALL_VALUE_TRACER(trc, member_val->method_val, "method_val");
        JS_CALL_VALUE_TRACER(trc, member_val->field_val, "field_val");
    }
}

JSClass JavaMember_class = {
    "JavaMember", JSCLASS_HAS_PRIVATE | JSCLASS_MARK_IS_TRACE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, 
    JavaMember_convert, JavaMember_finalize,

    NULL, 
    NULL, 
    JavaMember_Call,
    NULL, 
    NULL, 
    NULL, 
    JS_CLASS_TRACE(JavaMember_trace), 
    0,    
};

JSBool
jsj_init_JavaMember(JSContext *cx, JSObject *global_obj)
{
    if (!JS_InitClass(cx, global_obj, 
        0, &JavaMember_class, 0, 0,
        0, 0,
        0, 0))
        return JS_FALSE;

    return JS_TRUE;
}
