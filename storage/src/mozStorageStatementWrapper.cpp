







































#include "nsMemory.h"
#include "nsString.h"

#include "mozStorageStatementWrapper.h"
#include "mozStorageStatementParams.h"
#include "mozStorageStatementRow.h"

#include "sqlite3.h"

using namespace mozilla::storage;







NS_IMPL_ISUPPORTS2(mozStorageStatementWrapper, mozIStorageStatementWrapper, nsIXPCScriptable)

mozStorageStatementWrapper::mozStorageStatementWrapper()
    : mStatement(nsnull)
{
}

mozStorageStatementWrapper::~mozStorageStatementWrapper()
{
    mStatement = nsnull;
}

NS_IMETHODIMP
mozStorageStatementWrapper::Initialize(mozIStorageStatement *aStatement)
{
    NS_ASSERTION(mStatement == nsnull, "mozStorageStatementWrapper is already initialized");
    NS_ENSURE_ARG_POINTER(aStatement);

    mStatement = static_cast<mozStorageStatement *>(aStatement);

    
    mStatement->GetParameterCount(&mParamCount);
    mStatement->GetColumnCount(&mResultColumnCount);

    for (unsigned int i = 0; i < mResultColumnCount; i++) {
        const void *name = sqlite3_column_name16 (NativeStatement(), i);
        mColumnNames.AppendElement(nsDependentString(static_cast<const PRUnichar*>(name)));
    }

    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementWrapper::GetStatement(mozIStorageStatement **aStatement)
{
    NS_IF_ADDREF(*aStatement = mStatement);
    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementWrapper::Reset()
{
    if (!mStatement)
        return NS_ERROR_FAILURE;

    return mStatement->Reset();
}

NS_IMETHODIMP
mozStorageStatementWrapper::Step(PRBool *_retval)
{
    if (!mStatement)
        return NS_ERROR_FAILURE;

    PRBool hasMore = PR_FALSE;
    nsresult rv = mStatement->ExecuteStep(&hasMore);
    if (NS_SUCCEEDED(rv) && !hasMore) {
        *_retval = PR_FALSE;
        mStatement->Reset();
        return NS_OK;
    }

    *_retval = hasMore;
    return rv;
}

NS_IMETHODIMP
mozStorageStatementWrapper::Execute()
{
    if (!mStatement)
        return NS_ERROR_FAILURE;

    return mStatement->Execute();
}

NS_IMETHODIMP
mozStorageStatementWrapper::GetRow(mozIStorageStatementRow **aRow)
{
    NS_ENSURE_ARG_POINTER(aRow);

    if (!mStatement)
        return NS_ERROR_FAILURE;

    PRInt32 state;
    mStatement->GetState(&state);
    if (state != mozIStorageStatement::MOZ_STORAGE_STATEMENT_EXECUTING)
        return NS_ERROR_FAILURE;

    if (!mStatementRow) {
        StatementRow *row = new StatementRow(mStatement);
        if (!row)
            return NS_ERROR_OUT_OF_MEMORY;
        mStatementRow = row;
    }

    NS_ADDREF(*aRow = mStatementRow);
    return NS_OK;
}

NS_IMETHODIMP
mozStorageStatementWrapper::GetParams(mozIStorageStatementParams **aParams)
{
    NS_ENSURE_ARG_POINTER(aParams);

    if (!mStatementParams) {
        StatementParams *params = new StatementParams(mStatement);
        if (!params)
            return NS_ERROR_OUT_OF_MEMORY;
        mStatementParams = params;
    }

    NS_ADDREF(*aParams = mStatementParams);
    return NS_OK;
}




NS_IMETHODIMP
mozStorageStatementWrapper::GetClassName(char * *aClassName)
{
    NS_ENSURE_ARG_POINTER(aClassName);
    *aClassName = (char *) nsMemory::Clone("mozStorageStatementWrapper", 27);
    if (!*aClassName)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementWrapper::GetScriptableFlags(PRUint32 *aScriptableFlags)
{
    *aScriptableFlags =
        nsIXPCScriptable::WANT_CALL |
        nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY |
        nsIXPCScriptable::WANT_NEWRESOLVE |
        nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                          JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, PRBool *_retval)
{
    if (!mStatement) {
        *_retval = PR_TRUE;
        return NS_ERROR_FAILURE;
    }

    if (argc != mParamCount) {
        *_retval = PR_FALSE;
        return NS_ERROR_FAILURE;
    }

    
    mStatement->Reset();

    
    for (int i = 0; i < (int) argc; i++) {
        if (!JSValStorageStatementBinder(cx, mStatement, i, argv[i])) {
            *_retval = PR_FALSE;
            return NS_ERROR_INVALID_ARG;
        }
    }

    
    if (mResultColumnCount == 0)
        mStatement->Execute();

    *vp = JSVAL_TRUE;
    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementWrapper::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                 JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_OK;
}



NS_IMETHODIMP
mozStorageStatementWrapper::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementWrapper::PreCreate(nsISupports *nativeObj, JSContext * cx,
                       JSObject * globalObj, JSObject * *parentObj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Create(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                 JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                 JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                               JSObject * obj, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                  JSObject * obj, PRUint32 enum_op, jsval * statep, jsid *idp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                JSObject * obj, jsval id, PRUint32 flags, JSObject * *objp, PRBool *_retval)
{
    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Convert(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                JSObject * obj, PRUint32 type, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Finalize(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                              JSObject * obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                 JSObject * obj, jsval id, PRUint32 mode, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                               JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                                 JSObject * obj, jsval val, PRBool *bp, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Trace(nsIXPConnectWrappedNative *wrapper,
                                  JSTracer *trc, JSObject *obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::Equality(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, JSObject *obj, jsval val,
                                    PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::OuterObject(nsIXPConnectWrappedNative *wrapper,
                                        JSContext *cx, JSObject *obj,
                                        JSObject **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::InnerObject(nsIXPConnectWrappedNative *wrapper,
                                        JSContext *cx, JSObject *obj,
                                        JSObject **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
mozStorageStatementWrapper::PostCreatePrototype(JSContext * cx,
                                                JSObject * proto)
{
    return NS_OK;
}
