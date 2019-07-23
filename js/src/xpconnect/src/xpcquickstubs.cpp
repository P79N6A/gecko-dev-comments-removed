






































#include "jsapi.h"
#include "jsstr.h"
#include "jscntxt.h"  
#include "nsCOMPtr.h"
#include "xpcprivate.h"
#include "xpcinlines.h"
#include "xpcquickstubs.h"
#include "XPCWrapper.h"
#include "XPCNativeWrapper.h"

static const xpc_qsHashEntry *
LookupEntry(PRUint32 tableSize, const xpc_qsHashEntry *table, const nsID &iid)
{
    size_t i;
    const xpc_qsHashEntry *p;

    i = iid.m0 % tableSize;
    do
    {
        p = table + i;
        if(p->iid.Equals(iid))
            return p;
        i = p->chain;
    } while(i != XPC_QS_NULL_INDEX);
    return nsnull;
}

static const xpc_qsHashEntry *
LookupInterfaceOrAncestor(PRUint32 tableSize, const xpc_qsHashEntry *table,
                          const nsID &iid)
{
    const xpc_qsHashEntry *entry = LookupEntry(tableSize, table, iid);
    if(!entry)
    {
        



        nsCOMPtr<nsIInterfaceInfo> info;
        if(NS_FAILED(nsXPConnect::GetXPConnect()->GetInfoForIID(
                          &iid, getter_AddRefs(info))))
            return nsnull;

        const nsIID *piid;
        for(;;)
        {
            nsCOMPtr<nsIInterfaceInfo> parent;
            if(NS_FAILED(info->GetParent(getter_AddRefs(parent))) ||
               !parent ||
               NS_FAILED(parent->GetIIDShared(&piid)))
            {
                break;
            }
            entry = LookupEntry(tableSize, table, *piid);
            if(entry)
                break;
            info.swap(parent);
        }
    }
    return entry;
}

static JSBool
PropertyOpForwarder(JSContext *cx, uintN argc, jsval *vp)
{
    
    
    
    

    JSObject *callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    jsval v;

    if(!JS_GetReservedSlot(cx, callee, 0, &v))
        return JS_FALSE;
    JSObject *ptrobj = JSVAL_TO_OBJECT(v);
    JSPropertyOp *popp = static_cast<JSPropertyOp *>(JS_GetPrivate(cx, ptrobj));

    if(!JS_GetReservedSlot(cx, callee, 1, &v))
        return JS_FALSE;

    jsval argval = (argc > 0) ? JS_ARGV(cx, vp)[0] : JSVAL_VOID;
    JS_SET_RVAL(cx, vp, argval);
    return (*popp)(cx, obj, v, vp);
}

static void
PointerFinalize(JSContext *cx, JSObject *obj)
{
    JSPropertyOp *popp = static_cast<JSPropertyOp *>(JS_GetPrivate(cx, obj));
    delete popp;
}

static JSClass
PointerHolderClass = {
    "Pointer", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, PointerFinalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSObject *
GeneratePropertyOp(JSContext *cx, JSObject *obj, jsval idval, uintN argc,
                   const char *name, JSPropertyOp pop)
{
    
    
    JSFunction *fun =
        JS_NewFunction(cx, reinterpret_cast<JSNative>(PropertyOpForwarder),
                       argc, JSFUN_FAST_NATIVE, obj, name);
    if(!fun)
        return JS_FALSE;

    JSObject *funobj = JS_GetFunctionObject(fun);

    JSAutoTempValueRooter tvr(cx, OBJECT_TO_JSVAL(funobj));

    
    
    JSObject *ptrobj = JS_NewObject(cx, &PointerHolderClass, nsnull, funobj);
    if(!ptrobj)
        return JS_FALSE;
    JSPropertyOp *popp = new JSPropertyOp;
    if(!popp)
        return JS_FALSE;
    *popp = pop;
    JS_SetPrivate(cx, ptrobj, popp);

    JS_SetReservedSlot(cx, funobj, 0, OBJECT_TO_JSVAL(ptrobj));
    JS_SetReservedSlot(cx, funobj, 1, idval);
    return funobj;
}

static JSBool
ReifyPropertyOps(JSContext *cx, JSObject *obj, jsval idval, jsid interned_id,
                 const char *name, JSPropertyOp getter, JSPropertyOp setter,
                 JSObject **getterobjp, JSObject **setterobjp)
{
    
    jsval roots[2] = { JSVAL_NULL, JSVAL_NULL };
    JSAutoTempValueRooter tvr(cx, 2, roots);

    uintN attrs = JSPROP_SHARED;
    JSObject *getterobj;
    if(getter)
    {
        getterobj = GeneratePropertyOp(cx, obj, idval, 0, name, getter);
        if(!getterobj)
            return JS_FALSE;
        roots[0] = OBJECT_TO_JSVAL(getterobj);
        attrs |= JSPROP_GETTER;
    }
    else
        getterobj = nsnull;

    JSObject *setterobj;
    if (setter)
    {
        setterobj = GeneratePropertyOp(cx, obj, idval, 1, name, setter);
        if(!setterobj)
            return JS_FALSE;
        roots[1] = OBJECT_TO_JSVAL(setterobj);
        attrs |= JSPROP_SETTER;
    }
    else
        setterobj = nsnull;

    if(getterobjp)
        *getterobjp = getterobj;
    if(setterobjp)
        *setterobjp = setterobj;
    return JS_DefinePropertyById(cx, obj, interned_id, JSVAL_VOID,
                                 JS_DATA_TO_FUNC_PTR(JSPropertyOp, getterobj),
                                 JS_DATA_TO_FUNC_PTR(JSPropertyOp, setterobj),
                                 attrs);
}

static JSBool
LookupGetterOrSetter(JSContext *cx, JSBool wantGetter, jsval *vp)
{
    uintN attrs;
    JSBool found;
    JSPropertyOp getter, setter;
    JSObject *obj2;
    jsid interned_id;
    jsval v;

    XPC_QS_ASSERT_CONTEXT_OK(cx);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;
    jsval idval = JS_ARGV(cx, vp)[0];

    const char *name = JSVAL_IS_STRING(idval)
                       ? JS_GetStringBytes(JSVAL_TO_STRING(idval))
                       : nsnull;
    if(!JS_ValueToId(cx, idval, &interned_id) ||
       !JS_LookupPropertyWithFlagsById(cx, obj, interned_id,
                                       JSRESOLVE_QUALIFIED, &obj2, &v) ||
       (obj2 &&
        !JS_GetPropertyAttrsGetterAndSetterById(cx, obj2, interned_id, &attrs,
                                                &found, &getter, &setter)))
        return JS_FALSE;

    
    if(!obj2 || !found)
    {
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
        return JS_TRUE;
    }

    
    if(wantGetter)
    {
        if(attrs & JSPROP_GETTER)
        {
            JS_SET_RVAL(cx, vp,
                        OBJECT_TO_JSVAL(JS_FUNC_TO_DATA_PTR(JSObject *, getter)));
            return JS_TRUE;
        }
    }
    else
    {
        if(attrs & JSPROP_SETTER)
        {
            JS_SET_RVAL(cx, vp,
                        OBJECT_TO_JSVAL(JS_FUNC_TO_DATA_PTR(JSObject *, setter)));
            return JS_TRUE;
        }
    }

    
    
    
    
    if(!name ||
       !IS_PROTO_CLASS(STOBJ_GET_CLASS(obj2)) ||
       (attrs & (JSPROP_GETTER | JSPROP_SETTER)) ||
       !(getter || setter))
    {
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
        return JS_TRUE;
    }

    JSObject *getterobj, *setterobj;
    if(!ReifyPropertyOps(cx, obj, idval, interned_id, name, getter, setter,
                         &getterobj, &setterobj))
        return JS_FALSE;

    JSObject *wantedobj = wantGetter ? getterobj : setterobj;
    v = wantedobj ? OBJECT_TO_JSVAL(wantedobj) : JSVAL_VOID;
    JS_SET_RVAL(cx, vp, v);
    return JS_TRUE;
}

static JSBool
SharedLookupGetter(JSContext *cx, uintN argc, jsval *vp)
{
    return LookupGetterOrSetter(cx, PR_TRUE, vp);
}

static JSBool
SharedLookupSetter(JSContext *cx, uintN argc, jsval *vp)
{
    return LookupGetterOrSetter(cx, PR_FALSE, vp);
}


JS_FRIEND_API(JSBool) js_obj_defineGetter(JSContext *cx, uintN argc, jsval *vp);
JS_FRIEND_API(JSBool) js_obj_defineSetter(JSContext *cx, uintN argc, jsval *vp);

static JSBool
DefineGetterOrSetter(JSContext *cx, uintN argc, JSBool wantGetter, jsval *vp)
{
    uintN attrs;
    JSBool found;
    JSPropertyOp getter, setter;
    JSObject *obj2;
    jsval v;
    jsid interned_id;

    XPC_QS_ASSERT_CONTEXT_OK(cx);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;
    JSFastNative forward = wantGetter ? js_obj_defineGetter : js_obj_defineSetter;
    jsval id = (argc >= 1) ? JS_ARGV(cx, vp)[0] : JSVAL_VOID;
    if(!JSVAL_IS_STRING(id))
        return forward(cx, argc, vp);
    JSString *str = JSVAL_TO_STRING(id);

    const char *name = JS_GetStringBytes(str);
    if(!JS_ValueToId(cx, id, &interned_id) ||
       !JS_LookupPropertyWithFlagsById(cx, obj, interned_id,
                                       JSRESOLVE_QUALIFIED, &obj2, &v) ||
       (obj2 &&
        !JS_GetPropertyAttrsGetterAndSetterById(cx, obj2, interned_id, &attrs,
                                                &found, &getter, &setter)))
        return JS_FALSE;

    
    
    if(!obj2 ||
       (attrs & (JSPROP_GETTER | JSPROP_SETTER)) ||
       !(getter || setter) ||
       !IS_PROTO_CLASS(STOBJ_GET_CLASS(obj2)))
        return forward(cx, argc, vp);

    
    if(!ReifyPropertyOps(cx, obj, id, interned_id, name, getter, setter,
                         nsnull, nsnull))
        return JS_FALSE;

    return forward(cx, argc, vp);
}

static JSBool
SharedDefineGetter(JSContext *cx, uintN argc, jsval *vp)
{
    return DefineGetterOrSetter(cx, argc, PR_TRUE, vp);
}

static JSBool
SharedDefineSetter(JSContext *cx, uintN argc, jsval *vp)
{
    return DefineGetterOrSetter(cx, argc, PR_FALSE, vp);
}


JSBool
xpc_qsDefineQuickStubs(JSContext *cx, JSObject *proto, uintN flags,
                       PRUint32 ifacec, const nsIID **interfaces,
                       PRUint32 tableSize, const xpc_qsHashEntry *table)
{
    







    PRBool definedProperty = PR_FALSE;
    for(uint32 i = ifacec; i-- != 0;)
    {
        const nsID &iid = *interfaces[i];
        const xpc_qsHashEntry *entry =
            LookupInterfaceOrAncestor(tableSize, table, iid);

        if(entry)
        {
            for(;;)
            {
                
                const xpc_qsPropertySpec *ps = entry->properties;
                if(ps)
                {
                    for(; ps->name; ps++)
                    {
                        definedProperty = PR_TRUE;
                        if(!JS_DefineProperty(cx, proto, ps->name, JSVAL_VOID,
                                              ps->getter, ps->setter,
                                              flags | JSPROP_SHARED))
                            return JS_FALSE;
                    }
                }

                
                const xpc_qsFunctionSpec *fs = entry->functions;
                if(fs)
                {
                    for(; fs->name; fs++)
                    {
                        if(!JS_DefineFunction(
                               cx, proto, fs->name,
                               reinterpret_cast<JSNative>(fs->native),
                               fs->arity, flags | JSFUN_FAST_NATIVE))
                            return JS_FALSE;
                    }
                }

                const xpc_qsTraceableSpec *ts = entry->traceables;
                if(ts)
                {
                    for(; ts->name; ts++)
                    {
                        if(!JS_DefineFunction(
                               cx, proto, ts->name, ts->native, ts->arity,
                               flags | JSFUN_FAST_NATIVE | JSFUN_STUB_GSOPS |
                                       JSFUN_TRACEABLE))
                            return JS_FALSE;
                    }
                }

                
                size_t j = entry->parentInterface;
                if(j == XPC_QS_NULL_INDEX)
                    break;
                entry = table + j;
            }
        }
    }

    static JSFunctionSpec getterfns[] = {
        JS_FN("__lookupGetter__", SharedLookupGetter, 1, 0),
        JS_FN("__lookupSetter__", SharedLookupSetter, 1, 0),
        JS_FN("__defineGetter__", SharedDefineGetter, 2, 0),
        JS_FN("__defineSetter__", SharedDefineSetter, 2, 0),
        JS_FS_END
    };

    if(definedProperty && !JS_DefineFunctions(cx, proto, getterfns))
        return JS_FALSE;

    return JS_TRUE;
}

JSBool
xpc_qsThrow(JSContext *cx, nsresult rv)
{
    XPCThrower::Throw(rv, cx);
    return JS_FALSE;
}










static void
GetMemberInfo(JSObject *obj,
              jsval memberId,
              const char **ifaceName,
              const char **memberName)
{
    
    
    
    
    
    *ifaceName = "Unknown";

    NS_ASSERTION(IS_WRAPPER_CLASS(STOBJ_GET_CLASS(obj)) ||
                 STOBJ_GET_CLASS(obj) == &XPC_WN_Tearoff_JSClass,
                 "obj must be an XPCWrappedNative");
    XPCWrappedNative *wrapper = (XPCWrappedNative *) STOBJ_GET_PRIVATE(obj);
    XPCWrappedNativeProto *proto = wrapper->GetProto();
    if(proto)
    {
        XPCNativeSet *set = proto->GetSet();
        if(set)
        {
            XPCNativeMember *member;
            XPCNativeInterface *iface;

            if(set->FindMember(memberId, &member, &iface))
                *ifaceName = iface->GetNameString();
        }
    }

    *memberName = (JSVAL_IS_STRING(memberId)
                   ? JS_GetStringBytes(JSVAL_TO_STRING(memberId))
                   : "unknown");
}

static void
GetMethodInfo(JSContext *cx,
              jsval *vp,
              const char **ifaceName,
              const char **memberName)
{
    JSObject *funobj = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
    NS_ASSERTION(JS_ObjectIsFunction(cx, funobj),
                 "JSFastNative callee should be Function object");
    JSString *str = JS_GetFunctionId((JSFunction *) JS_GetPrivate(cx, funobj));
    jsval methodId = str ? STRING_TO_JSVAL(str) : JSVAL_NULL;

    GetMemberInfo(JSVAL_TO_OBJECT(vp[1]), methodId, ifaceName, memberName);
}

static JSBool
ThrowCallFailed(JSContext *cx, nsresult rv,
                const char *ifaceName, const char *memberName)
{
    
    char* sz;
    const char* format;
    const char* name;

    





    if(XPCThrower::CheckForPendingException(rv, cx))
        return JS_FALSE;

    

    if(!nsXPCException::NameAndFormatForNSResult(
            NS_ERROR_XPC_NATIVE_RETURNED_FAILURE, nsnull, &format) ||
        !format)
    {
        format = "";
    }

    if(nsXPCException::NameAndFormatForNSResult(rv, &name, nsnull)
        && name)
    {
        sz = JS_smprintf("%s 0x%x (%s) [%s.%s]",
                         format, rv, name, ifaceName, memberName);
    }
    else
    {
        sz = JS_smprintf("%s 0x%x [%s.%s]",
                         format, rv, ifaceName, memberName);
    }

    XPCThrower::BuildAndThrowException(cx, rv, sz);

    if(sz)
        JS_smprintf_free(sz);

    return JS_FALSE;
}

JSBool
xpc_qsThrowGetterSetterFailed(JSContext *cx, nsresult rv, JSObject *obj,
                              jsval memberId)
{
    const char *ifaceName, *memberName;
    GetMemberInfo(obj, memberId, &ifaceName, &memberName);
    return ThrowCallFailed(cx, rv, ifaceName, memberName);
}

JSBool
xpc_qsThrowMethodFailed(JSContext *cx, nsresult rv, jsval *vp)
{
    const char *ifaceName, *memberName;
    GetMethodInfo(cx, vp, &ifaceName, &memberName);
    return ThrowCallFailed(cx, rv, ifaceName, memberName);
}

JSBool
xpc_qsThrowMethodFailedWithCcx(XPCCallContext &ccx, nsresult rv)
{
    ThrowBadResult(rv, ccx);
    return JS_FALSE;
}

void
xpc_qsThrowMethodFailedWithDetails(JSContext *cx, nsresult rv,
                                   const char *ifaceName,
                                   const char *memberName)
{
    ThrowCallFailed(cx, rv, ifaceName, memberName);
}

static void
ThrowBadArg(JSContext *cx, nsresult rv,
            const char *ifaceName, const char *memberName, uintN paramnum)
{
    
    char* sz;
    const char* format;

    if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";

    sz = JS_smprintf("%s arg %u [%s.%s]",
                     format, (unsigned int) paramnum, ifaceName, memberName);

    XPCThrower::BuildAndThrowException(cx, rv, sz);

    if(sz)
        JS_smprintf_free(sz);
}

void
xpc_qsThrowBadArg(JSContext *cx, nsresult rv, jsval *vp, uintN paramnum)
{
    const char *ifaceName, *memberName;
    GetMethodInfo(cx, vp, &ifaceName, &memberName);
    ThrowBadArg(cx, rv, ifaceName, memberName, paramnum);
}

void
xpc_qsThrowBadArgWithCcx(XPCCallContext &ccx, nsresult rv, uintN paramnum)
{
    XPCThrower::ThrowBadParam(rv, paramnum, ccx);
}

void
xpc_qsThrowBadArgWithDetails(JSContext *cx, nsresult rv, uintN paramnum,
                             const char *ifaceName, const char *memberName)
{
    ThrowBadArg(cx, rv, ifaceName, memberName, paramnum);
}

void
xpc_qsThrowBadSetterValue(JSContext *cx, nsresult rv,
                          JSObject *obj, jsval propId)
{
    const char *ifaceName, *memberName;
    GetMemberInfo(obj, propId, &ifaceName, &memberName);
    ThrowBadArg(cx, rv, ifaceName, memberName, 0);
}

xpc_qsDOMString::xpc_qsDOMString(JSContext *cx, jsval *pval)
{
    
    typedef implementation_type::char_traits traits;
    jsval v;
    JSString *s;
    const PRUnichar *chars;
    size_t len;

    v = *pval;
    if(JSVAL_IS_STRING(v))
    {
        s = JSVAL_TO_STRING(v);
    }
    else
    {
        if(JSVAL_IS_NULL(v))
        {
            (new(mBuf) implementation_type(
                traits::sEmptyBuffer, PRUint32(0)))->SetIsVoid(PR_TRUE);
            mValid = JS_TRUE;
            return;
        }

        s = JS_ValueToString(cx, v);
        if(!s)
        {
            mValid = JS_FALSE;
            return;
        }
        *pval = STRING_TO_JSVAL(s);  
    }

    len = JS_GetStringLength(s);
    chars = (len == 0 ? traits::sEmptyBuffer : (const PRUnichar*)JS_GetStringChars(s));
    new(mBuf) implementation_type(chars, len);
    mValid = JS_TRUE;
}

xpc_qsAString::xpc_qsAString(JSContext *cx, jsval *pval)
{
    
    typedef implementation_type::char_traits traits;
    jsval v;
    JSString *s;
    const PRUnichar *chars;
    size_t len;

    v = *pval;
    if(JSVAL_IS_STRING(v))
    {
        s = JSVAL_TO_STRING(v);
    }
    else
    {
        if(JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v))
        {
            (new(mBuf) implementation_type(
                traits::sEmptyBuffer, PRUint32(0)))->SetIsVoid(PR_TRUE);
            mValid = JS_TRUE;
            return;
        }

        s = JS_ValueToString(cx, v);
        if(!s)
        {
            mValid = JS_FALSE;
            return;
        }
        *pval = STRING_TO_JSVAL(s);  
    }

    len = JS_GetStringLength(s);
    chars = (len == 0 ? traits::sEmptyBuffer : (const PRUnichar*)JS_GetStringChars(s));
    new(mBuf) implementation_type(chars, len);
    mValid = JS_TRUE;
}

xpc_qsACString::xpc_qsACString(JSContext *cx, jsval *pval)
{
    
    jsval v;
    JSString *s;

    v = *pval;
    if(JSVAL_IS_STRING(v))
    {
        s = JSVAL_TO_STRING(v);
    }
    else
    {
        if(JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v))
        {
            (new(mBuf) implementation_type())->SetIsVoid(PR_TRUE);
            mValid = JS_TRUE;
            return;
        }

        s = JS_ValueToString(cx, v);
        if(!s)
        {
            mValid = JS_FALSE;
            return;
        }
        *pval = STRING_TO_JSVAL(s);  
    }

    const char *bytes = JS_GetStringBytes(s);
    size_t len = JS_GetStringLength(s);
    new(mBuf) implementation_type(bytes, len);
    mValid = JS_TRUE;
}

static nsresult
getNativeFromWrapper(XPCWrappedNative *wrapper,
                     const nsIID &iid,
                     void **ppThis,
                     nsISupports **pThisRef,
                     jsval *vp)
{
    nsISupports *idobj = wrapper->GetIdentityObject();

    
    QITableEntry* entries = wrapper->GetOffsets();
    if(entries)
    {
        for(QITableEntry* e = entries; e->iid; e++)
        {
            if(e->iid->Equals(iid))
            {
                *ppThis = (char*) idobj + e->offset - entries[0].offset;
                *vp = OBJECT_TO_JSVAL(wrapper->GetFlatJSObject());
                *pThisRef = nsnull;
                return NS_OK;
            }
        }
    }

    nsresult rv = idobj->QueryInterface(iid, ppThis);
    *pThisRef = static_cast<nsISupports*>(*ppThis);
    if(NS_SUCCEEDED(rv))
        *vp = OBJECT_TO_JSVAL(wrapper->GetFlatJSObject());
    return rv;
}

JSBool
xpc_qsUnwrapThisImpl(JSContext *cx,
                     JSObject *obj,
                     const nsIID &iid,
                     void **ppThis,
                     nsISupports **pThisRef,
                     jsval *vp)
{
    
    
    
    

    NS_ASSERTION(obj, "this == null");

    JSObject *cur = obj;
    while(cur)
    {
        JSClass *clazz;
        XPCWrappedNative *wrapper;
        nsresult rv;

        clazz = STOBJ_GET_CLASS(cur);
        if(IS_WRAPPER_CLASS(clazz))
        {
            wrapper = (XPCWrappedNative*) xpc_GetJSPrivate(cur);
            NS_ASSERTION(wrapper, "XPCWN wrapping nothing");
        }
        else if(clazz == &XPC_WN_Tearoff_JSClass)
        {
            wrapper = (XPCWrappedNative*) xpc_GetJSPrivate(STOBJ_GET_PARENT(cur));
            NS_ASSERTION(wrapper, "XPCWN wrapping nothing");
        }
        else
        {
            JSObject *unsafeObj = XPCWrapper::Unwrap(cx, cur);
            if(unsafeObj)
            {
                cur = unsafeObj;
                continue;
            }

            
            
            goto next;
        }

        rv = getNativeFromWrapper(wrapper, iid, ppThis, pThisRef, vp);
        if(NS_SUCCEEDED(rv))
            return JS_TRUE;
        if(rv != NS_ERROR_NO_INTERFACE)
            return xpc_qsThrow(cx, rv);

    next:
        cur = STOBJ_GET_PROTO(cur);
    }

    
    

    JSClass *clazz = STOBJ_GET_CLASS(obj);

    if((clazz->flags & JSCLASS_IS_EXTENDED) &&
        ((JSExtendedClass*)clazz)->outerObject)
    {
        JSObject *outer = ((JSExtendedClass*)clazz)->outerObject(cx, obj);

        
        JSObject *unsafeObj;
        clazz = STOBJ_GET_CLASS(outer);
        if(clazz == &sXPC_XOW_JSClass.base &&
           (unsafeObj = XPCWrapper::Unwrap(cx, outer)))
        {
            outer = unsafeObj;
        }

        if(outer && outer != obj)
            return xpc_qsUnwrapThisImpl(cx, outer, iid, ppThis, pThisRef, vp);
    }

    *pThisRef = nsnull;
    return xpc_qsThrow(cx, NS_ERROR_XPC_BAD_OP_ON_WN_PROTO);
}

JSBool
xpc_qsUnwrapThisFromCcxImpl(XPCCallContext &ccx,
                            const nsIID &iid,
                            void **ppThis,
                            nsISupports **pThisRef,
                            jsval *vp)
{
    XPCWrappedNative *wrapper = ccx.GetWrapper();
    if(!wrapper)
        return xpc_qsThrow(ccx.GetJSContext(), NS_ERROR_XPC_BAD_OP_ON_WN_PROTO);
    if(!wrapper->IsValid())
        return xpc_qsThrow(ccx.GetJSContext(), NS_ERROR_XPC_HAS_BEEN_SHUTDOWN);

    nsresult rv = getNativeFromWrapper(wrapper, iid, ppThis, pThisRef, vp);
    if(NS_FAILED(rv))
        return xpc_qsThrow(ccx.GetJSContext(), rv);
    return JS_TRUE;
}

nsresult
xpc_qsUnwrapArgImpl(JSContext *cx,
                    jsval v,
                    const nsIID &iid,
                    void **ppArg)
{
    
    if(JSVAL_IS_VOID(v) || JSVAL_IS_NULL(v))
        return NS_OK;

    if(!JSVAL_IS_OBJECT(v))
    {
        return ((JSVAL_IS_INT(v) && JSVAL_TO_INT(v) == 0)
                ? NS_ERROR_XPC_BAD_CONVERT_JS_ZERO_ISNOT_NULL
                : NS_ERROR_XPC_BAD_CONVERT_JS);
    }
    JSObject *src = JSVAL_TO_OBJECT(v);

    
    XPCWrappedNative* wrappedNative =
        XPCWrappedNative::GetWrappedNativeOfJSObject(cx, src);
    nsISupports *iface;
    if(wrappedNative)
    {
        iface = wrappedNative->GetIdentityObject();
        if(NS_FAILED(iface->QueryInterface(iid, ppArg)))
            return NS_ERROR_XPC_BAD_CONVERT_JS;
        return NS_OK;
    }
    
    

    
    
    
    if(JS_TypeOfValue(cx, OBJECT_TO_JSVAL(src)) == JSTYPE_XML)
        return NS_ERROR_XPC_BAD_CONVERT_JS;

    
    
    
    if(XPCConvert::GetISupportsFromJSObject(src, &iface))
    {
        if(!iface || NS_FAILED(iface->QueryInterface(iid, ppArg)))
            return NS_ERROR_XPC_BAD_CONVERT_JS;
        return NS_OK;
    }

    
    XPCCallContext ccx(JS_CALLER, cx);
    if(!ccx.IsValid())
        return NS_ERROR_XPC_BAD_CONVERT_JS;

    nsXPCWrappedJS *wrapper;
    nsresult rv =
        nsXPCWrappedJS::GetNewOrUsed(ccx, src, iid, nsnull, &wrapper);
    if(NS_FAILED(rv) || !wrapper)
        return rv;

    
    
    
    
    rv = wrapper->QueryInterface(iid, ppArg);
    NS_RELEASE(wrapper);
    return rv;
}

JSBool
xpc_qsJsvalToCharStr(JSContext *cx, jsval *pval, char **pstr)
{
    jsval v = *pval;
    JSString *str;

    if(JSVAL_IS_STRING(v))
    {
        str = JSVAL_TO_STRING(v);
    }
    else if(JSVAL_IS_VOID(v) || JSVAL_IS_NULL(v))
    {
        *pstr = NULL;
        return JS_TRUE;
    }
    else
    {
        if(!(str = JS_ValueToString(cx, v)))
            return JS_FALSE;
        *pval = STRING_TO_JSVAL(str);  
    }

    *pstr = JS_GetStringBytes(str);
    return JS_TRUE;
}

JSBool
xpc_qsJsvalToWcharStr(JSContext *cx, jsval *pval, PRUnichar **pstr)
{
    jsval v = *pval;
    JSString *str;

    if(JSVAL_IS_STRING(v))
    {
        str = JSVAL_TO_STRING(v);
    }
    else if(JSVAL_IS_VOID(v) || JSVAL_IS_NULL(v))
    {
        *pstr = NULL;
        return JS_TRUE;
    }
    else
    {
        if(!(str = JS_ValueToString(cx, v)))
            return JS_FALSE;
        *pval = STRING_TO_JSVAL(str);  
    }

    *pstr = (PRUnichar*)JS_GetStringChars(str);
    return JS_TRUE;
}

JSBool
xpc_qsStringToJsval(JSContext *cx, const nsAString &str, jsval *rval)
{
    
    if(str.IsVoid())
    {
        *rval = JSVAL_NULL;
        return JS_TRUE;
    }

    JSString *jsstr = XPCStringConvert::ReadableToJSString(cx, str);
    if(!jsstr)
        return JS_FALSE;
    *rval = STRING_TO_JSVAL(jsstr);
    return JS_TRUE;
}

JSBool
xpc_qsXPCOMObjectToJsval(XPCCallContext &ccx, nsISupports *p,
                         nsWrapperCache *cache, XPCNativeInterface *iface,
                         jsval *rval)
{
    
    

    JSObject *scope = ccx.GetCurrentJSObject();
    NS_ASSERTION(scope, "bad ccx");

    if(!iface)
        return xpc_qsThrow(ccx, NS_ERROR_XPC_BAD_CONVERT_NATIVE);

    
    
    
    
    
    nsresult rv;
    if(!XPCConvert::NativeInterface2JSObject(ccx, rval, nsnull, p, nsnull,
                                             iface, cache, scope, PR_TRUE,
                                             OBJ_IS_NOT_GLOBAL, &rv))
    {
        
        
        
        if(!JS_IsExceptionPending(ccx))
            xpc_qsThrow(ccx, NS_FAILED(rv) ? rv : NS_ERROR_UNEXPECTED);
        return JS_FALSE;
    }

#ifdef DEBUG
    JSObject* jsobj = JSVAL_TO_OBJECT(*rval);
    if(jsobj && !STOBJ_GET_PARENT(jsobj))
        NS_ASSERTION(STOBJ_GET_CLASS(jsobj)->flags & JSCLASS_IS_GLOBAL,
                     "Why did we recreate this wrapper?");
#endif

    return JS_TRUE;
}

JSBool
xpc_qsVariantToJsval(XPCCallContext &ccx,
                     nsIVariant *p,
                     uintN paramNum,
                     jsval *rval)
{
    
    
    if(p)
    {
        nsresult rv;
        JSBool ok = XPCVariant::VariantDataToJS(ccx, p,
                                                ccx.GetCurrentJSObject(),
                                                &rv, rval);
        if (!ok)
            XPCThrower::ThrowBadParam(rv, 0, ccx);
        return ok;
    }
    *rval = JSVAL_NULL;
    return JS_TRUE;
}

JSBool
xpc_qsReadOnlySetter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_GETTER_ONLY, NULL);
    return JS_FALSE;
}

#ifdef DEBUG
void
xpc_qsAssertContextOK(JSContext *cx)
{
    XPCPerThreadData *thread = XPCPerThreadData::GetData(cx);
    XPCJSContextStack* stack = thread->GetJSContextStack();

    JSContext* topJSContext = nsnull;
    nsresult rv = stack->Peek(&topJSContext);
    NS_ASSERTION(NS_SUCCEEDED(rv), "XPCJSContextStack::Peek failed");

    
    NS_ASSERTION(cx == topJSContext, "wrong context on XPCJSContextStack!");

    NS_ASSERTION(XPCPerThreadData::IsMainThread(cx),
                 "XPConnect quick stub called on non-main thread");
}
#endif
