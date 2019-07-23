












































#include <stdlib.h>
#include <string.h>

#ifdef OJI
#include "prtypes.h"          

#endif

#include "jsj_private.h"      


#ifdef IS_LITTLE_ENDIAN
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[1])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[0])
#else
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[0])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[1])
#endif
#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff

#define JSDOUBLE_IS_NaN(x)                                                    \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) == JSDOUBLE_HI32_EXPMASK &&   \
     (JSDOUBLE_LO32(x) || (JSDOUBLE_HI32(x) & JSDOUBLE_HI32_MANTMASK)))

#define JSDOUBLE_IS_INFINITE(x)                                               \
    ((JSDOUBLE_HI32(x) & ~JSDOUBLE_HI32_SIGNBIT) == JSDOUBLE_HI32_EXPMASK &&  \
     !JSDOUBLE_LO32(x))

static JSBool
convert_js_obj_to_JSObject_wrapper(JSContext *cx, JNIEnv *jEnv, JSObject *js_obj,
                                   JavaSignature *signature,
                                   int *cost, jobject *java_value)
{
    if (!njJSObject) {
        if (java_value)
            JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_CANT_LOAD_JSOBJECT);
        return JS_FALSE;
    }

    if (!(*jEnv)->IsAssignableFrom(jEnv, njJSObject, signature->java_class))
        return JS_FALSE;

    if (!java_value)
        return JS_TRUE;

    *java_value = jsj_WrapJSObject(cx, jEnv, js_obj);

    return (*java_value != NULL);   
}




static JSBool
convert_js_array_to_java_array(JSContext *cx, JNIEnv *jEnv, JSObject *js_array,
                               JavaSignature *signature,
                               jobject *java_valuep)
{
    jsuint i;
    jsval js_val;
    jsuint length;
    jclass component_class;
    jarray java_array;
    JavaSignature *array_component_signature;

    if (!JS_GetArrayLength(cx, js_array, &length))
        return JS_FALSE;

    
    array_component_signature = signature->array_component_signature;
    component_class = array_component_signature->java_class;

    
    java_array = (*jEnv)->CallStaticObjectMethod(jEnv, jlrArray, jlrArray_newInstance,
                                                 component_class, length);
    if (!java_array) {
        jsj_ReportJavaError(cx, jEnv, "Error while constructing empty array of %s",
                            jsj_GetJavaClassName(cx, jEnv, component_class));
        return JS_FALSE;
    }

    



    for (i = 0; i < length; i++) {
        if (!JS_LookupElement(cx, js_array, i, &js_val))
            return JS_FALSE;

        if (!jsj_SetJavaArrayElement(cx, jEnv, java_array, i, array_component_signature, js_val))
            return JS_FALSE;
    }

    
    *java_valuep = java_array;
    return JS_TRUE;
}

jstring
jsj_ConvertJSStringToJavaString(JSContext *cx, JNIEnv *jEnv, JSString *js_str)
{
    jstring result;
    result = (*jEnv)->NewString(jEnv, JS_GetStringChars(js_str),
                                JS_GetStringLength(js_str));
    if (!result) {
        jsj_UnexpectedJavaError(cx, jEnv, "Couldn't construct instance "
                                          "of java.lang.String");
    }
    return result;
}









JSBool
jsj_ConvertJSValueToJavaObject(JSContext *cx, JNIEnv *jEnv, jsval v, JavaSignature *signature,
                               int *cost, jobject *java_value, JSBool *is_local_refp)
{
    JSString *jsstr;
    jclass target_java_class;
    
    JS_ASSERT(IS_REFERENCE_TYPE(signature->type));

    


    *is_local_refp = JS_FALSE;

    
    target_java_class = signature->java_class;
    
    if (JSVAL_IS_OBJECT(v)) {
        JSObject *js_obj = JSVAL_TO_OBJECT(v);
        
        
        if (!js_obj) {
            if (java_value)
                *java_value = NULL;
            return JS_TRUE;
        }
        
        if (JS_InstanceOf(cx, js_obj, &JavaObject_class, 0) ||
            JS_InstanceOf(cx, js_obj, &JavaArray_class, 0)) {
            
            



            JavaObjectWrapper *java_wrapper = JS_GetPrivate(cx, js_obj);
            jobject java_obj = java_wrapper->java_obj;
            
            if ((*jEnv)->IsInstanceOf(jEnv, java_obj, target_java_class)) {
                if (java_value)
                    *java_value = java_obj;
                return JS_TRUE;
            }
            
            
            
        } else if (JS_InstanceOf(cx, js_obj, &JavaClass_class, 0)) {
            
            JavaClassDescriptor *java_class_descriptor = JS_GetPrivate(cx, js_obj);
            
            
            if ((*jEnv)->IsAssignableFrom(jEnv, jlClass, target_java_class)) {
                if (java_value)
                    *java_value = java_class_descriptor->java_class;
                return JS_TRUE;
            }
            
            
            if (convert_js_obj_to_JSObject_wrapper(cx, jEnv, js_obj, signature, cost, java_value)) {
                if (java_value && *java_value)
                    *is_local_refp = JS_TRUE;
                return JS_TRUE;
            }
            
            
            
        } else if (JS_InstanceOf(cx, js_obj, &JavaMember_class, 0)) {

            if (!JS_ConvertValue(cx, v, JSTYPE_OBJECT, &v))
                return JS_FALSE;
            return jsj_ConvertJSValueToJavaObject(cx, jEnv, v, signature, cost,
                                                  java_value, is_local_refp);

        
        } else if (JS_IsArrayObject(cx, js_obj) && (signature->type == JAVA_SIGNATURE_ARRAY)) {
            if (convert_js_array_to_java_array(cx, jEnv, js_obj, signature, java_value)) {
                if (java_value && *java_value)
                    *is_local_refp = JS_TRUE;
                return JS_TRUE;
            }
            return JS_FALSE;

        } else {
            



            if (convert_js_obj_to_JSObject_wrapper(cx, jEnv, js_obj, signature, cost, java_value))             {
                if (java_value && *java_value)
                    *is_local_refp = JS_TRUE;
                return JS_TRUE;
            }
            
            
        }
        
    } else if (JSVAL_IS_NUMBER(v)) {
        
        if ((*jEnv)->IsAssignableFrom(jEnv, jlDouble, target_java_class)) {
            if (java_value) {
                jsdouble d;
                if (!JS_ValueToNumber(cx, v, &d))
                    goto conversion_error;
                *java_value = (*jEnv)->NewObject(jEnv, jlDouble, jlDouble_Double, d);
                if (*java_value) {
                    *is_local_refp = JS_TRUE;
                } else {
                    jsj_UnexpectedJavaError(cx, jEnv,
                        "Couldn't construct instance of java.lang.Double");
                    return JS_FALSE;
                }
            }

            return JS_TRUE;
        }
        
        
    } else if (JSVAL_IS_BOOLEAN(v)) {
        
        if ((*jEnv)->IsAssignableFrom(jEnv, jlBoolean, target_java_class)) {
            if (java_value) {
                JSBool b;
                if (!JS_ValueToBoolean(cx, v, &b))
                    goto conversion_error;
                *java_value =
                    (*jEnv)->NewObject(jEnv, jlBoolean, jlBoolean_Boolean, b);
                if (*java_value) {
                    *is_local_refp = JS_TRUE;
                } else {
                    jsj_UnexpectedJavaError(cx, jEnv, "Couldn't construct instance " 
                        "of java.lang.Boolean");
                    return JS_FALSE;
                }
            }

            return JS_TRUE;
        }
        
    }
    
    


    if ((*jEnv)->IsAssignableFrom(jEnv, jlString, target_java_class)) {
        
        
        jsstr = JS_ValueToString(cx, v);
        if (jsstr) {
            if (java_value) {
                *java_value = jsj_ConvertJSStringToJavaString(cx, jEnv, jsstr);
                if (*java_value) {
                    *is_local_refp = JS_TRUE;
                } else {
                    return JS_FALSE;
                }
            }

            return JS_TRUE;
        }
    }
    
conversion_error:
    return JS_FALSE;
}


#define jbyte_MAX_VALUE   127.0
#define jbyte_MIN_VALUE  -128.0
#define jchar_MAX_VALUE   65535.0
#define jchar_MIN_VALUE   0.0
#define jshort_MAX_VALUE  32767.0
#define jshort_MIN_VALUE -32768.0
#define jint_MAX_VALUE    2147483647.0
#define jint_MIN_VALUE   -2147483648.0
#define jlong_MAX_VALUE   9223372036854775807.0
#define jlong_MIN_VALUE  -9223372036854775808.0


#define JSVAL_TO_INTEGRAL_JVALUE(type_name, member_name, member_type, jsval, java_value) \
    if (!JSVAL_IS_NUMBER(v)) {                                           \
        if (!JS_ConvertValue(cx, v, JSTYPE_NUMBER, &v))                  \
            goto conversion_error;                                       \
        (*cost)++;                                                       \
    }                                                                    \
    {                                                                    \
        member_type member_name;                                         \
                                                                         \
        if (JSVAL_IS_INT(v)) {                                           \
            jsint ival = JSVAL_TO_INT(v);                                \
            member_name = (member_type) ival;                            \
                                                                         \
            /* Check to see if the jsval's magnitude is too large to be  \
               representable in the target java type */                  \
            if (member_name != ival)                                     \
                goto numeric_conversion_error;                           \
        } else {                                                         \
            jdouble dval = *JSVAL_TO_DOUBLE(v);                          \
                                                                         \
            /* NaN becomes zero when converted to integral value */      \
            if (JSDOUBLE_IS_NaN(dval))                                   \
                goto numeric_conversion_error;                           \
                                                                         \
            /* Unrepresentably large numbers, including infinities, */   \
            /* cause an error. */                                        \
            else if ((dval >= member_type ## _MAX_VALUE + 1) ||          \
                     (dval <= member_type ## _MIN_VALUE - 1)) {          \
                goto numeric_conversion_error;                           \
            } else                                                       \
                member_name = (member_type) dval;                        \
                                                                         \
            /* Don't allow a non-integral number to be converted         \
               to an integral type */                                    \
            /* Actually, we have to allow this for LC1 compatibility */  \
            /* if ((jdouble)member_name != dval)                         \
                (*cost)++; */                                            \
        }                                                                \
        if (java_value)                                                  \
            java_value->member_name = member_name;                       \
    }

#ifdef XP_OS2



#define JSVAL_TO_JLONG_JVALUE(member_name, member_type, jsvalue, java_value) \
   if (!JSVAL_IS_NUMBER(jsvalue)) {                                      \
      if (!JS_ConvertValue(cx, jsvalue, JSTYPE_NUMBER, &jsvalue))        \
         goto conversion_error;                                          \
      (*cost)++;                                                         \
   }                                                                     \
   {                                                                     \
      member_type member_name;                                           \
                                                                         \
      if (JSVAL_IS_INT(jsvalue)) {                                       \
          jsint ival = JSVAL_TO_INT(jsvalue);                            \
          jlong_I2L(member_name,ival);                                   \
                                                                         \
       } else {                                                          \
            jdouble dval = *JSVAL_TO_DOUBLE(jsvalue);                    \
                                                                         \
            /* NaN becomes zero when converted to integral value */      \
            if (JSDOUBLE_IS_NaN(dval))                                   \
                jlong_I2L(member_name,0);                                \
                                                                         \
            /* Unrepresentably large numbers, including infinities, */   \
            /* cause an error. */                                        \
            else if ((dval > member_type ## _MAX_VALUE) ||               \
                     (dval < member_type ## _MIN_VALUE)) {               \
                goto numeric_conversion_error;                           \
            } else                                                       \
                jlong_D2L(member_name,dval);                             \
                                                                         \
            /* Don't allow a non-integral number to be converted         \
               to an integral type */                                    \
            /* Actually, we have to allow this for LC1 compatibility */  \
            /*if (jlong_to_jdouble(member_name) != dval)                 \
                (*cost)++;*/                                             \
        }                                                                \
        if (java_value)                                                  \
            java_value->member_name = member_name;                       \
    }

static jdouble jlong_to_jdouble(jlong lvalue)
{
   jdouble d;
   jlong_L2D(d,lvalue);
   return d;
}

#else

#define jlong_to_jdouble(lvalue) ((jdouble) lvalue)

#endif












  
JSBool
jsj_ConvertJSValueToJavaValue(JSContext *cx, JNIEnv *jEnv, jsval v_arg,
                              JavaSignature *signature,
                              int *cost, jvalue *java_value, JSBool *is_local_refp)
{
    JavaSignatureChar type;
    jsval v;
    JSBool success = JS_FALSE;

    


    *is_local_refp = JS_FALSE;   
    
    type = signature->type;
    v = v_arg;
    switch (type) {
    case JAVA_SIGNATURE_BOOLEAN:
        if (!JSVAL_IS_BOOLEAN(v)) {
            if (!JS_ConvertValue(cx, v, JSTYPE_BOOLEAN, &v))
                goto conversion_error;
            if (JSVAL_IS_VOID(v))
                goto conversion_error;
            (*cost)++;
        }
        if (java_value)
            java_value->z = (jboolean)(JSVAL_TO_BOOLEAN(v) == JS_TRUE);
        break;

    case JAVA_SIGNATURE_SHORT:
        JSVAL_TO_INTEGRAL_JVALUE(short, s, jshort, v, java_value);
        break;

    case JAVA_SIGNATURE_BYTE:
        JSVAL_TO_INTEGRAL_JVALUE(byte, b, jbyte, v, java_value);
        break;

    case JAVA_SIGNATURE_CHAR:
        
        if (JSVAL_IS_STRING(v) && (JS_GetStringLength(JSVAL_TO_STRING(v)) == 1)) {
            v = INT_TO_JSVAL(*JS_GetStringChars(JSVAL_TO_STRING(v)));
        }
        JSVAL_TO_INTEGRAL_JVALUE(char, c, jchar, v, java_value);
        break;

    case JAVA_SIGNATURE_INT:
        JSVAL_TO_INTEGRAL_JVALUE(int, i, jint, v, java_value);
        break;

    case JAVA_SIGNATURE_LONG:
#if (defined(XP_OS2) && !defined(HAVE_LONG_LONG))
        JSVAL_TO_JLONG_JVALUE(j, jlong, v, java_value);
#else
        JSVAL_TO_INTEGRAL_JVALUE(long, j, jlong, v, java_value);
#endif
        break;
    
    case JAVA_SIGNATURE_FLOAT:
        if (!JSVAL_IS_NUMBER(v)) {
            if (!JS_ConvertValue(cx, v, JSTYPE_NUMBER, &v))
                goto conversion_error;
            (*cost)++;
        }
        if (java_value) {
            if (JSVAL_IS_INT(v))
                java_value->f = (jfloat) JSVAL_TO_INT(v);
            else
                java_value->f = (jfloat) *JSVAL_TO_DOUBLE(v);
        }
        break;

    case JAVA_SIGNATURE_DOUBLE:
        if (!JSVAL_IS_NUMBER(v)) {
            if (!JS_ConvertValue(cx, v, JSTYPE_NUMBER, &v))
                goto conversion_error;
            (*cost)++;
        }
        if (java_value) {
            if (JSVAL_IS_INT(v))
                java_value->d = (jdouble) JSVAL_TO_INT(v);
            else
                java_value->d = (jdouble) *JSVAL_TO_DOUBLE(v);
        }
        break;

    
    default:
        JS_ASSERT(IS_REFERENCE_TYPE(type));
        if (!jsj_ConvertJSValueToJavaObject(cx, jEnv, v, signature, cost,
            &java_value->l, is_local_refp))
            goto conversion_error;
        break;

    case JAVA_SIGNATURE_UNKNOWN:
    case JAVA_SIGNATURE_VOID:
        JS_ASSERT(0);
        return JS_FALSE;
    }

    
    return JS_TRUE;

numeric_conversion_error:
    success = JS_TRUE;
    

conversion_error:

    if (java_value) {
        const char *jsval_string;
        const char *class_name;
        JSString *jsstr;

        jsval_string = NULL;
        jsstr = JS_ValueToString(cx, v_arg);
        if (jsstr)
            jsval_string = JS_GetStringBytes(jsstr);
        if (!jsval_string)
            jsval_string = "";
        
        class_name = jsj_ConvertJavaSignatureToHRString(cx, signature);
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                             JSJMSG_CANT_CONVERT_JS, jsval_string,
                             class_name);

        return JS_FALSE;
    }
    return success;
}





JSString *
jsj_ConvertJavaStringToJSString(JSContext *cx, JNIEnv *jEnv, jstring java_str)
{
    JSString *js_str;
    jboolean is_copy;
    const jchar *ucs2_str;
    jsize ucs2_str_len;

    ucs2_str_len = (*jEnv)->GetStringLength(jEnv, java_str);
    ucs2_str = (*jEnv)->GetStringChars(jEnv, java_str, &is_copy);
    if (!ucs2_str) {
        jsj_UnexpectedJavaError(cx, jEnv,
                                "Unable to extract native Unicode from Java string");
        return NULL;
    }

    

    js_str = JS_NewUCStringCopyN(cx, ucs2_str, ucs2_str_len);

    (*jEnv)->ReleaseStringChars(jEnv, java_str, ucs2_str);
    return js_str;
}








JSBool
jsj_ConvertJavaObjectToJSString(JSContext *cx,
                                JNIEnv *jEnv,
                                JavaClassDescriptor *class_descriptor,
                                jobject java_obj, jsval *vp)
{
    JSString *js_str;
    jstring java_str;
    jmethodID toString;
    jclass java_class;
    
    
    if ((*jEnv)->IsInstanceOf(jEnv, java_obj, jlString)) {

        
        js_str = jsj_ConvertJavaStringToJSString(cx, jEnv, java_obj);
        if (!js_str)
            return JS_FALSE;
        *vp = STRING_TO_JSVAL(js_str);
        return JS_TRUE;
    }
    
    java_class = class_descriptor->java_class;
    toString = (*jEnv)->GetMethodID(jEnv, java_class, "toString",
        "()Ljava/lang/String;");
    if (!toString) {
        
        jsj_UnexpectedJavaError(cx, jEnv, "No toString() method for class %s!",
            class_descriptor->name);
        return JS_FALSE;
    }
    java_str = (*jEnv)->CallObjectMethod(jEnv, java_obj, toString);
    if (!java_str) {
        jsj_ReportJavaError(cx, jEnv, "toString() method failed");
        return JS_FALSE;
    }
    
    
    js_str = jsj_ConvertJavaStringToJSString(cx, jEnv, java_str);
    if (!js_str) {
        (*jEnv)->DeleteLocalRef(jEnv, java_str);
        return JS_FALSE;
    }

    *vp = STRING_TO_JSVAL(js_str);
    (*jEnv)->DeleteLocalRef(jEnv, java_str);
    return JS_TRUE;
}










JSBool
jsj_ConvertJavaObjectToJSNumber(JSContext *cx, JNIEnv *jEnv,
                                JavaClassDescriptor *class_descriptor,
                                jobject java_obj, jsval *vp)
{
    jdouble d;
    jmethodID doubleValue;
    jclass java_class;

    java_class = class_descriptor->java_class;
    doubleValue = (*jEnv)->GetMethodID(jEnv, java_class, "doubleValue", "()D");
    if (!doubleValue) {
        


        (*jEnv)->ExceptionClear(jEnv);
        return jsj_ConvertJavaObjectToJSString(cx, jEnv, class_descriptor,
                                               java_obj, vp);
    }
    




    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        jsj_UnexpectedJavaError(cx, jEnv, "No doubleValue() method for class %s!",
            class_descriptor->name);
        return JS_FALSE;
    }
    
    d = (*jEnv)->CallDoubleMethod(jEnv, java_obj, doubleValue);
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        jsj_UnexpectedJavaError(cx, jEnv, "doubleValue() method failed");
        return JS_FALSE;
    }
    return JS_NewDoubleValue(cx, d, vp);
}










extern JSBool
jsj_ConvertJavaObjectToJSBoolean(JSContext *cx, JNIEnv *jEnv,
                                 JavaClassDescriptor *class_descriptor,
                                 jobject java_obj, jsval *vp)
{
    jboolean b;
    jmethodID booleanValue;
    jclass java_class;
    
    
    if (!java_obj) {
        *vp = JSVAL_FALSE;
        return JS_TRUE;
    }
    java_class = class_descriptor->java_class;
    booleanValue = (*jEnv)->GetMethodID(jEnv, java_class, "booleanValue", "()Z");

    

    if (!booleanValue) {
        (*jEnv)->ExceptionClear(jEnv);
        *vp = JSVAL_TRUE;
        return JS_TRUE;
    }

    b = (*jEnv)->CallBooleanMethod(jEnv, java_obj, booleanValue);
    if ((*jEnv)->ExceptionOccurred(jEnv)) {
        jsj_UnexpectedJavaError(cx, jEnv, "booleanValue() method failed");
        return JS_FALSE;
    }
    *vp = BOOLEAN_TO_JSVAL(b);
    return JS_TRUE;
}




static JSBool
convert_javaobject_to_jsobject(JSContext *cx, JNIEnv *jEnv,
                               JavaClassDescriptor *class_descriptor,
                               jobject java_obj, jsval *vp)
{
    JSObject *js_obj;

    




    if (njJSObject && (*jEnv)->IsInstanceOf(jEnv, java_obj, njJSObject)) {
#ifdef PRESERVE_JSOBJECT_IDENTITY
#if JS_BYTES_PER_LONG == 8
        js_obj = (JSObject *)((*jEnv)->GetLongField(jEnv, java_obj, njJSObject_long_internal));
#else
        js_obj = (JSObject *)((*jEnv)->GetIntField(jEnv, java_obj, njJSObject_internal));
#endif
#else
        js_obj = jsj_UnwrapJSObjectWrapper(jEnv, java_obj);
#endif
    } else {
        
        js_obj = jsj_WrapJavaObject(cx, jEnv, java_obj, class_descriptor->java_class);
        if (!js_obj)
            return JS_FALSE;
    }

    *vp = OBJECT_TO_JSVAL(js_obj);
    return JS_TRUE;
}





JSBool
jsj_ConvertJavaObjectToJSValue(JSContext *cx, JNIEnv *jEnv,
                               jobject java_obj, jsval *vp)
{
    JavaClassDescriptor *class_descriptor;
    jclass java_class;
    JSBool ret;

    
    if (!java_obj) {
        *vp = JSVAL_NULL;
        return JS_TRUE;
    }

    java_class = (*jEnv)->GetObjectClass(jEnv, java_obj);

    class_descriptor = jsj_GetJavaClassDescriptor(cx, jEnv, java_class);
    if (!class_descriptor)
        return JS_FALSE;

    switch (class_descriptor->type) {
    case JAVA_SIGNATURE_JAVA_LANG_BOOLEAN:
        ret = jsj_ConvertJavaObjectToJSBoolean(cx, jEnv, class_descriptor, java_obj, vp);
        break;
    case JAVA_SIGNATURE_JAVA_LANG_DOUBLE:
        ret = jsj_ConvertJavaObjectToJSNumber(cx, jEnv, class_descriptor, java_obj, vp);
        break;
    case JAVA_SIGNATURE_JAVA_LANG_STRING:
        ret = jsj_ConvertJavaObjectToJSString(cx, jEnv, class_descriptor, java_obj, vp);
        break;
    default:
        ret = convert_javaobject_to_jsobject(cx, jEnv, class_descriptor, java_obj, vp);
        break;
    }

    (*jEnv)->DeleteLocalRef(jEnv, java_class);
    jsj_ReleaseJavaClassDescriptor(cx, jEnv, class_descriptor);
    return ret;
}







JSBool
jsj_ConvertJavaValueToJSValue(JSContext *cx, JNIEnv *jEnv,
                              JavaSignature *signature,
                              jvalue *java_value,
                              jsval *vp)
{
    int32 ival32;

    switch (signature->type) {
    case JAVA_SIGNATURE_VOID:
        *vp = JSVAL_VOID;
        return JS_TRUE;

    case JAVA_SIGNATURE_BYTE:
        *vp = INT_TO_JSVAL((jsint)java_value->b);
        return JS_TRUE;

    case JAVA_SIGNATURE_CHAR:
        *vp = INT_TO_JSVAL((jsint)java_value->c);
        return JS_TRUE;

    case JAVA_SIGNATURE_SHORT:
        *vp = INT_TO_JSVAL((jsint)java_value->s);
        return JS_TRUE;

    case JAVA_SIGNATURE_INT:
        ival32 = java_value->i;
        if (INT_FITS_IN_JSVAL(ival32)) {
            *vp = INT_TO_JSVAL((jsint) ival32);
            return JS_TRUE;
        } else {
            return JS_NewDoubleValue(cx, ival32, vp);
        }

    case JAVA_SIGNATURE_BOOLEAN:
        *vp = BOOLEAN_TO_JSVAL((JSBool) java_value->z);
        return JS_TRUE;

    case JAVA_SIGNATURE_LONG:
        return JS_NewDoubleValue(cx, jlong_to_jdouble(java_value->j), vp);
  
    case JAVA_SIGNATURE_FLOAT:
        return JS_NewDoubleValue(cx, java_value->f, vp);

    case JAVA_SIGNATURE_DOUBLE:
        return JS_NewDoubleValue(cx, java_value->d, vp);

    case JAVA_SIGNATURE_UNKNOWN:
        JS_ASSERT(0);
        return JS_FALSE;
        
    
    default:
        JS_ASSERT(IS_REFERENCE_TYPE(signature->type));
        return jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_value->l, vp);

    }
}

