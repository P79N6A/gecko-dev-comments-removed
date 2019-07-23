






































#ifndef _mozStorageEvents_h_
#define _mozStorageEvents_h_

#include "nscore.h"
#include "nsTArray.h"
struct sqlite3_stmt;
class mozIStorageStatementCallback;
class mozIStoragePendingStatement;

namespace mozilla {
namespace storage {
class Connection;
} 
} 














nsresult NS_executeAsync(
  nsTArray<sqlite3_stmt *> &aStatements,
  mozilla::storage::Connection *aConnection,
  mozIStorageStatementCallback *aCallback,
  mozIStoragePendingStatement **_stmt
);

#endif 
