


















#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>











int sqlite3_exec(
  sqlite3 *db,                
  const char *zSql,           
  sqlite3_callback xCallback, 
  void *pArg,                 
  char **pzErrMsg             
){
  int rc = SQLITE_OK;
  const char *zLeftover;
  sqlite3_stmt *pStmt = 0;
  char **azCols = 0;

  int nRetry = 0;
  int nChange = 0;
  int nCallback;

  if( zSql==0 ) return SQLITE_OK;
  while( (rc==SQLITE_OK || (rc==SQLITE_SCHEMA && (++nRetry)<2)) && zSql[0] ){
    int nCol;
    char **azVals = 0;

    pStmt = 0;
    rc = sqlite3_prepare(db, zSql, -1, &pStmt, &zLeftover);
    assert( rc==SQLITE_OK || pStmt==0 );
    if( rc!=SQLITE_OK ){
      continue;
    }
    if( !pStmt ){
      
      zSql = zLeftover;
      continue;
    }

    db->nChange += nChange;
    nCallback = 0;

    nCol = sqlite3_column_count(pStmt);
    azCols = sqliteMalloc(2*nCol*sizeof(const char *) + 1);
    if( azCols==0 ){
      goto exec_out;
    }

    while( 1 ){
      int i;
      rc = sqlite3_step(pStmt);

      
      if( xCallback && (SQLITE_ROW==rc || 
          (SQLITE_DONE==rc && !nCallback && db->flags&SQLITE_NullCallback)) ){
        if( 0==nCallback ){
          for(i=0; i<nCol; i++){
            azCols[i] = (char *)sqlite3_column_name(pStmt, i);
          }
          nCallback++;
        }
        if( rc==SQLITE_ROW ){
          azVals = &azCols[nCol];
          for(i=0; i<nCol; i++){
            azVals[i] = (char *)sqlite3_column_text(pStmt, i);
          }
        }
        if( xCallback(pArg, nCol, azVals, azCols) ){
          rc = SQLITE_ABORT;
          goto exec_out;
        }
      }

      if( rc!=SQLITE_ROW ){
        rc = sqlite3_finalize(pStmt);
        pStmt = 0;
        if( db->pVdbe==0 ){
          nChange = db->nChange;
        }
        if( rc!=SQLITE_SCHEMA ){
          nRetry = 0;
          zSql = zLeftover;
          while( isspace((unsigned char)zSql[0]) ) zSql++;
        }
        break;
      }
    }

    sqliteFree(azCols);
    azCols = 0;
  }

exec_out:
  if( pStmt ) sqlite3_finalize(pStmt);
  if( azCols ) sqliteFree(azCols);

  rc = sqlite3ApiExit(0, rc);
  if( rc!=SQLITE_OK && rc==sqlite3_errcode(db) && pzErrMsg ){
    *pzErrMsg = malloc(1+strlen(sqlite3_errmsg(db)));
    if( *pzErrMsg ){
      strcpy(*pzErrMsg, sqlite3_errmsg(db));
    }
  }else if( pzErrMsg ){
    *pzErrMsg = 0;
  }

  return rc;
}
