







































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
                             jsval aId,
                             jsval *_vp,
                             PRBool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  if (JSVAL_IS_INT(aId)) {
    int idx = JSVAL_TO_INT(aId);

    PRBool res = bindJSValue(aCtx, mStatement, idx, *_vp);
    NS_ENSURE_TRUE(res, NS_ERROR_UNEXPECTED);
  }
  else if (JSVAL_IS_STRING(aId)) {
    JSString *str = JSVAL_TO_STRING(aId);
    NS_ConvertUTF16toUTF8 name(reinterpret_cast<const PRUnichar *>
                                   (::JS_GetStringChars(str)),
                               ::JS_GetStringLength(str));

    
    PRUint32 index;
    nsresult rv = mStatement->GetParameterIndex(name, &index);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool res = bindJSValue(aCtx, mStatement, index, *_vp);
    NS_ENSURE_TRUE(res, NS_ERROR_UNEXPECTED);
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
StatementParams::NewEnumerate(nsIXPConnectWrappedNative *aWrapper,
                              JSContext *aCtx,
                              JSObject *aScopeObj,
                              PRUint32 aEnumOp,
                              jsval *_statep,
                              jsid *_idp,
                              PRBool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  switch (aEnumOp) {
    case JSENUMERATE_INIT:
    {
      
      *_statep = JSVAL_ZERO;

      
      if (_idp)
        *_idp = INT_TO_JSVAL(mParamCount);

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
        *_retval = PR_FALSE;
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
                            jsval aId,
                            PRUint32 aFlags,
                            JSObject **_objp,
                            PRBool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  
  
  

  PRUint32 idx;

  if (JSVAL_IS_INT(aId)) {
    idx = JSVAL_TO_INT(aId);

    
    
    if (idx >= mParamCount)
      return NS_ERROR_INVALID_ARG;
  }
  else if (JSVAL_IS_STRING(aId)) {
    JSString *str = JSVAL_TO_STRING(aId);
    jschar *nameChars = ::JS_GetStringChars(str);
    size_t nameLength = ::JS_GetStringLength(str);

    
    
    NS_ConvertUTF16toUTF8 name(reinterpret_cast<const PRUnichar *>(nameChars),
                               nameLength);
    nsresult rv = mStatement->GetParameterIndex(name, &idx);
    if (NS_FAILED(rv)) {
      *_objp = NULL;
      return NS_OK;
    }

    *_retval = ::JS_DefineUCProperty(aCtx, aScopeObj, nameChars, nameLength,
                                     JSVAL_VOID, nsnull, nsnull, 0);
    NS_ENSURE_TRUE(*_retval, NS_OK);
  }
  else {
    
    return NS_OK;
  }

  *_retval = ::JS_DefineElement(aCtx, aScopeObj, idx, JSVAL_VOID, nsnull,
                                nsnull, 0);
  if (*_retval)
    *_objp = aScopeObj;
  return NS_OK;
}

} 
} 
