




































#include "stdafx.h"

#include "jni.h"
#include "npapi.h"

#include "_java/java_lang_Throwable.h"
#include "_java/java_lang_Error.h"
#include "_java/java_lang_String.h"
#include "_java/java_lang_Boolean.h"
#include "_java/java_lang_Number.h"
#include "_java/java_lang_Integer.h"
#include "_java/java_lang_Long.h"





#include "_java/java_lang_Character.h"
#include "_java/netscape_plugin_Plugin.h"
#include "_java/MozAxPlugin.h"

#include "LegacyPlugin.h"
#include "LiveConnect.h"

void liveconnect_Shutdown()
{
    JRIEnv* env = NPN_GetJavaEnv();
    if (env) {
        unuse_MozAxPlugin(env);
        unuse_netscape_plugin_Plugin(env);
        unuse_java_lang_Error(env);

        unuse_java_lang_Number(env);
        unuse_java_lang_Boolean(env);
        unuse_java_lang_Integer(env);
        unuse_java_lang_Long(env);


        unuse_java_lang_Character(env);
    }
}

jref liveconnect_GetJavaClass()
{
    JRIEnv* env = NPN_GetJavaEnv();
    if (env) {
        
        
        
        use_netscape_plugin_Plugin(env);
        jref myClass = (jref) use_MozAxPlugin(env);
        use_java_lang_Error(env);

        use_java_lang_Number(env);
        use_java_lang_Boolean(env);
        use_java_lang_Integer(env);
        use_java_lang_Long(env);


        use_java_lang_Character(env);
        return myClass;
    }
    return NULL;
}


HRESULT
_GetIDispatchFromJRI(JRIEnv *env, struct MozAxPlugin* self, IDispatch **pdisp)
{
    if (pdisp == NULL || env == NULL || self == NULL)
    {
        return E_INVALIDARG;
    }
    *pdisp = NULL;

    
    
    
    

    NPP npp = (NPP) netscape_plugin_Plugin_getPeer(env,
            reinterpret_cast<netscape_plugin_Plugin*> (self));
    PluginInstanceData *pData = (PluginInstanceData *) npp->pdata;
    if (pData == NULL)
    { 
        return E_FAIL;
    }

    IUnknownPtr unk;
    HRESULT hr = pData->pControlSite->GetControlUnknown(&unk);
    if (unk.GetInterfacePtr() == NULL)
    {
        return E_FAIL; 
    }

    IDispatchPtr disp = unk;
    if (disp.GetInterfacePtr() == NULL)
    { 
        return E_FAIL; 
    }

    *pdisp = disp.GetInterfacePtr();
    (*pdisp)->AddRef();

    return S_OK;
}

HRESULT
_VariantToJRIObject(JRIEnv *env, VARIANT *v, java_lang_Object **o)
{
    if (v == NULL || env == NULL || o == NULL)
    {
        return E_INVALIDARG;
    }

    *o = NULL;

    
    if (v->vt == VT_EMPTY)
    {
        return S_OK;
    }
    else if (v->vt == VT_BOOL)
    {
        jbool value = (v->boolVal == VARIANT_TRUE) ? JRITrue : JRIFalse;
        java_lang_Boolean *j = java_lang_Boolean_new(env,
            class_java_lang_Boolean(env), value);
        *o = reinterpret_cast<java_lang_Object *>(j);
        return S_OK;
    }
    else if (v->vt == VT_I4)
    {
        jlong value = v->lVal;
        java_lang_Long *j = java_lang_Long_new(env,
            class_java_lang_Long(env), value);
        *o = reinterpret_cast<java_lang_Object *>(j);
        return S_OK;
    }
    else if (v->vt == VT_I2)
    {
        jlong value = v->iVal;
        java_lang_Long *j = java_lang_Long_new(env,
            class_java_lang_Long(env), value);
        *o = reinterpret_cast<java_lang_Object *>(j);
        return S_OK;
    }
















    else if (v->vt == VT_BSTR)
    {
        USES_CONVERSION;
        char * value = OLE2A(v->bstrVal);
        java_lang_String *j = JRI_NewStringUTF(env, value, strlen(value));
        *o = reinterpret_cast<java_lang_Object *>(j);
        return S_OK;
    }
    

    return E_FAIL;
}

HRESULT
_JRIObjectToVariant(JRIEnv *env, java_lang_Object *o, VARIANT *v)
{
    VariantInit(v);
    if (JRI_IsInstanceOf(env, (jref) o, class_java_lang_String(env)))
    {
        USES_CONVERSION;
        const char *value = JRI_GetStringUTFChars(env, reinterpret_cast<java_lang_String *>(o));
        v->vt = VT_BSTR;
        v->bstrVal = SysAllocString(A2COLE(value));
    }
    else if (JRI_IsInstanceOf(env, (jref) o, class_java_lang_Boolean(env)))
    {
        jbool value = java_lang_Boolean_booleanValue(env, reinterpret_cast<java_lang_Boolean *>(o));
        v->vt = VT_BOOL;
        v->boolVal = value == JRITrue ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else if (JRI_IsInstanceOf(env, o, class_java_lang_Integer(env)))
    {
        jint value = java_lang_Integer_intValue(env, reinterpret_cast<java_lang_Integer *>(o));
        v->vt = VT_I4;
        v->lVal = value;
    }
    else if (JRI_IsInstanceOf(env, o, class_java_lang_Long(env)))
    {
        jlong value = java_lang_Long_longValue(env, reinterpret_cast<java_lang_Long *>(o));
        v->vt = VT_I4;
        v->lVal = value;
    }












    else if (JRI_IsInstanceOf(env, o, class_java_lang_Character(env)))
    {
        jchar value = java_lang_Character_charValue(env, reinterpret_cast<java_lang_Character *>(o));
        v->vt = VT_UI1;
        v->bVal = value;
    }
    else if (JRI_IsInstanceOf(env, o, class_java_lang_Number(env)))
    {
        jlong value = java_lang_Number_longValue(env, reinterpret_cast<java_lang_Number *>(o));
        v->vt = VT_I4;
        v->lVal = value;
    }
    else
    {
        
        return E_FAIL;
    }
    return S_OK;
}

struct java_lang_Object *
_InvokeFromJRI(JRIEnv *env, struct MozAxPlugin* self, struct java_lang_String *func, int nargs, java_lang_Object *args[])
{
    HRESULT hr;
    DISPID dispid = 0;
    
    
    const char* funcName = JRI_GetStringUTFChars(env, func);

    IDispatchPtr disp;
    if (FAILED(_GetIDispatchFromJRI(env, self, &disp)))
    {
        return NULL;
    }

    _variant_t *vargs = new _variant_t[nargs];
    for (int i = 0; i < nargs; i++)
    {
        if (FAILED(_JRIObjectToVariant(env, args[i], &vargs[i])))
        {
            delete []vargs;
            char error[64];
            sprintf(error, "Argument %d could not be converted into a variant", i);
            JRI_ThrowNew(env, class_java_lang_Error(env), error); 
            return NULL;
        }
    }

    USES_CONVERSION;
    OLECHAR FAR* szMember = A2OLE(funcName);
    hr = disp->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr))
    { 
        char error[128];
        memset(error, 0, sizeof(error));
        _snprintf(error, sizeof(error) - 1, "invoke failed, member \"%s\" not found, hr=0x%08x", funcName, hr);
        JRI_ThrowNew(env, class_java_lang_Error(env), error); 
        return NULL; 
    }

    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    dispparams.rgvarg = vargs;
    dispparams.cArgs = nargs;

    _variant_t vResult;
    hr = disp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_METHOD,
        &dispparams, &vResult, NULL, NULL);
    
    if (FAILED(hr))
    { 
        char error[64];
        sprintf(error, "invoke failed, result from object = 0x%08x", hr);
        JRI_ThrowNew(env, class_java_lang_Error(env), error); 
        return NULL; 
    }

    java_lang_Object *oResult = NULL;
    _VariantToJRIObject(env, &vResult, &oResult);

    return reinterpret_cast<java_lang_Object *>(oResult);
}







extern "C" JRI_PUBLIC_API(struct java_lang_Object *)
native_MozAxPlugin_xinvoke(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a)
{
    return _InvokeFromJRI(env, self, a, 0, NULL);
}


extern "C" JRI_PUBLIC_API(struct java_lang_Object *)
native_MozAxPlugin_xinvoke1(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a, struct java_lang_Object *b)
{
    java_lang_Object *args[1];
    args[0] = b;
    return _InvokeFromJRI(env, self, a, sizeof(args) / sizeof(args[0]), args);
}


extern "C" JRI_PUBLIC_API(struct java_lang_Object *)
native_MozAxPlugin_xinvoke2(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a, struct java_lang_Object *b, struct java_lang_Object *c)
{
    java_lang_Object *args[2];
    args[0] = b;
    args[1] = c;
    return _InvokeFromJRI(env, self, a, sizeof(args) / sizeof(args[0]), args);
}


extern "C" JRI_PUBLIC_API(struct java_lang_Object *)
native_MozAxPlugin_xinvoke3(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a, struct java_lang_Object *b, struct java_lang_Object *c, struct java_lang_Object *d)
{
    java_lang_Object *args[3];
    args[0] = b;
    args[1] = c;
    args[2] = d;
    return _InvokeFromJRI(env, self, a, sizeof(args) / sizeof(args[0]), args);
}


extern "C" JRI_PUBLIC_API(struct java_lang_Object *)
native_MozAxPlugin_xinvoke4(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a, struct java_lang_Object *b, struct java_lang_Object *c, struct java_lang_Object *d, struct java_lang_Object *e)
{
    java_lang_Object *args[4];
    args[0] = b;
    args[1] = c;
    args[2] = d;
    args[3] = e;
    return _InvokeFromJRI(env, self, a, sizeof(args) / sizeof(args[0]), args);
}


extern "C" JRI_PUBLIC_API(struct java_lang_Object *)
native_MozAxPlugin_xinvokeX(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a, jobjectArray b)
{
    
    jsize length = JRI_GetObjectArrayLength(env, b);
    java_lang_Object **args = NULL;
    if (length)
    {
        args = (java_lang_Object **) malloc(length * sizeof(java_lang_Object *));
        for (long i = 0; i < length; i++)
        {
             args[i] = reinterpret_cast<java_lang_Object *>(JRI_GetObjectArrayElement(env, b, i));
        }
    }
    java_lang_Object *o = _InvokeFromJRI(env, self, a, length, args);
    free(args);
    return o;
}


extern "C" JRI_PUBLIC_API(struct java_lang_Object *)
native_MozAxPlugin_xgetProperty(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a)
{
    HRESULT hr;
    DISPID dispid;
    _variant_t vResult;

    IDispatchPtr disp;
    if (FAILED(_GetIDispatchFromJRI(env, self, &disp)))
    {
        return NULL;
    }

    
    USES_CONVERSION;
    OLECHAR FAR* szMember = A2OLE(JRI_GetStringUTFChars(env, a));
    hr = disp->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr))
    { 
        char error[128];
        memset(error, 0, sizeof(error));
        _snprintf(error, sizeof(error) - 1, "getProperty failed, member \"%s\" not found, hr=0x%08x", szMember, hr);
        JRI_ThrowNew(env, class_java_lang_Error(env), error); 
        return NULL; 
    }
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    hr = disp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYGET,
        &dispparamsNoArgs, &vResult, NULL, NULL);
    if (FAILED(hr))
    { 
        char error[64];
        sprintf(error, "getProperty failed, result from object = 0x%08x", hr);
        JRI_ThrowNew(env, class_java_lang_Error(env), error); 
        return NULL; 
    }

    java_lang_Object *oResult = NULL;
    _VariantToJRIObject(env, &vResult, &oResult);

    return oResult;
}


extern "C" JRI_PUBLIC_API(void)
native_MozAxPlugin_xsetProperty2(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a, struct java_lang_Object *b)

{
    HRESULT hr;
    DISPID dispid;
    VARIANT VarResult;

    IDispatchPtr disp;
    if (FAILED(_GetIDispatchFromJRI(env, self, &disp)))
    {
        return;
    }

    USES_CONVERSION;
    OLECHAR FAR* szMember = A2OLE(JRI_GetStringUTFChars(env, a));
    hr = disp->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr))
    { 
        char error[128];
        memset(error, 0, sizeof(error));
        _snprintf(error, sizeof(error) - 1, "setProperty failed, member \"%s\" not found, hr=0x%08x", szMember, hr);
        JRI_ThrowNew(env, class_java_lang_Error(env), error); 
        return;
    }

    _variant_t *pvars = new _variant_t[1];
    if (FAILED(_JRIObjectToVariant(env, b, &pvars[0])))
    {
        delete []pvars;
        char error[64];
        sprintf(error, "Property value could not be converted into a variant");
        JRI_ThrowNew(env, class_java_lang_Error(env), error); 
        return;
    }

    DISPID dispIdPut = DISPID_PROPERTYPUT;

    DISPPARAMS functionArgs;
    functionArgs.rgdispidNamedArgs = &dispIdPut;
    functionArgs.rgvarg = pvars;
    functionArgs.cArgs = 1;
    functionArgs.cNamedArgs = 1;

    hr = disp->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYPUT,
        &functionArgs, &VarResult, NULL, NULL);

    delete []pvars;
    
    if (FAILED(hr))
    {
        char error[64];
        sprintf(error, "setProperty failed, result from object = 0x%08x", hr);
        JRI_ThrowNew(env, class_java_lang_Error(env), error); 
        return;
    }
}


extern "C" JRI_PUBLIC_API(void)
native_MozAxPlugin_xsetProperty1(JRIEnv* env, struct MozAxPlugin* self, struct java_lang_String *a, struct java_lang_String *b)
{
    native_MozAxPlugin_xsetProperty2(env, self, a, reinterpret_cast<java_lang_Object *>(b));
}





extern "C" JRI_PUBLIC_API(void)
native_java_lang_Throwable_printStackTrace0(JRIEnv* env, struct java_lang_Throwable* self, struct java_io_PrintStream *a)
{
}


extern "C" JRI_PUBLIC_API(struct java_lang_Throwable *)
native_java_lang_Throwable_fillInStackTrace(JRIEnv* env, struct java_lang_Throwable* self)
{
    return self;
}
