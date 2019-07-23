






































#ifndef _mozStorageEvents_h_
#define _mozStorageEvents_h_

#include "nscore.h"
#include "nsTArray.h"
struct sqlite3_stmt;
class mozStorageConnection;
class mozIStorageStatementCallback;
class mozIStoragePendingStatement;














nsresult NS_executeAsync(
  nsTArray<sqlite3_stmt *> &aStatements,
  mozStorageConnection *aConnection,
  mozIStorageStatementCallback *aCallback,
  mozIStoragePendingStatement **_stmt
);

#endif 
