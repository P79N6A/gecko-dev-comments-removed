





































 








#include <stdlib.h>

#include "jsj_private.h"      







static JSBool
add_java_field_to_class_descriptor(JSContext *cx,
                                   JNIEnv *jEnv,
                                   JavaClassDescriptor *class_descriptor, 
                                   jstring field_name_jstr,
                                   jobject java_field,  
                                   jint modifiers)
{
    jclass fieldType;
    jfieldID fieldID;
    jclass java_class;

    JSBool is_static_field;
    JavaMemberDescriptor *member_descriptor = NULL;
    const char *sig_cstr = NULL;
    const char *field_name = NULL;
    JavaSignature *signature = NULL;
    JavaFieldSpec *field_spec = NULL;
        
    is_static_field = modifiers & ACC_STATIC;
    if (is_static_field) {
        member_descriptor = jsj_GetJavaStaticMemberDescriptor(cx, jEnv, class_descriptor, field_name_jstr);
    } else {
        member_descriptor = jsj_GetJavaMemberDescriptor(cx, jEnv, class_descriptor, field_name_jstr);
    }
    if (!member_descriptor)
        goto error;
    
    field_spec = (JavaFieldSpec*)JS_malloc(cx, sizeof(JavaFieldSpec));
    if (!field_spec)
        goto error;

    field_spec->modifiers = modifiers;

    
    fieldType = (*jEnv)->CallObjectMethod(jEnv, java_field, jlrField_getType);
    if (!fieldType) {
        jsj_UnexpectedJavaError(cx, jEnv,
                                "Unable to determine type of field using"
                                " java.lang.reflect.Field.getType()");
        goto error;
    }
    
    signature = jsj_GetJavaClassDescriptor(cx, jEnv, fieldType);
    (*jEnv)->DeleteLocalRef(jEnv, fieldType);
    if (!signature)
        goto error;
    field_spec->signature = signature;

    field_name = jsj_DupJavaStringUTF(cx, jEnv, field_name_jstr);
    if (!field_name)
        goto error;
    field_spec->name = field_name;

    
    sig_cstr = jsj_ConvertJavaSignatureToString(cx, signature);
    if (!sig_cstr)
        goto error;

    
    java_class = class_descriptor->java_class;
    if (is_static_field)
        fieldID = (*jEnv)->GetStaticFieldID(jEnv, java_class, field_name, sig_cstr);
    else
        fieldID = (*jEnv)->GetFieldID(jEnv, java_class, field_name, sig_cstr);
    if (!fieldID) {
       jsj_UnexpectedJavaError(cx, jEnv,
                           "Can't get Java field ID for class %s, field %s (sig=%s)",
                           class_descriptor->name, field_name, sig_cstr);
       goto error;
    }
    field_spec->fieldID = fieldID;
    
    JS_free(cx, (char*)sig_cstr);
    
    member_descriptor->field = field_spec;

    
    return JS_TRUE;

error:
    if (field_spec) {
        JS_FREE_IF(cx, (char*)field_spec->name);
        JS_free(cx, field_spec);
    }
    JS_FREE_IF(cx, (char*)sig_cstr);
    if (signature)
        jsj_ReleaseJavaClassDescriptor(cx, jEnv, signature);
    return JS_FALSE;
}




void
jsj_DestroyFieldSpec(JSContext *cx, JNIEnv *jEnv, JavaFieldSpec *field)
{
    JS_FREE_IF(cx, (char*)field->name);
    jsj_ReleaseJavaClassDescriptor(cx, jEnv, field->signature);
    JS_free(cx, field);
}











JSBool 
jsj_ReflectJavaFields(JSContext *cx, JNIEnv *jEnv, JavaClassDescriptor *class_descriptor,
                      JSBool reflect_only_static_fields)
{
    int i;
    JSBool ok;
    jint modifiers;
    jobject java_field;
    jstring field_name_jstr;
    jarray joFieldArray;
    jsize num_fields;
    jclass java_class;

    

    java_class = class_descriptor->java_class;
    joFieldArray = (*jEnv)->CallObjectMethod(jEnv, java_class, jlClass_getFields);
    if (!joFieldArray) {
        jsj_UnexpectedJavaError(cx, jEnv,
                                "Can't determine Java object's fields "
                                "using java.lang.Class.getFields()");
        return JS_FALSE;
    }

    
    num_fields = (*jEnv)->GetArrayLength(jEnv, joFieldArray);
    for (i = 0; i < num_fields; i++) {
       
        
        java_field = (*jEnv)->GetObjectArrayElement(jEnv, joFieldArray, i);
        if (!java_field) {
            jsj_UnexpectedJavaError(cx, jEnv, "Can't access a Field[] array");
            return JS_FALSE;
        }

        
        modifiers = (*jEnv)->CallIntMethod(jEnv, java_field, jlrField_getModifiers);
        if ((*jEnv)->ExceptionOccurred(jEnv)) {
            jsj_UnexpectedJavaError(cx, jEnv,
                                    "Can't access a Field's modifiers using"
                                    "java.lang.reflect.Field.getModifiers()");
            return JS_FALSE;
        }

        
        if (!(modifiers & ACC_PUBLIC))
            goto no_reflect;

        
        if (reflect_only_static_fields != ((modifiers & ACC_STATIC) != 0))
            goto no_reflect;

        
        field_name_jstr = (*jEnv)->CallObjectMethod(jEnv, java_field, jlrField_getName);
        if (!field_name_jstr) {
            jsj_UnexpectedJavaError(cx, jEnv,
                                    "Can't obtain a Field's name"
                                    "java.lang.reflect.Field.getName()");
            return JS_FALSE;
        }
        
        
        ok = add_java_field_to_class_descriptor(cx, jEnv, class_descriptor, field_name_jstr,
                                                java_field, modifiers);
        if (!ok)
            return JS_FALSE;

        (*jEnv)->DeleteLocalRef(jEnv, field_name_jstr);
        field_name_jstr = NULL;

no_reflect:
        (*jEnv)->DeleteLocalRef(jEnv, java_field);
        java_field = NULL;
    }

    (*jEnv)->DeleteLocalRef(jEnv, joFieldArray);

    
    return JS_TRUE;
}








JSBool
jsj_GetJavaFieldValue(JSContext *cx, JNIEnv *jEnv, JavaFieldSpec *field_spec,
                      jobject java_obj, jsval *vp)
{
    JSBool is_static_field, success;
    jvalue java_value;
    JavaSignature *signature;
    JavaSignatureChar field_type;
    jfieldID fieldID = field_spec->fieldID;

    is_static_field = field_spec->modifiers & ACC_STATIC;

#define GET_JAVA_FIELD(Type,member)                                          \
    JS_BEGIN_MACRO                                                           \
    if (is_static_field)                                                     \
        java_value.member =                                                  \
            (*jEnv)->GetStatic##Type##Field(jEnv, (*jEnv)->GetObjectClass(jEnv, java_obj), fieldID);        \
    else                                                                     \
        java_value.member =                                                  \
            (*jEnv)->Get##Type##Field(jEnv, java_obj, fieldID);              \
    if ((*jEnv)->ExceptionOccurred(jEnv)) {                                  \
        jsj_UnexpectedJavaError(cx, jEnv, "Error reading Java field");           \
        return JS_FALSE;                                                     \
    }                                                                        \
    JS_END_MACRO

    signature = field_spec->signature;
    field_type = signature->type;
    switch(field_type) {
    case JAVA_SIGNATURE_BYTE:
        GET_JAVA_FIELD(Byte,b);
        break;

    case JAVA_SIGNATURE_CHAR:
        GET_JAVA_FIELD(Char,c);
        break;

    case JAVA_SIGNATURE_SHORT:
        GET_JAVA_FIELD(Short,s);
        break;

    case JAVA_SIGNATURE_INT:
        GET_JAVA_FIELD(Int,i);
        break;

    case JAVA_SIGNATURE_BOOLEAN:
        GET_JAVA_FIELD(Boolean,z);
        break;

    case JAVA_SIGNATURE_LONG:
        GET_JAVA_FIELD(Long,j);
        break;
  
    case JAVA_SIGNATURE_FLOAT:
        GET_JAVA_FIELD(Float,f);
        break;

    case JAVA_SIGNATURE_DOUBLE:
        GET_JAVA_FIELD(Double,d);
        break;
     
    case JAVA_SIGNATURE_UNKNOWN:
    case JAVA_SIGNATURE_VOID:
        JS_ASSERT(0);        
        return JS_FALSE;
        
    
    default:
        JS_ASSERT(IS_REFERENCE_TYPE(field_type));
        GET_JAVA_FIELD(Object,l);
        success = jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_value.l, vp);
        (*jEnv)->DeleteLocalRef(jEnv, java_value.l);
        return success;
    }
    
#undef GET_JAVA_FIELD

    return jsj_ConvertJavaValueToJSValue(cx, jEnv, signature, &java_value, vp);
}

JSBool
jsj_SetJavaFieldValue(JSContext *cx, JNIEnv *jEnv, JavaFieldSpec *field_spec,
                      jclass java_obj, jsval js_val)
{
    JSBool is_static_field, is_local_ref;
    int dummy_cost;
    jvalue java_value;
    JavaSignature *signature;
    JavaSignatureChar field_type;
    jfieldID fieldID = field_spec->fieldID;

    is_static_field = field_spec->modifiers & ACC_STATIC;

#define SET_JAVA_FIELD(Type,member)                                          \
    JS_BEGIN_MACRO                                                           \
    if (is_static_field) {                                                   \
        (*jEnv)->SetStatic##Type##Field(jEnv, java_obj, fieldID,             \
                                        java_value.member);                  \
    } else {                                                                 \
        (*jEnv)->Set##Type##Field(jEnv, java_obj, fieldID,java_value.member);\
    }                                                                        \
    if ((*jEnv)->ExceptionOccurred(jEnv)) {                                  \
        jsj_UnexpectedJavaError(cx, jEnv, "Error assigning to Java field");      \
        return JS_FALSE;                                                     \
    }                                                                        \
    JS_END_MACRO

    signature = field_spec->signature;
    if (!jsj_ConvertJSValueToJavaValue(cx, jEnv, js_val, signature, &dummy_cost,
                                       &java_value, &is_local_ref))
        return JS_FALSE;

    field_type = signature->type;
    switch(field_type) {
    case JAVA_SIGNATURE_BYTE:
        SET_JAVA_FIELD(Byte,b);
        break;

    case JAVA_SIGNATURE_CHAR:
        SET_JAVA_FIELD(Char,c);
        break;

    case JAVA_SIGNATURE_SHORT:
        SET_JAVA_FIELD(Short,s);
        break;

    case JAVA_SIGNATURE_INT:
        SET_JAVA_FIELD(Int,i);
        break;

    case JAVA_SIGNATURE_BOOLEAN:
        SET_JAVA_FIELD(Boolean,z);
        break;

    case JAVA_SIGNATURE_LONG:
        SET_JAVA_FIELD(Long,j);
        break;
  
    case JAVA_SIGNATURE_FLOAT:
        SET_JAVA_FIELD(Float,f);
        break;

    case JAVA_SIGNATURE_DOUBLE:
        SET_JAVA_FIELD(Double,d);
        break;
        
    
    default:
        JS_ASSERT(IS_REFERENCE_TYPE(field_type));
        SET_JAVA_FIELD(Object,l);
        if (is_local_ref)
            (*jEnv)->DeleteLocalRef(jEnv, java_value.l);
        break;

    case JAVA_SIGNATURE_UNKNOWN:
    case JAVA_SIGNATURE_VOID:
        JS_ASSERT(0);        
        return JS_FALSE;
    }

#undef SET_JAVA_FIELD
    
    return JS_TRUE;
}
