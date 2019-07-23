






















































#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"        
#include "jsjava.h"             
#include "jsclist.h"            


typedef JSCList MethodList;


typedef struct MethodListElement {
    JSCList linkage;
    JavaMethodSpec *method;
} MethodListElement;






typedef enum JSJTypePreference {
    JSJPREF_FIRST_ARG  = 1,       
    JSJPREF_SECOND_ARG = 2,       
    JSJPREF_AMBIGUOUS  = 3        
} JSJTypePreference;





typedef enum JSJType {
    JSJTYPE_VOID,                
    JSJTYPE_BOOLEAN,             
    JSJTYPE_NUMBER,              
    JSJTYPE_STRING,              
    JSJTYPE_NULL,                
    JSJTYPE_JAVACLASS,           
    JSJTYPE_JAVAOBJECT,          
    JSJTYPE_JAVAARRAY,		 
    JSJTYPE_JSARRAY,             
    JSJTYPE_OBJECT,              
    JSJTYPE_LIMIT
} JSJType;








static const char *
convert_java_method_arg_signatures_to_string(JSContext *cx,
                                             JavaSignature **arg_signatures,
                                             int num_args)
{
    const char *first_arg_signature, *rest_arg_signatures, *sig;
    JavaSignature **rest_args;

    
    first_arg_signature = jsj_ConvertJavaSignatureToString(cx, arg_signatures[0]);
    if (!first_arg_signature)
        return NULL;

    
    if (num_args == 1)
        return first_arg_signature;

    
    rest_args = &arg_signatures[1];
    rest_arg_signatures =
        convert_java_method_arg_signatures_to_string(cx, rest_args, num_args - 1);
    if (!rest_arg_signatures) {
        JS_smprintf_free((char*)first_arg_signature);
        return NULL;
    }

    

    sig = JS_smprintf("%s%s", first_arg_signature, rest_arg_signatures);
    JS_smprintf_free((char*)first_arg_signature);
    JS_smprintf_free((char*)rest_arg_signatures);
    if (!sig) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }

    return sig;
}








static const char *
convert_java_method_arg_signatures_to_hr_string(JSContext *cx,
                                                JavaSignature **arg_signatures,
                                                int num_args,
						JSBool whitespace)
{
    const char *first_arg_signature, *rest_arg_signatures, *sig, *separator;
    JavaSignature **rest_args;

    if (num_args == 0)
        return JS_smprintf("%s", "");

    
    first_arg_signature = jsj_ConvertJavaSignatureToHRString(cx, arg_signatures[0]);
    if (!first_arg_signature)
        return NULL;

    
    if (num_args == 1)
        return first_arg_signature;

    
    rest_args = &arg_signatures[1];
    rest_arg_signatures =
        convert_java_method_arg_signatures_to_hr_string(cx, rest_args, num_args - 1, whitespace);
    if (!rest_arg_signatures) {
        JS_smprintf_free((char*)first_arg_signature);
        return NULL;
    }

    

    separator = whitespace ? " " : "";
    sig = JS_smprintf("%s,%s%s", first_arg_signature, separator, rest_arg_signatures);
    JS_smprintf_free((char*)first_arg_signature);
    JS_smprintf_free((char*)rest_arg_signatures);
    if (!sig) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }

    return sig;
}








const char *
jsj_ConvertJavaMethodSignatureToHRString(JSContext *cx,
                                         const char *method_name,
                                         JavaMethodSignature *method_signature)
{
    JavaSignature **arg_signatures, *return_val_signature;
    const char *arg_sigs_cstr;
    const char *return_val_sig_cstr;
    const char *sig_cstr;

    arg_signatures = method_signature->arg_signatures;
    return_val_signature = method_signature->return_val_signature;

    
    arg_sigs_cstr =
            convert_java_method_arg_signatures_to_hr_string(cx, arg_signatures,
                                                            method_signature->num_args,
							    JS_TRUE);
    if (!arg_sigs_cstr)
        return NULL;

    
    return_val_sig_cstr = jsj_ConvertJavaSignatureToHRString(cx, return_val_signature);
    if (!return_val_sig_cstr) {
        JS_smprintf_free((char*)arg_sigs_cstr);
        return NULL;
    }

    
    sig_cstr = JS_smprintf("%s %s(%s)", return_val_sig_cstr, method_name, arg_sigs_cstr);
    JS_smprintf_free((char*)arg_sigs_cstr);
    JS_smprintf_free((char*)return_val_sig_cstr);

    if (!sig_cstr) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    return sig_cstr;
}




void
jsj_PurgeJavaMethodSignature(JSContext *cx, JNIEnv *jEnv, JavaMethodSignature *method_signature)
{
    int i, num_args;
    JavaSignature **arg_signatures;

    if (!method_signature)  
        return;

    
    num_args = method_signature->num_args;
    arg_signatures = method_signature->arg_signatures;
    for (i = 0; i < num_args; i++)
        jsj_ReleaseJavaClassDescriptor(cx, jEnv, arg_signatures[i]);
    if (arg_signatures)
        JS_free(cx, arg_signatures);

    
    if (method_signature->return_val_signature)
        jsj_ReleaseJavaClassDescriptor(cx, jEnv, method_signature->return_val_signature);
}









JavaMethodSignature *
jsj_InitJavaMethodSignature(JSContext *cx, JNIEnv *jEnv,
                           jobject method,
                           JavaMethodSignature *method_signature)
{
    int i;
    jboolean is_constructor;
    jclass return_val_class;
    jsize num_args;
    JavaSignature *return_val_signature;
    jarray arg_classes;
    jmethodID getParameterTypes;

    memset(method_signature, 0, sizeof (JavaMethodSignature));
    
    is_constructor = (*jEnv)->IsInstanceOf(jEnv, method, jlrConstructor);

    
    if  (is_constructor)
        getParameterTypes = jlrConstructor_getParameterTypes;
    else
        getParameterTypes = jlrMethod_getParameterTypes;
    arg_classes = (*jEnv)->CallObjectMethod(jEnv, method, getParameterTypes);
    if (!arg_classes) {
        jsj_UnexpectedJavaError(cx, jEnv,
                                "Can't determine argument signature of method");
        goto error;
    }

    
    num_args = jsj_GetJavaArrayLength(cx, jEnv, arg_classes);
    if (num_args < 0)
        goto error;
    method_signature->num_args = num_args;

    
    if (num_args) {
        JavaSignature **arg_signatures;

        
        size_t arg_signatures_size = num_args * sizeof(JavaSignature *);
        arg_signatures = (JavaSignature **)JS_malloc(cx, arg_signatures_size);
        if (!arg_signatures)
            goto error;
        memset(arg_signatures, 0, arg_signatures_size);
        method_signature->arg_signatures = arg_signatures;
        
        

        for (i = 0; i < num_args; i++) {
            JavaSignature *a;
            jclass arg_class = (*jEnv)->GetObjectArrayElement(jEnv, arg_classes, i);
            
            a = arg_signatures[i] = jsj_GetJavaClassDescriptor(cx, jEnv, arg_class);
            (*jEnv)->DeleteLocalRef(jEnv, arg_class);
            if (!a) {
                jsj_UnexpectedJavaError(cx, jEnv, "Could not determine Java class "
                                                  "signature using java.lang.reflect");
                goto error;
            }
        }
    }

    
    if (is_constructor) {
        
        return_val_signature = jsj_GetJavaClassDescriptor(cx, jEnv, jlVoid_TYPE);
    } else {
        return_val_class =
            (*jEnv)->CallObjectMethod(jEnv, method, jlrMethod_getReturnType);
        if (!return_val_class) {
            jsj_UnexpectedJavaError(cx, jEnv,
                                    "Can't determine return type of method "
                                    "using java.lang.reflect.Method.getReturnType()");
            goto error;
        }

        
        return_val_signature = jsj_GetJavaClassDescriptor(cx, jEnv, return_val_class);
        (*jEnv)->DeleteLocalRef(jEnv, return_val_class);
    }

    if (!return_val_signature)
        goto error;
    method_signature->return_val_signature = return_val_signature;

    (*jEnv)->DeleteLocalRef(jEnv, arg_classes);
    return method_signature;

error:

    if (arg_classes)
        (*jEnv)->DeleteLocalRef(jEnv, arg_classes);
    jsj_PurgeJavaMethodSignature(cx, jEnv, method_signature);
    return NULL;
}







const char *
jsj_ConvertJavaMethodSignatureToString(JSContext *cx,
                                       JavaMethodSignature *method_signature)
{
    JavaSignature **arg_signatures, *return_val_signature;
    const char *arg_sigs_cstr;
    const char *return_val_sig_cstr;
    const char *sig_cstr;

    arg_signatures = method_signature->arg_signatures;
    return_val_signature = method_signature->return_val_signature;

    
    arg_sigs_cstr = NULL;
    if (arg_signatures) {
        arg_sigs_cstr =
            convert_java_method_arg_signatures_to_string(cx, arg_signatures,
                                                         method_signature->num_args);
        if (!arg_sigs_cstr)
            return NULL;
    }

    
    return_val_sig_cstr = jsj_ConvertJavaSignatureToString(cx, return_val_signature);
    if (!return_val_sig_cstr) {
        JS_smprintf_free((char*)arg_sigs_cstr);
        return NULL;
    }

    
    if (arg_sigs_cstr) {
        sig_cstr = JS_smprintf("(%s)%s", arg_sigs_cstr, return_val_sig_cstr);
        JS_smprintf_free((char*)arg_sigs_cstr);
    } else {
        sig_cstr = JS_smprintf("()%s", return_val_sig_cstr);
    }

    JS_smprintf_free((char*)return_val_sig_cstr);

    if (!sig_cstr) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    return sig_cstr;
}

static JSBool
add_java_method_to_class_descriptor(JSContext *cx, JNIEnv *jEnv,
                                    JavaClassDescriptor *class_descriptor, 
                                    jstring method_name_jstr,
                                    jobject java_method,
                                    JSBool is_static_method,
                                    JSBool is_constructor)
{
    jmethodID methodID;
    JSFunction *fun;
    jclass java_class = class_descriptor->java_class;

    JavaMemberDescriptor *member_descriptor = NULL;
    const char *sig_cstr = NULL;
    const char *method_name = NULL;
    JavaMethodSignature *signature = NULL;
    JavaMethodSpec **specp, *method_spec = NULL;
            
    if (is_constructor) {
        member_descriptor = jsj_GetJavaClassConstructors(cx, class_descriptor);
    } else {
        if (is_static_method) {
            member_descriptor = jsj_GetJavaStaticMemberDescriptor(cx, jEnv, class_descriptor, method_name_jstr);
        } else {
            member_descriptor = jsj_GetJavaMemberDescriptor(cx, jEnv, class_descriptor, method_name_jstr);
	}
        fun = JS_NewFunction(cx, jsj_JavaInstanceMethodWrapper, 0,
	                     JSFUN_BOUND_METHOD, NULL, member_descriptor->name);
	member_descriptor->invoke_func_obj = JS_GetFunctionObject(fun);
        JS_AddNamedRoot(cx, &member_descriptor->invoke_func_obj,
                        "&member_descriptor->invoke_func_obj");
    }
    if (!member_descriptor)
        return JS_FALSE;
    
    method_spec = (JavaMethodSpec*)JS_malloc(cx, sizeof(JavaMethodSpec));
    if (!method_spec)
        goto error;
    memset(method_spec, 0, sizeof(JavaMethodSpec));

    signature = jsj_InitJavaMethodSignature(cx, jEnv, java_method, &method_spec->signature);
    if (!signature)
        goto error;

    method_name = JS_strdup(cx, member_descriptor->name);
    if (!method_name)
        goto error;
    method_spec->name = method_name;
    
    sig_cstr = jsj_ConvertJavaMethodSignatureToString(cx, signature);
    if (!sig_cstr)
        goto error;

    if (is_static_method)
        methodID = (*jEnv)->GetStaticMethodID(jEnv, java_class, method_name, sig_cstr);
    else
        methodID = (*jEnv)->GetMethodID(jEnv, java_class, method_name, sig_cstr);
    method_spec->methodID = methodID;
    
    if (!methodID) {
        jsj_UnexpectedJavaError(cx, jEnv,
                                "Can't get Java method ID for %s.%s() (sig=%s)",
                                class_descriptor->name, method_name, sig_cstr);
        goto error;
    }
    
    JS_free(cx, (char*)sig_cstr);

     
    specp = &member_descriptor->methods;
    while (*specp) {
        specp = &(*specp)->next;
    }
    *specp = method_spec;

    return JS_TRUE;

error:
    if (method_spec)
        JS_FREE_IF(cx, (char*)method_spec->name);
    JS_FREE_IF(cx, (char*)sig_cstr);
    if (signature)
        jsj_PurgeJavaMethodSignature(cx, jEnv, signature);
    JS_FREE_IF(cx, method_spec);
    return JS_FALSE;
}

JSBool 
jsj_ReflectJavaMethods(JSContext *cx, JNIEnv *jEnv,
                       JavaClassDescriptor *class_descriptor,
                       JSBool reflect_only_static_methods)
{
    jarray joMethodArray, joConstructorArray;
    jsize num_methods, num_constructors;
    int i;
    jclass java_class;
    JSBool ok, reflect_only_instance_methods;

    reflect_only_instance_methods = !reflect_only_static_methods;

    

    java_class = class_descriptor->java_class;
    joMethodArray = (*jEnv)->CallObjectMethod(jEnv, java_class, jlClass_getMethods);
    if (!joMethodArray) {
        jsj_UnexpectedJavaError(cx, jEnv,
                                "Can't determine Java object's methods "
                                "using java.lang.Class.getMethods()");
        return JS_FALSE;
    }

    
    num_methods = (*jEnv)->GetArrayLength(jEnv, joMethodArray);
    for (i = 0; i < num_methods; i++) {
        jstring method_name_jstr;
        
        
        jobject java_method = (*jEnv)->GetObjectArrayElement(jEnv, joMethodArray, i);

        
        jint modifiers = (*jEnv)->CallIntMethod(jEnv, java_method, jlrMethod_getModifiers);

        
        if (!(modifiers & ACC_PUBLIC))
            goto dont_reflect_method;

        
        if (modifiers & ACC_ABSTRACT)
            goto dont_reflect_method;

        
        if (reflect_only_static_methods != ((modifiers & ACC_STATIC) != 0))
            goto dont_reflect_method;
        
        
        method_name_jstr = (*jEnv)->CallObjectMethod(jEnv, java_method, jlrMethod_getName);
        ok = add_java_method_to_class_descriptor(cx, jEnv, class_descriptor, method_name_jstr, java_method,
                                                 reflect_only_static_methods, JS_FALSE);
        (*jEnv)->DeleteLocalRef(jEnv, method_name_jstr);
        if (!ok) {
            (*jEnv)->DeleteLocalRef(jEnv, java_method);
            (*jEnv)->DeleteLocalRef(jEnv, joMethodArray);
            return JS_FALSE;
        }

dont_reflect_method:
        (*jEnv)->DeleteLocalRef(jEnv, java_method);
    }

    (*jEnv)->DeleteLocalRef(jEnv, joMethodArray);
    if (reflect_only_instance_methods)
        return JS_TRUE;
        
    joConstructorArray = (*jEnv)->CallObjectMethod(jEnv, java_class, jlClass_getConstructors);
    if (!joConstructorArray) {
        jsj_UnexpectedJavaError(cx, jEnv, "internal error: "
                                "Can't determine Java class's constructors "
                                "using java.lang.Class.getConstructors()");
        return JS_FALSE;
    }

    
    num_constructors = (*jEnv)->GetArrayLength(jEnv, joConstructorArray);
    for (i = 0; i < num_constructors; i++) {
        
        jobject java_constructor =
            (*jEnv)->GetObjectArrayElement(jEnv, joConstructorArray, i);

        
        jint modifiers = (*jEnv)->CallIntMethod(jEnv, java_constructor,
                                                jlrConstructor_getModifiers);

        
        if (!(modifiers & ACC_PUBLIC))
            goto dont_reflect_constructor;
        
        
        ok = add_java_method_to_class_descriptor(cx, jEnv, class_descriptor, NULL,
                                                 java_constructor,
                                                 JS_FALSE, JS_TRUE);
        if (!ok) {
            (*jEnv)->DeleteLocalRef(jEnv, joConstructorArray);
            (*jEnv)->DeleteLocalRef(jEnv, java_constructor);
            return JS_FALSE;
        }

dont_reflect_constructor:
        (*jEnv)->DeleteLocalRef(jEnv, java_constructor);
    }

    (*jEnv)->DeleteLocalRef(jEnv, joConstructorArray);
    return JS_TRUE;
}




void
jsj_DestroyMethodSpec(JSContext *cx, JNIEnv *jEnv, JavaMethodSpec *method_spec)
{
    if (!method_spec->is_alias) {
	JS_FREE_IF(cx, (char*)method_spec->name);
	jsj_PurgeJavaMethodSignature(cx, jEnv, &method_spec->signature);
    }
    JS_free(cx, method_spec);
}

static JavaMethodSpec *
copy_java_method_descriptor(JSContext *cx, JavaMethodSpec *method)
{
    JavaMethodSpec *copy;

    copy = (JavaMethodSpec*)JS_malloc(cx, sizeof(JavaMethodSpec));
    if (!copy)
        return NULL;
    memcpy(copy, method, sizeof(JavaMethodSpec));
    copy->next = NULL;
    copy->is_alias = JS_TRUE;
    return copy;
}








JavaMemberDescriptor *
jsj_ResolveExplicitMethod(JSContext *cx, JNIEnv *jEnv,
			  JavaClassDescriptor *class_descriptor, 
			  jsid method_name_id,
			  JSBool is_static)
{
    JavaMethodSpec *method;
    JavaMemberDescriptor *member_descriptor;
    JavaMethodSignature *ms;
    JSString *simple_name_jsstr;
    JSFunction *fun;
    JSBool is_constructor;
    int left_paren;
    const char *sig_cstr, *method_name;
    char *arg_start;
    jsid id;
    jsval method_name_jsval;
      
    



    JS_IdToValue(cx, method_name_id, &method_name_jsval);
    method_name = JS_GetStringBytes(JSVAL_TO_STRING(method_name_jsval));
    arg_start = strchr(method_name, '(');	
    
    if (!arg_start)
	return NULL;
    
    is_constructor = (is_static && (arg_start == method_name));
        
    left_paren = arg_start - method_name;
    simple_name_jsstr = JS_NewStringCopyN(cx, method_name, left_paren);
    if (!simple_name_jsstr)
	return NULL;

    
    JS_ValueToId(cx, STRING_TO_JSVAL(simple_name_jsstr), &id);
    if (is_constructor)
        member_descriptor = jsj_LookupJavaClassConstructors(cx, jEnv, class_descriptor);
    else if (is_static)
	member_descriptor = jsj_LookupJavaStaticMemberDescriptorById(cx, jEnv, class_descriptor, id);
    else
	member_descriptor = jsj_LookupJavaMemberDescriptorById(cx, jEnv, class_descriptor, id);
    if (!member_descriptor)	
	return NULL;
    
    



    if (!strlen(arg_start + 1))
	return NULL;
    arg_start = JS_strdup(cx, arg_start + 1);
    if (!arg_start)
	return NULL;
    arg_start[strlen(arg_start) - 1] = '\0';	
    sig_cstr = NULL;	
    for (method = member_descriptor->methods; method; method = method->next) {
	ms = &method->signature;
	sig_cstr = convert_java_method_arg_signatures_to_hr_string(cx, ms->arg_signatures,
								   ms->num_args, JS_FALSE);
	if (!sig_cstr) {
	    JS_free(cx, arg_start);
	    return NULL;
	}

	if (!strcmp(sig_cstr, arg_start))
	    break;
	JS_smprintf_free((char*)sig_cstr);
    }
    JS_free(cx, arg_start);
    if (!method)
	return NULL;
    JS_smprintf_free((char*)sig_cstr);
    
    
    if (!member_descriptor->methods->next)
	return member_descriptor;

    



    member_descriptor = JS_malloc(cx, sizeof(JavaMemberDescriptor));
    if (!member_descriptor)
        return NULL;
    memset(member_descriptor, 0, sizeof(JavaMemberDescriptor));

    member_descriptor->id = method_name_id;
    member_descriptor->name =
        JS_strdup(cx, is_constructor ? "<init>" : JS_GetStringBytes(simple_name_jsstr));
    if (!member_descriptor->name) {
        JS_free(cx, member_descriptor);
        return NULL;
    }

    member_descriptor->methods = copy_java_method_descriptor(cx, method);
    if (!member_descriptor->methods) {
	JS_free(cx, (void*)member_descriptor->name);
        JS_free(cx, member_descriptor);
        return NULL;
    }
 
    fun = JS_NewFunction(cx, jsj_JavaInstanceMethodWrapper, 0,
			 JSFUN_BOUND_METHOD, NULL, method_name);
    member_descriptor->invoke_func_obj = JS_GetFunctionObject(fun);
    JS_AddNamedRoot(cx, &member_descriptor->invoke_func_obj,
                    "&member_descriptor->invoke_func_obj");

    
    
    if (is_static) {
	member_descriptor->next = class_descriptor->static_members;
	class_descriptor->static_members = member_descriptor;
    } else {
	member_descriptor->next = class_descriptor->instance_members;
	class_descriptor->instance_members = member_descriptor;
    }

    return member_descriptor;
}








static const char *
get_js_arg_types_as_string(JSContext *cx, uintN argc, jsval *argv)
{
    uintN i;
    const char *arg_type, *arg_string, *tmp;

    if (argc == 0)
        return JS_smprintf("%s", "()");
    
    arg_string = strdup("(");
    if (!arg_string)
        goto out_of_memory;
    for (i = 0; i < argc; i++) {
        arg_type = JS_GetTypeName(cx, JS_TypeOfValue(cx, argv[i]));
        tmp = JS_smprintf("%s%s%s%s", arg_string,  i ? ", " : "", arg_type,
                         (i == argc-1) ? ")" : "");
        free((char*)arg_string);
        if (!tmp)
            goto out_of_memory;
        arg_string = tmp;
    }

    return arg_string;

out_of_memory:
    JS_ReportOutOfMemory(cx);
    return NULL;
}






static void
report_method_match_failure(JSContext *cx,
                            JavaMemberDescriptor *member_descriptor,
                            JavaClassDescriptor *class_descriptor,
                            JSBool is_static_method,
                            uintN argc, jsval *argv)
{
    const char *err, *js_arg_string, *tmp, *method_str, *method_name;
    JSBool is_constructor;
    JavaMethodSpec *method;

    err = NULL;
    is_constructor = (!strcmp(member_descriptor->name, "<init>"));

    js_arg_string = get_js_arg_types_as_string(cx, argc, argv);
    if (!js_arg_string)
        goto out_of_memory;

    if (is_constructor) {
        err =  JS_smprintf("There is no Java constructor for class %s that matches "
                           "JavaScript argument types %s.\n", class_descriptor->name,
                           js_arg_string);
        method_name = class_descriptor->name;
    } else {
        err =  JS_smprintf("There is no %sJava method %s.%s that matches "
                           "JavaScript argument types %s.\n",
                           is_static_method ? "static ": "",
                           class_descriptor->name, member_descriptor->name, js_arg_string);
        method_name = member_descriptor->name;
    }
    if (!err)
        goto out_of_memory;

    tmp = JS_smprintf("%sCandidate methods with the same name are:\n", err);
    if (!tmp)
        goto out_of_memory;
    err = tmp;

    method = member_descriptor->methods;
    while (method) {
        method_str =
            jsj_ConvertJavaMethodSignatureToHRString(cx, method_name, &method->signature);
        if (!method_str)
            goto out_of_memory;
        tmp = JS_smprintf("%s   %s\n", err, method_str);
        JS_smprintf_free((char*)method_str);
        if (!tmp)
            goto out_of_memory;
        err = tmp;

        method = method->next;
    }
    
    JS_ReportError(cx, err);
    return;

out_of_memory:
    if (js_arg_string)
        JS_smprintf_free((char*)js_arg_string);
    if (err)
        JS_smprintf_free((char*)err);
}







static void
report_ambiguous_method_match(JSContext *cx,
                              JavaMemberDescriptor *member_descriptor,
                              JavaClassDescriptor *class_descriptor,
                              MethodList *ambiguous_methods,
                              JSBool is_static_method,
                              uintN argc, jsval *argv)
{
    const char *err, *js_arg_string, *tmp, *method_str, *method_name;
    JSBool is_constructor;
    JavaMethodSpec *method;
    MethodListElement *method_list_element;

    err = NULL;
    is_constructor = (!strcmp(member_descriptor->name, "<init>"));

    js_arg_string = get_js_arg_types_as_string(cx, argc, argv);
    if (!js_arg_string)
        goto out_of_memory;

    if (is_constructor) {
        err =  JS_smprintf("The choice of Java constructor for class %s with "
                           "JavaScript argument types %s is ambiguous.\n",
                           class_descriptor->name,
                           js_arg_string);
        method_name = class_descriptor->name;
    } else {
        err =  JS_smprintf("The choice of %sJava method %s.%s matching "
                           "JavaScript argument types %s is ambiguous.\n",
                           is_static_method ? "static ": "",
                           class_descriptor->name, member_descriptor->name,
                           js_arg_string);
        method_name = member_descriptor->name;
    }
    if (!err)
        goto out_of_memory;

    tmp = JS_smprintf("%sCandidate methods are:\n", err);
    if (!tmp)
        goto out_of_memory;
    err = tmp;

    method_list_element = (MethodListElement*)JS_LIST_HEAD(ambiguous_methods);
    while ((MethodList*)method_list_element != ambiguous_methods) {
        method = method_list_element->method;
        method_str =
            jsj_ConvertJavaMethodSignatureToHRString(cx, method_name, &method->signature);
        if (!method_str)
            goto out_of_memory;
        tmp = JS_smprintf("%s   %s\n", err, method_str);
        JS_smprintf_free((char*)method_str);
        if (!tmp)
            goto out_of_memory;
        err = tmp;

        method_list_element = (MethodListElement*)method_list_element->linkage.next;
    }
    
    JS_ReportError(cx, err);
    return;

out_of_memory:
    if (js_arg_string)
        JS_smprintf_free((char*)js_arg_string);
    if (err)
        JS_smprintf_free((char*)err);
}





static JSJType
compute_jsj_type(JSContext *cx, jsval v)
{
    JSObject *js_obj;

    if (JSVAL_IS_OBJECT(v)) {
        if (JSVAL_IS_NULL(v))
            return JSJTYPE_NULL;
        js_obj = JSVAL_TO_OBJECT(v);
        if (JS_InstanceOf(cx, js_obj, &JavaObject_class, 0))
	    return JSJTYPE_JAVAOBJECT;
        if (JS_InstanceOf(cx, js_obj, &JavaArray_class, 0))
            return JSJTYPE_JAVAARRAY;
        if (JS_InstanceOf(cx, js_obj, &JavaClass_class, 0))
            return JSJTYPE_JAVACLASS;
        if (JS_IsArrayObject(cx, js_obj))
            return JSJTYPE_JSARRAY;
        return JSJTYPE_OBJECT;
    } else if (JSVAL_IS_NUMBER(v)) {
	return JSJTYPE_NUMBER;
    } else if (JSVAL_IS_STRING(v)) {
	return JSJTYPE_STRING;
    } else if (JSVAL_IS_BOOLEAN(v)) {
	return JSJTYPE_BOOLEAN;
    } else if (JSVAL_IS_VOID(v)) {
	return JSJTYPE_VOID;
    }
    JS_ASSERT(0);   
    return JSJTYPE_VOID;
}









static int rank_table[JSJTYPE_LIMIT][JAVA_SIGNATURE_LIMIT] = {








    {99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,  1,  1}, 
    { 1, 99, 99, 99, 99, 99, 99, 99, 99, 99,  2, 99, 99, 99,  3,  4}, 
    {99,  7,  8,  6,  5,  4,  3,  1, 99, 99, 99, 99,  2, 99, 11,  9}, 
    {99,  3,  4,  4,  4,  4,  4,  4, 99, 99, 99, 99, 99, 99,  2,  1}, 
    {99, 99, 99, 99, 99, 99, 99, 99,  1,  1,  1,  1,  1,  1,  1,  1}, 
    {99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,  1, 99,  2,  3,  4}, 
    {99,  7,  8,  6,  5,  4,  3,  2,  0,  0,  0,  0,  0,  0,  0,  1}, 
    {99, 99, 99, 99, 99, 99, 99, 99,  0,  0, 99, 99, 99, 99,  0,  1}, 
    {99, 99, 99, 99, 99, 99, 99, 99,  2, 99, 99, 99, 99,  1,  3,  4}, 
    {99,  9, 10,  8,  7,  6,  5,  4, 99, 99, 99, 99, 99,  1,  2,  3}  
};





static JSBool
method_signature_matches_JS_args(JSContext *cx, JNIEnv *jEnv, uintN argc, jsval *argv,
                                 JavaMethodSignature *method_signature)
{
    uintN i;
    JavaClassDescriptor *descriptor;
    JavaObjectWrapper *java_wrapper;
    jclass java_class;
    jobject java_obj;
    JSObject *js_obj;
    JSJType js_type;
    jsval js_val;
    int rank;

    if (argc != (uintN)method_signature->num_args)
        return JS_FALSE;

    for (i = 0; i < argc; i++) {
        js_val = argv[i];
        js_type = compute_jsj_type(cx, js_val);
        descriptor = method_signature->arg_signatures[i];
        rank = rank_table[js_type][(int)descriptor->type - 2];

        
        if (rank == 99)
            return JS_FALSE;

        
        if (rank == 0) {
            java_class = descriptor->java_class;
        
            js_obj = JSVAL_TO_OBJECT(js_val);
            java_wrapper = JS_GetPrivate(cx, js_obj);
            java_obj = java_wrapper->java_obj;
        
            if (!(*jEnv)->IsInstanceOf(jEnv, java_obj, java_class))
                return JS_FALSE;
        }
    }

    return JS_TRUE;
}

#ifdef HAS_OLD_STYLE_METHOD_RESOLUTION

static JavaMethodSpec *
resolve_overloaded_method(JSContext *cx, JNIEnv *jEnv, JavaMemberDescriptor *member_descriptor,
                          JavaClassDescriptor *class_descriptor,
                          JSBool is_static_method,
                          uintN argc, jsval *argv)
{
    int cost, lowest_cost, num_method_matches;
    JavaMethodSpec *best_method_match, *method;

    num_method_matches = 0;
    lowest_cost = 10000;
    best_method_match = NULL;

    for (method = member_descriptor->methods; method; method = method->next) {
        cost = 0;
        if (!method_signature_matches_JS_args(cx, jEnv, argc, argv, &method->signature, &cost))
            continue;

        if (cost < lowest_cost) {
            lowest_cost = cost;
            best_method_match = method;
            num_method_matches++;
        }
    }

    if (!best_method_match)
        report_method_match_failure(cx, member_descriptor, class_descriptor,
                                    is_static_method, argc, argv);

    if (lowest_cost != 0)
        return NULL;

    return best_method_match;
}

#else   







static JSJTypePreference
preferred_conversion(JSContext *cx, JNIEnv *jEnv, jsval js_val,
                     JavaClassDescriptor *descriptor1,
                     JavaClassDescriptor *descriptor2)
{
    JSJType js_type;
    int rank1, rank2;
    jclass java_class1, java_class2;
    JavaObjectWrapper *java_wrapper;
    jobject java_obj;
    JSObject *js_obj;
    
    js_type = compute_jsj_type(cx, js_val);
    rank1 = rank_table[js_type][(int)descriptor1->type - 2];
    rank2 = rank_table[js_type][(int)descriptor2->type - 2];
        
      
    if (rank1 < rank2)
        return JSJPREF_FIRST_ARG;

    



    if (rank2 == 0) {
        java_class1 = descriptor1->java_class;
        java_class2 = descriptor2->java_class;
        
        js_obj = JSVAL_TO_OBJECT(js_val);
        java_wrapper = JS_GetPrivate(cx, js_obj);
        java_obj = java_wrapper->java_obj;
        
        
        if (!(*jEnv)->IsInstanceOf(jEnv, java_obj, java_class2))
            return JSJPREF_FIRST_ARG;

        



        if (rank1 != 0)
            return JSJPREF_SECOND_ARG;
        
        



        if ((*jEnv)->IsAssignableFrom(jEnv, java_class1, java_class2))
            return JSJPREF_FIRST_ARG;
        
        if ((*jEnv)->IsAssignableFrom(jEnv, java_class2, java_class1))
            return JSJPREF_SECOND_ARG;

        
        return JSJPREF_AMBIGUOUS;
    }
    
    if (rank1 > rank2)
        return JSJPREF_SECOND_ARG;
    
    return JSJPREF_AMBIGUOUS;
}
              
static JSJTypePreference
method_preferred(JSContext *cx, JNIEnv *jEnv, jsval *argv,
                 JavaMethodSignature *method_signature1,
                 JavaMethodSignature *method_signature2)
{
    int arg_index, argc, preference;
    jsval val;
    JavaSignature* *arg_signatures1;
    JavaSignature* *arg_signatures2;
    JavaSignature *arg_type1, *arg_type2;

    arg_signatures1 = method_signature1->arg_signatures;
    arg_signatures2 = method_signature2->arg_signatures;
    argc = method_signature1->num_args;
    JS_ASSERT(argc == method_signature2->num_args);

    preference = 0;
    for (arg_index = 0; arg_index < argc; arg_index++) {
        val = argv[arg_index];
        arg_type1 = *arg_signatures1++;
        arg_type2 = *arg_signatures2++;

        if (arg_type1 == arg_type2)
            continue;

        preference |= preferred_conversion(cx, jEnv, val, arg_type1, arg_type2);

        if ((JSJTypePreference)preference == JSJPREF_AMBIGUOUS)
            return JSJPREF_AMBIGUOUS;
    }
    return (JSJTypePreference)preference;
}









static JavaMethodSpec *
resolve_overloaded_method(JSContext *cx, JNIEnv *jEnv,
                          JavaMemberDescriptor *member_descriptor,
                          JavaClassDescriptor *class_descriptor,
                          JSBool is_static_method,
                          uintN argc, jsval *argv)
{
    JSJTypePreference preference;
    JavaMethodSpec *method, *best_method_match;
    MethodList ambiguous_methods;
    MethodListElement *method_list_element, *next_element;

    



    for (method = member_descriptor->methods; method; method = method->next) {
        if (method_signature_matches_JS_args(cx, jEnv, argc, argv, &method->signature))
            break;
    }

    
    if (!method) {
        report_method_match_failure(cx, member_descriptor, class_descriptor,
                                    is_static_method, argc, argv);
        return NULL;
    }

    
    if (!method->next)
        return method;

    



    JS_INIT_CLIST(&ambiguous_methods);

    best_method_match = method;

    
    for (method = method->next; method; method = method->next) {
        if (method->signature.num_args != (int)argc)
            continue;
        preference = method_preferred(cx, jEnv, argv, &best_method_match->signature,
                                      &method->signature);
        if (preference == JSJPREF_SECOND_ARG) {
            best_method_match = method;
        } else  if (preference == JSJPREF_AMBIGUOUS) {
            
            method_list_element =
                (MethodListElement*)JS_malloc(cx, sizeof(MethodListElement));
            if (!method_list_element)
                goto error;
            method_list_element->method = method;
            JS_APPEND_LINK(&method_list_element->linkage, &ambiguous_methods);
        }
    }
    
    



    
    for (method_list_element = (MethodListElement*)JS_LIST_HEAD(&ambiguous_methods);
        (MethodList*)method_list_element != &ambiguous_methods;
         method_list_element = next_element) {
        next_element = (MethodListElement*)method_list_element->linkage.next;
        method = method_list_element->method;
        preference = method_preferred(cx, jEnv, argv, &best_method_match->signature,
                                      &method->signature);
        if (preference != JSJPREF_FIRST_ARG)
            continue;
        JS_REMOVE_LINK(&method_list_element->linkage);
        JS_free(cx, method_list_element);
    }
    
    



    if (!JS_CLIST_IS_EMPTY(&ambiguous_methods)) {
        
	method_list_element =
	    (MethodListElement*)JS_malloc(cx, sizeof(MethodListElement));
	if (!method_list_element)
	    goto error;
	method_list_element->method = best_method_match;
	JS_APPEND_LINK(&method_list_element->linkage, &ambiguous_methods);

	
        report_ambiguous_method_match(cx, member_descriptor, class_descriptor,
                                      &ambiguous_methods, is_static_method, argc, argv);
        goto error;
    }

    return best_method_match;

error:
    
    while (!JS_CLIST_IS_EMPTY(&ambiguous_methods)) {
        method_list_element = (MethodListElement*)JS_LIST_HEAD(&ambiguous_methods);
        JS_REMOVE_LINK(&method_list_element->linkage);
        JS_free(cx, method_list_element);
    }

    return NULL;
}

#endif  

static jvalue *
convert_JS_method_args_to_java_argv(JSContext *cx, JNIEnv *jEnv, jsval *argv,
                        JavaMethodSpec *method, JSBool **localvp)
{
    jvalue *jargv;
    JSBool ok, *localv;
    uintN i, argc;
    JavaSignature **arg_signatures;
    JavaMethodSignature *signature;
    
    
    signature = &method->signature;
    argc = signature->num_args;
    JS_ASSERT(argc != 0);
    arg_signatures = signature->arg_signatures;
    
    jargv = (jvalue *)JS_malloc(cx, sizeof(jvalue) * argc);
    
    if (!jargv)
        return NULL;

    




    localv = (JSBool *)JS_malloc(cx, sizeof(JSBool) * argc);
    *localvp = localv;
    if (!localv) {
        JS_free(cx, jargv);
        return NULL;
    }
 
    for (i = 0; i < argc; i++) {
        int dummy_cost;
        
        ok = jsj_ConvertJSValueToJavaValue(cx, jEnv, argv[i], arg_signatures[i],
                                           &dummy_cost, &jargv[i], &localv[i]);
        if (!ok) {
            JS_free(cx, jargv);
            JS_free(cx, localv);
            *localvp = NULL;
            return NULL;
        }
    }

    return jargv;
}  

static JSBool
invoke_java_method(JSContext *cx, JSJavaThreadState *jsj_env,
                   jobject java_class_or_instance,
                   JavaClassDescriptor *class_descriptor,
                   JavaMethodSpec *method,
                   JSBool is_static_method,
                   jsval *argv, jsval *vp)
{
    jvalue java_value;
    jvalue *jargv;
    uintN argc, i;
    jobject java_object;
    jclass java_class;
    jmethodID methodID;
    JavaMethodSignature *signature;
    JavaSignature *return_val_signature;
    JNIEnv *jEnv;
    JSBool *localv, error_occurred, success;

    success = error_occurred = JS_FALSE;
    return_val_signature = NULL;    

    methodID = method->methodID;
    signature = &method->signature;
    argc = signature->num_args;

    jEnv = jsj_env->jEnv;

    if (is_static_method) {
        java_object = NULL;
        java_class = java_class_or_instance;
    } else {
        java_object = java_class_or_instance;
        java_class = NULL;
    }

    jargv = NULL;
    localv = NULL;
    if (argc) {
        jargv = convert_JS_method_args_to_java_argv(cx, jEnv, argv, method, &localv);
        if (!jargv) {
            error_occurred = JS_TRUE;
            goto out;
        }
    }

    

#ifdef JSJ_THREADSAFE
    JS_EndRequest(cx);
#endif

#define CALL_JAVA_METHOD(type, member)                                       \
    JS_BEGIN_MACRO                                                           \
    if (is_static_method) {                                                  \
        java_value.member = (*jEnv)->CallStatic##type##MethodA(jEnv, java_class, methodID, jargv);\
    } else {                                                                 \
        java_value.member = (*jEnv)->Call##type##MethodA(jEnv, java_object, methodID, jargv);\
    }                                                                        \
    if ((*jEnv)->ExceptionOccurred(jEnv)) {                                  \
        jsj_ReportJavaError(cx, jEnv, "Error calling method %s.%s()",        \
                            class_descriptor->name, method->name);           \
        error_occurred = JS_TRUE;                                            \
        goto out;                                                            \
    }                                                                        \
    JS_END_MACRO

    return_val_signature = signature->return_val_signature;
    switch(return_val_signature->type) {
    case JAVA_SIGNATURE_BYTE:
        CALL_JAVA_METHOD(Byte, b);
        break;

    case JAVA_SIGNATURE_CHAR:
        CALL_JAVA_METHOD(Char, c);
        break;
    
    case JAVA_SIGNATURE_FLOAT:
        CALL_JAVA_METHOD(Float, f);
        break;
    
    case JAVA_SIGNATURE_DOUBLE:
        CALL_JAVA_METHOD(Double, d);
        break;
    
    case JAVA_SIGNATURE_INT:
        CALL_JAVA_METHOD(Int, i);
        break;
    
    case JAVA_SIGNATURE_LONG:
        CALL_JAVA_METHOD(Long, j);
        break;
    
    case JAVA_SIGNATURE_SHORT:
        CALL_JAVA_METHOD(Short, s);
        break;
    
    case JAVA_SIGNATURE_BOOLEAN:
        CALL_JAVA_METHOD(Boolean, z);
        break;
    
    case JAVA_SIGNATURE_VOID:
        if (is_static_method)
            (*jEnv)->CallStaticVoidMethodA(jEnv, java_class, methodID, jargv);
        else
            (*jEnv)->CallVoidMethodA(jEnv, java_object, methodID, jargv);
        if ((*jEnv)->ExceptionOccurred(jEnv)) {
            jsj_ReportJavaError(cx, jEnv, "Error calling method %s.%s()",
                                class_descriptor->name, method->name);
            error_occurred = JS_TRUE;
            goto out;
        }
        break;
        
    case JAVA_SIGNATURE_UNKNOWN:
        JS_ASSERT(0);
        error_occurred = JS_TRUE;
        goto out;
            
    
    default:
        JS_ASSERT(IS_REFERENCE_TYPE(return_val_signature->type));
        CALL_JAVA_METHOD(Object, l);
        break;
    }

out:

    if (localv) {
        for (i = 0; i < argc; i++) {
            if (localv[i])
                (*jEnv)->DeleteLocalRef(jEnv, jargv[i].l);
        }
        JS_free(cx, localv);
    }
    if (jargv)
       JS_free(cx, jargv);

#ifdef JSJ_THREADSAFE
    JS_BeginRequest(cx);
#endif

    if (!error_occurred) {
        success = jsj_ConvertJavaValueToJSValue(cx, jEnv, return_val_signature, &java_value, vp);
        if (IS_REFERENCE_TYPE(return_val_signature->type))
            (*jEnv)->DeleteLocalRef(jEnv, java_value.l);
    }
    return success;
}

static JSBool
invoke_overloaded_java_method(JSContext *cx, JSJavaThreadState *jsj_env,
                              JavaMemberDescriptor *member,
                              JSBool is_static_method,
                              jobject java_class_or_instance,
                              JavaClassDescriptor *class_descriptor,
                              uintN argc, jsval *argv,
                              jsval *vp)
{
    JavaMethodSpec *method;
    JNIEnv *jEnv;

    jEnv = jsj_env->jEnv;

    method = resolve_overloaded_method(cx, jEnv, member, class_descriptor,
                                       is_static_method, argc, argv);
    if (!method)
        return JS_FALSE;

    return invoke_java_method(cx, jsj_env, java_class_or_instance, class_descriptor, 
                              method, is_static_method, argv, vp);
}

static JSBool
invoke_java_constructor(JSContext *cx,
                        JSJavaThreadState *jsj_env,
                        jclass java_class,
                JavaMethodSpec *method,
                jsval *argv, jsval *vp)
{
    jvalue *jargv;
    uintN argc, i;
    jobject java_object;
    jmethodID methodID;
    JavaMethodSignature *signature;
    JNIEnv *jEnv;
    JSBool *localv;
    JSBool success, error_occurred;
    java_object = NULL;	    

    success = error_occurred = JS_FALSE;
    
    methodID = method->methodID;
    signature = &method->signature;
    argc = signature->num_args;

    jEnv = jsj_env->jEnv;

    jargv = NULL;
    localv = NULL;
    if (argc) {
        jargv = convert_JS_method_args_to_java_argv(cx, jEnv, argv, method, &localv);
        if (!jargv) {
            error_occurred = JS_TRUE;
            goto out;
        }
    }

    

#ifdef JSJ_THREADSAFE
    JS_EndRequest(cx);
#endif

    
    java_object = (*jEnv)->NewObjectA(jEnv, java_class, methodID, jargv);

#ifdef JSJ_THREADSAFE
    JS_BeginRequest(cx);
#endif

    if (!java_object) {
        jsj_ReportJavaError(cx, jEnv, "Error while constructing instance of %s",
                            jsj_GetJavaClassName(cx, jEnv, java_class));
        error_occurred = JS_TRUE;
        goto out;
    }

out:
    if (localv) {
        for (i = 0; i < argc; i++) {
            if (localv[i])
                (*jEnv)->DeleteLocalRef(jEnv, jargv[i].l);
        }
        JS_free(cx, localv);
    }
    if (jargv)
       JS_free(cx, jargv);
        
    if (!error_occurred)
        success = jsj_ConvertJavaObjectToJSValue(cx, jEnv, java_object, vp);
    (*jEnv)->DeleteLocalRef(jEnv, java_object);
    return success;
}

static JSBool
invoke_overloaded_java_constructor(JSContext *cx,
                                   JSJavaThreadState *jsj_env,
                                   JavaMemberDescriptor *member,
                                   JavaClassDescriptor *class_descriptor,
                                   uintN argc, jsval *argv,
                                   jsval *vp)
{
    jclass java_class;
    JavaMethodSpec *method;
    JNIEnv *jEnv;

    jEnv = jsj_env->jEnv;

    method = resolve_overloaded_method(cx, jEnv, member, class_descriptor, JS_TRUE, 
                                       argc, argv);
    if (!method)
        return JS_FALSE;

    java_class = class_descriptor->java_class;
    return invoke_java_constructor(cx, jsj_env, java_class, method, argv, vp);
}

static JSBool
java_constructor_wrapper(JSContext *cx, JSJavaThreadState *jsj_env,
                         JavaMemberDescriptor *member_descriptor,
                         JavaClassDescriptor *class_descriptor,
                         uintN argc, jsval *argv, jsval *vp)
{
    jint modifiers;
    JNIEnv *jEnv;

    jEnv = jsj_env->jEnv;
    
    
    modifiers = class_descriptor->modifiers;
    if (modifiers & ACC_ABSTRACT) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                            JSJMSG_ABSTRACT_JCLASS, class_descriptor->name);
        return JS_FALSE;
    }
    if (modifiers & ACC_INTERFACE) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                            JSJMSG_IS_INTERFACE, class_descriptor->name);
        return JS_FALSE;
    }
    if ( !(modifiers & ACC_PUBLIC) ) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                            JSJMSG_NOT_PUBLIC, class_descriptor->name);
        return JS_FALSE;
    }
    
    if (!member_descriptor) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                            JSJMSG_NO_CONSTRUCTORS, class_descriptor->name);
        return JS_FALSE;
    }

    return invoke_overloaded_java_constructor(cx, jsj_env, member_descriptor,
                                              class_descriptor, argc, argv, vp);
}

JS_EXPORT_API(JSBool)
jsj_JavaConstructorWrapper(JSContext *cx, JSObject *obj,
                           uintN argc, jsval *argv, jsval *vp)
{
    JavaClassDescriptor *class_descriptor;
    JavaMemberDescriptor *member_descriptor;
    JSJavaThreadState *jsj_env;
    JNIEnv *jEnv;
    JSBool result;

    obj = JSVAL_TO_OBJECT(argv[-2]);
    class_descriptor = JS_GetPrivate(cx, obj);
    JS_ASSERT(class_descriptor);
    if (!class_descriptor)
        return JS_FALSE;

    



    if (strstr(class_descriptor->name, "sun.plugin.") == class_descriptor->name)
        return JS_FALSE;
  
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;
    member_descriptor = jsj_LookupJavaClassConstructors(cx, jEnv, class_descriptor);
    result = java_constructor_wrapper(cx, jsj_env, member_descriptor, 
                                      class_descriptor, argc, argv, vp);
    jsj_ExitJava(jsj_env);
    return result;
}


static JSBool
static_method_wrapper(JSContext *cx, JSJavaThreadState *jsj_env,
                      JavaClassDescriptor *class_descriptor,
                      jsid id,
                      uintN argc, jsval *argv, jsval *vp)
{
    JNIEnv *jEnv;
    JavaMemberDescriptor *member_descriptor;

    jEnv = jsj_env->jEnv;
    member_descriptor = jsj_LookupJavaStaticMemberDescriptorById(cx, jEnv, class_descriptor, id);
    
    
    if (member_descriptor && strcmp(member_descriptor->name, "<init>")) {
        return invoke_overloaded_java_method(cx, jsj_env, member_descriptor, JS_TRUE, 
                                             class_descriptor->java_class,
                                             class_descriptor, argc, argv, vp);
    }

    JS_ASSERT(member_descriptor);
    if (!member_descriptor)
        return JS_FALSE;
    
    
    return java_constructor_wrapper(cx, jsj_env, member_descriptor,
                                    class_descriptor, argc, argv, vp);
}

JS_EXTERN_API(JSBool)
jsj_JavaStaticMethodWrapper(JSContext *cx, JSObject *obj,
                            uintN argc, jsval *argv, jsval *vp)
{
    JSFunction *function;
    JavaClassDescriptor *class_descriptor;
    jsid id;
    jsval idval;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;
    JSBool result;
    
    class_descriptor = JS_GetPrivate(cx, obj);
    if (!class_descriptor)
        return JS_FALSE;

    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;
    
    JS_ASSERT(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION);
    function = JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[-2]));
    idval = STRING_TO_JSVAL(JS_InternString(cx, JS_GetFunctionName(function)));
    JS_ValueToId(cx, idval, &id);

    result = static_method_wrapper(cx, jsj_env, class_descriptor, id, argc, argv, vp);
    jsj_ExitJava(jsj_env);
    return result;
}

JS_EXPORT_API(JSBool)
jsj_JavaInstanceMethodWrapper(JSContext *cx, JSObject *obj,
                              uintN argc, jsval *argv, jsval *vp)
{
    JSFunction *function;
    JavaMemberDescriptor *member_descriptor;
    JavaObjectWrapper *java_wrapper;
    JavaClassDescriptor *class_descriptor;
    jsid id;
    jsval idval;
    JSJavaThreadState *jsj_env;
    JNIEnv *jEnv;
    jobject java_obj;
    JSBool result;
    
    java_wrapper = JS_GetPrivate(cx, obj);
    if (!java_wrapper)
        return JS_FALSE;
    java_obj = java_wrapper->java_obj;
    
    JS_ASSERT(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION);
    function = JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[-2]));
    idval = STRING_TO_JSVAL(JS_InternString(cx, JS_GetFunctionName(function)));
    JS_ValueToId(cx, idval, &id);
    class_descriptor = java_wrapper->class_descriptor;
    
    
    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv)
        return JS_FALSE;

    if (jaApplet && (*jEnv)->IsInstanceOf(jEnv, java_obj, jaApplet)) {
        jsj_JSIsCallingApplet = JS_TRUE;
    }

    
    member_descriptor = jsj_LookupJavaMemberDescriptorById(cx, jEnv, class_descriptor, id);
    if (member_descriptor)
        result = invoke_overloaded_java_method(cx, jsj_env, member_descriptor,
                                               JS_FALSE, java_obj, 
                                               class_descriptor, argc, argv, vp);

    
    else
	result = static_method_wrapper(cx, jsj_env, class_descriptor, id, argc, argv, vp);
    jsj_ExitJava(jsj_env);
    return result;
}

