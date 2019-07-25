







































#include "nsMemory.h"
#include "nsString.h"
#include "nsCOMPtr.h"

#include "mozStoragePrivateHelpers.h"
#include "mozStorageAsyncStatement.h"
#include "mozStorageAsyncStatementParams.h"
#include "mozIStorageStatement.h"

namespace mozilla {
namespace storage {




AsyncStatementParams::AsyncStatementParams(AsyncStatement *aStatement)
: mStatement(aStatement)
{
  NS_ASSERTION(mStatement != nsnull, "mStatement is null");
}

NS_IMPL_ISUPPORTS2(
  AsyncStatementParams
, mozIStorageStatementParams
, nsIXPCScriptable
)




#define XPC_MAP_CLASSNAME AsyncStatementParams
#define XPC_MAP_QUOTED_CLASSNAME "AsyncStatementParams"
#define XPC_MAP_WANT_SETPROPERTY
#define XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h"

NS_IMETHODIMP
AsyncStatementParams::SetProperty(
  nsIXPConnectWrappedNative *aWrapper,
  JSContext *aCtx,
  JSObject *aScopeObj,
  jsid aId,
  jsval *_vp,
  bool *_retval
)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  if (JSID_IS_INT(aId)) {
    int idx = JSID_TO_INT(aId);

    nsCOMPtr<nsIVariant> variant(convertJSValToVariant(aCtx, *_vp));
    NS_ENSURE_TRUE(variant, NS_ERROR_UNEXPECTED);
    nsresult rv = mStatement->BindByIndex(idx, variant);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (JSID_IS_STRING(aId)) {
    JSString *str = JSID_TO_STRING(aId);
    size_t length;
    const jschar *chars = JS_GetInternedStringCharsAndLength(str, &length);
    NS_ConvertUTF16toUTF8 name(chars, length);

    nsCOMPtr<nsIVariant> variant(convertJSValToVariant(aCtx, *_vp));
    NS_ENSURE_TRUE(variant, NS_ERROR_UNEXPECTED);
    nsresult rv = mStatement->BindByName(name, variant);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  *_retval = true;
  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementParams::NewResolve(
  nsIXPConnectWrappedNative *aWrapper,
  JSContext *aCtx,
  JSObject *aScopeObj,
  jsid aId,
  PRUint32 aFlags,
  JSObject **_objp,
  bool *_retval
)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  
  

  bool resolved = false;
  bool ok = true;
  if (JSID_IS_INT(aId)) {
    PRUint32 idx = JSID_TO_INT(aId);
    
    
    ok = ::JS_DefineElement(aCtx, aScopeObj, idx, JSVAL_VOID, nsnull,
                            nsnull, 0);
    resolved = true;
  }
  else if (JSID_IS_STRING(aId)) {
    
    
    
    ok = ::JS_DefinePropertyById(aCtx, aScopeObj, aId, JSVAL_VOID, nsnull,
                                 nsnull, 0);
    resolved = true;
  }

  *_retval = ok;
  *_objp = resolved && ok ? aScopeObj : nsnull;
  return NS_OK;
}

} 
} 
