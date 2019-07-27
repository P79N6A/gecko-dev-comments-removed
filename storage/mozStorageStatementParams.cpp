





#include "nsJSUtils.h"
#include "nsMemory.h"
#include "nsString.h"

#include "jsapi.h"

#include "mozStoragePrivateHelpers.h"
#include "mozStorageStatementParams.h"
#include "mozIStorageStatement.h"

namespace mozilla {
namespace storage {




StatementParams::StatementParams(mozIStorageStatement *aStatement) :
    mStatement(aStatement)
{
  NS_ASSERTION(mStatement != nullptr, "mStatement is null");
  (void)mStatement->GetParameterCount(&mParamCount);
}

NS_IMPL_ISUPPORTS(
  StatementParams,
  mozIStorageStatementParams,
  nsIXPCScriptable
)




#define XPC_MAP_CLASSNAME StatementParams
#define XPC_MAP_QUOTED_CLASSNAME "StatementParams"
#define XPC_MAP_WANT_SETPROPERTY
#define XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_WANT_RESOLVE
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h"

NS_IMETHODIMP
StatementParams::SetProperty(nsIXPConnectWrappedNative *aWrapper,
                             JSContext *aCtx,
                             JSObject *aScopeObj,
                             jsid aId,
                             JS::Value *_vp,
                             bool *_retval)
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
    nsAutoJSString autoStr;
    if (!autoStr.init(aCtx, str)) {
      return NS_ERROR_FAILURE;
    }

    NS_ConvertUTF16toUTF8 name(autoStr);

    
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
StatementParams::NewEnumerate(nsIXPConnectWrappedNative *aWrapper,
                              JSContext *aCtx,
                              JSObject *aScopeObj,
                              JS::AutoIdVector &aProperties,
                              bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  JS::RootedObject scope(aCtx, aScopeObj);

  if (!aProperties.reserve(mParamCount)) {
    *_retval = false;
    return NS_OK;
  }

  for (uint32_t i = 0; i < mParamCount; i++) {
    
    nsAutoCString name;
    nsresult rv = mStatement->GetParameterName(i, name);
    NS_ENSURE_SUCCESS(rv, rv);

    
    JS::RootedString jsname(aCtx, ::JS_NewStringCopyN(aCtx, &(name.get()[1]),
                                                      name.Length() - 1));
    NS_ENSURE_TRUE(jsname, NS_ERROR_OUT_OF_MEMORY);

    
    JS::Rooted<jsid> id(aCtx);
    if (!::JS_StringToId(aCtx, jsname, &id)) {
      *_retval = false;
      return NS_OK;
    }

    aProperties.infallibleAppend(id);
  }

  return NS_OK;
}

NS_IMETHODIMP
StatementParams::Resolve(nsIXPConnectWrappedNative *aWrapper,
                         JSContext *aCtx,
                         JSObject *aScopeObj,
                         jsid aId,
                         bool *resolvedp,
                         bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  
  
  

  JS::RootedObject scope(aCtx, aScopeObj);
  JS::RootedId id(aCtx, aId);
  bool resolved = false;
  bool ok = true;
  if (JSID_IS_INT(id)) {
    uint32_t idx = JSID_TO_INT(id);

    
    
    if (idx >= mParamCount)
      return NS_ERROR_INVALID_ARG;

    ok = ::JS_DefineElement(aCtx, scope, idx, JS::UndefinedHandleValue,
                            JSPROP_ENUMERATE | JSPROP_RESOLVING);
    resolved = true;
  }
  else if (JSID_IS_STRING(id)) {
    JSString *str = JSID_TO_STRING(id);
    nsAutoJSString autoStr;
    if (!autoStr.init(aCtx, str)) {
      return NS_ERROR_FAILURE;
    }

    
    
    NS_ConvertUTF16toUTF8 name(autoStr);
    uint32_t idx;
    nsresult rv = mStatement->GetParameterIndex(name, &idx);
    if (NS_SUCCEEDED(rv)) {
      ok = ::JS_DefinePropertyById(aCtx, scope, id, JS::UndefinedHandleValue,
                                   JSPROP_ENUMERATE | JSPROP_RESOLVING);
      resolved = true;
    }
  }

  *_retval = ok;
  *resolvedp = resolved && ok;
  return NS_OK;
}

} 
} 
