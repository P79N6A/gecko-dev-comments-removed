

















#include "sqliteInt.h"
#include <stdlib.h>
#include <string.h>

#ifndef SQLITE_OMIT_GET_TABLE





typedef struct TabResult {
  char **azResult;
  char *zErrMsg;
  int nResult;
  int nAlloc;
  int nRow;
  int nColumn;
  int nData;
  int rc;
} TabResult;






static int sqlite3_get_table_cb(void *pArg, int nCol, char **argv, char **colv){
  TabResult *p = (TabResult*)pArg;
  int need;
  int i;
  char *z;

  


  if( p->nRow==0 && argv!=0 ){
    need = nCol*2;
  }else{
    need = nCol;
  }
  if( p->nData + need >= p->nAlloc ){
    char **azNew;
    p->nAlloc = p->nAlloc*2 + need + 1;
    azNew = realloc( p->azResult, sizeof(char*)*p->nAlloc );
    if( azNew==0 ) goto malloc_failed;
    p->azResult = azNew;
  }

  


  if( p->nRow==0 ){
    p->nColumn = nCol;
    for(i=0; i<nCol; i++){
      if( colv[i]==0 ){
        z = 0;
      }else{
        z = malloc( strlen(colv[i])+1 );
        if( z==0 ) goto malloc_failed;
        strcpy(z, colv[i]);
      }
      p->azResult[p->nData++] = z;
    }
  }else if( p->nColumn!=nCol ){
    sqlite3SetString(&p->zErrMsg,
       "sqlite3_get_table() called with two or more incompatible queries",
       (char*)0);
    p->rc = SQLITE_ERROR;
    return 1;
  }

  

  if( argv!=0 ){
    for(i=0; i<nCol; i++){
      if( argv[i]==0 ){
        z = 0;
      }else{
        z = malloc( strlen(argv[i])+1 );
        if( z==0 ) goto malloc_failed;
        strcpy(z, argv[i]);
      }
      p->azResult[p->nData++] = z;
    }
    p->nRow++;
  }
  return 0;

malloc_failed:
  p->rc = SQLITE_NOMEM;
  return 1;
}











int sqlite3_get_table(
  sqlite3 *db,                
  const char *zSql,           
  char ***pazResult,          
  int *pnRow,                 
  int *pnColumn,              
  char **pzErrMsg             
){
  int rc;
  TabResult res;
  if( pazResult==0 ){ return SQLITE_ERROR; }
  *pazResult = 0;
  if( pnColumn ) *pnColumn = 0;
  if( pnRow ) *pnRow = 0;
  res.zErrMsg = 0;
  res.nResult = 0;
  res.nRow = 0;
  res.nColumn = 0;
  res.nData = 1;
  res.nAlloc = 20;
  res.rc = SQLITE_OK;
  res.azResult = malloc( sizeof(char*)*res.nAlloc );
  if( res.azResult==0 ) return SQLITE_NOMEM;
  res.azResult[0] = 0;
  rc = sqlite3_exec(db, zSql, sqlite3_get_table_cb, &res, pzErrMsg);
  if( res.azResult ){
    assert( sizeof(res.azResult[0])>= sizeof(res.nData) );
    res.azResult[0] = (char*)res.nData;
  }
  if( rc==SQLITE_ABORT ){
    sqlite3_free_table(&res.azResult[1]);
    if( res.zErrMsg ){
      if( pzErrMsg ){
        free(*pzErrMsg);
        *pzErrMsg = sqlite3_mprintf("%s",res.zErrMsg);
      }
      sqliteFree(res.zErrMsg);
    }
    db->errCode = res.rc;
    return res.rc;
  }
  sqliteFree(res.zErrMsg);
  if( rc!=SQLITE_OK ){
    sqlite3_free_table(&res.azResult[1]);
    return rc;
  }
  if( res.nAlloc>res.nData ){
    char **azNew;
    azNew = realloc( res.azResult, sizeof(char*)*(res.nData+1) );
    if( azNew==0 ){
      sqlite3_free_table(&res.azResult[1]);
      return SQLITE_NOMEM;
    }
    res.nAlloc = res.nData+1;
    res.azResult = azNew;
  }
  *pazResult = &res.azResult[1];
  if( pnColumn ) *pnColumn = res.nColumn;
  if( pnRow ) *pnRow = res.nRow;
  return rc;
}




void sqlite3_free_table(
  char **azResult            
){
  if( azResult ){
    int i, n;
    azResult--;
    if( azResult==0 ) return;
    n = (int)azResult[0];
    for(i=1; i<n; i++){ if( azResult[i] ) free(azResult[i]); }
    free(azResult);
  }
}

#endif 
