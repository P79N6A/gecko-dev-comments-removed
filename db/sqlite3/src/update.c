















#include "sqliteInt.h"


























void sqlite3ColumnDefault(Vdbe *v, Table *pTab, int i){
  if( pTab && !pTab->pSelect ){
    sqlite3_value *pValue;
    u8 enc = ENC(sqlite3VdbeDb(v));
    Column *pCol = &pTab->aCol[i];
    sqlite3ValueFromExpr(pCol->pDflt, enc, pCol->affinity, &pValue);
    if( pValue ){
      sqlite3VdbeChangeP3(v, -1, (const char *)pValue, P3_MEM);
    }else{
      VdbeComment((v, "# %s.%s", pTab->zName, pCol->zName));
    }
  }
}








void sqlite3Update(
  Parse *pParse,         
  SrcList *pTabList,     
  ExprList *pChanges,    
  Expr *pWhere,          
  int onError            
){
  int i, j;              
  Table *pTab;           
  int addr = 0;          
  WhereInfo *pWInfo;     
  Vdbe *v;               
  Index *pIdx;           
  int nIdx;              
  int nIdxTotal;         
  int iCur;              
  sqlite3 *db;           
  Index **apIdx = 0;     
  char *aIdxUsed = 0;    
  int *aXRef = 0;        


  int chngRowid;         
  Expr *pRowidExpr = 0;  
  int openAll = 0;       
  AuthContext sContext;  
  NameContext sNC;       
  int iDb;               

#ifndef SQLITE_OMIT_TRIGGER
  int isView;                  
  int triggers_exist = 0;      
#endif

  int newIdx      = -1;  
  int oldIdx      = -1;  

  sContext.pParse = 0;
  if( pParse->nErr || sqlite3MallocFailed() ){
    goto update_cleanup;
  }
  db = pParse->db;
  assert( pTabList->nSrc==1 );

  

  pTab = sqlite3SrcListLookup(pParse, pTabList);
  if( pTab==0 ) goto update_cleanup;
  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);

  


#ifndef SQLITE_OMIT_TRIGGER
  triggers_exist = sqlite3TriggersExist(pParse, pTab, TK_UPDATE, pChanges);
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
    goto update_cleanup;
  }
  if( isView ){
    if( sqlite3ViewGetColumnNames(pParse, pTab) ){
      goto update_cleanup;
    }
  }
  aXRef = sqliteMallocRaw( sizeof(int) * pTab->nCol );
  if( aXRef==0 ) goto update_cleanup;
  for(i=0; i<pTab->nCol; i++) aXRef[i] = -1;

  


  if( triggers_exist ){
    newIdx = pParse->nTab++;
    oldIdx = pParse->nTab++;
  }

  




  pTabList->a[0].iCursor = iCur = pParse->nTab++;
  for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
    pParse->nTab++;
  }

  
  memset(&sNC, 0, sizeof(sNC));
  sNC.pParse = pParse;
  sNC.pSrcList = pTabList;

  





  chngRowid = 0;
  for(i=0; i<pChanges->nExpr; i++){
    if( sqlite3ExprResolveNames(&sNC, pChanges->a[i].pExpr) ){
      goto update_cleanup;
    }
    for(j=0; j<pTab->nCol; j++){
      if( sqlite3StrICmp(pTab->aCol[j].zName, pChanges->a[i].zName)==0 ){
        if( j==pTab->iPKey ){
          chngRowid = 1;
          pRowidExpr = pChanges->a[i].pExpr;
        }
        aXRef[j] = i;
        break;
      }
    }
    if( j>=pTab->nCol ){
      if( sqlite3IsRowid(pChanges->a[i].zName) ){
        chngRowid = 1;
        pRowidExpr = pChanges->a[i].pExpr;
      }else{
        sqlite3ErrorMsg(pParse, "no such column: %s", pChanges->a[i].zName);
        goto update_cleanup;
      }
    }
#ifndef SQLITE_OMIT_AUTHORIZATION
    {
      int rc;
      rc = sqlite3AuthCheck(pParse, SQLITE_UPDATE, pTab->zName,
                           pTab->aCol[j].zName, db->aDb[iDb].zName);
      if( rc==SQLITE_DENY ){
        goto update_cleanup;
      }else if( rc==SQLITE_IGNORE ){
        aXRef[j] = -1;
      }
    }
#endif
  }

  




  for(nIdx=nIdxTotal=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, nIdxTotal++){
    if( chngRowid ){
      i = 0;
    }else {
      for(i=0; i<pIdx->nColumn; i++){
        if( aXRef[pIdx->aiColumn[i]]>=0 ) break;
      }
    }
    if( i<pIdx->nColumn ) nIdx++;
  }
  if( nIdxTotal>0 ){
    apIdx = sqliteMallocRaw( sizeof(Index*) * nIdx + nIdxTotal );
    if( apIdx==0 ) goto update_cleanup;
    aIdxUsed = (char*)&apIdx[nIdx];
  }
  for(nIdx=j=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, j++){
    if( chngRowid ){
      i = 0;
    }else{
      for(i=0; i<pIdx->nColumn; i++){
        if( aXRef[pIdx->aiColumn[i]]>=0 ) break;
      }
    }
    if( i<pIdx->nColumn ){
      apIdx[nIdx++] = pIdx;
      aIdxUsed[j] = 1;
    }else{
      aIdxUsed[j] = 0;
    }
  }

  


  if( sqlite3ExprResolveNames(&sNC, pWhere) ){
    goto update_cleanup;
  }

  

  if( isView ){
    sqlite3AuthContextPush(pParse, &sContext, pTab->zName);
  }

  

  v = sqlite3GetVdbe(pParse);
  if( v==0 ) goto update_cleanup;
  if( pParse->nested==0 ) sqlite3VdbeCountChanges(v);
  sqlite3BeginWriteOperation(pParse, 1, iDb);

  


  if( isView ){
    Select *pView;
    pView = sqlite3SelectDup(pTab->pSelect);
    sqlite3Select(pParse, pView, SRT_VirtualTab, iCur, 0, 0, 0, 0);
    sqlite3SelectDelete(pView);
  }

  

  pWInfo = sqlite3WhereBegin(pParse, pTabList, pWhere, 0);
  if( pWInfo==0 ) goto update_cleanup;

  

  sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
  sqlite3VdbeAddOp(v, OP_FifoWrite, 0, 0);

  

  sqlite3WhereEnd(pWInfo);

  

  if( db->flags & SQLITE_CountRows && !pParse->trigStack ){
    sqlite3VdbeAddOp(v, OP_Integer, 0, 0);
  }

  if( triggers_exist ){
    

    sqlite3VdbeAddOp(v, OP_OpenPseudo, oldIdx, 0);
    sqlite3VdbeAddOp(v, OP_SetNumColumns, oldIdx, pTab->nCol);
    sqlite3VdbeAddOp(v, OP_OpenPseudo, newIdx, 0);
    sqlite3VdbeAddOp(v, OP_SetNumColumns, newIdx, pTab->nCol);

    

    addr = sqlite3VdbeAddOp(v, OP_FifoRead, 0, 0);

    if( !isView ){
      sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
      sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
      


      sqlite3OpenTable(pParse, iCur, iDb, pTab, OP_OpenRead);
    }
    sqlite3VdbeAddOp(v, OP_MoveGe, iCur, 0);

    

    sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
    sqlite3VdbeAddOp(v, OP_RowData, iCur, 0);
    sqlite3VdbeAddOp(v, OP_Insert, oldIdx, 0);

    

    if( chngRowid ){
      sqlite3ExprCodeAndCache(pParse, pRowidExpr);
    }else{
      sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
    }
    for(i=0; i<pTab->nCol; i++){
      if( i==pTab->iPKey ){
        sqlite3VdbeAddOp(v, OP_Null, 0, 0);
        continue;
      }
      j = aXRef[i];
      if( j<0 ){
        sqlite3VdbeAddOp(v, OP_Column, iCur, i);
        sqlite3ColumnDefault(v, pTab, i);
      }else{
        sqlite3ExprCodeAndCache(pParse, pChanges->a[j].pExpr);
      }
    }
    sqlite3VdbeAddOp(v, OP_MakeRecord, pTab->nCol, 0);
    if( !isView ){
      sqlite3TableAffinityStr(v, pTab);
    }
    if( pParse->nErr ) goto update_cleanup;
    sqlite3VdbeAddOp(v, OP_Insert, newIdx, 0);
    if( !isView ){
      sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
    }

    

    if( sqlite3CodeRowTrigger(pParse, TK_UPDATE, pChanges, TRIGGER_BEFORE, pTab,
          newIdx, oldIdx, onError, addr) ){
      goto update_cleanup;
    }
  }

  if( !isView ){
    





    sqlite3OpenTable(pParse, iCur, iDb, pTab, OP_OpenWrite); 
    if( onError==OE_Replace ){
      openAll = 1;
    }else{
      openAll = 0;
      for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
        if( pIdx->onError==OE_Replace ){
          openAll = 1;
          break;
        }
      }
    }
    for(i=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, i++){
      if( openAll || aIdxUsed[i] ){
        KeyInfo *pKey = sqlite3IndexKeyinfo(pParse, pIdx);
        sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
        sqlite3VdbeOp3(v, OP_OpenWrite, iCur+i+1, pIdx->tnum,
                       (char*)pKey, P3_KEYINFO_HANDOFF);
        assert( pParse->nTab>iCur+i+1 );
      }
    }

    





    if( !triggers_exist ){
      addr = sqlite3VdbeAddOp(v, OP_FifoRead, 0, 0);
      sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
    }
    sqlite3VdbeAddOp(v, OP_NotExists, iCur, addr);

    



    if( chngRowid ){
      sqlite3ExprCode(pParse, pRowidExpr);
      sqlite3VdbeAddOp(v, OP_MustBeInt, 0, 0);
    }

    

    for(i=0; i<pTab->nCol; i++){
      if( i==pTab->iPKey ){
        sqlite3VdbeAddOp(v, OP_Null, 0, 0);
        continue;
      }
      j = aXRef[i];
      if( j<0 ){
        sqlite3VdbeAddOp(v, OP_Column, iCur, i);
        sqlite3ColumnDefault(v, pTab, i);
      }else{
        sqlite3ExprCode(pParse, pChanges->a[j].pExpr);
      }
    }

    

    sqlite3GenerateConstraintChecks(pParse, pTab, iCur, aIdxUsed, chngRowid, 1,
                                   onError, addr);

    

    sqlite3GenerateRowIndexDelete(v, pTab, iCur, aIdxUsed);

    

    if( chngRowid ){
      sqlite3VdbeAddOp(v, OP_Delete, iCur, 0);
    }

    

    sqlite3CompleteInsertion(pParse, pTab, iCur, aIdxUsed, chngRowid, 1, -1);
  }

  

  if( db->flags & SQLITE_CountRows && !pParse->trigStack){
    sqlite3VdbeAddOp(v, OP_AddImm, 1, 0);
  }

  


  if( triggers_exist ){
    if( !isView ){
      for(i=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, i++){
        if( openAll || aIdxUsed[i] )
          sqlite3VdbeAddOp(v, OP_Close, iCur+i+1, 0);
      }
      sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
    }
    if( sqlite3CodeRowTrigger(pParse, TK_UPDATE, pChanges, TRIGGER_AFTER, pTab, 
          newIdx, oldIdx, onError, addr) ){
      goto update_cleanup;
    }
  }

  


  sqlite3VdbeAddOp(v, OP_Goto, 0, addr);
  sqlite3VdbeJumpHere(v, addr);

  
  if( !triggers_exist ){
    for(i=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, i++){
      if( openAll || aIdxUsed[i] ){
        sqlite3VdbeAddOp(v, OP_Close, iCur+i+1, 0);
      }
    }
    sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
  }else{
    sqlite3VdbeAddOp(v, OP_Close, newIdx, 0);
    sqlite3VdbeAddOp(v, OP_Close, oldIdx, 0);
  }

  




  if( db->flags & SQLITE_CountRows && !pParse->trigStack && pParse->nested==0 ){
    sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
    sqlite3VdbeSetNumCols(v, 1);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "rows updated", P3_STATIC);
  }

update_cleanup:
  sqlite3AuthContextPop(&sContext);
  sqliteFree(apIdx);
  sqliteFree(aXRef);
  sqlite3SrcListDelete(pTabList);
  sqlite3ExprListDelete(pChanges);
  sqlite3ExprDelete(pWhere);
  return;
}
