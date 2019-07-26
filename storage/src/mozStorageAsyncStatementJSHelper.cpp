





#include "nsIXPConnect.h"
#include "mozStorageAsyncStatement.h"
#include "mozStorageService.h"

#include "nsMemory.h"
#include "nsString.h"
#include "nsServiceManagerUtils.h"

#include "mozStorageAsyncStatementJSHelper.h"

#include "mozStorageAsyncStatementParams.h"

#include "jsapi.h"

namespace mozilla {
namespace storage {




nsresult
AsyncStatementJSHelper::getParams(AsyncStatement *aStatement,
                                  JSContext *aCtx,
                                  JSObject *aScopeObj,
                                  jsval *_params)
{
  nsresult rv;

#ifdef DEBUG
  int32_t state;
  (void)aStatement->GetState(&state);
  NS_ASSERTION(state == mozIStorageAsyncStatement::MOZ_STORAGE_STATEMENT_READY,
               "Invalid state to get the params object - all calls will fail!");
#endif

  if (!aStatement->mStatementParamsHolder) {
    nsCOMPtr<mozIStorageStatementParams> params =
      new AsyncStatementParams(aStatement);
    NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

    JS::RootedObject scope(aCtx, aScopeObj);
    nsCOMPtr<nsIXPConnect> xpc(Service::getXPConnect());
    rv = xpc->WrapNative(
      aCtx,
      ::JS_GetGlobalForObject(aCtx, scope),
      params,
      NS_GET_IID(mozIStorageStatementParams),
      getter_AddRefs(aStatement->mStatementParamsHolder)
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  JS::Rooted<JSObject*> obj(aCtx);
  obj = aStatement->mStatementParamsHolder->GetJSObject();
  NS_ENSURE_STATE(obj);

  *_params = OBJECT_TO_JSVAL(obj);
  return NS_OK;
}

NS_IMETHODIMP_(MozExternalRefCountType) AsyncStatementJSHelper::AddRef() { return 2; }
NS_IMETHODIMP_(MozExternalRefCountType) AsyncStatementJSHelper::Release() { return 1; }
NS_INTERFACE_MAP_BEGIN(AsyncStatementJSHelper)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




#define XPC_MAP_CLASSNAME AsyncStatementJSHelper
#define XPC_MAP_QUOTED_CLASSNAME "AsyncStatementJSHelper"
#define XPC_MAP_WANT_GETPROPERTY
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h"

NS_IMETHODIMP
AsyncStatementJSHelper::GetProperty(nsIXPConnectWrappedNative *aWrapper,
                                    JSContext *aCtx,
                                    JSObject *aScopeObj,
                                    jsid aId,
                                    jsval *_result,
                                    bool *_retval)
{
  if (!JSID_IS_STRING(aId))
    return NS_OK;

  
  JS::RootedObject scope(aCtx, aScopeObj);
  JS::RootedId id(aCtx, aId);
  mozIStorageAsyncStatement *iAsyncStmt =
    static_cast<mozIStorageAsyncStatement *>(aWrapper->Native());
  AsyncStatement *stmt = static_cast<AsyncStatement *>(iAsyncStmt);

#ifdef DEBUG
  {
    nsISupports *supp = aWrapper->Native();
    nsCOMPtr<mozIStorageAsyncStatement> isStatement(do_QueryInterface(supp));
    NS_ASSERTION(isStatement, "How is this not an async statement?!");
  }
#endif

  if (::JS_FlatStringEqualsAscii(JSID_TO_FLAT_STRING(id), "params"))
    return getParams(stmt, aCtx, scope, _result);

  return NS_OK;
}

} 
} 
