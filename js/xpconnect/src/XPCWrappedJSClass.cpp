











































#include "xpcprivate.h"
#include "nsArrayEnumerator.h"
#include "nsWrapperCache.h"
#include "XPCWrapper.h"
#include "AccessCheck.h"
#include "nsJSUtils.h"

#include "jsapi.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsXPCWrappedJSClass, nsIXPCWrappedJSClass)


static uint32_t zero_methods_descriptor;

bool AutoScriptEvaluate::StartEvaluating(JSObject *scope, JSErrorReporter errorReporter)
{
    NS_PRECONDITION(!mEvaluated, "AutoScriptEvaluate::Evaluate should only be called once");

    if (!mJSContext)
        return true;

    mEvaluated = true;
    if (!JS_GetErrorReporter(mJSContext)) {
        JS_SetErrorReporter(mJSContext, errorReporter);
        mErrorReporterSet = true;
    }

    JS_BeginRequest(mJSContext);
    if (!mEnterCompartment.enter(mJSContext, scope))
        return false;

    
    
    
    
    
    
    
    
    
    
    if (JS_IsExceptionPending(mJSContext)) {
        mState = JS_SaveExceptionState(mJSContext);
        JS_ClearPendingException(mJSContext);
    }

    return true;
}

AutoScriptEvaluate::~AutoScriptEvaluate()
{
    if (!mJSContext || !mEvaluated)
        return;
    if (mState)
        JS_RestoreExceptionState(mJSContext, mState);
    else
        JS_ClearPendingException(mJSContext);

    JS_EndRequest(mJSContext);

    
    
    
    
    
    
    

    if (JS_GetOptions(mJSContext) & JSOPTION_PRIVATE_IS_NSISUPPORTS) {
        nsCOMPtr<nsIXPCScriptNotify> scriptNotify =
            do_QueryInterface(static_cast<nsISupports*>
                                         (JS_GetContextPrivate(mJSContext)));
        if (scriptNotify)
            scriptNotify->ScriptExecuted();
    }

    if (mErrorReporterSet)
        JS_SetErrorReporter(mJSContext, NULL);
}



JSBool xpc_IsReportableErrorCode(nsresult code)
{
    if (NS_SUCCEEDED(code))
        return false;

    switch (code) {
        
        
        case NS_ERROR_FACTORY_REGISTER_AGAIN:
        case NS_BASE_STREAM_WOULD_BLOCK:
            return false;
    }

    return true;
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

    if (!clazz) {
        nsCOMPtr<nsIInterfaceInfo> info;
        ccx.GetXPConnect()->GetInfoForIID(&aIID, getter_AddRefs(info));
        if (info) {
            bool canScript, isBuiltin;
            if (NS_SUCCEEDED(info->IsScriptable(&canScript)) && canScript &&
                NS_SUCCEEDED(info->IsBuiltinClass(&isBuiltin)) && !isBuiltin &&
                nsXPConnect::IsISupportsDescendant(info)) {
                clazz = new nsXPCWrappedJSClass(ccx, aIID, info);
                if (clazz && !clazz->mDescriptors)
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

    uint16_t methodCount;
    if (NS_SUCCEEDED(mInfo->GetMethodCount(&methodCount))) {
        if (methodCount) {
            int wordCount = (methodCount/32)+1;
            if (nsnull != (mDescriptors = new uint32_t[wordCount])) {
                int i;
                
                for (i = wordCount-1; i >= 0; i--)
                    mDescriptors[i] = 0;

                for (i = 0; i < methodCount; i++) {
                    const nsXPTMethodInfo* info;
                    if (NS_SUCCEEDED(mInfo->GetMethodInfo(i, &info)))
                        SetReflectable(i, XPCConvert::IsMethodReflectable(*info));
                    else {
                        delete [] mDescriptors;
                        mDescriptors = nsnull;
                        break;
                    }
                }
            }
        } else {
            mDescriptors = &zero_methods_descriptor;
        }
    }
}

nsXPCWrappedJSClass::~nsXPCWrappedJSClass()
{
    if (mDescriptors && mDescriptors != &zero_methods_descriptor)
        delete [] mDescriptors;
    if (mRuntime)
    {   
        XPCAutoLock lock(mRuntime->GetMapLock());
        mRuntime->GetWrappedJSClassMap()->Remove(this);
    }
    if (mName)
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
    JSBool success = false;
    jsid funid;
    jsval fun;

    
    
    
    
    if (XPCPerThreadData::IsMainThread(ccx) &&
        !xpc::AccessCheck::isChrome(js::GetObjectCompartment(jsobj))) {
        return nsnull;
    }

    
    AutoScriptEvaluate scriptEval(cx);

    
    
    if (!scriptEval.StartEvaluating(jsobj))
        return nsnull;

    
    funid = mRuntime->GetStringID(XPCJSRuntime::IDX_QUERY_INTERFACE);
    if (!JS_GetPropertyById(cx, jsobj, funid, &fun) || JSVAL_IS_PRIMITIVE(fun))
        return nsnull;

    
    AUTO_MARK_JSVAL(ccx, fun);

    
    
    
    
    
    
    if (!aIID.Equals(NS_GET_IID(nsISupports))) {
        nsCOMPtr<nsIInterfaceInfo> info;
        ccx.GetXPConnect()->GetInfoForIID(&aIID, getter_AddRefs(info));
        if (!info)
            return nsnull;
        bool canScript, isBuiltin;
        if (NS_FAILED(info->IsScriptable(&canScript)) || !canScript ||
            NS_FAILED(info->IsBuiltinClass(&isBuiltin)) || isBuiltin)
            return nsnull;
    }

    id = xpc_NewIDObject(cx, jsobj, aIID);
    if (id) {
        
        
        

        uint32_t oldOpts =
          JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_DONT_REPORT_UNCAUGHT);

        jsval args[1] = {OBJECT_TO_JSVAL(id)};
        success = JS_CallFunctionValue(cx, jsobj, fun, 1, args, &retval);

        JS_SetOptions(cx, oldOpts);

        if (!success) {
            NS_ASSERTION(JS_IsExceptionPending(cx),
                         "JS failed without setting an exception!");

            jsval jsexception = JSVAL_NULL;
            AUTO_MARK_JSVAL(ccx, &jsexception);

            if (JS_GetPendingException(cx, &jsexception)) {
                nsresult rv;
                if (JSVAL_IS_OBJECT(jsexception)) {
                    
                    
                    nsCOMPtr<nsIXPConnectWrappedNative> wrapper;

                    nsXPConnect::GetXPConnect()->
                        GetWrappedNativeOfJSObject(ccx,
                                                   JSVAL_TO_OBJECT(jsexception),
                                                   getter_AddRefs(wrapper));

                    if (wrapper) {
                        nsCOMPtr<nsIException> exception =
                            do_QueryWrappedNative(wrapper);
                        if (exception &&
                            NS_SUCCEEDED(exception->GetResult(&rv)) &&
                            rv == NS_NOINTERFACE) {
                            JS_ClearPendingException(cx);
                        }
                    }
                } else if (JSVAL_IS_NUMBER(jsexception)) {
                    
                    if (JSVAL_IS_DOUBLE(jsexception))
                        rv = (nsresult)(JSVAL_TO_DOUBLE(jsexception));
                    else
                        rv = (nsresult)(JSVAL_TO_INT(jsexception));

                    if (rv == NS_NOINTERFACE)
                        JS_ClearPendingException(cx);
                }
            }

            
            if (!(oldOpts & JSOPTION_DONT_REPORT_UNCAUGHT))
                JS_ReportPendingException(cx);
        }
    }

    if (success)
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
    nsXPTType type = nsXPTType((uint8_t)TD_INTERFACE_TYPE);
    jsval val;

    return JS_GetPropertyById(ccx, aJSObj, aName, &val) &&
           
           
           
           XPCConvert::JSData2Native(ccx, aResult, val, type, true,
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
    if (!scriptEval.StartEvaluating(aJSObj))
        return NS_ERROR_FAILURE;

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

    AutoScriptEvaluate scriptEval(cx);
    if (!scriptEval.StartEvaluating(aJSObj))
        return NS_ERROR_FAILURE;

    JS::AutoIdArray idArray(cx, JS_Enumerate(cx, aJSObj));
    if (!idArray)
        return NS_ERROR_FAILURE;

    nsCOMArray<nsIProperty> propertyArray(idArray.length());
    for (size_t i = 0; i < idArray.length(); i++) {
        jsid idName = idArray[i];

        nsCOMPtr<nsIVariant> value;
        nsresult rv;
        if (!GetNamedPropertyAsVariantRaw(ccx, aJSObj, idName,
                                          getter_AddRefs(value), &rv)) {
            if (NS_FAILED(rv))
                return rv;
            return NS_ERROR_FAILURE;
        }

        jsval jsvalName;
        if (!JS_IdToValue(cx, idName, &jsvalName))
            return NS_ERROR_FAILURE;

        JSString* name = JS_ValueToString(cx, jsvalName);
        if (!name)
            return NS_ERROR_FAILURE;

        size_t length;
        const jschar *chars = JS_GetStringCharsAndLength(cx, name, &length);
        if (!chars)
            return NS_ERROR_FAILURE;

        nsCOMPtr<nsIProperty> property =
            new xpcProperty(chars, (PRUint32) length, value);

        if (!propertyArray.AppendObject(property))
            return NS_ERROR_FAILURE;
    }

    return NS_NewArrayEnumerator(aEnumerate, propertyArray);
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











#define NS_IXPCONNECT_WRAPPED_JS_IDENTITY_CLASS_IID                           \
{ 0x5c5c3bb0, 0xa9ba, 0x11d2,                                                 \
  { 0xba, 0x64, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class WrappedJSIdentity
{
    
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_IDENTITY_CLASS_IID)

    static void* GetSingleton()
    {
        static WrappedJSIdentity* singleton = nsnull;
        if (!singleton)
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

    if (stack && stack->Peek())
        return nsnull;

    
    XPCCallContext ccx(NATIVE_CALLER);
    if (!ccx.IsValid())
        return nsnull;

    JSAutoEnterCompartment ac;
    if (!ac.enter(ccx, obj))
        return nsnull;
    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, obj);
    XPCContext *xpcc = scope->GetContext();

    if (xpcc) {
        JSContext *cx = xpcc->GetJSContext();
        JS_AbortIfWrongThread(JS_GetRuntime(cx));
        return cx;
    }

    return nsnull;
}

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

NS_IMETHODIMP
nsXPCWrappedJSClass::DelegatedQueryInterface(nsXPCWrappedJS* self,
                                             REFNSIID aIID,
                                             void** aInstancePtr)
{
    if (aIID.Equals(NS_GET_IID(nsIXPConnectJSObjectHolder))) {
        NS_ADDREF(self);
        *aInstancePtr = (void*) static_cast<nsIXPConnectJSObjectHolder*>(self);
        return NS_OK;
    }

    
    
    if (aIID.Equals(NS_GET_IID(WrappedJSIdentity))) {
        
        *aInstancePtr = WrappedJSIdentity::GetSingleton();
        return NS_OK;
    }

    if (aIID.Equals(NS_GET_IID(nsIPropertyBag))) {
        
        nsXPCWrappedJS* root = self->GetRootWrapper();

        if (!root->IsValid()) {
            *aInstancePtr = nsnull;
            return NS_NOINTERFACE;
        }

        NS_ADDREF(root);
        *aInstancePtr = (void*) static_cast<nsIPropertyBag*>(root);
        return NS_OK;
    }

    
    if (aIID.Equals(NS_GET_IID(nsWrapperCache))) {
        *aInstancePtr = nsnull;
        return NS_NOINTERFACE;
    }

    JSContext *context = GetContextFromObject(self->GetJSObject());
    XPCCallContext ccx(NATIVE_CALLER, context);
    if (!ccx.IsValid()) {
        *aInstancePtr = nsnull;
        return NS_NOINTERFACE;
    }

    
    
    if (aIID.Equals(NS_GET_IID(nsISupportsWeakReference))) {
        
        nsXPCWrappedJS* root = self->GetRootWrapper();

        
        if (!root->IsValid() ||
            !CallQueryInterfaceOnJSObject(ccx, root->GetJSObject(), aIID)) {
            *aInstancePtr = nsnull;
            return NS_NOINTERFACE;
        }

        NS_ADDREF(root);
        *aInstancePtr = (void*) static_cast<nsISupportsWeakReference*>(root);
        return NS_OK;
    }

    nsXPCWrappedJS* sibling;

    
    
    
    if (nsnull != (sibling = self->Find(aIID))) {
        NS_ADDREF(sibling);
        *aInstancePtr = sibling->GetXPTCStub();
        return NS_OK;
    }

    
    if (nsnull != (sibling = self->FindInherited(aIID))) {
        NS_ADDREF(sibling);
        *aInstancePtr = sibling->GetXPTCStub();
        return NS_OK;
    }

    

    
    
    
    
    
    
    
    
    
    
    

    if (aIID.Equals(NS_GET_IID(nsISecurityCheckedComponent))) {
        
        
        

        *aInstancePtr = nsnull;

        if (!XPCPerThreadData::IsMainThread(ccx.GetJSContext()))
            return NS_NOINTERFACE;

        nsXPConnect *xpc = nsXPConnect::GetXPConnect();
        nsCOMPtr<nsIScriptSecurityManager> secMan =
            do_QueryInterface(xpc->GetDefaultSecurityManager());
        if (!secMan)
            return NS_NOINTERFACE;

        JSObject *selfObj = self->GetJSObject();
        nsCOMPtr<nsIPrincipal> objPrin;
        nsresult rv = secMan->GetObjectPrincipal(ccx, selfObj,
                                                 getter_AddRefs(objPrin));
        if (NS_FAILED(rv))
            return rv;

        bool isSystem;
        rv = secMan->IsSystemPrincipal(objPrin, &isSystem);
        if ((NS_FAILED(rv) || !isSystem) &&
            !IS_WRAPPER_CLASS(js::GetObjectClass(selfObj))) {
            
            nsRefPtr<SameOriginCheckedComponent> checked =
                new SameOriginCheckedComponent(self);
            if (!checked)
                return NS_ERROR_OUT_OF_MEMORY;
            *aInstancePtr = checked.forget().get();
            return NS_OK;
        }
    }

    
    JSObject* jsobj = CallQueryInterfaceOnJSObject(ccx, self->GetJSObject(),
                                                   aIID);
    if (jsobj) {
        
        AUTO_MARK_JSVAL(ccx, OBJECT_TO_JSVAL(jsobj));

        
        
        
        
        
        
        
        
        
        
        
        nsXPCWrappedJS* wrapper;
        nsresult rv = nsXPCWrappedJS::GetNewOrUsed(ccx, jsobj, aIID, nsnull,
                                                   &wrapper);
        if (NS_SUCCEEDED(rv) && wrapper) {
            
            
            
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
    if (!result)
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
    if (report) {
        
        
        if (JSREPORT_IS_EXCEPTION(report->flags)) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            return;
        }

        if (JSREPORT_IS_WARNING(report->flags)) {
            
            
            return;
        }
    }

    XPCCallContext ccx(NATIVE_CALLER, cx);
    if (!ccx.IsValid())
        return;

    nsCOMPtr<nsIException> e;
    XPCConvert::JSErrorToXPCException(ccx, message, nsnull, nsnull, report,
                                      getter_AddRefs(e));
    if (e)
        ccx.GetXPCContext()->SetException(e);
}

JSBool
nsXPCWrappedJSClass::GetArraySizeFromParam(JSContext* cx,
                                           const XPTMethodDescriptor* method,
                                           const nsXPTParamInfo& param,
                                           uint16_t methodIndex,
                                           uint8_t paramIndex,
                                           nsXPTCMiniVariant* nativeParams,
                                           uint32_t* result)
{
    uint8_t argnum;
    nsresult rv;

    rv = mInfo->GetSizeIsArgNumberForParam(methodIndex, &param, 0, &argnum);
    if (NS_FAILED(rv))
        return false;

    const nsXPTParamInfo& arg_param = method->params[argnum];
    const nsXPTType& arg_type = arg_param.GetType();

    
    
    NS_ABORT_IF_FALSE(arg_type.TagPart() == nsXPTType::T_U32,
                      "size_is references parameter of invalid type.");

    if (arg_param.IsIndirect())
        *result = *(uint32_t*)nativeParams[argnum].val.p;
    else
        *result = nativeParams[argnum].val.u32;

    return true;
}

JSBool
nsXPCWrappedJSClass::GetInterfaceTypeFromParam(JSContext* cx,
                                               const XPTMethodDescriptor* method,
                                               const nsXPTParamInfo& param,
                                               uint16_t methodIndex,
                                               const nsXPTType& type,
                                               nsXPTCMiniVariant* nativeParams,
                                               nsID* result)
{
    uint8_t type_tag = type.TagPart();

    if (type_tag == nsXPTType::T_INTERFACE) {
        if (NS_SUCCEEDED(GetInterfaceInfo()->
                         GetIIDForParamNoAlloc(methodIndex, &param, result))) {
            return true;
        }
    } else if (type_tag == nsXPTType::T_INTERFACE_IS) {
        uint8_t argnum;
        nsresult rv;
        rv = mInfo->GetInterfaceIsArgNumberForParam(methodIndex,
                                                    &param, &argnum);
        if (NS_FAILED(rv))
            return false;

        const nsXPTParamInfo& arg_param = method->params[argnum];
        const nsXPTType& arg_type = arg_param.GetType();

        if (arg_type.TagPart() == nsXPTType::T_IID) {
            if (arg_param.IsIndirect()) {
                nsID** p = (nsID**) nativeParams[argnum].val.p;
                if (!p || !*p)
                    return false;
                *result = **p;
            } else {
                nsID* p = (nsID*) nativeParams[argnum].val.p;
                if (!p)
                    return false;
                *result = *p;
            }
            return true;
        }
    }
    return false;
}

void
nsXPCWrappedJSClass::CleanupPointerArray(const nsXPTType& datum_type,
                                         uint32_t array_count,
                                         void** arrayp)
{
    if (datum_type.IsInterfacePointer()) {
        nsISupports** pp = (nsISupports**) arrayp;
        for (uint32_t k = 0; k < array_count; k++) {
            nsISupports* p = pp[k];
            NS_IF_RELEASE(p);
        }
    } else {
        void** pp = (void**) arrayp;
        for (uint32_t k = 0; k < array_count; k++) {
            void* p = pp[k];
            if (p) nsMemory::Free(p);
        }
    }
}

void
nsXPCWrappedJSClass::CleanupPointerTypeObject(const nsXPTType& type,
                                              void** pp)
{
    NS_ASSERTION(pp,"null pointer");
    if (type.IsInterfacePointer()) {
        nsISupports* p = *((nsISupports**)pp);
        if (p) p->Release();
    } else {
        void* p = *((void**)pp);
        if (p) nsMemory::Free(p);
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
                                       bool aForceReport)
{
    XPCContext * xpcc = ccx.GetXPCContext();
    JSContext * cx = ccx.GetJSContext();
    nsCOMPtr<nsIException> xpc_exception;
    

    xpcc->GetException(getter_AddRefs(xpc_exception));
    if (xpc_exception)
        xpcc->SetException(nsnull);

    
    
    nsresult pending_result = xpcc->GetPendingResult();

    jsval js_exception;
    JSBool is_js_exception = JS_GetPendingException(cx, &js_exception);

    
    if (is_js_exception) {
        if (!xpc_exception)
            XPCConvert::JSValToXPCException(ccx, js_exception, anInterfaceName,
                                            aPropertyName,
                                            getter_AddRefs(xpc_exception));

        
        if (!xpc_exception) {
            ccx.GetThreadData()->SetException(nsnull); 
        }
    }

    AutoClearPendingException acpe(cx);

    if (xpc_exception) {
        nsresult e_result;
        if (NS_SUCCEEDED(xpc_exception->GetResult(&e_result))) {
            
            bool reportable = xpc_IsReportableErrorCode(e_result);
            if (reportable) {
                
                
                reportable = aForceReport ||
                    NS_ERROR_GET_MODULE(e_result) == NS_ERROR_MODULE_XPCONNECT;

                
                
                
                if (!reportable)
                    reportable = nsXPConnect::ReportAllJSExceptions();

                
                
                if (!reportable) {
                    bool onlyNativeStackFrames = true;
                    JSStackFrame * fp = nsnull;
                    while ((fp = JS_FrameIterator(cx, &fp))) {
                        if (JS_IsScriptFrame(cx, fp)) {
                            onlyNativeStackFrames = false;
                            break;
                        }
                    }
                    reportable = onlyNativeStackFrames;
                }

                
                
                
                
                
                if (reportable && e_result == NS_ERROR_NO_INTERFACE &&
                    !strcmp(anInterfaceName, "nsIInterfaceRequestor") &&
                    !strcmp(aPropertyName, "getInterface")) {
                    reportable = false;
                }
            }

            
            
            if (reportable && is_js_exception &&
                JS_GetErrorReporter(cx) != xpcWrappedJSErrorReporter) {
                reportable = !JS_ReportPendingException(cx);
            }

            if (reportable) {
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
                if (NS_SUCCEEDED(xpc_exception->ToString(&text)) && text) {
                    fputs(text, stdout);
                    fputs("\n", stdout);
                    nsMemory::Free(text);
                } else
                    fputs(cant_get_text, stdout);
                fputs(line, stdout);
#endif

                
                
                nsCOMPtr<nsIConsoleService> consoleService
                    (do_GetService(XPC_CONSOLE_CONTRACTID));
                if (nsnull != consoleService) {
                    nsresult rv;
                    nsCOMPtr<nsIScriptError> scriptError;
                    nsCOMPtr<nsISupports> errorData;
                    rv = xpc_exception->GetData(getter_AddRefs(errorData));
                    if (NS_SUCCEEDED(rv))
                        scriptError = do_QueryInterface(errorData);

                    if (nsnull == scriptError) {
                        
                        
                        scriptError = do_CreateInstance(XPC_SCRIPT_ERROR_CONTRACTID);
                        if (nsnull != scriptError) {
                            char* exn_string;
                            rv = xpc_exception->ToString(&exn_string);
                            if (NS_SUCCEEDED(rv)) {
                                
                                nsAutoString newMessage;
                                newMessage.AssignWithConversion(exn_string);
                                nsMemory::Free((void *) exn_string);

                                
                                
                                PRInt32 lineNumber = 0;
                                nsXPIDLCString sourceName;

                                nsCOMPtr<nsIStackFrame> location;
                                xpc_exception->
                                    GetLocation(getter_AddRefs(location));
                                if (location) {
                                    
                                    location->GetLineNumber(&lineNumber);

                                    
                                    rv = location->GetFilename(getter_Copies(sourceName));
                                }

                                rv = scriptError->InitWithWindowID(newMessage.get(),
                                                                   NS_ConvertASCIItoUTF16(sourceName).get(),
                                                                   nsnull,
                                                                   lineNumber, 0, 0,
                                                                   "XPConnect JavaScript",
                                                                   nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx));
                                if (NS_FAILED(rv))
                                    scriptError = nsnull;
                            }
                        }
                    }
                    if (nsnull != scriptError)
                        consoleService->LogMessage(scriptError);
                }
            }
            
            
            if (NS_FAILED(e_result)) {
                ccx.GetThreadData()->SetException(xpc_exception);
                return e_result;
            }
        }
    } else {
        
        if (NS_FAILED(pending_result)) {
            return pending_result;
        }
    }
    return NS_ERROR_FAILURE;
}

class ContextPrincipalGuard
{
    nsIScriptSecurityManager *ssm;
    XPCCallContext &ccx;
  public:
    ContextPrincipalGuard(XPCCallContext &ccx)
      : ssm(nsnull), ccx(ccx) {}
    void principalPushed(nsIScriptSecurityManager *ssm) { this->ssm = ssm; }
    ~ContextPrincipalGuard() { if (ssm) ssm->PopContextPrincipal(ccx); }
};

NS_IMETHODIMP
nsXPCWrappedJSClass::CallMethod(nsXPCWrappedJS* wrapper, uint16_t methodIndex,
                                const XPTMethodDescriptor* info,
                                nsXPTCMiniVariant* nativeParams)
{
    jsval* sp = nsnull;
    jsval* argv = nsnull;
    uint8_t i;
    nsresult retval = NS_ERROR_FAILURE;
    nsresult pending_result = NS_OK;
    JSBool success;
    JSBool readyToDoTheCall = false;
    nsID  param_iid;
    const char* name = info->name;
    jsval fval;
    JSBool foundDependentParam;

    
    
    
    
    JSContext *context = GetContextFromObject(wrapper->GetJSObject());
    XPCCallContext ccx(NATIVE_CALLER, context);
    if (!ccx.IsValid())
        return retval;

    XPCContext *xpcc = ccx.GetXPCContext();
    JSContext *cx = ccx.GetJSContext();

    if (!cx || !xpcc || !IsReflectable(methodIndex))
        return NS_ERROR_FAILURE;

    JSObject *obj = wrapper->GetJSObject();
    JSObject *thisObj = obj;

    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, obj))
        return NS_ERROR_FAILURE;

    ccx.SetScopeForNewJSObjects(obj);

    JS::AutoValueVector args(cx);
    AutoScriptEvaluate scriptEval(cx);
    ContextPrincipalGuard principalGuard(ccx);

    
    uint8_t paramCount = info->num_args;
    uint8_t argc = paramCount -
        (paramCount && XPT_PD_IS_RETVAL(info->params[paramCount-1].flags) ? 1 : 0);

    if (!scriptEval.StartEvaluating(obj, xpcWrappedJSErrorReporter))
        goto pre_call_clean_up;

    xpcc->SetPendingResult(pending_result);
    xpcc->SetException(nsnull);
    ccx.GetThreadData()->SetException(nsnull);

    if (XPCPerThreadData::IsMainThread(ccx)) {
        
        nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
        if (ssm) {
            nsIPrincipal *objPrincipal =
                xpc::AccessCheck::getPrincipal(js::GetObjectCompartment(obj));
            if (objPrincipal) {
                JSStackFrame* fp = nsnull;
                nsresult rv =
                    ssm->PushContextPrincipal(ccx, JS_FrameIterator(ccx, &fp),
                                              objPrincipal);
                if (NS_FAILED(rv)) {
                    JS_ReportOutOfMemory(ccx);
                    retval = NS_ERROR_OUT_OF_MEMORY;
                    goto pre_call_clean_up;
                }

                principalGuard.principalPushed(ssm);
            }
        }
    }

    
    

    

    
    if (!(XPT_MD_IS_SETTER(info->flags) || XPT_MD_IS_GETTER(info->flags))) {
        
        
        

        
        

        bool isFunction;
        if (NS_FAILED(mInfo->IsFunction(&isFunction)))
            goto pre_call_clean_up;

        
        
        
        
        
        
        
        
        
        

        if (isFunction &&
            JS_TypeOfValue(ccx, OBJECT_TO_JSVAL(obj)) == JSTYPE_FUNCTION) {
            fval = OBJECT_TO_JSVAL(obj);

            

            if (paramCount) {
                const nsXPTParamInfo& firstParam = info->params[0];
                if (firstParam.IsIn()) {
                    const nsXPTType& firstType = firstParam.GetType();

                    if (firstType.IsInterfacePointer()) {
                        nsIXPCFunctionThisTranslator* translator;

                        IID2ThisTranslatorMap* map =
                            mRuntime->GetThisTranslatorMap();

                        {
                            XPCAutoLock lock(mRuntime->GetMapLock()); 
                            translator = map->Find(mIID);
                        }

                        if (translator) {
                            bool hideFirstParamFromJS = false;
                            nsIID* newWrapperIID = nsnull;
                            nsCOMPtr<nsISupports> newThis;

                            if (NS_FAILED(translator->
                                          TranslateThis((nsISupports*)nativeParams[0].val.p,
                                                        mInfo, methodIndex,
                                                        &hideFirstParamFromJS,
                                                        &newWrapperIID,
                                                        getter_AddRefs(newThis)))) {
                                goto pre_call_clean_up;
                            }
                            if (hideFirstParamFromJS) {
                                NS_ERROR("HideFirstParamFromJS not supported");
                                goto pre_call_clean_up;
                            }
                            if (newThis) {
                                jsval v;
                                xpcObjectHelper helper(newThis);
                                JSBool ok =
                                  XPCConvert::NativeInterface2JSObject(ccx,
                                                                       &v, nsnull, helper, newWrapperIID,
                                                                       nsnull, false, false,
                                                                       nsnull);
                                if (newWrapperIID)
                                    nsMemory::Free(newWrapperIID);
                                if (!ok) {
                                    goto pre_call_clean_up;
                                }
                                thisObj = JSVAL_TO_OBJECT(v);
                                if (!JS_WrapObject(cx, &thisObj))
                                    goto pre_call_clean_up;
                            }
                        }
                    }
                }
            }
        } else if (!JS_GetMethod(cx, obj, name, &thisObj, &fval)) {
            
            
            
            
            
            goto pre_call_clean_up;
        }
    }

    if (!args.resize(argc)) {
        retval = NS_ERROR_OUT_OF_MEMORY;
        goto pre_call_clean_up;
    }

    argv = args.begin();
    sp = argv;

    
    
    
    
    
    
    for (i = 0; i < argc; i++) {
        const nsXPTParamInfo& param = info->params[i];
        const nsXPTType& type = param.GetType();
        nsXPTType datum_type;
        uint32_t array_count;
        bool isArray = type.IsArray();
        jsval val = JSVAL_NULL;
        AUTO_MARK_JSVAL(ccx, &val);
        bool isSizedString = isArray ?
                false :
                type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;


        
        if (param.IsOut() && !nativeParams[i].val.p) {
            retval = NS_ERROR_INVALID_ARG;
            goto pre_call_clean_up;
        }

        if (isArray) {
            if (NS_FAILED(mInfo->GetTypeForParam(methodIndex, &param, 1,
                                                 &datum_type)))
                goto pre_call_clean_up;
        } else
            datum_type = type;

        if (param.IsIn()) {
            nsXPTCMiniVariant* pv;

            if (param.IsIndirect())
                pv = (nsXPTCMiniVariant*) nativeParams[i].val.p;
            else
                pv = &nativeParams[i];

            if (datum_type.IsInterfacePointer() &&
                !GetInterfaceTypeFromParam(cx, info, param, methodIndex,
                                           datum_type, nativeParams,
                                           &param_iid))
                goto pre_call_clean_up;

            if (isArray || isSizedString) {
                if (!GetArraySizeFromParam(cx, info, param, methodIndex,
                                           i, nativeParams, &array_count))
                    goto pre_call_clean_up;
            }

            if (isArray) {
                XPCLazyCallContext lccx(ccx);
                if (!XPCConvert::NativeArray2JS(lccx, &val,
                                                (const void**)&pv->val,
                                                datum_type, &param_iid,
                                                array_count, nsnull))
                    goto pre_call_clean_up;
            } else if (isSizedString) {
                if (!XPCConvert::NativeStringWithSize2JS(ccx, &val,
                                                         (const void*)&pv->val,
                                                         datum_type,
                                                         array_count, nsnull))
                    goto pre_call_clean_up;
            } else {
                if (!XPCConvert::NativeData2JS(ccx, &val, &pv->val, type,
                                               &param_iid, nsnull))
                    goto pre_call_clean_up;
            }
        }

        if (param.IsOut() || param.IsDipper()) {
            
            JSObject* out_obj = NewOutObject(cx, obj);
            if (!out_obj) {
                retval = NS_ERROR_OUT_OF_MEMORY;
                goto pre_call_clean_up;
            }

            if (param.IsIn()) {
                if (!JS_SetPropertyById(cx, out_obj,
                                        mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                                        &val)) {
                    goto pre_call_clean_up;
                }
            }
            *sp++ = OBJECT_TO_JSVAL(out_obj);
        } else
            *sp++ = val;
    }

    readyToDoTheCall = true;

pre_call_clean_up:
    
    for (i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->params[i];
        if (!param.IsOut())
            continue;

        const nsXPTType& type = param.GetType();
        if (!type.deprecated_IsPointer())
            continue;
        void* p;
        if (!(p = nativeParams[i].val.p))
            continue;

        if (param.IsIn()) {
            if (type.IsArray()) {
                void** pp;
                if (nsnull != (pp = *((void***)p))) {

                    
                    uint32_t array_count;
                    nsXPTType datum_type;

                    if (NS_SUCCEEDED(mInfo->GetTypeForParam(methodIndex, &param,
                                                            1, &datum_type)) &&
                        datum_type.deprecated_IsPointer() &&
                        GetArraySizeFromParam(cx, info, param, methodIndex,
                                              i, nativeParams, &array_count) &&
                        array_count) {

                        CleanupPointerArray(datum_type, array_count, pp);
                    }

                    
                    nsMemory::Free(pp);
                }
            } else
                CleanupPointerTypeObject(type, (void**)p);
        }
        *((void**)p) = nsnull;
    }

    
    nsCOMPtr<nsIXPCWrappedJSClass> kungFuDeathGrip(this);

    if (!readyToDoTheCall)
        return retval;

    

    JS_ClearPendingException(cx);

    jsval rval;
    if (XPT_MD_IS_GETTER(info->flags)) {
        success = JS_GetProperty(cx, obj, name, argv);
        rval = *argv;
    } else if (XPT_MD_IS_SETTER(info->flags)) {
        success = JS_SetProperty(cx, obj, name, argv);
        rval = *argv;
    } else {
        if (!JSVAL_IS_PRIMITIVE(fval)) {
            uint32_t oldOpts = JS_GetOptions(cx);
            JS_SetOptions(cx, oldOpts | JSOPTION_DONT_REPORT_UNCAUGHT);

            success = JS_CallFunctionValue(cx, thisObj, fval, argc, argv, &rval);

            JS_SetOptions(cx, oldOpts);
        } else {
            
            

            static const nsresult code =
                    NS_ERROR_XPC_JSOBJECT_HAS_NO_FUNCTION_NAMED;
            static const char format[] = "%s \"%s\"";
            const char * msg;
            char* sz = nsnull;

            if (nsXPCException::NameAndFormatForNSResult(code, nsnull, &msg) && msg)
                sz = JS_smprintf(format, msg, name);

            nsCOMPtr<nsIException> e;

            XPCConvert::ConstructException(code, sz, GetInterfaceName(), name,
                                           nsnull, getter_AddRefs(e), nsnull, nsnull);
            xpcc->SetException(e);
            if (sz)
                JS_smprintf_free(sz);
            success = false;
        }
    }

    if (!success) {
        bool forceReport;
        if (NS_FAILED(mInfo->IsFunction(&forceReport)))
            forceReport = false;

        
        

        return CheckForException(ccx, name, GetInterfaceName(), forceReport);
    }

    ccx.GetThreadData()->SetException(nsnull); 

    
    
    
    
    
    

    foundDependentParam = false;
    for (i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->params[i];
        NS_ABORT_IF_FALSE(!param.IsShared(), "[shared] implies [noscript]!");
        if (!param.IsOut() && !param.IsDipper())
            continue;

        const nsXPTType& type = param.GetType();
        if (type.IsDependent()) {
            foundDependentParam = true;
            continue;
        }

        jsval val;
        uint8_t type_tag = type.TagPart();
        nsXPTCMiniVariant* pv;

        if (param.IsDipper())
            pv = (nsXPTCMiniVariant*) &nativeParams[i].val.p;
        else
            pv = (nsXPTCMiniVariant*) nativeParams[i].val.p;

        if (param.IsRetval())
            val = rval;
        else if (JSVAL_IS_PRIMITIVE(argv[i]) ||
                 !JS_GetPropertyById(cx, JSVAL_TO_OBJECT(argv[i]),
                                     mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                                     &val))
            break;

        

        if (type_tag == nsXPTType::T_INTERFACE) {
            if (NS_FAILED(GetInterfaceInfo()->
                          GetIIDForParamNoAlloc(methodIndex, &param,
                                                &param_iid)))
                break;
        }

        if (!XPCConvert::JSData2Native(ccx, &pv->val, val, type,
                                       !param.IsDipper(), &param_iid, nsnull))
            break;
    }

    
    if (foundDependentParam && i == paramCount) {
        for (i = 0; i < paramCount; i++) {
            const nsXPTParamInfo& param = info->params[i];
            if (!param.IsOut())
                continue;

            const nsXPTType& type = param.GetType();
            if (!type.IsDependent())
                continue;

            jsval val;
            nsXPTCMiniVariant* pv;
            nsXPTType datum_type;
            uint32_t array_count;
            bool isArray = type.IsArray();
            bool isSizedString = isArray ?
                    false :
                    type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                    type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

            pv = (nsXPTCMiniVariant*) nativeParams[i].val.p;

            if (param.IsRetval())
                val = rval;
            else if (!JS_GetPropertyById(cx, JSVAL_TO_OBJECT(argv[i]),
                                         mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                                         &val))
                break;

            

            if (isArray) {
                if (NS_FAILED(mInfo->GetTypeForParam(methodIndex, &param, 1,
                                                     &datum_type)))
                    break;
            } else
                datum_type = type;

            if (datum_type.IsInterfacePointer()) {
               if (!GetInterfaceTypeFromParam(cx, info, param, methodIndex,
                                              datum_type, nativeParams,
                                              &param_iid))
                   break;
            }

            if (isArray || isSizedString) {
                if (!GetArraySizeFromParam(cx, info, param, methodIndex,
                                           i, nativeParams, &array_count))
                    break;
            }

            if (isArray) {
                if (array_count &&
                    !XPCConvert::JSArray2Native(ccx, (void**)&pv->val, val,
                                                array_count, datum_type,
                                                &param_iid, nsnull))
                    break;
            } else if (isSizedString) {
                if (!XPCConvert::JSStringWithSize2Native(ccx,
                                                         (void*)&pv->val, val,
                                                         array_count, datum_type,
                                                         nsnull))
                    break;
            } else {
                if (!XPCConvert::JSData2Native(ccx, &pv->val, val, type,
                                               true, &param_iid,
                                               nsnull))
                    break;
            }
        }
    }

    if (i != paramCount) {
        
        

        for (uint8_t k = 0; k < i; k++) {
            const nsXPTParamInfo& param = info->params[k];
            if (!param.IsOut())
                continue;
            const nsXPTType& type = param.GetType();
            if (!type.deprecated_IsPointer())
                continue;
            void* p;
            if (!(p = nativeParams[k].val.p))
                continue;

            if (type.IsArray()) {
                void** pp;
                if (nsnull != (pp = *((void***)p))) {
                    
                    uint32_t array_count;
                    nsXPTType datum_type;

                    if (NS_SUCCEEDED(mInfo->GetTypeForParam(methodIndex, &param,
                                                            1, &datum_type)) &&
                        datum_type.deprecated_IsPointer() &&
                        GetArraySizeFromParam(cx, info, param, methodIndex,
                                              k, nativeParams, &array_count) &&
                        array_count) {

                        CleanupPointerArray(datum_type, array_count, pp);
                    }
                    nsMemory::Free(pp);
                }
            } else
                CleanupPointerTypeObject(type, (void**)p);
            *((void**)p) = nsnull;
        }
    } else {
        
        retval = pending_result;
    }

    return retval;
}

const char*
nsXPCWrappedJSClass::GetInterfaceName()
{
    if (!mName)
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
        if (name)
            nsMemory::Free(name);
        char * iid = mIID.ToString();
        XPC_LOG_ALWAYS(("IID number is %s", iid ? iid : "invalid"));
        if (iid)
            NS_Free(iid);
        XPC_LOG_ALWAYS(("InterfaceInfo @ %x", mInfo));
        uint16_t methodCount = 0;
        if (depth) {
            uint16_t i;
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
        if (depth && mDescriptors && methodCount) {
            depth--;
            XPC_LOG_INDENT();
            for (uint16_t i = 0; i < methodCount; i++) {
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
