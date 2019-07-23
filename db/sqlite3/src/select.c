















#include "sqliteInt.h"






static void clearSelect(Select *p){
  sqlite3ExprListDelete(p->pEList);
  sqlite3SrcListDelete(p->pSrc);
  sqlite3ExprDelete(p->pWhere);
  sqlite3ExprListDelete(p->pGroupBy);
  sqlite3ExprDelete(p->pHaving);
  sqlite3ExprListDelete(p->pOrderBy);
  sqlite3SelectDelete(p->pPrior);
  sqlite3ExprDelete(p->pLimit);
  sqlite3ExprDelete(p->pOffset);
}






Select *sqlite3SelectNew(
  ExprList *pEList,     
  SrcList *pSrc,        
  Expr *pWhere,         
  ExprList *pGroupBy,   
  Expr *pHaving,        
  ExprList *pOrderBy,   
  int isDistinct,       
  Expr *pLimit,         
  Expr *pOffset         
){
  Select *pNew;
  Select standin;
  pNew = sqliteMalloc( sizeof(*pNew) );
  assert( !pOffset || pLimit );   
  if( pNew==0 ){
    pNew = &standin;
    memset(pNew, 0, sizeof(*pNew));
  }
  if( pEList==0 ){
    pEList = sqlite3ExprListAppend(0, sqlite3Expr(TK_ALL,0,0,0), 0);
  }
  pNew->pEList = pEList;
  pNew->pSrc = pSrc;
  pNew->pWhere = pWhere;
  pNew->pGroupBy = pGroupBy;
  pNew->pHaving = pHaving;
  pNew->pOrderBy = pOrderBy;
  pNew->isDistinct = isDistinct;
  pNew->op = TK_SELECT;
  pNew->pLimit = pLimit;
  pNew->pOffset = pOffset;
  pNew->iLimit = -1;
  pNew->iOffset = -1;
  pNew->addrOpenVirt[0] = -1;
  pNew->addrOpenVirt[1] = -1;
  pNew->addrOpenVirt[2] = -1;
  if( pNew==&standin) {
    clearSelect(pNew);
    pNew = 0;
  }
  return pNew;
}




void sqlite3SelectDelete(Select *p){
  if( p ){
    clearSelect(p);
    sqliteFree(p);
  }
}


















int sqlite3JoinType(Parse *pParse, Token *pA, Token *pB, Token *pC){
  int jointype = 0;
  Token *apAll[3];
  Token *p;
  static const struct {
    const char zKeyword[8];
    u8 nChar;
    u8 code;
  } keywords[] = {
    { "natural", 7, JT_NATURAL },
    { "left",    4, JT_LEFT|JT_OUTER },
    { "right",   5, JT_RIGHT|JT_OUTER },
    { "full",    4, JT_LEFT|JT_RIGHT|JT_OUTER },
    { "outer",   5, JT_OUTER },
    { "inner",   5, JT_INNER },
    { "cross",   5, JT_INNER|JT_CROSS },
  };
  int i, j;
  apAll[0] = pA;
  apAll[1] = pB;
  apAll[2] = pC;
  for(i=0; i<3 && apAll[i]; i++){
    p = apAll[i];
    for(j=0; j<sizeof(keywords)/sizeof(keywords[0]); j++){
      if( p->n==keywords[j].nChar 
          && sqlite3StrNICmp((char*)p->z, keywords[j].zKeyword, p->n)==0 ){
        jointype |= keywords[j].code;
        break;
      }
    }
    if( j>=sizeof(keywords)/sizeof(keywords[0]) ){
      jointype |= JT_ERROR;
      break;
    }
  }
  if(
     (jointype & (JT_INNER|JT_OUTER))==(JT_INNER|JT_OUTER) ||
     (jointype & JT_ERROR)!=0
  ){
    const char *zSp1 = " ";
    const char *zSp2 = " ";
    if( pB==0 ){ zSp1++; }
    if( pC==0 ){ zSp2++; }
    sqlite3ErrorMsg(pParse, "unknown or unsupported join type: "
       "%T%s%T%s%T", pA, zSp1, pB, zSp2, pC);
    jointype = JT_INNER;
  }else if( jointype & JT_RIGHT ){
    sqlite3ErrorMsg(pParse, 
      "RIGHT and FULL OUTER JOINs are not currently supported");
    jointype = JT_INNER;
  }
  return jointype;
}





static int columnIndex(Table *pTab, const char *zCol){
  int i;
  for(i=0; i<pTab->nCol; i++){
    if( sqlite3StrICmp(pTab->aCol[i].zName, zCol)==0 ) return i;
  }
  return -1;
}




static void setToken(Token *p, const char *z){
  p->z = (u8*)z;
  p->n = z ? strlen(z) : 0;
  p->dyn = 0;
}




static Expr *createIdExpr(const char *zName){
  Token dummy;
  setToken(&dummy, zName);
  return sqlite3Expr(TK_ID, 0, 0, &dummy);
}






static void addWhereTerm(
  const char *zCol,        
  const Table *pTab1,      
  const char *zAlias1,     
  const Table *pTab2,      
  const char *zAlias2,     
  int iRightJoinTable,     
  Expr **ppExpr            
){
  Expr *pE1a, *pE1b, *pE1c;
  Expr *pE2a, *pE2b, *pE2c;
  Expr *pE;

  pE1a = createIdExpr(zCol);
  pE2a = createIdExpr(zCol);
  if( zAlias1==0 ){
    zAlias1 = pTab1->zName;
  }
  pE1b = createIdExpr(zAlias1);
  if( zAlias2==0 ){
    zAlias2 = pTab2->zName;
  }
  pE2b = createIdExpr(zAlias2);
  pE1c = sqlite3Expr(TK_DOT, pE1b, pE1a, 0);
  pE2c = sqlite3Expr(TK_DOT, pE2b, pE2a, 0);
  pE = sqlite3Expr(TK_EQ, pE1c, pE2c, 0);
  ExprSetProperty(pE, EP_FromJoin);
  pE->iRightJoinTable = iRightJoinTable;
  *ppExpr = sqlite3ExprAnd(*ppExpr, pE);
}



























static void setJoinExpr(Expr *p, int iTable){
  while( p ){
    ExprSetProperty(p, EP_FromJoin);
    p->iRightJoinTable = iTable;
    setJoinExpr(p->pLeft, iTable);
    p = p->pRight;
  } 
}















static int sqliteProcessJoin(Parse *pParse, Select *p){
  SrcList *pSrc;                  
  int i, j;                       
  struct SrcList_item *pLeft;     
  struct SrcList_item *pRight;    

  pSrc = p->pSrc;
  pLeft = &pSrc->a[0];
  pRight = &pLeft[1];
  for(i=0; i<pSrc->nSrc-1; i++, pRight++, pLeft++){
    Table *pLeftTab = pLeft->pTab;
    Table *pRightTab = pRight->pTab;

    if( pLeftTab==0 || pRightTab==0 ) continue;

    


    if( pLeft->jointype & JT_NATURAL ){
      if( pLeft->pOn || pLeft->pUsing ){
        sqlite3ErrorMsg(pParse, "a NATURAL join may not have "
           "an ON or USING clause", 0);
        return 1;
      }
      for(j=0; j<pLeftTab->nCol; j++){
        char *zName = pLeftTab->aCol[j].zName;
        if( columnIndex(pRightTab, zName)>=0 ){
          addWhereTerm(zName, pLeftTab, pLeft->zAlias, 
                              pRightTab, pRight->zAlias,
                              pRight->iCursor, &p->pWhere);
          
        }
      }
    }

    

    if( pLeft->pOn && pLeft->pUsing ){
      sqlite3ErrorMsg(pParse, "cannot have both ON and USING "
        "clauses in the same join");
      return 1;
    }

    


    if( pLeft->pOn ){
      setJoinExpr(pLeft->pOn, pRight->iCursor);
      p->pWhere = sqlite3ExprAnd(p->pWhere, pLeft->pOn);
      pLeft->pOn = 0;
    }

    






    if( pLeft->pUsing ){
      IdList *pList = pLeft->pUsing;
      for(j=0; j<pList->nId; j++){
        char *zName = pList->a[j].zName;
        if( columnIndex(pLeftTab, zName)<0 || columnIndex(pRightTab, zName)<0 ){
          sqlite3ErrorMsg(pParse, "cannot join using column %s - column "
            "not present in both tables", zName);
          return 1;
        }
        addWhereTerm(zName, pLeftTab, pLeft->zAlias, 
                            pRightTab, pRight->zAlias,
                            pRight->iCursor, &p->pWhere);
      }
    }
  }
  return 0;
}





static void pushOntoSorter(
  Parse *pParse,         
  ExprList *pOrderBy,    
  Select *pSelect        
){
  Vdbe *v = pParse->pVdbe;
  sqlite3ExprCodeExprList(pParse, pOrderBy);
  sqlite3VdbeAddOp(v, OP_Sequence, pOrderBy->iECursor, 0);
  sqlite3VdbeAddOp(v, OP_Pull, pOrderBy->nExpr + 1, 0);
  sqlite3VdbeAddOp(v, OP_MakeRecord, pOrderBy->nExpr + 2, 0);
  sqlite3VdbeAddOp(v, OP_IdxInsert, pOrderBy->iECursor, 0);
  if( pSelect->iLimit>=0 ){
    int addr1, addr2;
    addr1 = sqlite3VdbeAddOp(v, OP_IfMemZero, pSelect->iLimit+1, 0);
    sqlite3VdbeAddOp(v, OP_MemIncr, -1, pSelect->iLimit+1);
    addr2 = sqlite3VdbeAddOp(v, OP_Goto, 0, 0);
    sqlite3VdbeJumpHere(v, addr1);
    sqlite3VdbeAddOp(v, OP_Last, pOrderBy->iECursor, 0);
    sqlite3VdbeAddOp(v, OP_Delete, pOrderBy->iECursor, 0);
    sqlite3VdbeJumpHere(v, addr2);
    pSelect->iLimit = -1;
  }
}




static void codeOffset(
  Vdbe *v,          
  Select *p,        
  int iContinue,    
  int nPop          
){
  if( p->iOffset>=0 && iContinue!=0 ){
    int addr;
    sqlite3VdbeAddOp(v, OP_MemIncr, -1, p->iOffset);
    addr = sqlite3VdbeAddOp(v, OP_IfMemNeg, p->iOffset, 0);
    if( nPop>0 ){
      sqlite3VdbeAddOp(v, OP_Pop, nPop, 0);
    }
    sqlite3VdbeAddOp(v, OP_Goto, 0, iContinue);
    VdbeComment((v, "# skip OFFSET records"));
    sqlite3VdbeJumpHere(v, addr);
  }
}










static void codeDistinct(
  Vdbe *v,           
  int iTab,          
  int addrRepeat,    
  int N              
){
  sqlite3VdbeAddOp(v, OP_MakeRecord, -N, 0);
  sqlite3VdbeAddOp(v, OP_Distinct, iTab, sqlite3VdbeCurrentAddr(v)+3);
  sqlite3VdbeAddOp(v, OP_Pop, N+1, 0);
  sqlite3VdbeAddOp(v, OP_Goto, 0, addrRepeat);
  VdbeComment((v, "# skip indistinct records"));
  sqlite3VdbeAddOp(v, OP_IdxInsert, iTab, 0);
}











static int selectInnerLoop(
  Parse *pParse,          
  Select *p,              
  ExprList *pEList,       
  int srcTab,             
  int nColumn,            
  ExprList *pOrderBy,     
  int distinct,           
  int eDest,              
  int iParm,              
  int iContinue,          
  int iBreak,             
  char *aff               
){
  Vdbe *v = pParse->pVdbe;
  int i;
  int hasDistinct;        

  if( v==0 ) return 0;
  assert( pEList!=0 );

  


  hasDistinct = distinct>=0 && pEList->nExpr>0;
  if( pOrderBy==0 && !hasDistinct ){
    codeOffset(v, p, iContinue, 0);
  }

  

  if( nColumn>0 ){
    for(i=0; i<nColumn; i++){
      sqlite3VdbeAddOp(v, OP_Column, srcTab, i);
    }
  }else{
    nColumn = pEList->nExpr;
    sqlite3ExprCodeExprList(pParse, pEList);
  }

  



  if( hasDistinct ){
    assert( pEList!=0 );
    assert( pEList->nExpr==nColumn );
    codeDistinct(v, distinct, iContinue, nColumn);
    if( pOrderBy==0 ){
      codeOffset(v, p, iContinue, nColumn);
    }
  }

  switch( eDest ){
    


#ifndef SQLITE_OMIT_COMPOUND_SELECT
    case SRT_Union: {
      sqlite3VdbeAddOp(v, OP_MakeRecord, nColumn, 0);
      if( aff ){
        sqlite3VdbeChangeP3(v, -1, aff, P3_STATIC);
      }
      sqlite3VdbeAddOp(v, OP_IdxInsert, iParm, 0);
      break;
    }

    



    case SRT_Except: {
      int addr;
      addr = sqlite3VdbeAddOp(v, OP_MakeRecord, nColumn, 0);
      sqlite3VdbeChangeP3(v, -1, aff, P3_STATIC);
      sqlite3VdbeAddOp(v, OP_NotFound, iParm, addr+3);
      sqlite3VdbeAddOp(v, OP_Delete, iParm, 0);
      break;
    }
#endif

    

    case SRT_Table:
    case SRT_VirtualTab: {
      sqlite3VdbeAddOp(v, OP_MakeRecord, nColumn, 0);
      if( pOrderBy ){
        pushOntoSorter(pParse, pOrderBy, p);
      }else{
        sqlite3VdbeAddOp(v, OP_NewRowid, iParm, 0);
        sqlite3VdbeAddOp(v, OP_Pull, 1, 0);
        sqlite3VdbeAddOp(v, OP_Insert, iParm, 0);
      }
      break;
    }

#ifndef SQLITE_OMIT_SUBQUERY
    



    case SRT_Set: {
      int addr1 = sqlite3VdbeCurrentAddr(v);
      int addr2;

      assert( nColumn==1 );
      sqlite3VdbeAddOp(v, OP_NotNull, -1, addr1+3);
      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      addr2 = sqlite3VdbeAddOp(v, OP_Goto, 0, 0);
      if( pOrderBy ){
        



        pushOntoSorter(pParse, pOrderBy, p);
      }else{
        char affinity = (iParm>>16)&0xFF;
        affinity = sqlite3CompareAffinity(pEList->a[0].pExpr, affinity);
        sqlite3VdbeOp3(v, OP_MakeRecord, 1, 0, &affinity, 1);
        sqlite3VdbeAddOp(v, OP_IdxInsert, (iParm&0x0000FFFF), 0);
      }
      sqlite3VdbeJumpHere(v, addr2);
      break;
    }

    

    case SRT_Exists: {
      sqlite3VdbeAddOp(v, OP_MemInt, 1, iParm);
      sqlite3VdbeAddOp(v, OP_Pop, nColumn, 0);
      
      break;
    }

    



    case SRT_Mem: {
      assert( nColumn==1 );
      if( pOrderBy ){
        pushOntoSorter(pParse, pOrderBy, p);
      }else{
        sqlite3VdbeAddOp(v, OP_MemStore, iParm, 1);
        
      }
      break;
    }
#endif 

    



    case SRT_Subroutine:
    case SRT_Callback: {
      if( pOrderBy ){
        sqlite3VdbeAddOp(v, OP_MakeRecord, nColumn, 0);
        pushOntoSorter(pParse, pOrderBy, p);
      }else if( eDest==SRT_Subroutine ){
        sqlite3VdbeAddOp(v, OP_Gosub, 0, iParm);
      }else{
        sqlite3VdbeAddOp(v, OP_Callback, nColumn, 0);
      }
      break;
    }

#if !defined(SQLITE_OMIT_TRIGGER)
    




    default: {
      assert( eDest==SRT_Discard );
      sqlite3VdbeAddOp(v, OP_Pop, nColumn, 0);
      break;
    }
#endif
  }

  

  if( p->iLimit>=0 && pOrderBy==0 ){
    sqlite3VdbeAddOp(v, OP_MemIncr, -1, p->iLimit);
    sqlite3VdbeAddOp(v, OP_IfMemZero, p->iLimit, iBreak);
  }
  return 0;
}
















static KeyInfo *keyInfoFromExprList(Parse *pParse, ExprList *pList){
  sqlite3 *db = pParse->db;
  int nExpr;
  KeyInfo *pInfo;
  struct ExprList_item *pItem;
  int i;

  nExpr = pList->nExpr;
  pInfo = sqliteMalloc( sizeof(*pInfo) + nExpr*(sizeof(CollSeq*)+1) );
  if( pInfo ){
    pInfo->aSortOrder = (u8*)&pInfo->aColl[nExpr];
    pInfo->nField = nExpr;
    pInfo->enc = ENC(db);
    for(i=0, pItem=pList->a; i<nExpr; i++, pItem++){
      CollSeq *pColl;
      pColl = sqlite3ExprCollSeq(pParse, pItem->pExpr);
      if( !pColl ){
        pColl = db->pDfltColl;
      }
      pInfo->aColl[i] = pColl;
      pInfo->aSortOrder[i] = pItem->sortOrder;
    }
  }
  return pInfo;
}








static void generateSortTail(
  Parse *pParse,   
  Select *p,       
  Vdbe *v,         
  int nColumn,     
  int eDest,       
  int iParm        
){
  int brk = sqlite3VdbeMakeLabel(v);
  int cont = sqlite3VdbeMakeLabel(v);
  int addr;
  int iTab;
  int pseudoTab;
  ExprList *pOrderBy = p->pOrderBy;

  iTab = pOrderBy->iECursor;
  if( eDest==SRT_Callback || eDest==SRT_Subroutine ){
    pseudoTab = pParse->nTab++;
    sqlite3VdbeAddOp(v, OP_OpenPseudo, pseudoTab, 0);
    sqlite3VdbeAddOp(v, OP_SetNumColumns, pseudoTab, nColumn);
  }
  addr = 1 + sqlite3VdbeAddOp(v, OP_Sort, iTab, brk);
  codeOffset(v, p, cont, 0);
  if( eDest==SRT_Callback || eDest==SRT_Subroutine ){
    sqlite3VdbeAddOp(v, OP_Integer, 1, 0);
  }
  sqlite3VdbeAddOp(v, OP_Column, iTab, pOrderBy->nExpr + 1);
  switch( eDest ){
    case SRT_Table:
    case SRT_VirtualTab: {
      sqlite3VdbeAddOp(v, OP_NewRowid, iParm, 0);
      sqlite3VdbeAddOp(v, OP_Pull, 1, 0);
      sqlite3VdbeAddOp(v, OP_Insert, iParm, 0);
      break;
    }
#ifndef SQLITE_OMIT_SUBQUERY
    case SRT_Set: {
      assert( nColumn==1 );
      sqlite3VdbeAddOp(v, OP_NotNull, -1, sqlite3VdbeCurrentAddr(v)+3);
      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      sqlite3VdbeAddOp(v, OP_Goto, 0, sqlite3VdbeCurrentAddr(v)+3);
      sqlite3VdbeOp3(v, OP_MakeRecord, 1, 0, "c", P3_STATIC);
      sqlite3VdbeAddOp(v, OP_IdxInsert, (iParm&0x0000FFFF), 0);
      break;
    }
    case SRT_Mem: {
      assert( nColumn==1 );
      sqlite3VdbeAddOp(v, OP_MemStore, iParm, 1);
      
      break;
    }
#endif
    case SRT_Callback:
    case SRT_Subroutine: {
      int i;
      sqlite3VdbeAddOp(v, OP_Insert, pseudoTab, 0);
      for(i=0; i<nColumn; i++){
        sqlite3VdbeAddOp(v, OP_Column, pseudoTab, i);
      }
      if( eDest==SRT_Callback ){
        sqlite3VdbeAddOp(v, OP_Callback, nColumn, 0);
      }else{
        sqlite3VdbeAddOp(v, OP_Gosub, 0, iParm);
      }
      break;
    }
    default: {
      
      break;
    }
  }

  

  if( p->iLimit>=0 ){
    sqlite3VdbeAddOp(v, OP_MemIncr, -1, p->iLimit);
    sqlite3VdbeAddOp(v, OP_IfMemZero, p->iLimit, brk);
  }

  

  sqlite3VdbeResolveLabel(v, cont);
  sqlite3VdbeAddOp(v, OP_Next, iTab, addr);
  sqlite3VdbeResolveLabel(v, brk);
  if( eDest==SRT_Callback || eDest==SRT_Subroutine ){
    sqlite3VdbeAddOp(v, OP_Close, pseudoTab, 0);
  }

}



















static const char *columnType(
  NameContext *pNC, 
  Expr *pExpr,
  const char **pzOriginDb,
  const char **pzOriginTab,
  const char **pzOriginCol
){
  char const *zType = 0;
  char const *zOriginDb = 0;
  char const *zOriginTab = 0;
  char const *zOriginCol = 0;
  int j;
  if( pExpr==0 || pNC->pSrcList==0 ) return 0;

  



  assert( pExpr->op!=TK_AS );

  switch( pExpr->op ){
    case TK_AGG_COLUMN:
    case TK_COLUMN: {
      



      Table *pTab = 0;            
      Select *pS = 0;             
      int iCol = pExpr->iColumn;  
      while( pNC && !pTab ){
        SrcList *pTabList = pNC->pSrcList;
        for(j=0;j<pTabList->nSrc && pTabList->a[j].iCursor!=pExpr->iTable;j++);
        if( j<pTabList->nSrc ){
          pTab = pTabList->a[j].pTab;
          pS = pTabList->a[j].pSelect;
        }else{
          pNC = pNC->pNext;
        }
      }

      if( pTab==0 ){
        









        zType = "TEXT";
        break;
      }

      assert( pTab );
#ifndef SQLITE_OMIT_SUBQUERY
      if( pS ){
        



        if( iCol>=0 && iCol<pS->pEList->nExpr ){
          



          NameContext sNC;
          Expr *p = pS->pEList->a[iCol].pExpr;
          sNC.pSrcList = pS->pSrc;
          sNC.pNext = 0;
          sNC.pParse = pNC->pParse;
          zType = columnType(&sNC, p, &zOriginDb, &zOriginTab, &zOriginCol); 
        }
      }else
#endif
      if( pTab->pSchema ){
        
        assert( !pS );
        if( iCol<0 ) iCol = pTab->iPKey;
        assert( iCol==-1 || (iCol>=0 && iCol<pTab->nCol) );
        if( iCol<0 ){
          zType = "INTEGER";
          zOriginCol = "rowid";
        }else{
          zType = pTab->aCol[iCol].zType;
          zOriginCol = pTab->aCol[iCol].zName;
        }
        zOriginTab = pTab->zName;
        if( pNC->pParse ){
          int iDb = sqlite3SchemaToIndex(pNC->pParse->db, pTab->pSchema);
          zOriginDb = pNC->pParse->db->aDb[iDb].zName;
        }
      }
      break;
    }
#ifndef SQLITE_OMIT_SUBQUERY
    case TK_SELECT: {
      



      NameContext sNC;
      Select *pS = pExpr->pSelect;
      Expr *p = pS->pEList->a[0].pExpr;
      sNC.pSrcList = pS->pSrc;
      sNC.pNext = pNC;
      sNC.pParse = pNC->pParse;
      zType = columnType(&sNC, p, &zOriginDb, &zOriginTab, &zOriginCol); 
      break;
    }
#endif
  }
  
  if( pzOriginDb ){
    assert( pzOriginTab && pzOriginCol );
    *pzOriginDb = zOriginDb;
    *pzOriginTab = zOriginTab;
    *pzOriginCol = zOriginCol;
  }
  return zType;
}





static void generateColumnTypes(
  Parse *pParse,      
  SrcList *pTabList,  
  ExprList *pEList    
){
  Vdbe *v = pParse->pVdbe;
  int i;
  NameContext sNC;
  sNC.pSrcList = pTabList;
  sNC.pParse = pParse;
  for(i=0; i<pEList->nExpr; i++){
    Expr *p = pEList->a[i].pExpr;
    const char *zOrigDb = 0;
    const char *zOrigTab = 0;
    const char *zOrigCol = 0;
    const char *zType = columnType(&sNC, p, &zOrigDb, &zOrigTab, &zOrigCol);

    



    sqlite3VdbeSetColName(v, i, COLNAME_DECLTYPE, zType, P3_TRANSIENT);
    sqlite3VdbeSetColName(v, i, COLNAME_DATABASE, zOrigDb, P3_TRANSIENT);
    sqlite3VdbeSetColName(v, i, COLNAME_TABLE, zOrigTab, P3_TRANSIENT);
    sqlite3VdbeSetColName(v, i, COLNAME_COLUMN, zOrigCol, P3_TRANSIENT);
  }
}






static void generateColumnNames(
  Parse *pParse,      
  SrcList *pTabList,  
  ExprList *pEList    
){
  Vdbe *v = pParse->pVdbe;
  int i, j;
  sqlite3 *db = pParse->db;
  int fullNames, shortNames;

#ifndef SQLITE_OMIT_EXPLAIN
  
  if( pParse->explain ){
    return;
  }
#endif

  assert( v!=0 );
  if( pParse->colNamesSet || v==0 || sqlite3MallocFailed() ) return;
  pParse->colNamesSet = 1;
  fullNames = (db->flags & SQLITE_FullColNames)!=0;
  shortNames = (db->flags & SQLITE_ShortColNames)!=0;
  sqlite3VdbeSetNumCols(v, pEList->nExpr);
  for(i=0; i<pEList->nExpr; i++){
    Expr *p;
    p = pEList->a[i].pExpr;
    if( p==0 ) continue;
    if( pEList->a[i].zName ){
      char *zName = pEList->a[i].zName;
      sqlite3VdbeSetColName(v, i, COLNAME_NAME, zName, strlen(zName));
      continue;
    }
    if( p->op==TK_COLUMN && pTabList ){
      Table *pTab;
      char *zCol;
      int iCol = p->iColumn;
      for(j=0; j<pTabList->nSrc && pTabList->a[j].iCursor!=p->iTable; j++){}
      assert( j<pTabList->nSrc );
      pTab = pTabList->a[j].pTab;
      if( iCol<0 ) iCol = pTab->iPKey;
      assert( iCol==-1 || (iCol>=0 && iCol<pTab->nCol) );
      if( iCol<0 ){
        zCol = "rowid";
      }else{
        zCol = pTab->aCol[iCol].zName;
      }
      if( !shortNames && !fullNames && p->span.z && p->span.z[0] ){
        sqlite3VdbeSetColName(v, i, COLNAME_NAME, (char*)p->span.z, p->span.n);
      }else if( fullNames || (!shortNames && pTabList->nSrc>1) ){
        char *zName = 0;
        char *zTab;
 
        zTab = pTabList->a[j].zAlias;
        if( fullNames || zTab==0 ) zTab = pTab->zName;
        sqlite3SetString(&zName, zTab, ".", zCol, (char*)0);
        sqlite3VdbeSetColName(v, i, COLNAME_NAME, zName, P3_DYNAMIC);
      }else{
        sqlite3VdbeSetColName(v, i, COLNAME_NAME, zCol, strlen(zCol));
      }
    }else if( p->span.z && p->span.z[0] ){
      sqlite3VdbeSetColName(v, i, COLNAME_NAME, (char*)p->span.z, p->span.n);
      
    }else{
      char zName[30];
      assert( p->op!=TK_COLUMN || pTabList==0 );
      sprintf(zName, "column%d", i+1);
      sqlite3VdbeSetColName(v, i, COLNAME_NAME, zName, 0);
    }
  }
  generateColumnTypes(pParse, pTabList, pEList);
}

#ifndef SQLITE_OMIT_COMPOUND_SELECT



static const char *selectOpName(int id){
  char *z;
  switch( id ){
    case TK_ALL:       z = "UNION ALL";   break;
    case TK_INTERSECT: z = "INTERSECT";   break;
    case TK_EXCEPT:    z = "EXCEPT";      break;
    default:           z = "UNION";       break;
  }
  return z;
}
#endif 




static int prepSelectStmt(Parse*, Select*);





Table *sqlite3ResultSetOfSelect(Parse *pParse, char *zTabName, Select *pSelect){
  Table *pTab;
  int i, j;
  ExprList *pEList;
  Column *aCol, *pCol;

  while( pSelect->pPrior ) pSelect = pSelect->pPrior;
  if( prepSelectStmt(pParse, pSelect) ){
    return 0;
  }
  if( sqlite3SelectResolve(pParse, pSelect, 0) ){
    return 0;
  }
  pTab = sqliteMalloc( sizeof(Table) );
  if( pTab==0 ){
    return 0;
  }
  pTab->nRef = 1;
  pTab->zName = zTabName ? sqliteStrDup(zTabName) : 0;
  pEList = pSelect->pEList;
  pTab->nCol = pEList->nExpr;
  assert( pTab->nCol>0 );
  pTab->aCol = aCol = sqliteMalloc( sizeof(pTab->aCol[0])*pTab->nCol );
  for(i=0, pCol=aCol; i<pTab->nCol; i++, pCol++){
    Expr *p, *pR;
    char *zType;
    char *zName;
    char *zBasename;
    CollSeq *pColl;
    int cnt;
    NameContext sNC;
    
    

    p = pEList->a[i].pExpr;
    assert( p->pRight==0 || p->pRight->token.z==0 || p->pRight->token.z[0]!=0 );
    if( (zName = pEList->a[i].zName)!=0 ){
      
      zName = sqliteStrDup(zName);
    }else if( p->op==TK_DOT 
              && (pR=p->pRight)!=0 && pR->token.z && pR->token.z[0] ){
      
      zName = sqlite3MPrintf("%T", &pR->token);
    }else if( p->span.z && p->span.z[0] ){
      
      zName = sqlite3MPrintf("%T", &p->span);
    }else{
      
      zName = sqlite3MPrintf("column%d", i+1);
    }
    sqlite3Dequote(zName);
    if( sqlite3MallocFailed() ){
      sqliteFree(zName);
      sqlite3DeleteTable(0, pTab);
      return 0;
    }

    


    zBasename = zName;
    for(j=cnt=0; j<i; j++){
      if( sqlite3StrICmp(aCol[j].zName, zName)==0 ){
        zName = sqlite3MPrintf("%s:%d", zBasename, ++cnt);
        j = -1;
        if( zName==0 ) break;
      }
    }
    if( zBasename!=zName ){
      sqliteFree(zBasename);
    }
    pCol->zName = zName;

    


    memset(&sNC, 0, sizeof(sNC));
    sNC.pSrcList = pSelect->pSrc;
    zType = sqliteStrDup(columnType(&sNC, p, 0, 0, 0));
    pCol->zType = zType;
    pCol->affinity = sqlite3ExprAffinity(p);
    pColl = sqlite3ExprCollSeq(pParse, p);
    if( pColl ){
      pCol->zColl = sqliteStrDup(pColl->zName);
    }
  }
  pTab->iPKey = -1;
  return pTab;
}



























static int prepSelectStmt(Parse *pParse, Select *p){
  int i, j, k, rc;
  SrcList *pTabList;
  ExprList *pEList;
  struct SrcList_item *pFrom;

  if( p==0 || p->pSrc==0 || sqlite3MallocFailed() ){
    return 1;
  }
  pTabList = p->pSrc;
  pEList = p->pEList;

  


  sqlite3SrcListAssignCursors(pParse, p->pSrc);

  



  for(i=0, pFrom=pTabList->a; i<pTabList->nSrc; i++, pFrom++){
    Table *pTab;
    if( pFrom->pTab!=0 ){
      

      assert( i==0 );
      return 0;
    }
    if( pFrom->zName==0 ){
#ifndef SQLITE_OMIT_SUBQUERY
      
      assert( pFrom->pSelect!=0 );
      if( pFrom->zAlias==0 ){
        pFrom->zAlias =
          sqlite3MPrintf("sqlite_subquery_%p_", (void*)pFrom->pSelect);
      }
      assert( pFrom->pTab==0 );
      pFrom->pTab = pTab = 
        sqlite3ResultSetOfSelect(pParse, pFrom->zAlias, pFrom->pSelect);
      if( pTab==0 ){
        return 1;
      }
      



      pTab->isTransient = 1;
#endif
    }else{
      
      assert( pFrom->pTab==0 );
      pFrom->pTab = pTab = 
        sqlite3LocateTable(pParse,pFrom->zName,pFrom->zDatabase);
      if( pTab==0 ){
        return 1;
      }
      pTab->nRef++;
#ifndef SQLITE_OMIT_VIEW
      if( pTab->pSelect ){
        
        if( sqlite3ViewGetColumnNames(pParse, pTab) ){
          return 1;
        }
        




        if( pFrom->pSelect==0 ){
          pFrom->pSelect = sqlite3SelectDup(pTab->pSelect);
        }
      }
#endif
    }
  }

  

  if( sqliteProcessJoin(pParse, p) ) return 1;

  









  for(k=0; k<pEList->nExpr; k++){
    Expr *pE = pEList->a[k].pExpr;
    if( pE->op==TK_ALL ) break;
    if( pE->op==TK_DOT && pE->pRight && pE->pRight->op==TK_ALL
         && pE->pLeft && pE->pLeft->op==TK_ID ) break;
  }
  rc = 0;
  if( k<pEList->nExpr ){
    




    struct ExprList_item *a = pEList->a;
    ExprList *pNew = 0;
    int flags = pParse->db->flags;
    int longNames = (flags & SQLITE_FullColNames)!=0 &&
                      (flags & SQLITE_ShortColNames)==0;

    for(k=0; k<pEList->nExpr; k++){
      Expr *pE = a[k].pExpr;
      if( pE->op!=TK_ALL &&
           (pE->op!=TK_DOT || pE->pRight==0 || pE->pRight->op!=TK_ALL) ){
        

        pNew = sqlite3ExprListAppend(pNew, a[k].pExpr, 0);
        if( pNew ){
          pNew->a[pNew->nExpr-1].zName = a[k].zName;
        }else{
          rc = 1;
        }
        a[k].pExpr = 0;
        a[k].zName = 0;
      }else{
        

        int tableSeen = 0;      
        char *zTName;            
        if( pE->op==TK_DOT && pE->pLeft ){
          zTName = sqlite3NameFromToken(&pE->pLeft->token);
        }else{
          zTName = 0;
        }
        for(i=0, pFrom=pTabList->a; i<pTabList->nSrc; i++, pFrom++){
          Table *pTab = pFrom->pTab;
          char *zTabName = pFrom->zAlias;
          if( zTabName==0 || zTabName[0]==0 ){ 
            zTabName = pTab->zName;
          }
          if( zTName && (zTabName==0 || zTabName[0]==0 || 
                 sqlite3StrICmp(zTName, zTabName)!=0) ){
            continue;
          }
          tableSeen = 1;
          for(j=0; j<pTab->nCol; j++){
            Expr *pExpr, *pRight;
            char *zName = pTab->aCol[j].zName;

            if( i>0 ){
              struct SrcList_item *pLeft = &pTabList->a[i-1];
              if( (pLeft->jointype & JT_NATURAL)!=0 &&
                        columnIndex(pLeft->pTab, zName)>=0 ){
                

                continue;
              }
              if( sqlite3IdListIndex(pLeft->pUsing, zName)>=0 ){
                

                continue;
              }
            }
            pRight = sqlite3Expr(TK_ID, 0, 0, 0);
            if( pRight==0 ) break;
            setToken(&pRight->token, zName);
            if( zTabName && (longNames || pTabList->nSrc>1) ){
              Expr *pLeft = sqlite3Expr(TK_ID, 0, 0, 0);
              pExpr = sqlite3Expr(TK_DOT, pLeft, pRight, 0);
              if( pExpr==0 ) break;
              setToken(&pLeft->token, zTabName);
              setToken(&pExpr->span, sqlite3MPrintf("%s.%s", zTabName, zName));
              pExpr->span.dyn = 1;
              pExpr->token.z = 0;
              pExpr->token.n = 0;
              pExpr->token.dyn = 0;
            }else{
              pExpr = pRight;
              pExpr->span = pExpr->token;
            }
            if( longNames ){
              pNew = sqlite3ExprListAppend(pNew, pExpr, &pExpr->span);
            }else{
              pNew = sqlite3ExprListAppend(pNew, pExpr, &pRight->token);
            }
          }
        }
        if( !tableSeen ){
          if( zTName ){
            sqlite3ErrorMsg(pParse, "no such table: %s", zTName);
          }else{
            sqlite3ErrorMsg(pParse, "no tables specified");
          }
          rc = 1;
        }
        sqliteFree(zTName);
      }
    }
    sqlite3ExprListDelete(pEList);
    p->pEList = pNew;
  }
  return rc;
}

#ifndef SQLITE_OMIT_COMPOUND_SELECT













static int matchOrderbyToColumn(
  Parse *pParse,          
  Select *pSelect,        
  ExprList *pOrderBy,     
  int iTable,             
  int mustComplete        
){
  int nErr = 0;
  int i, j;
  ExprList *pEList;

  if( pSelect==0 || pOrderBy==0 ) return 1;
  if( mustComplete ){
    for(i=0; i<pOrderBy->nExpr; i++){ pOrderBy->a[i].done = 0; }
  }
  if( prepSelectStmt(pParse, pSelect) ){
    return 1;
  }
  if( pSelect->pPrior ){
    if( matchOrderbyToColumn(pParse, pSelect->pPrior, pOrderBy, iTable, 0) ){
      return 1;
    }
  }
  pEList = pSelect->pEList;
  for(i=0; i<pOrderBy->nExpr; i++){
    Expr *pE = pOrderBy->a[i].pExpr;
    int iCol = -1;
    if( pOrderBy->a[i].done ) continue;
    if( sqlite3ExprIsInteger(pE, &iCol) ){
      if( iCol<=0 || iCol>pEList->nExpr ){
        sqlite3ErrorMsg(pParse,
          "ORDER BY position %d should be between 1 and %d",
          iCol, pEList->nExpr);
        nErr++;
        break;
      }
      if( !mustComplete ) continue;
      iCol--;
    }
    for(j=0; iCol<0 && j<pEList->nExpr; j++){
      if( pEList->a[j].zName && (pE->op==TK_ID || pE->op==TK_STRING) ){
        char *zName, *zLabel;
        zName = pEList->a[j].zName;
        zLabel = sqlite3NameFromToken(&pE->token);
        assert( zLabel!=0 );
        if( sqlite3StrICmp(zName, zLabel)==0 ){ 
          iCol = j;
        }
        sqliteFree(zLabel);
      }
      if( iCol<0 && sqlite3ExprCompare(pE, pEList->a[j].pExpr) ){
        iCol = j;
      }
    }
    if( iCol>=0 ){
      pE->op = TK_COLUMN;
      pE->iColumn = iCol;
      pE->iTable = iTable;
      pE->iAgg = -1;
      pOrderBy->a[i].done = 1;
    }
    if( iCol<0 && mustComplete ){
      sqlite3ErrorMsg(pParse,
        "ORDER BY term number %d does not match any result column", i+1);
      nErr++;
      break;
    }
  }
  return nErr;  
}
#endif 





Vdbe *sqlite3GetVdbe(Parse *pParse){
  Vdbe *v = pParse->pVdbe;
  if( v==0 ){
    v = pParse->pVdbe = sqlite3VdbeCreate(pParse->db);
  }
  return v;
}




















static void computeLimitRegisters(Parse *pParse, Select *p, int iBreak){
  Vdbe *v = 0;
  int iLimit = 0;
  int iOffset;
  int addr1, addr2;

  





  if( p->pLimit ){
    p->iLimit = iLimit = pParse->nMem;
    pParse->nMem += 2;
    v = sqlite3GetVdbe(pParse);
    if( v==0 ) return;
    sqlite3ExprCode(pParse, p->pLimit);
    sqlite3VdbeAddOp(v, OP_MustBeInt, 0, 0);
    sqlite3VdbeAddOp(v, OP_MemStore, iLimit, 0);
    VdbeComment((v, "# LIMIT counter"));
    sqlite3VdbeAddOp(v, OP_IfMemZero, iLimit, iBreak);
  }
  if( p->pOffset ){
    p->iOffset = iOffset = pParse->nMem++;
    v = sqlite3GetVdbe(pParse);
    if( v==0 ) return;
    sqlite3ExprCode(pParse, p->pOffset);
    sqlite3VdbeAddOp(v, OP_MustBeInt, 0, 0);
    sqlite3VdbeAddOp(v, OP_MemStore, iOffset, p->pLimit==0);
    VdbeComment((v, "# OFFSET counter"));
    addr1 = sqlite3VdbeAddOp(v, OP_IfMemPos, iOffset, 0);
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
    sqlite3VdbeAddOp(v, OP_Integer, 0, 0);
    sqlite3VdbeJumpHere(v, addr1);
    if( p->pLimit ){
      sqlite3VdbeAddOp(v, OP_Add, 0, 0);
    }
  }
  if( p->pLimit ){
    addr1 = sqlite3VdbeAddOp(v, OP_IfMemPos, iLimit, 0);
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
    sqlite3VdbeAddOp(v, OP_MemInt, -1, iLimit+1);
    addr2 = sqlite3VdbeAddOp(v, OP_Goto, 0, 0);
    sqlite3VdbeJumpHere(v, addr1);
    sqlite3VdbeAddOp(v, OP_MemStore, iLimit+1, 1);
    VdbeComment((v, "# LIMIT+OFFSET"));
    sqlite3VdbeJumpHere(v, addr2);
  }
}




static void createSortingIndex(Parse *pParse, Select *p, ExprList *pOrderBy){
  if( pOrderBy ){
    int addr;
    assert( pOrderBy->iECursor==0 );
    pOrderBy->iECursor = pParse->nTab++;
    addr = sqlite3VdbeAddOp(pParse->pVdbe, OP_OpenVirtual,
                            pOrderBy->iECursor, pOrderBy->nExpr+1);
    assert( p->addrOpenVirt[2] == -1 );
    p->addrOpenVirt[2] = addr;
  }
}

#ifndef SQLITE_OMIT_COMPOUND_SELECT








static CollSeq *multiSelectCollSeq(Parse *pParse, Select *p, int iCol){
  CollSeq *pRet;
  if( p->pPrior ){
    pRet = multiSelectCollSeq(pParse, p->pPrior, iCol);
  }else{
    pRet = 0;
  }
  if( pRet==0 ){
    pRet = sqlite3ExprCollSeq(pParse, p->pEList->a[iCol].pExpr);
  }
  return pRet;
}
#endif 

#ifndef SQLITE_OMIT_COMPOUND_SELECT






























static int multiSelect(
  Parse *pParse,        
  Select *p,            
  int eDest,            
  int iParm,            
  char *aff             
){
  int rc = SQLITE_OK;   
  Select *pPrior;       
  Vdbe *v;              
  int nCol;             
  ExprList *pOrderBy;   
  int aSetP2[2];        
  int nSetP2 = 0;       

  


  if( p==0 || p->pPrior==0 ){
    rc = 1;
    goto multi_select_end;
  }
  pPrior = p->pPrior;
  assert( pPrior->pRightmost!=pPrior );
  assert( pPrior->pRightmost==p->pRightmost );
  if( pPrior->pOrderBy ){
    sqlite3ErrorMsg(pParse,"ORDER BY clause should come after %s not before",
      selectOpName(p->op));
    rc = 1;
    goto multi_select_end;
  }
  if( pPrior->pLimit ){
    sqlite3ErrorMsg(pParse,"LIMIT clause should come after %s not before",
      selectOpName(p->op));
    rc = 1;
    goto multi_select_end;
  }

  

  v = sqlite3GetVdbe(pParse);
  if( v==0 ){
    rc = 1;
    goto multi_select_end;
  }

  

  if( eDest==SRT_VirtualTab ){
    assert( p->pEList );
    assert( nSetP2<sizeof(aSetP2)/sizeof(aSetP2[0]) );
    aSetP2[nSetP2++] = sqlite3VdbeAddOp(v, OP_OpenVirtual, iParm, 0);
    eDest = SRT_Table;
  }

  

  pOrderBy = p->pOrderBy;
  switch( p->op ){
    case TK_ALL: {
      if( pOrderBy==0 ){
        int addr = 0;
        assert( !pPrior->pLimit );
        pPrior->pLimit = p->pLimit;
        pPrior->pOffset = p->pOffset;
        rc = sqlite3Select(pParse, pPrior, eDest, iParm, 0, 0, 0, aff);
        p->pLimit = 0;
        p->pOffset = 0;
        if( rc ){
          goto multi_select_end;
        }
        p->pPrior = 0;
        p->iLimit = pPrior->iLimit;
        p->iOffset = pPrior->iOffset;
        if( p->iLimit>=0 ){
          addr = sqlite3VdbeAddOp(v, OP_IfMemZero, p->iLimit, 0);
          VdbeComment((v, "# Jump ahead if LIMIT reached"));
        }
        rc = sqlite3Select(pParse, p, eDest, iParm, 0, 0, 0, aff);
        p->pPrior = pPrior;
        if( rc ){
          goto multi_select_end;
        }
        if( addr ){
          sqlite3VdbeJumpHere(v, addr);
        }
        break;
      }
      
    }
    case TK_EXCEPT:
    case TK_UNION: {
      int unionTab;    
      int op = 0;      
      int priorOp;     
      Expr *pLimit, *pOffset; 
      int addr;

      priorOp = p->op==TK_ALL ? SRT_Table : SRT_Union;
      if( eDest==priorOp && pOrderBy==0 && !p->pLimit && !p->pOffset ){
        


        unionTab = iParm;
      }else{
        


        unionTab = pParse->nTab++;
        if( pOrderBy && matchOrderbyToColumn(pParse, p, pOrderBy, unionTab,1) ){
          rc = 1;
          goto multi_select_end;
        }
        addr = sqlite3VdbeAddOp(v, OP_OpenVirtual, unionTab, 0);
        if( priorOp==SRT_Table ){
          assert( nSetP2<sizeof(aSetP2)/sizeof(aSetP2[0]) );
          aSetP2[nSetP2++] = addr;
        }else{
          assert( p->addrOpenVirt[0] == -1 );
          p->addrOpenVirt[0] = addr;
          p->pRightmost->usesVirt = 1;
        }
        createSortingIndex(pParse, p, pOrderBy);
        assert( p->pEList );
      }

      

      assert( !pPrior->pOrderBy );
      rc = sqlite3Select(pParse, pPrior, priorOp, unionTab, 0, 0, 0, aff);
      if( rc ){
        goto multi_select_end;
      }

      

      switch( p->op ){
         case TK_EXCEPT:  op = SRT_Except;   break;
         case TK_UNION:   op = SRT_Union;    break;
         case TK_ALL:     op = SRT_Table;    break;
      }
      p->pPrior = 0;
      p->pOrderBy = 0;
      p->disallowOrderBy = pOrderBy!=0;
      pLimit = p->pLimit;
      p->pLimit = 0;
      pOffset = p->pOffset;
      p->pOffset = 0;
      rc = sqlite3Select(pParse, p, op, unionTab, 0, 0, 0, aff);
      p->pPrior = pPrior;
      p->pOrderBy = pOrderBy;
      sqlite3ExprDelete(p->pLimit);
      p->pLimit = pLimit;
      p->pOffset = pOffset;
      p->iLimit = -1;
      p->iOffset = -1;
      if( rc ){
        goto multi_select_end;
      }


      

      
      if( eDest!=priorOp || unionTab!=iParm ){
        int iCont, iBreak, iStart;
        assert( p->pEList );
        if( eDest==SRT_Callback ){
          Select *pFirst = p;
          while( pFirst->pPrior ) pFirst = pFirst->pPrior;
          generateColumnNames(pParse, 0, pFirst->pEList);
        }
        iBreak = sqlite3VdbeMakeLabel(v);
        iCont = sqlite3VdbeMakeLabel(v);
        computeLimitRegisters(pParse, p, iBreak);
        sqlite3VdbeAddOp(v, OP_Rewind, unionTab, iBreak);
        iStart = sqlite3VdbeCurrentAddr(v);
        rc = selectInnerLoop(pParse, p, p->pEList, unionTab, p->pEList->nExpr,
                             pOrderBy, -1, eDest, iParm, 
                             iCont, iBreak, 0);
        if( rc ){
          rc = 1;
          goto multi_select_end;
        }
        sqlite3VdbeResolveLabel(v, iCont);
        sqlite3VdbeAddOp(v, OP_Next, unionTab, iStart);
        sqlite3VdbeResolveLabel(v, iBreak);
        sqlite3VdbeAddOp(v, OP_Close, unionTab, 0);
      }
      break;
    }
    case TK_INTERSECT: {
      int tab1, tab2;
      int iCont, iBreak, iStart;
      Expr *pLimit, *pOffset;
      int addr;

      



      tab1 = pParse->nTab++;
      tab2 = pParse->nTab++;
      if( pOrderBy && matchOrderbyToColumn(pParse,p,pOrderBy,tab1,1) ){
        rc = 1;
        goto multi_select_end;
      }
      createSortingIndex(pParse, p, pOrderBy);

      addr = sqlite3VdbeAddOp(v, OP_OpenVirtual, tab1, 0);
      assert( p->addrOpenVirt[0] == -1 );
      p->addrOpenVirt[0] = addr;
      p->pRightmost->usesVirt = 1;
      assert( p->pEList );

      

      rc = sqlite3Select(pParse, pPrior, SRT_Union, tab1, 0, 0, 0, aff);
      if( rc ){
        goto multi_select_end;
      }

      

      addr = sqlite3VdbeAddOp(v, OP_OpenVirtual, tab2, 0);
      assert( p->addrOpenVirt[1] == -1 );
      p->addrOpenVirt[1] = addr;
      p->pPrior = 0;
      pLimit = p->pLimit;
      p->pLimit = 0;
      pOffset = p->pOffset;
      p->pOffset = 0;
      rc = sqlite3Select(pParse, p, SRT_Union, tab2, 0, 0, 0, aff);
      p->pPrior = pPrior;
      sqlite3ExprDelete(p->pLimit);
      p->pLimit = pLimit;
      p->pOffset = pOffset;
      if( rc ){
        goto multi_select_end;
      }

      


      assert( p->pEList );
      if( eDest==SRT_Callback ){
        Select *pFirst = p;
        while( pFirst->pPrior ) pFirst = pFirst->pPrior;
        generateColumnNames(pParse, 0, pFirst->pEList);
      }
      iBreak = sqlite3VdbeMakeLabel(v);
      iCont = sqlite3VdbeMakeLabel(v);
      computeLimitRegisters(pParse, p, iBreak);
      sqlite3VdbeAddOp(v, OP_Rewind, tab1, iBreak);
      iStart = sqlite3VdbeAddOp(v, OP_RowKey, tab1, 0);
      sqlite3VdbeAddOp(v, OP_NotFound, tab2, iCont);
      rc = selectInnerLoop(pParse, p, p->pEList, tab1, p->pEList->nExpr,
                             pOrderBy, -1, eDest, iParm, 
                             iCont, iBreak, 0);
      if( rc ){
        rc = 1;
        goto multi_select_end;
      }
      sqlite3VdbeResolveLabel(v, iCont);
      sqlite3VdbeAddOp(v, OP_Next, tab1, iStart);
      sqlite3VdbeResolveLabel(v, iBreak);
      sqlite3VdbeAddOp(v, OP_Close, tab2, 0);
      sqlite3VdbeAddOp(v, OP_Close, tab1, 0);
      break;
    }
  }

  


  assert( p->pEList && pPrior->pEList );
  if( p->pEList->nExpr!=pPrior->pEList->nExpr ){
    sqlite3ErrorMsg(pParse, "SELECTs to the left and right of %s"
      " do not have the same number of result columns", selectOpName(p->op));
    rc = 1;
    goto multi_select_end;
  }

  

  nCol = p->pEList->nExpr;
  while( nSetP2 ){
    sqlite3VdbeChangeP2(v, aSetP2[--nSetP2], nCol);
  }

  









  if( pOrderBy || p->usesVirt ){
    int i;                        
    KeyInfo *pKeyInfo;            
    Select *pLoop;                
    CollSeq **apColl;
    CollSeq **aCopy;

    assert( p->pRightmost==p );
    pKeyInfo = sqliteMalloc(sizeof(*pKeyInfo)+nCol*2*sizeof(CollSeq*) + nCol);
    if( !pKeyInfo ){
      rc = SQLITE_NOMEM;
      goto multi_select_end;
    }

    pKeyInfo->enc = ENC(pParse->db);
    pKeyInfo->nField = nCol;

    for(i=0, apColl=pKeyInfo->aColl; i<nCol; i++, apColl++){
      *apColl = multiSelectCollSeq(pParse, p, i);
      if( 0==*apColl ){
        *apColl = pParse->db->pDfltColl;
      }
    }

    for(pLoop=p; pLoop; pLoop=pLoop->pPrior){
      for(i=0; i<2; i++){
        int addr = pLoop->addrOpenVirt[i];
        if( addr<0 ){
          

          assert( pLoop->addrOpenVirt[1]<0 );
          break;
        }
        sqlite3VdbeChangeP2(v, addr, nCol);
        sqlite3VdbeChangeP3(v, addr, (char*)pKeyInfo, P3_KEYINFO);
      }
    }

    if( pOrderBy ){
      struct ExprList_item *pOTerm = pOrderBy->a;
      int nOrderByExpr = pOrderBy->nExpr;
      int addr;
      u8 *pSortOrder;

      aCopy = &pKeyInfo->aColl[nCol];
      pSortOrder = pKeyInfo->aSortOrder = (u8*)&aCopy[nCol];
      memcpy(aCopy, pKeyInfo->aColl, nCol*sizeof(CollSeq*));
      apColl = pKeyInfo->aColl;
      for(i=0; i<nOrderByExpr; i++, pOTerm++, apColl++, pSortOrder++){
        Expr *pExpr = pOTerm->pExpr;
        char *zName = pOTerm->zName;
        assert( pExpr->op==TK_COLUMN && pExpr->iColumn<nCol );
        if( zName ){
          *apColl = sqlite3LocateCollSeq(pParse, zName, -1);
        }else{
          *apColl = aCopy[pExpr->iColumn];
        }
        *pSortOrder = pOTerm->sortOrder;
      }
      assert( p->pRightmost==p );
      assert( p->addrOpenVirt[2]>=0 );
      addr = p->addrOpenVirt[2];
      sqlite3VdbeChangeP2(v, addr, p->pEList->nExpr+2);
      pKeyInfo->nField = nOrderByExpr;
      sqlite3VdbeChangeP3(v, addr, (char*)pKeyInfo, P3_KEYINFO_HANDOFF);
      pKeyInfo = 0;
      generateSortTail(pParse, p, v, p->pEList->nExpr, eDest, iParm);
    }

    sqliteFree(pKeyInfo);
  }

multi_select_end:
  return rc;
}
#endif 

#ifndef SQLITE_OMIT_VIEW













static void substExprList(ExprList*,int,ExprList*);  
static void substSelect(Select *, int, ExprList *);  
static void substExpr(Expr *pExpr, int iTable, ExprList *pEList){
  if( pExpr==0 ) return;
  if( pExpr->op==TK_COLUMN && pExpr->iTable==iTable ){
    if( pExpr->iColumn<0 ){
      pExpr->op = TK_NULL;
    }else{
      Expr *pNew;
      assert( pEList!=0 && pExpr->iColumn<pEList->nExpr );
      assert( pExpr->pLeft==0 && pExpr->pRight==0 && pExpr->pList==0 );
      pNew = pEList->a[pExpr->iColumn].pExpr;
      assert( pNew!=0 );
      pExpr->op = pNew->op;
      assert( pExpr->pLeft==0 );
      pExpr->pLeft = sqlite3ExprDup(pNew->pLeft);
      assert( pExpr->pRight==0 );
      pExpr->pRight = sqlite3ExprDup(pNew->pRight);
      assert( pExpr->pList==0 );
      pExpr->pList = sqlite3ExprListDup(pNew->pList);
      pExpr->iTable = pNew->iTable;
      pExpr->iColumn = pNew->iColumn;
      pExpr->iAgg = pNew->iAgg;
      sqlite3TokenCopy(&pExpr->token, &pNew->token);
      sqlite3TokenCopy(&pExpr->span, &pNew->span);
      pExpr->pSelect = sqlite3SelectDup(pNew->pSelect);
      pExpr->flags = pNew->flags;
    }
  }else{
    substExpr(pExpr->pLeft, iTable, pEList);
    substExpr(pExpr->pRight, iTable, pEList);
    substSelect(pExpr->pSelect, iTable, pEList);
    substExprList(pExpr->pList, iTable, pEList);
  }
}
static void substExprList(ExprList *pList, int iTable, ExprList *pEList){
  int i;
  if( pList==0 ) return;
  for(i=0; i<pList->nExpr; i++){
    substExpr(pList->a[i].pExpr, iTable, pEList);
  }
}
static void substSelect(Select *p, int iTable, ExprList *pEList){
  if( !p ) return;
  substExprList(p->pEList, iTable, pEList);
  substExprList(p->pGroupBy, iTable, pEList);
  substExprList(p->pOrderBy, iTable, pEList);
  substExpr(p->pHaving, iTable, pEList);
  substExpr(p->pWhere, iTable, pEList);
}
#endif 

#ifndef SQLITE_OMIT_VIEW









































































static int flattenSubquery(
  Select *p,           
  int iFrom,           
  int isAgg,           
  int subqueryIsAgg    
){
  Select *pSub;       
  SrcList *pSrc;      
  SrcList *pSubSrc;   
  ExprList *pList;    
  int iParent;        
  int i;              
  Expr *pWhere;                    
  struct SrcList_item *pSubitem;   

  

  if( p==0 ) return 0;
  pSrc = p->pSrc;
  assert( pSrc && iFrom>=0 && iFrom<pSrc->nSrc );
  pSubitem = &pSrc->a[iFrom];
  pSub = pSubitem->pSelect;
  assert( pSub!=0 );
  if( isAgg && subqueryIsAgg ) return 0;                 
  if( subqueryIsAgg && pSrc->nSrc>1 ) return 0;          
  pSubSrc = pSub->pSrc;
  assert( pSubSrc );
  




  if( pSub->pLimit && p->pLimit ) return 0;              
  if( pSub->pOffset ) return 0;                          
  if( pSubSrc->nSrc==0 ) return 0;                       
  if( (pSub->isDistinct || pSub->pLimit) 
         && (pSrc->nSrc>1 || isAgg) ){          
     return 0;       
  }
  if( p->isDistinct && subqueryIsAgg ) return 0;         
  if( (p->disallowOrderBy || p->pOrderBy) && pSub->pOrderBy ){
     return 0;                                           
  }

  











  if( pSubSrc->nSrc>1 && iFrom>0 && (pSrc->a[iFrom-1].jointype & JT_OUTER)!=0 ){
    return 0;
  }

  












  if( iFrom>0 && (pSrc->a[iFrom-1].jointype & JT_OUTER)!=0 
      && pSub->pWhere!=0 ){
    return 0;
  }

  



  







  iParent = pSubitem->iCursor;
  {
    int nSubSrc = pSubSrc->nSrc;
    int jointype = pSubitem->jointype;

    sqlite3DeleteTable(0, pSubitem->pTab);
    sqliteFree(pSubitem->zDatabase);
    sqliteFree(pSubitem->zName);
    sqliteFree(pSubitem->zAlias);
    if( nSubSrc>1 ){
      int extra = nSubSrc - 1;
      for(i=1; i<nSubSrc; i++){
        pSrc = sqlite3SrcListAppend(pSrc, 0, 0);
      }
      p->pSrc = pSrc;
      for(i=pSrc->nSrc-1; i-extra>=iFrom; i--){
        pSrc->a[i] = pSrc->a[i-extra];
      }
    }
    for(i=0; i<nSubSrc; i++){
      pSrc->a[i+iFrom] = pSubSrc->a[i];
      memset(&pSubSrc->a[i], 0, sizeof(pSubSrc->a[i]));
    }
    pSrc->a[iFrom+nSubSrc-1].jointype = jointype;
  }

  











  pList = p->pEList;
  for(i=0; i<pList->nExpr; i++){
    Expr *pExpr;
    if( pList->a[i].zName==0 && (pExpr = pList->a[i].pExpr)->span.z!=0 ){
      pList->a[i].zName = sqliteStrNDup((char*)pExpr->span.z, pExpr->span.n);
    }
  }
  substExprList(p->pEList, iParent, pSub->pEList);
  if( isAgg ){
    substExprList(p->pGroupBy, iParent, pSub->pEList);
    substExpr(p->pHaving, iParent, pSub->pEList);
  }
  if( pSub->pOrderBy ){
    assert( p->pOrderBy==0 );
    p->pOrderBy = pSub->pOrderBy;
    pSub->pOrderBy = 0;
  }else if( p->pOrderBy ){
    substExprList(p->pOrderBy, iParent, pSub->pEList);
  }
  if( pSub->pWhere ){
    pWhere = sqlite3ExprDup(pSub->pWhere);
  }else{
    pWhere = 0;
  }
  if( subqueryIsAgg ){
    assert( p->pHaving==0 );
    p->pHaving = p->pWhere;
    p->pWhere = pWhere;
    substExpr(p->pHaving, iParent, pSub->pEList);
    p->pHaving = sqlite3ExprAnd(p->pHaving, sqlite3ExprDup(pSub->pHaving));
    assert( p->pGroupBy==0 );
    p->pGroupBy = sqlite3ExprListDup(pSub->pGroupBy);
  }else{
    substExpr(p->pWhere, iParent, pSub->pEList);
    p->pWhere = sqlite3ExprAnd(p->pWhere, pWhere);
  }

  


  p->isDistinct = p->isDistinct || pSub->isDistinct;

  





  if( pSub->pLimit ){
    p->pLimit = pSub->pLimit;
    pSub->pLimit = 0;
  }

  


  sqlite3SelectDelete(pSub);
  return 1;
}
#endif 





















static int simpleMinMaxQuery(Parse *pParse, Select *p, int eDest, int iParm){
  Expr *pExpr;
  int iCol;
  Table *pTab;
  Index *pIdx;
  int base;
  Vdbe *v;
  int seekOp;
  ExprList *pEList, *pList, eList;
  struct ExprList_item eListItem;
  SrcList *pSrc;
  int brk;
  int iDb;

  


  if( p->pGroupBy || p->pHaving || p->pWhere ) return 0;
  pSrc = p->pSrc;
  if( pSrc->nSrc!=1 ) return 0;
  pEList = p->pEList;
  if( pEList->nExpr!=1 ) return 0;
  pExpr = pEList->a[0].pExpr;
  if( pExpr->op!=TK_AGG_FUNCTION ) return 0;
  pList = pExpr->pList;
  if( pList==0 || pList->nExpr!=1 ) return 0;
  if( pExpr->token.n!=3 ) return 0;
  if( sqlite3StrNICmp((char*)pExpr->token.z,"min",3)==0 ){
    seekOp = OP_Rewind;
  }else if( sqlite3StrNICmp((char*)pExpr->token.z,"max",3)==0 ){
    seekOp = OP_Last;
  }else{
    return 0;
  }
  pExpr = pList->a[0].pExpr;
  if( pExpr->op!=TK_COLUMN ) return 0;
  iCol = pExpr->iColumn;
  pTab = pSrc->a[0].pTab;


  





  if( iCol<0 ){
    pIdx = 0;
  }else{
    CollSeq *pColl = sqlite3ExprCollSeq(pParse, pExpr);
    for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
      assert( pIdx->nColumn>=1 );
      if( pIdx->aiColumn[0]==iCol && 
          0==sqlite3StrICmp(pIdx->azColl[0], pColl->zName) ){
        break;
      }
    }
    if( pIdx==0 ) return 0;
  }

  



  v = sqlite3GetVdbe(pParse);
  if( v==0 ) return 0;

  

  if( eDest==SRT_VirtualTab ){
    sqlite3VdbeAddOp(v, OP_OpenVirtual, iParm, 1);
  }

  




  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  assert( iDb>=0 || pTab->isTransient );
  sqlite3CodeVerifySchema(pParse, iDb);
  sqlite3TableLock(pParse, iDb, pTab->tnum, 0, pTab->zName);
  base = pSrc->a[0].iCursor;
  brk = sqlite3VdbeMakeLabel(v);
  computeLimitRegisters(pParse, p, brk);
  if( pSrc->a[0].pSelect==0 ){
    sqlite3OpenTable(pParse, base, iDb, pTab, OP_OpenRead);
  }
  if( pIdx==0 ){
    sqlite3VdbeAddOp(v, seekOp, base, 0);
  }else{
    





    int iIdx;
    KeyInfo *pKey = sqlite3IndexKeyinfo(pParse, pIdx);
    iIdx = pParse->nTab++;
    assert( pIdx->pSchema==pTab->pSchema );
    sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
    sqlite3VdbeOp3(v, OP_OpenRead, iIdx, pIdx->tnum, 
        (char*)pKey, P3_KEYINFO_HANDOFF);
    if( seekOp==OP_Rewind ){
      sqlite3VdbeAddOp(v, OP_Null, 0, 0);
      sqlite3VdbeAddOp(v, OP_MakeRecord, 1, 0);
      seekOp = OP_MoveGt;
    }
    sqlite3VdbeAddOp(v, seekOp, iIdx, 0);
    sqlite3VdbeAddOp(v, OP_IdxRowid, iIdx, 0);
    sqlite3VdbeAddOp(v, OP_Close, iIdx, 0);
    sqlite3VdbeAddOp(v, OP_MoveGe, base, 0);
  }
  eList.nExpr = 1;
  memset(&eListItem, 0, sizeof(eListItem));
  eList.a = &eListItem;
  eList.a[0].pExpr = pExpr;
  selectInnerLoop(pParse, p, &eList, 0, 0, 0, -1, eDest, iParm, brk, brk, 0);
  sqlite3VdbeResolveLabel(v, brk);
  sqlite3VdbeAddOp(v, OP_Close, base, 0);
  
  return 1;
}









static int processOrderGroupBy(
  NameContext *pNC,     
  ExprList *pOrderBy,   
  const char *zType     
){
  int i;
  ExprList *pEList = pNC->pEList;     
  Parse *pParse = pNC->pParse;     
  assert( pEList );

  if( pOrderBy==0 ) return 0;
  for(i=0; i<pOrderBy->nExpr; i++){
    int iCol;
    Expr *pE = pOrderBy->a[i].pExpr;
    if( sqlite3ExprIsInteger(pE, &iCol) ){
      if( iCol>0 && iCol<=pEList->nExpr ){
        sqlite3ExprDelete(pE);
        pE = pOrderBy->a[i].pExpr = sqlite3ExprDup(pEList->a[iCol-1].pExpr);
      }else{
        sqlite3ErrorMsg(pParse, 
           "%s BY column number %d out of range - should be "
           "between 1 and %d", zType, iCol, pEList->nExpr);
        return 1;
      }
    }
    if( sqlite3ExprResolveNames(pNC, pE) ){
      return 1;
    }
  }
  return 0;
}







int sqlite3SelectResolve(
  Parse *pParse,         
  Select *p,             
  NameContext *pOuterNC  
){
  ExprList *pEList;          
  int i;                     
  NameContext sNC;           
  ExprList *pGroupBy;        

  
  if( p->isResolved ){
    assert( !pOuterNC );
    return SQLITE_OK;
  }
  p->isResolved = 1;

  
  if( pParse->nErr>0 ){
    return SQLITE_ERROR;
  }

  


  if( prepSelectStmt(pParse, p) ){
    return SQLITE_ERROR;
  }

  


  memset(&sNC, 0, sizeof(sNC));
  sNC.pParse = pParse;
  if( sqlite3ExprResolveNames(&sNC, p->pLimit) ||
      sqlite3ExprResolveNames(&sNC, p->pOffset) ){
    return SQLITE_ERROR;
  }

  


  sNC.allowAgg = 1;
  sNC.pSrcList = p->pSrc;
  sNC.pNext = pOuterNC;

  
  pEList = p->pEList;
  if( !pEList ) return SQLITE_ERROR;
  for(i=0; i<pEList->nExpr; i++){
    Expr *pX = pEList->a[i].pExpr;
    if( sqlite3ExprResolveNames(&sNC, pX) ){
      return SQLITE_ERROR;
    }
  }

  


  assert( !p->isAgg );
  pGroupBy = p->pGroupBy;
  if( pGroupBy || sNC.hasAgg ){
    p->isAgg = 1;
  }else{
    sNC.allowAgg = 0;
  }

  

  if( p->pHaving && !pGroupBy ){
    sqlite3ErrorMsg(pParse, "a GROUP BY clause is required before HAVING");
    return SQLITE_ERROR;
  }

  







  sNC.pEList = p->pEList;
  if( sqlite3ExprResolveNames(&sNC, p->pWhere) ||
      sqlite3ExprResolveNames(&sNC, p->pHaving) ||
      processOrderGroupBy(&sNC, p->pOrderBy, "ORDER") ||
      processOrderGroupBy(&sNC, pGroupBy, "GROUP")
  ){
    return SQLITE_ERROR;
  }

  

  if( pGroupBy ){
    struct ExprList_item *pItem;
  
    for(i=0, pItem=pGroupBy->a; i<pGroupBy->nExpr; i++, pItem++){
      if( ExprHasProperty(pItem->pExpr, EP_Agg) ){
        sqlite3ErrorMsg(pParse, "aggregate functions are not allowed in "
            "the GROUP BY clause");
        return SQLITE_ERROR;
      }
    }
  }

  return SQLITE_OK;
}








static void resetAccumulator(Parse *pParse, AggInfo *pAggInfo){
  Vdbe *v = pParse->pVdbe;
  int i;
  struct AggInfo_func *pFunc;
  if( pAggInfo->nFunc+pAggInfo->nColumn==0 ){
    return;
  }
  for(i=0; i<pAggInfo->nColumn; i++){
    sqlite3VdbeAddOp(v, OP_MemNull, pAggInfo->aCol[i].iMem, 0);
  }
  for(pFunc=pAggInfo->aFunc, i=0; i<pAggInfo->nFunc; i++, pFunc++){
    sqlite3VdbeAddOp(v, OP_MemNull, pFunc->iMem, 0);
    if( pFunc->iDistinct>=0 ){
      Expr *pE = pFunc->pExpr;
      if( pE->pList==0 || pE->pList->nExpr!=1 ){
        sqlite3ErrorMsg(pParse, "DISTINCT in aggregate must be followed "
           "by an expression");
        pFunc->iDistinct = -1;
      }else{
        KeyInfo *pKeyInfo = keyInfoFromExprList(pParse, pE->pList);
        sqlite3VdbeOp3(v, OP_OpenVirtual, pFunc->iDistinct, 0, 
                          (char*)pKeyInfo, P3_KEYINFO_HANDOFF);
      }
    }
  }
}





static void finalizeAggFunctions(Parse *pParse, AggInfo *pAggInfo){
  Vdbe *v = pParse->pVdbe;
  int i;
  struct AggInfo_func *pF;
  for(i=0, pF=pAggInfo->aFunc; i<pAggInfo->nFunc; i++, pF++){
    ExprList *pList = pF->pExpr->pList;
    sqlite3VdbeOp3(v, OP_AggFinal, pF->iMem, pList ? pList->nExpr : 0,
                      (void*)pF->pFunc, P3_FUNCDEF);
  }
}





static void updateAccumulator(Parse *pParse, AggInfo *pAggInfo){
  Vdbe *v = pParse->pVdbe;
  int i;
  struct AggInfo_func *pF;
  struct AggInfo_col *pC;

  pAggInfo->directMode = 1;
  for(i=0, pF=pAggInfo->aFunc; i<pAggInfo->nFunc; i++, pF++){
    int nArg;
    int addrNext = 0;
    ExprList *pList = pF->pExpr->pList;
    if( pList ){
      nArg = pList->nExpr;
      sqlite3ExprCodeExprList(pParse, pList);
    }else{
      nArg = 0;
    }
    if( pF->iDistinct>=0 ){
      addrNext = sqlite3VdbeMakeLabel(v);
      assert( nArg==1 );
      codeDistinct(v, pF->iDistinct, addrNext, 1);
    }
    if( pF->pFunc->needCollSeq ){
      CollSeq *pColl = 0;
      struct ExprList_item *pItem;
      int j;
      assert( pList!=0 );  
      for(j=0, pItem=pList->a; !pColl && j<nArg; j++, pItem++){
        pColl = sqlite3ExprCollSeq(pParse, pItem->pExpr);
      }
      if( !pColl ){
        pColl = pParse->db->pDfltColl;
      }
      sqlite3VdbeOp3(v, OP_CollSeq, 0, 0, (char *)pColl, P3_COLLSEQ);
    }
    sqlite3VdbeOp3(v, OP_AggStep, pF->iMem, nArg, (void*)pF->pFunc, P3_FUNCDEF);
    if( addrNext ){
      sqlite3VdbeResolveLabel(v, addrNext);
    }
  }
  for(i=0, pC=pAggInfo->aCol; i<pAggInfo->nAccumulator; i++, pC++){
    sqlite3ExprCode(pParse, pC->pExpr);
    sqlite3VdbeAddOp(v, OP_MemStore, pC->iMem, 1);
  }
  pAggInfo->directMode = 0;
}






















































int sqlite3Select(
  Parse *pParse,         
  Select *p,             
  int eDest,             
  int iParm,             
  Select *pParent,       
  int parentTab,         
  int *pParentAgg,       
  char *aff              
){
  int i, j;              
  WhereInfo *pWInfo;     
  Vdbe *v;               
  int isAgg;             
  ExprList *pEList;      
  SrcList *pTabList;     
  Expr *pWhere;          
  ExprList *pOrderBy;    
  ExprList *pGroupBy;    
  Expr *pHaving;         
  int isDistinct;        
  int distinct;          
  int rc = 1;            
  int addrSortIndex;     
  AggInfo sAggInfo;      
  int iEnd;              

  if( p==0 || sqlite3MallocFailed() || pParse->nErr ){
    return 1;
  }
  if( sqlite3AuthCheck(pParse, SQLITE_SELECT, 0, 0, 0) ) return 1;
  memset(&sAggInfo, 0, sizeof(sAggInfo));

#ifndef SQLITE_OMIT_COMPOUND_SELECT
  

  if( p->pPrior ){
    if( p->pRightmost==0 ){
      Select *pLoop;
      for(pLoop=p; pLoop; pLoop=pLoop->pPrior){
        pLoop->pRightmost = p;
      }
    }
    return multiSelect(pParse, p, eDest, iParm, aff);
  }
#endif

  pOrderBy = p->pOrderBy;
  if( IgnorableOrderby(eDest) ){
    p->pOrderBy = 0;
  }
  if( sqlite3SelectResolve(pParse, p, 0) ){
    goto select_end;
  }
  p->pOrderBy = pOrderBy;

  

  pTabList = p->pSrc;
  pWhere = p->pWhere;
  pGroupBy = p->pGroupBy;
  pHaving = p->pHaving;
  isAgg = p->isAgg;
  isDistinct = p->isDistinct;
  pEList = p->pEList;
  if( pEList==0 ) goto select_end;

  



  if( pParse->nErr>0 ) goto select_end;

  


#ifndef SQLITE_OMIT_SUBQUERY
  if( (eDest==SRT_Mem || eDest==SRT_Set) && pEList->nExpr>1 ){
    sqlite3ErrorMsg(pParse, "only a single result allowed for "
       "a SELECT that is part of an expression");
    goto select_end;
  }
#endif

  

  if( IgnorableOrderby(eDest) ){
    pOrderBy = 0;
  }

  

  v = sqlite3GetVdbe(pParse);
  if( v==0 ) goto select_end;

  

#if !defined(SQLITE_OMIT_SUBQUERY) || !defined(SQLITE_OMIT_VIEW)
  for(i=0; i<pTabList->nSrc; i++){
    const char *zSavedAuthContext = 0;
    int needRestoreContext;
    struct SrcList_item *pItem = &pTabList->a[i];

    if( pItem->pSelect==0 || pItem->isPopulated ) continue;
    if( pItem->zName!=0 ){
      zSavedAuthContext = pParse->zAuthContext;
      pParse->zAuthContext = pItem->zName;
      needRestoreContext = 1;
    }else{
      needRestoreContext = 0;
    }
    sqlite3Select(pParse, pItem->pSelect, SRT_VirtualTab, 
                 pItem->iCursor, p, i, &isAgg, 0);
    if( needRestoreContext ){
      pParse->zAuthContext = zSavedAuthContext;
    }
    pTabList = p->pSrc;
    pWhere = p->pWhere;
    if( !IgnorableOrderby(eDest) ){
      pOrderBy = p->pOrderBy;
    }
    pGroupBy = p->pGroupBy;
    pHaving = p->pHaving;
    isDistinct = p->isDistinct;
  }
#endif

  


  if( simpleMinMaxQuery(pParse, p, eDest, iParm) ){
    rc = 0;
    goto select_end;
  }

  


#ifndef SQLITE_OMIT_VIEW
  if( pParent && pParentAgg &&
      flattenSubquery(pParent, parentTab, *pParentAgg, isAgg) ){
    if( isAgg ) *pParentAgg = 1;
    goto select_end;
  }
#endif

  








  if( pOrderBy ){
    struct ExprList_item *pTerm;
    KeyInfo *pKeyInfo;
    for(i=0, pTerm=pOrderBy->a; i<pOrderBy->nExpr; i++, pTerm++){
      if( pTerm->zName ){
        pTerm->pExpr->pColl = sqlite3LocateCollSeq(pParse, pTerm->zName, -1);
      }
    }
    if( pParse->nErr ){
      goto select_end;
    }
    pKeyInfo = keyInfoFromExprList(pParse, pOrderBy);
    pOrderBy->iECursor = pParse->nTab++;
    p->addrOpenVirt[2] = addrSortIndex =
       sqlite3VdbeOp3(v, OP_OpenVirtual, pOrderBy->iECursor, pOrderBy->nExpr+2, 
                        (char*)pKeyInfo, P3_KEYINFO_HANDOFF);
  }else{
    addrSortIndex = -1;
  }

  

  if( eDest==SRT_VirtualTab ){
    sqlite3VdbeAddOp(v, OP_OpenVirtual, iParm, pEList->nExpr);
  }

  

  iEnd = sqlite3VdbeMakeLabel(v);
  computeLimitRegisters(pParse, p, iEnd);

  

  if( isDistinct ){
    KeyInfo *pKeyInfo;
    distinct = pParse->nTab++;
    pKeyInfo = keyInfoFromExprList(pParse, p->pEList);
    sqlite3VdbeOp3(v, OP_OpenVirtual, distinct, 0, 
                        (char*)pKeyInfo, P3_KEYINFO_HANDOFF);
  }else{
    distinct = -1;
  }

  
  if( !isAgg && pGroupBy==0 ){
    


    pWInfo = sqlite3WhereBegin(pParse, pTabList, pWhere, &pOrderBy);
    if( pWInfo==0 ) goto select_end;

    



    if( addrSortIndex>=0 && pOrderBy==0 ){
      sqlite3VdbeChangeToNoop(v, addrSortIndex, 1);
      p->addrOpenVirt[2] = -1;
    }

    

    if( selectInnerLoop(pParse, p, pEList, 0, 0, pOrderBy, distinct, eDest,
                    iParm, pWInfo->iContinue, pWInfo->iBreak, aff) ){
       goto select_end;
    }

    

    sqlite3WhereEnd(pWInfo);
  }else{
    
    NameContext sNC;    
    int iAMem;          
    int iBMem;          
    int iUseFlag;       


    int iAbortFlag;     
    int groupBySort;    


    

    int addrOutputRow;      
    int addrSetAbort;       
    int addrInitializeLoop; 
    int addrTopOfLoop;      
    int addrGroupByChange;  
    int addrProcessRow;     
    int addrEnd;            
    int addrSortingIdx;     
    int addrReset;          

    addrEnd = sqlite3VdbeMakeLabel(v);

    



    memset(&sNC, 0, sizeof(sNC));
    sNC.pParse = pParse;
    sNC.pSrcList = pTabList;
    sNC.pAggInfo = &sAggInfo;
    sAggInfo.nSortingColumn = pGroupBy ? pGroupBy->nExpr+1 : 0;
    sAggInfo.pGroupBy = pGroupBy;
    if( sqlite3ExprAnalyzeAggList(&sNC, pEList) ){
      goto select_end;
    }
    if( sqlite3ExprAnalyzeAggList(&sNC, pOrderBy) ){
      goto select_end;
    }
    if( pHaving && sqlite3ExprAnalyzeAggregates(&sNC, pHaving) ){
      goto select_end;
    }
    sAggInfo.nAccumulator = sAggInfo.nColumn;
    for(i=0; i<sAggInfo.nFunc; i++){
      if( sqlite3ExprAnalyzeAggList(&sNC, sAggInfo.aFunc[i].pExpr->pList) ){
        goto select_end;
      }
    }
    if( sqlite3MallocFailed() ) goto select_end;

    


    if( pGroupBy ){
      KeyInfo *pKeyInfo;  

      

     
      addrInitializeLoop = sqlite3VdbeMakeLabel(v);
      addrGroupByChange = sqlite3VdbeMakeLabel(v);
      addrProcessRow = sqlite3VdbeMakeLabel(v);

      




      sAggInfo.sortingIdx = pParse->nTab++;
      pKeyInfo = keyInfoFromExprList(pParse, pGroupBy);
      addrSortingIdx =
          sqlite3VdbeOp3(v, OP_OpenVirtual, sAggInfo.sortingIdx,
                         sAggInfo.nSortingColumn,
                         (char*)pKeyInfo, P3_KEYINFO_HANDOFF);

      

      iUseFlag = pParse->nMem++;
      iAbortFlag = pParse->nMem++;
      iAMem = pParse->nMem;
      pParse->nMem += pGroupBy->nExpr;
      iBMem = pParse->nMem;
      pParse->nMem += pGroupBy->nExpr;
      sqlite3VdbeAddOp(v, OP_MemInt, 0, iAbortFlag);
      VdbeComment((v, "# clear abort flag"));
      sqlite3VdbeAddOp(v, OP_MemInt, 0, iUseFlag);
      VdbeComment((v, "# indicate accumulator empty"));
      sqlite3VdbeAddOp(v, OP_Goto, 0, addrInitializeLoop);

      






      addrSetAbort = sqlite3VdbeCurrentAddr(v);
      sqlite3VdbeAddOp(v, OP_MemInt, 1, iAbortFlag);
      VdbeComment((v, "# set abort flag"));
      sqlite3VdbeAddOp(v, OP_Return, 0, 0);
      addrOutputRow = sqlite3VdbeCurrentAddr(v);
      sqlite3VdbeAddOp(v, OP_IfMemPos, iUseFlag, addrOutputRow+2);
      VdbeComment((v, "# Groupby result generator entry point"));
      sqlite3VdbeAddOp(v, OP_Return, 0, 0);
      finalizeAggFunctions(pParse, &sAggInfo);
      if( pHaving ){
        sqlite3ExprIfFalse(pParse, pHaving, addrOutputRow+1, 1);
      }
      rc = selectInnerLoop(pParse, p, p->pEList, 0, 0, pOrderBy,
                           distinct, eDest, iParm, 
                           addrOutputRow+1, addrSetAbort, aff);
      if( rc ){
        goto select_end;
      }
      sqlite3VdbeAddOp(v, OP_Return, 0, 0);
      VdbeComment((v, "# end groupby result generator"));

      

      addrReset = sqlite3VdbeCurrentAddr(v);
      resetAccumulator(pParse, &sAggInfo);
      sqlite3VdbeAddOp(v, OP_Return, 0, 0);

      




      sqlite3VdbeResolveLabel(v, addrInitializeLoop);
      sqlite3VdbeAddOp(v, OP_Gosub, 0, addrReset);
      pWInfo = sqlite3WhereBegin(pParse, pTabList, pWhere, &pGroupBy);
      if( pWInfo==0 ) goto select_end;
      if( pGroupBy==0 ){
        



        pGroupBy = p->pGroupBy;
        groupBySort = 0;
      }else{
        




        groupBySort = 1;
        sqlite3ExprCodeExprList(pParse, pGroupBy);
        sqlite3VdbeAddOp(v, OP_Sequence, sAggInfo.sortingIdx, 0);
        j = pGroupBy->nExpr+1;
        for(i=0; i<sAggInfo.nColumn; i++){
          struct AggInfo_col *pCol = &sAggInfo.aCol[i];
          if( pCol->iSorterColumn<j ) continue;
          if( pCol->iColumn<0 ){
            sqlite3VdbeAddOp(v, OP_Rowid, pCol->iTable, 0);
          }else{
            sqlite3VdbeAddOp(v, OP_Column, pCol->iTable, pCol->iColumn);
          }
          j++;
        }
        sqlite3VdbeAddOp(v, OP_MakeRecord, j, 0);
        sqlite3VdbeAddOp(v, OP_IdxInsert, sAggInfo.sortingIdx, 0);
        sqlite3WhereEnd(pWInfo);
        sqlite3VdbeAddOp(v, OP_Sort, sAggInfo.sortingIdx, addrEnd);
        VdbeComment((v, "# GROUP BY sort"));
        sAggInfo.useSortingIdx = 1;
      }

      




      addrTopOfLoop = sqlite3VdbeCurrentAddr(v);
      for(j=0; j<pGroupBy->nExpr; j++){
        if( groupBySort ){
          sqlite3VdbeAddOp(v, OP_Column, sAggInfo.sortingIdx, j);
        }else{
          sAggInfo.directMode = 1;
          sqlite3ExprCode(pParse, pGroupBy->a[j].pExpr);
        }
        sqlite3VdbeAddOp(v, OP_MemStore, iBMem+j, j<pGroupBy->nExpr-1);
      }
      for(j=pGroupBy->nExpr-1; j>=0; j--){
        if( j<pGroupBy->nExpr-1 ){
          sqlite3VdbeAddOp(v, OP_MemLoad, iBMem+j, 0);
        }
        sqlite3VdbeAddOp(v, OP_MemLoad, iAMem+j, 0);
        if( j==0 ){
          sqlite3VdbeAddOp(v, OP_Eq, 0x200, addrProcessRow);
        }else{
          sqlite3VdbeAddOp(v, OP_Ne, 0x200, addrGroupByChange);
        }
        sqlite3VdbeChangeP3(v, -1, (void*)pKeyInfo->aColl[j], P3_COLLSEQ);
      }

      








      sqlite3VdbeResolveLabel(v, addrGroupByChange);
      for(j=0; j<pGroupBy->nExpr; j++){
        sqlite3VdbeAddOp(v, OP_MemMove, iAMem+j, iBMem+j);
      }
      sqlite3VdbeAddOp(v, OP_Gosub, 0, addrOutputRow);
      VdbeComment((v, "# output one row"));
      sqlite3VdbeAddOp(v, OP_IfMemPos, iAbortFlag, addrEnd);
      VdbeComment((v, "# check abort flag"));
      sqlite3VdbeAddOp(v, OP_Gosub, 0, addrReset);
      VdbeComment((v, "# reset accumulator"));

      


      sqlite3VdbeResolveLabel(v, addrProcessRow);
      updateAccumulator(pParse, &sAggInfo);
      sqlite3VdbeAddOp(v, OP_MemInt, 1, iUseFlag);
      VdbeComment((v, "# indicate data in accumulator"));

      

      if( groupBySort ){
        sqlite3VdbeAddOp(v, OP_Next, sAggInfo.sortingIdx, addrTopOfLoop);
      }else{
        sqlite3WhereEnd(pWInfo);
        sqlite3VdbeChangeToNoop(v, addrSortingIdx, 1);
      }

      

      sqlite3VdbeAddOp(v, OP_Gosub, 0, addrOutputRow);
      VdbeComment((v, "# output final row"));
      
    } 
    else {
      



      resetAccumulator(pParse, &sAggInfo);
      pWInfo = sqlite3WhereBegin(pParse, pTabList, pWhere, 0);
      if( pWInfo==0 ) goto select_end;
      updateAccumulator(pParse, &sAggInfo);
      sqlite3WhereEnd(pWInfo);
      finalizeAggFunctions(pParse, &sAggInfo);
      pOrderBy = 0;
      if( pHaving ){
        sqlite3ExprIfFalse(pParse, pHaving, addrEnd, 1);
      }
      selectInnerLoop(pParse, p, p->pEList, 0, 0, 0, -1, 
                      eDest, iParm, addrEnd, addrEnd, aff);
    }
    sqlite3VdbeResolveLabel(v, addrEnd);
    
  } 

  


  if( pOrderBy ){
    generateSortTail(pParse, p, v, pEList->nExpr, eDest, iParm);
  }

#ifndef SQLITE_OMIT_SUBQUERY
  




  if( pParent ){
    assert( pParent->pSrc->nSrc>parentTab );
    assert( pParent->pSrc->a[parentTab].pSelect==p );
    pParent->pSrc->a[parentTab].isPopulated = 1;
  }
#endif

  

  sqlite3VdbeResolveLabel(v, iEnd);

  


  rc = 0;

  


select_end:

  


  if( rc==SQLITE_OK && eDest==SRT_Callback ){
    generateColumnNames(pParse, pTabList, pEList);
  }

  sqliteFree(sAggInfo.aCol);
  sqliteFree(sAggInfo.aFunc);
  return rc;
}
