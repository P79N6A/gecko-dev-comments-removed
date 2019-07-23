






































#ifndef xpcquickstubs_h___
#define xpcquickstubs_h___



class XPCCallContext;

#define XPC_QS_NULL_INDEX  ((size_t) -1)

struct xpc_qsPropertySpec {
    const char *name;
    JSPropertyOp getter;
    JSPropertyOp setter;
};

struct xpc_qsFunctionSpec {
    const char *name;
    JSFastNative native;
    uintN arity;
};

struct xpc_qsTraceableSpec {
    const char *name;
    JSNative native;
    uintN arity;
};


struct xpc_qsHashEntry {
    nsID iid;
    const xpc_qsPropertySpec *properties;
    const xpc_qsFunctionSpec *functions;
    const xpc_qsTraceableSpec *traceables;
    
    
    size_t parentInterface;
    size_t chain;
};

JSBool
xpc_qsDefineQuickStubs(JSContext *cx, JSObject *proto, uintN extraFlags,
                       PRUint32 ifacec, const nsIID **interfaces,
                       PRUint32 tableSize, const xpc_qsHashEntry *table);


JSBool
xpc_qsThrow(JSContext *cx, nsresult rv);














JSBool
xpc_qsThrowGetterSetterFailed(JSContext *cx, nsresult rv,
                              JSObject *obj, jsval memberId);






JSBool
xpc_qsThrowMethodFailed(JSContext *cx, nsresult rv, jsval *vp);

JSBool
xpc_qsThrowMethodFailedWithCcx(XPCCallContext &ccx, nsresult rv);

void
xpc_qsThrowMethodFailedWithDetails(JSContext *cx, nsresult rv,
                                   const char *ifaceName,
                                   const char *memberName);






void
xpc_qsThrowBadArg(JSContext *cx, nsresult rv, jsval *vp, uintN paramnum);

void
xpc_qsThrowBadArgWithCcx(XPCCallContext &ccx, nsresult rv, uintN paramnum);

void
xpc_qsThrowBadArgWithDetails(JSContext *cx, nsresult rv, uintN paramnum,
                             const char *ifaceName, const char *memberName);






void
xpc_qsThrowBadSetterValue(JSContext *cx, nsresult rv, JSObject *obj,
                          jsval propId);




inline JSBool
xpc_qsInt32ToJsval(JSContext *cx, PRInt32 i, jsval *rv)
{
    if(INT_FITS_IN_JSVAL(i))
    {
        *rv = INT_TO_JSVAL(i);
        return JS_TRUE;
    }
    return JS_NewDoubleValue(cx, i, rv);
}

inline JSBool
xpc_qsUint32ToJsval(JSContext *cx, PRUint32 u, jsval *rv)
{
    if(u <= JSVAL_INT_MAX)
    {
        *rv = INT_TO_JSVAL(u);
        return JS_TRUE;
    }
    return JS_NewDoubleValue(cx, u, rv);
}

#ifdef HAVE_LONG_LONG

#define INT64_TO_DOUBLE(i)      ((jsdouble) (i))

#define UINT64_TO_DOUBLE(u)     ((jsdouble) (int64) (u))

#else

inline jsdouble
INT64_TO_DOUBLE(const int64 &v)
{
    jsdouble d;
    LL_L2D(d, v);
    return d;
}


#define UINT64_TO_DOUBLE INT64_TO_DOUBLE

#endif

inline JSBool
xpc_qsInt64ToJsval(JSContext *cx, PRInt64 i, jsval *rv)
{
    double d = INT64_TO_DOUBLE(i);
    return JS_NewNumberValue(cx, d, rv);
}

inline JSBool
xpc_qsUint64ToJsval(JSContext *cx, PRUint64 u, jsval *rv)
{
    double d = UINT64_TO_DOUBLE(u);
    return JS_NewNumberValue(cx, d, rv);
}




template <class S, class T>
class xpc_qsBasicString
{
public:
    typedef S interface_type;
    typedef T implementation_type;

    ~xpc_qsBasicString()
    {
        if (mValid)
            Ptr()->~implementation_type();
    }

    JSBool IsValid() { return mValid; }

    implementation_type *Ptr()
    {
        return reinterpret_cast<implementation_type *>(mBuf);
    }

    operator interface_type &()
    {
        return *Ptr();
    }

protected:
    




    void *mBuf[JS_HOWMANY(sizeof(implementation_type), sizeof(void *))];
    JSBool mValid;
};















class xpc_qsDOMString : public xpc_qsBasicString<nsAString, nsDependentString>
{
public:
    xpc_qsDOMString(JSContext *cx, jsval v, jsval *pval);
};





class xpc_qsAString : public xpc_qsBasicString<nsAString, nsDependentString>
{
public:
    xpc_qsAString(JSContext *cx, jsval v, jsval *pval);
};





class xpc_qsACString : public xpc_qsBasicString<nsACString, nsCString>
{
public:
    xpc_qsACString(JSContext *cx, jsval v, jsval *pval);
};

struct xpc_qsSelfRef
{
    xpc_qsSelfRef() : ptr(nsnull) {}
    explicit xpc_qsSelfRef(nsISupports *p) : ptr(p) {}
    ~xpc_qsSelfRef() { NS_IF_RELEASE(ptr); }

    nsISupports* ptr;
};

template<size_t N>
struct xpc_qsArgValArray
{
    xpc_qsArgValArray(JSContext *cx) : tvr(cx, N, array)
    {
        memset(array, 0, N * sizeof(jsval));
    }

    JSAutoTempValueRooter tvr;
    jsval array[N];
};













JSBool
xpc_qsJsvalToCharStr(JSContext *cx, jsval v, jsval *pval, char **pstr);

JSBool
xpc_qsJsvalToWcharStr(JSContext *cx, jsval v, jsval *pval, PRUnichar **pstr);



JSBool
xpc_qsStringToJsval(JSContext *cx, const nsAString &str, jsval *rval);

JSBool
xpc_qsUnwrapThisImpl(JSContext *cx,
                     JSObject *obj,
                     JSObject *callee,
                     const nsIID &iid,
                     void **ppThis,
                     nsISupports **ppThisRef,
                     jsval *vp,
                     XPCLazyCallContext *lccx);

















template <class T>
inline JSBool
xpc_qsUnwrapThis(JSContext *cx,
                 JSObject *obj,
                 JSObject *callee,
                 T **ppThis,
                 nsISupports **pThisRef,
                 jsval *pThisVal,
                 XPCLazyCallContext *lccx)
{
    return xpc_qsUnwrapThisImpl(cx,
                                obj,
                                callee,
                                NS_GET_TEMPLATE_IID(T),
                                reinterpret_cast<void **>(ppThis),
                                pThisRef,
                                pThisVal,
                                lccx);
}

JSBool
xpc_qsUnwrapThisFromCcxImpl(XPCCallContext &ccx,
                            const nsIID &iid,
                            void **ppThis,
                            nsISupports **pThisRef,
                            jsval *vp);





template <class T>
inline JSBool
xpc_qsUnwrapThisFromCcx(XPCCallContext &ccx,
                        T **ppThis,
                        nsISupports **pThisRef,
                        jsval *pThisVal)
{
    return xpc_qsUnwrapThisFromCcxImpl(ccx,
                                       NS_GET_TEMPLATE_IID(T),
                                       reinterpret_cast<void **>(ppThis),
                                       pThisRef,
                                       pThisVal);
}

nsresult
xpc_qsUnwrapArgImpl(JSContext *cx, jsval v, const nsIID &iid, void **ppArg,
                    nsISupports **ppArgRef, jsval *vp);


template <class T>
inline nsresult
xpc_qsUnwrapArg(JSContext *cx, jsval v, T **ppArg, nsISupports **ppArgRef,
                jsval *vp)
{
    return xpc_qsUnwrapArgImpl(cx, v, NS_GET_TEMPLATE_IID(T),
                               reinterpret_cast<void **>(ppArg), ppArgRef, vp);
}

inline nsWrapperCache*
xpc_qsGetWrapperCache(nsWrapperCache *cache)
{
    return cache;
}

inline nsWrapperCache*
xpc_qsGetWrapperCache(void *p)
{
    return nsnull;
}


JSBool
xpc_qsXPCOMObjectToJsval(XPCLazyCallContext &lccx,
                         nsISupports *p,
                         nsWrapperCache *cache,
                         const nsIID *iid,
                         XPCNativeInterface **iface,
                         jsval *rval);




JSBool
xpc_qsVariantToJsval(XPCLazyCallContext &ccx,
                     nsIVariant *p,
                     jsval *rval);

#ifdef DEBUG
void
xpc_qsAssertContextOK(JSContext *cx);

inline PRBool
xpc_qsSameResult(nsISupports *result1, nsISupports *result2)
{
    return SameCOMIdentity(result1, result2);
}

#define XPC_QS_ASSERT_CONTEXT_OK(cx) xpc_qsAssertContextOK(cx)
#else
#define XPC_QS_ASSERT_CONTEXT_OK(cx) ((void) 0)
#endif

#endif 
