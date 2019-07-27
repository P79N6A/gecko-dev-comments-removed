





#include "nsMemory.h"
#include "nsString.h"

#include "mozStorageStatementRow.h"
#include "mozStorageStatement.h"

#include "jsapi.h"

namespace mozilla {
namespace storage {




StatementRow::StatementRow(Statement *aStatement)
: mStatement(aStatement)
{
}

NS_IMPL_ISUPPORTS(
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
                          jsid aId,
                          jsval *_vp,
                          bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  JS::RootedObject scope(aCtx, aScopeObj);
  if (JSID_IS_STRING(aId)) {
    ::JSAutoByteString idBytes(aCtx, JSID_TO_STRING(aId));
    NS_ENSURE_TRUE(!!idBytes, NS_ERROR_OUT_OF_MEMORY);
    nsDependentCString jsid(idBytes.ptr());

    uint32_t idx;
    nsresult rv = mStatement->GetColumnIndex(jsid, &idx);
    NS_ENSURE_SUCCESS(rv, rv);
    int32_t type;
    rv = mStatement->GetTypeOfIndex(idx, &type);
    NS_ENSURE_SUCCESS(rv, rv);

    if (type == mozIStorageValueArray::VALUE_TYPE_INTEGER ||
        type == mozIStorageValueArray::VALUE_TYPE_FLOAT) {
      double dval;
      rv = mStatement->GetDouble(idx, &dval);
      NS_ENSURE_SUCCESS(rv, rv);
      *_vp = ::JS_NumberValue(dval);
    }
    else if (type == mozIStorageValueArray::VALUE_TYPE_TEXT) {
      uint32_t bytes;
      const char16_t *sval = reinterpret_cast<const char16_t *>(
        static_cast<mozIStorageStatement *>(mStatement)->
          AsSharedWString(idx, &bytes)
      );
      JSString *str = ::JS_NewUCStringCopyN(aCtx, sval, bytes / 2);
      if (!str) {
        *_retval = false;
        return NS_OK;
      }
      *_vp = STRING_TO_JSVAL(str);
    }
    else if (type == mozIStorageValueArray::VALUE_TYPE_BLOB) {
      uint32_t length;
      const uint8_t *blob = static_cast<mozIStorageStatement *>(mStatement)->
        AsSharedBlob(idx, &length);
      JSObject *obj = ::JS_NewArrayObject(aCtx, length);
      if (!obj) {
        *_retval = false;
        return NS_OK;
      }
      *_vp = OBJECT_TO_JSVAL(obj);

      
      for (uint32_t i = 0; i < length; i++) {
        if (!::JS_SetElement(aCtx, scope, i, blob[i])) {
          *_retval = false;
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
                         jsid aId,
                         JSObject **_objp,
                         bool *_retval)
{
  JS::Rooted<JSObject*> scopeObj(aCtx, aScopeObj);

  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  
  

  if (JSID_IS_STRING(aId)) {
    ::JSAutoByteString idBytes(aCtx, JSID_TO_STRING(aId));
    NS_ENSURE_TRUE(!!idBytes, NS_ERROR_OUT_OF_MEMORY);
    nsDependentCString name(idBytes.ptr());

    uint32_t idx;
    nsresult rv = mStatement->GetColumnIndex(name, &idx);
    if (NS_FAILED(rv)) {
      
      
      
      *_objp = nullptr;
      return NS_OK;
    }

    JS::Rooted<jsid> id(aCtx, aId);
    *_retval = ::JS_DefinePropertyById(aCtx, scopeObj, id, JS::UndefinedHandleValue, 0);
    *_objp = scopeObj;
    return NS_OK;
  }

  return NS_OK;
}

} 
} 
