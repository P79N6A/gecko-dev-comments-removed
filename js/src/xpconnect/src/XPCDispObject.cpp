









































#include "xpcprivate.h"
#include "nsIActiveXSecurityPolicy.h"




const nsID NSID_IDISPATCH = { 0x00020400, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

PRBool
XPCDispObject::WrapIDispatch(IDispatch *pDispatch, XPCCallContext &ccx,
                             JSObject *obj, jsval *rval)
{
    if(!pDispatch)
    {
        return PR_FALSE;
    }

    
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = ccx.GetXPConnect()->WrapNative(
        ccx, obj, reinterpret_cast<nsISupports*>(pDispatch), NSID_IDISPATCH,
        getter_AddRefs(holder));
    if(NS_FAILED(rv) || !holder)
    {
        return PR_FALSE;
    }
    JSObject * jsobj;
    if(NS_FAILED(holder->GetJSObject(&jsobj)))
        return PR_FALSE;
    *rval = OBJECT_TO_JSVAL(jsobj);
    return PR_TRUE;
}

HRESULT XPCDispObject::SecurityCheck(XPCCallContext & ccx, const CLSID & aCID,
                                     IDispatch ** createdObject)
{
    nsresult rv;
    nsCOMPtr<nsIDispatchSupport> dispSupport = do_GetService(NS_IDISPATCH_SUPPORT_CONTRACTID, &rv);
    if(NS_FAILED(rv)) return E_UNEXPECTED;

    PRUint32 hostingFlags = nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_NOTHING;
    dispSupport->GetHostingFlags(nsnull, &hostingFlags);
    PRBool allowSafeObjects;
    if(hostingFlags & (nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_SAFE_OBJECTS))
        allowSafeObjects = PR_TRUE;
    else
        allowSafeObjects = PR_FALSE;
    PRBool allowAnyObjects;
    if(hostingFlags & (nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_ALL_OBJECTS))
        allowAnyObjects = PR_TRUE;
    else
        allowAnyObjects = PR_FALSE;
    if(!allowSafeObjects && !allowAnyObjects)
        return E_FAIL;

    PRBool classExists = PR_FALSE;
    PRBool ok = PR_FALSE;
    const nsCID & ourCID = XPCDispCLSID2nsCID(aCID);
    dispSupport->IsClassSafeToHost(ccx, ourCID, PR_FALSE, &classExists, &ok);
    if(classExists && !ok)
        return E_FAIL;

    
    PRBool isScriptable = PR_FALSE;
    if(!allowAnyObjects)
    {
        PRBool classExists = PR_FALSE;
        dispSupport->IsClassMarkedSafeForScripting(ourCID, &classExists, &isScriptable);
        if(!classExists)
            return REGDB_E_CLASSNOTREG;
    }

    
    CComPtr<IDispatch> disp;
    
    if (createdObject)
    {
        HRESULT hr = disp.CoCreateInstance(aCID);
        if(FAILED(hr))
            return hr;
        
        
        if (!allowAnyObjects && !isScriptable)
        {
            dispSupport->IsObjectSafeForScripting(disp, NSID_IDISPATCH, &isScriptable);
            if(!isScriptable)
                return E_FAIL;
        }
        disp.CopyTo(createdObject);
    }

    return S_OK;
}

HRESULT XPCDispObject::COMCreateInstance(XPCCallContext & ccx, BSTR className,
                                         PRBool enforceSecurity,
                                         IDispatch ** result)
{
    NS_ENSURE_ARG_POINTER(result);
    
    _bstr_t bstrName(className);
    CLSID classID = CLSID_NULL;
    HRESULT hr = CLSIDFromString(bstrName, &classID);
    if(FAILED(hr))
        hr = CLSIDFromProgID(bstrName, &classID);
    if(FAILED(hr) || ::IsEqualCLSID(classID, CLSID_NULL))
        return hr;
    
    
    
    
    if(enforceSecurity)
        return SecurityCheck(ccx, classID, result);
    
    CComPtr<IDispatch> disp;
    hr = disp.CoCreateInstance(classID);
    if(FAILED(hr))
        return hr;

    disp.CopyTo(result);

    return S_OK;
}


JSBool XPCDispObject::Dispatch(XPCCallContext& ccx, IDispatch * disp,
                               DISPID dispID, CallMode mode, 
                               XPCDispParams * params,
                               jsval* retval,
                               XPCDispInterface::Member * member,
                               XPCJSRuntime* rt)
{
    _variant_t dispResult;
    jsval val;
    uintN err;
    uintN argc = params->GetParamCount();
    
    WORD dispFlags;
    if(mode == CALL_SETTER)
    {
        dispFlags = DISPATCH_PROPERTYPUT;
    }
    else if(mode == CALL_GETTER)
    {
        dispFlags = DISPATCH_PROPERTYGET;
    }
    else
    {
        dispFlags = DISPATCH_METHOD;
    }
    HRESULT invokeResult;
    EXCEPINFO exception;
    
    {
        
        AutoJSSuspendRequest req(ccx);  
        
        invokeResult= disp->Invoke(
            dispID,                  
            IID_NULL,                
            LOCALE_SYSTEM_DEFAULT,   
            dispFlags,               
            params->GetDispParams(), 
            &dispResult,             
            &exception,              
            0);                      
    }
    if(SUCCEEDED(invokeResult))
    {
        *retval = JSVAL_VOID;
        if(mode == CALL_METHOD)
        {
            NS_ASSERTION(member, "member must not be null if this is a method");
            for(PRUint32 index = 0; index < argc; ++index)
            {
                const XPCDispInterface::Member::ParamInfo & paramInfo = member->GetParamInfo(index);
                if(paramInfo.IsOut())
                {
                    if(!XPCDispConvert::COMToJS(ccx, params->GetParamRef(index), val, err))
                        return ThrowBadParam(err, index, ccx);

                    if(paramInfo.IsRetVal())
                    {
                        *retval = val;
                    }
                    else
                    {
                        jsval * argv = ccx.GetArgv();
                        
                        if(!JSVAL_IS_OBJECT(argv[index]) ||
                            !JS_SetPropertyById(ccx, JSVAL_TO_OBJECT(argv[index]),
                                rt->GetStringID(XPCJSRuntime::IDX_VALUE), &val))
                            return ThrowBadParam(NS_ERROR_XPC_CANT_SET_OUT_VAL, index, ccx);
                    }
                }
            }
        }
        if(dispResult.vt != VT_EMPTY)
        {
            if(!XPCDispConvert::COMToJS(ccx, dispResult, val, err))
            {
                ThrowBadParam(err, 0, ccx);
            }
            *retval = val;
        }
    }
    
    ccx.GetXPCContext()->SetLastResult(invokeResult);

    if(NS_FAILED(invokeResult))
    {
        XPCThrower::ThrowCOMError(ccx, invokeResult, NS_ERROR_XPC_COM_ERROR, 
                                  invokeResult == DISP_E_EXCEPTION ? 
                                      &exception : nsnull);
        return JS_FALSE;
    }
    return JS_TRUE;
}

JSBool XPCDispObject::Invoke(XPCCallContext & ccx, CallMode mode)
{
    nsresult rv = ccx.CanCallNow();
    if(NS_FAILED(rv))
    {
        
        
        NS_ASSERTION(rv == NS_ERROR_XPC_SECURITY_MANAGER_VETO, 
                     "hmm? CanCallNow failed in XPCDispObject::Invoke. "
                     "We are finding out about this late!");
        XPCThrower::Throw(rv, ccx);
        return JS_FALSE;
    }

    
    XPCDispInterface::Member* member = reinterpret_cast<XPCDispInterface::Member*>(ccx.GetIDispatchMember());
    XPCJSRuntime* rt = ccx.GetRuntime();
    XPCContext* xpcc = ccx.GetXPCContext();
    XPCPerThreadData* tls = ccx.GetThreadData();
    
    jsval* argv = ccx.GetArgv();
    uintN argc = ccx.GetArgc();

    tls->SetException(nsnull);
    xpcc->SetLastResult(NS_ERROR_UNEXPECTED);

    

    PRUint32 secFlag;
    PRUint32 secAction;

    switch(mode)
    {
        case CALL_METHOD:
            secFlag   = nsIXPCSecurityManager::HOOK_CALL_METHOD;
            secAction = nsIXPCSecurityManager::ACCESS_CALL_METHOD;
            break;
        case CALL_GETTER:
            secFlag   = nsIXPCSecurityManager::HOOK_GET_PROPERTY;
            secAction = nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
            break;
        case CALL_SETTER:
            secFlag   = nsIXPCSecurityManager::HOOK_SET_PROPERTY;
            secAction = nsIXPCSecurityManager::ACCESS_SET_PROPERTY;
            break;
        default:
            NS_ERROR("bad value");
            return JS_FALSE;
    }
    jsval name = member->GetName();

    nsIXPCSecurityManager* sm = xpcc->GetAppropriateSecurityManager(secFlag);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(sm && NS_FAILED(sm->CanAccess(secAction, &ccx, ccx,
                                     ccx.GetFlattenedJSObject(),
                                     wrapper->GetIdentityObject(),
                                     wrapper->GetClassInfo(), name,
                                     wrapper->GetSecurityInfoAddr())))
    {
        
        return JS_FALSE;
    }

    IDispatch * pObj = reinterpret_cast<IDispatch*>
                                       (ccx.GetTearOff()->GetNative());
    PRUint32 args = member->GetParamCount();
    uintN err;
    
    if(mode == CALL_SETTER)
        args = 1;
    
    
    if(argc < args)
        args = argc;
    XPCDispParams * params = new XPCDispParams(args);
    jsval val;
    
    if(mode == CALL_SETTER)
    {
        params->SetNamedPropID();
        if(!XPCDispConvert::JSToCOM(ccx, argv[0], params->GetParamRef(0), err))
        {
            delete params;
            return ThrowBadParam(err, 0, ccx);
        }
    }
    else if(mode != CALL_GETTER)    
    {
        
        for(PRUint32 index = 0; index < args; ++index)
        {
            const XPCDispInterface::Member::ParamInfo & paramInfo = member->GetParamInfo(index);
            if(paramInfo.IsIn())
            {
                val = argv[index];
                if(paramInfo.IsOut())
                {
                    if(JSVAL_IS_PRIMITIVE(val) ||
                        !JS_GetPropertyById(ccx, JSVAL_TO_OBJECT(val),
                            rt->GetStringID(XPCJSRuntime::IDX_VALUE), &val))
                    {
                        delete params;
                        return ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, index, ccx);
                    }
                    paramInfo.InitializeOutputParam(params->GetOutputBuffer(index), params->GetParamRef(index));
                }
                if(!XPCDispConvert::JSToCOM(ccx, val, params->GetParamRef(index), err, paramInfo.IsOut()))
                {
                    delete params;
                    return ThrowBadParam(err, index, ccx);
                }
            }
            else
            {
                paramInfo.InitializeOutputParam(params->GetOutputBuffer(index), params->GetParamRef(index));
            }
        }
    }
    
    if(member->IsParameterizedProperty())
    {
        
        
        if(XPCDispParamPropJSClass::NewInstance(ccx, wrapper,
                                                member->GetDispID(),
                                                params, &val))
        {
            ccx.SetRetVal(val);
            if(!JS_IdToValue(ccx, 1, &val))
            {
                
                NS_ERROR("JS_IdToValue failed in XPCDispParamPropJSClass::NewInstance");
                return JS_FALSE;
            }
            JS_SetCallReturnValue2(ccx, val);
            return JS_TRUE;
        }
        
        JS_ReportOutOfMemory(ccx);
        delete params;
        return JS_FALSE;
    }
    JSBool retval = Dispatch(ccx, pObj, member->GetDispID(), mode, params, &val, member, rt);
    if(retval && mode == CALL_SETTER)
    {
        ccx.SetRetVal(argv[0]);
    }
    else
    {
        ccx.SetRetVal(val);
    }
    delete params;
    return retval;
}

static
JSBool GetMember(XPCCallContext& ccx, JSObject* funobj, XPCNativeInterface*& iface, XPCDispInterface::Member*& member)
{
    jsval val;
    if(!JS_GetReservedSlot(ccx, funobj, 1, &val))
        return JS_FALSE;
    if(!JSVAL_IS_INT(val))
        return JS_FALSE;
    iface = reinterpret_cast<XPCNativeInterface*>(JSVAL_TO_PRIVATE(val));
    if(!JS_GetReservedSlot(ccx, funobj, 0, &val))
        return JS_FALSE;
    if(!JSVAL_IS_INT(val))
        return JS_FALSE;
    member = reinterpret_cast<XPCDispInterface::Member*>(JSVAL_TO_PRIVATE(val));
    return JS_TRUE;
}


#define THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper)                         \
    PR_BEGIN_MACRO                                                           \
    if(!wrapper)                                                             \
    {                                                                        \
        XPCThrower::Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);              \
        return JS_FALSE;                                                     \
    }                                                                        \
    if(!wrapper->IsValid())                                                  \
    {                                                                        \
        XPCThrower::Throw(NS_ERROR_XPC_HAS_BEEN_SHUTDOWN, cx);               \
        return JS_FALSE;                                                     \
    }                                                                        \
    PR_END_MACRO












JSBool
XPC_IDispatch_CallMethod(JSContext* cx, JSObject* obj, uintN argc,
                         jsval* argv, jsval* vp)
{
    NS_ASSERTION(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION, "bad function");
    JSObject* funobj = JSVAL_TO_OBJECT(argv[-2]);
    XPCCallContext ccx(JS_CALLER, cx, obj, funobj, 0, argc, argv, vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);
    ccx.SetArgsAndResultPtr(argc, argv, vp);

    XPCDispInterface::Member* member;
    XPCNativeInterface* iface;
#ifdef DEBUG
    PRBool ok =
#endif
    GetMember(ccx, funobj, iface, member);
    NS_ASSERTION(ok, "GetMember faild in XPC_IDispatch_CallMethod");
    ccx.SetIDispatchInfo(iface, member);

    return XPCDispObject::Invoke(ccx, XPCDispObject::CALL_METHOD);
}












JSBool
XPC_IDispatch_GetterSetter(JSContext *cx, JSObject *obj, uintN argc,
                           jsval *argv, jsval *vp)
{
    NS_ASSERTION(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION, "bad function");
    JSObject* funobj = JSVAL_TO_OBJECT(argv[-2]);

    XPCCallContext ccx(JS_CALLER, cx, obj, funobj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    ccx.SetArgsAndResultPtr(argc, argv, vp);
    XPCDispInterface::Member* member;
    XPCNativeInterface* iface;
#ifdef DEBUG
    PRBool ok =
#endif
    GetMember(ccx, funobj, iface, member);
    NS_ASSERTION(ok, "GetMember faild in XPC_IDispatch_CallMethod");

    ccx.SetIDispatchInfo(iface, member);
    return XPCDispObject::Invoke(ccx, argc != 0 ? XPCDispObject::CALL_SETTER : XPCDispObject::CALL_GETTER);
}
