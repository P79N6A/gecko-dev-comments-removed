






































#ifndef _sharedCacheUnlockNotify_h_
#define _sharedCacheUnlockNotify_h_

#include "sqlite3.h"

namespace mozilla {
namespace storage {

extern int moz_sqlite3_step(sqlite3_stmt* pStmt);

extern int moz_sqlite3_prepare_v2(sqlite3* db,
                                  const char* zSql,
                                  int nByte,
                                  sqlite3_stmt** ppStmt,
                                  const char** pzTail);

} 
} 

#endif 
