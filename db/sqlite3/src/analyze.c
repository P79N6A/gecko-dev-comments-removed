














#ifndef SQLITE_OMIT_ANALYZE
#include "sqliteInt.h"









static void openStatTable(
  Parse *pParse,          
  int iDb,                
  int iStatCur,           
  const char *zWhere      
){
  sqlite3 *db = pParse->db;
  Db *pDb;
  int iRootPage;
  Table *pStat;
  Vdbe *v = sqlite3GetVdbe(pParse);

  pDb = &db->aDb[iDb];
  if( (pStat = sqlite3FindTable(db, "sqlite_stat1", pDb->zName))==0 ){
    



    sqlite3NestedParse(pParse,
      "CREATE TABLE %Q.sqlite_stat1(tbl,idx,stat)",
      pDb->zName
    );
    iRootPage = 0;  
  }else if( zWhere ){
    

    sqlite3NestedParse(pParse,
       "DELETE FROM %Q.sqlite_stat1 WHERE tbl=%Q",
       pDb->zName, zWhere
    );
    iRootPage = pStat->tnum;
  }else{
    
    iRootPage = pStat->tnum;
    sqlite3VdbeAddOp(v, OP_Clear, pStat->tnum, iDb);
  }

  




  if( iRootPage>0 ){
    sqlite3TableLock(pParse, iDb, iRootPage, 1, "sqlite_stat1");
  }
  sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
  sqlite3VdbeAddOp(v, OP_OpenWrite, iStatCur, iRootPage);
  sqlite3VdbeAddOp(v, OP_SetNumColumns, iStatCur, 3);
}





static void analyzeOneTable(
  Parse *pParse,   
  Table *pTab,     
  int iStatCur,    
  int iMem         
){
  Index *pIdx;     
  int iIdxCur;     
  int nCol;        
  Vdbe *v;         
  int i;           
  int topOfLoop;   
  int endOfLoop;   
  int addr;        
  int iDb;         

  v = sqlite3GetVdbe(pParse);
  if( pTab==0 || pTab->pIndex==0 ){
    
    return;
  }

  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  assert( iDb>=0 );
#ifndef SQLITE_OMIT_AUTHORIZATION
  if( sqlite3AuthCheck(pParse, SQLITE_ANALYZE, pTab->zName, 0,
      pParse->db->aDb[iDb].zName ) ){
    return;
  }
#endif

  
  sqlite3TableLock(pParse, iDb, pTab->tnum, 0, pTab->zName);

  iIdxCur = pParse->nTab;
  for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
    KeyInfo *pKey = sqlite3IndexKeyinfo(pParse, pIdx);

    

    assert( iDb==sqlite3SchemaToIndex(pParse->db, pIdx->pSchema) );
    sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
    VdbeComment((v, "# %s", pIdx->zName));
    sqlite3VdbeOp3(v, OP_OpenRead, iIdxCur, pIdx->tnum,
        (char *)pKey, P3_KEYINFO_HANDOFF);
    nCol = pIdx->nColumn;
    if( iMem+nCol*2>=pParse->nMem ){
      pParse->nMem = iMem+nCol*2+1;
    }
    sqlite3VdbeAddOp(v, OP_SetNumColumns, iIdxCur, nCol+1);

    












    for(i=0; i<=nCol; i++){
      sqlite3VdbeAddOp(v, OP_MemInt, 0, iMem+i);
    }
    for(i=0; i<nCol; i++){
      sqlite3VdbeAddOp(v, OP_MemNull, iMem+nCol+i+1, 0);
    }

    

    endOfLoop = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_Rewind, iIdxCur, endOfLoop);
    topOfLoop = sqlite3VdbeCurrentAddr(v);
    sqlite3VdbeAddOp(v, OP_MemIncr, 1, iMem);
    for(i=0; i<nCol; i++){
      sqlite3VdbeAddOp(v, OP_Column, iIdxCur, i);
      sqlite3VdbeAddOp(v, OP_MemLoad, iMem+nCol+i+1, 0);
      sqlite3VdbeAddOp(v, OP_Ne, 0x100, 0);
    }
    sqlite3VdbeAddOp(v, OP_Goto, 0, endOfLoop);
    for(i=0; i<nCol; i++){
      addr = sqlite3VdbeAddOp(v, OP_MemIncr, 1, iMem+i+1);
      sqlite3VdbeChangeP2(v, topOfLoop + 3*i + 3, addr);
      sqlite3VdbeAddOp(v, OP_Column, iIdxCur, i);
      sqlite3VdbeAddOp(v, OP_MemStore, iMem+nCol+i+1, 1);
    }
    sqlite3VdbeResolveLabel(v, endOfLoop);
    sqlite3VdbeAddOp(v, OP_Next, iIdxCur, topOfLoop);
    sqlite3VdbeAddOp(v, OP_Close, iIdxCur, 0);

    

















    sqlite3VdbeAddOp(v, OP_MemLoad, iMem, 0);
    addr = sqlite3VdbeAddOp(v, OP_IfNot, 0, 0);
    sqlite3VdbeAddOp(v, OP_NewRowid, iStatCur, 0);
    sqlite3VdbeOp3(v, OP_String8, 0, 0, pTab->zName, 0);
    sqlite3VdbeOp3(v, OP_String8, 0, 0, pIdx->zName, 0);
    sqlite3VdbeAddOp(v, OP_MemLoad, iMem, 0);
    sqlite3VdbeOp3(v, OP_String8, 0, 0, " ", 0);
    for(i=0; i<nCol; i++){
      sqlite3VdbeAddOp(v, OP_MemLoad, iMem, 0);
      sqlite3VdbeAddOp(v, OP_MemLoad, iMem+i+1, 0);
      sqlite3VdbeAddOp(v, OP_Add, 0, 0);
      sqlite3VdbeAddOp(v, OP_AddImm, -1, 0);
      sqlite3VdbeAddOp(v, OP_MemLoad, iMem+i+1, 0);
      sqlite3VdbeAddOp(v, OP_Divide, 0, 0);
      sqlite3VdbeAddOp(v, OP_ToInt, 0, 0);
      if( i==nCol-1 ){
        sqlite3VdbeAddOp(v, OP_Concat, nCol*2-1, 0);
      }else{
        sqlite3VdbeAddOp(v, OP_Dup, 1, 0);
      }
    }
    sqlite3VdbeOp3(v, OP_MakeRecord, 3, 0, "aaa", 0);
    sqlite3VdbeAddOp(v, OP_Insert, iStatCur, 0);
    sqlite3VdbeJumpHere(v, addr);
  }
}





static void loadAnalysis(Parse *pParse, int iDb){
  Vdbe *v = sqlite3GetVdbe(pParse);
  sqlite3VdbeAddOp(v, OP_LoadAnalysis, iDb, 0);
}




static void analyzeDatabase(Parse *pParse, int iDb){
  sqlite3 *db = pParse->db;
  Schema *pSchema = db->aDb[iDb].pSchema;    
  HashElem *k;
  int iStatCur;
  int iMem;

  sqlite3BeginWriteOperation(pParse, 0, iDb);
  iStatCur = pParse->nTab++;
  openStatTable(pParse, iDb, iStatCur, 0);
  iMem = pParse->nMem;
  for(k=sqliteHashFirst(&pSchema->tblHash); k; k=sqliteHashNext(k)){
    Table *pTab = (Table*)sqliteHashData(k);
    analyzeOneTable(pParse, pTab, iStatCur, iMem);
  }
  loadAnalysis(pParse, iDb);
}





static void analyzeTable(Parse *pParse, Table *pTab){
  int iDb;
  int iStatCur;

  assert( pTab!=0 );
  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  sqlite3BeginWriteOperation(pParse, 0, iDb);
  iStatCur = pParse->nTab++;
  openStatTable(pParse, iDb, iStatCur, pTab->zName);
  analyzeOneTable(pParse, pTab, iStatCur, pParse->nMem);
  loadAnalysis(pParse, iDb);
}













void sqlite3Analyze(Parse *pParse, Token *pName1, Token *pName2){
  sqlite3 *db = pParse->db;
  int iDb;
  int i;
  char *z, *zDb;
  Table *pTab;
  Token *pTableName;

  

  if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
    return;
  }

  if( pName1==0 ){
    
    for(i=0; i<db->nDb; i++){
      if( i==1 ) continue;  
      analyzeDatabase(pParse, i);
    }
  }else if( pName2==0 || pName2->n==0 ){
    
    iDb = sqlite3FindDb(db, pName1);
    if( iDb>=0 ){
      analyzeDatabase(pParse, iDb);
    }else{
      z = sqlite3NameFromToken(pName1);
      pTab = sqlite3LocateTable(pParse, z, 0);
      sqliteFree(z);
      if( pTab ){
        analyzeTable(pParse, pTab);
      }
    }
  }else{
    
    iDb = sqlite3TwoPartName(pParse, pName1, pName2, &pTableName);
    if( iDb>=0 ){
      zDb = db->aDb[iDb].zName;
      z = sqlite3NameFromToken(pTableName);
      pTab = sqlite3LocateTable(pParse, z, zDb);
      sqliteFree(z);
      if( pTab ){
        analyzeTable(pParse, pTab);
      }
    }   
  }
}





typedef struct analysisInfo analysisInfo;
struct analysisInfo {
  sqlite3 *db;
  const char *zDatabase;
};








static int analysisLoader(void *pData, int argc, char **argv, char **azNotUsed){
  analysisInfo *pInfo = (analysisInfo*)pData;
  Index *pIndex;
  int i, c;
  unsigned int v;
  const char *z;

  assert( argc==2 );
  if( argv==0 || argv[0]==0 || argv[1]==0 ){
    return 0;
  }
  pIndex = sqlite3FindIndex(pInfo->db, argv[0], pInfo->zDatabase);
  if( pIndex==0 ){
    return 0;
  }
  z = argv[1];
  for(i=0; *z && i<=pIndex->nColumn; i++){
    v = 0;
    while( (c=z[0])>='0' && c<='9' ){
      v = v*10 + c - '0';
      z++;
    }
    pIndex->aiRowEst[i] = v;
    if( *z==' ' ) z++;
  }
  return 0;
}




void sqlite3AnalysisLoad(sqlite3 *db, int iDb){
  analysisInfo sInfo;
  HashElem *i;
  char *zSql;

  
  for(i=sqliteHashFirst(&db->aDb[iDb].pSchema->idxHash);i;i=sqliteHashNext(i)){
    Index *pIdx = sqliteHashData(i);
    sqlite3DefaultRowEst(pIdx);
  }

  
  sInfo.db = db;
  sInfo.zDatabase = db->aDb[iDb].zName;
  if( sqlite3FindTable(db, "sqlite_stat1", sInfo.zDatabase)==0 ){
     return;
  }


  
  zSql = sqlite3MPrintf("SELECT idx, stat FROM %Q.sqlite_stat1",
                        sInfo.zDatabase);
  sqlite3SafetyOff(db);
  sqlite3_exec(db, zSql, analysisLoader, &sInfo, 0);
  sqlite3SafetyOn(db);
  sqliteFree(zSql);
}


#endif 
