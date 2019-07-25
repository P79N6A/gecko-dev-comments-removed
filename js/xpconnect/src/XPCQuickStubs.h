






































#ifndef xpcquickstubs_h___
#define xpcquickstubs_h___

#include "xpcpublic.h"
#include "xpcprivate.h"

#include "nsINode.h"



class XPCCallContext;

#define XPC_QS_NULL_INDEX  ((uint16_t) -1)

struct xpc_qsPropertySpec {
    uint16_t name_index;
    JSPropertyOp getter;
    JSStrictPropertyOp setter;
};

struct xpc_qsFunctionSpec {
    uint16_t name_index;
    uint16_t arity;
    JSNative native;
};


struct xpc_qsHashEntry {
    nsID iid;
    uint16_t prop_index;
    uint16_t n_props;
    uint16_t func_index;
    uint16_t n_funcs;
    
    
    uint16_t parentInterface;
    uint16_t chain;
};

inline nsISupports*
ToSupports(nsISupports *p)
{
    return p;
}

inline nsISupports*
ToCanonicalSupports(nsISupports* p)
{
  return nsnull;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 2) || \
    _MSC_FULL_VER >= 140050215



#define QS_CASTABLE_TO(_interface, _class) __is_base_of(_interface, _class)

#else









template <typename Interface> struct QS_CastableTo {
  struct false_type { int x[1]; };
  struct true_type { int x[2]; };
  static false_type p(void*);
  static true_type p(Interface*);
};

#define QS_CASTABLE_TO(_interface, _class)                                    \
  (sizeof(QS_CastableTo<_interface>::p(static_cast<_class*>(0))) ==           \
   sizeof(QS_CastableTo<_interface>::true_type))

#endif

#define QS_IS_NODE(_class)                                                    \
  QS_CASTABLE_TO(nsINode, _class) ||                                          \
  QS_CASTABLE_TO(nsIDOMNode, _class)

class qsObjectHelper : public xpcObjectHelper
{
public:
  template <class T>
  inline
  qsObjectHelper(T *aObject, nsWrapperCache *aCache)
  : xpcObjectHelper(ToSupports(aObject), ToCanonicalSupports(aObject),
                    aCache, QS_IS_NODE(T))
  {}
  template <class T>
  inline
  qsObjectHelper(nsCOMPtr<T>& aObject, nsWrapperCache *aCache)
  : xpcObjectHelper(ToSupports(aObject.get()),
                    ToCanonicalSupports(aObject.get()), aCache, QS_IS_NODE(T))
  {
    if (mCanonical) {
        
        mCanonicalStrong = dont_AddRef(mCanonical);
        aObject.forget();
    }
  }
  template <class T>
  inline
  qsObjectHelper(nsRefPtr<T>& aObject, nsWrapperCache *aCache)
  : xpcObjectHelper(ToSupports(aObject.get()),
                    ToCanonicalSupports(aObject.get()), aCache, QS_IS_NODE(T))
  {
    if (mCanonical) {
        
        mCanonicalStrong = dont_AddRef(mCanonical);
        aObject.forget();
    }
  }
};

JSBool
xpc_qsDefineQuickStubs(JSContext *cx, JSObject *proto, uintN extraFlags,
                       PRUint32 ifacec, const nsIID **interfaces,
                       PRUint32 tableSize, const xpc_qsHashEntry *table,
                       const xpc_qsPropertySpec *propspecs,
                       const xpc_qsFunctionSpec *funcspecs,
                       const char *stringTable);


JSBool
xpc_qsThrow(JSContext *cx, nsresult rv);














JSBool
xpc_qsThrowGetterSetterFailed(JSContext *cx, nsresult rv,
                              JSObject *obj, jsid memberId);






JSBool
xpc_qsThrowMethodFailed(JSContext *cx, nsresult rv, jsval *vp);

JSBool
xpc_qsThrowMethodFailedWithCcx(XPCCallContext &ccx, nsresult rv);

bool
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
                          jsid propId);


JSBool
xpc_qsGetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);



inline JSBool
xpc_qsInt64ToJsval(JSContext *cx, PRInt64 i, jsval *rv)
{
    return JS_NewNumberValue(cx, static_cast<jsdouble>(i), rv);
}

inline JSBool
xpc_qsUint64ToJsval(JSContext *cx, PRUint64 u, jsval *rv)
{
    return JS_NewNumberValue(cx, static_cast<jsdouble>(u), rv);
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
        MOZ_ASSERT(mValid);
        return reinterpret_cast<implementation_type *>(mBuf);
    }

    operator interface_type &()
    {
        MOZ_ASSERT(mValid);
        return *Ptr();
    }

    











    enum StringificationBehavior {
        eStringify,
        eEmpty,
        eNull,
        eDefaultNullBehavior = eNull,
        eDefaultUndefinedBehavior = eStringify
    };

protected:
    




    void *mBuf[JS_HOWMANY(sizeof(implementation_type), sizeof(void *))];
    JSBool mValid;

    







    template<class traits>
    JSString* InitOrStringify(JSContext* cx, jsval v, jsval* pval,
                              StringificationBehavior nullBehavior,
                              StringificationBehavior undefinedBehavior) {
        JSString *s;
        if (JSVAL_IS_STRING(v)) {
            s = JSVAL_TO_STRING(v);
        } else {
            StringificationBehavior behavior = eStringify;
            if (JSVAL_IS_NULL(v)) {
                behavior = nullBehavior;
            } else if (JSVAL_IS_VOID(v)) {
                behavior = undefinedBehavior;
            }

            
            
            
            if (behavior != eStringify || !pval) {
                
                
                (new(mBuf) implementation_type(traits::sEmptyBuffer, PRUint32(0)))->
                    SetIsVoid(behavior != eEmpty);
                mValid = true;
                return nsnull;
            }

            s = JS_ValueToString(cx, v);
            if (!s) {
                mValid = false;
                return nsnull;
            }
            *pval = STRING_TO_JSVAL(s);  
        }

        return s;
    }
};















class xpc_qsDOMString : public xpc_qsBasicString<nsAString, nsDependentString>
{
public:
    xpc_qsDOMString(JSContext *cx, jsval v, jsval *pval,
                    StringificationBehavior nullBehavior,
                    StringificationBehavior undefinedBehavior);
};





class xpc_qsAString : public xpc_qsDOMString
{
public:
    xpc_qsAString(JSContext *cx, jsval v, jsval *pval)
        : xpc_qsDOMString(cx, v, pval, eNull, eNull)
    {}
};





class xpc_qsACString : public xpc_qsBasicString<nsACString, nsCString>
{
public:
    xpc_qsACString(JSContext *cx, jsval v, jsval *pval,
                   StringificationBehavior nullBehavior = eNull,
                   StringificationBehavior undefinedBehavior = eNull);
};




class xpc_qsAUTF8String :
  public xpc_qsBasicString<nsACString, NS_ConvertUTF16toUTF8>
{
public:
  xpc_qsAUTF8String(JSContext* cx, jsval v, jsval *pval);
};

struct xpc_qsSelfRef
{
    xpc_qsSelfRef() : ptr(nsnull) {}
    explicit xpc_qsSelfRef(nsISupports *p) : ptr(p) {}
    ~xpc_qsSelfRef() { NS_IF_RELEASE(ptr); }

    nsISupports* ptr;
};












JSBool
xpc_qsJsvalToCharStr(JSContext *cx, jsval v, JSAutoByteString *bytes);

JSBool
xpc_qsJsvalToWcharStr(JSContext *cx, jsval v, jsval *pval, const PRUnichar **pstr);



JSBool
xpc_qsStringToJsstring(JSContext *cx, nsString &str, JSString **rval);

nsresult
getWrapper(JSContext *cx,
           JSObject *obj,
           XPCWrappedNative **wrapper,
           JSObject **cur,
           XPCWrappedNativeTearOff **tearoff);

nsresult
castNative(JSContext *cx,
           XPCWrappedNative *wrapper,
           JSObject *cur,
           XPCWrappedNativeTearOff *tearoff,
           const nsIID &iid,
           void **ppThis,
           nsISupports **ppThisRef,
           jsval *vp,
           XPCLazyCallContext *lccx);

















template <class T>
inline JSBool
xpc_qsUnwrapThis(JSContext *cx,
                 JSObject *obj,
                 T **ppThis,
                 nsISupports **pThisRef,
                 jsval *pThisVal,
                 XPCLazyCallContext *lccx,
                 bool failureFatal = true)
{
    XPCWrappedNative *wrapper;
    XPCWrappedNativeTearOff *tearoff;
    nsresult rv = getWrapper(cx, obj, &wrapper, &obj, &tearoff);
    if (NS_SUCCEEDED(rv))
        rv = castNative(cx, wrapper, obj, tearoff, NS_GET_TEMPLATE_IID(T),
                        reinterpret_cast<void **>(ppThis), pThisRef, pThisVal,
                        lccx);

    if (failureFatal)
        return NS_SUCCEEDED(rv) || xpc_qsThrow(cx, rv);

    if (NS_FAILED(rv))
        *ppThis = nsnull;
    return true;
}

inline nsISupports*
castNativeFromWrapper(JSContext *cx,
                      JSObject *obj,
                      PRUint32 interfaceBit,
                      nsISupports **pRef,
                      jsval *pVal,
                      XPCLazyCallContext *lccx,
                      nsresult *rv NS_OUTPARAM)
{
    XPCWrappedNative *wrapper;
    XPCWrappedNativeTearOff *tearoff;
    JSObject *cur;

    if (IS_WRAPPER_CLASS(js::GetObjectClass(obj))) {
        cur = obj;
        wrapper = IS_WN_WRAPPER_OBJECT(cur) ?
                  (XPCWrappedNative*)xpc_GetJSPrivate(obj) :
                  nsnull;
        tearoff = nsnull;
    } else {
        *rv = getWrapper(cx, obj, &wrapper, &cur, &tearoff);
        if (NS_FAILED(*rv))
            return nsnull;
    }

    nsISupports *native;
    if (wrapper) {
        native = wrapper->GetIdentityObject();
        cur = wrapper->GetFlatJSObject();
    } else {
        native = cur ?
                 static_cast<nsISupports*>(xpc_GetJSPrivate(cur)) :
                 nsnull;
    }

    *rv = NS_ERROR_XPC_BAD_CONVERT_JS;

    if (!native)
        return nsnull;

    NS_ASSERTION(IS_WRAPPER_CLASS(js::GetObjectClass(cur)), "Not a wrapper?");

    XPCNativeScriptableSharedJSClass *clasp =
      (XPCNativeScriptableSharedJSClass*)js::GetObjectClass(cur);
    if (!(clasp->interfacesBitmap & (1 << interfaceBit)))
        return nsnull;

    *pRef = nsnull;
    *pVal = OBJECT_TO_JSVAL(cur);

    if (lccx) {
        if (wrapper)
            lccx->SetWrapper(wrapper, tearoff);
        else
            lccx->SetWrapper(cur);
    }

    *rv = NS_OK;

    return native;
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

JSObject*
xpc_qsUnwrapObj(jsval v, nsISupports **ppArgRef, nsresult *rv);

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

inline nsISupports*
castNativeArgFromWrapper(JSContext *cx,
                         jsval v,
                         PRUint32 bit,
                         nsISupports **pArgRef,
                         jsval *vp,
                         nsresult *rv NS_OUTPARAM)
{
    JSObject *src = xpc_qsUnwrapObj(v, pArgRef, rv);
    if (!src)
        return nsnull;

    return castNativeFromWrapper(cx, src, bit, pArgRef, vp, nsnull, rv);
}

inline nsWrapperCache*
xpc_qsGetWrapperCache(nsWrapperCache *cache)
{
    return cache;
}



class nsGlobalWindow;
inline nsWrapperCache*
xpc_qsGetWrapperCache(nsGlobalWindow *not_allowed);

inline nsWrapperCache*
xpc_qsGetWrapperCache(void *p)
{
    return nsnull;
}





JSBool
xpc_qsXPCOMObjectToJsval(XPCLazyCallContext &lccx,
                         qsObjectHelper &aHelper,
                         const nsIID *iid,
                         XPCNativeInterface **iface,
                         jsval *rval);




JSBool
xpc_qsVariantToJsval(XPCLazyCallContext &ccx,
                     nsIVariant *p,
                     jsval *rval);




inline JSBool
xpc_qsValueToInt64(JSContext *cx,
                   jsval v,
                   PRInt64 *result)
{
    if (JSVAL_IS_INT(v)) {
        int32_t intval;
        if (!JS_ValueToECMAInt32(cx, v, &intval))
            return false;
        *result = static_cast<PRInt64>(intval);
    } else {
        jsdouble doubleval;
        if (!JS_ValueToNumber(cx, v, &doubleval))
            return false;
        *result = static_cast<PRInt64>(doubleval);
    }
    return true;
}




inline JSBool
xpc_qsValueToUint64(JSContext *cx,
                    jsval v,
                    PRUint64 *result)
{
    if (JSVAL_IS_INT(v)) {
        uint32_t intval;
        if (!JS_ValueToECMAUint32(cx, v, &intval))
            return false;
        *result = static_cast<PRUint64>(intval);
    } else {
        jsdouble doubleval;
        if (!JS_ValueToNumber(cx, v, &doubleval))
            return false;
        *result = static_cast<PRUint64>(doubleval);
    }
    return true;
}

#ifdef DEBUG
void
xpc_qsAssertContextOK(JSContext *cx);

inline bool
xpc_qsSameResult(nsISupports *result1, nsISupports *result2)
{
    return SameCOMIdentity(result1, result2);
}

inline bool
xpc_qsSameResult(const nsString &result1, const nsString &result2)
{
    return result1.Equals(result2);
}

inline bool
xpc_qsSameResult(PRInt32 result1, PRInt32 result2)
{
    return result1 == result2;
}

#define XPC_QS_ASSERT_CONTEXT_OK(cx) xpc_qsAssertContextOK(cx)
#else
#define XPC_QS_ASSERT_CONTEXT_OK(cx) ((void) 0)
#endif

#endif 
