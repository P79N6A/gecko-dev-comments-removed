







































#include "sqlite3.h"

#include "jsapi.h"
#include "jsdate.h"

#include "nsPrintfCString.h"
#include "nsString.h"
#include "nsError.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "nsThreadUtils.h"
#include "nsJSUtils.h"

#include "Variant.h"
#include "mozStoragePrivateHelpers.h"
#include "mozIStorageStatement.h"
#include "mozIStorageCompletionCallback.h"
#include "mozIStorageBindingParams.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gStorageLog;
#endif

namespace mozilla {
namespace storage {

nsresult
convertResultCode(int aSQLiteResultCode)
{
  
  int rc = aSQLiteResultCode & 0xFF;

  switch (rc) {
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
    case SQLITE_CONSTRAINT:
      return NS_ERROR_STORAGE_CONSTRAINT;
  }

  
#ifdef DEBUG
  nsCAutoString message;
  message.AppendLiteral("SQLite returned error code ");
  message.AppendInt(rc);
  message.AppendLiteral(" , Storage will convert it to NS_ERROR_FAILURE");
  NS_WARNING(message.get());
#endif
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

nsIVariant *
convertJSValToVariant(
  JSContext *aCtx,
  jsval aValue)
{
  if (JSVAL_IS_INT(aValue))
    return new IntegerVariant(JSVAL_TO_INT(aValue));

  if (JSVAL_IS_DOUBLE(aValue))
    return new FloatVariant(JSVAL_TO_DOUBLE(aValue));

  if (JSVAL_IS_STRING(aValue)) {
    JSString *str = JSVAL_TO_STRING(aValue);
    nsDependentJSString value;
    if (!value.init(aCtx, str))
        return nsnull;
    return new TextVariant(value);
  }

  if (JSVAL_IS_BOOLEAN(aValue))
    return new IntegerVariant((aValue == JSVAL_TRUE) ? 1 : 0);

  if (JSVAL_IS_NULL(aValue))
    return new NullVariant();

  if (JSVAL_IS_OBJECT(aValue)) {
    JSObject *obj = JSVAL_TO_OBJECT(aValue);
    
    if (!::js_DateIsValid(aCtx, obj))
      return nsnull;

    double msecd = ::js_DateGetMsecSinceEpoch(aCtx, obj);
    msecd *= 1000.0;
    PRInt64 msec;
    LL_D2L(msec, msecd);

    return new IntegerVariant(msec);
  }

  return nsnull;
}

namespace {
class CallbackEvent : public nsRunnable
{
public:
  CallbackEvent(mozIStorageCompletionCallback *aCallback)
  : mCallback(aCallback)
  {
  }

  NS_IMETHOD Run()
  {
    (void)mCallback->Complete();
    return NS_OK;
  }
private:
  nsCOMPtr<mozIStorageCompletionCallback> mCallback;
};
} 
already_AddRefed<nsIRunnable>
newCompletionEvent(mozIStorageCompletionCallback *aCallback)
{
  NS_ASSERTION(aCallback, "Passing a null callback is a no-no!");
  nsCOMPtr<nsIRunnable> event = new CallbackEvent(aCallback);
  return event.forget();
}



} 
} 
