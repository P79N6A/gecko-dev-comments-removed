










































#include "xpcprivate.h"








JSObject *
xpc_CloneJSFunction(XPCCallContext &ccx, JSObject *funobj, JSObject *parent)
{
    JSObject *clone = JS_CloneFunctionObject(ccx, funobj, parent);
    if(!clone)
        return nsnull;

    AUTO_MARK_JSVAL(ccx, OBJECT_TO_JSVAL(clone));

    XPCWrappedNativeScope *scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, parent);

    if (!scope) {
        return nsnull;
    }

    
    
    
    JS_SetPrototype(ccx, clone, scope->GetPrototypeJSFunction());

    
    jsval ifaceVal, memberVal;
    if(!JS_GetReservedSlot(ccx, funobj, 0, &ifaceVal) ||
       !JS_GetReservedSlot(ccx, funobj, 1, &memberVal))
        return nsnull;

    if(!JS_SetReservedSlot(ccx, clone, 0, ifaceVal) ||
       !JS_SetReservedSlot(ccx, clone, 1, memberVal))
        return nsnull;

    return clone;
}




JSBool
XPCNativeMember::GetCallInfo(XPCCallContext& ccx,
                             JSObject* funobj,
                             XPCNativeInterface** pInterface,
                             XPCNativeMember**    pMember)
{
    funobj = js::UnwrapObject(funobj);
    jsval ifaceVal = js::GetReservedSlot(funobj, 0);
    jsval memberVal = js::GetReservedSlot(funobj, 1);

    *pInterface = (XPCNativeInterface*) JSVAL_TO_PRIVATE(ifaceVal);
    *pMember = (XPCNativeMember*) JSVAL_TO_PRIVATE(memberVal);

    return JS_TRUE;
}

JSBool
XPCNativeMember::NewFunctionObject(XPCCallContext& ccx,
                                   XPCNativeInterface* iface, JSObject *parent,
                                   jsval* pval)
{
    NS_ASSERTION(!IsConstant(),
                 "Only call this if you're sure this is not a constant!");

    return Resolve(ccx, iface, parent, pval);
}

JSBool
XPCNativeMember::Resolve(XPCCallContext& ccx, XPCNativeInterface* iface,
                         JSObject *parent, jsval *vp)
{
    if(IsConstant())
    {
        const nsXPTConstant* constant;
        if(NS_FAILED(iface->GetInterfaceInfo()->GetConstant(mIndex, &constant)))
            return JS_FALSE;

        const nsXPTCMiniVariant& mv = *constant->GetValue();

        
        nsXPTCVariant v;
        v.flags = 0;
        v.type = constant->GetType();
        memcpy(&v.val, &mv.val, sizeof(mv.val));

        jsval resultVal;

        if(!XPCConvert::NativeData2JS(ccx, &resultVal, &v.val, v.type,
                                      nsnull, nsnull))
            return JS_FALSE;

        *vp = resultVal;

        return JS_TRUE;
    }
    

    

    intN argc;
    JSNative callback;

    if(IsMethod())
    {
        const nsXPTMethodInfo* info;
        if(NS_FAILED(iface->GetInterfaceInfo()->GetMethodInfo(mIndex, &info)))
            return JS_FALSE;

        
        argc = (intN) info->GetParamCount();
        if(argc && info->GetParam((uint8)(argc-1)).IsRetval())
            argc-- ;

        callback = XPC_WN_CallMethod;
    }
    else
    {
        argc = 0;
        callback = XPC_WN_GetterSetter;
    }

    JSFunction *fun = JS_NewFunctionById(ccx, callback, argc, 0, parent, GetName());
    if(!fun)
        return JS_FALSE;

    JSObject* funobj = JS_GetFunctionObject(fun);
    if(!funobj)
        return JS_FALSE;

    if(!JS_SetReservedSlot(ccx, funobj, 0, PRIVATE_TO_JSVAL(iface))||
       !JS_SetReservedSlot(ccx, funobj, 1, PRIVATE_TO_JSVAL(this)))
        return JS_FALSE;

    *vp = OBJECT_TO_JSVAL(funobj);

    return JS_TRUE;
}





XPCNativeInterface*
XPCNativeInterface::GetNewOrUsed(XPCCallContext& ccx, const nsIID* iid)
{
    AutoMarkingNativeInterfacePtr iface(ccx);
    XPCJSRuntime* rt = ccx.GetRuntime();

    IID2NativeInterfaceMap* map = rt->GetIID2NativeInterfaceMap();
    if(!map)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        iface = map->Find(*iid);
    }

    if(iface)
        return iface;

    nsCOMPtr<nsIInterfaceInfo> info;
    ccx.GetXPConnect()->GetInfoForIID(iid, getter_AddRefs(info));
    if(!info)
        return nsnull;

    iface = NewInstance(ccx, info);
    if(!iface)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        XPCNativeInterface* iface2 = map->Add(iface);
        if(!iface2)
        {
            NS_ERROR("failed to add our interface!");
            DestroyInstance(iface);
            iface = nsnull;
        }
        else if(iface2 != iface)
        {
            DestroyInstance(iface);
            iface = iface2;
        }
    }

    return iface;
}


XPCNativeInterface*
XPCNativeInterface::GetNewOrUsed(XPCCallContext& ccx, nsIInterfaceInfo* info)
{
    AutoMarkingNativeInterfacePtr iface(ccx);

    const nsIID* iid;
    if(NS_FAILED(info->GetIIDShared(&iid)) || !iid)
        return nsnull;

    XPCJSRuntime* rt = ccx.GetRuntime();

    IID2NativeInterfaceMap* map = rt->GetIID2NativeInterfaceMap();
    if(!map)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        iface = map->Find(*iid);
    }

    if(iface)
        return iface;

    iface = NewInstance(ccx, info);
    if(!iface)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        XPCNativeInterface* iface2 = map->Add(iface);
        if(!iface2)
        {
            NS_ERROR("failed to add our interface!");
            DestroyInstance(iface);
            iface = nsnull;
        }
        else if(iface2 != iface)
        {
            DestroyInstance(iface);
            iface = iface2;
        }
    }

    return iface;
}


XPCNativeInterface*
XPCNativeInterface::GetNewOrUsed(XPCCallContext& ccx, const char* name)
{
    nsCOMPtr<nsIInterfaceInfo> info;
    ccx.GetXPConnect()->GetInfoForName(name, getter_AddRefs(info));
    return info ? GetNewOrUsed(ccx, info) : nsnull;
}


XPCNativeInterface*
XPCNativeInterface::GetISupports(XPCCallContext& ccx)
{
    
    return GetNewOrUsed(ccx, &NS_GET_IID(nsISupports));
}


XPCNativeInterface*
XPCNativeInterface::NewInstance(XPCCallContext& ccx,
                                nsIInterfaceInfo* aInfo)
{
    static const PRUint16 MAX_LOCAL_MEMBER_COUNT = 16;
    XPCNativeMember local_members[MAX_LOCAL_MEMBER_COUNT];
    XPCNativeInterface* obj = nsnull;
    XPCNativeMember* members = nsnull;

    int i;
    JSBool failed = JS_FALSE;
    PRUint16 constCount;
    PRUint16 methodCount;
    PRUint16 totalCount;
    PRUint16 realTotalCount = 0;
    XPCNativeMember* cur;
    JSString*  str;
    jsid name;
    jsid interfaceName;

    
    
    
    
    
    

    bool canScript;
    if(NS_FAILED(aInfo->IsScriptable(&canScript)) || !canScript)
        return nsnull;

    if(NS_FAILED(aInfo->GetMethodCount(&methodCount)) ||
       NS_FAILED(aInfo->GetConstantCount(&constCount)))
        return nsnull;

    
    
    
    
    
    if(!nsXPConnect::IsISupportsDescendant(aInfo))
        methodCount = 0;

    totalCount = methodCount + constCount;

    if(totalCount > MAX_LOCAL_MEMBER_COUNT)
    {
        members = new XPCNativeMember[totalCount];
        if(!members)
            return nsnull;
    }
    else
    {
        members = local_members;
    }

    
    

    for(i = 0; i < methodCount; i++)
    {
        const nsXPTMethodInfo* info;
        if(NS_FAILED(aInfo->GetMethodInfo(i, &info)))
        {
            failed = JS_TRUE;
            break;
        }

        
        if(i == 1 || i == 2)
            continue;

        if(!XPCConvert::IsMethodReflectable(*info))
            continue;

        str = JS_InternString(ccx, info->GetName());
        if(!str)
        {
            NS_ERROR("bad method name");
            failed = JS_TRUE;
            break;
        }
        name = INTERNED_STRING_TO_JSID(ccx, str);

        if(info->IsSetter())
        {
            NS_ASSERTION(realTotalCount,"bad setter");
            
            
            cur = &members[realTotalCount-1];
            NS_ASSERTION(cur->GetName() == name,"bad setter");
            NS_ASSERTION(cur->IsReadOnlyAttribute(),"bad setter");
            NS_ASSERTION(cur->GetIndex() == i-1,"bad setter");
            cur->SetWritableAttribute();
        }
        else
        {
            
            
            cur = &members[realTotalCount++];
            cur->SetName(name);
            if(info->IsGetter())
                cur->SetReadOnlyAttribute(i);
            else
                cur->SetMethod(i);
        }
    }

    if(!failed)
    {
        for(i = 0; i < constCount; i++)
        {
            const nsXPTConstant* constant;
            if(NS_FAILED(aInfo->GetConstant(i, &constant)))
            {
                failed = JS_TRUE;
                break;
            }

            str = JS_InternString(ccx, constant->GetName());
            if(!str)
            {
                NS_ERROR("bad constant name");
                failed = JS_TRUE;
                break;
            }
            name = INTERNED_STRING_TO_JSID(ccx, str);

            
            

            cur = &members[realTotalCount++];
            cur->SetName(name);
            cur->SetConstant(i);
        }
    }

    if(!failed)
    {
        const char* bytes;
        if(NS_FAILED(aInfo->GetNameShared(&bytes)) || !bytes ||
           nsnull == (str = JS_InternString(ccx, bytes)))
        {
            failed = JS_TRUE;
        }
        interfaceName = INTERNED_STRING_TO_JSID(ccx, str);
    }

    if(!failed)
    {
        
        
        int size = sizeof(XPCNativeInterface);
        if(realTotalCount > 1)
            size += (realTotalCount - 1) * sizeof(XPCNativeMember);
        void* place = new char[size];
        if(place)
            obj = new(place) XPCNativeInterface(aInfo, interfaceName);

        if(obj)
        {
            obj->mMemberCount = realTotalCount;
            
            if(realTotalCount)
                memcpy(obj->mMembers, members,
                       realTotalCount * sizeof(XPCNativeMember));
        }
    }

    if(members && members != local_members)
        delete [] members;

    return obj;
}


void
XPCNativeInterface::DestroyInstance(XPCNativeInterface* inst)
{
    inst->~XPCNativeInterface();
    delete [] (char*) inst;
}

void
XPCNativeInterface::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCNativeInterface @ %x", this));
        XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("name is %s", GetNameString()));
        XPC_LOG_ALWAYS(("mMemberCount is %d", mMemberCount));
        XPC_LOG_ALWAYS(("mInfo @ %x", mInfo.get()));
        XPC_LOG_OUTDENT();
#endif
}





XPCNativeSet*
XPCNativeSet::GetNewOrUsed(XPCCallContext& ccx, const nsIID* iid)
{
    AutoMarkingNativeSetPtr set(ccx);

    AutoMarkingNativeInterfacePtr iface(ccx);
    iface = XPCNativeInterface::GetNewOrUsed(ccx, iid);
    if(!iface)
        return nsnull;

    XPCNativeSetKey key(nsnull, iface, 0);

    XPCJSRuntime* rt = ccx.GetRuntime();
    NativeSetMap* map = rt->GetNativeSetMap();
    if(!map)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        set = map->Find(&key);
    }

    if(set)
        return set;

    
    XPCNativeInterface* temp[] = {iface};
    set = NewInstance(ccx, temp, 1);
    if(!set)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        XPCNativeSet* set2 = map->Add(&key, set);
        if(!set2)
        {
            NS_ERROR("failed to add our set!");
            DestroyInstance(set);
            set = nsnull;
        }
        else if(set2 != set)
        {
            DestroyInstance(set);
            set = set2;
        }
    }

    return set;
}


XPCNativeSet*
XPCNativeSet::GetNewOrUsed(XPCCallContext& ccx, nsIClassInfo* classInfo)
{
    AutoMarkingNativeSetPtr set(ccx);
    XPCJSRuntime* rt = ccx.GetRuntime();

    ClassInfo2NativeSetMap* map = rt->GetClassInfo2NativeSetMap();
    if(!map)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        set = map->Find(classInfo);
    }

    if(set)
        return set;

    nsIID** iidArray = nsnull;
    AutoMarkingNativeInterfacePtrArrayPtr interfaceArray(ccx);
    PRUint32 iidCount = 0;

    if(NS_FAILED(classInfo->GetInterfaces(&iidCount, &iidArray)))
    {
        
        
        

        
        iidArray = nsnull;
        iidCount = 0;
    }

    NS_ASSERTION((iidCount && iidArray) || !(iidCount || iidArray), "GetInterfaces returned bad array");

    

    if(iidCount)
    {
        AutoMarkingNativeInterfacePtrArrayPtr
            arr(ccx, new XPCNativeInterface*[iidCount], iidCount, PR_TRUE);
        if (!arr)
            goto out;

        interfaceArray = arr;

        XPCNativeInterface** currentInterface = interfaceArray;
        nsIID**              currentIID = iidArray;
        PRUint16             interfaceCount = 0;

        for(PRUint32 i = 0; i < iidCount; i++)
        {
            nsIID* iid = *(currentIID++);
            if (!iid) {
                NS_ERROR("Null found in classinfo interface list");
                continue;
            }

            XPCNativeInterface* iface =
                XPCNativeInterface::GetNewOrUsed(ccx, iid);

            if(!iface)
            {
                
                continue;
            }

            *(currentInterface++) = iface;
            interfaceCount++;
        }

        if(interfaceCount)
        {
            set = NewInstance(ccx, interfaceArray, interfaceCount);
            if(set)
            {
                NativeSetMap* map2 = rt->GetNativeSetMap();
                if(!map2)
                    goto out;

                XPCNativeSetKey key(set, nsnull, 0);

                {   
                    XPCAutoLock lock(rt->GetMapLock());
                    XPCNativeSet* set2 = map2->Add(&key, set);
                    if(!set2)
                    {
                        NS_ERROR("failed to add our set!");
                        DestroyInstance(set);
                        set = nsnull;
                        goto out;
                    }
                    if(set2 != set)
                    {
                        DestroyInstance(set);
                        set = set2;
                    }
                }
            }
        }
        else
            set = GetNewOrUsed(ccx, &NS_GET_IID(nsISupports));
    }
    else
        set = GetNewOrUsed(ccx, &NS_GET_IID(nsISupports));

    if(set)
    {   
        XPCAutoLock lock(rt->GetMapLock());

#ifdef DEBUG
        XPCNativeSet* set2 =
#endif
          map->Add(classInfo, set);
        NS_ASSERTION(set2, "failed to add our set!");
        NS_ASSERTION(set2 == set, "hashtables inconsistent!");
    }

out:
    if(iidArray)
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(iidCount, iidArray);
    if(interfaceArray)
        delete [] interfaceArray.get();

    return set;
}


void
XPCNativeSet::ClearCacheEntryForClassInfo(nsIClassInfo* classInfo)
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    ClassInfo2NativeSetMap* map = rt->GetClassInfo2NativeSetMap();
    if(map)
    {   
        XPCAutoLock lock(rt->GetMapLock());
        map->Remove(classInfo);
    }
}


XPCNativeSet*
XPCNativeSet::GetNewOrUsed(XPCCallContext& ccx,
                           XPCNativeSet* otherSet,
                           XPCNativeInterface* newInterface,
                           PRUint16 position)
{
    AutoMarkingNativeSetPtr set(ccx);
    XPCJSRuntime* rt = ccx.GetRuntime();
    NativeSetMap* map = rt->GetNativeSetMap();
    if(!map)
        return nsnull;

    XPCNativeSetKey key(otherSet, newInterface, position);

    {   
        XPCAutoLock lock(rt->GetMapLock());
        set = map->Find(&key);
    }

    if(set)
        return set;

    if(otherSet)
        set = NewInstanceMutate(otherSet, newInterface, position);
    else
        set = NewInstance(ccx, &newInterface, 1);

    if(!set)
        return nsnull;

    {   
        XPCAutoLock lock(rt->GetMapLock());
        XPCNativeSet* set2 = map->Add(&key, set);
        if(!set2)
        {
            NS_ERROR("failed to add our set!");
            DestroyInstance(set);
            set = nsnull;
        }
        else if(set2 != set)
        {
            DestroyInstance(set);
            set = set2;
        }
    }

    return set;
}


XPCNativeSet*
XPCNativeSet::NewInstance(XPCCallContext& ccx,
                          XPCNativeInterface** array,
                          PRUint16 count)
{
    XPCNativeSet* obj = nsnull;

    if(!array || !count)
        return nsnull;

    
    
    
    

    XPCNativeInterface* isup = XPCNativeInterface::GetISupports(ccx);
    PRUint16 slots = count+1;

    PRUint16 i;
    XPCNativeInterface** pcur;

    for(i = 0, pcur = array; i < count; i++, pcur++)
    {
        if(*pcur == isup)
            slots--;
    }

    
    
    int size = sizeof(XPCNativeSet);
    if(slots > 1)
        size += (slots - 1) * sizeof(XPCNativeInterface*);
    void* place = new char[size];
    if(place)
        obj = new(place) XPCNativeSet();

    if(obj)
    {
        
        XPCNativeInterface** inp = array;
        XPCNativeInterface** outp = (XPCNativeInterface**) &obj->mInterfaces;
        PRUint16 memberCount = 1;   

        *(outp++) = isup;

        for(i = 0; i < count; i++)
        {
            XPCNativeInterface* cur;

            if(isup == (cur = *(inp++)))
                continue;
            *(outp++) = cur;
            memberCount += cur->GetMemberCount();
        }
        obj->mMemberCount = memberCount;
        obj->mInterfaceCount = slots;
    }

    return obj;
}


XPCNativeSet*
XPCNativeSet::NewInstanceMutate(XPCNativeSet*       otherSet,
                                XPCNativeInterface* newInterface,
                                PRUint16            position)
{
    XPCNativeSet* obj = nsnull;

    if(!newInterface)
        return nsnull;
    if(otherSet && position > otherSet->mInterfaceCount)
        return nsnull;

    
    
    int size = sizeof(XPCNativeSet);
    if(otherSet)
        size += otherSet->mInterfaceCount * sizeof(XPCNativeInterface*);
    void* place = new char[size];
    if(place)
        obj = new(place) XPCNativeSet();

    if(obj)
    {
        if(otherSet)
        {
            obj->mMemberCount = otherSet->GetMemberCount() +
                                newInterface->GetMemberCount();
            obj->mInterfaceCount = otherSet->mInterfaceCount + 1;

            XPCNativeInterface** src = otherSet->mInterfaces;
            XPCNativeInterface** dest = obj->mInterfaces;
            for(PRUint16 i = 0; i < obj->mInterfaceCount; i++)
            {
                if(i == position)
                    *dest++ = newInterface;
                else
                    *dest++ = *src++;
            }
        }
        else
        {
            obj->mMemberCount = newInterface->GetMemberCount();
            obj->mInterfaceCount = 1;
            obj->mInterfaces[0] = newInterface;
        }
    }

    return obj;
}


void
XPCNativeSet::DestroyInstance(XPCNativeSet* inst)
{
    inst->~XPCNativeSet();
    delete [] (char*) inst;
}

void
XPCNativeSet::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCNativeSet @ %x", this));
        XPC_LOG_INDENT();

        XPC_LOG_ALWAYS(("mInterfaceCount of %d", mInterfaceCount));
        if(depth)
        {
            for(PRUint16 i = 0; i < mInterfaceCount; i++)
                mInterfaces[i]->DebugDump(depth);
        }
        XPC_LOG_ALWAYS(("mMemberCount of %d", mMemberCount));
        XPC_LOG_OUTDENT();
#endif
}
