





#ifndef xpcquickstubs_h___
#define xpcquickstubs_h___

#include "XPCForwards.h"

class qsObjectHelper;



class XPCCallContext;


bool
xpc_qsThrow(JSContext *cx, nsresult rv);




bool
xpc_qsThrowMethodFailed(JSContext *cx, nsresult rv, jsval *vp);




void
xpc_qsThrowBadArg(JSContext *cx, nsresult rv, jsval *vp, unsigned paramnum);

void
xpc_qsThrowBadArgWithCcx(XPCCallContext &ccx, nsresult rv, unsigned paramnum);

void
xpc_qsThrowBadArgWithDetails(JSContext *cx, nsresult rv, unsigned paramnum,
                             const char *ifaceName, const char *memberName);

bool
xpc_qsGetterOnlyNativeStub(JSContext *cx, unsigned argc, jsval *vp);



inline bool
xpc_qsInt64ToJsval(JSContext *cx, int64_t i, JS::MutableHandleValue rv)
{
    rv.setNumber(static_cast<double>(i));
    return true;
}

inline bool
xpc_qsUint64ToJsval(JSContext *cx, uint64_t u, JS::MutableHandleValue rv)
{
    rv.setNumber(static_cast<double>(u));
    return true;
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

    bool IsValid() const { return mValid; }

    implementation_type *Ptr()
    {
        MOZ_ASSERT(mValid);
        return reinterpret_cast<implementation_type *>(mBuf);
    }

    const implementation_type *Ptr() const
    {
        MOZ_ASSERT(mValid);
        return reinterpret_cast<const implementation_type *>(mBuf);
    }

    operator interface_type &()
    {
        MOZ_ASSERT(mValid);
        return *Ptr();
    }

    operator const interface_type &() const
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
    bool mValid;

    







    template<class traits>
    JSString* InitOrStringify(JSContext* cx, JS::HandleValue v,
                              JS::MutableHandleValue pval,
                              bool notpassed,
                              StringificationBehavior nullBehavior,
                              StringificationBehavior undefinedBehavior) {
        JSString *s;
        if (v.isString()) {
            s = v.toString();
        } else {
            StringificationBehavior behavior = eStringify;
            if (v.isNull()) {
                behavior = nullBehavior;
            } else if (v.isUndefined()) {
                behavior = undefinedBehavior;
            }

            
            
            
            if (behavior != eStringify || notpassed) {
                
                
                (new(mBuf) implementation_type(traits::sEmptyBuffer, uint32_t(0)))->
                    SetIsVoid(behavior != eEmpty);
                mValid = true;
                return nullptr;
            }

            s = JS::ToString(cx, v);
            if (!s) {
                mValid = false;
                return nullptr;
            }
            pval.setString(s);  
        }

        return s;
    }
};















class xpc_qsDOMString : public xpc_qsBasicString<nsAString, nsAutoString>
{
public:
    xpc_qsDOMString(JSContext *cx, JS::HandleValue v,
                    JS::MutableHandleValue pval, bool notpassed,
                    StringificationBehavior nullBehavior,
                    StringificationBehavior undefinedBehavior);
};





class xpc_qsAString : public xpc_qsDOMString
{
public:
    xpc_qsAString(JSContext *cx, JS::HandleValue v,
                  JS::MutableHandleValue pval, bool notpassed)
        : xpc_qsDOMString(cx, v, pval, notpassed, eNull, eNull)
    {}
};





class xpc_qsACString : public xpc_qsBasicString<nsACString, nsCString>
{
public:
    xpc_qsACString(JSContext *cx, JS::HandleValue v,
                   JS::MutableHandleValue pval, bool notpassed,
                   StringificationBehavior nullBehavior = eNull,
                   StringificationBehavior undefinedBehavior = eNull);
};




class xpc_qsAUTF8String :
  public xpc_qsBasicString<nsACString, NS_ConvertUTF16toUTF8>
{
public:
  xpc_qsAUTF8String(JSContext* cx, JS::HandleValue v,
                    JS::MutableHandleValue pval, bool notpassed);
};

struct xpc_qsSelfRef
{
    xpc_qsSelfRef() : ptr(nullptr) {}
    explicit xpc_qsSelfRef(nsISupports *p) : ptr(p) {}
    ~xpc_qsSelfRef() { NS_IF_RELEASE(ptr); }

    nsISupports* ptr;
};












bool
xpc_qsJsvalToCharStr(JSContext *cx, jsval v, JSAutoByteString *bytes);

bool
xpc_qsJsvalToWcharStr(JSContext *cx, jsval v, JS::MutableHandleValue pval, const char16_t **pstr);


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
           JS::MutableHandleValue vp);

















template <class T>
inline bool
xpc_qsUnwrapThis(JSContext *cx,
                 JS::HandleObject obj,
                 T **ppThis,
                 nsISupports **pThisRef,
                 JS::MutableHandleValue pThisVal,
                 bool failureFatal = true)
{
    XPCWrappedNative *wrapper;
    XPCWrappedNativeTearOff *tearoff;
    JS::RootedObject current(cx);
    nsresult rv = getWrapper(cx, obj, &wrapper, current.address(), &tearoff);
    if (NS_SUCCEEDED(rv))
        rv = castNative(cx, wrapper, current, tearoff, NS_GET_TEMPLATE_IID(T),
                        reinterpret_cast<void **>(ppThis), pThisRef, pThisVal);

    if (failureFatal)
        return NS_SUCCEEDED(rv) || xpc_qsThrow(cx, rv);

    if (NS_FAILED(rv))
        *ppThis = nullptr;
    return true;
}

nsISupports*
castNativeFromWrapper(JSContext *cx,
                      JSObject *obj,
                      uint32_t interfaceBit,
                      uint32_t protoID,
                      int32_t protoDepth,
                      nsISupports **pRef,
                      JS::MutableHandleValue pVal,
                      nsresult *rv);

MOZ_ALWAYS_INLINE JSObject*
xpc_qsUnwrapObj(jsval v, nsISupports **ppArgRef, nsresult *rv)
{
    *rv = NS_OK;
    if (v.isObject()) {
        return &v.toObject();
    }

    if (!v.isNullOrUndefined()) {
        *rv = ((v.isInt32() && v.toInt32() == 0)
               ? NS_ERROR_XPC_BAD_CONVERT_JS_ZERO_ISNOT_NULL
               : NS_ERROR_XPC_BAD_CONVERT_JS);
    }

    *ppArgRef = nullptr;
    return nullptr;
}

nsresult
xpc_qsUnwrapArgImpl(JSContext *cx, JS::HandleValue v, const nsIID &iid, void **ppArg,
                    nsISupports **ppArgRef, JS::MutableHandleValue vp);


template <class Interface, class StrongRefType>
inline nsresult
xpc_qsUnwrapArg(JSContext *cx, JS::HandleValue v, Interface **ppArg,
                StrongRefType **ppArgRef, JS::MutableHandleValue vp)
{
    nsISupports* argRef = *ppArgRef;
    nsresult rv = xpc_qsUnwrapArgImpl(cx, v, NS_GET_TEMPLATE_IID(Interface),
                                      reinterpret_cast<void **>(ppArg), &argRef,
                                      vp);
    *ppArgRef = static_cast<StrongRefType*>(argRef);
    return rv;
}

MOZ_ALWAYS_INLINE nsISupports*
castNativeArgFromWrapper(JSContext *cx,
                         jsval v,
                         uint32_t bit,
                         uint32_t protoID,
                         int32_t protoDepth,
                         nsISupports **pArgRef,
                         JS::MutableHandleValue vp,
                         nsresult *rv)
{
    JSObject *src = xpc_qsUnwrapObj(v, pArgRef, rv);
    if (!src)
        return nullptr;

    return castNativeFromWrapper(cx, src, bit, protoID, protoDepth, pArgRef, vp, rv);
}

inline nsWrapperCache*
xpc_qsGetWrapperCache(nsWrapperCache *cache)
{
    return cache;
}

inline nsWrapperCache*
xpc_qsGetWrapperCache(void *p)
{
    return nullptr;
}





bool
xpc_qsXPCOMObjectToJsval(JSContext *aCx,
                         qsObjectHelper &aHelper,
                         const nsIID *iid,
                         XPCNativeInterface **iface,
                         JS::MutableHandleValue rval);




bool
xpc_qsVariantToJsval(JSContext *cx,
                     nsIVariant *p,
                     JS::MutableHandleValue rval);

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
xpc_qsSameResult(int32_t result1, int32_t result2)
{
    return result1 == result2;
}

#define XPC_QS_ASSERT_CONTEXT_OK(cx) xpc_qsAssertContextOK(cx)
#else
#define XPC_QS_ASSERT_CONTEXT_OK(cx) ((void) 0)
#endif

#endif 
