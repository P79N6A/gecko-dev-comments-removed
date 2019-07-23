














#define _SQLITE_OS_C_ 1
#include "sqliteInt.h"
#include "os.h"







int sqlite3OsClose(OsFile **pId){
  OsFile *id;
  if( pId!=0 && (id = *pId)!=0 ){
    return id->pMethod->xClose(pId);
  }else{
    return SQLITE_OK;
  }
}
int sqlite3OsOpenDirectory(OsFile *id, const char *zName){
  return id->pMethod->xOpenDirectory(id, zName);
}
int sqlite3OsRead(OsFile *id, void *pBuf, int amt){
  return id->pMethod->xRead(id, pBuf, amt);
}
int sqlite3OsWrite(OsFile *id, const void *pBuf, int amt){
  return id->pMethod->xWrite(id, pBuf, amt);
}
int sqlite3OsSeek(OsFile *id, i64 offset){
  return id->pMethod->xSeek(id, offset);
}
int sqlite3OsTruncate(OsFile *id, i64 size){
  return id->pMethod->xTruncate(id, size);
}
int sqlite3OsSync(OsFile *id, int fullsync){
  return id->pMethod->xSync(id, fullsync);
}
void sqlite3OsSetFullSync(OsFile *id, int value){
  id->pMethod->xSetFullSync(id, value);
}
#if defined(SQLITE_TEST) || defined(SQLITE_DEBUG)



int sqlite3OsFileHandle(OsFile *id){
  return id->pMethod->xFileHandle(id);
}
#endif
int sqlite3OsFileSize(OsFile *id, i64 *pSize){
  return id->pMethod->xFileSize(id, pSize);
}
int sqlite3OsLock(OsFile *id, int lockType){
  return id->pMethod->xLock(id, lockType);
}
int sqlite3OsUnlock(OsFile *id, int lockType){
  return id->pMethod->xUnlock(id, lockType);
}
int sqlite3OsLockState(OsFile *id){
  return id->pMethod->xLockState(id);
}
int sqlite3OsCheckReservedLock(OsFile *id){
  return id->pMethod->xCheckReservedLock(id);
}

#ifdef SQLITE_ENABLE_REDEF_IO









struct sqlite3OsVtbl *sqlite3_os_switch(void){
  return &sqlite3Os;
}
#endif
