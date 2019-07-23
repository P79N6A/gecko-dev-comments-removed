









































#include "xpcprivate.h"






inline
PRBool IsReflectable(FUNCDESC * pFuncDesc)
{
    return (pFuncDesc->wFuncFlags&FUNCFLAG_FRESTRICTED) == 0 &&
           pFuncDesc->funckind == FUNC_DISPATCH || 
           pFuncDesc->funckind == FUNC_PUREVIRTUAL ||
           pFuncDesc->funckind == FUNC_VIRTUAL;
}

XPCDispInterface::Allocator::Allocator(JSContext * cx, ITypeInfo * pTypeInfo) :
    mMemIDs(nsnull), mCount(0), mIDispatchMembers(0), mCX(cx), 
    mTypeInfo(pTypeInfo)
{
    TYPEATTR * attr;
    HRESULT hr = pTypeInfo->GetTypeAttr(&attr);
    if(SUCCEEDED(hr))
    {
        mIDispatchMembers = attr->cFuncs;
        mMemIDs = new DISPID[mIDispatchMembers];
        pTypeInfo->ReleaseTypeAttr(attr);
        
        if(!mMemIDs)
            return;
    }
    for(UINT iMethod = 0; iMethod < mIDispatchMembers; iMethod++ )
    {
        FUNCDESC* pFuncDesc;
        if(SUCCEEDED(pTypeInfo->GetFuncDesc(iMethod, &pFuncDesc)))
        {
            
            
            if(IsReflectable(pFuncDesc))
                Add(pFuncDesc->memid);
            pTypeInfo->ReleaseFuncDesc(pFuncDesc);
        }
    }
}

void XPCDispInterface::Allocator::Add(DISPID memID)
{
    NS_ASSERTION(Valid(), "Add should never be called if out of memory");
    
    
    PRUint32 index = mCount;
    while(index > 0)
    {
        if(mMemIDs[--index] == memID)
            return;
    };
    NS_ASSERTION(Count() < mIDispatchMembers, "mCount should always be less "
                                             "than the IDispatch member count "
                                             "here");
    mMemIDs[mCount++] = memID;
    return;
}

inline
PRUint32 XPCDispInterface::Allocator::Count() const 
{
    return mCount;
}

XPCDispInterface*
XPCDispInterface::NewInstance(JSContext* cx, nsISupports * pIface)
{
    CComQIPtr<IDispatch> pDispatch(reinterpret_cast<IUnknown*>(pIface));

    if(pDispatch)
    {
        unsigned int count;
        HRESULT hr = pDispatch->GetTypeInfoCount(&count);
        if(SUCCEEDED(hr) && count > 0)
        {
            CComPtr<ITypeInfo> pTypeInfo;
            hr = pDispatch->GetTypeInfo(0,LOCALE_SYSTEM_DEFAULT, &pTypeInfo);
            if(SUCCEEDED(hr))
            {
                Allocator allocator(cx, pTypeInfo);
                return allocator.Allocate();
            }
        }
    }
    return nsnull;
}




static
void ConvertInvokeKind(INVOKEKIND invokeKind, XPCDispInterface::Member & member)
{
    switch (invokeKind)
    {
        case INVOKE_FUNC:
        {
            member.SetFunction();
        }
        break;
        case INVOKE_PROPERTYGET:
        {
            member.MakeGetter();
        }
        break;
        case INVOKE_PROPERTYPUT:
        {
            member.MakeSetter();
        }
        break;
        
        default:
        {
            NS_ERROR("Invalid invoke kind found in COM type info");
        }
        break;
    }
}

static
PRBool InitializeMember(JSContext * cx, ITypeInfo * pTypeInfo,
                        FUNCDESC * pFuncDesc, 
                        XPCDispInterface::Member * pInfo)
{
    pInfo->SetMemID(pFuncDesc->memid);
    BSTR name;
    UINT nameCount;
    if(FAILED(pTypeInfo->GetNames(
        pFuncDesc->memid,
        &name,
        1,
        &nameCount)))
        return PR_FALSE;
    if(nameCount != 1)
        return PR_FALSE;
    JSString* str = JS_InternUCStringN(cx,
                                       reinterpret_cast<const jschar *>(name),
                                       ::SysStringLen(name));
    ::SysFreeString(name);
    if(!str)
        return PR_FALSE;
    
    pInfo = new (pInfo) XPCDispInterface::Member;
    if(!pInfo)
        return PR_FALSE;
    pInfo->SetName(STRING_TO_JSVAL(str));
    pInfo->ResetType();
    ConvertInvokeKind(pFuncDesc->invkind, *pInfo);
    pInfo->SetTypeInfo(pFuncDesc->memid, pTypeInfo, pFuncDesc);
    return PR_TRUE;
}

static
XPCDispInterface::Member * FindExistingMember(XPCDispInterface::Member * first,
                                              XPCDispInterface::Member * last,
                                              MEMBERID memberID)
{
    
    XPCDispInterface::Member * cur = last;
    if (cur != first)
    {
        do 
        {
            --cur;
            if(cur->GetMemID() == memberID)
                return cur;
        } while(cur != first);
    } 
    
    return last;
}

PRBool XPCDispInterface::InspectIDispatch(JSContext * cx, ITypeInfo * pTypeInfo, PRUint32 members)
{
    HRESULT hResult;

    XPCDispInterface::Member * pInfo = mMembers;
    mMemberCount = 0;
    for(PRUint32 index = 0; index < members; index++ )
    {
        FUNCDESC* pFuncDesc;
        hResult = pTypeInfo->GetFuncDesc(index, &pFuncDesc );
        if(FAILED(hResult))
            continue;
        if(IsReflectable(pFuncDesc))
        {
            switch(pFuncDesc->invkind)
            {
                case INVOKE_PROPERTYPUT:
                case INVOKE_PROPERTYPUTREF:
                case INVOKE_PROPERTYGET:
                {
                    XPCDispInterface::Member * pExisting = FindExistingMember(mMembers, pInfo, pFuncDesc->memid);
                    if(pExisting == pInfo)
                    {
                        if(InitializeMember(cx, pTypeInfo, pFuncDesc, pInfo))
                        {
                            ++pInfo;
                            ++mMemberCount;
                        }
                    }
                    else
                    {
                        ConvertInvokeKind(pFuncDesc->invkind, *pExisting);
                    }
                    if(pFuncDesc->invkind == INVOKE_PROPERTYGET)
                    {
                        pExisting->SetGetterFuncDesc(pFuncDesc);
                    }
                }
                break;
                case INVOKE_FUNC:
                {
                    if(InitializeMember(cx, pTypeInfo, pFuncDesc, pInfo))
                    {
                        ++pInfo;
                        ++mMemberCount;
                    }
                }
                break;
                default:
                    pTypeInfo->ReleaseFuncDesc(pFuncDesc);
                break;
            }
        }
        else
        {
            pTypeInfo->ReleaseFuncDesc(pFuncDesc);
        }
    }
    return PR_TRUE;
}









inline
PRBool CaseInsensitiveCompare(XPCCallContext& ccx, const PRUnichar* lhs, size_t lhsLength, jsval rhs)
{
    if(lhsLength == 0)
        return PR_FALSE;
    size_t rhsLength;
    PRUnichar* rhsString = xpc_JSString2PRUnichar(ccx, rhs, &rhsLength);
    return rhsString && 
        lhsLength == rhsLength &&
        _wcsnicmp(lhs, rhsString, lhsLength) == 0;
}

const XPCDispInterface::Member* XPCDispInterface::FindMemberCI(XPCCallContext& ccx, jsval name) const
{
    size_t nameLength;
    PRUnichar* sName = xpc_JSString2PRUnichar(ccx, name, &nameLength);
    if(!sName)
        return nsnull;
    
    const Member* member = mMembers + mMemberCount;
    while(member > mMembers)
    {
        --member;
        if(CaseInsensitiveCompare(ccx, sName, nameLength, member->GetName()))
        {
            return member;
        }
    }
    return nsnull;
}

JSBool XPCDispInterface::Member::GetValue(XPCCallContext& ccx,
                                          XPCNativeInterface * iface, 
                                          jsval * retval) const
{
    

    
    
    
    if((mType & RESOLVED) == 0)
    {
        JSContext* cx = ccx.GetSafeJSContext();
        if(!cx)
            return JS_FALSE;

        intN argc;
        intN flags;
        JSNative callback;
        
        if(IsFunction() || IsParameterizedProperty())
        {
            argc = GetParamCount();
            flags = 0;
            callback = XPC_IDispatch_CallMethod;
        }
        else
        {
            if(IsSetter())
            {
                flags = JSFUN_GETTER | JSFUN_SETTER;
            }
            else
            {
                flags = JSFUN_GETTER;
            }
            argc = 0;
            callback = XPC_IDispatch_GetterSetter;
        }

        JSFunction *fun = JS_NewFunction(cx, callback, argc, flags, nsnull,
                                         JS_GetStringBytes(JSVAL_TO_STRING(mName)));
        if(!fun)
            return JS_FALSE;

        JSObject* funobj = JS_GetFunctionObject(fun);
        if(!funobj)
            return JS_FALSE;

        
        if(!JS_SetReservedSlot(ccx, funobj, 0, PRIVATE_TO_JSVAL(this)))
            return JS_FALSE;

        if(!JS_SetReservedSlot(ccx, funobj, 1, PRIVATE_TO_JSVAL(iface)))
            return JS_FALSE;

        {   
            XPCAutoLock lock(ccx.GetRuntime()->GetMapLock());
            const_cast<Member*>(this)->mVal = OBJECT_TO_JSVAL(funobj);
            const_cast<Member*>(this)->mType |= RESOLVED;
        }
    }
    *retval = mVal;
    return JS_TRUE;
}
