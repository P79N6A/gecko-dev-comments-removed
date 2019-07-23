





































 






#include "jsj_private.h"      








JSBool
jsj_GetJavaArrayElement(JSContext *cx, JNIEnv *jEnv, jarray java_array, jsize index,
                        JavaSignature *array_component_signature,
                        jsval *vp)
{
    jvalue java_value;
    JavaSignatureChar component_type;
    JSBool success;

#define GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Type,member)                   \
    (*jEnv)->Get##Type##ArrayRegion(jEnv, java_array, index, 1,              \
                                    &java_value.member);                     \
    if ((*jEnv)->ExceptionOccurred(jEnv)) {                                  \
        jsj_ReportJavaError(cx, jEnv, "Error reading element of "            \
                                      "Java primitive array");               \
        return JS_FALSE;                                                     \
    }

    component_type = array_component_signature->type;
    switch(component_type) {
    case JAVA_SIGNATURE_BYTE:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Byte,b);
        break;

    case JAVA_SIGNATURE_CHAR:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Char,c);
        break;

    case JAVA_SIGNATURE_SHORT:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Short,s);
        break;

    case JAVA_SIGNATURE_INT:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Int,i);
        break;

    case JAVA_SIGNATURE_BOOLEAN:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Boolean,z);
        break;

    case JAVA_SIGNATURE_LONG:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Long,j);
        break;
  
    case JAVA_SIGNATURE_FLOAT:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Float,f);
        break;

    case JAVA_SIGNATURE_DOUBLE:
        GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Double,d);
        break;

    
    default:
        JS_ASSERT(component_type >= JAVA_SIGNATURE_ARRAY);
        java_value.l = (*jEnv)->GetObjectArrayElement(jEnv, java_array, index);
        if ((*jEnv)->ExceptionOccurred(jEnv)) {
            jsj_ReportJavaError(cx, jEnv, "Error reading Java object array");
            return JS_FALSE;
        }
        success = jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_value.l, vp);
        (*jEnv)->DeleteLocalRef(jEnv, java_value.l);
        return success;

#undef GET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY
    case JAVA_SIGNATURE_UNKNOWN:
    case JAVA_SIGNATURE_VOID:
        JS_ASSERT(0);        
        return JS_FALSE;
    }

    return jsj_ConvertJavaValueToJSValue(cx, jEnv, array_component_signature, &java_value, vp);
}

JSBool
jsj_SetJavaArrayElement(JSContext *cx, JNIEnv *jEnv, jarray java_array, jsize index,
                        JavaSignature *array_component_signature,
                        jsval js_val)
{
    int dummy_cost;
    jvalue java_value;
    JavaSignatureChar component_type;
    JSBool is_local_ref;

    if (!jsj_ConvertJSValueToJavaValue(cx, jEnv, js_val, array_component_signature,
                                       &dummy_cost, &java_value, &is_local_ref))
        return JS_FALSE;

#define SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Type,member)                   \
    (*jEnv)->Set##Type##ArrayRegion(jEnv, java_array, index, 1,              \
                                    &java_value.member);                     \
    if ((*jEnv)->ExceptionOccurred(jEnv)) {                                  \
        jsj_ReportJavaError(cx, jEnv, "Error assigning to element of "       \
                                      "Java primitive array");               \
        return JS_FALSE;                                                     \
    }

    component_type = array_component_signature->type;
    switch(component_type) {
    case JAVA_SIGNATURE_BYTE:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Byte,b);
        break;

    case JAVA_SIGNATURE_CHAR:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Char,c);
        break;

    case JAVA_SIGNATURE_SHORT:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Short,s);
        break;

    case JAVA_SIGNATURE_INT:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Int,i);
        break;

    case JAVA_SIGNATURE_BOOLEAN:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Boolean,z);
        break;

    case JAVA_SIGNATURE_LONG:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Long,j);
        break;
  
    case JAVA_SIGNATURE_FLOAT:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Float,f);
        break;

    case JAVA_SIGNATURE_DOUBLE:
        SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY(Double,d);
        break;

    
    default:
        JS_ASSERT(IS_REFERENCE_TYPE(component_type));
        (*jEnv)->SetObjectArrayElement(jEnv, java_array, index, java_value.l);
        if (is_local_ref)                                                           \
            (*jEnv)->DeleteLocalRef(jEnv, java_value.l);
        if ((*jEnv)->ExceptionOccurred(jEnv)) {
            jsj_ReportJavaError(cx, jEnv, "Error assigning to Java object array");
            return JS_FALSE;
        }
        break;

#undef SET_ELEMENT_FROM_PRIMITIVE_JAVA_ARRAY
    case JAVA_SIGNATURE_UNKNOWN:
    case JAVA_SIGNATURE_VOID:
        JS_ASSERT(0);        
        return JS_FALSE;
    }

    return JS_TRUE;
}

