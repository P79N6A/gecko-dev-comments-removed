
















































#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"        
#include "jsj_hash.h"           

#ifdef JSJ_THREADSAFE
#   include "prmon.h"
#endif



static JSJHashTable *java_class_reflections;

#ifdef JSJ_THREADSAFE
static PRMonitor *java_class_reflections_monitor;
#endif








const char *
jsj_GetJavaClassName(JSContext *cx, JNIEnv *jEnv, jclass java_class)
{
    jstring java_class_name_jstr;
    const char *java_class_name;

    
    java_class_name_jstr =
        (*jEnv)->CallObjectMethod(jEnv, java_class, jlClass_getName);

    if (!java_class_name_jstr)
        goto error;

    


#ifdef XP_UNIX
    if ((*jEnv)->ExceptionOccurred(jEnv))
        goto error;
#endif

    
    java_class_name = jsj_DupJavaStringUTF(cx, jEnv, java_class_name_jstr);

    (*jEnv)->DeleteLocalRef(jEnv, java_class_name_jstr);
    return java_class_name;

error:
    jsj_UnexpectedJavaError(cx, jEnv, "Can't get Java class name using"
                                      "java.lang.Class.getName()");
    return NULL;
}






void
jsj_MakeJNIClassname(char * class_name)
{
    char * c;
    for (c = class_name; *c; c++)
        if (*c == '.')
            *c = '/';
}







static JavaSignatureChar
get_signature_type(JSContext *cx, JavaClassDescriptor *class_descriptor)
{
    JavaSignatureChar type;
    const char *java_class_name;

    
    java_class_name = class_descriptor->name;
    JS_ASSERT(java_class_name);
    if (!java_class_name)
        return JAVA_SIGNATURE_UNKNOWN;

    if (!strcmp(java_class_name, "byte"))
        type = JAVA_SIGNATURE_BYTE;
    else if (!strcmp(java_class_name, "char"))
        type = JAVA_SIGNATURE_CHAR;
    else if (!strcmp(java_class_name, "float"))
        type = JAVA_SIGNATURE_FLOAT;
    else if (!strcmp(java_class_name, "double"))
        type = JAVA_SIGNATURE_DOUBLE;
    else if (!strcmp(java_class_name, "int"))
        type = JAVA_SIGNATURE_INT;
    else if (!strcmp(java_class_name, "long"))
        type = JAVA_SIGNATURE_LONG;
    else if (!strcmp(java_class_name, "short"))
        type = JAVA_SIGNATURE_SHORT;
    else if (!strcmp(java_class_name, "boolean"))
        type = JAVA_SIGNATURE_BOOLEAN;
    else if (!strcmp(java_class_name, "void"))
        type = JAVA_SIGNATURE_VOID;
    else if (!strcmp(java_class_name, "java.lang.Boolean"))
        type = JAVA_SIGNATURE_JAVA_LANG_BOOLEAN;
    else if (!strcmp(java_class_name, "java.lang.Double"))
        type = JAVA_SIGNATURE_JAVA_LANG_DOUBLE;
    else if (!strcmp(java_class_name, "java.lang.String"))
        type = JAVA_SIGNATURE_JAVA_LANG_STRING;
    else if (!strcmp(java_class_name, "java.lang.Object"))
        type = JAVA_SIGNATURE_JAVA_LANG_OBJECT;
    else if (!strcmp(java_class_name, "java.lang.Class"))
        type = JAVA_SIGNATURE_JAVA_LANG_CLASS;
    else if (!strcmp(java_class_name, "netscape.javascript.JSObject"))
        type = JAVA_SIGNATURE_NETSCAPE_JAVASCRIPT_JSOBJECT;
    else
        type = JAVA_SIGNATURE_OBJECT;
    return type;
}

static JSBool
is_java_array_class(JNIEnv *jEnv, jclass java_class)
{
    return (*jEnv)->CallBooleanMethod(jEnv, java_class, jlClass_isArray);
}








static jclass
get_java_array_component_class(JSContext *cx, JNIEnv *jEnv, jclass java_class)
{
    jclass result;
    result = (*jEnv)->CallObjectMethod(jEnv, java_class, jlClass_getComponentType);
    if (!result) {
        jsj_UnexpectedJavaError(cx, jEnv,
                                "Can't get Java array component class using "
                                "java.lang.Class.getComponentType()");
        return NULL;
    }
    return result;
}





static JSBool
compute_java_class_signature(JSContext *cx, JNIEnv *jEnv, JavaSignature *signature)
{
    jclass java_class = signature->java_class;

    if (is_java_array_class(jEnv, java_class)) {
        jclass component_class;
        
        signature->type = JAVA_SIGNATURE_ARRAY;

        component_class = get_java_array_component_class(cx, jEnv, java_class);
        if (!component_class)
            return JS_FALSE;

        signature->array_component_signature =
            jsj_GetJavaClassDescriptor(cx, jEnv, component_class);
        if (!signature->array_component_signature) {
            (*jEnv)->DeleteLocalRef(jEnv, component_class);
            return JS_FALSE;
        }
    } else {
        signature->type = get_signature_type(cx, signature);
    }
    return JS_TRUE;
}





static char
get_jdk_signature_char(JavaSignatureChar type)
{
    return "XVZCBSIJFD[LLLLLL"[(int)type];
}








const char *
jsj_ConvertJavaSignatureToString(JSContext *cx, JavaSignature *signature)
{
    char *sig;

    if (IS_OBJECT_TYPE(signature->type)) {
        
        sig = JS_smprintf("L%s;", signature->name);
        if (sig)
            jsj_MakeJNIClassname(sig);

    } else if (signature->type == JAVA_SIGNATURE_ARRAY) {
        
        const char *component_signature_string;

        component_signature_string =
            jsj_ConvertJavaSignatureToString(cx, signature->array_component_signature);
        if (!component_signature_string)
            return NULL;
        sig = JS_smprintf("[%s", component_signature_string);
        JS_smprintf_free((char*)component_signature_string);

    } else {
        
        sig = JS_smprintf("%c", get_jdk_signature_char(signature->type));
    }

    if (!sig) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    return sig;
}








const char *
jsj_ConvertJavaSignatureToHRString(JSContext *cx,
                                   JavaSignature *signature)
{
    char *sig;
    JavaSignature *acs;

    if (signature->type == JAVA_SIGNATURE_ARRAY) {
        
        const char *component_signature_string;
        acs = signature->array_component_signature;
        component_signature_string =
            jsj_ConvertJavaSignatureToHRString(cx, acs);
        if (!component_signature_string)
            return NULL;
        sig = JS_smprintf("%s[]", component_signature_string);
        JS_smprintf_free((char*)component_signature_string);

    } else {
        
        sig = JS_smprintf("%s", signature->name);
    }

    if (!sig) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    return sig;
}

static void
destroy_java_member_descriptor(JSContext *cx, JNIEnv *jEnv, JavaMemberDescriptor *member_descriptor)
{
    JavaMethodSpec *method, *next_method;
    if (member_descriptor->field)
        jsj_DestroyFieldSpec(cx, jEnv, member_descriptor->field);

    method = member_descriptor->methods;
    while (method) {
        next_method = method->next;
        jsj_DestroyMethodSpec(cx, jEnv, method);
        method = next_method;
    }

    JS_RemoveRoot(cx, &member_descriptor->invoke_func_obj);
    JS_FREE_IF(cx, (char *)member_descriptor->name);
    JS_free(cx, member_descriptor);
}

static void
destroy_class_member_descriptors(JSContext *cx, JNIEnv *jEnv, JavaMemberDescriptor *member_descriptor)
{
    JavaMemberDescriptor *next_member;
    
    while (member_descriptor) {
        next_member = member_descriptor->next;
        destroy_java_member_descriptor(cx, jEnv, member_descriptor);
        member_descriptor = next_member;
    }
}

static void
destroy_class_descriptor(JSContext *cx, JNIEnv *jEnv, JavaClassDescriptor *class_descriptor)
{
    JS_FREE_IF(cx, (char *)class_descriptor->name);
    if (class_descriptor->java_class)
        (*jEnv)->DeleteGlobalRef(jEnv, class_descriptor->java_class);

    if (class_descriptor->array_component_signature)
        jsj_ReleaseJavaClassDescriptor(cx, jEnv, class_descriptor->array_component_signature);

    destroy_class_member_descriptors(cx, jEnv, class_descriptor->instance_members);
    destroy_class_member_descriptors(cx, jEnv, class_descriptor->static_members);
    destroy_class_member_descriptors(cx, jEnv, class_descriptor->constructors);
    JS_free(cx, class_descriptor);
}

static JavaClassDescriptor *
new_class_descriptor(JSContext *cx, JNIEnv *jEnv, jclass java_class)
{
    JavaClassDescriptor *class_descriptor;

    class_descriptor = (JavaClassDescriptor *)JS_malloc(cx, sizeof(JavaClassDescriptor));
    if (!class_descriptor)
        return NULL;
    memset(class_descriptor, 0, sizeof(JavaClassDescriptor));

    class_descriptor->name = jsj_GetJavaClassName(cx, jEnv, java_class);
    if (!class_descriptor->name)
        goto error;

    java_class = (*jEnv)->NewGlobalRef(jEnv, java_class);
    if (!java_class) {
        jsj_UnexpectedJavaError(cx, jEnv, "Unable to reference Java class");
        goto error;
    }
    class_descriptor->java_class = java_class;

    if (!compute_java_class_signature(cx, jEnv, class_descriptor))
        goto error;

    class_descriptor->modifiers =
        (*jEnv)->CallIntMethod(jEnv, java_class, jlClass_getModifiers);
    class_descriptor->ref_count = 1;

    if (!JSJ_HashTableAdd(java_class_reflections, java_class, class_descriptor,
                          (void*)jEnv))
        goto error;

    return class_descriptor;

error:
    destroy_class_descriptor(cx, jEnv, class_descriptor);
    return NULL;
}


JS_STATIC_DLL_CALLBACK(JSIntn)
enumerate_remove_java_class(JSJHashEntry *he, JSIntn i, void *arg)
{
    JSJavaThreadState *jsj_env = (JSJavaThreadState *)arg;
    JavaClassDescriptor *class_descriptor;

    class_descriptor = (JavaClassDescriptor*)he->value;

    destroy_class_descriptor(jsj_env->cx, jsj_env->jEnv, class_descriptor);

    return HT_ENUMERATE_REMOVE;
}




void
jsj_DiscardJavaClassReflections(JNIEnv *jEnv)
{
    JSJavaThreadState *jsj_env;
    char *err_msg;
    JSContext *cx;

    
    jsj_env = jsj_MapJavaThreadToJSJavaThreadState(jEnv, &err_msg);
    JS_ASSERT(jsj_env);
    if (!jsj_env)
        goto error;

    
    cx = jsj_env->cx;
    if (!cx) {
        


        if (JSJ_callbacks->map_jsj_thread_to_js_context) {
#ifdef OJI
            cx = JSJ_callbacks->map_jsj_thread_to_js_context(jsj_env,
                                                             NULL, 
                                                             jEnv, &err_msg);
#else
            cx = JSJ_callbacks->map_jsj_thread_to_js_context(jsj_env,
                                                             jEnv, &err_msg);
#endif
            if (!cx)
                goto error;
        } else {
            err_msg = JS_smprintf("Unable to find/create JavaScript execution "
                                  "context for JNI thread 0x%08x", jEnv);
            goto error;
        }
    }

    if (java_class_reflections) {
        JSJ_HashTableEnumerateEntries(java_class_reflections,
                                      enumerate_remove_java_class,
                                      (void*)jsj_env);
        JSJ_HashTableDestroy(java_class_reflections);
        java_class_reflections = NULL;
    }

    return;

error:
    JS_ASSERT(!cx);
    if (err_msg) {
        jsj_LogError(err_msg);
        JS_smprintf_free(err_msg);
    }
}

extern JavaClassDescriptor *
jsj_GetJavaClassDescriptor(JSContext *cx, JNIEnv *jEnv, jclass java_class)
{
    JavaClassDescriptor *class_descriptor = NULL;

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(java_class_reflections_monitor);
#endif

    if (java_class_reflections) {
        class_descriptor = JSJ_HashTableLookup(java_class_reflections,
                                               (const void *)java_class,
                                               (void*)jEnv);
    }
    if (!class_descriptor) {
        class_descriptor = new_class_descriptor(cx, jEnv, java_class);

#ifdef JSJ_THREADSAFE
        PR_ExitMonitor(java_class_reflections_monitor);
#endif
        return class_descriptor;
    }

#ifdef JSJ_THREADSAFE
    PR_ExitMonitor(java_class_reflections_monitor);
#endif

    JS_ASSERT(class_descriptor->ref_count > 0);
    class_descriptor->ref_count++;
    return class_descriptor;
}

void
jsj_ReleaseJavaClassDescriptor(JSContext *cx, JNIEnv *jEnv, JavaClassDescriptor *class_descriptor)
{
#if 0
    

    JS_ASSERT(class_descriptor->ref_count >= 1);
    if (!--class_descriptor->ref_count) {
        JSJ_HashTableRemove(java_class_reflections,
                            class_descriptor->java_class, (void*)jEnv);
        destroy_class_descriptor(cx, jEnv, class_descriptor);
    }
#endif
}

#ifdef JSJ_THREADSAFE
static PRMonitor *java_reflect_monitor = NULL;
#endif

static JSBool
reflect_java_methods_and_fields(JSContext *cx,
                                JNIEnv *jEnv,
                                JavaClassDescriptor *class_descriptor,
                                JSBool reflect_statics_only)
{
    JavaMemberDescriptor *member_descriptor;
    JSBool success;

    success = JS_TRUE;  

#ifdef JSJ_THREADSAFE
    PR_EnterMonitor(java_reflect_monitor);
#endif

    



    if (reflect_statics_only) {
        if (class_descriptor->static_members_reflected != REFLECT_NO)
            goto done;
        class_descriptor->static_members_reflected = REFLECT_IN_PROGRESS;
    } else {
        if (class_descriptor->instance_members_reflected != REFLECT_NO)
            goto done;
        class_descriptor->instance_members_reflected = REFLECT_IN_PROGRESS;
    }
    
    if (!jsj_ReflectJavaMethods(cx, jEnv, class_descriptor, reflect_statics_only))
        goto error;
    if (!jsj_ReflectJavaFields(cx, jEnv, class_descriptor, reflect_statics_only))
        goto error;

    if (reflect_statics_only) {
        member_descriptor = class_descriptor->static_members;
        while (member_descriptor) {
            class_descriptor->num_static_members++;
            member_descriptor = member_descriptor->next;
        }
        class_descriptor->static_members_reflected = REFLECT_COMPLETE;
    } else {
        member_descriptor = class_descriptor->instance_members;
        while (member_descriptor) {
            class_descriptor->num_instance_members++;
            member_descriptor = member_descriptor->next;
        }
        class_descriptor->instance_members_reflected = REFLECT_COMPLETE;
    }

done:
#ifdef JSJ_THREADSAFE
    PR_ExitMonitor(java_reflect_monitor);
#endif
    return success;

error:
    success = JS_FALSE;
    goto done;
}

JavaMemberDescriptor *
jsj_GetClassStaticMembers(JSContext *cx,
                          JNIEnv *jEnv,
                          JavaClassDescriptor *class_descriptor)
{
    if (class_descriptor->static_members_reflected != REFLECT_COMPLETE)
        reflect_java_methods_and_fields(cx, jEnv, class_descriptor, JS_TRUE);
    return class_descriptor->static_members;
}

JavaMemberDescriptor *
jsj_GetClassInstanceMembers(JSContext *cx,
                            JNIEnv *jEnv,
                            JavaClassDescriptor *class_descriptor)
{
    if (class_descriptor->instance_members_reflected != REFLECT_COMPLETE)
        reflect_java_methods_and_fields(cx, jEnv, class_descriptor, JS_FALSE);
    return class_descriptor->instance_members;
}

JavaMemberDescriptor *
jsj_LookupJavaStaticMemberDescriptorById(JSContext *cx,
                                         JNIEnv *jEnv,
                                         JavaClassDescriptor *class_descriptor,
                                         jsid id)
{
    JavaMemberDescriptor *member_descriptor;

    member_descriptor = jsj_GetClassStaticMembers(cx, jEnv, class_descriptor);
    while (member_descriptor) {
        if (id == member_descriptor->id)
            return member_descriptor;
        member_descriptor = member_descriptor->next;
    }
    return NULL;
}

JavaMemberDescriptor *
jsj_GetJavaStaticMemberDescriptor(JSContext *cx,
                                  JNIEnv *jEnv, 
                                  JavaClassDescriptor *class_descriptor,
                                  jstring member_name_jstr)
{
    JavaMemberDescriptor *member_descriptor;
    jsid id;

    if (!JavaStringToId(cx, jEnv, member_name_jstr, &id))
        return NULL;

    member_descriptor = jsj_LookupJavaStaticMemberDescriptorById(cx, jEnv, class_descriptor, id);
    if (member_descriptor)
        return member_descriptor;

    member_descriptor = JS_malloc(cx, sizeof(JavaMemberDescriptor));
    if (!member_descriptor)
        return NULL;
    memset(member_descriptor, 0, sizeof(JavaMemberDescriptor));

    member_descriptor->name = jsj_DupJavaStringUTF(cx, jEnv, member_name_jstr);
    if (!member_descriptor->name) {
        JS_free(cx, member_descriptor);
        return NULL;
    }
    member_descriptor->id = id;

    member_descriptor->next = class_descriptor->static_members;
    class_descriptor->static_members = member_descriptor;

    return member_descriptor;
}

JavaMemberDescriptor *
jsj_GetJavaClassConstructors(JSContext *cx,
                             JavaClassDescriptor *class_descriptor)
{
    JavaMemberDescriptor *member_descriptor;

    if (class_descriptor->constructors)
        return class_descriptor->constructors;

    member_descriptor = JS_malloc(cx, sizeof(JavaMemberDescriptor));
    if (!member_descriptor)
        return NULL;
    memset(member_descriptor, 0, sizeof(JavaMemberDescriptor));

    member_descriptor->name = JS_strdup(cx, "<init>");
    if (!member_descriptor->name) {
        JS_free(cx, member_descriptor);
        return NULL;
    }

    class_descriptor->constructors = member_descriptor;

    return member_descriptor;
}

JavaMemberDescriptor *
jsj_LookupJavaClassConstructors(JSContext *cx, JNIEnv *jEnv,
                                JavaClassDescriptor *class_descriptor)
{
    if (class_descriptor->static_members_reflected != REFLECT_COMPLETE)
        reflect_java_methods_and_fields(cx, jEnv, class_descriptor, JS_TRUE);
    return class_descriptor->constructors;
}

JavaMemberDescriptor *
jsj_LookupJavaMemberDescriptorById(JSContext *cx, JNIEnv *jEnv,
                                   JavaClassDescriptor *class_descriptor,
                                   jsid id)
{
    JavaMemberDescriptor *member_descriptor;

    member_descriptor = jsj_GetClassInstanceMembers(cx, jEnv, class_descriptor);
    while (member_descriptor) {
        if (id == member_descriptor->id)
            return member_descriptor;
        member_descriptor = member_descriptor->next;
    }
    return NULL;
}

JavaMemberDescriptor *
jsj_GetJavaMemberDescriptor(JSContext *cx,
                            JNIEnv *jEnv, 
                            JavaClassDescriptor *class_descriptor,
                            jstring member_name_jstr)
{
    JavaMemberDescriptor *member_descriptor;
    jsid id;

    if (!JavaStringToId(cx, jEnv, member_name_jstr, &id))
        return NULL;

    member_descriptor = jsj_LookupJavaMemberDescriptorById(cx, jEnv, class_descriptor, id);
    if (member_descriptor)
        return member_descriptor;

    member_descriptor = JS_malloc(cx, sizeof(JavaMemberDescriptor));
    if (!member_descriptor)
        return NULL;
    memset(member_descriptor, 0, sizeof(JavaMemberDescriptor));

    member_descriptor->name = jsj_DupJavaStringUTF(cx, jEnv, member_name_jstr);
    if (!member_descriptor->name) {
        JS_free(cx, member_descriptor);
        return NULL;
    }
    member_descriptor->id = id;

    member_descriptor->next = class_descriptor->instance_members;
    class_descriptor->instance_members = member_descriptor;

    return member_descriptor;
}

JSBool
jsj_InitJavaClassReflectionsTable()
{
    if (!java_class_reflections) {
        java_class_reflections =
            JSJ_NewHashTable(64, jsj_HashJavaObject, jsj_JavaObjectComparator,
                            NULL, NULL, NULL);

        if (!java_class_reflections)
            return JS_FALSE;

#ifdef JSJ_THREADSAFE
        java_class_reflections_monitor =
                (struct PRMonitor *) PR_NewMonitor();
        if (!java_class_reflections_monitor)
            return JS_FALSE;

        java_reflect_monitor =
                (struct PRMonitor *) PR_NewMonitor();
        if (!java_reflect_monitor)
            return JS_FALSE;
#endif
    }
    
    return JS_TRUE;
}
