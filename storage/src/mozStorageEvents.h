






































#ifndef _mozStorageEvents_h_
#define _mozStorageEvents_h_

#include "nscore.h"
#include "mozStorageBackground.h"
struct sqlite3_stmt;
class mozIStorageStatementCallback;
class mozIStoragePendingStatement;












nsresult NS_executeAsync(
  sqlite3_stmt *aStatement,
  mozIStorageStatementCallback *aCallback,
  mozIStoragePendingStatement **_stmt
);

#endif 
