

















#include "sqliteInt.h"





#ifndef SQLITE_OMIT_AUTHORIZATION














































int sqlite3_set_authorizer(
  sqlite3 *db,
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*),
  void *pArg
){
  db->xAuth = xAuth;
  db->pAuthArg = pArg;
  sqlite3ExpirePreparedStatements(db);
  return SQLITE_OK;
}





static void sqliteAuthBadReturnCode(Parse *pParse, int rc){
  sqlite3ErrorMsg(pParse, "illegal return value (%d) from the "
    "authorization function - should be SQLITE_OK, SQLITE_IGNORE, "
    "or SQLITE_DENY", rc);
  pParse->rc = SQLITE_ERROR;
}










void sqlite3AuthRead(
  Parse *pParse,        
  Expr *pExpr,          
  SrcList *pTabList     
){
  sqlite3 *db = pParse->db;
  int rc;
  Table *pTab;          
  const char *zCol;     
  int iSrc;             
  const char *zDBase;   
  TriggerStack *pStack; 
  int iDb;              

  if( db->xAuth==0 ) return;
  if( pExpr->op==TK_AS ) return;
  assert( pExpr->op==TK_COLUMN );
  iDb = sqlite3SchemaToIndex(pParse->db, pExpr->pSchema);
  if( iDb<0 ){
    

    return;
  }
  for(iSrc=0; pTabList && iSrc<pTabList->nSrc; iSrc++){
    if( pExpr->iTable==pTabList->a[iSrc].iCursor ) break;
  }
  if( iSrc>=0 && pTabList && iSrc<pTabList->nSrc ){
    pTab = pTabList->a[iSrc].pTab;
  }else if( (pStack = pParse->trigStack)!=0 ){
    


    assert( pExpr->iTable==pStack->newIdx || pExpr->iTable==pStack->oldIdx );
    pTab = pStack->pTab;
  }else{
    return;
  }
  if( pTab==0 ) return;
  if( pExpr->iColumn>=0 ){
    assert( pExpr->iColumn<pTab->nCol );
    zCol = pTab->aCol[pExpr->iColumn].zName;
  }else if( pTab->iPKey>=0 ){
    assert( pTab->iPKey<pTab->nCol );
    zCol = pTab->aCol[pTab->iPKey].zName;
  }else{
    zCol = "ROWID";
  }
  assert( iDb>=0 && iDb<db->nDb );
  zDBase = db->aDb[iDb].zName;
  rc = db->xAuth(db->pAuthArg, SQLITE_READ, pTab->zName, zCol, zDBase, 
                 pParse->zAuthContext);
  if( rc==SQLITE_IGNORE ){
    pExpr->op = TK_NULL;
  }else if( rc==SQLITE_DENY ){
    if( db->nDb>2 || iDb!=0 ){
      sqlite3ErrorMsg(pParse, "access to %s.%s.%s is prohibited", 
         zDBase, pTab->zName, zCol);
    }else{
      sqlite3ErrorMsg(pParse, "access to %s.%s is prohibited",pTab->zName,zCol);
    }
    pParse->rc = SQLITE_AUTH;
  }else if( rc!=SQLITE_OK ){
    sqliteAuthBadReturnCode(pParse, rc);
  }
}







int sqlite3AuthCheck(
  Parse *pParse,
  int code,
  const char *zArg1,
  const char *zArg2,
  const char *zArg3
){
  sqlite3 *db = pParse->db;
  int rc;

  
  if( db->init.busy ){
    return SQLITE_OK;
  }

  if( db->xAuth==0 ){
    return SQLITE_OK;
  }
  rc = db->xAuth(db->pAuthArg, code, zArg1, zArg2, zArg3, pParse->zAuthContext);
  if( rc==SQLITE_DENY ){
    sqlite3ErrorMsg(pParse, "not authorized");
    pParse->rc = SQLITE_AUTH;
  }else if( rc!=SQLITE_OK && rc!=SQLITE_IGNORE ){
    rc = SQLITE_DENY;
    sqliteAuthBadReturnCode(pParse, rc);
  }
  return rc;
}






void sqlite3AuthContextPush(
  Parse *pParse,
  AuthContext *pContext, 
  const char *zContext
){
  pContext->pParse = pParse;
  if( pParse ){
    pContext->zAuthContext = pParse->zAuthContext;
    pParse->zAuthContext = zContext;
  }
}





void sqlite3AuthContextPop(AuthContext *pContext){
  if( pContext->pParse ){
    pContext->pParse->zAuthContext = pContext->zAuthContext;
    pContext->pParse = 0;
  }
}

#endif 
