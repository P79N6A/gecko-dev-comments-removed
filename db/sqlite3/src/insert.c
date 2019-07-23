















#include "sqliteInt.h"














void sqlite3IndexAffinityStr(Vdbe *v, Index *pIdx){
  if( !pIdx->zColAff ){
    







    int n;
    Table *pTab = pIdx->pTable;
    pIdx->zColAff = (char *)sqliteMalloc(pIdx->nColumn+1);
    if( !pIdx->zColAff ){
      return;
    }
    for(n=0; n<pIdx->nColumn; n++){
      pIdx->zColAff[n] = pTab->aCol[pIdx->aiColumn[n]].affinity;
    }
    pIdx->zColAff[pIdx->nColumn] = '\0';
  }
 
  sqlite3VdbeChangeP3(v, -1, pIdx->zColAff, 0);
}















void sqlite3TableAffinityStr(Vdbe *v, Table *pTab){
  






  if( !pTab->zColAff ){
    char *zColAff;
    int i;

    zColAff = (char *)sqliteMalloc(pTab->nCol+1);
    if( !zColAff ){
      return;
    }

    for(i=0; i<pTab->nCol; i++){
      zColAff[i] = pTab->aCol[i].affinity;
    }
    zColAff[pTab->nCol] = '\0';

    pTab->zColAff = zColAff;
  }

  sqlite3VdbeChangeP3(v, -1, pTab->zColAff, 0);
}









static int selectReadsTable(Select *p, Schema *pSchema, int iTab){
  int i;
  struct SrcList_item *pItem;
  if( p->pSrc==0 ) return 0;
  for(i=0, pItem=p->pSrc->a; i<p->pSrc->nSrc; i++, pItem++){
    if( pItem->pSelect ){
      if( selectReadsTable(pItem->pSelect, pSchema, iTab) ) return 1;
    }else{
      if( pItem->pTab->pSchema==pSchema && pItem->pTab->tnum==iTab ) return 1;
    }
  }
  return 0;
}




































































void sqlite3Insert(
  Parse *pParse,        
  SrcList *pTabList,    
  ExprList *pList,      
  Select *pSelect,      
  IdList *pColumn,      
  int onError           
){
  Table *pTab;          
  char *zTab;           
  const char *zDb;      
  int i, j, idx;        
  Vdbe *v;              
  Index *pIdx;          
  int nColumn;          
  int base = 0;         
  int iCont=0,iBreak=0; 
  sqlite3 *db;          
  int keyColumn = -1;   
  int endOfLoop;        
  int useTempTable = 0; 
  int srcTab = 0;       
  int iSelectLoop = 0;  
  int iCleanup = 0;     
  int iInsertBlock = 0; 
  int iCntMem = 0;      
  int newIdx = -1;      
  Db *pDb;              
  int counterMem = 0;   
  int iDb;

#ifndef SQLITE_OMIT_TRIGGER
  int isView;                 
  int triggers_exist = 0;     
#endif

#ifndef SQLITE_OMIT_AUTOINCREMENT
  int counterRowid = 0;  
#endif

  if( pParse->nErr || sqlite3MallocFailed() ){
    goto insert_cleanup;
  }
  db = pParse->db;

  

  assert( pTabList->nSrc==1 );
  zTab = pTabList->a[0].zName;
  if( zTab==0 ) goto insert_cleanup;
  pTab = sqlite3SrcListLookup(pParse, pTabList);
  if( pTab==0 ){
    goto insert_cleanup;
  }
  iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
  assert( iDb<db->nDb );
  pDb = &db->aDb[iDb];
  zDb = pDb->zName;
  if( sqlite3AuthCheck(pParse, SQLITE_INSERT, pTab->zName, 0, zDb) ){
    goto insert_cleanup;
  }

  


#ifndef SQLITE_OMIT_TRIGGER
  triggers_exist = sqlite3TriggersExist(pParse, pTab, TK_INSERT, 0);
  isView = pTab->pSelect!=0;
#else
# define triggers_exist 0
# define isView 0
#endif
#ifdef SQLITE_OMIT_VIEW
# undef isView
# define isView 0
#endif

  



  if( sqlite3IsReadOnly(pParse, pTab, triggers_exist) ){
    goto insert_cleanup;
  }
  assert( pTab!=0 );

  

  if( isView && sqlite3ViewGetColumnNames(pParse, pTab) ){
    goto insert_cleanup;
  }

  

  v = sqlite3GetVdbe(pParse);
  if( v==0 ) goto insert_cleanup;
  if( pParse->nested==0 ) sqlite3VdbeCountChanges(v);
  sqlite3BeginWriteOperation(pParse, pSelect || triggers_exist, iDb);

  
  if( triggers_exist ){
    newIdx = pParse->nTab++;
  }

#ifndef SQLITE_OMIT_AUTOINCREMENT
  




  if( pTab->autoInc ){
    int iCur = pParse->nTab;
    int addr = sqlite3VdbeCurrentAddr(v);
    counterRowid = pParse->nMem++;
    counterMem = pParse->nMem++;
    sqlite3OpenTable(pParse, iCur, iDb, pDb->pSchema->pSeqTab, OP_OpenRead);
    sqlite3VdbeAddOp(v, OP_Rewind, iCur, addr+13);
    sqlite3VdbeAddOp(v, OP_Column, iCur, 0);
    sqlite3VdbeOp3(v, OP_String8, 0, 0, pTab->zName, 0);
    sqlite3VdbeAddOp(v, OP_Ne, 0x100, addr+12);
    sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
    sqlite3VdbeAddOp(v, OP_MemStore, counterRowid, 1);
    sqlite3VdbeAddOp(v, OP_Column, iCur, 1);
    sqlite3VdbeAddOp(v, OP_MemStore, counterMem, 1);
    sqlite3VdbeAddOp(v, OP_Goto, 0, addr+13);
    sqlite3VdbeAddOp(v, OP_Next, iCur, addr+4);
    sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
  }
#endif 

  







  if( pSelect ){
    

    int rc, iInitCode;
    iInitCode = sqlite3VdbeAddOp(v, OP_Goto, 0, 0);
    iSelectLoop = sqlite3VdbeCurrentAddr(v);
    iInsertBlock = sqlite3VdbeMakeLabel(v);

    
    rc = sqlite3Select(pParse, pSelect, SRT_Subroutine, iInsertBlock,0,0,0,0);
    if( rc || pParse->nErr || sqlite3MallocFailed() ){
      goto insert_cleanup;
    }

    iCleanup = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_Goto, 0, iCleanup);
    assert( pSelect->pEList );
    nColumn = pSelect->pEList->nExpr;

    







    if( triggers_exist || selectReadsTable(pSelect,pTab->pSchema,pTab->tnum) ){
      useTempTable = 1;
    }

    if( useTempTable ){
      


      srcTab = pParse->nTab++;
      sqlite3VdbeResolveLabel(v, iInsertBlock);
      sqlite3VdbeAddOp(v, OP_MakeRecord, nColumn, 0);
      sqlite3VdbeAddOp(v, OP_NewRowid, srcTab, 0);
      sqlite3VdbeAddOp(v, OP_Pull, 1, 0);
      sqlite3VdbeAddOp(v, OP_Insert, srcTab, 0);
      sqlite3VdbeAddOp(v, OP_Return, 0, 0);

      



      sqlite3VdbeJumpHere(v, iInitCode);
      sqlite3VdbeAddOp(v, OP_OpenVirtual, srcTab, 0);
      sqlite3VdbeAddOp(v, OP_SetNumColumns, srcTab, nColumn);
      sqlite3VdbeAddOp(v, OP_Goto, 0, iSelectLoop);
      sqlite3VdbeResolveLabel(v, iCleanup);
    }else{
      sqlite3VdbeJumpHere(v, iInitCode);
    }
  }else{
    


    NameContext sNC;
    memset(&sNC, 0, sizeof(sNC));
    sNC.pParse = pParse;
    assert( pList!=0 );
    srcTab = -1;
    useTempTable = 0;
    assert( pList );
    nColumn = pList->nExpr;
    for(i=0; i<nColumn; i++){
      if( sqlite3ExprResolveNames(&sNC, pList->a[i].pExpr) ){
        goto insert_cleanup;
      }
    }
  }

  


  if( pColumn==0 && nColumn!=pTab->nCol ){
    sqlite3ErrorMsg(pParse, 
       "table %S has %d columns but %d values were supplied",
       pTabList, 0, pTab->nCol, nColumn);
    goto insert_cleanup;
  }
  if( pColumn!=0 && nColumn!=pColumn->nId ){
    sqlite3ErrorMsg(pParse, "%d values for %d columns", nColumn, pColumn->nId);
    goto insert_cleanup;
  }

  










  if( pColumn ){
    for(i=0; i<pColumn->nId; i++){
      pColumn->a[i].idx = -1;
    }
    for(i=0; i<pColumn->nId; i++){
      for(j=0; j<pTab->nCol; j++){
        if( sqlite3StrICmp(pColumn->a[i].zName, pTab->aCol[j].zName)==0 ){
          pColumn->a[i].idx = j;
          if( j==pTab->iPKey ){
            keyColumn = i;
          }
          break;
        }
      }
      if( j>=pTab->nCol ){
        if( sqlite3IsRowid(pColumn->a[i].zName) ){
          keyColumn = i;
        }else{
          sqlite3ErrorMsg(pParse, "table %S has no column named %s",
              pTabList, 0, pColumn->a[i].zName);
          pParse->nErr++;
          goto insert_cleanup;
        }
      }
    }
  }

  



  if( pColumn==0 ){
    keyColumn = pTab->iPKey;
  }

  

  if( triggers_exist ){
    sqlite3VdbeAddOp(v, OP_OpenPseudo, newIdx, 0);
    sqlite3VdbeAddOp(v, OP_SetNumColumns, newIdx, pTab->nCol);
  }
    
  

  if( db->flags & SQLITE_CountRows ){
    iCntMem = pParse->nMem++;
    sqlite3VdbeAddOp(v, OP_MemInt, 0, iCntMem);
  }

  
  if( !triggers_exist ){
    base = pParse->nTab;
    sqlite3OpenTableAndIndices(pParse, pTab, base, OP_OpenWrite);
  }

  




  if( useTempTable ){
    iBreak = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_Rewind, srcTab, iBreak);
    iCont = sqlite3VdbeCurrentAddr(v);
  }else if( pSelect ){
    sqlite3VdbeAddOp(v, OP_Goto, 0, iSelectLoop);
    sqlite3VdbeResolveLabel(v, iInsertBlock);
  }

  

  endOfLoop = sqlite3VdbeMakeLabel(v);
  if( triggers_exist & TRIGGER_BEFORE ){

    





    if( keyColumn<0 ){
      sqlite3VdbeAddOp(v, OP_Integer, -1, 0);
    }else if( useTempTable ){
      sqlite3VdbeAddOp(v, OP_Column, srcTab, keyColumn);
    }else{
      assert( pSelect==0 );  
      sqlite3ExprCode(pParse, pList->a[keyColumn].pExpr);
      sqlite3VdbeAddOp(v, OP_NotNull, -1, sqlite3VdbeCurrentAddr(v)+3);
      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      sqlite3VdbeAddOp(v, OP_Integer, -1, 0);
      sqlite3VdbeAddOp(v, OP_MustBeInt, 0, 0);
    }

    

    for(i=0; i<pTab->nCol; i++){
      if( pColumn==0 ){
        j = i;
      }else{
        for(j=0; j<pColumn->nId; j++){
          if( pColumn->a[j].idx==i ) break;
        }
      }
      if( pColumn && j>=pColumn->nId ){
        sqlite3ExprCode(pParse, pTab->aCol[i].pDflt);
      }else if( useTempTable ){
        sqlite3VdbeAddOp(v, OP_Column, srcTab, j); 
      }else{
        assert( pSelect==0 ); 
        sqlite3ExprCodeAndCache(pParse, pList->a[j].pExpr);
      }
    }
    sqlite3VdbeAddOp(v, OP_MakeRecord, pTab->nCol, 0);

    




    if( !isView ){
      sqlite3TableAffinityStr(v, pTab);
    }
    sqlite3VdbeAddOp(v, OP_Insert, newIdx, 0);

    
    if( sqlite3CodeRowTrigger(pParse, TK_INSERT, 0, TRIGGER_BEFORE, pTab, 
        newIdx, -1, onError, endOfLoop) ){
      goto insert_cleanup;
    }
  }

  


  if( triggers_exist && !isView ){
    base = pParse->nTab;
    sqlite3OpenTableAndIndices(pParse, pTab, base, OP_OpenWrite);
  }

  




  if( !isView ){
    if( keyColumn>=0 ){
      if( useTempTable ){
        sqlite3VdbeAddOp(v, OP_Column, srcTab, keyColumn);
      }else if( pSelect ){
        sqlite3VdbeAddOp(v, OP_Dup, nColumn - keyColumn - 1, 1);
      }else{
        sqlite3ExprCode(pParse, pList->a[keyColumn].pExpr);
      }
      


      sqlite3VdbeAddOp(v, OP_NotNull, -1, sqlite3VdbeCurrentAddr(v)+3);
      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      sqlite3VdbeAddOp(v, OP_NewRowid, base, counterMem);
      sqlite3VdbeAddOp(v, OP_MustBeInt, 0, 0);
    }else{
      sqlite3VdbeAddOp(v, OP_NewRowid, base, counterMem);
    }
#ifndef SQLITE_OMIT_AUTOINCREMENT
    if( pTab->autoInc ){
      sqlite3VdbeAddOp(v, OP_MemMax, counterMem, 0);
    }
#endif 

    


    for(i=0; i<pTab->nCol; i++){
      if( i==pTab->iPKey ){
        



        sqlite3VdbeAddOp(v, OP_Null, 0, 0);
        continue;
      }
      if( pColumn==0 ){
        j = i;
      }else{
        for(j=0; j<pColumn->nId; j++){
          if( pColumn->a[j].idx==i ) break;
        }
      }
      if( pColumn && j>=pColumn->nId ){
        sqlite3ExprCode(pParse, pTab->aCol[i].pDflt);
      }else if( useTempTable ){
        sqlite3VdbeAddOp(v, OP_Column, srcTab, j); 
      }else if( pSelect ){
        sqlite3VdbeAddOp(v, OP_Dup, i+nColumn-j, 1);
      }else{
        sqlite3ExprCode(pParse, pList->a[j].pExpr);
      }
    }

    


    sqlite3GenerateConstraintChecks(pParse, pTab, base, 0, keyColumn>=0,
                                   0, onError, endOfLoop);
    sqlite3CompleteInsertion(pParse, pTab, base, 0,0,0,
                            (triggers_exist & TRIGGER_AFTER)!=0 ? newIdx : -1);
  }

  

  if( (db->flags & SQLITE_CountRows)!=0 ){
    sqlite3VdbeAddOp(v, OP_MemIncr, 1, iCntMem);
  }

  if( triggers_exist ){
    
    if( !isView ){
      sqlite3VdbeAddOp(v, OP_Close, base, 0);
      for(idx=1, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, idx++){
        sqlite3VdbeAddOp(v, OP_Close, idx+base, 0);
      }
    }

    
    if( sqlite3CodeRowTrigger(pParse, TK_INSERT, 0, TRIGGER_AFTER, pTab,
          newIdx, -1, onError, endOfLoop) ){
      goto insert_cleanup;
    }
  }

  

  sqlite3VdbeResolveLabel(v, endOfLoop);
  if( useTempTable ){
    sqlite3VdbeAddOp(v, OP_Next, srcTab, iCont);
    sqlite3VdbeResolveLabel(v, iBreak);
    sqlite3VdbeAddOp(v, OP_Close, srcTab, 0);
  }else if( pSelect ){
    sqlite3VdbeAddOp(v, OP_Pop, nColumn, 0);
    sqlite3VdbeAddOp(v, OP_Return, 0, 0);
    sqlite3VdbeResolveLabel(v, iCleanup);
  }

  if( !triggers_exist ){
    
    sqlite3VdbeAddOp(v, OP_Close, base, 0);
    for(idx=1, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, idx++){
      sqlite3VdbeAddOp(v, OP_Close, idx+base, 0);
    }
  }

#ifndef SQLITE_OMIT_AUTOINCREMENT
  



  if( pTab->autoInc ){
    int iCur = pParse->nTab;
    int addr = sqlite3VdbeCurrentAddr(v);
    sqlite3OpenTable(pParse, iCur, iDb, pDb->pSchema->pSeqTab, OP_OpenWrite);
    sqlite3VdbeAddOp(v, OP_MemLoad, counterRowid, 0);
    sqlite3VdbeAddOp(v, OP_NotNull, -1, addr+7);
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
    sqlite3VdbeAddOp(v, OP_NewRowid, iCur, 0);
    sqlite3VdbeOp3(v, OP_String8, 0, 0, pTab->zName, 0);
    sqlite3VdbeAddOp(v, OP_MemLoad, counterMem, 0);
    sqlite3VdbeAddOp(v, OP_MakeRecord, 2, 0);
    sqlite3VdbeAddOp(v, OP_Insert, iCur, 0);
    sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
  }
#endif

  




  if( db->flags & SQLITE_CountRows && pParse->nested==0 && !pParse->trigStack ){
    sqlite3VdbeAddOp(v, OP_MemLoad, iCntMem, 0);
    sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
    sqlite3VdbeSetNumCols(v, 1);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "rows inserted", P3_STATIC);
  }

insert_cleanup:
  sqlite3SrcListDelete(pTabList);
  sqlite3ExprListDelete(pList);
  sqlite3SelectDelete(pSelect);
  sqlite3IdListDelete(pColumn);
}
















































































void sqlite3GenerateConstraintChecks(
  Parse *pParse,      
  Table *pTab,        
  int base,           
  char *aIdxUsed,     
  int rowidChng,      
  int isUpdate,       
  int overrideError,  
  int ignoreDest      
){
  int i;
  Vdbe *v;
  int nCol;
  int onError;
  int addr;
  int extra;
  int iCur;
  Index *pIdx;
  int seenReplace = 0;
  int jumpInst1=0, jumpInst2;
  int hasTwoRowids = (isUpdate && rowidChng);

  v = sqlite3GetVdbe(pParse);
  assert( v!=0 );
  assert( pTab->pSelect==0 );  
  nCol = pTab->nCol;

  

  for(i=0; i<nCol; i++){
    if( i==pTab->iPKey ){
      continue;
    }
    onError = pTab->aCol[i].notNull;
    if( onError==OE_None ) continue;
    if( overrideError!=OE_Default ){
      onError = overrideError;
    }else if( onError==OE_Default ){
      onError = OE_Abort;
    }
    if( onError==OE_Replace && pTab->aCol[i].pDflt==0 ){
      onError = OE_Abort;
    }
    sqlite3VdbeAddOp(v, OP_Dup, nCol-1-i, 1);
    addr = sqlite3VdbeAddOp(v, OP_NotNull, 1, 0);
    assert( onError==OE_Rollback || onError==OE_Abort || onError==OE_Fail
        || onError==OE_Ignore || onError==OE_Replace );
    switch( onError ){
      case OE_Rollback:
      case OE_Abort:
      case OE_Fail: {
        char *zMsg = 0;
        sqlite3VdbeAddOp(v, OP_Halt, SQLITE_CONSTRAINT, onError);
        sqlite3SetString(&zMsg, pTab->zName, ".", pTab->aCol[i].zName,
                        " may not be NULL", (char*)0);
        sqlite3VdbeChangeP3(v, -1, zMsg, P3_DYNAMIC);
        break;
      }
      case OE_Ignore: {
        sqlite3VdbeAddOp(v, OP_Pop, nCol+1+hasTwoRowids, 0);
        sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
        break;
      }
      case OE_Replace: {
        sqlite3ExprCode(pParse, pTab->aCol[i].pDflt);
        sqlite3VdbeAddOp(v, OP_Push, nCol-i, 0);
        break;
      }
    }
    sqlite3VdbeJumpHere(v, addr);
  }

  

#ifndef SQLITE_OMIT_CHECK
  if( pTab->pCheck && (pParse->db->flags & SQLITE_IgnoreChecks)==0 ){
    int allOk = sqlite3VdbeMakeLabel(v);
    assert( pParse->ckOffset==0 );
    pParse->ckOffset = nCol;
    sqlite3ExprIfTrue(pParse, pTab->pCheck, allOk, 1);
    assert( pParse->ckOffset==nCol );
    pParse->ckOffset = 0;
    onError = overrideError!=OE_Default ? overrideError : OE_Abort;
    if( onError==OE_Ignore || onError==OE_Replace ){
      sqlite3VdbeAddOp(v, OP_Pop, nCol+1+hasTwoRowids, 0);
      sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
    }else{
      sqlite3VdbeAddOp(v, OP_Halt, SQLITE_CONSTRAINT, onError);
    }
    sqlite3VdbeResolveLabel(v, allOk);
  }
#endif 

  



  if( rowidChng ){
    onError = pTab->keyConf;
    if( overrideError!=OE_Default ){
      onError = overrideError;
    }else if( onError==OE_Default ){
      onError = OE_Abort;
    }
    
    if( isUpdate ){
      sqlite3VdbeAddOp(v, OP_Dup, nCol+1, 1);
      sqlite3VdbeAddOp(v, OP_Dup, nCol+1, 1);
      jumpInst1 = sqlite3VdbeAddOp(v, OP_Eq, 0, 0);
    }
    sqlite3VdbeAddOp(v, OP_Dup, nCol, 1);
    jumpInst2 = sqlite3VdbeAddOp(v, OP_NotExists, base, 0);
    switch( onError ){
      default: {
        onError = OE_Abort;
        
      }
      case OE_Rollback:
      case OE_Abort:
      case OE_Fail: {
        sqlite3VdbeOp3(v, OP_Halt, SQLITE_CONSTRAINT, onError,
                         "PRIMARY KEY must be unique", P3_STATIC);
        break;
      }
      case OE_Replace: {
        sqlite3GenerateRowIndexDelete(v, pTab, base, 0);
        if( isUpdate ){
          sqlite3VdbeAddOp(v, OP_Dup, nCol+hasTwoRowids, 1);
          sqlite3VdbeAddOp(v, OP_MoveGe, base, 0);
        }
        seenReplace = 1;
        break;
      }
      case OE_Ignore: {
        assert( seenReplace==0 );
        sqlite3VdbeAddOp(v, OP_Pop, nCol+1+hasTwoRowids, 0);
        sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
        break;
      }
    }
    sqlite3VdbeJumpHere(v, jumpInst2);
    if( isUpdate ){
      sqlite3VdbeJumpHere(v, jumpInst1);
      sqlite3VdbeAddOp(v, OP_Dup, nCol+1, 1);
      sqlite3VdbeAddOp(v, OP_MoveGe, base, 0);
    }
  }

  



  extra = -1;
  for(iCur=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, iCur++){
    if( aIdxUsed && aIdxUsed[iCur]==0 ) continue;  
    extra++;

    
    sqlite3VdbeAddOp(v, OP_Dup, nCol+extra, 1);
    for(i=0; i<pIdx->nColumn; i++){
      int idx = pIdx->aiColumn[i];
      if( idx==pTab->iPKey ){
        sqlite3VdbeAddOp(v, OP_Dup, i+extra+nCol+1, 1);
      }else{
        sqlite3VdbeAddOp(v, OP_Dup, i+extra+nCol-idx, 1);
      }
    }
    jumpInst1 = sqlite3VdbeAddOp(v, OP_MakeIdxRec, pIdx->nColumn, 0);
    sqlite3IndexAffinityStr(v, pIdx);

    
    onError = pIdx->onError;
    if( onError==OE_None ) continue;  
    if( overrideError!=OE_Default ){
      onError = overrideError;
    }else if( onError==OE_Default ){
      onError = OE_Abort;
    }
    if( seenReplace ){
      if( onError==OE_Ignore ) onError = OE_Replace;
      else if( onError==OE_Fail ) onError = OE_Abort;
    }
    

    
    sqlite3VdbeAddOp(v, OP_Dup, extra+nCol+1+hasTwoRowids, 1);
    jumpInst2 = sqlite3VdbeAddOp(v, OP_IsUnique, base+iCur+1, 0);

    
    assert( onError==OE_Rollback || onError==OE_Abort || onError==OE_Fail
        || onError==OE_Ignore || onError==OE_Replace );
    switch( onError ){
      case OE_Rollback:
      case OE_Abort:
      case OE_Fail: {
        int j, n1, n2;
        char zErrMsg[200];
        strcpy(zErrMsg, pIdx->nColumn>1 ? "columns " : "column ");
        n1 = strlen(zErrMsg);
        for(j=0; j<pIdx->nColumn && n1<sizeof(zErrMsg)-30; j++){
          char *zCol = pTab->aCol[pIdx->aiColumn[j]].zName;
          n2 = strlen(zCol);
          if( j>0 ){
            strcpy(&zErrMsg[n1], ", ");
            n1 += 2;
          }
          if( n1+n2>sizeof(zErrMsg)-30 ){
            strcpy(&zErrMsg[n1], "...");
            n1 += 3;
            break;
          }else{
            strcpy(&zErrMsg[n1], zCol);
            n1 += n2;
          }
        }
        strcpy(&zErrMsg[n1], 
            pIdx->nColumn>1 ? " are not unique" : " is not unique");
        sqlite3VdbeOp3(v, OP_Halt, SQLITE_CONSTRAINT, onError, zErrMsg, 0);
        break;
      }
      case OE_Ignore: {
        assert( seenReplace==0 );
        sqlite3VdbeAddOp(v, OP_Pop, nCol+extra+3+hasTwoRowids, 0);
        sqlite3VdbeAddOp(v, OP_Goto, 0, ignoreDest);
        break;
      }
      case OE_Replace: {
        sqlite3GenerateRowDelete(pParse->db, v, pTab, base, 0);
        if( isUpdate ){
          sqlite3VdbeAddOp(v, OP_Dup, nCol+extra+1+hasTwoRowids, 1);
          sqlite3VdbeAddOp(v, OP_MoveGe, base, 0);
        }
        seenReplace = 1;
        break;
      }
    }
#if NULL_DISTINCT_FOR_UNIQUE
    sqlite3VdbeJumpHere(v, jumpInst1);
#endif
    sqlite3VdbeJumpHere(v, jumpInst2);
  }
}











void sqlite3CompleteInsertion(
  Parse *pParse,      
  Table *pTab,        
  int base,           
  char *aIdxUsed,     
  int rowidChng,      
  int isUpdate,       
  int newIdx          
){
  int i;
  Vdbe *v;
  int nIdx;
  Index *pIdx;
  int pik_flags;

  v = sqlite3GetVdbe(pParse);
  assert( v!=0 );
  assert( pTab->pSelect==0 );  
  for(nIdx=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, nIdx++){}
  for(i=nIdx-1; i>=0; i--){
    if( aIdxUsed && aIdxUsed[i]==0 ) continue;
    sqlite3VdbeAddOp(v, OP_IdxInsert, base+i+1, 0);
  }
  sqlite3VdbeAddOp(v, OP_MakeRecord, pTab->nCol, 0);
  sqlite3TableAffinityStr(v, pTab);
#ifndef SQLITE_OMIT_TRIGGER
  if( newIdx>=0 ){
    sqlite3VdbeAddOp(v, OP_Dup, 1, 0);
    sqlite3VdbeAddOp(v, OP_Dup, 1, 0);
    sqlite3VdbeAddOp(v, OP_Insert, newIdx, 0);
  }
#endif
  if( pParse->nested ){
    pik_flags = 0;
  }else{
    pik_flags = OPFLAG_NCHANGE;
    pik_flags |= (isUpdate?OPFLAG_ISUPDATE:OPFLAG_LASTROWID);
  }
  sqlite3VdbeAddOp(v, OP_Insert, base, pik_flags);
  if( !pParse->nested ){
    sqlite3VdbeChangeP3(v, -1, pTab->zName, P3_STATIC);
  }
  
  if( isUpdate && rowidChng ){
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
  }
}






void sqlite3OpenTableAndIndices(
  Parse *pParse,   
  Table *pTab,     
  int base,        
  int op           
){
  int i;
  int iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  Index *pIdx;
  Vdbe *v = sqlite3GetVdbe(pParse);
  assert( v!=0 );
  sqlite3OpenTable(pParse, base, iDb, pTab, op);
  for(i=1, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, i++){
    KeyInfo *pKey = sqlite3IndexKeyinfo(pParse, pIdx);
    assert( pIdx->pSchema==pTab->pSchema );
    sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
    VdbeComment((v, "# %s", pIdx->zName));
    sqlite3VdbeOp3(v, op, i+base, pIdx->tnum, (char*)pKey, P3_KEYINFO_HANDOFF);
  }
  if( pParse->nTab<=base+i ){
    pParse->nTab = base+i;
  }
}
