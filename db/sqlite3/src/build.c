

























#include "sqliteInt.h"
#include <ctype.h>

#include "pager.h"
#include "btree.h"





void sqlite3BeginParse(Parse *pParse, int explainFlag){
  pParse->explain = explainFlag;
  pParse->nVar = 0;
}

#ifndef SQLITE_OMIT_SHARED_CACHE




struct TableLock {
  int iDb;             
  int iTab;            
  u8 isWriteLock;      
  const char *zName;   
};











void sqlite3TableLock(
  Parse *pParse,     
  int iDb,           
  int iTab,          
  u8 isWriteLock,    
  const char *zName  
){
  int i;
  int nBytes;
  TableLock *p;

  if( 0==sqlite3ThreadDataReadOnly()->useSharedData || iDb<0 ){
    return;
  }

  for(i=0; i<pParse->nTableLock; i++){
    p = &pParse->aTableLock[i];
    if( p->iDb==iDb && p->iTab==iTab ){
      p->isWriteLock = (p->isWriteLock || isWriteLock);
      return;
    }
  }

  nBytes = sizeof(TableLock) * (pParse->nTableLock+1);
  sqliteReallocOrFree((void **)&pParse->aTableLock, nBytes);
  if( pParse->aTableLock ){
    p = &pParse->aTableLock[pParse->nTableLock++];
    p->iDb = iDb;
    p->iTab = iTab;
    p->isWriteLock = isWriteLock;
    p->zName = zName;
  }
}





static void codeTableLocks(Parse *pParse){
  int i;
  Vdbe *pVdbe; 
  assert( sqlite3ThreadDataReadOnly()->useSharedData || pParse->nTableLock==0 );

  if( 0==(pVdbe = sqlite3GetVdbe(pParse)) ){
    return;
  }

  for(i=0; i<pParse->nTableLock; i++){
    TableLock *p = &pParse->aTableLock[i];
    int p1 = p->iDb;
    if( p->isWriteLock ){
      p1 = -1*(p1+1);
    }
    sqlite3VdbeOp3(pVdbe, OP_TableLock, p1, p->iTab, p->zName, P3_STATIC);
  }
}
#else
  #define codeTableLocks(x)
#endif











void sqlite3FinishCoding(Parse *pParse){
  sqlite3 *db;
  Vdbe *v;

  if( sqlite3MallocFailed() ) return;
  if( pParse->nested ) return;
  if( !pParse->pVdbe ){
    if( pParse->rc==SQLITE_OK && pParse->nErr ){
      pParse->rc = SQLITE_ERROR;
      return;
    }
  }

  


  db = pParse->db;
  v = sqlite3GetVdbe(pParse);
  if( v ){
    sqlite3VdbeAddOp(v, OP_Halt, 0, 0);

    





    if( pParse->cookieGoto>0 ){
      u32 mask;
      int iDb;
      sqlite3VdbeJumpHere(v, pParse->cookieGoto-1);
      for(iDb=0, mask=1; iDb<db->nDb; mask<<=1, iDb++){
        if( (mask & pParse->cookieMask)==0 ) continue;
        sqlite3VdbeAddOp(v, OP_Transaction, iDb, (mask & pParse->writeMask)!=0);
        sqlite3VdbeAddOp(v, OP_VerifyCookie, iDb, pParse->cookieValue[iDb]);
      }

      



      codeTableLocks(pParse);
      sqlite3VdbeAddOp(v, OP_Goto, 0, pParse->cookieGoto);
    }

#ifndef SQLITE_OMIT_TRACE
    





    sqlite3VdbeOp3(v, OP_Noop, 0, 0, pParse->zSql, pParse->zTail-pParse->zSql);
#endif 
  }


  

  if( v && pParse->nErr==0 && !sqlite3MallocFailed() ){
    FILE *trace = (db->flags & SQLITE_VdbeTrace)!=0 ? stdout : 0;
    sqlite3VdbeTrace(v, trace);
    sqlite3VdbeMakeReady(v, pParse->nVar, pParse->nMem+3,
                         pParse->nTab+3, pParse->explain);
    pParse->rc = SQLITE_DONE;
    pParse->colNamesSet = 0;
  }else if( pParse->rc==SQLITE_OK ){
    pParse->rc = SQLITE_ERROR;
  }
  pParse->nTab = 0;
  pParse->nMem = 0;
  pParse->nSet = 0;
  pParse->nVar = 0;
  pParse->cookieMask = 0;
  pParse->cookieGoto = 0;
}













void sqlite3NestedParse(Parse *pParse, const char *zFormat, ...){
  va_list ap;
  char *zSql;
# define SAVE_SZ  (sizeof(Parse) - offsetof(Parse,nVar))
  char saveBuf[SAVE_SZ];

  if( pParse->nErr ) return;
  assert( pParse->nested<10 );  
  va_start(ap, zFormat);
  zSql = sqlite3VMPrintf(zFormat, ap);
  va_end(ap);
  if( zSql==0 ){
    return;   
  }
  pParse->nested++;
  memcpy(saveBuf, &pParse->nVar, SAVE_SZ);
  memset(&pParse->nVar, 0, SAVE_SZ);
  sqlite3RunParser(pParse, zSql, 0);
  sqliteFree(zSql);
  memcpy(&pParse->nVar, saveBuf, SAVE_SZ);
  pParse->nested--;
}













Table *sqlite3FindTable(sqlite3 *db, const char *zName, const char *zDatabase){
  Table *p = 0;
  int i;
  assert( zName!=0 );
  for(i=OMIT_TEMPDB; i<db->nDb; i++){
    int j = (i<2) ? i^1 : i;   
    if( zDatabase!=0 && sqlite3StrICmp(zDatabase, db->aDb[j].zName) ) continue;
    p = sqlite3HashFind(&db->aDb[j].pSchema->tblHash, zName, strlen(zName)+1);
    if( p ) break;
  }
  return p;
}











Table *sqlite3LocateTable(Parse *pParse, const char *zName, const char *zDbase){
  Table *p;

  

  if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
    return 0;
  }

  p = sqlite3FindTable(pParse->db, zName, zDbase);
  if( p==0 ){
    if( zDbase ){
      sqlite3ErrorMsg(pParse, "no such table: %s.%s", zDbase, zName);
    }else{
      sqlite3ErrorMsg(pParse, "no such table: %s", zName);
    }
    pParse->checkSchema = 1;
  }
  return p;
}













Index *sqlite3FindIndex(sqlite3 *db, const char *zName, const char *zDb){
  Index *p = 0;
  int i;
  for(i=OMIT_TEMPDB; i<db->nDb; i++){
    int j = (i<2) ? i^1 : i;  
    Schema *pSchema = db->aDb[j].pSchema;
    if( zDb && sqlite3StrICmp(zDb, db->aDb[j].zName) ) continue;
    assert( pSchema || (j==1 && !db->aDb[1].pBt) );
    if( pSchema ){
      p = sqlite3HashFind(&pSchema->idxHash, zName, strlen(zName)+1);
    }
    if( p ) break;
  }
  return p;
}




static void freeIndex(Index *p){
  sqliteFree(p->zColAff);
  sqliteFree(p);
}









static void sqliteDeleteIndex(Index *p){
  Index *pOld;
  const char *zName = p->zName;

  pOld = sqlite3HashInsert(&p->pSchema->idxHash, zName, strlen( zName)+1, 0);
  assert( pOld==0 || pOld==p );
  freeIndex(p);
}







void sqlite3UnlinkAndDeleteIndex(sqlite3 *db, int iDb, const char *zIdxName){
  Index *pIndex;
  int len;
  Hash *pHash = &db->aDb[iDb].pSchema->idxHash;

  len = strlen(zIdxName);
  pIndex = sqlite3HashInsert(pHash, zIdxName, len+1, 0);
  if( pIndex ){
    if( pIndex->pTable->pIndex==pIndex ){
      pIndex->pTable->pIndex = pIndex->pNext;
    }else{
      Index *p;
      for(p=pIndex->pTable->pIndex; p && p->pNext!=pIndex; p=p->pNext){}
      if( p && p->pNext==pIndex ){
        p->pNext = pIndex->pNext;
      }
    }
    freeIndex(pIndex);
  }
  db->flags |= SQLITE_InternChanges;
}












void sqlite3ResetInternalSchema(sqlite3 *db, int iDb){
  int i, j;

  assert( iDb>=0 && iDb<db->nDb );
  for(i=iDb; i<db->nDb; i++){
    Db *pDb = &db->aDb[i];
    if( pDb->pSchema ){
      sqlite3SchemaFree(pDb->pSchema);
    }
    if( iDb>0 ) return;
  }
  assert( iDb==0 );
  db->flags &= ~SQLITE_InternChanges;

  





  for(i=0; i<db->nDb; i++){
    struct Db *pDb = &db->aDb[i];
    if( pDb->pBt==0 ){
      if( pDb->pAux && pDb->xFreeAux ) pDb->xFreeAux(pDb->pAux);
      pDb->pAux = 0;
    }
  }
  for(i=j=2; i<db->nDb; i++){
    struct Db *pDb = &db->aDb[i];
    if( pDb->pBt==0 ){
      sqliteFree(pDb->zName);
      pDb->zName = 0;
      continue;
    }
    if( j<i ){
      db->aDb[j] = db->aDb[i];
    }
    j++;
  }
  memset(&db->aDb[j], 0, (db->nDb-j)*sizeof(db->aDb[j]));
  db->nDb = j;
  if( db->nDb<=2 && db->aDb!=db->aDbStatic ){
    memcpy(db->aDbStatic, db->aDb, 2*sizeof(db->aDb[0]));
    sqliteFree(db->aDb);
    db->aDb = db->aDbStatic;
  }
}






void sqlite3RollbackInternalChanges(sqlite3 *db){
  if( db->flags & SQLITE_InternChanges ){
    sqlite3ResetInternalSchema(db, 0);
  }
}




void sqlite3CommitInternalChanges(sqlite3 *db){
  db->flags &= ~SQLITE_InternChanges;
}




static void sqliteResetColumnNames(Table *pTable){
  int i;
  Column *pCol;
  assert( pTable!=0 );
  if( (pCol = pTable->aCol)!=0 ){
    for(i=0; i<pTable->nCol; i++, pCol++){
      sqliteFree(pCol->zName);
      sqlite3ExprDelete(pCol->pDflt);
      sqliteFree(pCol->zType);
      sqliteFree(pCol->zColl);
    }
    sqliteFree(pTable->aCol);
  }
  pTable->aCol = 0;
  pTable->nCol = 0;
}
















void sqlite3DeleteTable(sqlite3 *db, Table *pTable){
  Index *pIndex, *pNext;
  FKey *pFKey, *pNextFKey;

  db = 0;

  if( pTable==0 ) return;

  
  pTable->nRef--;
  if( pTable->nRef>0 ){
    return;
  }
  assert( pTable->nRef==0 );

  

  for(pIndex = pTable->pIndex; pIndex; pIndex=pNext){
    pNext = pIndex->pNext;
    assert( pIndex->pSchema==pTable->pSchema );
    sqliteDeleteIndex(pIndex);
  }

#ifndef SQLITE_OMIT_FOREIGN_KEY
  


  for(pFKey=pTable->pFKey; pFKey; pFKey=pNextFKey){
    pNextFKey = pFKey->pNextFrom;
    assert( sqlite3HashFind(&pTable->pSchema->aFKey,
                           pFKey->zTo, strlen(pFKey->zTo)+1)!=pFKey );
    sqliteFree(pFKey);
  }
#endif

  

  sqliteResetColumnNames(pTable);
  sqliteFree(pTable->zName);
  sqliteFree(pTable->zColAff);
  sqlite3SelectDelete(pTable->pSelect);
#ifndef SQLITE_OMIT_CHECK
  sqlite3ExprDelete(pTable->pCheck);
#endif
  sqliteFree(pTable);
}





void sqlite3UnlinkAndDeleteTable(sqlite3 *db, int iDb, const char *zTabName){
  Table *p;
  FKey *pF1, *pF2;
  Db *pDb;

  assert( db!=0 );
  assert( iDb>=0 && iDb<db->nDb );
  assert( zTabName && zTabName[0] );
  pDb = &db->aDb[iDb];
  p = sqlite3HashInsert(&pDb->pSchema->tblHash, zTabName, strlen(zTabName)+1,0);
  if( p ){
#ifndef SQLITE_OMIT_FOREIGN_KEY
    for(pF1=p->pFKey; pF1; pF1=pF1->pNextFrom){
      int nTo = strlen(pF1->zTo) + 1;
      pF2 = sqlite3HashFind(&pDb->pSchema->aFKey, pF1->zTo, nTo);
      if( pF2==pF1 ){
        sqlite3HashInsert(&pDb->pSchema->aFKey, pF1->zTo, nTo, pF1->pNextTo);
      }else{
        while( pF2 && pF2->pNextTo!=pF1 ){ pF2=pF2->pNextTo; }
        if( pF2 ){
          pF2->pNextTo = pF1->pNextTo;
        }
      }
    }
#endif
    sqlite3DeleteTable(db, p);
  }
  db->flags |= SQLITE_InternChanges;
}











char *sqlite3NameFromToken(Token *pName){
  char *zName;
  if( pName ){
    zName = sqliteStrNDup((char*)pName->z, pName->n);
    sqlite3Dequote(zName);
  }else{
    zName = 0;
  }
  return zName;
}





void sqlite3OpenMasterTable(Parse *p, int iDb){
  Vdbe *v = sqlite3GetVdbe(p);
  sqlite3TableLock(p, iDb, MASTER_ROOT, 1, SCHEMA_TABLE(iDb));
  sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
  sqlite3VdbeAddOp(v, OP_OpenWrite, 0, MASTER_ROOT);
  sqlite3VdbeAddOp(v, OP_SetNumColumns, 0, 5); 
}







int sqlite3FindDb(sqlite3 *db, Token *pName){
  int i = -1;    
  int n;         
  Db *pDb;       
  char *zName;   

  zName = sqlite3NameFromToken(pName);
  if( zName ){
    n = strlen(zName);
    for(i=(db->nDb-1), pDb=&db->aDb[i]; i>=0; i--, pDb--){
      if( (!OMIT_TEMPDB || i!=1 ) && n==strlen(pDb->zName) && 
          0==sqlite3StrICmp(pDb->zName, zName) ){
        break;
      }
    }
    sqliteFree(zName);
  }
  return i;
}

















int sqlite3TwoPartName(
  Parse *pParse,      
  Token *pName1,      
  Token *pName2,      
  Token **pUnqual     
){
  int iDb;                    
  sqlite3 *db = pParse->db;

  if( pName2 && pName2->n>0 ){
    assert( !db->init.busy );
    *pUnqual = pName2;
    iDb = sqlite3FindDb(db, pName1);
    if( iDb<0 ){
      sqlite3ErrorMsg(pParse, "unknown database %T", pName1);
      pParse->nErr++;
      return -1;
    }
  }else{
    assert( db->init.iDb==0 || db->init.busy );
    iDb = db->init.iDb;
    *pUnqual = pName1;
  }
  return iDb;
}








int sqlite3CheckObjectName(Parse *pParse, const char *zName){
  if( !pParse->db->init.busy && pParse->nested==0 
          && (pParse->db->flags & SQLITE_WriteSchema)==0
          && 0==sqlite3StrNICmp(zName, "sqlite_", 7) ){
    sqlite3ErrorMsg(pParse, "object name reserved for internal use: %s", zName);
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}

















void sqlite3StartTable(
  Parse *pParse,   
  Token *pName1,   
  Token *pName2,   
  int isTemp,      
  int isView,      
  int noErr        
){
  Table *pTable;
  char *zName = 0; 
  sqlite3 *db = pParse->db;
  Vdbe *v;
  int iDb;         
  Token *pName;    

  
















  iDb = sqlite3TwoPartName(pParse, pName1, pName2, &pName);
  if( iDb<0 ) return;
  if( !OMIT_TEMPDB && isTemp && iDb>1 ){
    
    sqlite3ErrorMsg(pParse, "temporary table name must be unqualified");
    return;
  }
  if( !OMIT_TEMPDB && isTemp ) iDb = 1;

  pParse->sNameToken = *pName;
  zName = sqlite3NameFromToken(pName);
  if( zName==0 ) return;
  if( SQLITE_OK!=sqlite3CheckObjectName(pParse, zName) ){
    goto begin_table_error;
  }
  if( db->init.iDb==1 ) isTemp = 1;
#ifndef SQLITE_OMIT_AUTHORIZATION
  assert( (isTemp & 1)==isTemp );
  {
    int code;
    char *zDb = db->aDb[iDb].zName;
    if( sqlite3AuthCheck(pParse, SQLITE_INSERT, SCHEMA_TABLE(isTemp), 0, zDb) ){
      goto begin_table_error;
    }
    if( isView ){
      if( !OMIT_TEMPDB && isTemp ){
        code = SQLITE_CREATE_TEMP_VIEW;
      }else{
        code = SQLITE_CREATE_VIEW;
      }
    }else{
      if( !OMIT_TEMPDB && isTemp ){
        code = SQLITE_CREATE_TEMP_TABLE;
      }else{
        code = SQLITE_CREATE_TABLE;
      }
    }
    if( sqlite3AuthCheck(pParse, code, zName, 0, zDb) ){
      goto begin_table_error;
    }
  }
#endif

  



  if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
    goto begin_table_error;
  }
  pTable = sqlite3FindTable(db, zName, db->aDb[iDb].zName);
  if( pTable ){
    if( !noErr ){
      sqlite3ErrorMsg(pParse, "table %T already exists", pName);
    }
    goto begin_table_error;
  }
  if( sqlite3FindIndex(db, zName, 0)!=0 && (iDb==0 || !db->init.busy) ){
    sqlite3ErrorMsg(pParse, "there is already an index named %s", zName);
    goto begin_table_error;
  }
  pTable = sqliteMalloc( sizeof(Table) );
  if( pTable==0 ){
    pParse->rc = SQLITE_NOMEM;
    pParse->nErr++;
    goto begin_table_error;
  }
  pTable->zName = zName;
  pTable->nCol = 0;
  pTable->aCol = 0;
  pTable->iPKey = -1;
  pTable->pIndex = 0;
  pTable->pSchema = db->aDb[iDb].pSchema;
  pTable->nRef = 1;
  if( pParse->pNewTable ) sqlite3DeleteTable(db, pParse->pNewTable);
  pParse->pNewTable = pTable;

  



#ifndef SQLITE_OMIT_AUTOINCREMENT
  if( !pParse->nested && strcmp(zName, "sqlite_sequence")==0 ){
    pTable->pSchema->pSeqTab = pTable;
  }
#endif

  







  if( !db->init.busy && (v = sqlite3GetVdbe(pParse))!=0 ){
    int lbl;
    int fileFormat;
    sqlite3BeginWriteOperation(pParse, 0, iDb);

    


    sqlite3VdbeAddOp(v, OP_ReadCookie, iDb, 1);   
    lbl = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_If, 0, lbl);
    fileFormat = (db->flags & SQLITE_LegacyFileFmt)!=0 ?
                  1 : SQLITE_DEFAULT_FILE_FORMAT;
    sqlite3VdbeAddOp(v, OP_Integer, fileFormat, 0);
    sqlite3VdbeAddOp(v, OP_SetCookie, iDb, 1);
    sqlite3VdbeAddOp(v, OP_Integer, ENC(db), 0);
    sqlite3VdbeAddOp(v, OP_SetCookie, iDb, 4);
    sqlite3VdbeResolveLabel(v, lbl);

    







#ifndef SQLITE_OMIT_VIEW
    if( isView ){
      sqlite3VdbeAddOp(v, OP_Integer, 0, 0);
    }else
#endif
    {
      sqlite3VdbeAddOp(v, OP_CreateTable, iDb, 0);
    }
    sqlite3OpenMasterTable(pParse, iDb);
    sqlite3VdbeAddOp(v, OP_NewRowid, 0, 0);
    sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
    sqlite3VdbeAddOp(v, OP_Null, 0, 0);
    sqlite3VdbeAddOp(v, OP_Insert, 0, 0);
    sqlite3VdbeAddOp(v, OP_Close, 0, 0);
    sqlite3VdbeAddOp(v, OP_Pull, 1, 0);
  }

  
  return;

  
begin_table_error:
  sqliteFree(zName);
  return;
}









#define STRICMP(x, y) (\
sqlite3UpperToLower[*(unsigned char *)(x)]==   \
sqlite3UpperToLower[*(unsigned char *)(y)]     \
&& sqlite3StrICmp((x)+1,(y)+1)==0 )









void sqlite3AddColumn(Parse *pParse, Token *pName){
  Table *p;
  int i;
  char *z;
  Column *pCol;
  if( (p = pParse->pNewTable)==0 ) return;
  z = sqlite3NameFromToken(pName);
  if( z==0 ) return;
  for(i=0; i<p->nCol; i++){
    if( STRICMP(z, p->aCol[i].zName) ){
      sqlite3ErrorMsg(pParse, "duplicate column name: %s", z);
      sqliteFree(z);
      return;
    }
  }
  if( (p->nCol & 0x7)==0 ){
    Column *aNew;
    aNew = sqliteRealloc( p->aCol, (p->nCol+8)*sizeof(p->aCol[0]));
    if( aNew==0 ){
      sqliteFree(z);
      return;
    }
    p->aCol = aNew;
  }
  pCol = &p->aCol[p->nCol];
  memset(pCol, 0, sizeof(p->aCol[0]));
  pCol->zName = z;
 
  



  pCol->affinity = SQLITE_AFF_NONE;
  p->nCol++;
}







void sqlite3AddNotNull(Parse *pParse, int onError){
  Table *p;
  int i;
  if( (p = pParse->pNewTable)==0 ) return;
  i = p->nCol-1;
  if( i>=0 ) p->aCol[i].notNull = onError;
}


























char sqlite3AffinityType(const Token *pType){
  u32 h = 0;
  char aff = SQLITE_AFF_NUMERIC;
  const unsigned char *zIn = pType->z;
  const unsigned char *zEnd = &pType->z[pType->n];

  while( zIn!=zEnd ){
    h = (h<<8) + sqlite3UpperToLower[*zIn];
    zIn++;
    if( h==(('c'<<24)+('h'<<16)+('a'<<8)+'r') ){             
      aff = SQLITE_AFF_TEXT; 
    }else if( h==(('c'<<24)+('l'<<16)+('o'<<8)+'b') ){       
      aff = SQLITE_AFF_TEXT;
    }else if( h==(('t'<<24)+('e'<<16)+('x'<<8)+'t') ){       
      aff = SQLITE_AFF_TEXT;
    }else if( h==(('b'<<24)+('l'<<16)+('o'<<8)+'b')          
        && (aff==SQLITE_AFF_NUMERIC || aff==SQLITE_AFF_REAL) ){
      aff = SQLITE_AFF_NONE;
#ifndef SQLITE_OMIT_FLOATING_POINT
    }else if( h==(('r'<<24)+('e'<<16)+('a'<<8)+'l')          
        && aff==SQLITE_AFF_NUMERIC ){
      aff = SQLITE_AFF_REAL;
    }else if( h==(('f'<<24)+('l'<<16)+('o'<<8)+'a')          
        && aff==SQLITE_AFF_NUMERIC ){
      aff = SQLITE_AFF_REAL;
    }else if( h==(('d'<<24)+('o'<<16)+('u'<<8)+'b')          
        && aff==SQLITE_AFF_NUMERIC ){
      aff = SQLITE_AFF_REAL;
#endif
    }else if( (h&0x00FFFFFF)==(('i'<<16)+('n'<<8)+'t') ){    /* INT */
      aff = SQLITE_AFF_INTEGER;
      break;
    }
  }

  return aff;
}









 
void sqlite3AddColumnType(Parse *pParse, Token *pType){
  Table *p;
  int i;
  Column *pCol;

  if( (p = pParse->pNewTable)==0 ) return;
  i = p->nCol-1;
  if( i<0 ) return;
  pCol = &p->aCol[i];
  sqliteFree(pCol->zType);
  pCol->zType = sqlite3NameFromToken(pType);
  pCol->affinity = sqlite3AffinityType(pType);
}











void sqlite3AddDefaultValue(Parse *pParse, Expr *pExpr){
  Table *p;
  Column *pCol;
  if( (p = pParse->pNewTable)!=0 ){
    pCol = &(p->aCol[p->nCol-1]);
    if( !sqlite3ExprIsConstantOrFunction(pExpr) ){
      sqlite3ErrorMsg(pParse, "default value of column [%s] is not constant",
          pCol->zName);
    }else{
      sqlite3ExprDelete(pCol->pDflt);
      pCol->pDflt = sqlite3ExprDup(pExpr);
    }
  }
  sqlite3ExprDelete(pExpr);
}



















void sqlite3AddPrimaryKey(
  Parse *pParse,    
  ExprList *pList,  
  int onError,      
  int autoInc,      
  int sortOrder     
){
  Table *pTab = pParse->pNewTable;
  char *zType = 0;
  int iCol = -1, i;
  if( pTab==0 ) goto primary_key_exit;
  if( pTab->hasPrimKey ){
    sqlite3ErrorMsg(pParse, 
      "table \"%s\" has more than one primary key", pTab->zName);
    goto primary_key_exit;
  }
  pTab->hasPrimKey = 1;
  if( pList==0 ){
    iCol = pTab->nCol - 1;
    pTab->aCol[iCol].isPrimKey = 1;
  }else{
    for(i=0; i<pList->nExpr; i++){
      for(iCol=0; iCol<pTab->nCol; iCol++){
        if( sqlite3StrICmp(pList->a[i].zName, pTab->aCol[iCol].zName)==0 ){
          break;
        }
      }
      if( iCol<pTab->nCol ){
        pTab->aCol[iCol].isPrimKey = 1;
      }
    }
    if( pList->nExpr>1 ) iCol = -1;
  }
  if( iCol>=0 && iCol<pTab->nCol ){
    zType = pTab->aCol[iCol].zType;
  }
  if( zType && sqlite3StrICmp(zType, "INTEGER")==0
        && sortOrder==SQLITE_SO_ASC ){
    pTab->iPKey = iCol;
    pTab->keyConf = onError;
    pTab->autoInc = autoInc;
  }else if( autoInc ){
#ifndef SQLITE_OMIT_AUTOINCREMENT
    sqlite3ErrorMsg(pParse, "AUTOINCREMENT is only allowed on an "
       "INTEGER PRIMARY KEY");
#endif
  }else{
    sqlite3CreateIndex(pParse, 0, 0, 0, pList, onError, 0, 0, sortOrder, 0);
    pList = 0;
  }

primary_key_exit:
  sqlite3ExprListDelete(pList);
  return;
}




void sqlite3AddCheckConstraint(
  Parse *pParse,    
  Expr *pCheckExpr  
){
#ifndef SQLITE_OMIT_CHECK
  Table *pTab = pParse->pNewTable;
  if( pTab ){
    


    pTab->pCheck = sqlite3ExprAnd(pTab->pCheck, sqlite3ExprDup(pCheckExpr));
  }
#endif
  sqlite3ExprDelete(pCheckExpr);
}





void sqlite3AddCollateType(Parse *pParse, const char *zType, int nType){
  Table *p;
  int i;

  if( (p = pParse->pNewTable)==0 ) return;
  i = p->nCol-1;

  if( sqlite3LocateCollSeq(pParse, zType, nType) ){
    Index *pIdx;
    p->aCol[i].zColl = sqliteStrNDup(zType, nType);
  
    



    for(pIdx=p->pIndex; pIdx; pIdx=pIdx->pNext){
      assert( pIdx->nColumn==1 );
      if( pIdx->aiColumn[0]==i ){
        pIdx->azColl[0] = p->aCol[i].zColl;
      }
    }
  }
}















CollSeq *sqlite3LocateCollSeq(Parse *pParse, const char *zName, int nName){
  sqlite3 *db = pParse->db;
  u8 enc = ENC(db);
  u8 initbusy = db->init.busy;
  CollSeq *pColl;

  pColl = sqlite3FindCollSeq(db, enc, zName, nName, initbusy);
  if( !initbusy && (!pColl || !pColl->xCmp) ){
    pColl = sqlite3GetCollSeq(db, pColl, zName, nName);
    if( !pColl ){
      if( nName<0 ){
        nName = strlen(zName);
      }
      sqlite3ErrorMsg(pParse, "no such collation sequence: %.*s", nName, zName);
      pColl = 0;
    }
  }

  return pColl;
}


















void sqlite3ChangeCookie(sqlite3 *db, Vdbe *v, int iDb){
  sqlite3VdbeAddOp(v, OP_Integer, db->aDb[iDb].pSchema->schema_cookie+1, 0);
  sqlite3VdbeAddOp(v, OP_SetCookie, iDb, 0);
}









static int identLength(const char *z){
  int n;
  for(n=0; *z; n++, z++){
    if( *z=='"' ){ n++; }
  }
  return n + 2;
}





static void identPut(char *z, int *pIdx, char *zSignedIdent){
  unsigned char *zIdent = (unsigned char*)zSignedIdent;
  int i, j, needQuote;
  i = *pIdx;
  for(j=0; zIdent[j]; j++){
    if( !isalnum(zIdent[j]) && zIdent[j]!='_' ) break;
  }
  needQuote =  zIdent[j]!=0 || isdigit(zIdent[0])
                  || sqlite3KeywordCode(zIdent, j)!=TK_ID;
  if( needQuote ) z[i++] = '"';
  for(j=0; zIdent[j]; j++){
    z[i++] = zIdent[j];
    if( zIdent[j]=='"' ) z[i++] = '"';
  }
  if( needQuote ) z[i++] = '"';
  z[i] = 0;
  *pIdx = i;
}






static char *createTableStmt(Table *p, int isTemp){
  int i, k, n;
  char *zStmt;
  char *zSep, *zSep2, *zEnd, *z;
  Column *pCol;
  n = 0;
  for(pCol = p->aCol, i=0; i<p->nCol; i++, pCol++){
    n += identLength(pCol->zName);
    z = pCol->zType;
    if( z ){
      n += (strlen(z) + 1);
    }
  }
  n += identLength(p->zName);
  if( n<50 ){
    zSep = "";
    zSep2 = ",";
    zEnd = ")";
  }else{
    zSep = "\n  ";
    zSep2 = ",\n  ";
    zEnd = "\n)";
  }
  n += 35 + 6*p->nCol;
  zStmt = sqliteMallocRaw( n );
  if( zStmt==0 ) return 0;
  strcpy(zStmt, !OMIT_TEMPDB&&isTemp ? "CREATE TEMP TABLE ":"CREATE TABLE ");
  k = strlen(zStmt);
  identPut(zStmt, &k, p->zName);
  zStmt[k++] = '(';
  for(pCol=p->aCol, i=0; i<p->nCol; i++, pCol++){
    strcpy(&zStmt[k], zSep);
    k += strlen(&zStmt[k]);
    zSep = zSep2;
    identPut(zStmt, &k, pCol->zName);
    if( (z = pCol->zType)!=0 ){
      zStmt[k++] = ' ';
      strcpy(&zStmt[k], z);
      k += strlen(z);
    }
  }
  strcpy(&zStmt[k], zEnd);
  return zStmt;
}





















void sqlite3EndTable(
  Parse *pParse,          
  Token *pCons,           
  Token *pEnd,            
  Select *pSelect         
){
  Table *p;
  sqlite3 *db = pParse->db;
  int iDb;

  if( (pEnd==0 && pSelect==0) || pParse->nErr || sqlite3MallocFailed() ) {
    return;
  }
  p = pParse->pNewTable;
  if( p==0 ) return;

  assert( !db->init.busy || !pSelect );

  iDb = sqlite3SchemaToIndex(pParse->db, p->pSchema);

#ifndef SQLITE_OMIT_CHECK
  

  if( p->pCheck ){
    SrcList sSrc;                   
    NameContext sNC;                

    memset(&sNC, 0, sizeof(sNC));
    memset(&sSrc, 0, sizeof(sSrc));
    sSrc.nSrc = 1;
    sSrc.a[0].zName = p->zName;
    sSrc.a[0].pTab = p;
    sSrc.a[0].iCursor = -1;
    sNC.pParse = pParse;
    sNC.pSrcList = &sSrc;
    sNC.isCheck = 1;
    if( sqlite3ExprResolveNames(&sNC, p->pCheck) ){
      return;
    }
  }
#endif 

  





  if( db->init.busy ){
    p->tnum = db->init.newTnum;
  }

  






  if( !db->init.busy ){
    int n;
    Vdbe *v;
    char *zType;    
    char *zType2;   
    char *zStmt;    

    v = sqlite3GetVdbe(pParse);
    if( v==0 ) return;

    sqlite3VdbeAddOp(v, OP_Close, 0, 0);

    



    if( p->pSelect==0 ){
      
      zType = "table";
      zType2 = "TABLE";
#ifndef SQLITE_OMIT_VIEW
    }else{
      
      zType = "view";
      zType2 = "VIEW";
#endif
    }

    












    if( pSelect ){
      Table *pSelTab;
      sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
      sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
      sqlite3VdbeAddOp(v, OP_OpenWrite, 1, 0);
      pParse->nTab = 2;
      sqlite3Select(pParse, pSelect, SRT_Table, 1, 0, 0, 0, 0);
      sqlite3VdbeAddOp(v, OP_Close, 1, 0);
      if( pParse->nErr==0 ){
        pSelTab = sqlite3ResultSetOfSelect(pParse, 0, pSelect);
        if( pSelTab==0 ) return;
        assert( p->aCol==0 );
        p->nCol = pSelTab->nCol;
        p->aCol = pSelTab->aCol;
        pSelTab->nCol = 0;
        pSelTab->aCol = 0;
        sqlite3DeleteTable(0, pSelTab);
      }
    }

    
    if( pSelect ){
      zStmt = createTableStmt(p, p->pSchema==pParse->db->aDb[1].pSchema);
    }else{
      n = pEnd->z - pParse->sNameToken.z + 1;
      zStmt = sqlite3MPrintf("CREATE %s %.*s", zType2, n, pParse->sNameToken.z);
    }

    





    sqlite3NestedParse(pParse,
      "UPDATE %Q.%s "
         "SET type='%s', name=%Q, tbl_name=%Q, rootpage=#0, sql=%Q "
       "WHERE rowid=#1",
      db->aDb[iDb].zName, SCHEMA_TABLE(iDb),
      zType,
      p->zName,
      p->zName,
      zStmt
    );
    sqliteFree(zStmt);
    sqlite3ChangeCookie(db, v, iDb);

#ifndef SQLITE_OMIT_AUTOINCREMENT
    


    if( p->autoInc ){
      Db *pDb = &db->aDb[iDb];
      if( pDb->pSchema->pSeqTab==0 ){
        sqlite3NestedParse(pParse,
          "CREATE TABLE %Q.sqlite_sequence(name,seq)",
          pDb->zName
        );
      }
    }
#endif

    
    sqlite3VdbeOp3(v, OP_ParseSchema, iDb, 0,
        sqlite3MPrintf("tbl_name='%q'",p->zName), P3_DYNAMIC);
  }


  

  if( db->init.busy && pParse->nErr==0 ){
    Table *pOld;
    FKey *pFKey; 
    Schema *pSchema = p->pSchema;
    pOld = sqlite3HashInsert(&pSchema->tblHash, p->zName, strlen(p->zName)+1,p);
    if( pOld ){
      assert( p==pOld );  
      return;
    }
#ifndef SQLITE_OMIT_FOREIGN_KEY
    for(pFKey=p->pFKey; pFKey; pFKey=pFKey->pNextFrom){
      int nTo = strlen(pFKey->zTo) + 1;
      pFKey->pNextTo = sqlite3HashFind(&pSchema->aFKey, pFKey->zTo, nTo);
      sqlite3HashInsert(&pSchema->aFKey, pFKey->zTo, nTo, pFKey);
    }
#endif
    pParse->pNewTable = 0;
    db->nTable++;
    db->flags |= SQLITE_InternChanges;

#ifndef SQLITE_OMIT_ALTERTABLE
    if( !p->pSelect ){
      const char *zName = (const char *)pParse->sNameToken.z;
      int nName;
      assert( !pSelect && pCons && pEnd );
      if( pCons->z==0 ){
        pCons = pEnd;
      }
      nName = (const char *)pCons->z - zName;
      p->addColOffset = 13 + sqlite3utf8CharLen(zName, nName);
    }
#endif
  }
}

#ifndef SQLITE_OMIT_VIEW



void sqlite3CreateView(
  Parse *pParse,     
  Token *pBegin,     
  Token *pName1,     
  Token *pName2,     
  Select *pSelect,   
  int isTemp         
){
  Table *p;
  int n;
  const unsigned char *z;
  Token sEnd;
  DbFixer sFix;
  Token *pName;
  int iDb;

  if( pParse->nVar>0 ){
    sqlite3ErrorMsg(pParse, "parameters are not allowed in views");
    sqlite3SelectDelete(pSelect);
    return;
  }
  sqlite3StartTable(pParse, pName1, pName2, isTemp, 1, 0);
  p = pParse->pNewTable;
  if( p==0 || pParse->nErr ){
    sqlite3SelectDelete(pSelect);
    return;
  }
  sqlite3TwoPartName(pParse, pName1, pName2, &pName);
  iDb = sqlite3SchemaToIndex(pParse->db, p->pSchema);
  if( sqlite3FixInit(&sFix, pParse, iDb, "view", pName)
    && sqlite3FixSelect(&sFix, pSelect)
  ){
    sqlite3SelectDelete(pSelect);
    return;
  }

  




  p->pSelect = sqlite3SelectDup(pSelect);
  sqlite3SelectDelete(pSelect);
  if( sqlite3MallocFailed() ){
    return;
  }
  if( !pParse->db->init.busy ){
    sqlite3ViewGetColumnNames(pParse, p);
  }

  


  sEnd = pParse->sLastToken;
  if( sEnd.z[0]!=0 && sEnd.z[0]!=';' ){
    sEnd.z += sEnd.n;
  }
  sEnd.n = 0;
  n = sEnd.z - pBegin->z;
  z = (const unsigned char*)pBegin->z;
  while( n>0 && (z[n-1]==';' || isspace(z[n-1])) ){ n--; }
  sEnd.z = &z[n-1];
  sEnd.n = 1;

  
  sqlite3EndTable(pParse, 0, &sEnd, 0);
  return;
}
#endif 

#ifndef SQLITE_OMIT_VIEW





int sqlite3ViewGetColumnNames(Parse *pParse, Table *pTable){
  Table *pSelTab;   
  Select *pSel;     
  int nErr = 0;     
  int n;            

  assert( pTable );

  


  if( pTable->nCol>0 ) return 0;

  









  if( pTable->nCol<0 ){
    sqlite3ErrorMsg(pParse, "view %s is circularly defined", pTable->zName);
    return 1;
  }
  assert( pTable->nCol>=0 );

  






  assert( pTable->pSelect );
  pSel = sqlite3SelectDup(pTable->pSelect);
  if( pSel ){
    n = pParse->nTab;
    sqlite3SrcListAssignCursors(pParse, pSel->pSrc);
    pTable->nCol = -1;
    pSelTab = sqlite3ResultSetOfSelect(pParse, 0, pSel);
    pParse->nTab = n;
    if( pSelTab ){
      assert( pTable->aCol==0 );
      pTable->nCol = pSelTab->nCol;
      pTable->aCol = pSelTab->aCol;
      pSelTab->nCol = 0;
      pSelTab->aCol = 0;
      sqlite3DeleteTable(0, pSelTab);
      pTable->pSchema->flags |= DB_UnresetViews;
    }else{
      pTable->nCol = 0;
      nErr++;
    }
    sqlite3SelectDelete(pSel);
  } else {
    nErr++;
  }
  return nErr;  
}
#endif 

#ifndef SQLITE_OMIT_VIEW



static void sqliteViewResetAll(sqlite3 *db, int idx){
  HashElem *i;
  if( !DbHasProperty(db, idx, DB_UnresetViews) ) return;
  for(i=sqliteHashFirst(&db->aDb[idx].pSchema->tblHash); i;i=sqliteHashNext(i)){
    Table *pTab = sqliteHashData(i);
    if( pTab->pSelect ){
      sqliteResetColumnNames(pTab);
    }
  }
  DbClearProperty(db, idx, DB_UnresetViews);
}
#else
# define sqliteViewResetAll(A,B)
#endif 


















#ifndef SQLITE_OMIT_AUTOVACUUM
void sqlite3RootPageMoved(Db *pDb, int iFrom, int iTo){
  HashElem *pElem;
  Hash *pHash;

  pHash = &pDb->pSchema->tblHash;
  for(pElem=sqliteHashFirst(pHash); pElem; pElem=sqliteHashNext(pElem)){
    Table *pTab = sqliteHashData(pElem);
    if( pTab->tnum==iFrom ){
      pTab->tnum = iTo;
    }
  }
  pHash = &pDb->pSchema->idxHash;
  for(pElem=sqliteHashFirst(pHash); pElem; pElem=sqliteHashNext(pElem)){
    Index *pIdx = sqliteHashData(pElem);
    if( pIdx->tnum==iFrom ){
      pIdx->tnum = iTo;
    }
  }
}
#endif






 
static void destroyRootPage(Parse *pParse, int iTable, int iDb){
  Vdbe *v = sqlite3GetVdbe(pParse);
  sqlite3VdbeAddOp(v, OP_Destroy, iTable, iDb);
#ifndef SQLITE_OMIT_AUTOVACUUM
  







  sqlite3NestedParse(pParse, 
     "UPDATE %Q.%s SET rootpage=%d WHERE #0 AND rootpage=#0",
     pParse->db->aDb[iDb].zName, SCHEMA_TABLE(iDb), iTable);
#endif
}







static void destroyTable(Parse *pParse, Table *pTab){
#ifdef SQLITE_OMIT_AUTOVACUUM
  Index *pIdx;
  int iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  destroyRootPage(pParse, pTab->tnum, iDb);
  for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
    destroyRootPage(pParse, pIdx->tnum, iDb);
  }
#else
  















  int iTab = pTab->tnum;
  int iDestroyed = 0;

  while( 1 ){
    Index *pIdx;
    int iLargest = 0;

    if( iDestroyed==0 || iTab<iDestroyed ){
      iLargest = iTab;
    }
    for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
      int iIdx = pIdx->tnum;
      assert( pIdx->pSchema==pTab->pSchema );
      if( (iDestroyed==0 || (iIdx<iDestroyed)) && iIdx>iLargest ){
        iLargest = iIdx;
      }
    }
    if( iLargest==0 ){
      return;
    }else{
      int iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
      destroyRootPage(pParse, iLargest, iDb);
      iDestroyed = iLargest;
    }
  }
#endif
}





void sqlite3DropTable(Parse *pParse, SrcList *pName, int isView, int noErr){
  Table *pTab;
  Vdbe *v;
  sqlite3 *db = pParse->db;
  int iDb;

  if( pParse->nErr || sqlite3MallocFailed() ){
    goto exit_drop_table;
  }
  assert( pName->nSrc==1 );
  pTab = sqlite3LocateTable(pParse, pName->a[0].zName, pName->a[0].zDatabase);

  if( pTab==0 ){
    if( noErr ){
      sqlite3ErrorClear(pParse);
    }
    goto exit_drop_table;
  }
  iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
  assert( iDb>=0 && iDb<db->nDb );
#ifndef SQLITE_OMIT_AUTHORIZATION
  {
    int code;
    const char *zTab = SCHEMA_TABLE(iDb);
    const char *zDb = db->aDb[iDb].zName;
    if( sqlite3AuthCheck(pParse, SQLITE_DELETE, zTab, 0, zDb)){
      goto exit_drop_table;
    }
    if( isView ){
      if( !OMIT_TEMPDB && iDb==1 ){
        code = SQLITE_DROP_TEMP_VIEW;
      }else{
        code = SQLITE_DROP_VIEW;
      }
    }else{
      if( !OMIT_TEMPDB && iDb==1 ){
        code = SQLITE_DROP_TEMP_TABLE;
      }else{
        code = SQLITE_DROP_TABLE;
      }
    }
    if( sqlite3AuthCheck(pParse, code, pTab->zName, 0, zDb) ){
      goto exit_drop_table;
    }
    if( sqlite3AuthCheck(pParse, SQLITE_DELETE, pTab->zName, 0, zDb) ){
      goto exit_drop_table;
    }
  }
#endif
  if( pTab->readOnly || pTab==db->aDb[iDb].pSchema->pSeqTab ){
    sqlite3ErrorMsg(pParse, "table %s may not be dropped", pTab->zName);
    goto exit_drop_table;
  }

#ifndef SQLITE_OMIT_VIEW
  


  if( isView && pTab->pSelect==0 ){
    sqlite3ErrorMsg(pParse, "use DROP TABLE to delete table %s", pTab->zName);
    goto exit_drop_table;
  }
  if( !isView && pTab->pSelect ){
    sqlite3ErrorMsg(pParse, "use DROP VIEW to delete view %s", pTab->zName);
    goto exit_drop_table;
  }
#endif

  


  v = sqlite3GetVdbe(pParse);
  if( v ){
    Trigger *pTrigger;
    Db *pDb = &db->aDb[iDb];
    sqlite3BeginWriteOperation(pParse, 0, iDb);

    



    pTrigger = pTab->pTrigger;
    while( pTrigger ){
      assert( pTrigger->pSchema==pTab->pSchema || 
          pTrigger->pSchema==db->aDb[1].pSchema );
      sqlite3DropTriggerPtr(pParse, pTrigger);
      pTrigger = pTrigger->pNext;
    }

#ifndef SQLITE_OMIT_AUTOINCREMENT
    




    if( pTab->autoInc ){
      sqlite3NestedParse(pParse,
        "DELETE FROM %s.sqlite_sequence WHERE name=%Q",
        pDb->zName, pTab->zName
      );
    }
#endif

    






    sqlite3NestedParse(pParse, 
        "DELETE FROM %Q.%s WHERE tbl_name=%Q and type!='trigger'",
        pDb->zName, SCHEMA_TABLE(iDb), pTab->zName);
    if( !isView ){
      destroyTable(pParse, pTab);
    }

    


    sqlite3VdbeOp3(v, OP_DropTable, iDb, 0, pTab->zName, 0);
    sqlite3ChangeCookie(db, v, iDb);
  }
  sqliteViewResetAll(db, iDb);

exit_drop_table:
  sqlite3SrcListDelete(pName);
}



















void sqlite3CreateForeignKey(
  Parse *pParse,       
  ExprList *pFromCol,  
  Token *pTo,          
  ExprList *pToCol,    
  int flags            
){
#ifndef SQLITE_OMIT_FOREIGN_KEY
  FKey *pFKey = 0;
  Table *p = pParse->pNewTable;
  int nByte;
  int i;
  int nCol;
  char *z;

  assert( pTo!=0 );
  if( p==0 || pParse->nErr ) goto fk_end;
  if( pFromCol==0 ){
    int iCol = p->nCol-1;
    if( iCol<0 ) goto fk_end;
    if( pToCol && pToCol->nExpr!=1 ){
      sqlite3ErrorMsg(pParse, "foreign key on %s"
         " should reference only one column of table %T",
         p->aCol[iCol].zName, pTo);
      goto fk_end;
    }
    nCol = 1;
  }else if( pToCol && pToCol->nExpr!=pFromCol->nExpr ){
    sqlite3ErrorMsg(pParse,
        "number of columns in foreign key does not match the number of "
        "columns in the referenced table");
    goto fk_end;
  }else{
    nCol = pFromCol->nExpr;
  }
  nByte = sizeof(*pFKey) + nCol*sizeof(pFKey->aCol[0]) + pTo->n + 1;
  if( pToCol ){
    for(i=0; i<pToCol->nExpr; i++){
      nByte += strlen(pToCol->a[i].zName) + 1;
    }
  }
  pFKey = sqliteMalloc( nByte );
  if( pFKey==0 ) goto fk_end;
  pFKey->pFrom = p;
  pFKey->pNextFrom = p->pFKey;
  z = (char*)&pFKey[1];
  pFKey->aCol = (struct sColMap*)z;
  z += sizeof(struct sColMap)*nCol;
  pFKey->zTo = z;
  memcpy(z, pTo->z, pTo->n);
  z[pTo->n] = 0;
  z += pTo->n+1;
  pFKey->pNextTo = 0;
  pFKey->nCol = nCol;
  if( pFromCol==0 ){
    pFKey->aCol[0].iFrom = p->nCol-1;
  }else{
    for(i=0; i<nCol; i++){
      int j;
      for(j=0; j<p->nCol; j++){
        if( sqlite3StrICmp(p->aCol[j].zName, pFromCol->a[i].zName)==0 ){
          pFKey->aCol[i].iFrom = j;
          break;
        }
      }
      if( j>=p->nCol ){
        sqlite3ErrorMsg(pParse, 
          "unknown column \"%s\" in foreign key definition", 
          pFromCol->a[i].zName);
        goto fk_end;
      }
    }
  }
  if( pToCol ){
    for(i=0; i<nCol; i++){
      int n = strlen(pToCol->a[i].zName);
      pFKey->aCol[i].zCol = z;
      memcpy(z, pToCol->a[i].zName, n);
      z[n] = 0;
      z += n+1;
    }
  }
  pFKey->isDeferred = 0;
  pFKey->deleteConf = flags & 0xff;
  pFKey->updateConf = (flags >> 8 ) & 0xff;
  pFKey->insertConf = (flags >> 16 ) & 0xff;

  

  p->pFKey = pFKey;
  pFKey = 0;

fk_end:
  sqliteFree(pFKey);
#endif 
  sqlite3ExprListDelete(pFromCol);
  sqlite3ExprListDelete(pToCol);
}








void sqlite3DeferForeignKey(Parse *pParse, int isDeferred){
#ifndef SQLITE_OMIT_FOREIGN_KEY
  Table *pTab;
  FKey *pFKey;
  if( (pTab = pParse->pNewTable)==0 || (pFKey = pTab->pFKey)==0 ) return;
  pFKey->isDeferred = isDeferred;
#endif
}












static void sqlite3RefillIndex(Parse *pParse, Index *pIndex, int memRootPage){
  Table *pTab = pIndex->pTable;  
  int iTab = pParse->nTab;       
  int iIdx = pParse->nTab+1;     
  int addr1;                     
  int tnum;                      
  Vdbe *v;                       
  KeyInfo *pKey;                 
  int iDb = sqlite3SchemaToIndex(pParse->db, pIndex->pSchema);

#ifndef SQLITE_OMIT_AUTHORIZATION
  if( sqlite3AuthCheck(pParse, SQLITE_REINDEX, pIndex->zName, 0,
      pParse->db->aDb[iDb].zName ) ){
    return;
  }
#endif

  
  sqlite3TableLock(pParse, iDb, pTab->tnum, 1, pTab->zName);

  v = sqlite3GetVdbe(pParse);
  if( v==0 ) return;
  if( memRootPage>=0 ){
    sqlite3VdbeAddOp(v, OP_MemLoad, memRootPage, 0);
    tnum = 0;
  }else{
    tnum = pIndex->tnum;
    sqlite3VdbeAddOp(v, OP_Clear, tnum, iDb);
  }
  sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
  pKey = sqlite3IndexKeyinfo(pParse, pIndex);
  sqlite3VdbeOp3(v, OP_OpenWrite, iIdx, tnum, (char *)pKey, P3_KEYINFO_HANDOFF);
  sqlite3OpenTable(pParse, iTab, iDb, pTab, OP_OpenRead);
  addr1 = sqlite3VdbeAddOp(v, OP_Rewind, iTab, 0);
  sqlite3GenerateIndexKey(v, pIndex, iTab);
  if( pIndex->onError!=OE_None ){
    int curaddr = sqlite3VdbeCurrentAddr(v);
    int addr2 = curaddr+4;
    sqlite3VdbeChangeP2(v, curaddr-1, addr2);
    sqlite3VdbeAddOp(v, OP_Rowid, iTab, 0);
    sqlite3VdbeAddOp(v, OP_AddImm, 1, 0);
    sqlite3VdbeAddOp(v, OP_IsUnique, iIdx, addr2);
    sqlite3VdbeOp3(v, OP_Halt, SQLITE_CONSTRAINT, OE_Abort,
                    "indexed columns are not unique", P3_STATIC);
    assert( addr2==sqlite3VdbeCurrentAddr(v) );
  }
  sqlite3VdbeAddOp(v, OP_IdxInsert, iIdx, 0);
  sqlite3VdbeAddOp(v, OP_Next, iTab, addr1+1);
  sqlite3VdbeJumpHere(v, addr1);
  sqlite3VdbeAddOp(v, OP_Close, iTab, 0);
  sqlite3VdbeAddOp(v, OP_Close, iIdx, 0);
}













void sqlite3CreateIndex(
  Parse *pParse,     
  Token *pName1,     
  Token *pName2,     
  SrcList *pTblName, 
  ExprList *pList,   
  int onError,       
  Token *pStart,     
  Token *pEnd,       
  int sortOrder,     
  int ifNotExist     
){
  Table *pTab = 0;     
  Index *pIndex = 0;   
  char *zName = 0;     
  int nName;           
  int i, j;
  Token nullId;        
  DbFixer sFix;        
  int sortOrderMask;   
  sqlite3 *db = pParse->db;
  Db *pDb;             
  int iDb;             
  Token *pName = 0;    
  struct ExprList_item *pListItem; 
  int nCol;
  int nExtra = 0;
  char *zExtra;

  if( pParse->nErr || sqlite3MallocFailed() ){
    goto exit_create_index;
  }

  


  if( pTblName!=0 ){

    



    assert( pName1 && pName2 );
    iDb = sqlite3TwoPartName(pParse, pName1, pName2, &pName);
    if( iDb<0 ) goto exit_create_index;

#ifndef SQLITE_OMIT_TEMPDB
    


    pTab = sqlite3SrcListLookup(pParse, pTblName);
    if( pName2 && pName2->n==0 && pTab && pTab->pSchema==db->aDb[1].pSchema ){
      iDb = 1;
    }
#endif

    if( sqlite3FixInit(&sFix, pParse, iDb, "index", pName) &&
        sqlite3FixSrcList(&sFix, pTblName)
    ){
      

      assert(0);
    }
    pTab = sqlite3LocateTable(pParse, pTblName->a[0].zName, 
        pTblName->a[0].zDatabase);
    if( !pTab ) goto exit_create_index;
    assert( db->aDb[iDb].pSchema==pTab->pSchema );
  }else{
    assert( pName==0 );
    pTab = pParse->pNewTable;
    if( !pTab ) goto exit_create_index;
    iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
  }
  pDb = &db->aDb[iDb];

  if( pTab==0 || pParse->nErr ) goto exit_create_index;
  if( pTab->readOnly ){
    sqlite3ErrorMsg(pParse, "table %s may not be indexed", pTab->zName);
    goto exit_create_index;
  }
#ifndef SQLITE_OMIT_VIEW
  if( pTab->pSelect ){
    sqlite3ErrorMsg(pParse, "views may not be indexed");
    goto exit_create_index;
  }
#endif

  












  if( pName ){
    zName = sqlite3NameFromToken(pName);
    if( SQLITE_OK!=sqlite3ReadSchema(pParse) ) goto exit_create_index;
    if( zName==0 ) goto exit_create_index;
    if( SQLITE_OK!=sqlite3CheckObjectName(pParse, zName) ){
      goto exit_create_index;
    }
    if( !db->init.busy ){
      if( SQLITE_OK!=sqlite3ReadSchema(pParse) ) goto exit_create_index;
      if( sqlite3FindIndex(db, zName, pDb->zName)!=0 ){
        if( !ifNotExist ){
          sqlite3ErrorMsg(pParse, "index %s already exists", zName);
        }
        goto exit_create_index;
      }
      if( sqlite3FindTable(db, zName, 0)!=0 ){
        sqlite3ErrorMsg(pParse, "there is already a table named %s", zName);
        goto exit_create_index;
      }
    }
  }else{
    char zBuf[30];
    int n;
    Index *pLoop;
    for(pLoop=pTab->pIndex, n=1; pLoop; pLoop=pLoop->pNext, n++){}
    sprintf(zBuf,"_%d",n);
    zName = 0;
    sqlite3SetString(&zName, "sqlite_autoindex_", pTab->zName, zBuf, (char*)0);
    if( zName==0 ) goto exit_create_index;
  }

  

#ifndef SQLITE_OMIT_AUTHORIZATION
  {
    const char *zDb = pDb->zName;
    if( sqlite3AuthCheck(pParse, SQLITE_INSERT, SCHEMA_TABLE(iDb), 0, zDb) ){
      goto exit_create_index;
    }
    i = SQLITE_CREATE_INDEX;
    if( !OMIT_TEMPDB && iDb==1 ) i = SQLITE_CREATE_TEMP_INDEX;
    if( sqlite3AuthCheck(pParse, i, zName, pTab->zName, zDb) ){
      goto exit_create_index;
    }
  }
#endif

  



  if( pList==0 ){
    nullId.z = (u8*)pTab->aCol[pTab->nCol-1].zName;
    nullId.n = strlen((char*)nullId.z);
    pList = sqlite3ExprListAppend(0, 0, &nullId);
    if( pList==0 ) goto exit_create_index;
    pList->a[0].sortOrder = sortOrder;
  }

  


  for(i=0; i<pList->nExpr; i++){
    Expr *pExpr = pList->a[i].pExpr;
    if( pExpr ){
      nExtra += (1 + strlen(pExpr->pColl->zName));
    }
  }

  


  nName = strlen(zName);
  nCol = pList->nExpr;
  pIndex = sqliteMalloc( 
      sizeof(Index) +              
      sizeof(int)*nCol +           
      sizeof(int)*(nCol+1) +       
      sizeof(char *)*nCol +        
      sizeof(u8)*nCol +            
      nName + 1 +                  
      nExtra                       
  );
  if( sqlite3MallocFailed() ) goto exit_create_index;
  pIndex->azColl = (char**)(&pIndex[1]);
  pIndex->aiColumn = (int *)(&pIndex->azColl[nCol]);
  pIndex->aiRowEst = (unsigned *)(&pIndex->aiColumn[nCol]);
  pIndex->aSortOrder = (u8 *)(&pIndex->aiRowEst[nCol+1]);
  pIndex->zName = (char *)(&pIndex->aSortOrder[nCol]);
  zExtra = (char *)(&pIndex->zName[nName+1]);
  strcpy(pIndex->zName, zName);
  pIndex->pTable = pTab;
  pIndex->nColumn = pList->nExpr;
  pIndex->onError = onError;
  pIndex->autoIndex = pName==0;
  pIndex->pSchema = db->aDb[iDb].pSchema;

  

  if( pDb->pSchema->file_format>=4 ){
    sortOrderMask = -1;   
  }else{
    sortOrderMask = 0;    
  }

  



  for(i=0, pListItem=pList->a; i<pList->nExpr; i++, pListItem++){
    const char *zColName = pListItem->zName;
    Column *pTabCol;
    int requestedSortOrder;
    char *zColl;                   

    for(j=0, pTabCol=pTab->aCol; j<pTab->nCol; j++, pTabCol++){
      if( sqlite3StrICmp(zColName, pTabCol->zName)==0 ) break;
    }
    if( j>=pTab->nCol ){
      sqlite3ErrorMsg(pParse, "table %s has no column named %s",
        pTab->zName, zColName);
      goto exit_create_index;
    }
    pIndex->aiColumn[i] = j;
    if( pListItem->pExpr ){
      assert( pListItem->pExpr->pColl );
      zColl = zExtra;
      strcpy(zExtra, pListItem->pExpr->pColl->zName);
      zExtra += (strlen(zColl) + 1);
    }else{
      zColl = pTab->aCol[j].zColl;
      if( !zColl ){
        zColl = db->pDfltColl->zName;
      }
    }
    if( !db->init.busy && !sqlite3LocateCollSeq(pParse, zColl, -1) ){
      goto exit_create_index;
    }
    pIndex->azColl[i] = zColl;
    requestedSortOrder = pListItem->sortOrder & sortOrderMask;
    pIndex->aSortOrder[i] = requestedSortOrder;
  }
  sqlite3DefaultRowEst(pIndex);

  if( pTab==pParse->pNewTable ){
    












    Index *pIdx;
    for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
      int k;
      assert( pIdx->onError!=OE_None );
      assert( pIdx->autoIndex );
      assert( pIndex->onError!=OE_None );

      if( pIdx->nColumn!=pIndex->nColumn ) continue;
      for(k=0; k<pIdx->nColumn; k++){
        const char *z1 = pIdx->azColl[k];
        const char *z2 = pIndex->azColl[k];
        if( pIdx->aiColumn[k]!=pIndex->aiColumn[k] ) break;
        if( pIdx->aSortOrder[k]!=pIndex->aSortOrder[k] ) break;
        if( z1!=z2 && sqlite3StrICmp(z1, z2) ) break;
      }
      if( k==pIdx->nColumn ){
        if( pIdx->onError!=pIndex->onError ){
          






          if( !(pIdx->onError==OE_Default || pIndex->onError==OE_Default) ){
            sqlite3ErrorMsg(pParse, 
                "conflicting ON CONFLICT clauses specified", 0);
          }
          if( pIdx->onError==OE_Default ){
            pIdx->onError = pIndex->onError;
          }
        }
        goto exit_create_index;
      }
    }
  }

  


  if( db->init.busy ){
    Index *p;
    p = sqlite3HashInsert(&pIndex->pSchema->idxHash, 
                         pIndex->zName, strlen(pIndex->zName)+1, pIndex);
    if( p ){
      assert( p==pIndex );  
      goto exit_create_index;
    }
    db->flags |= SQLITE_InternChanges;
    if( pTblName!=0 ){
      pIndex->tnum = db->init.newTnum;
    }
  }

  














  else if( db->init.busy==0 ){
    Vdbe *v;
    char *zStmt;
    int iMem = pParse->nMem++;

    v = sqlite3GetVdbe(pParse);
    if( v==0 ) goto exit_create_index;


    

    sqlite3BeginWriteOperation(pParse, 1, iDb);
    sqlite3VdbeAddOp(v, OP_CreateIndex, iDb, 0);
    sqlite3VdbeAddOp(v, OP_MemStore, iMem, 0);

    


    if( pStart && pEnd ){
      
      zStmt = sqlite3MPrintf("CREATE%s INDEX %.*s",
        onError==OE_None ? "" : " UNIQUE",
        pEnd->z - pName->z + 1,
        pName->z);
    }else{
      
      
      zStmt = 0;
    }

    

    sqlite3NestedParse(pParse, 
        "INSERT INTO %Q.%s VALUES('index',%Q,%Q,#0,%Q);",
        db->aDb[iDb].zName, SCHEMA_TABLE(iDb),
        pIndex->zName,
        pTab->zName,
        zStmt
    );
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
    sqliteFree(zStmt);

    


    if( pTblName ){
      sqlite3RefillIndex(pParse, pIndex, iMem);
      sqlite3ChangeCookie(db, v, iDb);
      sqlite3VdbeOp3(v, OP_ParseSchema, iDb, 0,
         sqlite3MPrintf("name='%q'", pIndex->zName), P3_DYNAMIC);
      sqlite3VdbeAddOp(v, OP_Expire, 0, 0);
    }
  }

  




  if( db->init.busy || pTblName==0 ){
    if( onError!=OE_Replace || pTab->pIndex==0
         || pTab->pIndex->onError==OE_Replace){
      pIndex->pNext = pTab->pIndex;
      pTab->pIndex = pIndex;
    }else{
      Index *pOther = pTab->pIndex;
      while( pOther->pNext && pOther->pNext->onError!=OE_Replace ){
        pOther = pOther->pNext;
      }
      pIndex->pNext = pOther->pNext;
      pOther->pNext = pIndex;
    }
    pIndex = 0;
  }

  
exit_create_index:
  if( pIndex ){
    freeIndex(pIndex);
  }
  sqlite3ExprListDelete(pList);
  sqlite3SrcListDelete(pTblName);
  sqliteFree(zName);
  return;
}





void sqlite3MinimumFileFormat(Parse *pParse, int iDb, int minFormat){
  Vdbe *v;
  v = sqlite3GetVdbe(pParse);
  if( v ){
    sqlite3VdbeAddOp(v, OP_ReadCookie, iDb, 1);
    sqlite3VdbeAddOp(v, OP_Integer, minFormat, 0);
    sqlite3VdbeAddOp(v, OP_Ge, 0, sqlite3VdbeCurrentAddr(v)+3);
    sqlite3VdbeAddOp(v, OP_Integer, minFormat, 0);
    sqlite3VdbeAddOp(v, OP_SetCookie, iDb, 1);
  }
}



















void sqlite3DefaultRowEst(Index *pIdx){
  unsigned *a = pIdx->aiRowEst;
  int i;
  assert( a!=0 );
  a[0] = 1000000;
  for(i=pIdx->nColumn; i>=5; i--){
    a[i] = 5;
  }
  while( i>=1 ){
    a[i] = 11 - i;
    i--;
  }
  if( pIdx->onError!=OE_None ){
    a[pIdx->nColumn] = 1;
  }
}





void sqlite3DropIndex(Parse *pParse, SrcList *pName, int ifExists){
  Index *pIndex;
  Vdbe *v;
  sqlite3 *db = pParse->db;
  int iDb;

  if( pParse->nErr || sqlite3MallocFailed() ){
    goto exit_drop_index;
  }
  assert( pName->nSrc==1 );
  if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
    goto exit_drop_index;
  }
  pIndex = sqlite3FindIndex(db, pName->a[0].zName, pName->a[0].zDatabase);
  if( pIndex==0 ){
    if( !ifExists ){
      sqlite3ErrorMsg(pParse, "no such index: %S", pName, 0);
    }
    pParse->checkSchema = 1;
    goto exit_drop_index;
  }
  if( pIndex->autoIndex ){
    sqlite3ErrorMsg(pParse, "index associated with UNIQUE "
      "or PRIMARY KEY constraint cannot be dropped", 0);
    goto exit_drop_index;
  }
  iDb = sqlite3SchemaToIndex(db, pIndex->pSchema);
#ifndef SQLITE_OMIT_AUTHORIZATION
  {
    int code = SQLITE_DROP_INDEX;
    Table *pTab = pIndex->pTable;
    const char *zDb = db->aDb[iDb].zName;
    const char *zTab = SCHEMA_TABLE(iDb);
    if( sqlite3AuthCheck(pParse, SQLITE_DELETE, zTab, 0, zDb) ){
      goto exit_drop_index;
    }
    if( !OMIT_TEMPDB && iDb ) code = SQLITE_DROP_TEMP_INDEX;
    if( sqlite3AuthCheck(pParse, code, pIndex->zName, pTab->zName, zDb) ){
      goto exit_drop_index;
    }
  }
#endif

  
  v = sqlite3GetVdbe(pParse);
  if( v ){
    sqlite3NestedParse(pParse,
       "DELETE FROM %Q.%s WHERE name=%Q",
       db->aDb[iDb].zName, SCHEMA_TABLE(iDb),
       pIndex->zName
    );
    sqlite3ChangeCookie(db, v, iDb);
    destroyRootPage(pParse, pIndex->tnum, iDb);
    sqlite3VdbeOp3(v, OP_DropIndex, iDb, 0, pIndex->zName, 0);
  }

exit_drop_index:
  sqlite3SrcListDelete(pName);
}























int sqlite3ArrayAllocate(void **ppArray, int szEntry, int initSize){
  char *p;
  int *an = (int*)&ppArray[1];
  if( an[0]>=an[1] ){
    void *pNew;
    int newSize;
    newSize = an[1]*2 + initSize;
    pNew = sqliteRealloc(*ppArray, newSize*szEntry);
    if( pNew==0 ){
      return -1;
    }
    an[1] = newSize;
    *ppArray = pNew;
  }
  p = *ppArray;
  memset(&p[an[0]*szEntry], 0, szEntry);
  return an[0]++;
}







IdList *sqlite3IdListAppend(IdList *pList, Token *pToken){
  int i;
  if( pList==0 ){
    pList = sqliteMalloc( sizeof(IdList) );
    if( pList==0 ) return 0;
    pList->nAlloc = 0;
  }
  i = sqlite3ArrayAllocate((void**)&pList->a, sizeof(pList->a[0]), 5);
  if( i<0 ){
    sqlite3IdListDelete(pList);
    return 0;
  }
  pList->a[i].zName = sqlite3NameFromToken(pToken);
  return pList;
}




void sqlite3IdListDelete(IdList *pList){
  int i;
  if( pList==0 ) return;
  for(i=0; i<pList->nId; i++){
    sqliteFree(pList->a[i].zName);
  }
  sqliteFree(pList->a);
  sqliteFree(pList);
}





int sqlite3IdListIndex(IdList *pList, const char *zName){
  int i;
  if( pList==0 ) return -1;
  for(i=0; i<pList->nId; i++){
    if( sqlite3StrICmp(pList->a[i].zName, zName)==0 ) return i;
  }
  return -1;
}


























SrcList *sqlite3SrcListAppend(SrcList *pList, Token *pTable, Token *pDatabase){
  struct SrcList_item *pItem;
  if( pList==0 ){
    pList = sqliteMalloc( sizeof(SrcList) );
    if( pList==0 ) return 0;
    pList->nAlloc = 1;
  }
  if( pList->nSrc>=pList->nAlloc ){
    SrcList *pNew;
    pList->nAlloc *= 2;
    pNew = sqliteRealloc(pList,
               sizeof(*pList) + (pList->nAlloc-1)*sizeof(pList->a[0]) );
    if( pNew==0 ){
      sqlite3SrcListDelete(pList);
      return 0;
    }
    pList = pNew;
  }
  pItem = &pList->a[pList->nSrc];
  memset(pItem, 0, sizeof(pList->a[0]));
  if( pDatabase && pDatabase->z==0 ){
    pDatabase = 0;
  }
  if( pDatabase && pTable ){
    Token *pTemp = pDatabase;
    pDatabase = pTable;
    pTable = pTemp;
  }
  pItem->zName = sqlite3NameFromToken(pTable);
  pItem->zDatabase = sqlite3NameFromToken(pDatabase);
  pItem->iCursor = -1;
  pItem->isPopulated = 0;
  pList->nSrc++;
  return pList;
}




void sqlite3SrcListAssignCursors(Parse *pParse, SrcList *pList){
  int i;
  struct SrcList_item *pItem;
  assert(pList || sqlite3MallocFailed() );
  if( pList ){
    for(i=0, pItem=pList->a; i<pList->nSrc; i++, pItem++){
      if( pItem->iCursor>=0 ) break;
      pItem->iCursor = pParse->nTab++;
      if( pItem->pSelect ){
        sqlite3SrcListAssignCursors(pParse, pItem->pSelect->pSrc);
      }
    }
  }
}




void sqlite3SrcListAddAlias(SrcList *pList, Token *pToken){
  if( pList && pList->nSrc>0 ){
    pList->a[pList->nSrc-1].zAlias = sqlite3NameFromToken(pToken);
  }
}




void sqlite3SrcListDelete(SrcList *pList){
  int i;
  struct SrcList_item *pItem;
  if( pList==0 ) return;
  for(pItem=pList->a, i=0; i<pList->nSrc; i++, pItem++){
    sqliteFree(pItem->zDatabase);
    sqliteFree(pItem->zName);
    sqliteFree(pItem->zAlias);
    sqlite3DeleteTable(0, pItem->pTab);
    sqlite3SelectDelete(pItem->pSelect);
    sqlite3ExprDelete(pItem->pOn);
    sqlite3IdListDelete(pItem->pUsing);
  }
  sqliteFree(pList);
}




void sqlite3BeginTransaction(Parse *pParse, int type){
  sqlite3 *db;
  Vdbe *v;
  int i;

  if( pParse==0 || (db=pParse->db)==0 || db->aDb[0].pBt==0 ) return;
  if( pParse->nErr || sqlite3MallocFailed() ) return;
  if( sqlite3AuthCheck(pParse, SQLITE_TRANSACTION, "BEGIN", 0, 0) ) return;

  v = sqlite3GetVdbe(pParse);
  if( !v ) return;
  if( type!=TK_DEFERRED ){
    for(i=0; i<db->nDb; i++){
      sqlite3VdbeAddOp(v, OP_Transaction, i, (type==TK_EXCLUSIVE)+1);
    }
  }
  sqlite3VdbeAddOp(v, OP_AutoCommit, 0, 0);
}




void sqlite3CommitTransaction(Parse *pParse){
  sqlite3 *db;
  Vdbe *v;

  if( pParse==0 || (db=pParse->db)==0 || db->aDb[0].pBt==0 ) return;
  if( pParse->nErr || sqlite3MallocFailed() ) return;
  if( sqlite3AuthCheck(pParse, SQLITE_TRANSACTION, "COMMIT", 0, 0) ) return;

  v = sqlite3GetVdbe(pParse);
  if( v ){
    sqlite3VdbeAddOp(v, OP_AutoCommit, 1, 0);
  }
}




void sqlite3RollbackTransaction(Parse *pParse){
  sqlite3 *db;
  Vdbe *v;

  if( pParse==0 || (db=pParse->db)==0 || db->aDb[0].pBt==0 ) return;
  if( pParse->nErr || sqlite3MallocFailed() ) return;
  if( sqlite3AuthCheck(pParse, SQLITE_TRANSACTION, "ROLLBACK", 0, 0) ) return;

  v = sqlite3GetVdbe(pParse);
  if( v ){
    sqlite3VdbeAddOp(v, OP_AutoCommit, 1, 1);
  }
}





int sqlite3OpenTempDatabase(Parse *pParse){
  sqlite3 *db = pParse->db;
  if( db->aDb[1].pBt==0 && !pParse->explain ){
    int rc = sqlite3BtreeFactory(db, 0, 0, MAX_PAGES, &db->aDb[1].pBt);
    if( rc!=SQLITE_OK ){
      sqlite3ErrorMsg(pParse, "unable to open a temporary database "
        "file for storing temporary tables");
      pParse->rc = rc;
      return 1;
    }
    if( db->flags & !db->autoCommit ){
      rc = sqlite3BtreeBeginTrans(db->aDb[1].pBt, 1);
      if( rc!=SQLITE_OK ){
        sqlite3ErrorMsg(pParse, "unable to get a write lock on "
          "the temporary database file");
        pParse->rc = rc;
        return 1;
      }
    }
    assert( db->aDb[1].pSchema );
  }
  return 0;
}























void sqlite3CodeVerifySchema(Parse *pParse, int iDb){
  sqlite3 *db;
  Vdbe *v;
  int mask;

  v = sqlite3GetVdbe(pParse);
  if( v==0 ) return;  
  db = pParse->db;
  if( pParse->cookieGoto==0 ){
    pParse->cookieGoto = sqlite3VdbeAddOp(v, OP_Goto, 0, 0)+1;
  }
  if( iDb>=0 ){
    assert( iDb<db->nDb );
    assert( db->aDb[iDb].pBt!=0 || iDb==1 );
    assert( iDb<MAX_ATTACHED+2 );
    mask = 1<<iDb;
    if( (pParse->cookieMask & mask)==0 ){
      pParse->cookieMask |= mask;
      pParse->cookieValue[iDb] = db->aDb[iDb].pSchema->schema_cookie;
      if( !OMIT_TEMPDB && iDb==1 ){
        sqlite3OpenTempDatabase(pParse);
      }
    }
  }
}



















void sqlite3BeginWriteOperation(Parse *pParse, int setStatement, int iDb){
  Vdbe *v = sqlite3GetVdbe(pParse);
  if( v==0 ) return;
  sqlite3CodeVerifySchema(pParse, iDb);
  pParse->writeMask |= 1<<iDb;
  if( setStatement && pParse->nested==0 ){
    sqlite3VdbeAddOp(v, OP_Statement, iDb, 0);
  }
  if( (OMIT_TEMPDB || iDb!=1) && pParse->db->aDb[1].pBt!=0 ){
    sqlite3BeginWriteOperation(pParse, setStatement, 1);
  }
}





#ifndef SQLITE_OMIT_REINDEX
static int collationMatch(const char *zColl, Index *pIndex){
  int i;
  for(i=0; i<pIndex->nColumn; i++){
    const char *z = pIndex->azColl[i];
    if( z==zColl || (z && zColl && 0==sqlite3StrICmp(z, zColl)) ){
      return 1;
    }
  }
  return 0;
}
#endif





#ifndef SQLITE_OMIT_REINDEX
static void reindexTable(Parse *pParse, Table *pTab, char const *zColl){
  Index *pIndex;              

  for(pIndex=pTab->pIndex; pIndex; pIndex=pIndex->pNext){
    if( zColl==0 || collationMatch(zColl, pIndex) ){
      int iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
      sqlite3BeginWriteOperation(pParse, 0, iDb);
      sqlite3RefillIndex(pParse, pIndex, -1);
    }
  }
}
#endif






#ifndef SQLITE_OMIT_REINDEX
static void reindexDatabases(Parse *pParse, char const *zColl){
  Db *pDb;                    
  int iDb;                    
  sqlite3 *db = pParse->db;   
  HashElem *k;                
  Table *pTab;                

  for(iDb=0, pDb=db->aDb; iDb<db->nDb; iDb++, pDb++){
    assert( pDb!=0 );
    for(k=sqliteHashFirst(&pDb->pSchema->tblHash);  k; k=sqliteHashNext(k)){
      pTab = (Table*)sqliteHashData(k);
      reindexTable(pParse, pTab, zColl);
    }
  }
}
#endif














#ifndef SQLITE_OMIT_REINDEX
void sqlite3Reindex(Parse *pParse, Token *pName1, Token *pName2){
  CollSeq *pColl;             
  char *z;                    
  const char *zDb;            
  Table *pTab;                
  Index *pIndex;              
  int iDb;                    
  sqlite3 *db = pParse->db;   
  Token *pObjName;            

  

  if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
    return;
  }

  if( pName1==0 || pName1->z==0 ){
    reindexDatabases(pParse, 0);
    return;
  }else if( pName2==0 || pName2->z==0 ){
    assert( pName1->z );
    pColl = sqlite3FindCollSeq(db, ENC(db), (char*)pName1->z, pName1->n, 0);
    if( pColl ){
      char *zColl = sqliteStrNDup((const char *)pName1->z, pName1->n);
      if( zColl ){
        reindexDatabases(pParse, zColl);
        sqliteFree(zColl);
      }
      return;
    }
  }
  iDb = sqlite3TwoPartName(pParse, pName1, pName2, &pObjName);
  if( iDb<0 ) return;
  z = sqlite3NameFromToken(pObjName);
  zDb = db->aDb[iDb].zName;
  pTab = sqlite3FindTable(db, z, zDb);
  if( pTab ){
    reindexTable(pParse, pTab, 0);
    sqliteFree(z);
    return;
  }
  pIndex = sqlite3FindIndex(db, z, zDb);
  sqliteFree(z);
  if( pIndex ){
    sqlite3BeginWriteOperation(pParse, 0, iDb);
    sqlite3RefillIndex(pParse, pIndex, -1);
    return;
  }
  sqlite3ErrorMsg(pParse, "unable to identify the object to be reindexed");
}
#endif











KeyInfo *sqlite3IndexKeyinfo(Parse *pParse, Index *pIdx){
  int i;
  int nCol = pIdx->nColumn;
  int nBytes = sizeof(KeyInfo) + (nCol-1)*sizeof(CollSeq*) + nCol;
  KeyInfo *pKey = (KeyInfo *)sqliteMalloc(nBytes);

  if( pKey ){
    pKey->aSortOrder = (u8 *)&(pKey->aColl[nCol]);
    assert( &pKey->aSortOrder[nCol]==&(((u8 *)pKey)[nBytes]) );
    for(i=0; i<nCol; i++){
      char *zColl = pIdx->azColl[i];
      assert( zColl );
      pKey->aColl[i] = sqlite3LocateCollSeq(pParse, zColl, -1);
      pKey->aSortOrder[i] = pIdx->aSortOrder[i];
    }
    pKey->nField = nCol;
  }

  if( pParse->nErr ){
    sqliteFree(pKey);
    pKey = 0;
  }
  return pKey;
}



int sqlite3Preload(sqlite3* db)
{
  Pager* pPager;
  Btree* pBt;
  int rc;
  int i;
  int dbsLoaded = 0;

  for (i = 0; i < db->nDb; i ++) {
    pBt = db->aDb[i].pBt;
    if (! pBt)
      continue;
    pPager = sqlite3BtreePager(pBt);
    if (pPager) {
      rc = sqlite3pager_loadall(pPager);
      if (rc == SQLITE_OK)
        dbsLoaded ++;
    }
  }
  if (dbsLoaded == 0)
    return SQLITE_ERROR;
  return SQLITE_OK;
}
