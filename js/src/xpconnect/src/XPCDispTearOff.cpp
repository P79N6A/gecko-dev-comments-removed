








































#include "xpcprivate.h"








static HRESULT Error(HRESULT hResult, const CComBSTR & message)
{
    CComPtr<ICreateErrorInfo> pCreateError;
    CComPtr<IErrorInfo> pError;
    HRESULT result = CreateErrorInfo(&pCreateError);
    if(FAILED(result))
        return E_NOTIMPL;
    result = pCreateError->QueryInterface(&pError);
    if(FAILED(result))
        return E_NOTIMPL;
    result = pCreateError->SetDescription(message);
    if(FAILED(result))
        return E_NOTIMPL;
    result = pCreateError->SetGUID(IID_IDispatch);
    if(FAILED(result))
        return E_NOTIMPL;
    CComBSTR source(L"@mozilla.XPCDispatchTearOff");
    result = pCreateError->SetSource(source);
    if(FAILED(result))
        return E_NOTIMPL;
    result = SetErrorInfo(0, pError);
    if(FAILED(result))
        return E_NOTIMPL;
    return hResult;
}








inline
HRESULT Error(HRESULT hResult, const char * message)
{
    CComBSTR someText(message);
    return Error(hResult, someText);
}






static void BuildMessage(nsIException * exception, nsCString & result)
{
    nsXPIDLCString msg;
    exception->GetMessage(getter_Copies(msg));
    nsXPIDLCString filename;
    exception->GetFilename(getter_Copies(filename));

    PRUint32 lineNumber;
    if(NS_FAILED(exception->GetLineNumber(&lineNumber)))
        lineNumber = 0;
    result = "Error in file ";
    result += filename;
    result += ",#";
    result.AppendInt(lineNumber);
    result += " : ";
    result += msg;
}





inline
static void SetCOMError(nsIException * exception)
{
    nsCString message;
    BuildMessage(exception, message);
    Error(E_FAIL, message.get());
}

XPCDispatchTearOff::XPCDispatchTearOff(nsIXPConnectWrappedJS * wrappedJS) :
    mWrappedJS(wrappedJS),
    mCOMTypeInfo(nsnull),
    mRefCnt(0)
{
}

XPCDispatchTearOff::~XPCDispatchTearOff()
{
    NS_IF_RELEASE(mCOMTypeInfo);
}

NS_COM_IMPL_ADDREF(XPCDispatchTearOff)
NS_COM_IMPL_RELEASE(XPCDispatchTearOff)






static inline BOOL _IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
	  ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
	  ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
	  ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
	  ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

STDMETHODIMP XPCDispatchTearOff::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IDispatch,
    };

    for(int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if(_IsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP XPCDispatchTearOff::QueryInterface(const struct _GUID & guid,
                                              void ** pPtr)
{
    if(IsEqualIID(guid, IID_IDispatch))
    {
        *pPtr = NS_STATIC_CAST(IDispatch*,this);
        NS_ADDREF_THIS();
        return NS_OK;
    }

    if(IsEqualIID(guid, IID_ISupportErrorInfo))
    {
        *pPtr = NS_STATIC_CAST(IDispatch*,this);
        NS_ADDREF_THIS();
        return NS_OK;
    }

    return mWrappedJS->QueryInterface(XPCDispIID2nsIID(guid), pPtr);
}

STDMETHODIMP XPCDispatchTearOff::GetTypeInfoCount(unsigned int FAR * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

XPCDispTypeInfo * XPCDispatchTearOff::GetCOMTypeInfo()
{
    
    if(mCOMTypeInfo)
        return mCOMTypeInfo;
    
    XPCCallContext ccx(NATIVE_CALLER);
    if(!ccx.IsValid())
        return nsnull;
    JSObject* obj = GetJSObject();
    if(!obj)
        return nsnull;
    mCOMTypeInfo = XPCDispTypeInfo::New(ccx, obj);
    NS_IF_ADDREF(mCOMTypeInfo);
    return mCOMTypeInfo;
}

STDMETHODIMP XPCDispatchTearOff::GetTypeInfo(unsigned int, LCID, 
                                         ITypeInfo FAR* FAR* ppTInfo)
{
    *ppTInfo = GetCOMTypeInfo();
    NS_ADDREF(*ppTInfo);
    return S_OK;
}

STDMETHODIMP XPCDispatchTearOff::GetIDsOfNames(REFIID riid, 
                                           OLECHAR FAR* FAR* rgszNames, 
                                           unsigned int cNames, LCID  lcid,
                                           DISPID FAR* rgDispId)
{
    ITypeInfo * pTypeInfo = GetCOMTypeInfo();
    if(pTypeInfo != nsnull)
    {
        return pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
    }
    return S_OK;
}

void JS_DLL_CALLBACK
xpcWrappedJSErrorReporter(JSContext *cx, const char *message,
                          JSErrorReport *report);

STDMETHODIMP XPCDispatchTearOff::Invoke(DISPID dispIdMember, REFIID riid, 
                                        LCID lcid, WORD wFlags,
                                        DISPPARAMS FAR* pDispParams, 
                                        VARIANT FAR* pVarResult, 
                                        EXCEPINFO FAR* pExcepInfo, 
                                        unsigned int FAR* puArgErr)
{
    XPCDispTypeInfo* pTypeInfo = GetCOMTypeInfo();
    if(!pTypeInfo)
    {
        return E_FAIL;
    }
    XPCCallContext ccx(NATIVE_CALLER);
    XPCContext* xpcc;
    JSContext* cx;
    if(ccx.IsValid())
    {
        xpcc = ccx.GetXPCContext();
        cx = ccx.GetJSContext();
    }
    else
    {
        xpcc = nsnull;
        cx = nsnull;
    }
    
    
    NS_LossyConvertUTF16toASCII name(pTypeInfo->GetNameForDispID(dispIdMember));
    if(name.IsEmpty())
        return E_FAIL;
    
    PRBool getter = (wFlags & DISPATCH_PROPERTYGET) != 0;
    PRBool setter = (wFlags & DISPATCH_PROPERTYPUT) != 0;
    
    if(getter || setter)
    {
        jsval val;
        uintN err;
        JSObject* obj;
        if(getter)
        {
            
            obj = GetJSObject();
            if(!obj)
                return E_FAIL;
            if(!JS_GetProperty(cx, obj, name.get(), &val))
            {
                nsCString msg("Unable to retrieve property ");
                msg += name;
                return Error(E_FAIL, msg.get());
            }
            if(!XPCDispConvert::JSToCOM(ccx, val, *pVarResult, err))
            {
                nsCString msg("Failed to convert value from JS property ");
                msg += name;
                return Error(E_FAIL, msg.get());
            }
        }
        else if(pDispParams->cArgs > 0)
        {
            
            if(!XPCDispConvert::COMToJS(ccx, pDispParams->rgvarg[0], val, err))
            {
                nsCString msg("Failed to convert value for JS property ");
                msg += name;
                return Error(E_FAIL, msg.get());
            }
            AUTO_MARK_JSVAL(ccx, &val);
            obj = GetJSObject();
            if(!obj)
                return Error(E_FAIL, "The JS wrapper did not return a JS object");
            if(!JS_SetProperty(cx, obj, name.get(), &val))
            {
                nsCString msg("Unable to set property ");
                msg += name;
                return Error(E_FAIL, msg.get());
            }
        }
    }
    else 
    {
        jsval* stackbase;
        jsval* sp = nsnull;
        uint8 i;
        uint8 argc = pDispParams->cArgs;
        uint8 stack_size;
        jsval result;
        uint8 paramCount=0;
        nsresult retval = NS_ERROR_FAILURE;
        nsresult pending_result = NS_OK;
        JSBool success;
        JSBool readyToDoTheCall = JS_FALSE;
        uint8 outConversionFailedIndex;
        JSObject* obj;
        jsval fval;
        nsCOMPtr<nsIException> xpc_exception;
        void* mark;
        JSBool foundDependentParam;
        JSObject* thisObj;
        AutoScriptEvaluate scriptEval(ccx);
        XPCJSRuntime* rt = ccx.GetRuntime();
        int j;

        thisObj = obj = GetJSObject();;

        if(!cx || !xpcc)
            goto pre_call_clean_up;

        scriptEval.StartEvaluating(xpcWrappedJSErrorReporter);

        xpcc->SetPendingResult(pending_result);
        xpcc->SetException(nsnull);
        ccx.GetThreadData()->SetException(nsnull);

        
        
        
        

        

        
        stack_size = argc + 2;


        
        
        
        
        
        
        
        
        
        
        fval = OBJECT_TO_JSVAL(obj);
        if(JS_TypeOfValue(ccx, fval) != JSTYPE_FUNCTION && 
            !JS_GetProperty(cx, obj, name.get(), &fval))
        {
            
            
            
            
            
            goto pre_call_clean_up;
        }

        
        if(stack_size && !(stackbase = sp = js_AllocStack(cx, stack_size, &mark)))
        {
            retval = NS_ERROR_OUT_OF_MEMORY;
            goto pre_call_clean_up;
        }

        
        if(stack_size != argc)
        {
            *sp++ = fval;
            *sp++ = OBJECT_TO_JSVAL(thisObj);
        }

        
        for(i = 0; i < argc; i++)
        {
            sp[i] = JSVAL_VOID;
        }

        uintN err;
        
        
        for (j = argc - 1; j >= 0; --j )
        {
            jsval val;
            if((pDispParams->rgvarg[j].vt & VT_BYREF) == 0)
            {
                if(!XPCDispConvert::COMToJS(ccx, pDispParams->rgvarg[j], val, err))
                    goto pre_call_clean_up;
                *sp++ = val;
            }
            else
            {
                
                JSObject* out_obj = JS_NewObject(cx, nsnull, nsnull, nsnull);
                if(!out_obj)
                {
                    retval = NS_ERROR_OUT_OF_MEMORY;
                    goto pre_call_clean_up;
                }
                
                
                OBJ_SET_PROPERTY(cx, out_obj,
                        rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                        &val);
                *sp++ = OBJECT_TO_JSVAL(out_obj);
            }
        }

        readyToDoTheCall = JS_TRUE;

pre_call_clean_up:

        if(!readyToDoTheCall)
            goto done;

        

        JS_ClearPendingException(cx);

        if(!JSVAL_IS_PRIMITIVE(fval))
        {
            
            
            JSStackFrame *fp, *oldfp, frame;
            jsval *oldsp;

            fp = oldfp = cx->fp;
            if(!fp)
            {
                memset(&frame, 0, sizeof(frame));
                cx->fp = fp = &frame;
            }
            oldsp = fp->sp;
            fp->sp = sp;

            success = js_Invoke(cx, argc, JSINVOKE_INTERNAL);

            result = fp->sp[-1];
            fp->sp = oldsp;
            if(oldfp != fp)
                cx->fp = oldfp;
        }
        else
        {
            
            

            static const nsresult code =
                    NS_ERROR_XPC_JSOBJECT_HAS_NO_FUNCTION_NAMED;
            static const char format[] = "%s \"%s\"";
            const char * msg;
            char* sz = nsnull;

            if(nsXPCException::NameAndFormatForNSResult(code, nsnull, &msg) && msg)
                sz = JS_smprintf(format, msg, name);

            nsCOMPtr<nsIException> e;

            XPCConvert::ConstructException(code, sz, "IDispatch", name.get(),
                                           nsnull, getter_AddRefs(e));
            xpcc->SetException(e);
            if(sz)
                JS_smprintf_free(sz);
        }

        if (!success)
        {
            retval = nsXPCWrappedJSClass::CheckForException(ccx, name.get(), "IDispatch");
            goto done;
        }

        ccx.GetThreadData()->SetException(nsnull); 

        
        
        
        
        
        

        outConversionFailedIndex = paramCount;
        foundDependentParam = JS_FALSE;
        if(JSVAL_IS_VOID(result) || XPCDispConvert::JSToCOM(ccx, result, *pVarResult, err))
        {
            for(i = 0; i < paramCount; i++)
            {
                jsval val;
                if(JSVAL_IS_PRIMITIVE(stackbase[i+2]) ||
                        !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(stackbase[i+2]),
                            rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                            &val))
                {
                    outConversionFailedIndex = i;
                    break;
                }

            }
        }

        if(outConversionFailedIndex != paramCount)
        {
            
            

            for(PRUint32 index = 0; index < outConversionFailedIndex; index++)
            {
                if((pDispParams->rgvarg[index].vt & VT_BYREF) != 0)
                {
                    VariantClear(pDispParams->rgvarg + i);
                }
            }
        }
        else
        {
            
            retval = pending_result;
        }

done:
        if(sp)
            js_FreeStack(cx, mark);

        
        
        return retval;
    }
    return S_OK;
}

inline
JSObject* XPCDispatchTearOff::GetJSObject()
{
    JSObject* obj;
    if(NS_SUCCEEDED(mWrappedJS->GetJSObject(&obj)))
        return obj;
    return nsnull;
}
