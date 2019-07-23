







































#include "nsMemory.h"
#include "nsString.h"

#include "mozStorageStatementParams.h"








NS_IMPL_ISUPPORTS2(mozStorageStatementParams, mozIStorageStatementParams, nsIXPCScriptable)

mozStorageStatementParams::mozStorageStatementParams(mozIStorageStatement *aStatement)
    : mStatement(aStatement)
{
    NS_ASSERTION(mStatement != nsnull, "mStatement is null");
    mStatement->GetParameterCount(&mParamCount);
}






NS_IMETHODIMP
mozStorageStatementParams::GetClassName(char * *aClassName)
{
    NS_ENSURE_ARG_POINTER(aClassName);
    *aClassName = (char *) nsMemory::Clone("mozStorageStatementParams", 26);
    if (!*aClassName)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementParams::GetScriptableFlags(PRUint32 *aScriptableFlags)
{
    *aScriptableFlags =
        nsIXPCScriptable::WANT_SETPROPERTY |
        nsIXPCScriptable::WANT_NEWRESOLVE |
        nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementParams::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
mozStorageStatementParams::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

    if (JSVAL_IS_INT(id)) {
        int idx = JSVAL_TO_INT(id);

        PRBool res = JSValStorageStatementBinder(cx, mStatement, idx, *vp);
        NS_ENSURE_TRUE(res, NS_ERROR_UNEXPECTED);
    }
    else if (JSVAL_IS_STRING(id)) {
        JSString *str = JSVAL_TO_STRING(id);
        nsCAutoString name(":");
        name.Append(NS_ConvertUTF16toUTF8(::JS_GetStringChars(str),
                                          ::JS_GetStringLength(str)));

        
        PRUint32 index;
        nsresult rv = mStatement->GetParameterIndex(name, &index);
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool res = JSValStorageStatementBinder(cx, mStatement, index, *vp);
        NS_ENSURE_TRUE(res, NS_ERROR_UNEXPECTED);
    }
    else {
        return NS_ERROR_INVALID_ARG;
    }

    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementParams::PreCreate(nsISupports *nativeObj, JSContext * cx,
                       JSObject * globalObj, JSObject * *parentObj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::Create(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                  JSObject * obj, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                     JSObject * obj, PRUint32 enum_op, jsval * statep, jsid *idp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                   JSObject * obj, jsval id, PRUint32 flags, JSObject * *objp, PRBool *_retval)
{
    NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

    PRUint32 idx;

    if (JSVAL_IS_INT(id)) {
        idx = JSVAL_TO_INT(id);
    }
    else if (JSVAL_IS_STRING(id)) {
        JSString *str = JSVAL_TO_STRING(id);
        jschar *nameChars = JS_GetStringChars(str);
        size_t nameLength = JS_GetStringLength(str);

        nsCAutoString name(":");
        name.Append(NS_ConvertUTF16toUTF8(nameChars, nameLength));

        
        nsresult rv = mStatement->GetParameterIndex(name, &idx);
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool success = JS_DefineUCProperty(cx, obj, nameChars, nameLength,
                                             JSVAL_VOID, nsnull, nsnull, 0);
        NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
    }
    else {
        
        return NS_ERROR_UNEXPECTED;
    }

    
    if (idx >= mParamCount)
        return NS_ERROR_INVALID_ARG;

    *_retval = ::JS_DefineElement(cx, obj, idx, JSVAL_VOID, nsnull, nsnull, 0);
    if (*_retval)
        *objp = obj;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementParams::Convert(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                JSObject * obj, PRUint32 type, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::Finalize(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                 JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval id, PRUint32 mode, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                             JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                  JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval val, PRBool *bp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::Trace(nsIXPConnectWrappedNative *wrapper,
                                JSTracer *trc, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::Equality(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, JSObject *obj, jsval val,
                                    PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::OuterObject(nsIXPConnectWrappedNative *wrapper,
                                       JSContext *cx, JSObject *obj,
                                       JSObject **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::InnerObject(nsIXPConnectWrappedNative *wrapper,
                                       JSContext *cx, JSObject *obj,
                                       JSObject **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementParams::PostCreatePrototype(JSContext * cx, JSObject * proto)
{
    return NS_OK;
}
