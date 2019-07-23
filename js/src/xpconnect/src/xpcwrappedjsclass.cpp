










































#include "xpcprivate.h"
#include "nsArrayEnumerator.h"
#include "nsWrapperCache.h"
#include "XPCWrapper.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsXPCWrappedJSClass, nsIXPCWrappedJSClass)


static uint32 zero_methods_descriptor;

void AutoScriptEvaluate::StartEvaluating(JSErrorReporter errorReporter)
{
    NS_PRECONDITION(!mEvaluated, "AutoScriptEvaluate::Evaluate should only be called once");

    if(!mJSContext)
        return;
    mEvaluated = PR_TRUE;
    if(!mJSContext->errorReporter)
    {
        JS_SetErrorReporter(mJSContext, errorReporter);
        mErrorReporterSet = PR_TRUE;
    }
    mContextHasThread = JS_GetContextThread(mJSContext);
    if (mContextHasThread)
        JS_BeginRequest(mJSContext);

    
    
    
    
    
    
    
    
    
    
    if(JS_IsExceptionPending(mJSContext))
    {
        mState = JS_SaveExceptionState(mJSContext);
        JS_ClearPendingException(mJSContext);
    }
}

AutoScriptEvaluate::~AutoScriptEvaluate()
{
    if(!mJSContext || !mEvaluated)
        return;
    if(mState)
        JS_RestoreExceptionState(mJSContext, mState);
    else
        JS_ClearPendingException(mJSContext);

    if(mContextHasThread)
        JS_EndRequest(mJSContext);

    
    
    
    
    
    
    

    if (JS_GetOptions(mJSContext) & JSOPTION_PRIVATE_IS_NSISUPPORTS)
    {
        nsCOMPtr<nsIXPCScriptNotify> scriptNotify = 
            do_QueryInterface(static_cast<nsISupports*>
                                         (JS_GetContextPrivate(mJSContext)));
        if(scriptNotify)
            scriptNotify->ScriptExecuted();
    }

    if(mErrorReporterSet)
        JS_SetErrorReporter(mJSContext, NULL);
}



JSBool xpc_IsReportableErrorCode(nsresult code)
{
    if (NS_SUCCEEDED(code))
        return JS_FALSE;

    switch(code)
    {
        
        
        case NS_ERROR_FACTORY_REGISTER_AGAIN:
        case NS_BASE_STREAM_WOULD_BLOCK:
            return JS_FALSE;
    }

    return JS_TRUE;
}


nsresult
nsXPCWrappedJSClass::GetNewOrUsed(XPCCallContext& ccx, REFNSIID aIID,
                                  nsXPCWrappedJSClass** resultClazz)
{
    nsXPCWrappedJSClass* clazz = nsnull;
    XPCJSRuntime* rt = ccx.GetRuntime();

    {   
        XPCAutoLock lock(rt->GetMapLock());
        IID2WrappedJSClassMap* map = rt->GetWrappedJSClassMap();
        clazz = map->Find(aIID);
        NS_IF_ADDREF(clazz);
    }

    if(!clazz)
    {
        nsCOMPtr<nsIInterfaceInfo> info;
        ccx.GetXPConnect()->GetInfoForIID(&aIID, getter_AddRefs(info));
        if(info)
        {
            PRBool canScript;
            if(NS_SUCCEEDED(info->IsScriptable(&canScript)) && canScript &&
               nsXPConnect::IsISupportsDescendant(info))
            {
                clazz = new nsXPCWrappedJSClass(ccx, aIID, info);
                if(clazz && !clazz->mDescriptors)
                    NS_RELEASE(clazz);  
            }
        }
    }
    *resultClazz = clazz;
    return NS_OK;
}

nsXPCWrappedJSClass::nsXPCWrappedJSClass(XPCCallContext& ccx, REFNSIID aIID,
                                         nsIInterfaceInfo* aInfo)
    : mRuntime(ccx.GetRuntime()),
      mInfo(aInfo),
      mName(nsnull),
      mIID(aIID),
      mDescriptors(nsnull)
{
    NS_ADDREF(mInfo);
    NS_ADDREF_THIS();

    {   
        XPCAutoLock lock(mRuntime->GetMapLock());
        mRuntime->GetWrappedJSClassMap()->Add(this);
    }

    uint16 methodCount;
    if(NS_SUCCEEDED(mInfo->GetMethodCount(&methodCount)))
    {
        if(methodCount)
        {
            int wordCount = (methodCount/32)+1;
            if(nsnull != (mDescriptors = new uint32[wordCount]))
            {
                int i;
                
                for(i = wordCount-1; i >= 0; i--)
                    mDescriptors[i] = 0;

                for(i = 0; i < methodCount; i++)
                {
                    const nsXPTMethodInfo* info;
                    if(NS_SUCCEEDED(mInfo->GetMethodInfo(i, &info)))
                        SetReflectable(i, XPCConvert::IsMethodReflectable(*info));
                    else
                    {
                        delete [] mDescriptors;
                        mDescriptors = nsnull;
                        break;
                    }
                }
            }
        }
        else
        {
            mDescriptors = &zero_methods_descriptor;
        }
    }
}

nsXPCWrappedJSClass::~nsXPCWrappedJSClass()
{
    if(mDescriptors && mDescriptors != &zero_methods_descriptor)
        delete [] mDescriptors;
    if(mRuntime)
    {   
        XPCAutoLock lock(mRuntime->GetMapLock());
        mRuntime->GetWrappedJSClassMap()->Remove(this);
    }
    if(mName)
        nsMemory::Free(mName);
    NS_IF_RELEASE(mInfo);
}

JSObject*
nsXPCWrappedJSClass::CallQueryInterfaceOnJSObject(XPCCallContext& ccx,
                                                  JSObject* jsobj,
                                                  REFNSIID aIID)
{
    JSContext* cx = ccx.GetJSContext();
    JSObject* id;
    jsval retval;
    JSObject* retObj;
    JSBool success = JS_FALSE;
    jsid funid;
    jsval fun;

    
    
    
    
    if(XPCPerThreadData::IsMainThread(ccx) &&
       !JS_GetGlobalForObject(ccx, jsobj)->isSystem())
    {
        nsCOMPtr<nsIPrincipal> objprin;
        nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
        if(ssm)
        {
            nsresult rv = ssm->GetObjectPrincipal(ccx, jsobj, getter_AddRefs(objprin));
            NS_ENSURE_SUCCESS(rv, nsnull);

            PRBool isSystem;
            rv = ssm->IsSystemPrincipal(objprin, &isSystem);
            NS_ENSURE_SUCCESS(rv, nsnull);

            if(!isSystem)
                return nsnull;
        }
    }

    
    funid = mRuntime->GetStringID(XPCJSRuntime::IDX_QUERY_INTERFACE);
    if(!JS_GetPropertyById(cx, jsobj, funid, &fun) || JSVAL_IS_PRIMITIVE(fun))
        return nsnull;

    
    AUTO_MARK_JSVAL(ccx, fun);

    
    
    
    
    
    
    if(!aIID.Equals(NS_GET_IID(nsISupports)))
    {
        nsCOMPtr<nsIInterfaceInfo> info;
        ccx.GetXPConnect()->GetInfoForIID(&aIID, getter_AddRefs(info));
        if(!info)
            return nsnull;
        PRBool canScript;
        if(NS_FAILED(info->IsScriptable(&canScript)) || !canScript)
            return nsnull;
    }

    

    AutoScriptEvaluate scriptEval(cx);

    
    
    scriptEval.StartEvaluating();

    id = xpc_NewIDObject(cx, jsobj, aIID);
    if(id)
    {
        
        
        

        uint32 oldOpts =
          JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_DONT_REPORT_UNCAUGHT);

        jsval args[1] = {OBJECT_TO_JSVAL(id)};
        success = JS_CallFunctionValue(cx, jsobj, fun, 1, args, &retval);

        JS_SetOptions(cx, oldOpts);

        if(!success)
        {
            NS_ASSERTION(JS_IsExceptionPending(cx),
                         "JS failed without setting an exception!");

            jsval jsexception = JSVAL_NULL;
            AUTO_MARK_JSVAL(ccx, &jsexception);

            if(JS_GetPendingException(cx, &jsexception))
            {
                nsresult rv;
                if(JSVAL_IS_OBJECT(jsexception))
                {
                    
                    
                    nsCOMPtr<nsIXPConnectWrappedNative> wrapper;

                    nsXPConnect::GetXPConnect()->
                        GetWrappedNativeOfJSObject(ccx,
                                                   JSVAL_TO_OBJECT(jsexception),
                                                   getter_AddRefs(wrapper));

                    if(wrapper)
                    {
                        nsCOMPtr<nsIException> exception =
                            do_QueryWrappedNative(wrapper);
                        if(exception &&
                           NS_SUCCEEDED(exception->GetResult(&rv)) &&
                           rv == NS_NOINTERFACE)
                        {
                            JS_ClearPendingException(cx);
                        }
                    }
                }
                else if(JSVAL_IS_NUMBER(jsexception))
                {
                    
                    if(JSVAL_IS_DOUBLE(jsexception))
                        rv = (nsresult)(*JSVAL_TO_DOUBLE(jsexception));
                    else
                        rv = (nsresult)(JSVAL_TO_INT(jsexception));

                    if(rv == NS_NOINTERFACE)
                        JS_ClearPendingException(cx);
                }
            }

            
            if(!(oldOpts & JSOPTION_DONT_REPORT_UNCAUGHT))
                JS_ReportPendingException(cx);
        }
    }

    if(success)
        success = JS_ValueToObject(cx, retval, &retObj);

    return success ? retObj : nsnull;
}



static JSBool 
GetNamedPropertyAsVariantRaw(XPCCallContext& ccx, 
                             JSObject* aJSObj,
                             jsid aName, 
                             nsIVariant** aResult,
                             nsresult* pErr)
{
    nsXPTType type = nsXPTType((uint8)(TD_INTERFACE_TYPE | XPT_TDP_POINTER));
    jsval val;

    return JS_GetPropertyById(ccx, aJSObj, aName, &val) &&
           XPCConvert::JSData2Native(ccx, aResult, val, type, JS_FALSE, 
                                     &NS_GET_IID(nsIVariant), pErr);
}


nsresult
nsXPCWrappedJSClass::GetNamedPropertyAsVariant(XPCCallContext& ccx, 
                                               JSObject* aJSObj,
                                               jsval aName, 
                                               nsIVariant** aResult)
{
    JSContext* cx = ccx.GetJSContext();
    JSBool ok;
    jsid id;
    nsresult rv = NS_ERROR_FAILURE;

    AutoScriptEvaluate scriptEval(cx);
    scriptEval.StartEvaluating();

    ok = JS_ValueToId(cx, aName, &id) && 
         GetNamedPropertyAsVariantRaw(ccx, aJSObj, id, aResult, &rv);

    return ok ? NS_OK : NS_FAILED(rv) ? rv : NS_ERROR_FAILURE;
}




nsresult 
nsXPCWrappedJSClass::BuildPropertyEnumerator(XPCCallContext& ccx,
                                             JSObject* aJSObj,
                                             nsISimpleEnumerator** aEnumerate)
{
    JSContext* cx = ccx.GetJSContext();
    nsresult retval = NS_ERROR_FAILURE;
    JSIdArray* idArray = nsnull;
    int i;

    
    AutoScriptEvaluate scriptEval(cx);
    scriptEval.StartEvaluating();

    idArray = JS_Enumerate(cx, aJSObj);
    if(!idArray)
        return retval;
    
    nsCOMArray<nsIProperty> propertyArray(idArray->length);
    for(i = 0; i < idArray->length; i++)
    {
        nsCOMPtr<nsIVariant> value;
        jsid idName = idArray->vector[i];
        nsresult rv;

        if(!GetNamedPropertyAsVariantRaw(ccx, aJSObj, idName, 
                                         getter_AddRefs(value), &rv))
        {
            if(NS_FAILED(rv))
                retval = rv;                                        
            goto out;
        }

        jsval jsvalName;
        if(!JS_IdToValue(cx, idName, &jsvalName))
            goto out;

        JSString* name = JS_ValueToString(cx, jsvalName);
        if(!name)
            goto out;

        nsCOMPtr<nsIProperty> property = 
            new xpcProperty((const PRUnichar*) JS_GetStringChars(name), 
                            (PRUint32) JS_GetStringLength(name),
                            value);
        if(!property)
            goto out;

        if(!propertyArray.AppendObject(property))
            goto out;
    }

    retval = NS_NewArrayEnumerator(aEnumerate, propertyArray);

out:
    JS_DestroyIdArray(cx, idArray);

    return retval;
}



NS_IMPL_ISUPPORTS1(xpcProperty, nsIProperty)

xpcProperty::xpcProperty(const PRUnichar* aName, PRUint32 aNameLen, 
                         nsIVariant* aValue)
    : mName(aName, aNameLen), mValue(aValue)
{
}


NS_IMETHODIMP xpcProperty::GetName(nsAString & aName)
{
    aName.Assign(mName);
    return NS_OK;
}


NS_IMETHODIMP xpcProperty::GetValue(nsIVariant * *aValue)
{
    NS_ADDREF(*aValue = mValue);
    return NS_OK;
}











#define NS_IXPCONNECT_WRAPPED_JS_IDENTITY_CLASS_IID \
{ 0x5c5c3bb0, 0xa9ba, 0x11d2,                       \
  { 0xba, 0x64, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class WrappedJSIdentity
{
    
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_IDENTITY_CLASS_IID)

    static void* GetSingleton()
    {
        static WrappedJSIdentity* singleton = nsnull;
        if(!singleton)
            singleton = new WrappedJSIdentity();
        return (void*) singleton;
    }
};

NS_DEFINE_STATIC_IID_ACCESSOR(WrappedJSIdentity,
                              NS_IXPCONNECT_WRAPPED_JS_IDENTITY_CLASS_IID)




JSBool
nsXPCWrappedJSClass::IsWrappedJS(nsISupports* aPtr)
{
    void* result;
    NS_PRECONDITION(aPtr, "null pointer");
    return aPtr &&
           NS_OK == aPtr->QueryInterface(NS_GET_IID(WrappedJSIdentity), &result) &&
           result == WrappedJSIdentity::GetSingleton();
}

static JSContext *
GetContextFromObject(JSObject *obj)
{
    
    XPCJSContextStack* stack =
        XPCPerThreadData::GetData(nsnull)->GetJSContextStack();
    JSContext* topJSContext;

    if(stack && NS_SUCCEEDED(stack->Peek(&topJSContext)) && topJSContext)
        return nsnull;

    
    XPCCallContext ccx(NATIVE_CALLER);
    if(!ccx.IsValid())
        return nsnull;
    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, obj);
    XPCContext *xpcc = scope->GetContext();

    if(xpcc)
    {
        JSContext *cx = xpcc->GetJSContext();
        if(cx->thread->id == js_CurrentThreadId())
            return cx;
    }

    return nsnull;
}

#ifndef XPCONNECT_STANDALONE
class SameOriginCheckedComponent : public nsISecurityCheckedComponent
{
public:
    SameOriginCheckedComponent(nsXPCWrappedJS* delegate)
        : mDelegate(delegate)
    {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSISECURITYCHECKEDCOMPONENT

private:
    nsRefPtr<nsXPCWrappedJS> mDelegate;
};

NS_IMPL_ADDREF(SameOriginCheckedComponent)
NS_IMPL_RELEASE(SameOriginCheckedComponent)

NS_INTERFACE_MAP_BEGIN(SameOriginCheckedComponent)
    NS_INTERFACE_MAP_ENTRY(nsISecurityCheckedComponent)
NS_INTERFACE_MAP_END_AGGREGATED(mDelegate)

NS_IMETHODIMP
SameOriginCheckedComponent::CanCreateWrapper(const nsIID * iid,
                                             char **_retval NS_OUTPARAM)
{
    
    
    *_retval = NS_strdup("sameOrigin");
    return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
SameOriginCheckedComponent::CanCallMethod(const nsIID * iid,
                                          const PRUnichar *methodName,
                                          char **_retval NS_OUTPARAM)
{
    *_retval = NS_strdup("sameOrigin");
    return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
SameOriginCheckedComponent::CanGetProperty(const nsIID * iid,
                                           const PRUnichar *propertyName,
                                           char **_retval NS_OUTPARAM)
{
    *_retval = NS_strdup("sameOrigin");
    return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
SameOriginCheckedComponent::CanSetProperty(const nsIID * iid,
                                           const PRUnichar *propertyName,
                                           char **_retval NS_OUTPARAM)
{
    *_retval = NS_strdup("sameOrigin");
    return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

#endif

NS_IMETHODIMP
nsXPCWrappedJSClass::DelegatedQueryInterface(nsXPCWrappedJS* self,
                                             REFNSIID aIID,
                                             void** aInstancePtr)
{
    if(aIID.Equals(NS_GET_IID(nsIXPConnectJSObjectHolder)))
    {
        NS_ADDREF(self);
        *aInstancePtr = (void*) static_cast<nsIXPConnectJSObjectHolder*>(self);
        return NS_OK;
    }

    
    
    if(aIID.Equals(NS_GET_IID(WrappedJSIdentity)))
    {
        
        *aInstancePtr = WrappedJSIdentity::GetSingleton();
        return NS_OK;
    }

#ifdef XPC_IDISPATCH_SUPPORT
    
    if(nsXPConnect::IsIDispatchEnabled() && aIID.Equals(NSID_IDISPATCH))
    {
        return XPCIDispatchExtension::IDispatchQIWrappedJS(self, aInstancePtr);
    }
#endif
    if(aIID.Equals(NS_GET_IID(nsIPropertyBag)))
    {
        
        nsXPCWrappedJS* root = self->GetRootWrapper();

        if(!root->IsValid())
        {
            *aInstancePtr = nsnull;
            return NS_NOINTERFACE;
        }

        NS_ADDREF(root);
        *aInstancePtr = (void*) static_cast<nsIPropertyBag*>(root);
        return NS_OK;
    }

    
    if(aIID.Equals(NS_GET_IID(nsWrapperCache)))
    {
        *aInstancePtr = nsnull;
        return NS_NOINTERFACE;
    }

    JSContext *context = GetContextFromObject(self->GetJSObject());
    XPCCallContext ccx(NATIVE_CALLER, context);
    if(!ccx.IsValid())
    {
        *aInstancePtr = nsnull;
        return NS_NOINTERFACE;
    }

    
    
    if(aIID.Equals(NS_GET_IID(nsISupportsWeakReference)))
    {
        
        nsXPCWrappedJS* root = self->GetRootWrapper();

        
        if(!root->IsValid() ||
           !CallQueryInterfaceOnJSObject(ccx, root->GetJSObject(), aIID))
        {
            *aInstancePtr = nsnull;
            return NS_NOINTERFACE;
        }

        NS_ADDREF(root);
        *aInstancePtr = (void*) static_cast<nsISupportsWeakReference*>(root);
        return NS_OK;
    }

    nsXPCWrappedJS* sibling;

    
    
    
    if(nsnull != (sibling = self->Find(aIID)))
    {
        NS_ADDREF(sibling);
        *aInstancePtr = sibling->GetXPTCStub();
        return NS_OK;
    }

    
    if(nsnull != (sibling = self->FindInherited(aIID)))
    {
        NS_ADDREF(sibling);
        *aInstancePtr = sibling->GetXPTCStub();
        return NS_OK;
    }

    

#ifndef XPCONNECT_STANDALONE
    
    
    
    
    
    
    
    
    
    
    

    if(aIID.Equals(NS_GET_IID(nsISecurityCheckedComponent)))
    {
        
        
        

        *aInstancePtr = nsnull;

        if(!XPCPerThreadData::IsMainThread(ccx.GetJSContext()))
            return NS_NOINTERFACE;

        nsXPConnect *xpc = nsXPConnect::GetXPConnect();
        nsCOMPtr<nsIScriptSecurityManager> secMan =
            do_QueryInterface(xpc->GetDefaultSecurityManager());
        if(!secMan)
            return NS_NOINTERFACE;

        JSObject *selfObj = self->GetJSObject();
        nsCOMPtr<nsIPrincipal> objPrin;
        nsresult rv = secMan->GetObjectPrincipal(ccx, selfObj,
                                                 getter_AddRefs(objPrin));
        if(NS_FAILED(rv))
            return rv;

        PRBool isSystem;
        rv = secMan->IsSystemPrincipal(objPrin, &isSystem);
        if((NS_FAILED(rv) || !isSystem) &&
           !IS_WRAPPER_CLASS(STOBJ_GET_CLASS(selfObj)))
        {
            
            nsRefPtr<SameOriginCheckedComponent> checked =
                new SameOriginCheckedComponent(self);
            if(!checked)
                return NS_ERROR_OUT_OF_MEMORY;
            *aInstancePtr = checked.forget().get();
            return NS_OK;
        }
    }
#endif

    
    JSObject* jsobj = CallQueryInterfaceOnJSObject(ccx, self->GetJSObject(),
                                                   aIID);
    if(jsobj)
    {
        
        AUTO_MARK_JSVAL(ccx, OBJECT_TO_JSVAL(jsobj));

        
        
        
        
        
        
        
        
        
        
        
        nsXPCWrappedJS* wrapper;
        nsresult rv = nsXPCWrappedJS::GetNewOrUsed(ccx, jsobj, aIID, nsnull,
                                                   &wrapper);
        if(NS_SUCCEEDED(rv) && wrapper)
        {
            
            
            
            rv = wrapper->QueryInterface(aIID, aInstancePtr);
            NS_RELEASE(wrapper);
            return rv;
        }
    }

    
    
    *aInstancePtr = nsnull;
    return NS_NOINTERFACE;
}

JSObject*
nsXPCWrappedJSClass::GetRootJSObject(XPCCallContext& ccx, JSObject* aJSObj)
{
    JSObject* result = CallQueryInterfaceOnJSObject(ccx, aJSObj,
                                                    NS_GET_IID(nsISupports));
    if(!result)
        return aJSObj;
    JSObject* inner = XPCWrapper::Unwrap(ccx, result);
    if (inner)
        return inner;
    return result;
}

void
xpcWrappedJSErrorReporter(JSContext *cx, const char *message,
                          JSErrorReport *report)
{
    if(report)
    {
        
        
        if(JSREPORT_IS_EXCEPTION(report->flags))
        {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            return;
        }

        if(JSREPORT_IS_WARNING(report->flags))
        {
            
            
            return;
        }
    }

    XPCCallContext ccx(NATIVE_CALLER, cx);
    if(!ccx.IsValid())
        return;

    nsCOMPtr<nsIException> e;
    XPCConvert::JSErrorToXPCException(ccx, message, nsnull, nsnull, report,
                                      getter_AddRefs(e));
    if(e)
        ccx.GetXPCContext()->SetException(e);
}

JSBool
nsXPCWrappedJSClass::GetArraySizeFromParam(JSContext* cx,
                                           const XPTMethodDescriptor* method,
                                           const nsXPTParamInfo& param,
                                           uint16 methodIndex,
                                           uint8 paramIndex,
                                           SizeMode mode,
                                           nsXPTCMiniVariant* nativeParams,
                                           JSUint32* result)
{
    uint8 argnum;
    nsresult rv;

    if(mode == GET_SIZE)
        rv = mInfo->GetSizeIsArgNumberForParam(methodIndex, &param, 0, &argnum);
    else
        rv = mInfo->GetLengthIsArgNumberForParam(methodIndex, &param, 0, &argnum);
    if(NS_FAILED(rv))
        return JS_FALSE;

    const nsXPTParamInfo& arg_param = method->params[argnum];
    const nsXPTType& arg_type = arg_param.GetType();

    
    if(arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_U32)
        return JS_FALSE;

    if(arg_param.IsOut())
        *result = *(JSUint32*)nativeParams[argnum].val.p;
    else
        *result = nativeParams[argnum].val.u32;

    return JS_TRUE;
}

JSBool
nsXPCWrappedJSClass::GetInterfaceTypeFromParam(JSContext* cx,
                                               const XPTMethodDescriptor* method,
                                               const nsXPTParamInfo& param,
                                               uint16 methodIndex,
                                               const nsXPTType& type,
                                               nsXPTCMiniVariant* nativeParams,
                                               nsID* result)
{
    uint8 type_tag = type.TagPart();

    if(type_tag == nsXPTType::T_INTERFACE)
    {
        if(NS_SUCCEEDED(GetInterfaceInfo()->
                GetIIDForParamNoAlloc(methodIndex, &param, result)))
        {
            return JS_TRUE;
        }
    }
    else if(type_tag == nsXPTType::T_INTERFACE_IS)
    {
        uint8 argnum;
        nsresult rv;
        rv = mInfo->GetInterfaceIsArgNumberForParam(methodIndex,
                                                    &param, &argnum);
        if(NS_FAILED(rv))
            return JS_FALSE;

        const nsXPTParamInfo& arg_param = method->params[argnum];
        const nsXPTType& arg_type = arg_param.GetType();
        if(arg_type.IsPointer() &&
           arg_type.TagPart() == nsXPTType::T_IID)
        {
            if(arg_param.IsOut())
            {
                nsID** p = (nsID**) nativeParams[argnum].val.p;
                if(!p || !*p)
                    return JS_FALSE;
                *result = **p;
            }
            else
            {
                nsID* p = (nsID*) nativeParams[argnum].val.p;
                if(!p)
                    return JS_FALSE;
                *result = *p;
            }
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

void
nsXPCWrappedJSClass::CleanupPointerArray(const nsXPTType& datum_type,
                                         JSUint32 array_count,
                                         void** arrayp)
{
    if(datum_type.IsInterfacePointer())
    {
        nsISupports** pp = (nsISupports**) arrayp;
        for(JSUint32 k = 0; k < array_count; k++)
        {
            nsISupports* p = pp[k];
            NS_IF_RELEASE(p);
        }
    }
    else
    {
        void** pp = (void**) arrayp;
        for(JSUint32 k = 0; k < array_count; k++)
        {
            void* p = pp[k];
            if(p) nsMemory::Free(p);
        }
    }
}

void
nsXPCWrappedJSClass::CleanupPointerTypeObject(const nsXPTType& type,
                                              void** pp)
{
    NS_ASSERTION(pp,"null pointer");
    if(type.IsInterfacePointer())
    {
        nsISupports* p = *((nsISupports**)pp);
        if(p) p->Release();
    }
    else
    {
        void* p = *((void**)pp);
        if(p) nsMemory::Free(p);
    }
}

class AutoClearPendingException
{
public:
  AutoClearPendingException(JSContext *cx) : mCx(cx) { }
  ~AutoClearPendingException() { JS_ClearPendingException(mCx); }
private:
  JSContext* mCx;
};

nsresult
nsXPCWrappedJSClass::CheckForException(XPCCallContext & ccx,
                                       const char * aPropertyName,
                                       const char * anInterfaceName,
                                       PRBool aForceReport)
{
    XPCContext * xpcc = ccx.GetXPCContext();
    JSContext * cx = ccx.GetJSContext();
    nsCOMPtr<nsIException> xpc_exception;
    

    xpcc->GetException(getter_AddRefs(xpc_exception));
    if(xpc_exception)
        xpcc->SetException(nsnull);

    
    
    nsresult pending_result = xpcc->GetPendingResult();

    jsval js_exception;
    JSBool is_js_exception = JS_GetPendingException(cx, &js_exception);

    
    if(is_js_exception)
    {
        if(!xpc_exception)
            XPCConvert::JSValToXPCException(ccx, js_exception, anInterfaceName,
                                            aPropertyName,
                                            getter_AddRefs(xpc_exception));

        
        if(!xpc_exception)
        {
            ccx.GetThreadData()->SetException(nsnull); 
        }
    }

    AutoClearPendingException acpe(cx);

    if(xpc_exception)
    {
        nsresult e_result;
        if(NS_SUCCEEDED(xpc_exception->GetResult(&e_result)))
        {
            
            PRBool reportable = xpc_IsReportableErrorCode(e_result);
            if(reportable)
            {
                
                
                reportable = aForceReport ||
                    NS_ERROR_GET_MODULE(e_result) == NS_ERROR_MODULE_XPCONNECT;

                
                
                
                if(!reportable)
                    reportable = nsXPConnect::ReportAllJSExceptions();

                
                
                if(!reportable)
                {
                    PRBool onlyNativeStackFrames = PR_TRUE;
                    JSStackFrame * fp = nsnull;
                    while((fp = JS_FrameIterator(cx, &fp)))
                    {
                        if(!JS_IsNativeFrame(cx, fp))
                        {
                            onlyNativeStackFrames = PR_FALSE;
                            break;
                        }
                    }
                    reportable = onlyNativeStackFrames;
                }
                
                
                
                
                
                
                if(reportable && e_result == NS_ERROR_NO_INTERFACE &&
                   !strcmp(anInterfaceName, "nsIInterfaceRequestor") &&
                   !strcmp(aPropertyName, "getInterface"))
                {
                    reportable = PR_FALSE;
                }
            }

            
            
            if(reportable && is_js_exception &&
               cx->errorReporter != xpcWrappedJSErrorReporter)
            {
                reportable = !JS_ReportPendingException(cx);
            }

            if(reportable)
            {
#ifdef DEBUG
                static const char line[] =
                    "************************************************************\n";
                static const char preamble[] =
                    "* Call to xpconnect wrapped JSObject produced this error:  *\n";
                static const char cant_get_text[] =
                    "FAILED TO GET TEXT FROM EXCEPTION\n";

                fputs(line, stdout);
                fputs(preamble, stdout);
                char* text;
                if(NS_SUCCEEDED(xpc_exception->ToString(&text)) && text)
                {
                    fputs(text, stdout);
                    fputs("\n", stdout);
                    nsMemory::Free(text);
                }
                else
                    fputs(cant_get_text, stdout);
                fputs(line, stdout);
#endif

                
                
                nsCOMPtr<nsIConsoleService> consoleService
                    (do_GetService(XPC_CONSOLE_CONTRACTID));
                if(nsnull != consoleService)
                {
                    nsresult rv;
                    nsCOMPtr<nsIScriptError> scriptError;
                    nsCOMPtr<nsISupports> errorData;
                    rv = xpc_exception->GetData(getter_AddRefs(errorData));
                    if(NS_SUCCEEDED(rv))
                        scriptError = do_QueryInterface(errorData);

                    if(nsnull == scriptError)
                    {
                        
                        
                        scriptError = do_CreateInstance(XPC_SCRIPT_ERROR_CONTRACTID);
                        if(nsnull != scriptError)
                        {
                            char* exn_string;
                            rv = xpc_exception->ToString(&exn_string);
                            if(NS_SUCCEEDED(rv))
                            {
                                
                                nsAutoString newMessage;
                                newMessage.AssignWithConversion(exn_string);
                                nsMemory::Free((void *) exn_string);

                                
                                
                                PRInt32 lineNumber = 0;
                                nsXPIDLCString sourceName;

                                nsCOMPtr<nsIStackFrame> location;
                                xpc_exception->
                                    GetLocation(getter_AddRefs(location));
                                if(location)
                                {
                                    
                                    location->GetLineNumber(&lineNumber);

                                    
                                    rv = location->GetFilename(getter_Copies(sourceName));
                                }

                                rv = scriptError->Init(newMessage.get(),
                                                       NS_ConvertASCIItoUTF16(sourceName).get(),
                                                       nsnull,
                                                       lineNumber, 0, 0,
                                                       "XPConnect JavaScript");
                                if(NS_FAILED(rv))
                                    scriptError = nsnull;
                            }
                        }
                    }
                    if(nsnull != scriptError)
                        consoleService->LogMessage(scriptError);
                }
            }
            
            
            if(NS_FAILED(e_result))
            {
                ccx.GetThreadData()->SetException(xpc_exception);
                return e_result;
            }
        }
    }
    else
    {
        
        if(NS_FAILED(pending_result))
        {
            return pending_result;
        }
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXPCWrappedJSClass::CallMethod(nsXPCWrappedJS* wrapper, uint16 methodIndex,
                                const XPTMethodDescriptor* info,
                                nsXPTCMiniVariant* nativeParams)
{
    jsval* stackbase = nsnull;
    jsval* sp = nsnull;
    uint8 i;
    uint8 argc=0;
    uint8 stack_size;
    jsval result;
    uint8 paramCount=0;
    nsresult retval = NS_ERROR_FAILURE;
    nsresult pending_result = NS_OK;
    JSBool success;
    JSBool readyToDoTheCall = JS_FALSE;
    nsID  param_iid;
    JSObject* obj;
    const char* name = info->name;
    jsval fval;
    void* mark;
    JSBool foundDependentParam;
    XPCContext* xpcc;
    JSContext* cx;
    JSObject* thisObj;
    JSBool popPrincipal = JS_FALSE;
    nsIScriptSecurityManager* ssm = nsnull;

    
    
    
    
    JSContext *context = GetContextFromObject(wrapper->GetJSObject());
    XPCCallContext ccx(NATIVE_CALLER, context);
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

    AutoScriptEvaluate scriptEval(cx);
#ifdef DEBUG_stats_jband
    PRIntervalTime startTime = PR_IntervalNow();
    PRIntervalTime endTime = 0;
    static int totalTime = 0;


    static int count = 0;
    static const int interval = 10;
    if(0 == (++count % interval))
        printf("<<<<<<<< %d calls on nsXPCWrappedJSs made.  (%d)\n", count, PR_IntervalToMilliseconds(totalTime));
#endif

    obj = thisObj = wrapper->GetJSObject();

    
    paramCount = info->num_args;
    argc = paramCount -
        (paramCount && XPT_PD_IS_RETVAL(info->params[paramCount-1].flags) ? 1 : 0);

    if(!cx || !xpcc || !IsReflectable(methodIndex))
        goto pre_call_clean_up;

    scriptEval.StartEvaluating(xpcWrappedJSErrorReporter);

    xpcc->SetPendingResult(pending_result);
    xpcc->SetException(nsnull);
    ccx.GetThreadData()->SetException(nsnull);

    
    
    
    

    js_LeaveTrace(cx);

    

    
    if(XPT_MD_IS_GETTER(info->flags) || XPT_MD_IS_SETTER(info->flags))
    {
        stack_size = argc;
    }
    else
    {
        
        stack_size = argc + 2;

        
        
        

        
        

        PRBool isFunction;
        if(NS_FAILED(mInfo->IsFunction(&isFunction)))
            goto pre_call_clean_up;

        
        
        
        
        
        
        
        
        
        

        if(isFunction && 
           JS_TypeOfValue(ccx, OBJECT_TO_JSVAL(obj)) == JSTYPE_FUNCTION)
        {
            fval = OBJECT_TO_JSVAL(obj);

            

            if(paramCount)
            {
                const nsXPTParamInfo& firstParam = info->params[0];
                if(firstParam.IsIn())
                {
                    const nsXPTType& firstType = firstParam.GetType();
                    if(firstType.IsPointer() && firstType.IsInterfacePointer())
                    {
                        nsIXPCFunctionThisTranslator* translator;

                        IID2ThisTranslatorMap* map =
                            mRuntime->GetThisTranslatorMap();

                        {
                            XPCAutoLock lock(mRuntime->GetMapLock()); 
                            translator = map->Find(mIID);
                        }

                        if(translator)
                        {
                            PRBool hideFirstParamFromJS = PR_FALSE;
                            nsIID* newWrapperIID = nsnull;
                            nsCOMPtr<nsISupports> newThis;

                            if(NS_FAILED(translator->
                              TranslateThis((nsISupports*)nativeParams[0].val.p,
                                            mInfo, methodIndex,
                                            &hideFirstParamFromJS,
                                            &newWrapperIID,
                                            getter_AddRefs(newThis))))
                            {
                                goto pre_call_clean_up;
                            }
                            if(hideFirstParamFromJS)
                            {
                                NS_ERROR("HideFirstParamFromJS not supported");
                                goto pre_call_clean_up;
                            }
                            if(newThis)
                            {
                                jsval v;
                                JSBool ok =
                                  XPCConvert::NativeInterface2JSObject(ccx,
                                        &v, nsnull, newThis, newWrapperIID,
                                        nsnull, nsnull, obj, PR_FALSE, PR_FALSE,
                                        nsnull);
                                if(newWrapperIID)
                                    nsMemory::Free(newWrapperIID);
                                if(!ok)
                                {
                                    goto pre_call_clean_up;
                                }
                                thisObj = JSVAL_TO_OBJECT(v);
                            }
                        }
                    }
                }
            }
        }
        else if(!JS_GetMethod(cx, obj, name, &thisObj, &fval))
        {
            
            
            
            
            
            goto pre_call_clean_up;
        }
    }

    
    if(stack_size && !(stackbase = sp = js_AllocStack(cx, stack_size, &mark)))
    {
        retval = NS_ERROR_OUT_OF_MEMORY;
        goto pre_call_clean_up;
    }

    NS_ASSERTION(XPT_MD_IS_GETTER(info->flags) || sp,
                 "Only a getter needs no stack.");

    
    if(stack_size != argc)
    {
        *sp++ = fval;
        *sp++ = OBJECT_TO_JSVAL(thisObj);
    }

    
    if(XPT_MD_IS_GETTER(info->flags) || XPT_MD_IS_SETTER(info->flags))
    {
        
        uintN attrs;
        JSBool found;
        JSPropertyOp getter;
        JSPropertyOp setter;
        if(!JS_GetPropertyAttrsGetterAndSetter(cx, obj, name,
                                               &attrs, &found,
                                               &getter, &setter))
        {
            
            JS_ClearPendingException(cx);
            goto pre_call_clean_up;
        }

        if(XPT_MD_IS_GETTER(info->flags) && (attrs & JSPROP_GETTER))
        {
            
            
            ccx.SetCallee(JS_FUNC_TO_DATA_PTR(JSObject*, getter));
        }
        else if(XPT_MD_IS_SETTER(info->flags) && (attrs & JSPROP_SETTER))
        {
            
            
            ccx.SetCallee(JS_FUNC_TO_DATA_PTR(JSObject*, setter));
        }
    }
    else if(JSVAL_IS_OBJECT(fval))
    {
        ccx.SetCallee(JSVAL_TO_OBJECT(fval));
    }

    
    for(i = 0; i < argc; i++)
    {
        const nsXPTParamInfo& param = info->params[i];
        const nsXPTType& type = param.GetType();
        nsXPTType datum_type;
        JSUint32 array_count;
        PRBool isArray = type.IsArray();
        jsval val = JSVAL_NULL;
        AUTO_MARK_JSVAL(ccx, &val);
        PRBool isSizedString = isArray ?
                JS_FALSE :
                type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;


        
        if(param.IsOut() && !nativeParams[i].val.p)
        {
            retval = NS_ERROR_INVALID_ARG;
            goto pre_call_clean_up;
        }

        if(isArray)
        {
            if(NS_FAILED(mInfo->GetTypeForParam(methodIndex, &param, 1,
                                                &datum_type)))
                goto pre_call_clean_up;
        }
        else
            datum_type = type;

        if(param.IsIn())
        {
            nsXPTCMiniVariant* pv;

            if(param.IsOut())
                pv = (nsXPTCMiniVariant*) nativeParams[i].val.p;
            else
                pv = &nativeParams[i];

            if(datum_type.IsInterfacePointer() &&
               !GetInterfaceTypeFromParam(cx, info, param, methodIndex,
                                          datum_type, nativeParams,
                                          &param_iid))
                goto pre_call_clean_up;

            if(isArray || isSizedString)
            {
                if(!GetArraySizeFromParam(cx, info, param, methodIndex,
                                          i, GET_LENGTH, nativeParams,
                                          &array_count))
                    goto pre_call_clean_up;
            }

            if(isArray)
            {
                XPCLazyCallContext lccx(ccx);
                if(!XPCConvert::NativeArray2JS(lccx, &val,
                                               (const void**)&pv->val,
                                               datum_type, &param_iid,
                                               array_count, obj, nsnull))
                    goto pre_call_clean_up;
            }
            else if(isSizedString)
            {
                if(!XPCConvert::NativeStringWithSize2JS(ccx, &val,
                                               (const void*)&pv->val,
                                               datum_type,
                                               array_count, nsnull))
                    goto pre_call_clean_up;
            }
            else
            {
                if(!XPCConvert::NativeData2JS(ccx, &val, &pv->val, type,
                                              &param_iid, obj, nsnull))
                    goto pre_call_clean_up;
            }
        }

        if(param.IsOut())
        {
            
            JSObject* out_obj = NewOutObject(cx, obj);
            if(!out_obj)
            {
                retval = NS_ERROR_OUT_OF_MEMORY;
                goto pre_call_clean_up;
            }

            if(param.IsIn())
            {
                if(!JS_SetPropertyById(cx, out_obj,
                        mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                        &val))
                {
                    goto pre_call_clean_up;
                }
            }
            *sp++ = OBJECT_TO_JSVAL(out_obj);
        }
        else
            *sp++ = val;
    }

    readyToDoTheCall = JS_TRUE;

pre_call_clean_up:
    
    for(i = 0; i < paramCount; i++)
    {
        const nsXPTParamInfo& param = info->params[i];
        if(!param.IsOut())
            continue;

        const nsXPTType& type = param.GetType();
        if(!type.IsPointer())
            continue;
        void* p;
        if(!(p = nativeParams[i].val.p))
            continue;

        if(param.IsIn())
        {
            if(type.IsArray())
            {
                void** pp;
                if(nsnull != (pp = *((void***)p)))
                {

                    
                    JSUint32 array_count;
                    nsXPTType datum_type;

                    if(NS_SUCCEEDED(mInfo->GetTypeForParam(methodIndex, &param,
                                                           1, &datum_type)) &&
                       datum_type.IsPointer() &&
                       GetArraySizeFromParam(cx, info, param, methodIndex,
                                             i, GET_LENGTH, nativeParams,
                                             &array_count) && array_count)
                    {
                        CleanupPointerArray(datum_type, array_count, pp);
                    }
                    
                    nsMemory::Free(pp);
                }
            }
            else
                CleanupPointerTypeObject(type, (void**)p);
        }
        *((void**)p) = nsnull;
    }

    
    nsCOMPtr<nsIXPCWrappedJSClass> kungFuDeathGrip(this);

    result = JSVAL_NULL;
    AUTO_MARK_JSVAL(ccx, &result);

    if(!readyToDoTheCall)
        goto done;

    

    JS_ClearPendingException(cx);

    if(XPCPerThreadData::IsMainThread(ccx))
    {
        ssm = XPCWrapper::GetSecurityManager();
        if(ssm)
        {
            nsCOMPtr<nsIPrincipal> objPrincipal;
            ssm->GetObjectPrincipal(ccx, obj, getter_AddRefs(objPrincipal));
            if(objPrincipal)
            {
                JSStackFrame* fp = nsnull;
                nsresult rv =
                    ssm->PushContextPrincipal(ccx, JS_FrameIterator(ccx, &fp),
                                              objPrincipal);
                if(NS_FAILED(rv))
                {
                    JS_ReportOutOfMemory(ccx);
                    retval = NS_ERROR_OUT_OF_MEMORY;
                    goto done;
                }

                popPrincipal = JS_TRUE;
            }
        }
    }

    if(XPT_MD_IS_GETTER(info->flags))
        success = JS_GetProperty(cx, obj, name, &result);
    else if(XPT_MD_IS_SETTER(info->flags))
        success = JS_SetProperty(cx, obj, name, sp-1);
    else
    {
        if(!JSVAL_IS_PRIMITIVE(fval))
        {
            success = js_Invoke(cx, argc, stackbase, 0);
            result = *stackbase;
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

            XPCConvert::ConstructException(code, sz, GetInterfaceName(), name,
                                           nsnull, getter_AddRefs(e), nsnull, nsnull);
            xpcc->SetException(e);
            if(sz)
                JS_smprintf_free(sz);
            success = JS_FALSE;
        }
    }

    if(popPrincipal)
        ssm->PopContextPrincipal(ccx);

    if(!success)
    {
        PRBool forceReport;
        if(NS_FAILED(mInfo->IsFunction(&forceReport)))
            forceReport = PR_FALSE;

        
        

        retval = CheckForException(ccx, name, GetInterfaceName(), forceReport);
        goto done;
    }

    ccx.GetThreadData()->SetException(nsnull); 

    
    
    
    
    
    

    foundDependentParam = JS_FALSE;
    for(i = 0; i < paramCount; i++)
    {
        const nsXPTParamInfo& param = info->params[i];
        if(!param.IsOut() && !param.IsDipper())
            continue;

        const nsXPTType& type = param.GetType();
        if(type.IsDependent())
        {
            foundDependentParam = JS_TRUE;
            continue;
        }

        jsval val;
        uint8 type_tag = type.TagPart();
        JSBool useAllocator = JS_FALSE;
        nsXPTCMiniVariant* pv;

        if(param.IsDipper())
            pv = (nsXPTCMiniVariant*) &nativeParams[i].val.p;
        else
            pv = (nsXPTCMiniVariant*) nativeParams[i].val.p;

        if(param.IsRetval())
            val = result;
        else if(JSVAL_IS_PRIMITIVE(stackbase[i+2]) ||
                !JS_GetPropertyById(cx, JSVAL_TO_OBJECT(stackbase[i+2]),
                    mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                    &val))
            break;

        

        if(type_tag == nsXPTType::T_INTERFACE)
        {
            if(NS_FAILED(GetInterfaceInfo()->
                            GetIIDForParamNoAlloc(methodIndex, &param,
                                                  &param_iid)))
                break;
        }
        else if(type.IsPointer() && !param.IsShared() && !param.IsDipper())
            useAllocator = JS_TRUE;

        if(!XPCConvert::JSData2Native(ccx, &pv->val, val, type,
                                      useAllocator, &param_iid, nsnull))
            break;
    }

    
    if(foundDependentParam && i == paramCount)
    {
        for(i = 0; i < paramCount; i++)
        {
            const nsXPTParamInfo& param = info->params[i];
            if(!param.IsOut())
                continue;

            const nsXPTType& type = param.GetType();
            if(!type.IsDependent())
                continue;

            jsval val;
            nsXPTCMiniVariant* pv;
            nsXPTType datum_type;
            JSBool useAllocator = JS_FALSE;
            JSUint32 array_count;
            PRBool isArray = type.IsArray();
            PRBool isSizedString = isArray ?
                    JS_FALSE :
                    type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                    type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

            pv = (nsXPTCMiniVariant*) nativeParams[i].val.p;

            if(param.IsRetval())
                val = result;
            else if(!JS_GetPropertyById(cx, JSVAL_TO_OBJECT(stackbase[i+2]),
                        mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                        &val))
                break;

            

            if(isArray)
            {
                if(NS_FAILED(mInfo->GetTypeForParam(methodIndex, &param, 1,
                                                    &datum_type)))
                    break;
            }
            else
                datum_type = type;

            if(datum_type.IsInterfacePointer())
            {
               if(!GetInterfaceTypeFromParam(cx, info, param, methodIndex,
                                             datum_type, nativeParams,
                                             &param_iid))
                   break;
            }
            else if(type.IsPointer() && !param.IsShared())
                useAllocator = JS_TRUE;

            if(isArray || isSizedString)
            {
                if(!GetArraySizeFromParam(cx, info, param, methodIndex,
                                          i, GET_LENGTH, nativeParams,
                                          &array_count))
                    break;
            }

            if(isArray)
            {
                if(array_count &&
                   !XPCConvert::JSArray2Native(ccx, (void**)&pv->val, val,
                                               array_count, array_count,
                                               datum_type,
                                               useAllocator, &param_iid,
                                               nsnull))
                    break;
            }
            else if(isSizedString)
            {
                if(!XPCConvert::JSStringWithSize2Native(ccx,
                                                   (void*)&pv->val, val,
                                                   array_count, array_count,
                                                   datum_type, useAllocator,
                                                   nsnull))
                    break;
            }
            else
            {
                if(!XPCConvert::JSData2Native(ccx, &pv->val, val, type,
                                              useAllocator, &param_iid,
                                              nsnull))
                    break;
            }
        }
    }

    if(i != paramCount)
    {
        
        

        for(uint8 k = 0; k < i; k++)
        {
            const nsXPTParamInfo& param = info->params[k];
            if(!param.IsOut())
                continue;
            const nsXPTType& type = param.GetType();
            if(!type.IsPointer())
                continue;
            void* p;
            if(!(p = nativeParams[k].val.p))
                continue;

            if(type.IsArray())
            {
                void** pp;
                if(nsnull != (pp = *((void***)p)))
                {
                    
                    JSUint32 array_count;
                    nsXPTType datum_type;

                    if(NS_SUCCEEDED(mInfo->GetTypeForParam(methodIndex, &param,
                                                           1, &datum_type)) &&
                       datum_type.IsPointer() &&
                       GetArraySizeFromParam(cx, info, param, methodIndex,
                                             k, GET_LENGTH, nativeParams,
                                             &array_count) && array_count)
                    {
                        CleanupPointerArray(datum_type, array_count, pp);
                    }
                    nsMemory::Free(pp);
                }
            }
            else
                CleanupPointerTypeObject(type, (void**)p);
            *((void**)p) = nsnull;
        }
    }
    else
    {
        
        retval = pending_result;
    }

done:
    if(sp)
        js_FreeStack(cx, mark);

#ifdef DEBUG_stats_jband
    endTime = PR_IntervalNow();
    printf("%s::%s %d ( c->js ) \n", GetInterfaceName(), info->GetName(), PR_IntervalToMilliseconds(endTime-startTime));
    totalTime += endTime-startTime;
#endif
    return retval;
}

const char*
nsXPCWrappedJSClass::GetInterfaceName()
{
    if(!mName)
        mInfo->GetName(&mName);
    return mName;
}

JSObject*
nsXPCWrappedJSClass::NewOutObject(JSContext* cx, JSObject* scope)
{
    return JS_NewObject(cx, nsnull, nsnull, JS_GetGlobalForObject(cx, scope));
}


NS_IMETHODIMP
nsXPCWrappedJSClass::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("nsXPCWrappedJSClass @ %x with mRefCnt = %d", this, mRefCnt.get()));
    XPC_LOG_INDENT();
        char* name;
        mInfo->GetName(&name);
        XPC_LOG_ALWAYS(("interface name is %s", name));
        if(name)
            nsMemory::Free(name);
        char * iid = mIID.ToString();
        XPC_LOG_ALWAYS(("IID number is %s", iid ? iid : "invalid"));
        if(iid)
            NS_Free(iid);
        XPC_LOG_ALWAYS(("InterfaceInfo @ %x", mInfo));
        uint16 methodCount = 0;
        if(depth)
        {
            uint16 i;
            nsCOMPtr<nsIInterfaceInfo> parent;
            XPC_LOG_INDENT();
            mInfo->GetParent(getter_AddRefs(parent));
            XPC_LOG_ALWAYS(("parent @ %x", parent.get()));
            mInfo->GetMethodCount(&methodCount);
            XPC_LOG_ALWAYS(("MethodCount = %d", methodCount));
            mInfo->GetConstantCount(&i);
            XPC_LOG_ALWAYS(("ConstantCount = %d", i));
            XPC_LOG_OUTDENT();
        }
        XPC_LOG_ALWAYS(("mRuntime @ %x", mRuntime));
        XPC_LOG_ALWAYS(("mDescriptors @ %x count = %d", mDescriptors, methodCount));
        if(depth && mDescriptors && methodCount)
        {
            depth--;
            XPC_LOG_INDENT();
            for(uint16 i = 0; i < methodCount; i++)
            {
                XPC_LOG_ALWAYS(("Method %d is %s%s", \
                        i, IsReflectable(i) ? "":" NOT ","reflectable"));
            }
            XPC_LOG_OUTDENT();
            depth++;
        }
    XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}
