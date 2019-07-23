







































#include "sqlite3.h"

#include "nsString.h"
#include "nsError.h"

#include "mozStoragePrivateHelpers.h"

nsresult
ConvertResultCode(int aSQLiteResultCode)
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
CheckAndLogStatementPerformance(sqlite3_stmt *aStatement)
{
  
  
  int count = sqlite3_stmt_status(aStatement, SQLITE_STMTSTATUS_SORT, 1);
  if (count <= 0)
    return;

  nsCAutoString message;
  message.AppendInt(count);
  if (count == 1)
    message.Append(" sort operation has ");
  else
    message.Append(" sort operations have ");
  message.Append("occurred for the SQL statement '");
  message.Append(sqlite3_sql(aStatement));
  message.Append("'.  This may indicate an opportunity to improve performance "
                 "through the careful use of indexes.");
  NS_WARNING(message.get());
}
