






















































#include <stdlib.h>
#include <string.h>

#include "jsj_private.h"        
#include "jsjava.h"


JSClass JavaPackage_class;      





typedef struct {
    const char * path;          
    int flags;                  
} JavaPackage_Private;

static JSObject *
define_JavaPackage(JSContext *cx, JSObject *parent_obj,
                   const char *obj_name, const char *path, int flags, int access)
{
    JSObject *package_obj, *obj;
    JavaPackage_Private *package;
    jsval v;

    






    if (!strcmp(obj_name, path) &&
        (obj = JS_GetParent(cx, parent_obj)) &&
        JS_LookupProperty(cx, obj, obj_name, &v) &&
        !JSVAL_IS_PRIMITIVE(v))
    {
        if (!JS_DefineProperty(cx, parent_obj, obj_name, v, NULL, NULL,
                               JSPROP_PERMANENT | access)) {
            return NULL;
        }

        package_obj = JSVAL_TO_OBJECT(v);
        return package_obj;
    }

    package_obj = JS_DefineObject(cx, parent_obj, obj_name, &JavaPackage_class,
                                  0, JSPROP_PERMANENT | access);
    
    if (!package_obj)
        return NULL;
    
    
    package = (JavaPackage_Private *)JS_malloc(cx, sizeof(JavaPackage_Private));
    if (!package) {
        JS_DeleteProperty(cx, parent_obj, obj_name);
        return NULL;
    }
    JS_SetPrivate(cx, package_obj, (void *)package);
    if (path)
        package->path = JS_strdup(cx, path);
    else
        package->path = "";

    package->flags = flags;

    
    if (!package->path) {
        JS_DeleteProperty(cx, parent_obj, obj_name);
        JS_free(cx, package);
        return NULL;
    }

    return package_obj;
}








JS_STATIC_DLL_CALLBACK(JSBool)
JavaPackage_setProperty(JSContext *cx, JSObject *obj, jsval slot, jsval *vp)
{
    JavaPackage_Private *package = JS_GetPrivate(cx, obj);
    if (!package) {
        JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_BAD_ADD_TO_PACKAGE);
        return JS_FALSE;
    }
    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_DONT_ADD_TO_PACKAGE);
    return JS_FALSE;
}

static JSBool quiet_resolve_failure;




JS_STATIC_DLL_CALLBACK(JSBool)
JavaPackage_resolve(JSContext *cx, JSObject *obj, jsval id)
{
    JavaPackage_Private *package;
    JSBool ok = JS_TRUE;
    jclass jclazz;
    char *subPath, *newPath;
    const char *path;
    JNIEnv *jEnv;
    JSJavaThreadState *jsj_env;

    
    if (quiet_resolve_failure)
        return JS_FALSE;
                
    package = (JavaPackage_Private *)JS_GetPrivate(cx, obj);
    if (!package)
        return JS_TRUE;

    if (!JSVAL_IS_STRING(id))
        return JS_TRUE;
    subPath = JS_GetStringBytes(JSVAL_TO_STRING(id));

    






    if (!strcmp(subPath, "toString"))
        return JS_FALSE;

    path = package->path;
    newPath = JS_smprintf("%s%s%s", path, (path[0] ? "/" : ""), subPath);
    if (!newPath) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    jsj_env = jsj_EnterJava(cx, &jEnv);
    if (!jEnv) {
        ok = JS_FALSE;
        goto out;
    }

    























    jclazz = (*jEnv)->FindClass(jEnv, newPath);
    if (jclazz) {
        JSObject *newClass;

        newClass = jsj_define_JavaClass(cx, jEnv, obj, subPath, jclazz);
        (*jEnv)->DeleteLocalRef(jEnv, jclazz);
        if (!newClass) {
            ok = JS_FALSE;
            goto out;
        }
    } else {

        


        (*jEnv)->ExceptionClear(jEnv);

        




        if (JS_InstanceOf(cx, obj, &JavaPackage_class, NULL)) {
            JavaPackage_Private *current_package;

            current_package = JS_GetPrivate(cx, obj);
            if (current_package->flags & PKG_SYSTEM) {
                char *msg, *cp;
                msg = JS_strdup(cx, newPath);

                
                if (msg) {
                    
                    for (cp = msg; *cp != '\0'; cp++)
                        if (*cp == '/')
                            *cp = '.';
                    JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL, 
                                                JSJMSG_MISSING_PACKAGE, msg);
                    free((char*)msg);
                }
                               
                ok = JS_FALSE;
                goto out;
            }
        }

        if (!define_JavaPackage(cx, obj, subPath, newPath, 0, JSPROP_READONLY)) {
            ok = JS_FALSE;
            goto out;
        }
        
#ifdef DEBUG
        
#endif

    }
    
out:
    JS_smprintf_free(newPath);
    jsj_ExitJava(jsj_env);
    return ok;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaPackage_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    JSString *str;
    char *name, *cp;

    JavaPackage_Private *package = JS_GetPrivate(cx, obj);
    if (!package) {
        fprintf(stderr, "JavaPackage_resolve: no private data!\n");
        return JS_FALSE;
    }

    switch (type) {

    
    case JSTYPE_VOID:   
    case JSTYPE_NUMBER:
    case JSTYPE_STRING:
        
        if (!package->path)
            break;
        name = JS_smprintf("[JavaPackage %s]", package->path);
        if (!name) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
        for (cp = name; *cp != '\0'; cp++)
            if (*cp == '/')
                *cp = '.';
        str = JS_NewString(cx, name, strlen(name));
        if (!str) {
            JS_smprintf_free(name);
            

            return JS_FALSE;
        }

        *vp = STRING_TO_JSVAL(str);
        break;

    case JSTYPE_OBJECT:
        *vp = OBJECT_TO_JSVAL(obj);
        break;

    default:
        break;
    }
    return JS_TRUE;
}




JS_STATIC_DLL_CALLBACK(void)
JavaPackage_finalize(JSContext *cx, JSObject *obj)
{
    JavaPackage_Private *package = JS_GetPrivate(cx, obj);
    if (!package)
        return;

    if (package->path)
        JS_free(cx, (char *)package->path);
    JS_free(cx, package);
}




JSClass JavaPackage_class = {
    "JavaPackage", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JavaPackage_setProperty,
    JS_EnumerateStub, JavaPackage_resolve,
    JavaPackage_convert, JavaPackage_finalize,

    
    NULL,                       
    NULL,                       
    NULL,                       
    NULL,                       
    NULL,                       
    NULL,                       
    NULL,                       
    0,                          
};

JavaPackageDef
standard_java_packages[] = {
    {"java",                NULL,   PKG_USER,   0},
    {"java.applet",         NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.awt",            NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.awt.datatransfer",                       
                            NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.awt.event",      NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.awt.image",      NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.awt.peer",       NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.beans",          NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.io",             NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.lang",           NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.lang.reflect",   NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.math",           NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.net",            NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.rmi",            NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.rmi.dgc",        NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.rmi.user",       NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.rmi.registry",   NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.rmi.server",     NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.security",       NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.security.acl",   NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.security.interfaces",
                            NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.sql",            NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.text",           NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.text.resources", NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"java.util",           NULL,   PKG_USER,   JSPROP_READONLY},
    {"java.util.zip",       NULL,   PKG_SYSTEM, JSPROP_READONLY},

    {"netscape",            NULL,   PKG_USER,   0},
    {"netscape.applet",     NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.application",NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.debug",      NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.javascript", NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.ldap",       NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.misc",       NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.net",        NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.plugin",     NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.util",       NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.secfile",    NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.security",   NULL,   PKG_SYSTEM, JSPROP_READONLY},
    {"netscape.WAI",        NULL,   PKG_SYSTEM, JSPROP_READONLY},

    {"sun",                 NULL,   PKG_USER,   0},
    {"Packages",            "",     PKG_USER,   JSPROP_READONLY},

    {NULL,                  NULL,   0,          0}
};





#if HAVE_STRTOK_R
# define STRTOK_1ST(str, seps, res) strtok_r (str, seps, &res)
# define STRTOK_OTHER(seps, res) strtok_r (res, seps, &res)
#else
# define STRTOK_1ST(str, seps, res) strtok (str, seps)
# define STRTOK_OTHER(seps, res) strtok (NULL, seps)
#endif







static JSBool
pre_define_java_packages(JSContext *cx, JSObject *global_obj,
                         JavaPackageDef *predefined_packages)
{
    JSBool package_exists;
    JSObject *parent_obj;
    JavaPackageDef *package_def;
    char *simple_name, *cp, *package_name, *path;
    int flags;

    if (!predefined_packages)
        return JS_TRUE;

    
    for (package_def = predefined_packages; package_def->name; package_def++) {
#if HAVE_STRTOK_R
	char *nextstr;
#endif
        package_name = path = NULL;

        parent_obj = global_obj;
        package_name = strdup(package_def->name);
        if (!package_name)
            goto out_of_memory;

        

        for (simple_name = STRTOK_1ST(package_name, ".", nextstr); simple_name; simple_name = STRTOK_OTHER(".", nextstr)) {
            jsval v;

            
            quiet_resolve_failure = JS_TRUE;
            package_exists = JS_LookupProperty(cx, parent_obj, simple_name, &v) && JSVAL_IS_OBJECT(v);
            quiet_resolve_failure = JS_FALSE;

            if (package_exists) {
                parent_obj = JSVAL_TO_OBJECT(v);
                continue;
            }

            

            if (STRTOK_OTHER(".", nextstr)) {
                JS_ReportErrorNumber(cx, jsj_GetErrorMessage, NULL,
                                JSJMSG_BAD_PACKAGE_PREDEF,
                               package_def->name);
                goto error;
            }
            
            if (package_def->path) {
                path = strdup(package_def->path);
                if (!path)
                    goto out_of_memory;
            } else {
                



                path = strdup(package_def->name);
                if (!path)
                    goto out_of_memory;
                
                for (cp = path; *cp != '\0'; cp++) {
                    if (*cp == '.')
                         *cp = '/';
                }
            }
            flags = package_def->flags;
            parent_obj = define_JavaPackage(cx, parent_obj, simple_name, path, flags,
                                            package_def->access);
            if (!parent_obj)
                goto error;
 
            free(path);
            break;
        }
        free(package_name);
    }
    return JS_TRUE;

out_of_memory:
    JS_ReportOutOfMemory(cx);

error:
    JS_FREE_IF(cx, package_name);
    JS_FREE_IF(cx, path);
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
JavaPackage_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval)
{
    if (!JS_InstanceOf(cx, obj, &JavaPackage_class, argv))
        return JS_FALSE;
    return JavaPackage_convert(cx, obj, JSTYPE_STRING, rval);
}

static JSFunctionSpec JavaPackage_methods[] = {
    {"toString",   JavaPackage_toString,        0,      0,      0},
    {0, 0, 0, 0, 0},
};





JSBool
jsj_init_JavaPackage(JSContext *cx, JSObject *global_obj,
                     JavaPackageDef *additional_predefined_packages) {

    
    if (!JS_InitClass(cx, global_obj, 0, &JavaPackage_class,
                      0, 0, 0, JavaPackage_methods, 0, 0))
        return JS_FALSE;

    
    if (!pre_define_java_packages(cx, global_obj, standard_java_packages))
        return JS_FALSE;
    if (!pre_define_java_packages(cx, global_obj, additional_predefined_packages))
        return JS_FALSE;
    
    return JS_TRUE;
}
