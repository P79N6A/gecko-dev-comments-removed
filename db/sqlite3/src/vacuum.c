

















#include "sqliteInt.h"
#include "vdbeInt.h"
#include "os.h"

#ifndef SQLITE_OMIT_VACUUM



static void randomName(unsigned char *zBuf){
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789";
  int i;
  sqlite3Randomness(20, zBuf);
  for(i=0; i<20; i++){
    zBuf[i] = zChars[ zBuf[i]%(sizeof(zChars)-1) ];
  }
}




static int execSql(sqlite3 *db, const char *zSql){
  sqlite3_stmt *pStmt;
  if( SQLITE_OK!=sqlite3_prepare(db, zSql, -1, &pStmt, 0) ){
    return sqlite3_errcode(db);
  }
  while( SQLITE_ROW==sqlite3_step(pStmt) ){}
  return sqlite3_finalize(pStmt);
}





static int execExecSql(sqlite3 *db, const char *zSql){
  sqlite3_stmt *pStmt;
  int rc;

  rc = sqlite3_prepare(db, zSql, -1, &pStmt, 0);
  if( rc!=SQLITE_OK ) return rc;

  while( SQLITE_ROW==sqlite3_step(pStmt) ){
    rc = execSql(db, (char*)sqlite3_column_text(pStmt, 0));
    if( rc!=SQLITE_OK ){
      sqlite3_finalize(pStmt);
      return rc;
    }
  }

  return sqlite3_finalize(pStmt);
}

#endif











void sqlite3Vacuum(Parse *pParse){
  Vdbe *v = sqlite3GetVdbe(pParse);
  if( v ){
    sqlite3VdbeAddOp(v, OP_Vacuum, 0, 0);
  }
  return;
}




int sqlite3RunVacuum(char **pzErrMsg, sqlite3 *db){
  int rc = SQLITE_OK;     
#ifndef SQLITE_OMIT_VACUUM
  const char *zFilename;  
  int nFilename;          
  char *zTemp = 0;        
  Btree *pMain;           
  Btree *pTemp;
  char *zSql = 0;
  int saved_flags;       
  Db *pDb = 0;           

  
  saved_flags = db->flags;
  db->flags |= SQLITE_WriteSchema | SQLITE_IgnoreChecks;

  if( !db->autoCommit ){
    sqlite3SetString(pzErrMsg, "cannot VACUUM from within a transaction", 
       (char*)0);
    rc = SQLITE_ERROR;
    goto end_of_vacuum;
  }

  


  pMain = db->aDb[0].pBt;
  zFilename = sqlite3BtreeGetFilename(pMain);
  assert( zFilename );
  if( zFilename[0]=='\0' ){
    



    return SQLITE_OK;
  }
  nFilename = strlen(zFilename);
  zTemp = sqliteMalloc( nFilename+100 );
  if( zTemp==0 ){
    rc = SQLITE_NOMEM;
    goto end_of_vacuum;
  }
  strcpy(zTemp, zFilename);

  







  do {
    zTemp[nFilename] = '-';
    randomName((unsigned char*)&zTemp[nFilename+1]);
  } while( sqlite3OsFileExists(zTemp) );

  







  zSql = sqlite3MPrintf("ATTACH '%q' AS vacuum_db;", zTemp);
  if( !zSql ){
    rc = SQLITE_NOMEM;
    goto end_of_vacuum;
  }
  rc = execSql(db, zSql);
  sqliteFree(zSql);
  zSql = 0;
  if( rc!=SQLITE_OK ) goto end_of_vacuum;
  pDb = &db->aDb[db->nDb-1];
  assert( strcmp(db->aDb[db->nDb-1].zName,"vacuum_db")==0 );
  pTemp = db->aDb[db->nDb-1].pBt;
  sqlite3BtreeSetPageSize(pTemp, sqlite3BtreeGetPageSize(pMain),
     sqlite3BtreeGetReserve(pMain));
  assert( sqlite3BtreeGetPageSize(pTemp)==sqlite3BtreeGetPageSize(pMain) );
  rc = execSql(db, "PRAGMA vacuum_db.synchronous=OFF");
  if( rc!=SQLITE_OK ){
    goto end_of_vacuum;
  }

#ifndef SQLITE_OMIT_AUTOVACUUM
  sqlite3BtreeSetAutoVacuum(pTemp, sqlite3BtreeGetAutoVacuum(pMain));
#endif

  
  rc = execSql(db, "BEGIN EXCLUSIVE;");
  if( rc!=SQLITE_OK ) goto end_of_vacuum;

  


  rc = execExecSql(db, 
      "SELECT 'CREATE TABLE vacuum_db.' || substr(sql,14,100000000) "
      "  FROM sqlite_master WHERE type='table' AND name!='sqlite_sequence'");
  if( rc!=SQLITE_OK ) goto end_of_vacuum;
  rc = execExecSql(db, 
      "SELECT 'CREATE INDEX vacuum_db.' || substr(sql,14,100000000)"
      "  FROM sqlite_master WHERE sql LIKE 'CREATE INDEX %' ");
  if( rc!=SQLITE_OK ) goto end_of_vacuum;
  rc = execExecSql(db, 
      "SELECT 'CREATE UNIQUE INDEX vacuum_db.' || substr(sql,21,100000000) "
      "  FROM sqlite_master WHERE sql LIKE 'CREATE UNIQUE INDEX %'");
  if( rc!=SQLITE_OK ) goto end_of_vacuum;
  rc = execExecSql(db, 
      "SELECT 'CREATE VIEW vacuum_db.' || substr(sql,13,100000000) "
      "  FROM sqlite_master WHERE type='view'"
  );
  if( rc!=SQLITE_OK ) goto end_of_vacuum;

  



  rc = execExecSql(db, 
      "SELECT 'INSERT INTO vacuum_db.' || quote(name) "
      "|| ' SELECT * FROM ' || quote(name) || ';'"
      "FROM sqlite_master "
      "WHERE type = 'table' AND name!='sqlite_sequence';"
  );
  if( rc!=SQLITE_OK ) goto end_of_vacuum;

  

  rc = execExecSql(db, 
      "SELECT 'DELETE FROM vacuum_db.' || quote(name) || ';' "
      "FROM vacuum_db.sqlite_master WHERE name='sqlite_sequence' "
  );
  if( rc!=SQLITE_OK ) goto end_of_vacuum;
  rc = execExecSql(db, 
      "SELECT 'INSERT INTO vacuum_db.' || quote(name) "
      "|| ' SELECT * FROM ' || quote(name) || ';' "
      "FROM vacuum_db.sqlite_master WHERE name=='sqlite_sequence';"
  );
  if( rc!=SQLITE_OK ) goto end_of_vacuum;


  




  rc = execExecSql(db, 
      "SELECT 'CREATE TRIGGER  vacuum_db.' || substr(sql, 16, 1000000) "
      "FROM sqlite_master WHERE type='trigger'"
  );
  if( rc!=SQLITE_OK ) goto end_of_vacuum;


  







  if( rc==SQLITE_OK ){
    u32 meta;
    int i;

    





    static const unsigned char aCopy[] = {
       1, 1,    
       3, 0,    
       5, 0,    
       6, 0,    
    };

    assert( 1==sqlite3BtreeIsInTrans(pTemp) );
    assert( 1==sqlite3BtreeIsInTrans(pMain) );

    
    for(i=0; i<sizeof(aCopy)/sizeof(aCopy[0]); i+=2){
      rc = sqlite3BtreeGetMeta(pMain, aCopy[i], &meta);
      if( rc!=SQLITE_OK ) goto end_of_vacuum;
      rc = sqlite3BtreeUpdateMeta(pTemp, aCopy[i], meta+aCopy[i+1]);
      if( rc!=SQLITE_OK ) goto end_of_vacuum;
    }

    rc = sqlite3BtreeCopyFile(pMain, pTemp);
    if( rc!=SQLITE_OK ) goto end_of_vacuum;
    rc = sqlite3BtreeCommit(pTemp);
    if( rc!=SQLITE_OK ) goto end_of_vacuum;
    rc = sqlite3BtreeCommit(pMain);
  }

end_of_vacuum:
  
  db->flags = saved_flags;

  






  db->autoCommit = 1;

  if( pDb ){
    sqlite3MallocDisallow();
    sqlite3BtreeClose(pDb->pBt);
    sqlite3MallocAllow();
    pDb->pBt = 0;
    pDb->pSchema = 0;
  }

  



  if( rc==SQLITE_NOMEM ){
    sqlite3MallocFailed();
  }

  if( zTemp ){
    sqlite3OsDelete(zTemp);
    sqliteFree(zTemp);
  }
  sqliteFree( zSql );
  sqlite3ResetInternalSchema(db, 0);
#endif

  return rc;
}
