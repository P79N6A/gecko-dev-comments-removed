







#include "xpcprivate.h"
#include "jsprf.h"
#include "nsArrayEnumerator.h"
#include "nsContentUtils.h"
#include "nsWrapperCache.h"
#include "AccessCheck.h"
#include "nsJSUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/DOMExceptionBinding.h"
#include "mozilla/jsipc/CrossProcessObjectWrappers.h"

#include "jsapi.h"
#include "jsfriendapi.h"

using namespace xpc;
using namespace JS;
using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(nsXPCWrappedJSClass, nsIXPCWrappedJSClass)


static uint32_t zero_methods_descriptor;

bool AutoScriptEvaluate::StartEvaluating(HandleObject scope)
{
    NS_PRECONDITION(!mEvaluated, "AutoScriptEvaluate::Evaluate should only be called once");

    if (!mJSContext)
        return true;

    mEvaluated = true;

    JS_BeginRequest(mJSContext);
    mAutoCompartment.emplace(mJSContext, scope);

    
    
    
    
    
    
    mState.emplace(mJSContext);

    return true;
}

AutoScriptEvaluate::~AutoScriptEvaluate()
{
    if (!mJSContext || !mEvaluated)
        return;
    mState->restore();

    JS_EndRequest(mJSContext);
}



bool xpc_IsReportableErrorCode(nsresult code)
{
    if (NS_SUCCEEDED(code))
        return false;

    switch (code) {
        
        
        case NS_ERROR_FACTORY_REGISTER_AGAIN:
        case NS_BASE_STREAM_WOULD_BLOCK:
            return false;
        default:
            return true;
    }
}



class MOZ_STACK_CLASS AutoSavePendingResult {
public:
    explicit AutoSavePendingResult(XPCContext* xpcc) :
        mXPCContext(xpcc)
    {
        
        mSavedResult = xpcc->GetPendingResult();
        xpcc->SetPendingResult(NS_OK);
    }
    ~AutoSavePendingResult() {
        mXPCContext->SetPendingResult(mSavedResult);
    }
private:
    XPCContext* mXPCContext;
    nsresult mSavedResult;
};


already_AddRefed<nsXPCWrappedJSClass>
nsXPCWrappedJSClass::GetNewOrUsed(JSContext* cx, REFNSIID aIID, bool allowNonScriptable)
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    IID2WrappedJSClassMap* map = rt->GetWrappedJSClassMap();
    nsRefPtr<nsXPCWrappedJSClass> clasp = map->Find(aIID);

    if (!clasp) {
        nsCOMPtr<nsIInterfaceInfo> info;
        nsXPConnect::XPConnect()->GetInfoForIID(&aIID, getter_AddRefs(info));
        if (info) {
            bool canScript, isBuiltin;
            if (NS_SUCCEEDED(info->IsScriptable(&canScript)) && (canScript || allowNonScriptable) &&
                NS_SUCCEEDED(info->IsBuiltinClass(&isBuiltin)) && !isBuiltin &&
                nsXPConnect::IsISupportsDescendant(info))
            {
                clasp = new nsXPCWrappedJSClass(cx, aIID, info);
                if (!clasp->mDescriptors)
                    clasp = nullptr;
            }
        }
    }
    return clasp.forget();
}

nsXPCWrappedJSClass::nsXPCWrappedJSClass(JSContext* cx, REFNSIID aIID,
                                         nsIInterfaceInfo* aInfo)
    : mRuntime(nsXPConnect::GetRuntimeInstance()),
      mInfo(aInfo),
      mName(nullptr),
      mIID(aIID),
      mDescriptors(nullptr)
{
    mRuntime->GetWrappedJSClassMap()->Add(this);

    uint16_t methodCount;
    if (NS_SUCCEEDED(mInfo->GetMethodCount(&methodCount))) {
        if (methodCount) {
            int wordCount = (methodCount/32)+1;
            if (nullptr != (mDescriptors = new uint32_t[wordCount])) {
                int i;
                
                for (i = wordCount-1; i >= 0; i--)
                    mDescriptors[i] = 0;

                for (i = 0; i < methodCount; i++) {
                    const nsXPTMethodInfo* info;
                    if (NS_SUCCEEDED(mInfo->GetMethodInfo(i, &info)))
                        SetReflectable(i, XPCConvert::IsMethodReflectable(*info));
                    else {
                        delete [] mDescriptors;
                        mDescriptors = nullptr;
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
        mRuntime->GetWrappedJSClassMap()->Remove(this);

    if (mName)
        free(mName);
}

JSObject*
nsXPCWrappedJSClass::CallQueryInterfaceOnJSObject(JSContext* cx,
                                                  JSObject* jsobjArg,
                                                  REFNSIID aIID)
{
    RootedObject jsobj(cx, jsobjArg);
    JSObject* id;
    RootedValue retval(cx);
    RootedObject retObj(cx);
    bool success = false;
    RootedValue fun(cx);

    
    
    
    
    
    
    
    if (!AccessCheck::isChrome(jsobj) ||
        !AccessCheck::isChrome(js::UncheckedUnwrap(jsobj)))
    {
        return nullptr;
    }

    
    AutoScriptEvaluate scriptEval(cx);

    
    
    if (!scriptEval.StartEvaluating(jsobj))
        return nullptr;

    
    HandleId funid = mRuntime->GetStringID(XPCJSRuntime::IDX_QUERY_INTERFACE);
    if (!JS_GetPropertyById(cx, jsobj, funid, &fun) || fun.isPrimitive())
        return nullptr;

    
    
    
    
    
    
    if (!aIID.Equals(NS_GET_IID(nsISupports))) {
        bool allowNonScriptable = mozilla::jsipc::IsWrappedCPOW(jsobj);

        nsCOMPtr<nsIInterfaceInfo> info;
        nsXPConnect::XPConnect()->GetInfoForIID(&aIID, getter_AddRefs(info));
        if (!info)
            return nullptr;
        bool canScript, isBuiltin;
        if (NS_FAILED(info->IsScriptable(&canScript)) || (!canScript && !allowNonScriptable) ||
            NS_FAILED(info->IsBuiltinClass(&isBuiltin)) || isBuiltin)
            return nullptr;
    }

    id = xpc_NewIDObject(cx, jsobj, aIID);
    if (id) {
        
        
        

        {
            AutoSaveContextOptions asco(cx);
            ContextOptionsRef(cx).setDontReportUncaught(true);
            RootedValue arg(cx, JS::ObjectValue(*id));
            success = JS_CallFunctionValue(cx, jsobj, fun, HandleValueArray(arg), &retval);
        }

        if (!success && JS_IsExceptionPending(cx)) {
            RootedValue jsexception(cx, NullValue());

            if (JS_GetPendingException(cx, &jsexception)) {
                nsresult rv;
                if (jsexception.isObject()) {
                    
                    
                    Exception* e = nullptr;
                    UNWRAP_OBJECT(Exception, &jsexception.toObject(), e);

                    if (e &&
                        NS_SUCCEEDED(e->GetResult(&rv)) &&
                        rv == NS_NOINTERFACE) {
                        JS_ClearPendingException(cx);
                    }
                } else if (jsexception.isNumber()) {
                    
                    if (jsexception.isDouble())
                        
                        
                        
                        rv = (nsresult)(uint32_t)(jsexception.toDouble());
                    else
                        rv = (nsresult)(jsexception.toInt32());

                    if (rv == NS_NOINTERFACE)
                        JS_ClearPendingException(cx);
                }
            }

            
            if (!ContextOptionsRef(cx).dontReportUncaught() &&
                !ContextOptionsRef(cx).autoJSAPIOwnsErrorReporting())
                JS_ReportPendingException(cx);
        } else if (!success) {
            NS_WARNING("QI hook ran OOMed - this is probably a bug!");
        }
    }

    if (success)
        success = JS_ValueToObject(cx, retval, &retObj);

    return success ? retObj.get() : nullptr;
}



static bool
GetNamedPropertyAsVariantRaw(XPCCallContext& ccx,
                             HandleObject aJSObj,
                             HandleId aName,
                             nsIVariant** aResult,
                             nsresult* pErr)
{
    nsXPTType type = nsXPTType((uint8_t)TD_INTERFACE_TYPE);
    RootedValue val(ccx);

    return JS_GetPropertyById(ccx, aJSObj, aName, &val) &&
           XPCConvert::JSData2Native(aResult, val, type,
                                     &NS_GET_IID(nsIVariant), pErr);
}


nsresult
nsXPCWrappedJSClass::GetNamedPropertyAsVariant(XPCCallContext& ccx,
                                               JSObject* aJSObjArg,
                                               const nsAString& aName,
                                               nsIVariant** aResult)
{
    JSContext* cx = ccx.GetJSContext();
    RootedObject aJSObj(cx, aJSObjArg);

    AutoScriptEvaluate scriptEval(cx);
    if (!scriptEval.StartEvaluating(aJSObj))
        return NS_ERROR_FAILURE;

    
    
    nsStringBuffer* buf;
    RootedValue value(cx);
    if (!XPCStringConvert::ReadableToJSVal(ccx, aName, &buf, &value))
        return NS_ERROR_OUT_OF_MEMORY;
    if (buf)
        buf->AddRef();

    RootedId id(cx);
    nsresult rv = NS_OK;
    if (!JS_ValueToId(cx, value, &id) ||
        !GetNamedPropertyAsVariantRaw(ccx, aJSObj, id, aResult, &rv)) {
        if (NS_FAILED(rv))
            return rv;
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}




nsresult
nsXPCWrappedJSClass::BuildPropertyEnumerator(XPCCallContext& ccx,
                                             JSObject* aJSObjArg,
                                             nsISimpleEnumerator** aEnumerate)
{
    JSContext* cx = ccx.GetJSContext();
    RootedObject aJSObj(cx, aJSObjArg);

    AutoScriptEvaluate scriptEval(cx);
    if (!scriptEval.StartEvaluating(aJSObj))
        return NS_ERROR_FAILURE;

    AutoIdArray idArray(cx, JS_Enumerate(cx, aJSObj));
    if (!idArray)
        return NS_ERROR_FAILURE;

    nsCOMArray<nsIProperty> propertyArray(idArray.length());
    RootedId idName(cx);
    for (size_t i = 0; i < idArray.length(); i++) {
        idName = idArray[i];

        nsCOMPtr<nsIVariant> value;
        nsresult rv;
        if (!GetNamedPropertyAsVariantRaw(ccx, aJSObj, idName,
                                          getter_AddRefs(value), &rv)) {
            if (NS_FAILED(rv))
                return rv;
            return NS_ERROR_FAILURE;
        }

        RootedValue jsvalName(cx);
        if (!JS_IdToValue(cx, idName, &jsvalName))
            return NS_ERROR_FAILURE;

        JSString* name = ToString(cx, jsvalName);
        if (!name)
            return NS_ERROR_FAILURE;

        nsAutoJSString autoStr;
        if (!autoStr.init(cx, name))
            return NS_ERROR_FAILURE;

        nsCOMPtr<nsIProperty> property =
            new xpcProperty(autoStr.get(), (uint32_t)autoStr.Length(), value);

        if (!propertyArray.AppendObject(property))
            return NS_ERROR_FAILURE;
    }

    return NS_NewArrayEnumerator(aEnumerate, propertyArray);
}



NS_IMPL_ISUPPORTS(xpcProperty, nsIProperty)

xpcProperty::xpcProperty(const char16_t* aName, uint32_t aNameLen,
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
    nsCOMPtr<nsIVariant> rval = mValue;
    rval.forget(aValue);
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
        static WrappedJSIdentity* singleton = nullptr;
        if (!singleton)
            singleton = new WrappedJSIdentity();
        return (void*) singleton;
    }
};

NS_DEFINE_STATIC_IID_ACCESSOR(WrappedJSIdentity,
                              NS_IXPCONNECT_WRAPPED_JS_IDENTITY_CLASS_IID)




bool
nsXPCWrappedJSClass::IsWrappedJS(nsISupports* aPtr)
{
    void* result;
    NS_PRECONDITION(aPtr, "null pointer");
    return aPtr &&
           NS_OK == aPtr->QueryInterface(NS_GET_IID(WrappedJSIdentity), &result) &&
           result == WrappedJSIdentity::GetSingleton();
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
            *aInstancePtr = nullptr;
            return NS_NOINTERFACE;
        }

        NS_ADDREF(root);
        *aInstancePtr = (void*) static_cast<nsIPropertyBag*>(root);
        return NS_OK;
    }

    
    if (aIID.Equals(NS_GET_IID(nsWrapperCache))) {
        *aInstancePtr = nullptr;
        return NS_NOINTERFACE;
    }

    
    
    
    
    
    nsIGlobalObject* nativeGlobal =
      NativeGlobal(js::GetGlobalForObjectCrossCompartment(self->GetJSObject()));
    NS_ENSURE_TRUE(nativeGlobal, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(nativeGlobal->GetGlobalJSObject(), NS_ERROR_FAILURE);
    AutoEntryScript aes(nativeGlobal, "XPCWrappedJS QueryInterface",
                         true);
    XPCCallContext ccx(NATIVE_CALLER, aes.cx());
    if (!ccx.IsValid()) {
        *aInstancePtr = nullptr;
        return NS_NOINTERFACE;
    }

    
    
    if (aIID.Equals(NS_GET_IID(nsISupportsWeakReference))) {
        
        nsXPCWrappedJS* root = self->GetRootWrapper();

        
        if (!root->IsValid() ||
            !CallQueryInterfaceOnJSObject(ccx, root->GetJSObject(), aIID)) {
            *aInstancePtr = nullptr;
            return NS_NOINTERFACE;
        }

        NS_ADDREF(root);
        *aInstancePtr = (void*) static_cast<nsISupportsWeakReference*>(root);
        return NS_OK;
    }

    
    
    
    
    if (nsXPCWrappedJS* sibling = self->FindOrFindInherited(aIID)) {
        NS_ADDREF(sibling);
        *aInstancePtr = sibling->GetXPTCStub();
        return NS_OK;
    }

    

    
    RootedObject jsobj(ccx, CallQueryInterfaceOnJSObject(ccx, self->GetJSObject(),
                                                         aIID));
    if (jsobj) {
        
        
        
        
        
        
        
        
        
        
        
        nsXPCWrappedJS* wrapper;
        nsresult rv = nsXPCWrappedJS::GetNewOrUsed(jsobj, aIID, &wrapper);
        if (NS_SUCCEEDED(rv) && wrapper) {
            
            
            
            rv = wrapper->QueryInterface(aIID, aInstancePtr);
            NS_RELEASE(wrapper);
            return rv;
        }
    }

    
    
    *aInstancePtr = nullptr;
    return NS_NOINTERFACE;
}

JSObject*
nsXPCWrappedJSClass::GetRootJSObject(JSContext* cx, JSObject* aJSObjArg)
{
    RootedObject aJSObj(cx, aJSObjArg);
    JSObject* result = CallQueryInterfaceOnJSObject(cx, aJSObj,
                                                    NS_GET_IID(nsISupports));
    if (!result)
        result = aJSObj;
    JSObject* inner = js::UncheckedUnwrap(result);
    if (inner)
        return inner;
    return result;
}

bool
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

    
    
    MOZ_ASSERT(arg_param.GetType().TagPart() == nsXPTType::T_U32,
               "size_is references parameter of invalid type.");

    if (arg_param.IsIndirect())
        *result = *(uint32_t*)nativeParams[argnum].val.p;
    else
        *result = nativeParams[argnum].val.u32;

    return true;
}

bool
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
            if (p) free(p);
        }
    }
}

void
nsXPCWrappedJSClass::CleanupPointerTypeObject(const nsXPTType& type,
                                              void** pp)
{
    MOZ_ASSERT(pp,"null pointer");
    if (type.IsInterfacePointer()) {
        nsISupports* p = *((nsISupports**)pp);
        if (p) p->Release();
    } else {
        void* p = *((void**)pp);
        if (p) free(p);
    }
}

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
        xpcc->SetException(nullptr);

    
    
    nsresult pending_result = xpcc->GetPendingResult();

    RootedValue js_exception(cx);
    bool is_js_exception = JS_GetPendingException(cx, &js_exception);

    
    if (is_js_exception) {
        if (!xpc_exception)
            XPCConvert::JSValToXPCException(&js_exception, anInterfaceName,
                                            aPropertyName,
                                            getter_AddRefs(xpc_exception));

        
        if (!xpc_exception) {
            XPCJSRuntime::Get()->SetPendingException(nullptr); 
        }
    }

    
    
    
    JS_ClearPendingException(cx);

    if (xpc_exception) {
        nsresult e_result;
        if (NS_SUCCEEDED(xpc_exception->GetResult(&e_result))) {
            
            bool reportable = xpc_IsReportableErrorCode(e_result);
            if (reportable) {
                
                
                reportable = aForceReport ||
                    NS_ERROR_GET_MODULE(e_result) == NS_ERROR_MODULE_XPCONNECT;

                
                
                
                if (!reportable)
                    reportable = nsXPConnect::ReportAllJSExceptions();

                
                
                if (!reportable)
                    reportable = !JS::DescribeScriptedCaller(cx);

                
                
                
                
                
                if (reportable && e_result == NS_ERROR_NO_INTERFACE &&
                    !strcmp(anInterfaceName, "nsIInterfaceRequestor") &&
                    !strcmp(aPropertyName, "getInterface")) {
                    reportable = false;
                }

                
                if (e_result == NS_ERROR_XPC_JSOBJECT_HAS_NO_FUNCTION_NAMED) {
                    reportable = false;
                }
            }

            
            
            if (reportable && is_js_exception)
            {
                
                
                
                JS_SetPendingException(cx, js_exception);
                reportable = !JS_ReportPendingException(cx);
            }

            if (reportable) {
                if (nsContentUtils::DOMWindowDumpEnabled()) {
                    static const char line[] =
                        "************************************************************\n";
                    static const char preamble[] =
                        "* Call to xpconnect wrapped JSObject produced this error:  *\n";
                    static const char cant_get_text[] =
                        "FAILED TO GET TEXT FROM EXCEPTION\n";

                    fputs(line, stdout);
                    fputs(preamble, stdout);
                    nsCString text;
                    if (NS_SUCCEEDED(xpc_exception->ToString(text)) &&
                        !text.IsEmpty()) {
                        fputs(text.get(), stdout);
                        fputs("\n", stdout);
                    } else
                        fputs(cant_get_text, stdout);
                    fputs(line, stdout);
                }

                
                
                nsCOMPtr<nsIConsoleService> consoleService
                    (do_GetService(XPC_CONSOLE_CONTRACTID));
                if (nullptr != consoleService) {
                    nsresult rv;
                    nsCOMPtr<nsIScriptError> scriptError;
                    nsCOMPtr<nsISupports> errorData;
                    rv = xpc_exception->GetData(getter_AddRefs(errorData));
                    if (NS_SUCCEEDED(rv))
                        scriptError = do_QueryInterface(errorData);

                    if (nullptr == scriptError) {
                        
                        
                        scriptError = do_CreateInstance(XPC_SCRIPT_ERROR_CONTRACTID);
                        if (nullptr != scriptError) {
                            nsCString newMessage;
                            rv = xpc_exception->ToString(newMessage);
                            if (NS_SUCCEEDED(rv)) {
                                
                                
                                int32_t lineNumber = 0;
                                nsString sourceName;

                                nsCOMPtr<nsIStackFrame> location;
                                xpc_exception->
                                    GetLocation(getter_AddRefs(location));
                                if (location) {
                                    
                                    location->GetLineNumber(&lineNumber);

                                    
                                    rv = location->GetFilename(sourceName);
                                }

                                rv = scriptError->InitWithWindowID(NS_ConvertUTF8toUTF16(newMessage),
                                                                   sourceName,
                                                                   EmptyString(),
                                                                   lineNumber, 0, 0,
                                                                   "XPConnect JavaScript",
                                                                   nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx));
                                if (NS_FAILED(rv))
                                    scriptError = nullptr;
                            }
                        }
                    }
                    if (nullptr != scriptError)
                        consoleService->LogMessage(scriptError);
                }
            }
            
            
            if (NS_FAILED(e_result)) {
                XPCJSRuntime::Get()->SetPendingException(xpc_exception);
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

NS_IMETHODIMP
nsXPCWrappedJSClass::CallMethod(nsXPCWrappedJS* wrapper, uint16_t methodIndex,
                                const XPTMethodDescriptor* info_,
                                nsXPTCMiniVariant* nativeParams)
{
    jsval* sp = nullptr;
    jsval* argv = nullptr;
    uint8_t i;
    nsresult retval = NS_ERROR_FAILURE;
    bool success;
    bool readyToDoTheCall = false;
    nsID  param_iid;
    const nsXPTMethodInfo* info = static_cast<const nsXPTMethodInfo*>(info_);
    const char* name = info->name;
    bool foundDependentParam;

    
    
    
    
    
    
    
    
    nsIGlobalObject* nativeGlobal =
      NativeGlobal(js::GetGlobalForObjectCrossCompartment(wrapper->GetJSObject()));
    AutoEntryScript aes(nativeGlobal, "XPCWrappedJS method call",
                         true);
    XPCCallContext ccx(NATIVE_CALLER, aes.cx());
    if (!ccx.IsValid())
        return retval;

    XPCContext* xpcc = ccx.GetXPCContext();
    JSContext* cx = ccx.GetJSContext();

    if (!cx || !xpcc || !IsReflectable(methodIndex))
        return NS_ERROR_FAILURE;

    
    
    if (info->WantsOptArgc() || info->WantsContext()) {
        const char* str = "IDL methods marked with [implicit_jscontext] "
                          "or [optional_argc] may not be implemented in JS";
        
        JS_ReportError(cx, str);
        NS_WARNING(str);
        return NS_ERROR_FAILURE;
    }

    RootedValue fval(cx);
    RootedObject obj(cx, wrapper->GetJSObject());
    RootedObject thisObj(cx, obj);

    JSAutoCompartment ac(cx, obj);

    AutoValueVector args(cx);
    AutoScriptEvaluate scriptEval(cx);

    AutoSavePendingResult apr(xpcc);

    
    uint8_t paramCount = info->num_args;
    uint8_t argc = paramCount -
        (paramCount && XPT_PD_IS_RETVAL(info->params[paramCount-1].flags) ? 1 : 0);

    if (!scriptEval.StartEvaluating(obj))
        goto pre_call_clean_up;

    xpcc->SetException(nullptr);
    XPCJSRuntime::Get()->SetPendingException(nullptr);

    
    

    

    
    if (!(XPT_MD_IS_SETTER(info->flags) || XPT_MD_IS_GETTER(info->flags))) {
        
        
        

        
        

        bool isFunction;
        if (NS_FAILED(mInfo->IsFunction(&isFunction)))
            goto pre_call_clean_up;

        
        
        
        
        
        
        
        
        
        

        fval = ObjectValue(*obj);
        if (isFunction &&
            JS_TypeOfValue(ccx, fval) == JSTYPE_FUNCTION) {

            

            if (paramCount) {
                const nsXPTParamInfo& firstParam = info->params[0];
                if (firstParam.IsIn()) {
                    const nsXPTType& firstType = firstParam.GetType();

                    if (firstType.IsInterfacePointer()) {
                        nsIXPCFunctionThisTranslator* translator;

                        IID2ThisTranslatorMap* map =
                            mRuntime->GetThisTranslatorMap();

                        translator = map->Find(mIID);

                        if (translator) {
                            nsCOMPtr<nsISupports> newThis;
                            if (NS_FAILED(translator->
                                          TranslateThis((nsISupports*)nativeParams[0].val.p,
                                                        getter_AddRefs(newThis)))) {
                                goto pre_call_clean_up;
                            }
                            if (newThis) {
                                RootedValue v(cx);
                                xpcObjectHelper helper(newThis);
                                bool ok =
                                  XPCConvert::NativeInterface2JSObject(
                                      &v, nullptr, helper, nullptr,
                                      nullptr, false, nullptr);
                                if (!ok) {
                                    goto pre_call_clean_up;
                                }
                                thisObj = v.toObjectOrNull();
                                if (!JS_WrapObject(cx, &thisObj))
                                    goto pre_call_clean_up;
                            }
                        }
                    }
                }
            }
        } else {
            if (!JS_GetProperty(cx, obj, name, &fval))
                goto pre_call_clean_up;
            
            
            
            
            

            thisObj = obj;
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
        RootedValue val(cx, NullValue());
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
                if (!XPCConvert::NativeArray2JS(&val,
                                                (const void**)&pv->val,
                                                datum_type, &param_iid,
                                                array_count, nullptr))
                    goto pre_call_clean_up;
            } else if (isSizedString) {
                if (!XPCConvert::NativeStringWithSize2JS(&val,
                                                         (const void*)&pv->val,
                                                         datum_type,
                                                         array_count, nullptr))
                    goto pre_call_clean_up;
            } else {
                if (!XPCConvert::NativeData2JS(&val, &pv->val, type,
                                               &param_iid, nullptr))
                    goto pre_call_clean_up;
            }
        }

        if (param.IsOut() || param.IsDipper()) {
            
            RootedObject out_obj(cx, NewOutObject(cx));
            if (!out_obj) {
                retval = NS_ERROR_OUT_OF_MEMORY;
                goto pre_call_clean_up;
            }

            if (param.IsIn()) {
                if (!JS_SetPropertyById(cx, out_obj,
                                        mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                                        val)) {
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
                if (nullptr != (pp = *((void***)p))) {

                    
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

                    
                    free(pp);
                }
            } else
                CleanupPointerTypeObject(type, (void**)p);
        }
        *((void**)p) = nullptr;
    }

    
    nsCOMPtr<nsIXPCWrappedJSClass> kungFuDeathGrip(this);

    if (!readyToDoTheCall)
        return retval;

    

    JS_ClearPendingException(cx);

    RootedValue rval(cx);
    if (XPT_MD_IS_GETTER(info->flags)) {
        success = JS_GetProperty(cx, obj, name, &rval);
    } else if (XPT_MD_IS_SETTER(info->flags)) {
        rval = *argv;
        success = JS_SetProperty(cx, obj, name, rval);
    } else {
        if (!fval.isPrimitive()) {
            AutoSaveContextOptions asco(cx);
            ContextOptionsRef(cx).setDontReportUncaught(true);

            success = JS_CallFunctionValue(cx, thisObj, fval, args, &rval);
        } else {
            
            

            static const nsresult code =
                    NS_ERROR_XPC_JSOBJECT_HAS_NO_FUNCTION_NAMED;
            static const char format[] = "%s \"%s\"";
            const char * msg;
            char* sz = nullptr;

            if (nsXPCException::NameAndFormatForNSResult(code, nullptr, &msg) && msg)
                sz = JS_smprintf(format, msg, name);

            nsCOMPtr<nsIException> e;

            XPCConvert::ConstructException(code, sz, GetInterfaceName(), name,
                                           nullptr, getter_AddRefs(e), nullptr, nullptr);
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

    XPCJSRuntime::Get()->SetPendingException(nullptr); 

    
    
    
    
    
    

    foundDependentParam = false;
    for (i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->params[i];
        MOZ_ASSERT(!param.IsShared(), "[shared] implies [noscript]!");
        if (!param.IsOut() && !param.IsDipper())
            continue;

        const nsXPTType& type = param.GetType();
        if (type.IsDependent()) {
            foundDependentParam = true;
            continue;
        }

        RootedValue val(cx);
        uint8_t type_tag = type.TagPart();
        nsXPTCMiniVariant* pv;

        if (param.IsDipper())
            pv = (nsXPTCMiniVariant*) &nativeParams[i].val.p;
        else
            pv = (nsXPTCMiniVariant*) nativeParams[i].val.p;

        if (param.IsRetval())
            val = rval;
        else if (argv[i].isPrimitive())
            break;
        else {
            RootedObject obj(cx, &argv[i].toObject());
            if (!JS_GetPropertyById(cx, obj,
                                    mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                                    &val))
                break;
        }

        

        if (type_tag == nsXPTType::T_INTERFACE) {
            if (NS_FAILED(GetInterfaceInfo()->
                          GetIIDForParamNoAlloc(methodIndex, &param,
                                                &param_iid)))
                break;
        }

        if (!XPCConvert::JSData2Native(&pv->val, val, type,
                                       &param_iid, nullptr))
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

            RootedValue val(cx);
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
            else {
                RootedObject obj(cx, &argv[i].toObject());
                if (!JS_GetPropertyById(cx, obj,
                                        mRuntime->GetStringID(XPCJSRuntime::IDX_VALUE),
                                        &val))
                    break;
            }

            

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
                    !XPCConvert::JSArray2Native((void**)&pv->val, val,
                                                array_count, datum_type,
                                                &param_iid, nullptr))
                    break;
            } else if (isSizedString) {
                if (!XPCConvert::JSStringWithSize2Native((void*)&pv->val, val,
                                                         array_count, datum_type,
                                                         nullptr))
                    break;
            } else {
                if (!XPCConvert::JSData2Native(&pv->val, val, type,
                                               &param_iid,
                                               nullptr))
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
                if (nullptr != (pp = *((void***)p))) {
                    
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
                    free(pp);
                }
            } else
                CleanupPointerTypeObject(type, (void**)p);
            *((void**)p) = nullptr;
        }
    } else {
        
        retval = xpcc->GetPendingResult();
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

static void
FinalizeStub(JSFreeOp* fop, JSObject* obj)
{
}

static const JSClass XPCOutParamClass = {
    "XPCOutParam",
    0,
    nullptr,   
    nullptr,   
    nullptr,   
    nullptr,   
    nullptr,   
    nullptr,   
    nullptr,   
    nullptr,   
    FinalizeStub,
    nullptr,   
    nullptr,   
    nullptr,   
    nullptr    
};

bool
xpc::IsOutObject(JSContext* cx, JSObject* obj)
{
    return js::GetObjectJSClass(obj) == &XPCOutParamClass;
}

JSObject*
xpc::NewOutObject(JSContext* cx)
{
    return JS_NewObject(cx, &XPCOutParamClass);
}


NS_IMETHODIMP
nsXPCWrappedJSClass::DebugDump(int16_t depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("nsXPCWrappedJSClass @ %x with mRefCnt = %d", this, mRefCnt.get()));
    XPC_LOG_INDENT();
        char* name;
        mInfo->GetName(&name);
        XPC_LOG_ALWAYS(("interface name is %s", name));
        if (name)
            free(name);
        char * iid = mIID.ToString();
        XPC_LOG_ALWAYS(("IID number is %s", iid ? iid : "invalid"));
        if (iid)
            NS_Free(iid);
        XPC_LOG_ALWAYS(("InterfaceInfo @ %x", mInfo.get()));
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
