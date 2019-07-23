









































#include "xpcprivate.h"








inline
XPCDispParamPropJSClass* GetParamProp(JSContext* cx, JSObject* obj)
{
    return NS_REINTERPRET_CAST(XPCDispParamPropJSClass*, 
                               JS_GetPrivate(cx, obj));
}












JS_STATIC_DLL_CALLBACK(JSBool)
XPC_PP_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    XPCDispParamPropJSClass* paramProp = GetParamProp(cx, obj);
    JSObject* originalObj = paramProp->GetWrapper()->GetFlatJSObject();
    XPCCallContext ccx(JS_CALLER, cx, originalObj, nsnull, id, 
                       paramProp->GetParams()->GetParamCount(), nsnull, vp);
    return paramProp->Invoke(ccx, XPCDispObject::CALL_GETTER, vp);
}












JS_STATIC_DLL_CALLBACK(JSBool)
XPC_PP_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    XPCDispParamPropJSClass* paramProp = GetParamProp(cx, obj);
    JSObject* originalObj = paramProp->GetWrapper()->GetFlatJSObject();
    XPCCallContext ccx(JS_CALLER, cx, originalObj, nsnull, id, 
                       paramProp->GetParams()->GetParamCount(), nsnull, vp);
    _variant_t var;
    uintN err;
    if(!XPCDispConvert::JSToCOM(ccx, *vp, var, err))
        return JS_FALSE;
    XPCDispParams* params = paramProp->GetParams();
    params->SetNamedPropID();
    
    params->InsertParam(var);
    
    
    
    jsval retJSVal = *vp;
    AUTO_MARK_JSVAL(ccx, retJSVal);
    if (paramProp->Invoke(ccx, XPCDispObject::CALL_SETTER, vp))
    {
        *vp = retJSVal;
        return JS_TRUE;
    }
    return JS_FALSE;
}












JS_STATIC_DLL_CALLBACK(void)
XPC_PP_Finalize(JSContext *cx, JSObject *obj)
{
    delete GetParamProp(cx, obj);
}






JS_STATIC_DLL_CALLBACK(void)
XPC_PP_Trace(JSTracer *trc, JSObject *obj)
{
    XPCDispParamPropJSClass* paramProp = GetParamProp(trc->context, obj);
    if(paramProp)
    {
        XPCWrappedNative* wrapper = paramProp->GetWrapper();
        if(wrapper && wrapper->IsValid())
            xpc_TraceForValidWrapper(trc, wrapper);
    }
}





static JSClass ParamPropClass = {
    "XPCDispParamPropJSCass",   
    JSCLASS_HAS_PRIVATE | JSCLASS_MARK_IS_TRACE, 

    
    JS_PropertyStub,            
    JS_PropertyStub,            
    XPC_PP_GetProperty,         
    XPC_PP_SetProperty,         
    JS_EnumerateStub,           
    JS_ResolveStub,             
    JS_ConvertStub,             
    XPC_PP_Finalize,            

    
    nsnull,                     
    nsnull,                     
    nsnull,                     
    nsnull,                     
    nsnull,                     
    nsnull,                     
    JS_CLASS_TRACE(XPC_PP_Trace), 
    nsnull                      
};


JSBool XPCDispParamPropJSClass::NewInstance(XPCCallContext& ccx,
                                             XPCWrappedNative* wrapper, 
                                             PRUint32 dispID, 
                                             XPCDispParams* dispParams, 
                                             jsval* paramPropObj)
{
    XPCDispParamPropJSClass* pDispParam =
        new XPCDispParamPropJSClass(wrapper, ccx.GetTearOff()->GetNative(), 
                                    dispID, dispParams);
    if(!pDispParam)
        return JS_FALSE;
    JSObject * obj = JS_NewObject(ccx, &ParamPropClass, nsnull, nsnull);
    if(!obj)
        return JS_FALSE;
    if(!JS_SetPrivate(ccx, obj, pDispParam))
        return JS_FALSE;
    *paramPropObj = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

XPCDispParamPropJSClass::XPCDispParamPropJSClass(XPCWrappedNative* wrapper, 
                                                 nsISupports * dispObj, 
                                                 PRUint32 dispID,
                                                 XPCDispParams* dispParams) :
    mWrapper(wrapper),
    mDispID(dispID),
    mDispParams(dispParams),
    mDispObj(nsnull)
{
    NS_ADDREF(mWrapper);
    dispObj->QueryInterface(NSID_IDISPATCH, 
                                              NS_REINTERPRET_CAST(void**,
                                              &mDispObj));
}

XPCDispParamPropJSClass::~XPCDispParamPropJSClass()
{
    delete mDispParams;
    
    NS_IF_RELEASE(mWrapper);
    NS_IF_RELEASE(mDispObj);
}
