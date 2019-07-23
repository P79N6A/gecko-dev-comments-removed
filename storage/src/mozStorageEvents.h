






































#ifndef _mozStorageEvents_h_
#define _mozStorageEvents_h_

#include "nscore.h"
#include "mozStorageBackground.h"
class mozStorageStatement;
class mozIStorageStatementCallback;
class mozIStoragePendingStatement;












nsresult NS_executeAsync(
  mozStorageStatement *aStatement,
  mozIStorageStatementCallback *aCallback,
  mozIStoragePendingStatement **_stmt
);

#endif 
