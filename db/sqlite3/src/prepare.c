
















#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>





static void corruptSchema(InitData *pData, const char *zExtra){
  if( !sqlite3MallocFailed() ){
    sqlite3SetString(pData->pzErrMsg, "malformed database schema",
       zExtra!=0 && zExtra[0]!=0 ? " - " : (char*)0, zExtra, (char*)0);
  }
}















int sqlite3InitCallback(void *pInit, int argc, char **argv, char **azColName){
  InitData *pData = (InitData*)pInit;
  sqlite3 *db = pData->db;
  int iDb;

  if( sqlite3MallocFailed() ){
    return SQLITE_NOMEM;
  }

  assert( argc==4 );
  if( argv==0 ) return 0;   
  if( argv[1]==0 || argv[3]==0 ){
    corruptSchema(pData, 0);
    return 1;
  }
  iDb = atoi(argv[3]);
  assert( iDb>=0 && iDb<db->nDb );
  if( argv[2] && argv[2][0] ){
    




    char *zErr;
    int rc;
    assert( db->init.busy );
    db->init.iDb = iDb;
    db->init.newTnum = atoi(argv[1]);
    rc = sqlite3_exec(db, argv[2], 0, 0, &zErr);
    db->init.iDb = 0;
    assert( rc!=SQLITE_OK || zErr==0 );
    if( SQLITE_OK!=rc ){
      if( rc==SQLITE_NOMEM ){
        sqlite3FailedMalloc();
      }else{
        corruptSchema(pData, zErr);
      }
      sqlite3_free(zErr);
      return rc;
    }
  }else{
    





    Index *pIndex;
    pIndex = sqlite3FindIndex(db, argv[0], db->aDb[iDb].zName);
    if( pIndex==0 || pIndex->tnum!=0 ){
      




      ;
    }else{
      pIndex->tnum = atoi(argv[1]);
    }
  }
  return 0;
}









static int sqlite3InitOne(sqlite3 *db, int iDb, char **pzErrMsg){
  int rc;
  BtCursor *curMain;
  int size;
  Table *pTab;
  Db *pDb;
  char const *azArg[5];
  char zDbNum[30];
  int meta[10];
  InitData initData;
  char const *zMasterSchema;
  char const *zMasterName = SCHEMA_TABLE(iDb);

  


  static const char master_schema[] = 
     "CREATE TABLE sqlite_master(\n"
     "  type text,\n"
     "  name text,\n"
     "  tbl_name text,\n"
     "  rootpage integer,\n"
     "  sql text\n"
     ")"
  ;
#ifndef SQLITE_OMIT_TEMPDB
  static const char temp_master_schema[] = 
     "CREATE TEMP TABLE sqlite_temp_master(\n"
     "  type text,\n"
     "  name text,\n"
     "  tbl_name text,\n"
     "  rootpage integer,\n"
     "  sql text\n"
     ")"
  ;
#else
  #define temp_master_schema 0
#endif

  assert( iDb>=0 && iDb<db->nDb );
  assert( db->aDb[iDb].pSchema );

  



  if( !OMIT_TEMPDB && iDb==1 ){
    zMasterSchema = temp_master_schema;
  }else{
    zMasterSchema = master_schema;
  }
  zMasterName = SCHEMA_TABLE(iDb);

  
  sqlite3SafetyOff(db);
  azArg[0] = zMasterName;
  azArg[1] = "1";
  azArg[2] = zMasterSchema;
  sprintf(zDbNum, "%d", iDb);
  azArg[3] = zDbNum;
  azArg[4] = 0;
  initData.db = db;
  initData.pzErrMsg = pzErrMsg;
  rc = sqlite3InitCallback(&initData, 4, (char **)azArg, 0);
  if( rc!=SQLITE_OK ){
    sqlite3SafetyOn(db);
    return rc;
  }
  pTab = sqlite3FindTable(db, zMasterName, db->aDb[iDb].zName);
  if( pTab ){
    pTab->readOnly = 1;
  }
  sqlite3SafetyOn(db);

  

  pDb = &db->aDb[iDb];
  if( pDb->pBt==0 ){
    if( !OMIT_TEMPDB && iDb==1 ){
      DbSetProperty(db, 1, DB_SchemaLoaded);
    }
    return SQLITE_OK;
  }
  rc = sqlite3BtreeCursor(pDb->pBt, MASTER_ROOT, 0, 0, 0, &curMain);
  if( rc!=SQLITE_OK && rc!=SQLITE_EMPTY ){
    sqlite3SetString(pzErrMsg, sqlite3ErrStr(rc), (char*)0);
    return rc;
  }

  
















  if( rc==SQLITE_OK ){
    int i;
    for(i=0; rc==SQLITE_OK && i<sizeof(meta)/sizeof(meta[0]); i++){
      rc = sqlite3BtreeGetMeta(pDb->pBt, i+1, (u32 *)&meta[i]);
    }
    if( rc ){
      sqlite3SetString(pzErrMsg, sqlite3ErrStr(rc), (char*)0);
      sqlite3BtreeCloseCursor(curMain);
      return rc;
    }
  }else{
    memset(meta, 0, sizeof(meta));
  }
  pDb->pSchema->schema_cookie = meta[0];

  




  if( meta[4] ){  
    if( iDb==0 ){
      
      ENC(db) = (u8)meta[4];
      db->pDfltColl = sqlite3FindCollSeq(db, SQLITE_UTF8, "BINARY", 6, 0);
    }else{
      
      if( meta[4]!=ENC(db) ){
        sqlite3BtreeCloseCursor(curMain);
        sqlite3SetString(pzErrMsg, "attached databases must use the same"
            " text encoding as main database", (char*)0);
        return SQLITE_ERROR;
      }
    }
  }else{
    DbSetProperty(db, iDb, DB_Empty);
  }
  pDb->pSchema->enc = ENC(db);

  size = meta[2];
  if( size==0 ){ size = MAX_PAGES; }
  pDb->pSchema->cache_size = size;
  sqlite3BtreeSetCacheSize(pDb->pBt, pDb->pSchema->cache_size);

  





  pDb->pSchema->file_format = meta[1];
  if( pDb->pSchema->file_format==0 ){
    pDb->pSchema->file_format = 1;
  }
  if( pDb->pSchema->file_format>SQLITE_MAX_FILE_FORMAT ){
    sqlite3BtreeCloseCursor(curMain);
    sqlite3SetString(pzErrMsg, "unsupported file format", (char*)0);
    return SQLITE_ERROR;
  }


  

  assert( db->init.busy );
  if( rc==SQLITE_EMPTY ){
    
    rc = SQLITE_OK;
  }else{
    char *zSql;
    zSql = sqlite3MPrintf(
        "SELECT name, rootpage, sql, '%s' FROM '%q'.%s",
        zDbNum, db->aDb[iDb].zName, zMasterName);
    sqlite3SafetyOff(db);
    rc = sqlite3_exec(db, zSql, sqlite3InitCallback, &initData, 0);
    sqlite3SafetyOn(db);
    sqliteFree(zSql);
#ifndef SQLITE_OMIT_ANALYZE
    if( rc==SQLITE_OK ){
      sqlite3AnalysisLoad(db, iDb);
    }
#endif
    sqlite3BtreeCloseCursor(curMain);
  }
  if( sqlite3MallocFailed() ){
    
    rc = SQLITE_NOMEM;
    sqlite3ResetInternalSchema(db, 0);
  }
  if( rc==SQLITE_OK ){
    DbSetProperty(db, iDb, DB_SchemaLoaded);
  }else{
    sqlite3ResetInternalSchema(db, iDb);
  }
  return rc;
}











int sqlite3Init(sqlite3 *db, char **pzErrMsg){
  int i, rc;
  int called_initone = 0;
  
  if( db->init.busy ) return SQLITE_OK;
  rc = SQLITE_OK;
  db->init.busy = 1;
  for(i=0; rc==SQLITE_OK && i<db->nDb; i++){
    if( DbHasProperty(db, i, DB_SchemaLoaded) || i==1 ) continue;
    rc = sqlite3InitOne(db, i, pzErrMsg);
    if( rc ){
      sqlite3ResetInternalSchema(db, i);
    }
    called_initone = 1;
  }

  



#ifndef SQLITE_OMIT_TEMPDB
  if( rc==SQLITE_OK && db->nDb>1 && !DbHasProperty(db, 1, DB_SchemaLoaded) ){
    rc = sqlite3InitOne(db, 1, pzErrMsg);
    if( rc ){
      sqlite3ResetInternalSchema(db, 1);
    }
    called_initone = 1;
  }
#endif

  db->init.busy = 0;
  if( rc==SQLITE_OK && called_initone ){
    sqlite3CommitInternalChanges(db);
  }

  return rc; 
}





int sqlite3ReadSchema(Parse *pParse){
  int rc = SQLITE_OK;
  sqlite3 *db = pParse->db;
  if( !db->init.busy ){
    rc = sqlite3Init(db, &pParse->zErrMsg);
  }
  if( rc!=SQLITE_OK ){
    pParse->rc = rc;
    pParse->nErr++;
  }
  return rc;
}






static int schemaIsValid(sqlite3 *db){
  int iDb;
  int rc;
  BtCursor *curTemp;
  int cookie;
  int allOk = 1;

  for(iDb=0; allOk && iDb<db->nDb; iDb++){
    Btree *pBt;
    pBt = db->aDb[iDb].pBt;
    if( pBt==0 ) continue;
    rc = sqlite3BtreeCursor(pBt, MASTER_ROOT, 0, 0, 0, &curTemp);
    if( rc==SQLITE_OK ){
      rc = sqlite3BtreeGetMeta(pBt, 1, (u32 *)&cookie);
      if( rc==SQLITE_OK && cookie!=db->aDb[iDb].pSchema->schema_cookie ){
        allOk = 0;
      }
      sqlite3BtreeCloseCursor(curTemp);
    }
  }
  return allOk;
}








int sqlite3SchemaToIndex(sqlite3 *db, Schema *pSchema){
  int i = -1000000;

  









  if( pSchema ){
    for(i=0; i<db->nDb; i++){
      if( db->aDb[i].pSchema==pSchema ){
        break;
      }
    }
    assert( i>=0 &&i>=0 &&  i<db->nDb );
  }
  return i;
}




int sqlite3_prepare(
  sqlite3 *db,              
  const char *zSql,         
  int nBytes,               
  sqlite3_stmt **ppStmt,    
  const char** pzTail       
){
  Parse sParse;
  char *zErrMsg = 0;
  int rc = SQLITE_OK;
  int i;

  
  assert( !sqlite3MallocFailed() );

  assert( ppStmt );
  *ppStmt = 0;
  if( sqlite3SafetyOn(db) ){
    return SQLITE_MISUSE;
  }

  


  for(i=0; i<db->nDb; i++) {
    Btree *pBt = db->aDb[i].pBt;
    if( pBt && sqlite3BtreeSchemaLocked(pBt) ){
      const char *zDb = db->aDb[i].zName;
      sqlite3Error(db, SQLITE_LOCKED, "database schema is locked: %s", zDb);
      sqlite3SafetyOff(db);
      return SQLITE_LOCKED;
    }
  }
  
  memset(&sParse, 0, sizeof(sParse));
  sParse.db = db;
  if( nBytes>=0 && zSql[nBytes]!=0 ){
    char *zSqlCopy = sqlite3StrNDup(zSql, nBytes);
    sqlite3RunParser(&sParse, zSqlCopy, &zErrMsg);
    sParse.zTail += zSql - zSqlCopy;
    sqliteFree(zSqlCopy);
  }else{
    sqlite3RunParser(&sParse, zSql, &zErrMsg);
  }

  if( sqlite3MallocFailed() ){
    sParse.rc = SQLITE_NOMEM;
  }
  if( sParse.rc==SQLITE_DONE ) sParse.rc = SQLITE_OK;
  if( sParse.checkSchema && !schemaIsValid(db) ){
    sParse.rc = SQLITE_SCHEMA;
  }
  if( sParse.rc==SQLITE_SCHEMA ){
    sqlite3ResetInternalSchema(db, 0);
  }
  if( pzTail ) *pzTail = sParse.zTail;
  rc = sParse.rc;

#ifndef SQLITE_OMIT_EXPLAIN
  if( rc==SQLITE_OK && sParse.pVdbe && sParse.explain ){
    if( sParse.explain==2 ){
      sqlite3VdbeSetNumCols(sParse.pVdbe, 3);
      sqlite3VdbeSetColName(sParse.pVdbe, 0, COLNAME_NAME, "order", P3_STATIC);
      sqlite3VdbeSetColName(sParse.pVdbe, 1, COLNAME_NAME, "from", P3_STATIC);
      sqlite3VdbeSetColName(sParse.pVdbe, 2, COLNAME_NAME, "detail", P3_STATIC);
    }else{
      sqlite3VdbeSetNumCols(sParse.pVdbe, 5);
      sqlite3VdbeSetColName(sParse.pVdbe, 0, COLNAME_NAME, "addr", P3_STATIC);
      sqlite3VdbeSetColName(sParse.pVdbe, 1, COLNAME_NAME, "opcode", P3_STATIC);
      sqlite3VdbeSetColName(sParse.pVdbe, 2, COLNAME_NAME, "p1", P3_STATIC);
      sqlite3VdbeSetColName(sParse.pVdbe, 3, COLNAME_NAME, "p2", P3_STATIC);
      sqlite3VdbeSetColName(sParse.pVdbe, 4, COLNAME_NAME, "p3", P3_STATIC);
    }
  } 
#endif

  if( sqlite3SafetyOff(db) ){
    rc = SQLITE_MISUSE;
  }
  if( rc==SQLITE_OK ){
    *ppStmt = (sqlite3_stmt*)sParse.pVdbe;
  }else if( sParse.pVdbe ){
    sqlite3_finalize((sqlite3_stmt*)sParse.pVdbe);
  }

  if( zErrMsg ){
    sqlite3Error(db, rc, "%s", zErrMsg);
    sqliteFree(zErrMsg);
  }else{
    sqlite3Error(db, rc, 0);
  }

  rc = sqlite3ApiExit(db, rc);
  sqlite3ReleaseThreadData();
  return rc;
}

#ifndef SQLITE_OMIT_UTF16



int sqlite3_prepare16(
  sqlite3 *db,               
  const void *zSql,         
  int nBytes,               
  sqlite3_stmt **ppStmt,    
  const void **pzTail       
){
  



  char *zSql8;
  const char *zTail8 = 0;
  int rc = SQLITE_OK;

  if( sqlite3SafetyCheck(db) ){
    return SQLITE_MISUSE;
  }
  zSql8 = sqlite3utf16to8(zSql, nBytes);
  if( zSql8 ){
    rc = sqlite3_prepare(db, zSql8, -1, ppStmt, &zTail8);
  }

  if( zTail8 && pzTail ){
    




    int chars_parsed = sqlite3utf8CharLen(zSql8, zTail8-zSql8);
    *pzTail = (u8 *)zSql + sqlite3utf16ByteLen(zSql, chars_parsed);
  }
  sqliteFree(zSql8); 
  return sqlite3ApiExit(db, rc);
}
#endif 
