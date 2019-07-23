







































#include "sqlite3.h"

#include "jsapi.h"
#include "jsdate.h"

#include "nsPrintfCString.h"
#include "nsString.h"
#include "nsError.h"

#include "mozStoragePrivateHelpers.h"
#include "mozIStorageStatement.h"

namespace mozilla {
namespace storage {

nsresult
convertResultCode(int aSQLiteResultCode)
{
  switch (aSQLiteResultCode) {
    case SQLITE_OK:
    case SQLITE_ROW:
    case SQLITE_DONE:
      return NS_OK;
    case SQLITE_CORRUPT:
    case SQLITE_NOTADB:
      return NS_ERROR_FILE_CORRUPTED;
    case SQLITE_PERM:
    case SQLITE_CANTOPEN:
      return NS_ERROR_FILE_ACCESS_DENIED;
    case SQLITE_BUSY:
      return NS_ERROR_STORAGE_BUSY;
    case SQLITE_LOCKED:
      return NS_ERROR_FILE_IS_LOCKED;
    case SQLITE_READONLY:
      return NS_ERROR_FILE_READ_ONLY;
    case SQLITE_IOERR:
      return NS_ERROR_STORAGE_IOERR;
    case SQLITE_FULL:
    case SQLITE_TOOBIG:
      return NS_ERROR_FILE_NO_DEVICE_SPACE;
    case SQLITE_NOMEM:
      return NS_ERROR_OUT_OF_MEMORY;
    case SQLITE_MISUSE:
      return NS_ERROR_UNEXPECTED;
    case SQLITE_ABORT:
    case SQLITE_INTERRUPT:
      return NS_ERROR_ABORT;
  }

  
  return NS_ERROR_FAILURE;
}

void
checkAndLogStatementPerformance(sqlite3_stmt *aStatement)
{
  
  
  int count = ::sqlite3_stmt_status(aStatement, SQLITE_STMTSTATUS_SORT, 1);
  if (count <= 0)
    return;

  const char *sql = ::sqlite3_sql(aStatement);

  
  if (::strstr(sql, "/* do not warn (bug "))
    return;

  nsCAutoString message;
  message.AppendInt(count);
  if (count == 1)
    message.Append(" sort operation has ");
  else
    message.Append(" sort operations have ");
  message.Append("occurred for the SQL statement '");
  nsPrintfCString address("0x%p", aStatement);
  message.Append(address);
  message.Append("'.  See https://developer.mozilla.org/En/Storage/Warnings "
                 "details.");
  NS_WARNING(message.get());
}

bool
bindJSValue(JSContext *aCtx,
            mozIStorageStatement *aStatement,
            int aIdx,
            jsval aValue)
{
  if (JSVAL_IS_INT(aValue)) {
    int v = JSVAL_TO_INT(aValue);
    (void)aStatement->BindInt32Parameter(aIdx, v);
    return true;
  }

  if (JSVAL_IS_DOUBLE(aValue)) {
    double d = *JSVAL_TO_DOUBLE(aValue);
    (void)aStatement->BindDoubleParameter(aIdx, d);
    return true;
  }

  if (JSVAL_IS_STRING(aValue)) {
    JSString *str = JSVAL_TO_STRING(aValue);
    nsDependentString value(
      reinterpret_cast<PRUnichar *>(::JS_GetStringChars(str)),
      ::JS_GetStringLength(str)
    );
    (void)aStatement->BindStringParameter(aIdx, value);
    return true;
  }

  if (JSVAL_IS_BOOLEAN(aValue)) {
    (void)aStatement->BindInt32Parameter(aIdx, (aValue == JSVAL_TRUE) ? 1 : 0);
    return true;
  }

  if (JSVAL_IS_NULL(aValue)) {
    (void)aStatement->BindNullParameter(aIdx);
    return true;
  }

  if (JSVAL_IS_OBJECT(aValue)) {
    JSObject *obj = JSVAL_TO_OBJECT(aValue);
    
    if (!::js_DateIsValid(aCtx, obj))
      return false;
    
    double msecd = ::js_DateGetMsecSinceEpoch(aCtx, obj);
    msecd *= 1000.0;
    PRInt64 msec;
    LL_D2L(msec, msecd);
    
    (void)aStatement->BindInt64Parameter(aIdx, msec);
    return true;
  }

  return false;
}

} 
} 
