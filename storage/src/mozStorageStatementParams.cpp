







































#include "nsMemory.h"
#include "nsString.h"

#include "mozStoragePrivateHelpers.h"
#include "mozStorageStatementParams.h"
#include "mozIStorageStatement.h"

namespace mozilla {
namespace storage {




StatementParams::StatementParams(mozIStorageStatement *aStatement) :
    mStatement(aStatement)
{
  NS_ASSERTION(mStatement != nsnull, "mStatement is null");
  (void)mStatement->GetParameterCount(&mParamCount);
}

NS_IMPL_ISUPPORTS2(
  StatementParams,
  mozIStorageStatementParams,
  nsIXPCScriptable
)




#define XPC_MAP_CLASSNAME StatementParams
#define XPC_MAP_QUOTED_CLASSNAME "StatementParams"
#define XPC_MAP_WANT_SETPROPERTY
#define XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h"

NS_IMETHODIMP
StatementParams::SetProperty(nsIXPConnectWrappedNative *aWrapper,
                             JSContext *aCtx,
                             JSObject *aScopeObj,
                             jsid aId,
                             jsval *_vp,
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
    size_t length;
    const jschar *chars = JS_GetStringCharsAndLength(aCtx, str, &length);
    NS_ENSURE_TRUE(chars, NS_ERROR_UNEXPECTED);
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
StatementParams::NewEnumerate(nsIXPConnectWrappedNative *aWrapper,
                              JSContext *aCtx,
                              JSObject *aScopeObj,
                              PRUint32 aEnumOp,
                              jsval *_statep,
                              jsid *_idp,
                              bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  switch (aEnumOp) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
    {
      
      *_statep = JSVAL_ZERO;

      
      if (_idp)
        *_idp = INT_TO_JSID(mParamCount);

      break;
    }
    case JSENUMERATE_NEXT:
    {
      NS_ASSERTION(*_statep != JSVAL_NULL, "Internal state is null!");

      
      PRUint32 index = static_cast<PRUint32>(JSVAL_TO_INT(*_statep));
      if (index >= mParamCount) {
        *_statep = JSVAL_NULL;
        return NS_OK;
      }

      
      nsCAutoString name;
      nsresult rv = mStatement->GetParameterName(index, name);
      NS_ENSURE_SUCCESS(rv, rv);

      
      JSString *jsname = ::JS_NewStringCopyN(aCtx, &(name.get()[1]),
                                             name.Length() - 1);
      NS_ENSURE_TRUE(jsname, NS_ERROR_OUT_OF_MEMORY);

      
      if (!::JS_ValueToId(aCtx, STRING_TO_JSVAL(jsname), _idp)) {
        *_retval = false;
        return NS_OK;
      }

      
      *_statep = INT_TO_JSVAL(++index);

      break;
    }
    case JSENUMERATE_DESTROY:
    {
      
      *_statep = JSVAL_NULL;

      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
StatementParams::NewResolve(nsIXPConnectWrappedNative *aWrapper,
                            JSContext *aCtx,
                            JSObject *aScopeObj,
                            jsid aId,
                            PRUint32 aFlags,
                            JSObject **_objp,
                            bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  
  
  

  bool resolved = false;
  bool ok = true;
  if (JSID_IS_INT(aId)) {
    PRUint32 idx = JSID_TO_INT(aId);

    
    
    if (idx >= mParamCount)
      return NS_ERROR_INVALID_ARG;

    ok = ::JS_DefineElement(aCtx, aScopeObj, idx, JSVAL_VOID, nsnull,
                            nsnull, JSPROP_ENUMERATE);
    resolved = true;
  }
  else if (JSID_IS_STRING(aId)) {
    JSString *str = JSID_TO_STRING(aId);
    size_t nameLength;
    const jschar *nameChars = JS_GetStringCharsAndLength(aCtx, str, &nameLength);
    NS_ENSURE_TRUE(nameChars, NS_ERROR_UNEXPECTED);

    
    
    NS_ConvertUTF16toUTF8 name(nameChars, nameLength);
    PRUint32 idx;
    nsresult rv = mStatement->GetParameterIndex(name, &idx);
    if (NS_SUCCEEDED(rv)) {
      ok = ::JS_DefinePropertyById(aCtx, aScopeObj, aId, JSVAL_VOID, nsnull,
                                   nsnull, JSPROP_ENUMERATE);
      resolved = true;
    }
  }

  *_retval = ok;
  *_objp = resolved && ok ? aScopeObj : nsnull;
  return NS_OK;
}

} 
} 
