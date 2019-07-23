







































#include "nsMemory.h"
#include "nsString.h"

#include "mozStorageStatementRow.h"
#include "mozStorageStatement.h"

#include "jsapi.h"
#include "jsdate.h"

namespace mozilla {
namespace storage {




StatementRow::StatementRow(Statement *aStatement)
: mStatement(aStatement)
{
}

NS_IMPL_ISUPPORTS2(
  StatementRow,
  mozIStorageStatementRow,
  nsIXPCScriptable
)




#define XPC_MAP_CLASSNAME StatementRow
#define XPC_MAP_QUOTED_CLASSNAME "StatementRow"
#define XPC_MAP_WANT_GETPROPERTY
#define XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h"

NS_IMETHODIMP
StatementRow::GetProperty(nsIXPConnectWrappedNative *aWrapper,
                          JSContext *aCtx,
                          JSObject *aScopeObj,
                          jsval aId,
                          jsval *_vp,
                          PRBool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  if (JSVAL_IS_STRING(aId)) {
    nsDependentCString jsid(::JS_GetStringBytes(JSVAL_TO_STRING(aId)));

    PRUint32 idx;
    nsresult rv = mStatement->GetColumnIndex(jsid, &idx);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 type;
    rv = mStatement->GetTypeOfIndex(idx, &type);
    NS_ENSURE_SUCCESS(rv, rv);

    if (type == mozIStorageValueArray::VALUE_TYPE_INTEGER ||
        type == mozIStorageValueArray::VALUE_TYPE_FLOAT) {
      double dval;
      rv = mStatement->GetDouble(idx, &dval);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!::JS_NewNumberValue(aCtx, dval, _vp)) {
        *_retval = PR_FALSE;
        return NS_OK;
      }
    }
    else if (type == mozIStorageValueArray::VALUE_TYPE_TEXT) {
      PRUint32 bytes;
      const jschar *sval = reinterpret_cast<const jschar *>(
        mStatement->AsSharedWString(idx, &bytes)
      );
      JSString *str = ::JS_NewUCStringCopyN(aCtx, sval, bytes / 2);
      if (!str) {
        *_retval = PR_FALSE;
        return NS_OK;
      }
      *_vp = STRING_TO_JSVAL(str);
    }
    else if (type == mozIStorageValueArray::VALUE_TYPE_BLOB) {
      PRUint32 length;
      const PRUint8 *blob = mStatement->AsSharedBlob(idx, &length);
      JSObject *obj = ::JS_NewArrayObject(aCtx, length, nsnull);
      if (!obj) {
        *_retval = PR_FALSE;
        return NS_OK;
      }
      *_vp = OBJECT_TO_JSVAL(obj);

      
      for (PRUint32 i = 0; i < length; i++) {
        jsval val = INT_TO_JSVAL(blob[i]);
        if (!::JS_SetElement(aCtx, aScopeObj, i, &val)) {
          *_retval = PR_FALSE;
          return NS_OK;
        }
      }
    }
    else if (type == mozIStorageValueArray::VALUE_TYPE_NULL) {
      *_vp = JSVAL_NULL;
    }
    else {
      NS_ERROR("unknown column type returned, what's going on?");
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
StatementRow::NewResolve(nsIXPConnectWrappedNative *aWrapper,
                         JSContext *aCtx,
                         JSObject *aScopeObj,
                         jsval aId,
                         PRUint32 aFlags,
                         JSObject **_objp,
                         PRBool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  
  

  if (JSVAL_IS_STRING(aId)) {
    JSString *str = JSVAL_TO_STRING(aId);
    nsDependentCString name(::JS_GetStringBytes(str));

    PRUint32 idx;
    nsresult rv = mStatement->GetColumnIndex(name, &idx);
    if (NS_FAILED(rv)) {
      
      
      
      *_objp = NULL;
      return NS_OK;
    }

    *_retval = ::JS_DefineUCProperty(aCtx, aScopeObj, ::JS_GetStringChars(str),
                                     ::JS_GetStringLength(str),
                                     JSVAL_VOID,
                                     nsnull, nsnull, 0);
    *_objp = aScopeObj;
    return NS_OK;
  }

  return NS_OK;
}

} 
} 
