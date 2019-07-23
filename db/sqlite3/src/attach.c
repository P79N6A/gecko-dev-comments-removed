














#include "sqliteInt.h"



















static int resolveAttachExpr(NameContext *pName, Expr *pExpr)
{
  int rc = SQLITE_OK;
  if( pExpr ){
    if( pExpr->op!=TK_ID ){
      rc = sqlite3ExprResolveNames(pName, pExpr);
    }else{
      pExpr->op = TK_STRING;
    }
  }
  return rc;
}












static void attachFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  int i;
  int rc = 0;
  sqlite3 *db = sqlite3_user_data(context);
  const char *zName;
  const char *zFile;
  Db *aNew;
  char zErr[128];
  char *zErrDyn = 0;

  zFile = (const char *)sqlite3_value_text(argv[0]);
  zName = (const char *)sqlite3_value_text(argv[1]);

  





  if( db->nDb>=MAX_ATTACHED+2 ){
    sqlite3_snprintf(
      127, zErr, "too many attached databases - max %d", MAX_ATTACHED
    );
    goto attach_error;
  }
  if( !db->autoCommit ){
    strcpy(zErr, "cannot ATTACH database within transaction");
    goto attach_error;
  }
  for(i=0; i<db->nDb; i++){
    char *z = db->aDb[i].zName;
    if( z && sqlite3StrICmp(z, zName)==0 ){
      sqlite3_snprintf(127, zErr, "database %s is already in use", zName);
      goto attach_error;
    }
  }

  


  if( db->aDb==db->aDbStatic ){
    aNew = sqliteMalloc( sizeof(db->aDb[0])*3 );
    if( aNew==0 ){
      return;
    }
    memcpy(aNew, db->aDb, sizeof(db->aDb[0])*2);
  }else{
    aNew = sqliteRealloc(db->aDb, sizeof(db->aDb[0])*(db->nDb+1) );
    if( aNew==0 ){
      return;
    } 
  }
  db->aDb = aNew;
  aNew = &db->aDb[db->nDb++];
  memset(aNew, 0, sizeof(*aNew));

  



  rc = sqlite3BtreeFactory(db, zFile, 0, MAX_PAGES, &aNew->pBt);
  if( rc==SQLITE_OK ){
    aNew->pSchema = sqlite3SchemaGet(aNew->pBt);
    if( !aNew->pSchema ){
      rc = SQLITE_NOMEM;
    }else if( aNew->pSchema->file_format && aNew->pSchema->enc!=ENC(db) ){
      strcpy(zErr, 
        "attached databases must use the same text encoding as main database");
      goto attach_error;
    }
  }
  aNew->zName = sqliteStrDup(zName);
  aNew->safety_level = 3;

#if SQLITE_HAS_CODEC
  {
    extern int sqlite3CodecAttach(sqlite3*, int, void*, int);
    extern void sqlite3CodecGetKey(sqlite3*, int, void**, int*);
    int nKey;
    char *zKey;
    int t = sqlite3_value_type(argv[2]);
    switch( t ){
      case SQLITE_INTEGER:
      case SQLITE_FLOAT:
        zErrDyn = sqliteStrDup("Invalid key value");
        rc = SQLITE_ERROR;
        break;
        
      case SQLITE_TEXT:
      case SQLITE_BLOB:
        nKey = sqlite3_value_bytes(argv[2]);
        zKey = (char *)sqlite3_value_blob(argv[2]);
        sqlite3CodecAttach(db, db->nDb-1, zKey, nKey);
        break;

      case SQLITE_NULL:
        
        sqlite3CodecGetKey(db, 0, (void**)&zKey, &nKey);
        sqlite3CodecAttach(db, db->nDb-1, zKey, nKey);
        break;
    }
  }
#endif

  




  if( rc==SQLITE_OK ){
    sqlite3SafetyOn(db);
    rc = sqlite3Init(db, &zErrDyn);
    sqlite3SafetyOff(db);
  }
  if( rc ){
    int iDb = db->nDb - 1;
    assert( iDb>=2 );
    if( db->aDb[iDb].pBt ){
      sqlite3BtreeClose(db->aDb[iDb].pBt);
      db->aDb[iDb].pBt = 0;
      db->aDb[iDb].pSchema = 0;
    }
    sqlite3ResetInternalSchema(db, 0);
    db->nDb = iDb;
    if( rc==SQLITE_NOMEM ){
      if( !sqlite3MallocFailed() ) sqlite3FailedMalloc();
      sqlite3_snprintf(127, zErr, "out of memory");
    }else{
      sqlite3_snprintf(127, zErr, "unable to open database: %s", zFile);
    }
    goto attach_error;
  }
  
  return;

attach_error:
  
  if( zErrDyn ){
    sqlite3_result_error(context, zErrDyn, -1);
    sqliteFree(zErrDyn);
  }else{
    zErr[sizeof(zErr)-1] = 0;
    sqlite3_result_error(context, zErr, -1);
  }
}









static void detachFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const char *zName = (const char *)sqlite3_value_text(argv[0]);
  sqlite3 *db = sqlite3_user_data(context);
  int i;
  Db *pDb = 0;
  char zErr[128];

  assert(zName);
  for(i=0; i<db->nDb; i++){
    pDb = &db->aDb[i];
    if( pDb->pBt==0 ) continue;
    if( sqlite3StrICmp(pDb->zName, zName)==0 ) break;
  }

  if( i>=db->nDb ){
    sqlite3_snprintf(sizeof(zErr), zErr, "no such database: %s", zName);
    goto detach_error;
  }
  if( i<2 ){
    sqlite3_snprintf(sizeof(zErr), zErr, "cannot detach database %s", zName);
    goto detach_error;
  }
  if( !db->autoCommit ){
    strcpy(zErr, "cannot DETACH database within transaction");
    goto detach_error;
  }

  sqlite3BtreeClose(pDb->pBt);
  pDb->pBt = 0;
  pDb->pSchema = 0;
  sqlite3ResetInternalSchema(db, 0);
  return;

detach_error:
  sqlite3_result_error(context, zErr, -1);
}





static void codeAttach(
  Parse *pParse,       
  int type,            
  const char *zFunc,   
  int nFunc,           
  Expr *pAuthArg,      
  Expr *pFilename,     
  Expr *pDbname,       
  Expr *pKey           
){
  int rc;
  NameContext sName;
  Vdbe *v;
  FuncDef *pFunc;
  sqlite3* db = pParse->db;

#ifndef SQLITE_OMIT_AUTHORIZATION
  assert( sqlite3MallocFailed() || pAuthArg );
  if( pAuthArg ){
    char *zAuthArg = sqlite3NameFromToken(&pAuthArg->span);
    if( !zAuthArg ){
      goto attach_end;
    }
    rc = sqlite3AuthCheck(pParse, type, zAuthArg, 0, 0);
    sqliteFree(zAuthArg);
    if(rc!=SQLITE_OK ){
      goto attach_end;
    }
  }
#endif 

  memset(&sName, 0, sizeof(NameContext));
  sName.pParse = pParse;

  if( 
      SQLITE_OK!=(rc = resolveAttachExpr(&sName, pFilename)) ||
      SQLITE_OK!=(rc = resolveAttachExpr(&sName, pDbname)) ||
      SQLITE_OK!=(rc = resolveAttachExpr(&sName, pKey))
  ){
    pParse->nErr++;
    goto attach_end;
  }

  v = sqlite3GetVdbe(pParse);
  sqlite3ExprCode(pParse, pFilename);
  sqlite3ExprCode(pParse, pDbname);
  sqlite3ExprCode(pParse, pKey);

  assert( v || sqlite3MallocFailed() );
  if( v ){
    sqlite3VdbeAddOp(v, OP_Function, 0, nFunc);
    pFunc = sqlite3FindFunction(db, zFunc, strlen(zFunc), nFunc, SQLITE_UTF8,0);
    sqlite3VdbeChangeP3(v, -1, (char *)pFunc, P3_FUNCDEF);

    



    sqlite3VdbeAddOp(v, OP_Expire, (type==SQLITE_ATTACH), 0);
  }
  
attach_end:
  sqlite3ExprDelete(pFilename);
  sqlite3ExprDelete(pDbname);
  sqlite3ExprDelete(pKey);
}






void sqlite3Detach(Parse *pParse, Expr *pDbname){
  codeAttach(pParse, SQLITE_DETACH, "sqlite_detach", 1, pDbname, 0, 0, pDbname);
}






void sqlite3Attach(Parse *pParse, Expr *p, Expr *pDbname, Expr *pKey){
  codeAttach(pParse, SQLITE_ATTACH, "sqlite_attach", 3, p, p, pDbname, pKey);
}




void sqlite3AttachFunctions(sqlite3 *db){
  static const int enc = SQLITE_UTF8;
  sqlite3CreateFunc(db, "sqlite_attach", 3, enc, db, attachFunc, 0, 0);
  sqlite3CreateFunc(db, "sqlite_detach", 1, enc, db, detachFunc, 0, 0);
}








int sqlite3FixInit(
  DbFixer *pFix,      
  Parse *pParse,      
  int iDb,            
  const char *zType,  
  const Token *pName  
){
  sqlite3 *db;

  if( iDb<0 || iDb==1 ) return 0;
  db = pParse->db;
  assert( db->nDb>iDb );
  pFix->pParse = pParse;
  pFix->zDb = db->aDb[iDb].zName;
  pFix->zType = zType;
  pFix->pName = pName;
  return 1;
}















int sqlite3FixSrcList(
  DbFixer *pFix,       
  SrcList *pList       
){
  int i;
  const char *zDb;
  struct SrcList_item *pItem;

  if( pList==0 ) return 0;
  zDb = pFix->zDb;
  for(i=0, pItem=pList->a; i<pList->nSrc; i++, pItem++){
    if( pItem->zDatabase==0 ){
      pItem->zDatabase = sqliteStrDup(zDb);
    }else if( sqlite3StrICmp(pItem->zDatabase,zDb)!=0 ){
      sqlite3ErrorMsg(pFix->pParse,
         "%s %T cannot reference objects in database %s",
         pFix->zType, pFix->pName, pItem->zDatabase);
      return 1;
    }
#if !defined(SQLITE_OMIT_VIEW) || !defined(SQLITE_OMIT_TRIGGER)
    if( sqlite3FixSelect(pFix, pItem->pSelect) ) return 1;
    if( sqlite3FixExpr(pFix, pItem->pOn) ) return 1;
#endif
  }
  return 0;
}
#if !defined(SQLITE_OMIT_VIEW) || !defined(SQLITE_OMIT_TRIGGER)
int sqlite3FixSelect(
  DbFixer *pFix,       
  Select *pSelect      
){
  while( pSelect ){
    if( sqlite3FixExprList(pFix, pSelect->pEList) ){
      return 1;
    }
    if( sqlite3FixSrcList(pFix, pSelect->pSrc) ){
      return 1;
    }
    if( sqlite3FixExpr(pFix, pSelect->pWhere) ){
      return 1;
    }
    if( sqlite3FixExpr(pFix, pSelect->pHaving) ){
      return 1;
    }
    pSelect = pSelect->pPrior;
  }
  return 0;
}
int sqlite3FixExpr(
  DbFixer *pFix,     
  Expr *pExpr        
){
  while( pExpr ){
    if( sqlite3FixSelect(pFix, pExpr->pSelect) ){
      return 1;
    }
    if( sqlite3FixExprList(pFix, pExpr->pList) ){
      return 1;
    }
    if( sqlite3FixExpr(pFix, pExpr->pRight) ){
      return 1;
    }
    pExpr = pExpr->pLeft;
  }
  return 0;
}
int sqlite3FixExprList(
  DbFixer *pFix,     
  ExprList *pList    
){
  int i;
  struct ExprList_item *pItem;
  if( pList==0 ) return 0;
  for(i=0, pItem=pList->a; i<pList->nExpr; i++, pItem++){
    if( sqlite3FixExpr(pFix, pItem->pExpr) ){
      return 1;
    }
  }
  return 0;
}
#endif

#ifndef SQLITE_OMIT_TRIGGER
int sqlite3FixTriggerStep(
  DbFixer *pFix,     
  TriggerStep *pStep 
){
  while( pStep ){
    if( sqlite3FixSelect(pFix, pStep->pSelect) ){
      return 1;
    }
    if( sqlite3FixExpr(pFix, pStep->pWhere) ){
      return 1;
    }
    if( sqlite3FixExprList(pFix, pStep->pExprList) ){
      return 1;
    }
    pStep = pStep->pNext;
  }
  return 0;
}
#endif
