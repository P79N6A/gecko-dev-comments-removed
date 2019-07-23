

















#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>





const int sqlite3one = 1;




const char sqlite3_version[] = SQLITE_VERSION;
const char *sqlite3_libversion(void){ return sqlite3_version; }
int sqlite3_libversion_number(void){ return SQLITE_VERSION_NUMBER; }





static int binCollFunc(
  void *NotUsed,
  int nKey1, const void *pKey1,
  int nKey2, const void *pKey2
){
  int rc, n;
  n = nKey1<nKey2 ? nKey1 : nKey2;
  rc = memcmp(pKey1, pKey2, n);
  if( rc==0 ){
    rc = nKey1 - nKey2;
  }
  return rc;
}










static int nocaseCollatingFunc(
  void *NotUsed,
  int nKey1, const void *pKey1,
  int nKey2, const void *pKey2
){
  int r = sqlite3StrNICmp(
      (const char *)pKey1, (const char *)pKey2, (nKey1<nKey2)?nKey1:nKey2);
  if( 0==r ){
    r = nKey1-nKey2;
  }
  return r;
}




sqlite_int64 sqlite3_last_insert_rowid(sqlite3 *db){
  return db->lastRowid;
}




int sqlite3_changes(sqlite3 *db){
  return db->nChange;
}




int sqlite3_total_changes(sqlite3 *db){
  return db->nTotalChange;
}




int sqlite3_close(sqlite3 *db){
  HashElem *i;
  int j;

  if( !db ){
    return SQLITE_OK;
  }
  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }

#ifdef SQLITE_SSE
  {
    extern void sqlite3SseCleanup(sqlite3*);
    sqlite3SseCleanup(db);
  }
#endif 

  
  if( db->pVdbe ){
    sqlite3Error(db, SQLITE_BUSY, 
        "Unable to close due to unfinalised statements");
    return SQLITE_BUSY;
  }
  assert( !sqlite3SafetyCheck(db) );

  




  if( db->magic!=SQLITE_MAGIC_CLOSED && sqlite3SafetyOn(db) ){
    
    return SQLITE_ERROR;
  }

  for(j=0; j<db->nDb; j++){
    struct Db *pDb = &db->aDb[j];
    if( pDb->pBt ){
      sqlite3BtreeClose(pDb->pBt);
      pDb->pBt = 0;
      if( j!=1 ){
        pDb->pSchema = 0;
      }
    }
  }
  sqlite3ResetInternalSchema(db, 0);
  assert( db->nDb<=2 );
  assert( db->aDb==db->aDbStatic );
  for(i=sqliteHashFirst(&db->aFunc); i; i=sqliteHashNext(i)){
    FuncDef *pFunc, *pNext;
    for(pFunc = (FuncDef*)sqliteHashData(i); pFunc; pFunc=pNext){
      pNext = pFunc->pNext;
      sqliteFree(pFunc);
    }
  }

  for(i=sqliteHashFirst(&db->aCollSeq); i; i=sqliteHashNext(i)){
    CollSeq *pColl = (CollSeq *)sqliteHashData(i);
    sqliteFree(pColl);
  }
  sqlite3HashClear(&db->aCollSeq);

  sqlite3HashClear(&db->aFunc);
  sqlite3Error(db, SQLITE_OK, 0); 
  if( db->pErr ){
    sqlite3ValueFree(db->pErr);
  }

  db->magic = SQLITE_MAGIC_ERROR;

  





  sqliteFree(db->aDb[1].pSchema);
  sqliteFree(db);
  sqlite3ReleaseThreadData();
  return SQLITE_OK;
}




void sqlite3RollbackAll(sqlite3 *db){
  int i;
  int inTrans = 0;
  for(i=0; i<db->nDb; i++){
    if( db->aDb[i].pBt ){
      if( sqlite3BtreeIsInTrans(db->aDb[i].pBt) ){
        inTrans = 1;
      }
      sqlite3BtreeRollback(db->aDb[i].pBt);
      db->aDb[i].inTrans = 0;
    }
  }
  if( db->flags&SQLITE_InternChanges ){
    sqlite3ResetInternalSchema(db, 0);
  }

  
  if( db->xRollbackCallback && (inTrans || !db->autoCommit) ){
    db->xRollbackCallback(db->pRollbackArg);
  }
}





const char *sqlite3ErrStr(int rc){
  const char *z;
  switch( rc ){
    case SQLITE_ROW:
    case SQLITE_DONE:
    case SQLITE_OK:         z = "not an error";                          break;
    case SQLITE_ERROR:      z = "SQL logic error or missing database";   break;
    case SQLITE_PERM:       z = "access permission denied";              break;
    case SQLITE_ABORT:      z = "callback requested query abort";        break;
    case SQLITE_BUSY:       z = "database is locked";                    break;
    case SQLITE_LOCKED:     z = "database table is locked";              break;
    case SQLITE_NOMEM:      z = "out of memory";                         break;
    case SQLITE_READONLY:   z = "attempt to write a readonly database";  break;
    case SQLITE_INTERRUPT:  z = "interrupted";                           break;
    case SQLITE_IOERR:      z = "disk I/O error";                        break;
    case SQLITE_CORRUPT:    z = "database disk image is malformed";      break;
    case SQLITE_FULL:       z = "database or disk is full";              break;
    case SQLITE_CANTOPEN:   z = "unable to open database file";          break;
    case SQLITE_PROTOCOL:   z = "database locking protocol failure";     break;
    case SQLITE_EMPTY:      z = "table contains no data";                break;
    case SQLITE_SCHEMA:     z = "database schema has changed";           break;
    case SQLITE_CONSTRAINT: z = "constraint failed";                     break;
    case SQLITE_MISMATCH:   z = "datatype mismatch";                     break;
    case SQLITE_MISUSE:     z = "library routine called out of sequence";break;
    case SQLITE_NOLFS:      z = "kernel lacks large file support";       break;
    case SQLITE_AUTH:       z = "authorization denied";                  break;
    case SQLITE_FORMAT:     z = "auxiliary database format error";       break;
    case SQLITE_RANGE:      z = "bind or column index out of range";     break;
    case SQLITE_NOTADB:     z = "file is encrypted or is not a database";break;
    default:                z = "unknown error";                         break;
  }
  return z;
}







static int sqliteDefaultBusyCallback(
 void *ptr,               
 int count                
){
#if OS_WIN || (defined(HAVE_USLEEP) && HAVE_USLEEP)
  static const u8 delays[] =
     { 1, 2, 5, 10, 15, 20, 25, 25,  25,  50,  50, 100 };
  static const u8 totals[] =
     { 0, 1, 3,  8, 18, 33, 53, 78, 103, 128, 178, 228 };
# define NDELAY (sizeof(delays)/sizeof(delays[0]))
  int timeout = ((sqlite3 *)ptr)->busyTimeout;
  int delay, prior;

  assert( count>=0 );
  if( count < NDELAY ){
    delay = delays[count];
    prior = totals[count];
  }else{
    delay = delays[NDELAY-1];
    prior = totals[NDELAY-1] + delay*(count-(NDELAY-1));
  }
  if( prior + delay > timeout ){
    delay = timeout - prior;
    if( delay<=0 ) return 0;
  }
  sqlite3OsSleep(delay);
  return 1;
#else
  int timeout = ((sqlite3 *)ptr)->busyTimeout;
  if( (count+1)*1000 > timeout ){
    return 0;
  }
  sqlite3OsSleep(1000);
  return 1;
#endif
}








int sqlite3InvokeBusyHandler(BusyHandler *p){
  int rc;
  if( p==0 || p->xFunc==0 || p->nBusy<0 ) return 0;
  rc = p->xFunc(p->pArg, p->nBusy);
  if( rc==0 ){
    p->nBusy = -1;
  }else{
    p->nBusy++;
  }
  return rc; 
}





int sqlite3_busy_handler(
  sqlite3 *db,
  int (*xBusy)(void*,int),
  void *pArg
){
  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }
  db->busyHandler.xFunc = xBusy;
  db->busyHandler.pArg = pArg;
  db->busyHandler.nBusy = 0;
  return SQLITE_OK;
}

#ifndef SQLITE_OMIT_PROGRESS_CALLBACK





void sqlite3_progress_handler(
  sqlite3 *db, 
  int nOps,
  int (*xProgress)(void*), 
  void *pArg
){
  if( !sqlite3SafetyCheck(db) ){
    if( nOps>0 ){
      db->xProgress = xProgress;
      db->nProgressOps = nOps;
      db->pProgressArg = pArg;
    }else{
      db->xProgress = 0;
      db->nProgressOps = 0;
      db->pProgressArg = 0;
    }
  }
}
#endif






int sqlite3_busy_timeout(sqlite3 *db, int ms){
  if( ms>0 ){
    db->busyTimeout = ms;
    sqlite3_busy_handler(db, sqliteDefaultBusyCallback, (void*)db);
  }else{
    sqlite3_busy_handler(db, 0, 0);
  }
  return SQLITE_OK;
}




void sqlite3_interrupt(sqlite3 *db){
  if( !sqlite3SafetyCheck(db) ){
    db->flags |= SQLITE_Interrupt;
  }
}









void sqlite3_free(char *p){ free(p); }







int sqlite3CreateFunc(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int enc,
  void *pUserData,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value **),
  void (*xStep)(sqlite3_context*,int,sqlite3_value **),
  void (*xFinal)(sqlite3_context*)
){
  FuncDef *p;
  int nName;

  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }
  if( zFunctionName==0 ||
      (xFunc && (xFinal || xStep)) || 
      (!xFunc && (xFinal && !xStep)) ||
      (!xFunc && (!xFinal && xStep)) ||
      (nArg<-1 || nArg>127) ||
      (255<(nName = strlen(zFunctionName))) ){
    return SQLITE_ERROR;
  }
  
#ifndef SQLITE_OMIT_UTF16
  






  if( enc==SQLITE_UTF16 ){
    enc = SQLITE_UTF16NATIVE;
  }else if( enc==SQLITE_ANY ){
    int rc;
    rc = sqlite3CreateFunc(db, zFunctionName, nArg, SQLITE_UTF8,
         pUserData, xFunc, xStep, xFinal);
    if( rc!=SQLITE_OK ) return rc;
    rc = sqlite3CreateFunc(db, zFunctionName, nArg, SQLITE_UTF16LE,
        pUserData, xFunc, xStep, xFinal);
    if( rc!=SQLITE_OK ) return rc;
    enc = SQLITE_UTF16BE;
  }
#else
  enc = SQLITE_UTF8;
#endif
  
  




  p = sqlite3FindFunction(db, zFunctionName, nName, nArg, enc, 0);
  if( p && p->iPrefEnc==enc && p->nArg==nArg ){
    if( db->activeVdbeCnt ){
      sqlite3Error(db, SQLITE_BUSY, 
        "Unable to delete/modify user-function due to active statements");
      assert( !sqlite3MallocFailed() );
      return SQLITE_BUSY;
    }else{
      sqlite3ExpirePreparedStatements(db);
    }
  }

  p = sqlite3FindFunction(db, zFunctionName, nName, nArg, enc, 1);
  if( p ){
    p->flags = 0;
    p->xFunc = xFunc;
    p->xStep = xStep;
    p->xFinalize = xFinal;
    p->pUserData = pUserData;
  }
  return SQLITE_OK;
}




int sqlite3_create_function(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int enc,
  void *p,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value **),
  void (*xStep)(sqlite3_context*,int,sqlite3_value **),
  void (*xFinal)(sqlite3_context*)
){
  int rc;
  assert( !sqlite3MallocFailed() );
  rc = sqlite3CreateFunc(db, zFunctionName, nArg, enc, p, xFunc, xStep, xFinal);

  return sqlite3ApiExit(db, rc);
}

#ifndef SQLITE_OMIT_UTF16
int sqlite3_create_function16(
  sqlite3 *db,
  const void *zFunctionName,
  int nArg,
  int eTextRep,
  void *p,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
){
  int rc;
  char *zFunc8;
  assert( !sqlite3MallocFailed() );

  zFunc8 = sqlite3utf16to8(zFunctionName, -1);
  rc = sqlite3CreateFunc(db, zFunc8, nArg, eTextRep, p, xFunc, xStep, xFinal);
  sqliteFree(zFunc8);

  return sqlite3ApiExit(db, rc);
}
#endif

#ifndef SQLITE_OMIT_TRACE








void *sqlite3_trace(sqlite3 *db, void (*xTrace)(void*,const char*), void *pArg){
  void *pOld = db->pTraceArg;
  db->xTrace = xTrace;
  db->pTraceArg = pArg;
  return pOld;
}








void *sqlite3_profile(
  sqlite3 *db,
  void (*xProfile)(void*,const char*,sqlite_uint64),
  void *pArg
){
  void *pOld = db->pProfileArg;
  db->xProfile = xProfile;
  db->pProfileArg = pArg;
  return pOld;
}
#endif 







void *sqlite3_commit_hook(
  sqlite3 *db,              
  int (*xCallback)(void*),  
  void *pArg                
){
  void *pOld = db->pCommitArg;
  db->xCommitCallback = xCallback;
  db->pCommitArg = pArg;
  return pOld;
}





void *sqlite3_update_hook(
  sqlite3 *db,              
  void (*xCallback)(void*,int,char const *,char const *,sqlite_int64),
  void *pArg                
){
  void *pRet = db->pUpdateArg;
  db->xUpdateCallback = xCallback;
  db->pUpdateArg = pArg;
  return pRet;
}





void *sqlite3_rollback_hook(
  sqlite3 *db,              
  void (*xCallback)(void*), 
  void *pArg                
){
  void *pRet = db->pRollbackArg;
  db->xRollbackCallback = xCallback;
  db->pRollbackArg = pArg;
  return pRet;
}


























int sqlite3BtreeFactory(
  const sqlite3 *db,        
  const char *zFilename,    
  int omitJournal,          
  int nCache,               
  Btree **ppBtree           
){
  int btree_flags = 0;
  int rc;
  
  assert( ppBtree != 0);
  if( omitJournal ){
    btree_flags |= BTREE_OMIT_JOURNAL;
  }
  if( db->flags & SQLITE_NoReadlock ){
    btree_flags |= BTREE_NO_READLOCK;
  }
  if( zFilename==0 ){
#if TEMP_STORE==0
    
#endif
#ifndef SQLITE_OMIT_MEMORYDB
#if TEMP_STORE==1
    if( db->temp_store==2 ) zFilename = ":memory:";
#endif
#if TEMP_STORE==2
    if( db->temp_store!=1 ) zFilename = ":memory:";
#endif
#if TEMP_STORE==3
    zFilename = ":memory:";
#endif
#endif 
  }

  rc = sqlite3BtreeOpen(zFilename, (sqlite3 *)db, ppBtree, btree_flags);
  if( rc==SQLITE_OK ){
    sqlite3BtreeSetBusyHandler(*ppBtree, (void*)&db->busyHandler);
    sqlite3BtreeSetCacheSize(*ppBtree, nCache);
  }
  return rc;
}





const char *sqlite3_errmsg(sqlite3 *db){
  const char *z;
  if( !db || sqlite3MallocFailed() ){
    return sqlite3ErrStr(SQLITE_NOMEM);
  }
  if( sqlite3SafetyCheck(db) || db->errCode==SQLITE_MISUSE ){
    return sqlite3ErrStr(SQLITE_MISUSE);
  }
  z = (char*)sqlite3_value_text(db->pErr);
  if( z==0 ){
    z = sqlite3ErrStr(db->errCode);
  }
  return z;
}

#ifndef SQLITE_OMIT_UTF16




const void *sqlite3_errmsg16(sqlite3 *db){
  




  static const char outOfMemBe[] = {
    0, 'o', 0, 'u', 0, 't', 0, ' ', 
    0, 'o', 0, 'f', 0, ' ', 
    0, 'm', 0, 'e', 0, 'm', 0, 'o', 0, 'r', 0, 'y', 0, 0, 0
  };
  static const char misuseBe [] = {
    0, 'l', 0, 'i', 0, 'b', 0, 'r', 0, 'a', 0, 'r', 0, 'y', 0, ' ', 
    0, 'r', 0, 'o', 0, 'u', 0, 't', 0, 'i', 0, 'n', 0, 'e', 0, ' ', 
    0, 'c', 0, 'a', 0, 'l', 0, 'l', 0, 'e', 0, 'd', 0, ' ', 
    0, 'o', 0, 'u', 0, 't', 0, ' ', 
    0, 'o', 0, 'f', 0, ' ', 
    0, 's', 0, 'e', 0, 'q', 0, 'u', 0, 'e', 0, 'n', 0, 'c', 0, 'e', 0, 0, 0
  };

  const void *z;
  if( sqlite3MallocFailed() ){
    return (void *)(&outOfMemBe[SQLITE_UTF16NATIVE==SQLITE_UTF16LE?1:0]);
  }
  if( sqlite3SafetyCheck(db) || db->errCode==SQLITE_MISUSE ){
    return (void *)(&misuseBe[SQLITE_UTF16NATIVE==SQLITE_UTF16LE?1:0]);
  }
  z = sqlite3_value_text16(db->pErr);
  if( z==0 ){
    sqlite3ValueSetStr(db->pErr, -1, sqlite3ErrStr(db->errCode),
         SQLITE_UTF8, SQLITE_STATIC);
    z = sqlite3_value_text16(db->pErr);
  }
  sqlite3ApiExit(0, 0);
  return z;
}
#endif 





int sqlite3_errcode(sqlite3 *db){
  if( !db || sqlite3MallocFailed() ){
    return SQLITE_NOMEM;
  }
  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }
  return db->errCode;
}





static int createCollation(
  sqlite3* db, 
  const char *zName, 
  int enc, 
  void* pCtx,
  int(*xCompare)(void*,int,const void*,int,const void*)
){
  CollSeq *pColl;
  int enc2;
  
  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }

  



  enc2 = enc & ~SQLITE_UTF16_ALIGNED;
  if( enc2==SQLITE_UTF16 ){
    enc2 = SQLITE_UTF16NATIVE;
  }

  if( (enc2&~3)!=0 ){
    sqlite3Error(db, SQLITE_ERROR, "unknown encoding");
    return SQLITE_ERROR;
  }

  



  pColl = sqlite3FindCollSeq(db, (u8)enc2, zName, strlen(zName), 0);
  if( pColl && pColl->xCmp ){
    if( db->activeVdbeCnt ){
      sqlite3Error(db, SQLITE_BUSY, 
        "Unable to delete/modify collation sequence due to active statements");
      return SQLITE_BUSY;
    }
    sqlite3ExpirePreparedStatements(db);
  }

  pColl = sqlite3FindCollSeq(db, (u8)enc2, zName, strlen(zName), 1);
  if( pColl ){
    pColl->xCmp = xCompare;
    pColl->pUser = pCtx;
    pColl->enc = enc2 | (enc & SQLITE_UTF16_ALIGNED);
  }
  sqlite3Error(db, SQLITE_OK, 0);
  return SQLITE_OK;
}







static int openDatabase(
  const char *zFilename, 
  sqlite3 **ppDb         
){
  sqlite3 *db;
  int rc;
  CollSeq *pColl;

  assert( !sqlite3MallocFailed() );

  
  db = sqliteMalloc( sizeof(sqlite3) );
  if( db==0 ) goto opendb_out;
  db->priorNewRowid = 0;
  db->magic = SQLITE_MAGIC_BUSY;
  db->nDb = 2;
  db->aDb = db->aDbStatic;
  db->autoCommit = 1;
  db->flags |= SQLITE_ShortColNames;
  sqlite3HashInit(&db->aFunc, SQLITE_HASH_STRING, 0);
  sqlite3HashInit(&db->aCollSeq, SQLITE_HASH_STRING, 0);

  



  if( createCollation(db, "BINARY", SQLITE_UTF8, 0, binCollFunc) ||
      createCollation(db, "BINARY", SQLITE_UTF16BE, 0, binCollFunc) ||
      createCollation(db, "BINARY", SQLITE_UTF16LE, 0, binCollFunc) ||
      (db->pDfltColl = sqlite3FindCollSeq(db, SQLITE_UTF8, "BINARY", 6, 0))==0 
  ){
    assert( sqlite3MallocFailed() );
    db->magic = SQLITE_MAGIC_CLOSED;
    goto opendb_out;
  }

  
  createCollation(db, "NOCASE", SQLITE_UTF8, 0, nocaseCollatingFunc);

  
  db->pDfltColl->type = SQLITE_COLL_BINARY;
  pColl = sqlite3FindCollSeq(db, SQLITE_UTF8, "NOCASE", 6, 0);
  if( pColl ){
    pColl->type = SQLITE_COLL_NOCASE;
  }

  
  rc = sqlite3BtreeFactory(db, zFilename, 0, MAX_PAGES, &db->aDb[0].pBt);
  if( rc!=SQLITE_OK ){
    sqlite3Error(db, rc, 0);
    db->magic = SQLITE_MAGIC_CLOSED;
    goto opendb_out;
  }
  db->aDb[0].pSchema = sqlite3SchemaGet(db->aDb[0].pBt);
  db->aDb[1].pSchema = sqlite3SchemaGet(0);
  if( db->aDb[0].pSchema ){
    ENC(db) = SQLITE_UTF8;
  }


  


  db->aDb[0].zName = "main";
  db->aDb[0].safety_level = 3;
#ifndef SQLITE_OMIT_TEMPDB
  db->aDb[1].zName = "temp";
  db->aDb[1].safety_level = 1;
#endif

  



  if( !sqlite3MallocFailed() ){
    sqlite3RegisterBuiltinFunctions(db);
    sqlite3Error(db, SQLITE_OK, 0);
  }
  db->magic = SQLITE_MAGIC_OPEN;

opendb_out:
  if( SQLITE_NOMEM==(rc = sqlite3_errcode(db)) ){
    sqlite3_close(db);
    db = 0;
  }
  *ppDb = db;
  return sqlite3ApiExit(0, rc);
}




int sqlite3_open(
  const char *zFilename, 
  sqlite3 **ppDb 
){
  return openDatabase(zFilename, ppDb);
}

#ifndef SQLITE_OMIT_UTF16



int sqlite3_open16(
  const void *zFilename, 
  sqlite3 **ppDb
){
  char const *zFilename8;   
  int rc = SQLITE_OK;
  sqlite3_value *pVal;

  assert( zFilename );
  assert( ppDb );
  *ppDb = 0;
  pVal = sqlite3ValueNew();
  sqlite3ValueSetStr(pVal, -1, zFilename, SQLITE_UTF16NATIVE, SQLITE_STATIC);
  zFilename8 = sqlite3ValueText(pVal, SQLITE_UTF8);
  if( zFilename8 ){
    rc = openDatabase(zFilename8, ppDb);
    if( rc==SQLITE_OK && *ppDb ){
      rc = sqlite3_exec(*ppDb, "PRAGMA encoding = 'UTF-16'", 0, 0, 0);
      if( rc!=SQLITE_OK ){
        sqlite3_close(*ppDb);
        *ppDb = 0;
      }
    }
  }
  sqlite3ValueFree(pVal);

  return sqlite3ApiExit(0, rc);
}
#endif 










int sqlite3_finalize(sqlite3_stmt *pStmt){
  int rc;
  if( pStmt==0 ){
    rc = SQLITE_OK;
  }else{
    rc = sqlite3VdbeFinalize((Vdbe*)pStmt);
  }
  return rc;
}









int sqlite3_reset(sqlite3_stmt *pStmt){
  int rc;
  if( pStmt==0 ){
    rc = SQLITE_OK;
  }else{
    rc = sqlite3VdbeReset((Vdbe*)pStmt);
    sqlite3VdbeMakeReady((Vdbe*)pStmt, -1, 0, 0, 0);
  }
  return rc;
}




int sqlite3_create_collation(
  sqlite3* db, 
  const char *zName, 
  int enc, 
  void* pCtx,
  int(*xCompare)(void*,int,const void*,int,const void*)
){
  int rc;
  assert( !sqlite3MallocFailed() );
  rc = createCollation(db, zName, enc, pCtx, xCompare);
  return sqlite3ApiExit(db, rc);
}

#ifndef SQLITE_OMIT_UTF16



int sqlite3_create_collation16(
  sqlite3* db, 
  const char *zName, 
  int enc, 
  void* pCtx,
  int(*xCompare)(void*,int,const void*,int,const void*)
){
  int rc = SQLITE_OK;
  char *zName8; 
  assert( !sqlite3MallocFailed() );
  zName8 = sqlite3utf16to8(zName, -1);
  if( zName8 ){
    rc = createCollation(db, zName8, enc, pCtx, xCompare);
    sqliteFree(zName8);
  }
  return sqlite3ApiExit(db, rc);
}
#endif 





int sqlite3_collation_needed(
  sqlite3 *db, 
  void *pCollNeededArg, 
  void(*xCollNeeded)(void*,sqlite3*,int eTextRep,const char*)
){
  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }
  db->xCollNeeded = xCollNeeded;
  db->xCollNeeded16 = 0;
  db->pCollNeededArg = pCollNeededArg;
  return SQLITE_OK;
}

#ifndef SQLITE_OMIT_UTF16




int sqlite3_collation_needed16(
  sqlite3 *db, 
  void *pCollNeededArg, 
  void(*xCollNeeded16)(void*,sqlite3*,int eTextRep,const void*)
){
  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }
  db->xCollNeeded = 0;
  db->xCollNeeded16 = xCollNeeded16;
  db->pCollNeededArg = pCollNeededArg;
  return SQLITE_OK;
}
#endif 

#ifndef SQLITE_OMIT_GLOBALRECOVER




int sqlite3_global_recover(){
  return SQLITE_OK;
}
#endif









int sqlite3_get_autocommit(sqlite3 *db){
  return db->autoCommit;
}

#ifdef SQLITE_DEBUG





int sqlite3Corrupt(void){
  return SQLITE_CORRUPT;
}
#endif


#ifndef SQLITE_OMIT_SHARED_CACHE







int sqlite3_enable_shared_cache(int enable){
  ThreadData *pTd = sqlite3ThreadData();
  if( pTd ){
    




    if( pTd->pBtree && !enable ){
      assert( pTd->useSharedData );
      return SQLITE_MISUSE;
    }

    pTd->useSharedData = enable;
    sqlite3ReleaseThreadData();
  }
  return sqlite3ApiExit(0, SQLITE_OK);
}
#endif





void sqlite3_thread_cleanup(void){
  ThreadData *pTd = sqlite3OsThreadSpecificData(0);
  if( pTd ){
    memset(pTd, 0, sizeof(*pTd));
    sqlite3OsThreadSpecificData(-1);
  }
}





#ifdef SQLITE_ENABLE_COLUMN_METADATA
int sqlite3_table_column_metadata(
  sqlite3 *db,                
  const char *zDbName,        
  const char *zTableName,     
  const char *zColumnName,    
  char const **pzDataType,    
  char const **pzCollSeq,     
  int *pNotNull,              
  int *pPrimaryKey,           
  int *pAutoinc               
){
  int rc;
  char *zErrMsg = 0;
  Table *pTab = 0;
  Column *pCol = 0;
  int iCol;

  char const *zDataType = 0;
  char const *zCollSeq = 0;
  int notnull = 0;
  int primarykey = 0;
  int autoinc = 0;

  
  if( sqlite3SafetyOn(db) ){
    return SQLITE_MISUSE;
  }
  rc = sqlite3Init(db, &zErrMsg);
  if( SQLITE_OK!=rc ){
    goto error_out;
  }

  
  pTab = sqlite3FindTable(db, zTableName, zDbName);
  if( !pTab || pTab->pSelect ){
    pTab = 0;
    goto error_out;
  }

  
  if( sqlite3IsRowid(zColumnName) ){
    iCol = pTab->iPKey;
    if( iCol>=0 ){
      pCol = &pTab->aCol[iCol];
    }
  }else{
    for(iCol=0; iCol<pTab->nCol; iCol++){
      pCol = &pTab->aCol[iCol];
      if( 0==sqlite3StrICmp(pCol->zName, zColumnName) ){
        break;
      }
    }
    if( iCol==pTab->nCol ){
      pTab = 0;
      goto error_out;
    }
  }

  








 
  if( pCol ){
    zDataType = pCol->zType;
    zCollSeq = pCol->zColl;
    notnull = (pCol->notNull?1:0);
    primarykey  = (pCol->isPrimKey?1:0);
    autoinc = ((pTab->iPKey==iCol && pTab->autoInc)?1:0);
  }else{
    zDataType = "INTEGER";
    primarykey = 1;
  }
  if( !zCollSeq ){
    zCollSeq = "BINARY";
  }

error_out:
  if( sqlite3SafetyOff(db) ){
    rc = SQLITE_MISUSE;
  }

  



  if( pzDataType ) *pzDataType = zDataType;
  if( pzCollSeq ) *pzCollSeq = zCollSeq;
  if( pNotNull ) *pNotNull = notnull;
  if( pPrimaryKey ) *pPrimaryKey = primarykey;
  if( pAutoinc ) *pAutoinc = autoinc;

  if( SQLITE_OK==rc && !pTab ){
    sqlite3SetString(&zErrMsg, "no such table column: ", zTableName, ".", 
        zColumnName, 0);
    rc = SQLITE_ERROR;
  }
  sqlite3Error(db, rc, (zErrMsg?"%s":0), zErrMsg);
  sqliteFree(zErrMsg);
  return sqlite3ApiExit(db, rc);
}
#endif
