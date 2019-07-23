

















#include "sqliteInt.h"






static void callCollNeeded(sqlite3 *db, const char *zName, int nName){
  assert( !db->xCollNeeded || !db->xCollNeeded16 );
  if( nName<0 ) nName = strlen(zName);
  if( db->xCollNeeded ){
    char *zExternal = sqliteStrNDup(zName, nName);
    if( !zExternal ) return;
    db->xCollNeeded(db->pCollNeededArg, db, (int)ENC(db), zExternal);
    sqliteFree(zExternal);
  }
#ifndef SQLITE_OMIT_UTF16
  if( db->xCollNeeded16 ){
    char const *zExternal;
    sqlite3_value *pTmp = sqlite3ValueNew();
    sqlite3ValueSetStr(pTmp, nName, zName, SQLITE_UTF8, SQLITE_STATIC);
    zExternal = sqlite3ValueText(pTmp, SQLITE_UTF16NATIVE);
    if( zExternal ){
      db->xCollNeeded16(db->pCollNeededArg, db, (int)ENC(db), zExternal);
    }
    sqlite3ValueFree(pTmp);
  }
#endif
}








static int synthCollSeq(sqlite3 *db, CollSeq *pColl){
  CollSeq *pColl2;
  char *z = pColl->zName;
  int n = strlen(z);
  int i;
  static const u8 aEnc[] = { SQLITE_UTF16BE, SQLITE_UTF16LE, SQLITE_UTF8 };
  for(i=0; i<3; i++){
    pColl2 = sqlite3FindCollSeq(db, aEnc[i], z, n, 0);
    if( pColl2->xCmp!=0 ){
      memcpy(pColl, pColl2, sizeof(CollSeq));
      return SQLITE_OK;
    }
  }
  return SQLITE_ERROR;
}














CollSeq *sqlite3GetCollSeq(
  sqlite3* db, 
  CollSeq *pColl, 
  const char *zName, 
  int nName
){
  CollSeq *p;

  p = pColl;
  if( !p ){
    p = sqlite3FindCollSeq(db, ENC(db), zName, nName, 0);
  }
  if( !p || !p->xCmp ){
    


    callCollNeeded(db, zName, nName);
    p = sqlite3FindCollSeq(db, ENC(db), zName, nName, 0);
  }
  if( p && !p->xCmp && synthCollSeq(db, p) ){
    p = 0;
  }
  assert( !p || p->xCmp );
  return p;
}












int sqlite3CheckCollSeq(Parse *pParse, CollSeq *pColl){
  if( pColl ){
    const char *zName = pColl->zName;
    CollSeq *p = sqlite3GetCollSeq(pParse->db, pColl, zName, -1);
    if( !p ){
      if( pParse->nErr==0 ){
        sqlite3ErrorMsg(pParse, "no such collation sequence: %s", zName);
      }
      pParse->nErr++;
      return SQLITE_ERROR;
    }
    assert( p==pColl );
  }
  return SQLITE_OK;
}
















static CollSeq *findCollSeqEntry(
  sqlite3 *db,
  const char *zName,
  int nName,
  int create
){
  CollSeq *pColl;
  if( nName<0 ) nName = strlen(zName);
  pColl = sqlite3HashFind(&db->aCollSeq, zName, nName);

  if( 0==pColl && create ){
    pColl = sqliteMalloc( 3*sizeof(*pColl) + nName + 1 );
    if( pColl ){
      CollSeq *pDel = 0;
      pColl[0].zName = (char*)&pColl[3];
      pColl[0].enc = SQLITE_UTF8;
      pColl[1].zName = (char*)&pColl[3];
      pColl[1].enc = SQLITE_UTF16LE;
      pColl[2].zName = (char*)&pColl[3];
      pColl[2].enc = SQLITE_UTF16BE;
      memcpy(pColl[0].zName, zName, nName);
      pColl[0].zName[nName] = 0;
      pDel = sqlite3HashInsert(&db->aCollSeq, pColl[0].zName, nName, pColl);

      



      assert( !pDel || (sqlite3MallocFailed() && pDel==pColl) );
      if( pDel ){
        sqliteFree(pDel);
        pColl = 0;
      }
    }
  }
  return pColl;
}









CollSeq *sqlite3FindCollSeq(
  sqlite3 *db,
  u8 enc,
  const char *zName,
  int nName,
  int create
){
  CollSeq *pColl;
  if( zName ){
    pColl = findCollSeqEntry(db, zName, nName, create);
  }else{
    pColl = db->pDfltColl;
  }
  assert( SQLITE_UTF8==1 && SQLITE_UTF16LE==2 && SQLITE_UTF16BE==3 );
  assert( enc>=SQLITE_UTF8 && enc<=SQLITE_UTF16BE );
  if( pColl ) pColl += enc-1;
  return pColl;
}





















FuncDef *sqlite3FindFunction(
  sqlite3 *db,       
  const char *zName, 
  int nName,         
  int nArg,          
  u8 enc,            
  int createFlag     
){
  FuncDef *p;         
  FuncDef *pFirst;    
  FuncDef *pBest = 0; 
  int bestmatch = 0;  


  assert( enc==SQLITE_UTF8 || enc==SQLITE_UTF16LE || enc==SQLITE_UTF16BE );
  if( nArg<-1 ) nArg = -1;

  pFirst = (FuncDef*)sqlite3HashFind(&db->aFunc, zName, nName);
  for(p=pFirst; p; p=p->pNext){
    

















    if( p->nArg==-1 || p->nArg==nArg || nArg==-1 ){
      int match = 1;          
      if( p->nArg==nArg || nArg==-1 ){
        match = 4;
      }
      if( enc==p->iPrefEnc ){
        match += 2;
      }
      else if( (enc==SQLITE_UTF16LE && p->iPrefEnc==SQLITE_UTF16BE) ||
               (enc==SQLITE_UTF16BE && p->iPrefEnc==SQLITE_UTF16LE) ){
        match += 1;
      }

      if( match>bestmatch ){
        pBest = p;
        bestmatch = match;
      }
    }
  }

  



  if( createFlag && bestmatch<6 && 
      (pBest = sqliteMalloc(sizeof(*pBest)+nName))!=0 ){
    pBest->nArg = nArg;
    pBest->pNext = pFirst;
    pBest->iPrefEnc = enc;
    memcpy(pBest->zName, zName, nName);
    pBest->zName[nName] = 0;
    if( pBest==sqlite3HashInsert(&db->aFunc,pBest->zName,nName,(void*)pBest) ){
      sqliteFree(pBest);
      return 0;
    }
  }

  if( pBest && (pBest->xStep || pBest->xFunc || createFlag) ){
    return pBest;
  }
  return 0;
}







void sqlite3SchemaFree(void *p){
  Hash temp1;
  Hash temp2;
  HashElem *pElem;
  Schema *pSchema = (Schema *)p;

  temp1 = pSchema->tblHash;
  temp2 = pSchema->trigHash;
  sqlite3HashInit(&pSchema->trigHash, SQLITE_HASH_STRING, 0);
  sqlite3HashClear(&pSchema->aFKey);
  sqlite3HashClear(&pSchema->idxHash);
  for(pElem=sqliteHashFirst(&temp2); pElem; pElem=sqliteHashNext(pElem)){
    sqlite3DeleteTrigger((Trigger*)sqliteHashData(pElem));
  }
  sqlite3HashClear(&temp2);
  sqlite3HashInit(&pSchema->tblHash, SQLITE_HASH_STRING, 0);
  for(pElem=sqliteHashFirst(&temp1); pElem; pElem=sqliteHashNext(pElem)){
    Table *pTab = sqliteHashData(pElem);
    sqlite3DeleteTable(0, pTab);
  }
  sqlite3HashClear(&temp1);
  pSchema->pSeqTab = 0;
  pSchema->flags &= ~DB_SchemaLoaded;
}





Schema *sqlite3SchemaGet(Btree *pBt){
  Schema * p;
  if( pBt ){
    p = (Schema *)sqlite3BtreeSchema(pBt,sizeof(Schema),sqlite3SchemaFree);
  }else{
    p = (Schema *)sqliteMalloc(sizeof(Schema));
  }
  if( p && 0==p->file_format ){
    sqlite3HashInit(&p->tblHash, SQLITE_HASH_STRING, 0);
    sqlite3HashInit(&p->idxHash, SQLITE_HASH_STRING, 0);
    sqlite3HashInit(&p->trigHash, SQLITE_HASH_STRING, 0);
    sqlite3HashInit(&p->aFKey, SQLITE_HASH_STRING, 1);
  }
  return p;
}
