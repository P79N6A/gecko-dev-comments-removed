







































#include "nsMemory.h"
#include "nsString.h"

#include "mozStorageStatementRow.h"

#include "jsapi.h"
#include "jsdate.h"

#include "sqlite3.h"







NS_IMPL_ISUPPORTS2(mozStorageStatementRow, mozIStorageStatementRow, nsIXPCScriptable)

mozStorageStatementRow::mozStorageStatementRow(mozStorageStatement *aStatement)
    : mStatement(aStatement)
{
}






NS_IMETHODIMP
mozStorageStatementRow::GetClassName(char * *aClassName)
{
    NS_ENSURE_ARG_POINTER(aClassName);
    *aClassName = (char *) nsMemory::Clone("mozStorageStatementRow", 23);
    if (!*aClassName)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementRow::GetScriptableFlags(PRUint32 *aScriptableFlags)
{
    *aScriptableFlags =
        nsIXPCScriptable::WANT_GETPROPERTY |
        nsIXPCScriptable::WANT_NEWRESOLVE |
        nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementRow::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

    if (JSVAL_IS_STRING(id)) {
        nsDependentCString jsid(::JS_GetStringBytes(JSVAL_TO_STRING(id)));

        PRUint32 idx;
        nsresult rv = mStatement->GetColumnIndex(jsid, &idx);
        NS_ENSURE_SUCCESS(rv, rv);
        int ctype = sqlite3_column_type(NativeStatement(), idx);

        if (ctype == SQLITE_INTEGER || ctype == SQLITE_FLOAT) {
            double dval = sqlite3_column_double(NativeStatement(), idx);
            if (!JS_NewNumberValue(cx, dval, vp)) {
                *_retval = PR_FALSE;
                return NS_OK;
            }
        } else if (ctype == SQLITE_TEXT) {
            JSString *str = JS_NewUCStringCopyN(cx,
                                                (jschar*) sqlite3_column_text16(NativeStatement(), idx),
                                                sqlite3_column_bytes16(NativeStatement(), idx)/2);
            if (!str) {
                *_retval = PR_FALSE;
                return NS_OK;
            }
            *vp = STRING_TO_JSVAL(str);
        } else if (ctype == SQLITE_BLOB) {
            JSString *str = JS_NewStringCopyN(cx,
                                              (char*) sqlite3_column_blob(NativeStatement(), idx),
                                              sqlite3_column_bytes(NativeStatement(), idx));
            if (!str) {
                *_retval = PR_FALSE;
                return NS_OK;
            }
        } else if (ctype == SQLITE_NULL) {
            *vp = JSVAL_NULL;
        } else {
            NS_ERROR("sqlite3_column_type returned unknown column type, what's going on?");
        }
    }

    return NS_OK;
}



NS_IMETHODIMP
mozStorageStatementRow::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::PreCreate(nsISupports *nativeObj, JSContext * cx,
                       JSObject * globalObj, JSObject * *parentObj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::Create(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                  JSObject * obj, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                     JSObject * obj, PRUint32 enum_op, jsval * statep, jsid *idp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                   JSObject * obj, jsval id, PRUint32 flags, JSObject * *objp, PRBool *_retval)
{
    NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

    if (JSVAL_IS_STRING(id)) {
        JSString *str = JSVAL_TO_STRING(id);
        nsDependentCString name(::JS_GetStringBytes(str));

        PRUint32 idx;
        nsresult rv = mStatement->GetColumnIndex(name, &idx);
        NS_ENSURE_SUCCESS(rv, rv);

        *_retval = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                         ::JS_GetStringLength(str),
                                         JSVAL_VOID,
                                         nsnull, nsnull, 0);
        *objp = obj;
        return *_retval ? NS_OK : NS_ERROR_FAILURE;
    }

    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementRow::Convert(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                JSObject * obj, PRUint32 type, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::Finalize(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                 JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval id, PRUint32 mode, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                             JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                  JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                    JSObject * obj, jsval val, PRBool *bp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::Trace(nsIXPConnectWrappedNative *wrapper,
                              JSTracer * trc, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::Equality(nsIXPConnectWrappedNative *wrapper,
                                 JSContext *cx, JSObject *obj, jsval val,
                                 PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::OuterObject(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, JSObject *obj,
                                    JSObject **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::InnerObject(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, JSObject *obj,
                                    JSObject **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementRow::PostCreatePrototype(JSContext * cx, JSObject * proto)
{
    return NS_OK;
}
