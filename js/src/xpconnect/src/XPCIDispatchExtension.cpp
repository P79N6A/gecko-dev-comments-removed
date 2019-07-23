



































#include "xpcprivate.h"

static const char* const IDISPATCH_NAME = "IDispatch";

#define XPC_IDISPATCH_CTOR_MAX_ARG_LEN 2000

PRBool XPCIDispatchExtension::mIsEnabled = PR_TRUE;

JSBool XPCIDispatchExtension::DefineProperty(XPCCallContext & ccx, 
                                             JSObject *obj, jsval idval,
                                             XPCWrappedNative* wrapperToReflectInterfaceNames,
                                             uintN propFlags, JSBool* resolved)
{
    if(!JSVAL_IS_STRING(idval))
        return JS_FALSE;
    
    XPCNativeInterface* iface = XPCNativeInterface::GetNewOrUsed(ccx,
                                                                 "IDispatch");
    
    if(iface == nsnull)
        return JS_FALSE;
    XPCWrappedNativeTearOff* to = 
        wrapperToReflectInterfaceNames->LocateTearOff(ccx, iface);
    
    if(to == nsnull)
        return JS_FALSE;
    
    const XPCDispInterface::Member * member = to->GetIDispatchInfo()->FindMember(idval);
    if(!member)
    {
        
        
        
        
        member = to->GetIDispatchInfo()->FindMemberCI(ccx, idval);
        if(!member)
            return JS_FALSE;
    }
    
    jsval funval;
    if(!member->GetValue(ccx, iface, &funval))
        return JS_FALSE;
    
    AUTO_MARK_JSVAL(ccx, funval);
    
    JSObject* funobj = xpc_CloneJSFunction(ccx, JSVAL_TO_OBJECT(funval), obj);
    if(!funobj)
        return JS_FALSE;
    jsid id;
    
    if(member->IsFunction() || member->IsParameterizedProperty())
    {
        
        AutoResolveName arn(ccx, idval);
        if(resolved)
            *resolved = JS_TRUE;
        return JS_ValueToId(ccx, idval, &id) &&
               JS_DefinePropertyById(ccx, obj, id, OBJECT_TO_JSVAL(funobj),
                                     nsnull, nsnull, propFlags);
    }
    
    NS_ASSERTION(member->IsProperty(), "way broken!");
    propFlags |= JSPROP_GETTER | JSPROP_SHARED;
    JSPropertyOp getter = JS_DATA_TO_FUNC_PTR(JSPropertyOp, funobj);
    JSPropertyOp setter;
    if(member->IsSetter())
    {
        propFlags |= JSPROP_SETTER;
        propFlags &= ~JSPROP_READONLY;
        setter = getter;
    }
    else
    {
        setter = js_GetterOnlyPropertyStub;
    }
    AutoResolveName arn(ccx, idval);
    if(resolved)
        *resolved = JS_TRUE;
    return JS_ValueToId(ccx, idval, &id) &&
           JS_DefinePropertyById(ccx, obj, id, JSVAL_VOID, getter, setter,
                                 propFlags);

}

JSBool XPCIDispatchExtension::Enumerate(XPCCallContext& ccx, JSObject* obj,
                                        XPCWrappedNative * wrapper)
{
    XPCNativeInterface* iface = XPCNativeInterface::GetNewOrUsed(
        ccx,&NSID_IDISPATCH);
    if(!iface)
        return JS_FALSE;

    XPCWrappedNativeTearOff* tearoff = wrapper->FindTearOff(ccx, iface);
    if(!tearoff)
        return JS_FALSE;

    XPCDispInterface* pInfo = tearoff->GetIDispatchInfo();
    PRUint32 members = pInfo->GetMemberCount();
    
    for(PRUint32 index = 0; index < members; ++index)
    {
        const XPCDispInterface::Member & member = pInfo->GetMember(index);
        jsval name = member.GetName();
        if(!xpc_ForcePropertyResolve(ccx, obj, name))
            return JS_FALSE;
    }
    return JS_TRUE;
}

nsresult XPCIDispatchExtension::IDispatchQIWrappedJS(nsXPCWrappedJS * self, 
                                                     void ** aInstancePtr)
{
    
    nsXPCWrappedJS* root = self->GetRootWrapper();

    if(!root->IsValid())
    {
        *aInstancePtr = nsnull;
        return NS_NOINTERFACE;
    }
    XPCDispatchTearOff* tearOff = new XPCDispatchTearOff(root);
    if(!tearOff)
        return NS_ERROR_OUT_OF_MEMORY;
    tearOff->AddRef();
    *aInstancePtr = tearOff;
    
    return NS_OK;
}
