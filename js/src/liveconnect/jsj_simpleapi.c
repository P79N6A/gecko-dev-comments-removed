










































#include "jsjava.h"
#include "jsprf.h"
#include "jsutil.h"

#include <string.h>



static JSJavaVM *           the_jsj_vm = NULL;
static JSContext *          the_cx = NULL;
static JSJavaThreadState *  the_jsj_thread = NULL;
static JSObject *           the_global_js_obj = NULL;


static JSJavaThreadState *
default_map_js_context_to_jsj_thread(JSContext *cx, char **errp)
{
    return the_jsj_thread;
}


static JSContext *
default_map_jsj_thread_to_js_context(JSJavaThreadState *jsj_env,
#ifdef OJI
                                     void *java_applet_obj,
#endif
                                     JNIEnv *jEnv,
                                     char **errp)
{
    return the_cx;
}


static JSObject *
default_map_java_object_to_js_object(JNIEnv *jEnv, void *hint, char **errp)
{
    return the_global_js_obj;
}

static JSBool JS_DLL_CALLBACK
default_create_java_vm(SystemJavaVM* *jvm, JNIEnv* *initialEnv, void* initargs)
{
    jint err;
    const char* user_classpath = (const char*)initargs;
    char* full_classpath = NULL;

    
    JDK1_1InitArgs vm_args;
    memset(&vm_args, 0, sizeof(vm_args));

    
    vm_args.version = 0x00010001;
    JNI_GetDefaultJavaVMInitArgs(&vm_args);

    
    if (user_classpath) {
#if defined(XP_UNIX) || defined(XP_BEOS)
        full_classpath = JS_smprintf("%s:%s", user_classpath, vm_args.classpath);
#else
        full_classpath = JS_smprintf("%s;%s", user_classpath, vm_args.classpath);
#endif
        if (!full_classpath) {
            return JS_FALSE;
        }
        vm_args.classpath = (char*)full_classpath;
    }

    err = JNI_CreateJavaVM((JavaVM**)jvm, initialEnv, &vm_args);
    
    if (full_classpath)
        JS_smprintf_free(full_classpath);
    
    return err == 0;
}

static JSBool JS_DLL_CALLBACK
default_destroy_java_vm(SystemJavaVM* jvm, JNIEnv* initialEnv)
{
    JavaVM* java_vm = (JavaVM*)jvm;
    jint err = (*java_vm)->DestroyJavaVM(java_vm);
    return err == 0;
}

static JNIEnv* JS_DLL_CALLBACK
default_attach_current_thread(SystemJavaVM* jvm)
{
    JavaVM* java_vm = (JavaVM*)jvm;
    JNIEnv* env = NULL;
    (*java_vm)->AttachCurrentThread(java_vm, &env, NULL);
    return env;
}

static JSBool JS_DLL_CALLBACK
default_detach_current_thread(SystemJavaVM* jvm, JNIEnv* env)
{
    JavaVM* java_vm = (JavaVM*)jvm;
    
    jint err = (*java_vm)->DetachCurrentThread(java_vm);
    return err == 0;
}

static SystemJavaVM* JS_DLL_CALLBACK
default_get_java_vm(JNIEnv* env)
{
    JavaVM* java_vm = NULL;
    (*env)->GetJavaVM(env, &java_vm);
    return (SystemJavaVM*)java_vm;
}


JSJCallbacks jsj_default_callbacks = {
    default_map_jsj_thread_to_js_context,
    default_map_js_context_to_jsj_thread,
    default_map_java_object_to_js_object,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    default_create_java_vm,
    default_destroy_java_vm,
    default_attach_current_thread,
    default_detach_current_thread,
    default_get_java_vm
};








JSBool
JSJ_SimpleInit(JSContext *cx, JSObject *global_obj, SystemJavaVM *java_vm, const char *classpath)
{
    JNIEnv *jEnv;

    JSJ_Init(&jsj_default_callbacks);

    JS_ASSERT(!the_jsj_vm);
    the_jsj_vm = JSJ_ConnectToJavaVM(java_vm, (void*)classpath);
    if (!the_jsj_vm)
        return JS_FALSE;

    if (!JSJ_InitJSContext(cx, global_obj, NULL))
        goto error;
    the_cx = cx;
    the_global_js_obj = global_obj;

    the_jsj_thread = JSJ_AttachCurrentThreadToJava(the_jsj_vm, "main thread", &jEnv);
    if (!the_jsj_thread)
        goto error;
    JSJ_SetDefaultJSContextForJavaThread(cx, the_jsj_thread);
    return JS_TRUE;

error:
    JSJ_SimpleShutdown();
    return JS_FALSE;
}





JS_EXPORT_API(void)
JSJ_SimpleShutdown()
{
    JS_ASSERT(the_jsj_vm);
    JSJ_DisconnectFromJavaVM(the_jsj_vm);
    the_jsj_vm = NULL;
    the_cx = NULL;
    the_global_js_obj = NULL;
    the_jsj_thread = NULL;
}
