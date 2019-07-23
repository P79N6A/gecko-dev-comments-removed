















#include "sqliteInt.h"
#include "os.h"




int sqlite3_clear_bindings(sqlite3_stmt *pStmt){
  int i;
  int rc = SQLITE_OK;
  for(i=1; rc==SQLITE_OK && i<=sqlite3_bind_parameter_count(pStmt); i++){
    rc = sqlite3_bind_null(pStmt, i);
  }
  return rc;
}




int sqlite3_sleep(int ms){
  return sqlite3OsSleep(ms);
}
