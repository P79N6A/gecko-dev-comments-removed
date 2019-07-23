















#include "sqliteInt.h"






Table *sqlite3SrcListLookup(Parse *pParse, SrcList *pSrc){
  Table *pTab = 0;
  int i;
  struct SrcList_item *pItem;
  for(i=0, pItem=pSrc->a; i<pSrc->nSrc; i++, pItem++){
    pTab = sqlite3LocateTable(pParse, pItem->zName, pItem->zDatabase);
    sqlite3DeleteTable(pParse->db, pItem->pTab);
    pItem->pTab = pTab;
    if( pTab ){
      pTab->nRef++;
    }
  }
  return pTab;
}






int sqlite3IsReadOnly(Parse *pParse, Table *pTab, int viewOk){
  if( pTab->readOnly && (pParse->db->flags & SQLITE_WriteSchema)==0
        && pParse->nested==0 ){
    sqlite3ErrorMsg(pParse, "table %s may not be modified", pTab->zName);
    return 1;
  }
#ifndef SQLITE_OMIT_VIEW
  if( !viewOk && pTab->pSelect ){
    sqlite3ErrorMsg(pParse,"cannot modify %s because it is a view",pTab->zName);
    return 1;
  }
#endif
  return 0;
}




void sqlite3OpenTable(
  Parse *p,       
  int iCur,       
  int iDb,        
  Table *pTab,    
  int opcode      
){
  Vdbe *v = sqlite3GetVdbe(p);
  assert( opcode==OP_OpenWrite || opcode==OP_OpenRead );
  sqlite3TableLock(p, iDb, pTab->tnum, (opcode==OP_OpenWrite), pTab->zName);
  sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
  VdbeComment((v, "# %s", pTab->zName));
  sqlite3VdbeAddOp(v, opcode, iCur, pTab->tnum);
  sqlite3VdbeAddOp(v, OP_SetNumColumns, iCur, pTab->nCol);
}









void sqlite3DeleteFrom(
  Parse *pParse,         
  SrcList *pTabList,     
  Expr *pWhere           
){
  Vdbe *v;               
  Table *pTab;           
  const char *zDb;       
  int end, addr = 0;     
  int i;                 
  WhereInfo *pWInfo;     
  Index *pIdx;           
  int iCur;              
  sqlite3 *db;           
  AuthContext sContext;  
  int oldIdx = -1;       
  NameContext sNC;       
  int iDb;

#ifndef SQLITE_OMIT_TRIGGER
  int isView;                  
  int triggers_exist = 0;      
#endif

  sContext.pParse = 0;
  if( pParse->nErr || sqlite3MallocFailed() ){
    goto delete_from_cleanup;
  }
  db = pParse->db;
  assert( pTabList->nSrc==1 );

  




  pTab = sqlite3SrcListLookup(pParse, pTabList);
  if( pTab==0 )  goto delete_from_cleanup;

  


#ifndef SQLITE_OMIT_TRIGGER
  triggers_exist = sqlite3TriggersExist(pParse, pTab, TK_DELETE, 0);
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
    goto delete_from_cleanup;
  }
  iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
  assert( iDb<db->nDb );
  zDb = db->aDb[iDb].zName;
  if( sqlite3AuthCheck(pParse, SQLITE_DELETE, pTab->zName, 0, zDb) ){
    goto delete_from_cleanup;
  }

  

  if( isView && sqlite3ViewGetColumnNames(pParse, pTab) ){
    goto delete_from_cleanup;
  }

  

  if( triggers_exist ){ 
    oldIdx = pParse->nTab++;
  }

  

  assert( pTabList->nSrc==1 );
  iCur = pTabList->a[0].iCursor = pParse->nTab++;
  memset(&sNC, 0, sizeof(sNC));
  sNC.pParse = pParse;
  sNC.pSrcList = pTabList;
  if( sqlite3ExprResolveNames(&sNC, pWhere) ){
    goto delete_from_cleanup;
  }

  

  if( isView ){
    sqlite3AuthContextPush(pParse, &sContext, pTab->zName);
  }

  

  v = sqlite3GetVdbe(pParse);
  if( v==0 ){
    goto delete_from_cleanup;
  }
  if( pParse->nested==0 ) sqlite3VdbeCountChanges(v);
  sqlite3BeginWriteOperation(pParse, triggers_exist, iDb);

  


  if( isView ){
    Select *pView = sqlite3SelectDup(pTab->pSelect);
    sqlite3Select(pParse, pView, SRT_VirtualTab, iCur, 0, 0, 0, 0);
    sqlite3SelectDelete(pView);
  }

  


  if( db->flags & SQLITE_CountRows ){
    sqlite3VdbeAddOp(v, OP_Integer, 0, 0);
  }

  



  if( pWhere==0 && !triggers_exist ){
    if( db->flags & SQLITE_CountRows ){
      

      int endOfLoop = sqlite3VdbeMakeLabel(v);
      int addr2;
      if( !isView ){
        sqlite3OpenTable(pParse, iCur, iDb, pTab, OP_OpenRead);
      }
      sqlite3VdbeAddOp(v, OP_Rewind, iCur, sqlite3VdbeCurrentAddr(v)+2);
      addr2 = sqlite3VdbeAddOp(v, OP_AddImm, 1, 0);
      sqlite3VdbeAddOp(v, OP_Next, iCur, addr2);
      sqlite3VdbeResolveLabel(v, endOfLoop);
      sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
    }
    if( !isView ){
      sqlite3VdbeAddOp(v, OP_Clear, pTab->tnum, iDb);
      if( !pParse->nested ){
        sqlite3VdbeChangeP3(v, -1, pTab->zName, P3_STATIC);
      }
      for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
        assert( pIdx->pSchema==pTab->pSchema );
        sqlite3VdbeAddOp(v, OP_Clear, pIdx->tnum, iDb);
      }
    }
  }

  


  else{
    

    pWInfo = sqlite3WhereBegin(pParse, pTabList, pWhere, 0);
    if( pWInfo==0 ) goto delete_from_cleanup;

    

    sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
    sqlite3VdbeAddOp(v, OP_FifoWrite, 0, 0);
    if( db->flags & SQLITE_CountRows ){
      sqlite3VdbeAddOp(v, OP_AddImm, 1, 0);
    }

    

    sqlite3WhereEnd(pWInfo);

    

    if( triggers_exist ){
      sqlite3VdbeAddOp(v, OP_OpenPseudo, oldIdx, 0);
      sqlite3VdbeAddOp(v, OP_SetNumColumns, oldIdx, pTab->nCol);
    }

    



    end = sqlite3VdbeMakeLabel(v);

    


    if( triggers_exist ){
      addr = sqlite3VdbeAddOp(v, OP_FifoRead, 0, end);
      if( !isView ){
        sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
        sqlite3OpenTable(pParse, iCur, iDb, pTab, OP_OpenRead);
      }
      sqlite3VdbeAddOp(v, OP_MoveGe, iCur, 0);
      sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
      sqlite3VdbeAddOp(v, OP_RowData, iCur, 0);
      sqlite3VdbeAddOp(v, OP_Insert, oldIdx, 0);
      if( !isView ){
        sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
      }

      (void)sqlite3CodeRowTrigger(pParse, TK_DELETE, 0, TRIGGER_BEFORE, pTab,
          -1, oldIdx, (pParse->trigStack)?pParse->trigStack->orconf:OE_Default,
          addr);
    }

    if( !isView ){
      





      sqlite3OpenTableAndIndices(pParse, pTab, iCur, OP_OpenWrite);

      

      if( !triggers_exist ){ 
        addr = sqlite3VdbeAddOp(v, OP_FifoRead, 0, end);
      }

      
      sqlite3GenerateRowDelete(db, v, pTab, iCur, pParse->nested==0);
    }

    


    if( triggers_exist ){
      if( !isView ){
        for(i=1, pIdx=pTab->pIndex; pIdx; i++, pIdx=pIdx->pNext){
          sqlite3VdbeAddOp(v, OP_Close, iCur + i, pIdx->tnum);
        }
        sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
      }
      (void)sqlite3CodeRowTrigger(pParse, TK_DELETE, 0, TRIGGER_AFTER, pTab, -1,
          oldIdx, (pParse->trigStack)?pParse->trigStack->orconf:OE_Default,
          addr);
    }

    
    sqlite3VdbeAddOp(v, OP_Goto, 0, addr);
    sqlite3VdbeResolveLabel(v, end);

    
    if( !triggers_exist ){
      for(i=1, pIdx=pTab->pIndex; pIdx; i++, pIdx=pIdx->pNext){
        sqlite3VdbeAddOp(v, OP_Close, iCur + i, pIdx->tnum);
      }
      sqlite3VdbeAddOp(v, OP_Close, iCur, 0);
    }
  }

  




  if( db->flags & SQLITE_CountRows && pParse->nested==0 && !pParse->trigStack ){
    sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
    sqlite3VdbeSetNumCols(v, 1);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "rows deleted", P3_STATIC);
  }

delete_from_cleanup:
  sqlite3AuthContextPop(&sContext);
  sqlite3SrcListDelete(pTabList);
  sqlite3ExprDelete(pWhere);
  return;
}





















void sqlite3GenerateRowDelete(
  sqlite3 *db,       
  Vdbe *v,           
  Table *pTab,       
  int iCur,          
  int count          
){
  int addr;
  addr = sqlite3VdbeAddOp(v, OP_NotExists, iCur, 0);
  sqlite3GenerateRowIndexDelete(v, pTab, iCur, 0);
  sqlite3VdbeAddOp(v, OP_Delete, iCur, (count?OPFLAG_NCHANGE:0));
  if( count ){
    sqlite3VdbeChangeP3(v, -1, pTab->zName, P3_STATIC);
  }
  sqlite3VdbeJumpHere(v, addr);
}

















void sqlite3GenerateRowIndexDelete(
  Vdbe *v,           
  Table *pTab,       
  int iCur,          
  char *aIdxUsed     
){
  int i;
  Index *pIdx;

  for(i=1, pIdx=pTab->pIndex; pIdx; i++, pIdx=pIdx->pNext){
    if( aIdxUsed!=0 && aIdxUsed[i-1]==0 ) continue;
    sqlite3GenerateIndexKey(v, pIdx, iCur);
    sqlite3VdbeAddOp(v, OP_IdxDelete, iCur+i, 0);
  }
}







void sqlite3GenerateIndexKey(
  Vdbe *v,           
  Index *pIdx,       
  int iCur           
){
  int j;
  Table *pTab = pIdx->pTable;

  sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
  for(j=0; j<pIdx->nColumn; j++){
    int idx = pIdx->aiColumn[j];
    if( idx==pTab->iPKey ){
      sqlite3VdbeAddOp(v, OP_Dup, j, 0);
    }else{
      sqlite3VdbeAddOp(v, OP_Column, iCur, idx);
      sqlite3ColumnDefault(v, pTab, idx);
    }
  }
  sqlite3VdbeAddOp(v, OP_MakeIdxRec, pIdx->nColumn, 0);
  sqlite3IndexAffinityStr(v, pIdx);
}
