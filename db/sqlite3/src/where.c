



















#include "sqliteInt.h"




#define BMS  (sizeof(Bitmask)*8)




#define ARRAYSIZE(X)  (sizeof(X)/sizeof(X[0]))




#if defined(SQLITE_TEST) || defined(SQLITE_DEBUG)
int sqlite3_where_trace = 0;
# define TRACE(X)  if(sqlite3_where_trace) sqlite3DebugPrintf X
#else
# define TRACE(X)
#endif



typedef struct WhereClause WhereClause;


































typedef struct WhereTerm WhereTerm;
struct WhereTerm {
  Expr *pExpr;            
  i16 iParent;            
  i16 leftCursor;         
  i16 leftColumn;         
  u16 eOperator;          
  u8 flags;               
  u8 nChild;              
  WhereClause *pWC;       
  Bitmask prereqRight;    
  Bitmask prereqAll;      
};




#define TERM_DYNAMIC    0x01   /* Need to call sqlite3ExprDelete(pExpr) */
#define TERM_VIRTUAL    0x02   /* Added by the optimizer.  Do not code */
#define TERM_CODED      0x04   /* This term is already coded */
#define TERM_COPIED     0x08   /* Has a child */
#define TERM_OR_OK      0x10   /* Used during OR-clause processing */





struct WhereClause {
  Parse *pParse;           
  int nTerm;               
  int nSlot;               
  WhereTerm *a;            
  WhereTerm aStatic[10];   
};



























typedef struct ExprMaskSet ExprMaskSet;
struct ExprMaskSet {
  int n;                        
  int ix[sizeof(Bitmask)*8];    
};







#define WO_IN     1
#define WO_EQ     2
#define WO_LT     (WO_EQ<<(TK_LT-TK_EQ))
#define WO_LE     (WO_EQ<<(TK_LE-TK_EQ))
#define WO_GT     (WO_EQ<<(TK_GT-TK_EQ))
#define WO_GE     (WO_EQ<<(TK_GE-TK_EQ))




#define WHERE_ROWID_EQ       0x0001   /* rowid=EXPR or rowid IN (...) */
#define WHERE_ROWID_RANGE    0x0002   /* rowid<EXPR and/or rowid>EXPR */
#define WHERE_COLUMN_EQ      0x0010   /* x=EXPR or x IN (...) */
#define WHERE_COLUMN_RANGE   0x0020   /* x<EXPR and/or x>EXPR */
#define WHERE_COLUMN_IN      0x0040   /* x IN (...) */
#define WHERE_TOP_LIMIT      0x0100   /* x<EXPR or x<=EXPR constraint */
#define WHERE_BTM_LIMIT      0x0200   /* x>EXPR or x>=EXPR constraint */
#define WHERE_IDX_ONLY       0x0800   /* Use index only - omit table */
#define WHERE_ORDERBY        0x1000   /* Output will appear in correct order */
#define WHERE_REVERSE        0x2000   /* Scan in reverse order */
#define WHERE_UNIQUE         0x4000   /* Selects no more than one row */




static void whereClauseInit(WhereClause *pWC, Parse *pParse){
  pWC->pParse = pParse;
  pWC->nTerm = 0;
  pWC->nSlot = ARRAYSIZE(pWC->aStatic);
  pWC->a = pWC->aStatic;
}





static void whereClauseClear(WhereClause *pWC){
  int i;
  WhereTerm *a;
  for(i=pWC->nTerm-1, a=pWC->a; i>=0; i--, a++){
    if( a->flags & TERM_DYNAMIC ){
      sqlite3ExprDelete(a->pExpr);
    }
  }
  if( pWC->a!=pWC->aStatic ){
    sqliteFree(pWC->a);
  }
}










static int whereClauseInsert(WhereClause *pWC, Expr *p, int flags){
  WhereTerm *pTerm;
  int idx;
  if( pWC->nTerm>=pWC->nSlot ){
    WhereTerm *pOld = pWC->a;
    pWC->a = sqliteMalloc( sizeof(pWC->a[0])*pWC->nSlot*2 );
    if( pWC->a==0 ) return 0;
    memcpy(pWC->a, pOld, sizeof(pWC->a[0])*pWC->nTerm);
    if( pOld!=pWC->aStatic ){
      sqliteFree(pOld);
    }
    pWC->nSlot *= 2;
  }
  pTerm = &pWC->a[idx = pWC->nTerm];
  pWC->nTerm++;
  pTerm->pExpr = p;
  pTerm->flags = flags;
  pTerm->pWC = pWC;
  pTerm->iParent = -1;
  return idx;
}


















static void whereSplit(WhereClause *pWC, Expr *pExpr, int op){
  if( pExpr==0 ) return;
  if( pExpr->op!=op ){
    whereClauseInsert(pWC, pExpr, 0);
  }else{
    whereSplit(pWC, pExpr->pLeft, op);
    whereSplit(pWC, pExpr->pRight, op);
  }
}




#define initMaskSet(P)  memset(P, 0, sizeof(*P))





static Bitmask getMask(ExprMaskSet *pMaskSet, int iCursor){
  int i;
  for(i=0; i<pMaskSet->n; i++){
    if( pMaskSet->ix[i]==iCursor ){
      return ((Bitmask)1)<<i;
    }
  }
  return 0;
}









static void createMask(ExprMaskSet *pMaskSet, int iCursor){
  assert( pMaskSet->n < ARRAYSIZE(pMaskSet->ix) );
  pMaskSet->ix[pMaskSet->n++] = iCursor;
}















static Bitmask exprListTableUsage(ExprMaskSet*, ExprList*);
static Bitmask exprSelectTableUsage(ExprMaskSet*, Select*);
static Bitmask exprTableUsage(ExprMaskSet *pMaskSet, Expr *p){
  Bitmask mask = 0;
  if( p==0 ) return 0;
  if( p->op==TK_COLUMN ){
    mask = getMask(pMaskSet, p->iTable);
    return mask;
  }
  mask = exprTableUsage(pMaskSet, p->pRight);
  mask |= exprTableUsage(pMaskSet, p->pLeft);
  mask |= exprListTableUsage(pMaskSet, p->pList);
  mask |= exprSelectTableUsage(pMaskSet, p->pSelect);
  return mask;
}
static Bitmask exprListTableUsage(ExprMaskSet *pMaskSet, ExprList *pList){
  int i;
  Bitmask mask = 0;
  if( pList ){
    for(i=0; i<pList->nExpr; i++){
      mask |= exprTableUsage(pMaskSet, pList->a[i].pExpr);
    }
  }
  return mask;
}
static Bitmask exprSelectTableUsage(ExprMaskSet *pMaskSet, Select *pS){
  Bitmask mask;
  if( pS==0 ){
    mask = 0;
  }else{
    mask = exprListTableUsage(pMaskSet, pS->pEList);
    mask |= exprListTableUsage(pMaskSet, pS->pGroupBy);
    mask |= exprListTableUsage(pMaskSet, pS->pOrderBy);
    mask |= exprTableUsage(pMaskSet, pS->pWhere);
    mask |= exprTableUsage(pMaskSet, pS->pHaving);
  }
  return mask;
}






static int allowedOp(int op){
  assert( TK_GT>TK_EQ && TK_GT<TK_GE );
  assert( TK_LT>TK_EQ && TK_LT<TK_GE );
  assert( TK_LE>TK_EQ && TK_LE<TK_GE );
  assert( TK_GE==TK_EQ+4 );
  return op==TK_IN || (op>=TK_EQ && op<=TK_GE);
}




#define SWAP(TYPE,A,B) {TYPE t=A; A=B; B=t;}





static void exprCommute(Expr *pExpr){
  assert( allowedOp(pExpr->op) && pExpr->op!=TK_IN );
  SWAP(CollSeq*,pExpr->pRight->pColl,pExpr->pLeft->pColl);
  SWAP(Expr*,pExpr->pRight,pExpr->pLeft);
  if( pExpr->op>=TK_GT ){
    assert( TK_LT==TK_GT+2 );
    assert( TK_GE==TK_LE+2 );
    assert( TK_GT>TK_EQ );
    assert( TK_GT<TK_LE );
    assert( pExpr->op>=TK_GT && pExpr->op<=TK_GE );
    pExpr->op = ((pExpr->op-TK_GT)^2)+TK_GT;
  }
}




static int operatorMask(int op){
  int c;
  assert( allowedOp(op) );
  if( op==TK_IN ){
    c = WO_IN;
  }else{
    c = WO_EQ<<(op-TK_EQ);
  }
  assert( op!=TK_IN || c==WO_IN );
  assert( op!=TK_EQ || c==WO_EQ );
  assert( op!=TK_LT || c==WO_LT );
  assert( op!=TK_LE || c==WO_LE );
  assert( op!=TK_GT || c==WO_GT );
  assert( op!=TK_GE || c==WO_GE );
  return c;
}







static WhereTerm *findTerm(
  WhereClause *pWC,     
  int iCur,             
  int iColumn,          
  Bitmask notReady,     
  u16 op,               
  Index *pIdx           
){
  WhereTerm *pTerm;
  int k;
  for(pTerm=pWC->a, k=pWC->nTerm; k; k--, pTerm++){
    if( pTerm->leftCursor==iCur
       && (pTerm->prereqRight & notReady)==0
       && pTerm->leftColumn==iColumn
       && (pTerm->eOperator & op)!=0
    ){
      if( iCur>=0 && pIdx ){
        Expr *pX = pTerm->pExpr;
        CollSeq *pColl;
        char idxaff;
        int j;
        Parse *pParse = pWC->pParse;

        idxaff = pIdx->pTable->aCol[iColumn].affinity;
        if( !sqlite3IndexAffinityOk(pX, idxaff) ) continue;
        pColl = sqlite3ExprCollSeq(pParse, pX->pLeft);
        if( !pColl ){
          if( pX->pRight ){
            pColl = sqlite3ExprCollSeq(pParse, pX->pRight);
          }
          if( !pColl ){
            pColl = pParse->db->pDfltColl;
          }
        }
        for(j=0; j<pIdx->nColumn && pIdx->aiColumn[j]!=iColumn; j++){}
        assert( j<pIdx->nColumn );
        if( sqlite3StrICmp(pColl->zName, pIdx->azColl[j]) ) continue;
      }
      return pTerm;
    }
  }
  return 0;
}


static void exprAnalyze(SrcList*, ExprMaskSet*, WhereClause*, int);






static void exprAnalyzeAll(
  SrcList *pTabList,       
  ExprMaskSet *pMaskSet,   
  WhereClause *pWC         
){
  int i;
  for(i=pWC->nTerm-1; i>=0; i--){
    exprAnalyze(pTabList, pMaskSet, pWC, i);
  }
}

#ifndef SQLITE_OMIT_LIKE_OPTIMIZATION








static int isLikeOrGlob(
  sqlite3 *db,      
  Expr *pExpr,      
  int *pnPattern,   
  int *pisComplete  
){
  const char *z;
  Expr *pRight, *pLeft;
  ExprList *pList;
  int c, cnt;
  int noCase;
  char wc[3];
  CollSeq *pColl;

  if( !sqlite3IsLikeFunction(db, pExpr, &noCase, wc) ){
    return 0;
  }
  pList = pExpr->pList;
  pRight = pList->a[0].pExpr;
  if( pRight->op!=TK_STRING ){
    return 0;
  }
  pLeft = pList->a[1].pExpr;
  if( pLeft->op!=TK_COLUMN ){
    return 0;
  }
  pColl = pLeft->pColl;
  if( pColl==0 ){
    pColl = db->pDfltColl;
  }
  if( (pColl->type!=SQLITE_COLL_BINARY || noCase) &&
      (pColl->type!=SQLITE_COLL_NOCASE || !noCase) ){
    return 0;
  }
  sqlite3DequoteExpr(pRight);
  z = (char *)pRight->token.z;
  for(cnt=0; (c=z[cnt])!=0 && c!=wc[0] && c!=wc[1] && c!=wc[2]; cnt++){}
  if( cnt==0 || 255==(u8)z[cnt] ){
    return 0;
  }
  *pisComplete = z[cnt]==wc[0] && z[cnt+1]==0;
  *pnPattern = cnt;
  return 1;
}
#endif 





static void transferJoinMarkings(Expr *pDerived, Expr *pBase){
  pDerived->flags |= pBase->flags & EP_FromJoin;
  pDerived->iRightJoinTable = pBase->iRightJoinTable;
}














static void exprAnalyze(
  SrcList *pSrc,            
  ExprMaskSet *pMaskSet,    
  WhereClause *pWC,         
  int idxTerm               
){
  WhereTerm *pTerm = &pWC->a[idxTerm];
  Expr *pExpr = pTerm->pExpr;
  Bitmask prereqLeft;
  Bitmask prereqAll;
  int nPattern;
  int isComplete;

  if( sqlite3MallocFailed() ) return;
  prereqLeft = exprTableUsage(pMaskSet, pExpr->pLeft);
  if( pExpr->op==TK_IN ){
    assert( pExpr->pRight==0 );
    pTerm->prereqRight = exprListTableUsage(pMaskSet, pExpr->pList)
                          | exprSelectTableUsage(pMaskSet, pExpr->pSelect);
  }else{
    pTerm->prereqRight = exprTableUsage(pMaskSet, pExpr->pRight);
  }
  prereqAll = exprTableUsage(pMaskSet, pExpr);
  if( ExprHasProperty(pExpr, EP_FromJoin) ){
    prereqAll |= getMask(pMaskSet, pExpr->iRightJoinTable);
  }
  pTerm->prereqAll = prereqAll;
  pTerm->leftCursor = -1;
  pTerm->iParent = -1;
  pTerm->eOperator = 0;
  if( allowedOp(pExpr->op) && (pTerm->prereqRight & prereqLeft)==0 ){
    Expr *pLeft = pExpr->pLeft;
    Expr *pRight = pExpr->pRight;
    if( pLeft->op==TK_COLUMN ){
      pTerm->leftCursor = pLeft->iTable;
      pTerm->leftColumn = pLeft->iColumn;
      pTerm->eOperator = operatorMask(pExpr->op);
    }
    if( pRight && pRight->op==TK_COLUMN ){
      WhereTerm *pNew;
      Expr *pDup;
      if( pTerm->leftCursor>=0 ){
        int idxNew;
        pDup = sqlite3ExprDup(pExpr);
        idxNew = whereClauseInsert(pWC, pDup, TERM_VIRTUAL|TERM_DYNAMIC);
        if( idxNew==0 ) return;
        pNew = &pWC->a[idxNew];
        pNew->iParent = idxTerm;
        pTerm = &pWC->a[idxTerm];
        pTerm->nChild = 1;
        pTerm->flags |= TERM_COPIED;
      }else{
        pDup = pExpr;
        pNew = pTerm;
      }
      exprCommute(pDup);
      pLeft = pDup->pLeft;
      pNew->leftCursor = pLeft->iTable;
      pNew->leftColumn = pLeft->iColumn;
      pNew->prereqRight = prereqLeft;
      pNew->prereqAll = prereqAll;
      pNew->eOperator = operatorMask(pDup->op);
    }
  }

#ifndef SQLITE_OMIT_BETWEEN_OPTIMIZATION
  


  else if( pExpr->op==TK_BETWEEN ){
    ExprList *pList = pExpr->pList;
    int i;
    static const u8 ops[] = {TK_GE, TK_LE};
    assert( pList!=0 );
    assert( pList->nExpr==2 );
    for(i=0; i<2; i++){
      Expr *pNewExpr;
      int idxNew;
      pNewExpr = sqlite3Expr(ops[i], sqlite3ExprDup(pExpr->pLeft),
                             sqlite3ExprDup(pList->a[i].pExpr), 0);
      idxNew = whereClauseInsert(pWC, pNewExpr, TERM_VIRTUAL|TERM_DYNAMIC);
      exprAnalyze(pSrc, pMaskSet, pWC, idxNew);
      pTerm = &pWC->a[idxTerm];
      pWC->a[idxNew].iParent = idxTerm;
    }
    pTerm->nChild = 2;
  }
#endif 

#if !defined(SQLITE_OMIT_OR_OPTIMIZATION) && !defined(SQLITE_OMIT_SUBQUERY)
  











  else if( pExpr->op==TK_OR ){
    int ok;
    int i, j;
    int iColumn, iCursor;
    WhereClause sOr;
    WhereTerm *pOrTerm;

    assert( (pTerm->flags & TERM_DYNAMIC)==0 );
    whereClauseInit(&sOr, pWC->pParse);
    whereSplit(&sOr, pExpr, TK_OR);
    exprAnalyzeAll(pSrc, pMaskSet, &sOr);
    assert( sOr.nTerm>0 );
    j = 0;
    do{
      iColumn = sOr.a[j].leftColumn;
      iCursor = sOr.a[j].leftCursor;
      ok = iCursor>=0;
      for(i=sOr.nTerm-1, pOrTerm=sOr.a; i>=0 && ok; i--, pOrTerm++){
        if( pOrTerm->eOperator!=WO_EQ ){
          goto or_not_possible;
        }
        if( pOrTerm->leftCursor==iCursor && pOrTerm->leftColumn==iColumn ){
          pOrTerm->flags |= TERM_OR_OK;
        }else if( (pOrTerm->flags & TERM_COPIED)!=0 ||
                    ((pOrTerm->flags & TERM_VIRTUAL)!=0 &&
                     (sOr.a[pOrTerm->iParent].flags & TERM_OR_OK)!=0) ){
          pOrTerm->flags &= ~TERM_OR_OK;
        }else{
          ok = 0;
        }
      }
    }while( !ok && (sOr.a[j++].flags & TERM_COPIED)!=0 && j<sOr.nTerm );
    if( ok ){
      ExprList *pList = 0;
      Expr *pNew, *pDup;
      for(i=sOr.nTerm-1, pOrTerm=sOr.a; i>=0 && ok; i--, pOrTerm++){
        if( (pOrTerm->flags & TERM_OR_OK)==0 ) continue;
        pDup = sqlite3ExprDup(pOrTerm->pExpr->pRight);
        pList = sqlite3ExprListAppend(pList, pDup, 0);
      }
      pDup = sqlite3Expr(TK_COLUMN, 0, 0, 0);
      if( pDup ){
        pDup->iTable = iCursor;
        pDup->iColumn = iColumn;
      }
      pNew = sqlite3Expr(TK_IN, pDup, 0, 0);
      if( pNew ){
        int idxNew;
        transferJoinMarkings(pNew, pExpr);
        pNew->pList = pList;
        idxNew = whereClauseInsert(pWC, pNew, TERM_VIRTUAL|TERM_DYNAMIC);
        exprAnalyze(pSrc, pMaskSet, pWC, idxNew);
        pTerm = &pWC->a[idxTerm];
        pWC->a[idxNew].iParent = idxTerm;
        pTerm->nChild = 1;
      }else{
        sqlite3ExprListDelete(pList);
      }
    }
or_not_possible:
    whereClauseClear(&sOr);
  }
#endif 

#ifndef SQLITE_OMIT_LIKE_OPTIMIZATION
  


  if( isLikeOrGlob(pWC->pParse->db, pExpr, &nPattern, &isComplete) ){
    Expr *pLeft, *pRight;
    Expr *pStr1, *pStr2;
    Expr *pNewExpr1, *pNewExpr2;
    int idxNew1, idxNew2;

    pLeft = pExpr->pList->a[1].pExpr;
    pRight = pExpr->pList->a[0].pExpr;
    pStr1 = sqlite3Expr(TK_STRING, 0, 0, 0);
    if( pStr1 ){
      sqlite3TokenCopy(&pStr1->token, &pRight->token);
      pStr1->token.n = nPattern;
    }
    pStr2 = sqlite3ExprDup(pStr1);
    if( pStr2 ){
      assert( pStr2->token.dyn );
      ++*(u8*)&pStr2->token.z[nPattern-1];
    }
    pNewExpr1 = sqlite3Expr(TK_GE, sqlite3ExprDup(pLeft), pStr1, 0);
    idxNew1 = whereClauseInsert(pWC, pNewExpr1, TERM_VIRTUAL|TERM_DYNAMIC);
    exprAnalyze(pSrc, pMaskSet, pWC, idxNew1);
    pNewExpr2 = sqlite3Expr(TK_LT, sqlite3ExprDup(pLeft), pStr2, 0);
    idxNew2 = whereClauseInsert(pWC, pNewExpr2, TERM_VIRTUAL|TERM_DYNAMIC);
    exprAnalyze(pSrc, pMaskSet, pWC, idxNew2);
    pTerm = &pWC->a[idxTerm];
    if( isComplete ){
      pWC->a[idxNew1].iParent = idxTerm;
      pWC->a[idxNew2].iParent = idxTerm;
      pTerm->nChild = 2;
    }
  }
#endif 
}





















static int isSortingIndex(
  Parse *pParse,          
  Index *pIdx,            
  int base,               
  ExprList *pOrderBy,     
  int nEqCol,             
  int *pbRev              
){
  int i, j;                       
  int sortOrder = 0;              
  int nTerm;                      
  struct ExprList_item *pTerm;    
  sqlite3 *db = pParse->db;

  assert( pOrderBy!=0 );
  nTerm = pOrderBy->nExpr;
  assert( nTerm>0 );

  


  for(i=j=0, pTerm=pOrderBy->a; j<nTerm && i<pIdx->nColumn; i++){
    Expr *pExpr;       
    CollSeq *pColl;    
    int termSortOrder; 

    pExpr = pTerm->pExpr;
    if( pExpr->op!=TK_COLUMN || pExpr->iTable!=base ){
      

      return 0;
    }
    pColl = sqlite3ExprCollSeq(pParse, pExpr);
    if( !pColl ) pColl = db->pDfltColl;
    if( pExpr->iColumn!=pIdx->aiColumn[i] || 
        sqlite3StrICmp(pColl->zName, pIdx->azColl[i]) ){
      
      if( i<nEqCol ){
        


        continue;
      }else{
        


        return 0;
      }
    }
    assert( pIdx->aSortOrder!=0 );
    assert( pTerm->sortOrder==0 || pTerm->sortOrder==1 );
    assert( pIdx->aSortOrder[i]==0 || pIdx->aSortOrder[i]==1 );
    termSortOrder = pIdx->aSortOrder[i] ^ pTerm->sortOrder;
    if( i>nEqCol ){
      if( termSortOrder!=sortOrder ){
        

        return 0;
      }
    }else{
      sortOrder = termSortOrder;
    }
    j++;
    pTerm++;
  }

  


  if( j>=nTerm ){
    *pbRev = sortOrder!=0;
    return 1;
  }
  return 0;
}






static int sortableByRowid(
  int base,               
  ExprList *pOrderBy,     
  int *pbRev              
){
  Expr *p;

  assert( pOrderBy!=0 );
  assert( pOrderBy->nExpr>0 );
  p = pOrderBy->a[0].pExpr;
  if( pOrderBy->nExpr==1 && p->op==TK_COLUMN && p->iTable==base
          && p->iColumn==-1 ){
    *pbRev = pOrderBy->a[0].sortOrder;
    return 1;
  }
  return 0;
}








static double estLog(double N){
  double logN = 1;
  double x = 10;
  while( N>x ){
    logN += 1;
    x *= 10;
  }
  return logN;
}



















static double bestIndex(
  Parse *pParse,              
  WhereClause *pWC,           
  struct SrcList_item *pSrc,  
  Bitmask notReady,           
  ExprList *pOrderBy,         
  Index **ppIndex,            
  int *pFlags,                
  int *pnEq                   
){
  WhereTerm *pTerm;
  Index *bestIdx = 0;         
  double lowestCost;          
  int bestFlags = 0;          
  int bestNEq = 0;            
  int iCur = pSrc->iCursor;   
  Index *pProbe;              
  int rev;                    
  int flags;                  
  int nEq;                    
  double cost;                

  TRACE(("bestIndex: tbl=%s notReady=%x\n", pSrc->pTab->zName, notReady));
  lowestCost = SQLITE_BIG_DBL;
  pProbe = pSrc->pTab->pIndex;

  





  if( pProbe==0 &&
     findTerm(pWC, iCur, -1, 0, WO_EQ|WO_IN|WO_LT|WO_LE|WO_GT|WO_GE,0)==0 &&
     (pOrderBy==0 || !sortableByRowid(iCur, pOrderBy, &rev)) ){
    *pFlags = 0;
    *ppIndex = 0;
    *pnEq = 0;
    return 0.0;
  }

  

  pTerm = findTerm(pWC, iCur, -1, notReady, WO_EQ|WO_IN, 0);
  if( pTerm ){
    Expr *pExpr;
    *ppIndex = 0;
    bestFlags = WHERE_ROWID_EQ;
    if( pTerm->eOperator & WO_EQ ){
      

      *pFlags = WHERE_ROWID_EQ | WHERE_UNIQUE;
      *pnEq = 1;
      TRACE(("... best is rowid\n"));
      return 0.0;
    }else if( (pExpr = pTerm->pExpr)->pList!=0 ){
      

      lowestCost = pExpr->pList->nExpr;
      lowestCost *= estLog(lowestCost);
    }else{
      


      lowestCost = 200;
    }
    TRACE(("... rowid IN cost: %.9g\n", lowestCost));
  }

  


  cost = pProbe ? pProbe->aiRowEst[0] : 1000000;
  TRACE(("... table scan base cost: %.9g\n", cost));
  flags = WHERE_ROWID_RANGE;

  

  pTerm = findTerm(pWC, iCur, -1, notReady, WO_LT|WO_LE|WO_GT|WO_GE, 0);
  if( pTerm ){
    if( findTerm(pWC, iCur, -1, notReady, WO_LT|WO_LE, 0) ){
      flags |= WHERE_TOP_LIMIT;
      cost /= 3;  
    }
    if( findTerm(pWC, iCur, -1, notReady, WO_GT|WO_GE, 0) ){
      flags |= WHERE_BTM_LIMIT;
      cost /= 3;  
    }
    TRACE(("... rowid range reduces cost to %.9g\n", cost));
  }else{
    flags = 0;
  }

  

  if( pOrderBy ){
    if( sortableByRowid(iCur, pOrderBy, &rev) ){
      flags |= WHERE_ORDERBY|WHERE_ROWID_RANGE;
      if( rev ){
        flags |= WHERE_REVERSE;
      }
    }else{
      cost += cost*estLog(cost);
      TRACE(("... sorting increases cost to %.9g\n", cost));
    }
  }
  if( cost<lowestCost ){
    lowestCost = cost;
    bestFlags = flags;
  }

  

  for(; pProbe; pProbe=pProbe->pNext){
    int i;                       
    double inMultiplier = 1;

    TRACE(("... index %s:\n", pProbe->zName));

    


    flags = 0;
    for(i=0; i<pProbe->nColumn; i++){
      int j = pProbe->aiColumn[i];
      pTerm = findTerm(pWC, iCur, j, notReady, WO_EQ|WO_IN, pProbe);
      if( pTerm==0 ) break;
      flags |= WHERE_COLUMN_EQ;
      if( pTerm->eOperator & WO_IN ){
        Expr *pExpr = pTerm->pExpr;
        flags |= WHERE_COLUMN_IN;
        if( pExpr->pSelect!=0 ){
          inMultiplier *= 25;
        }else if( pExpr->pList!=0 ){
          inMultiplier *= pExpr->pList->nExpr + 1;
        }
      }
    }
    cost = pProbe->aiRowEst[i] * inMultiplier * estLog(inMultiplier);
    nEq = i;
    if( pProbe->onError!=OE_None && (flags & WHERE_COLUMN_IN)==0
         && nEq==pProbe->nColumn ){
      flags |= WHERE_UNIQUE;
    }
    TRACE(("...... nEq=%d inMult=%.9g cost=%.9g\n", nEq, inMultiplier, cost));

    

    if( nEq<pProbe->nColumn ){
      int j = pProbe->aiColumn[nEq];
      pTerm = findTerm(pWC, iCur, j, notReady, WO_LT|WO_LE|WO_GT|WO_GE, pProbe);
      if( pTerm ){
        flags |= WHERE_COLUMN_RANGE;
        if( findTerm(pWC, iCur, j, notReady, WO_LT|WO_LE, pProbe) ){
          flags |= WHERE_TOP_LIMIT;
          cost /= 3;
        }
        if( findTerm(pWC, iCur, j, notReady, WO_GT|WO_GE, pProbe) ){
          flags |= WHERE_BTM_LIMIT;
          cost /= 3;
        }
        TRACE(("...... range reduces cost to %.9g\n", cost));
      }
    }

    

    if( pOrderBy ){
      if( (flags & WHERE_COLUMN_IN)==0 &&
           isSortingIndex(pParse,pProbe,iCur,pOrderBy,nEq,&rev) ){
        if( flags==0 ){
          flags = WHERE_COLUMN_RANGE;
        }
        flags |= WHERE_ORDERBY;
        if( rev ){
          flags |= WHERE_REVERSE;
        }
      }else{
        cost += cost*estLog(cost);
        TRACE(("...... orderby increases cost to %.9g\n", cost));
      }
    }

    



    if( flags && pSrc->colUsed < (((Bitmask)1)<<(BMS-1)) ){
      Bitmask m = pSrc->colUsed;
      int j;
      for(j=0; j<pProbe->nColumn; j++){
        int x = pProbe->aiColumn[j];
        if( x<BMS-1 ){
          m &= ~(((Bitmask)1)<<x);
        }
      }
      if( m==0 ){
        flags |= WHERE_IDX_ONLY;
        cost /= 2;
        TRACE(("...... idx-only reduces cost to %.9g\n", cost));
      }
    }

    

    if( cost < lowestCost ){
      bestIdx = pProbe;
      lowestCost = cost;
      assert( flags!=0 );
      bestFlags = flags;
      bestNEq = nEq;
    }
  }

  

  *ppIndex = bestIdx;
  TRACE(("best index is %s, cost=%.9g, flags=%x, nEq=%d\n",
        bestIdx ? bestIdx->zName : "(none)", lowestCost, bestFlags, bestNEq));
  *pFlags = bestFlags;
  *pnEq = bestNEq;
  return lowestCost;
}

























static void disableTerm(WhereLevel *pLevel, WhereTerm *pTerm){
  if( pTerm
      && (pTerm->flags & TERM_CODED)==0
      && (pLevel->iLeftJoin==0 || ExprHasProperty(pTerm->pExpr, EP_FromJoin))
  ){
    pTerm->flags |= TERM_CODED;
    if( pTerm->iParent>=0 ){
      WhereTerm *pOther = &pTerm->pWC->a[pTerm->iParent];
      if( (--pOther->nChild)==0 ){
        disableTerm(pLevel, pOther);
      }
    }
  }
}
















static void buildIndexProbe(
  Vdbe *v, 
  int nColumn, 
  int nExtra, 
  int brk, 
  Index *pIdx
){
  sqlite3VdbeAddOp(v, OP_NotNull, -nColumn, sqlite3VdbeCurrentAddr(v)+3);
  sqlite3VdbeAddOp(v, OP_Pop, nColumn+nExtra, 0);
  sqlite3VdbeAddOp(v, OP_Goto, 0, brk);
  sqlite3VdbeAddOp(v, OP_MakeRecord, nColumn, 0);
  sqlite3IndexAffinityStr(v, pIdx);
}













static void codeEqualityTerm(
  Parse *pParse,      
  WhereTerm *pTerm,   
  int brk,            
  WhereLevel *pLevel  
){
  Expr *pX = pTerm->pExpr;
  if( pX->op!=TK_IN ){
    assert( pX->op==TK_EQ );
    sqlite3ExprCode(pParse, pX->pRight);
#ifndef SQLITE_OMIT_SUBQUERY
  }else{
    int iTab;
    int *aIn;
    Vdbe *v = pParse->pVdbe;

    sqlite3CodeSubselect(pParse, pX);
    iTab = pX->iTable;
    sqlite3VdbeAddOp(v, OP_Rewind, iTab, 0);
    VdbeComment((v, "# %.*s", pX->span.n, pX->span.z));
    pLevel->nIn++;
    sqliteReallocOrFree((void**)&pLevel->aInLoop,
                                 sizeof(pLevel->aInLoop[0])*2*pLevel->nIn);
    aIn = pLevel->aInLoop;
    if( aIn ){
      aIn += pLevel->nIn*2 - 2;
      aIn[0] = iTab;
      aIn[1] = sqlite3VdbeAddOp(v, OP_Column, iTab, 0);
    }else{
      pLevel->nIn = 0;
    }
#endif
  }
  disableTerm(pLevel, pTerm);
}
























static void codeAllEqualityTerms(
  Parse *pParse,        
  WhereLevel *pLevel,   
  WhereClause *pWC,     
  Bitmask notReady,     
  int brk               
){
  int nEq = pLevel->nEq;        
  int termsInMem = 0;           
  Vdbe *v = pParse->pVdbe;      
  Index *pIdx = pLevel->pIdx;   
  int iCur = pLevel->iTabCur;   
  WhereTerm *pTerm;             
  int j;                        

  




  pLevel->iMem = pParse->nMem++;
  if( pLevel->flags & WHERE_COLUMN_IN ){
    pParse->nMem += pLevel->nEq;
    termsInMem = 1;
  }

  

  for(j=0; j<pIdx->nColumn; j++){
    int k = pIdx->aiColumn[j];
    pTerm = findTerm(pWC, iCur, k, notReady, WO_EQ|WO_IN, pIdx);
    if( pTerm==0 ) break;
    assert( (pTerm->flags & TERM_CODED)==0 );
    codeEqualityTerm(pParse, pTerm, brk, pLevel);
    if( termsInMem ){
      sqlite3VdbeAddOp(v, OP_MemStore, pLevel->iMem+j+1, 1);
    }
  }
  assert( j==nEq );

  

  if( termsInMem ){
    for(j=0; j<nEq; j++){
      sqlite3VdbeAddOp(v, OP_MemLoad, pLevel->iMem+j+1, 0);
    }
  }
}

#if defined(SQLITE_TEST)






char sqlite3_query_plan[BMS*2*40];  
static int nQPlan = 0;              

#endif 



























































































WhereInfo *sqlite3WhereBegin(
  Parse *pParse,        
  SrcList *pTabList,    
  Expr *pWhere,         
  ExprList **ppOrderBy  
){
  int i;                     
  WhereInfo *pWInfo;         
  Vdbe *v = pParse->pVdbe;   
  int brk, cont = 0;         
  Bitmask notReady;          
  WhereTerm *pTerm;          
  ExprMaskSet maskSet;       
  WhereClause wc;            
  struct SrcList_item *pTabItem;  
  WhereLevel *pLevel;             
  int iFrom;                      
  int andFlags;              

  


  if( pTabList->nSrc>BMS ){
    sqlite3ErrorMsg(pParse, "at most %d tables in a join", BMS);
    return 0;
  }

  


  initMaskSet(&maskSet);
  whereClauseInit(&wc, pParse);
  whereSplit(&wc, pWhere, TK_AND);
    
  


  pWInfo = sqliteMalloc( sizeof(WhereInfo) + pTabList->nSrc*sizeof(WhereLevel));
  if( sqlite3MallocFailed() ){
    goto whereBeginNoMem;
  }
  pWInfo->pParse = pParse;
  pWInfo->pTabList = pTabList;
  pWInfo->iBreak = sqlite3VdbeMakeLabel(v);

  


  if( pWhere && (pTabList->nSrc==0 || sqlite3ExprIsConstant(pWhere)) ){
    sqlite3ExprIfFalse(pParse, pWhere, pWInfo->iBreak, 1);
    pWhere = 0;
  }

  




  for(i=0; i<pTabList->nSrc; i++){
    createMask(&maskSet, pTabList->a[i].iCursor);
  }
  exprAnalyzeAll(pTabList, &maskSet, &wc);
  if( sqlite3MallocFailed() ){
    goto whereBeginNoMem;
  }

  













  notReady = ~(Bitmask)0;
  pTabItem = pTabList->a;
  pLevel = pWInfo->a;
  andFlags = ~0;
  TRACE(("*** Optimizer Start ***\n"));
  for(i=iFrom=0, pLevel=pWInfo->a; i<pTabList->nSrc; i++, pLevel++){
    Index *pIdx;                
    int flags;                  
    int nEq;                    
    double cost;                
    int j;                      
    Index *pBest = 0;           
    int bestFlags = 0;          
    int bestNEq = 0;            
    double lowestCost;          
    int bestJ = 0;              
    Bitmask m;                  
    int once = 0;               

    lowestCost = SQLITE_BIG_DBL;
    for(j=iFrom, pTabItem=&pTabList->a[j]; j<pTabList->nSrc; j++, pTabItem++){
      if( once && 
          ((pTabItem->jointype & (JT_LEFT|JT_CROSS))!=0
           || (j>0 && (pTabItem[-1].jointype & (JT_LEFT|JT_CROSS))!=0))
      ){
        break;
      }
      m = getMask(&maskSet, pTabItem->iCursor);
      if( (m & notReady)==0 ){
        if( j==iFrom ) iFrom++;
        continue;
      }
      cost = bestIndex(pParse, &wc, pTabItem, notReady,
                       (i==0 && ppOrderBy) ? *ppOrderBy : 0,
                       &pIdx, &flags, &nEq);
      if( cost<lowestCost ){
        once = 1;
        lowestCost = cost;
        pBest = pIdx;
        bestFlags = flags;
        bestNEq = nEq;
        bestJ = j;
      }
    }
    TRACE(("*** Optimizer choose table %d for loop %d\n", bestJ,
           pLevel-pWInfo->a));
    if( (bestFlags & WHERE_ORDERBY)!=0 ){
      *ppOrderBy = 0;
    }
    andFlags &= bestFlags;
    pLevel->flags = bestFlags;
    pLevel->pIdx = pBest;
    pLevel->nEq = bestNEq;
    pLevel->aInLoop = 0;
    pLevel->nIn = 0;
    if( pBest ){
      pLevel->iIdxCur = pParse->nTab++;
    }else{
      pLevel->iIdxCur = -1;
    }
    notReady &= ~getMask(&maskSet, pTabList->a[bestJ].iCursor);
    pLevel->iFrom = bestJ;
  }
  TRACE(("*** Optimizer Finished ***\n"));

  


  if( (andFlags & WHERE_UNIQUE)!=0 && ppOrderBy ){
    *ppOrderBy = 0;
  }

  


  sqlite3CodeVerifySchema(pParse, -1); 
  pLevel = pWInfo->a;
  for(i=0, pLevel=pWInfo->a; i<pTabList->nSrc; i++, pLevel++){
    Table *pTab;     
    Index *pIx;      
    int iDb;         
    int iIdxCur = pLevel->iIdxCur;

#ifndef SQLITE_OMIT_EXPLAIN
    if( pParse->explain==2 ){
      char *zMsg;
      struct SrcList_item *pItem = &pTabList->a[pLevel->iFrom];
      zMsg = sqlite3MPrintf("TABLE %s", pItem->zName);
      if( pItem->zAlias ){
        zMsg = sqlite3MPrintf("%z AS %s", zMsg, pItem->zAlias);
      }
      if( (pIx = pLevel->pIdx)!=0 ){
        zMsg = sqlite3MPrintf("%z WITH INDEX %s", zMsg, pIx->zName);
      }else if( pLevel->flags & (WHERE_ROWID_EQ|WHERE_ROWID_RANGE) ){
        zMsg = sqlite3MPrintf("%z USING PRIMARY KEY", zMsg);
      }
      if( pLevel->flags & WHERE_ORDERBY ){
        zMsg = sqlite3MPrintf("%z ORDER BY", zMsg);
      }
      sqlite3VdbeOp3(v, OP_Explain, i, pLevel->iFrom, zMsg, P3_DYNAMIC);
    }
#endif 
    pTabItem = &pTabList->a[pLevel->iFrom];
    pTab = pTabItem->pTab;
    iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
    if( pTab->isTransient || pTab->pSelect ) continue;
    if( (pLevel->flags & WHERE_IDX_ONLY)==0 ){
      sqlite3OpenTable(pParse, pTabItem->iCursor, iDb, pTab, OP_OpenRead);
      if( pTab->nCol<(sizeof(Bitmask)*8) ){
        Bitmask b = pTabItem->colUsed;
        int n = 0;
        for(; b; b=b>>1, n++){}
        sqlite3VdbeChangeP2(v, sqlite3VdbeCurrentAddr(v)-1, n);
        assert( n<=pTab->nCol );
      }
    }else{
      sqlite3TableLock(pParse, iDb, pTab->tnum, 0, pTab->zName);
    }
    pLevel->iTabCur = pTabItem->iCursor;
    if( (pIx = pLevel->pIdx)!=0 ){
      KeyInfo *pKey = sqlite3IndexKeyinfo(pParse, pIx);
      assert( pIx->pSchema==pTab->pSchema );
      sqlite3VdbeAddOp(v, OP_Integer, iDb, 0);
      VdbeComment((v, "# %s", pIx->zName));
      sqlite3VdbeOp3(v, OP_OpenRead, iIdxCur, pIx->tnum,
                     (char*)pKey, P3_KEYINFO_HANDOFF);
    }
    if( (pLevel->flags & WHERE_IDX_ONLY)!=0 ){
      sqlite3VdbeAddOp(v, OP_SetNumColumns, iIdxCur, pIx->nColumn+1);
    }
    sqlite3CodeVerifySchema(pParse, iDb);
  }
  pWInfo->iTop = sqlite3VdbeCurrentAddr(v);

  



  notReady = ~(Bitmask)0;
  for(i=0, pLevel=pWInfo->a; i<pTabList->nSrc; i++, pLevel++){
    int j;
    int iCur = pTabItem->iCursor;  
    Index *pIdx;       
    int iIdxCur;       
    int omitTable;     
    int bRev;          

    pTabItem = &pTabList->a[pLevel->iFrom];
    iCur = pTabItem->iCursor;
    pIdx = pLevel->pIdx;
    iIdxCur = pLevel->iIdxCur;
    bRev = (pLevel->flags & WHERE_REVERSE)!=0;
    omitTable = (pLevel->flags & WHERE_IDX_ONLY)!=0;

    




    brk = pLevel->brk = sqlite3VdbeMakeLabel(v);
    cont = pLevel->cont = sqlite3VdbeMakeLabel(v);

    



    if( pLevel->iFrom>0 && (pTabItem[-1].jointype & JT_LEFT)!=0 ){
      if( !pParse->nMem ) pParse->nMem++;
      pLevel->iLeftJoin = pParse->nMem++;
      sqlite3VdbeAddOp(v, OP_MemInt, 0, pLevel->iLeftJoin);
      VdbeComment((v, "# init LEFT JOIN no-match flag"));
    }

    if( pLevel->flags & WHERE_ROWID_EQ ){
      




      pTerm = findTerm(&wc, iCur, -1, notReady, WO_EQ|WO_IN, 0);
      assert( pTerm!=0 );
      assert( pTerm->pExpr!=0 );
      assert( pTerm->leftCursor==iCur );
      assert( omitTable==0 );
      codeEqualityTerm(pParse, pTerm, brk, pLevel);
      sqlite3VdbeAddOp(v, OP_MustBeInt, 1, brk);
      sqlite3VdbeAddOp(v, OP_NotExists, iCur, brk);
      VdbeComment((v, "pk"));
      pLevel->op = OP_Noop;
    }else if( pLevel->flags & WHERE_ROWID_RANGE ){
      

      int testOp = OP_Noop;
      int start;
      WhereTerm *pStart, *pEnd;

      assert( omitTable==0 );
      pStart = findTerm(&wc, iCur, -1, notReady, WO_GT|WO_GE, 0);
      pEnd = findTerm(&wc, iCur, -1, notReady, WO_LT|WO_LE, 0);
      if( bRev ){
        pTerm = pStart;
        pStart = pEnd;
        pEnd = pTerm;
      }
      if( pStart ){
        Expr *pX;
        pX = pStart->pExpr;
        assert( pX!=0 );
        assert( pStart->leftCursor==iCur );
        sqlite3ExprCode(pParse, pX->pRight);
        sqlite3VdbeAddOp(v, OP_ForceInt, pX->op==TK_LE || pX->op==TK_GT, brk);
        sqlite3VdbeAddOp(v, bRev ? OP_MoveLt : OP_MoveGe, iCur, brk);
        VdbeComment((v, "pk"));
        disableTerm(pLevel, pStart);
      }else{
        sqlite3VdbeAddOp(v, bRev ? OP_Last : OP_Rewind, iCur, brk);
      }
      if( pEnd ){
        Expr *pX;
        pX = pEnd->pExpr;
        assert( pX!=0 );
        assert( pEnd->leftCursor==iCur );
        sqlite3ExprCode(pParse, pX->pRight);
        pLevel->iMem = pParse->nMem++;
        sqlite3VdbeAddOp(v, OP_MemStore, pLevel->iMem, 1);
        if( pX->op==TK_LT || pX->op==TK_GT ){
          testOp = bRev ? OP_Le : OP_Ge;
        }else{
          testOp = bRev ? OP_Lt : OP_Gt;
        }
        disableTerm(pLevel, pEnd);
      }
      start = sqlite3VdbeCurrentAddr(v);
      pLevel->op = bRev ? OP_Prev : OP_Next;
      pLevel->p1 = iCur;
      pLevel->p2 = start;
      if( testOp!=OP_Noop ){
        sqlite3VdbeAddOp(v, OP_Rowid, iCur, 0);
        sqlite3VdbeAddOp(v, OP_MemLoad, pLevel->iMem, 0);
        sqlite3VdbeAddOp(v, testOp, SQLITE_AFF_NUMERIC, brk);
      }
    }else if( pLevel->flags & WHERE_COLUMN_RANGE ){
      










      int start;
      int nEq = pLevel->nEq;
      int topEq=0;        
      int btmEq=0;        
      int topOp, btmOp;   
      int testOp;
      int nNotNull;       
      int topLimit = (pLevel->flags & WHERE_TOP_LIMIT)!=0;
      int btmLimit = (pLevel->flags & WHERE_BTM_LIMIT)!=0;

      


      codeAllEqualityTerms(pParse, pLevel, &wc, notReady, brk);

      



      for(j=0; j<nEq; j++){
        sqlite3VdbeAddOp(v, OP_Dup, nEq-1, 0);
      }

      




      nNotNull = nEq + topLimit;
      if( pIdx->aSortOrder[nEq]==SQLITE_SO_ASC ){
        topOp = WO_LT|WO_LE;
        btmOp = WO_GT|WO_GE;
      }else{
        topOp = WO_GT|WO_GE;
        btmOp = WO_LT|WO_LE;
        SWAP(int, topLimit, btmLimit);
      }

      






      if( topLimit ){
        Expr *pX;
        int k = pIdx->aiColumn[j];
        pTerm = findTerm(&wc, iCur, k, notReady, topOp, pIdx);
        assert( pTerm!=0 );
        pX = pTerm->pExpr;
        assert( (pTerm->flags & TERM_CODED)==0 );
        sqlite3ExprCode(pParse, pX->pRight);
        topEq = pTerm->eOperator & (WO_LE|WO_GE);
        disableTerm(pLevel, pTerm);
        testOp = OP_IdxGE;
      }else{
        testOp = nEq>0 ? OP_IdxGE : OP_Noop;
        topEq = 1;
      }
      if( testOp!=OP_Noop ){
        int nCol = nEq + topLimit;
        pLevel->iMem = pParse->nMem++;
        buildIndexProbe(v, nCol, nEq, brk, pIdx);
        if( bRev ){
          int op = topEq ? OP_MoveLe : OP_MoveLt;
          sqlite3VdbeAddOp(v, op, iIdxCur, brk);
        }else{
          sqlite3VdbeAddOp(v, OP_MemStore, pLevel->iMem, 1);
        }
      }else if( bRev ){
        sqlite3VdbeAddOp(v, OP_Last, iIdxCur, brk);
      }

      








      if( btmLimit ){
        Expr *pX;
        int k = pIdx->aiColumn[j];
        pTerm = findTerm(&wc, iCur, k, notReady, btmOp, pIdx);
        assert( pTerm!=0 );
        pX = pTerm->pExpr;
        assert( (pTerm->flags & TERM_CODED)==0 );
        sqlite3ExprCode(pParse, pX->pRight);
        btmEq = pTerm->eOperator & (WO_LE|WO_GE);
        disableTerm(pLevel, pTerm);
      }else{
        btmEq = 1;
      }
      if( nEq>0 || btmLimit ){
        int nCol = nEq + btmLimit;
        buildIndexProbe(v, nCol, 0, brk, pIdx);
        if( bRev ){
          pLevel->iMem = pParse->nMem++;
          sqlite3VdbeAddOp(v, OP_MemStore, pLevel->iMem, 1);
          testOp = OP_IdxLT;
        }else{
          int op = btmEq ? OP_MoveGe : OP_MoveGt;
          sqlite3VdbeAddOp(v, op, iIdxCur, brk);
        }
      }else if( bRev ){
        testOp = OP_Noop;
      }else{
        sqlite3VdbeAddOp(v, OP_Rewind, iIdxCur, brk);
      }

      



      start = sqlite3VdbeCurrentAddr(v);
      if( testOp!=OP_Noop ){
        sqlite3VdbeAddOp(v, OP_MemLoad, pLevel->iMem, 0);
        sqlite3VdbeAddOp(v, testOp, iIdxCur, brk);
        if( (topEq && !bRev) || (!btmEq && bRev) ){
          sqlite3VdbeChangeP3(v, -1, "+", P3_STATIC);
        }
      }
      sqlite3VdbeAddOp(v, OP_RowKey, iIdxCur, 0);
      sqlite3VdbeAddOp(v, OP_IdxIsNull, nNotNull, cont);
      if( !omitTable ){
        sqlite3VdbeAddOp(v, OP_IdxRowid, iIdxCur, 0);
        sqlite3VdbeAddOp(v, OP_MoveGe, iCur, 0);
      }

      

      pLevel->op = bRev ? OP_Prev : OP_Next;
      pLevel->p1 = iIdxCur;
      pLevel->p2 = start;
    }else if( pLevel->flags & WHERE_COLUMN_EQ ){
      


      int start;
      int nEq = pLevel->nEq;

      


      codeAllEqualityTerms(pParse, pLevel, &wc, notReady, brk);

      


      buildIndexProbe(v, nEq, 0, brk, pIdx);
      sqlite3VdbeAddOp(v, OP_MemStore, pLevel->iMem, 0);

      




      if( bRev ){
        
        sqlite3VdbeAddOp(v, OP_MoveLe, iIdxCur, brk);
        start = sqlite3VdbeAddOp(v, OP_MemLoad, pLevel->iMem, 0);
        sqlite3VdbeAddOp(v, OP_IdxLT, iIdxCur, brk);
        pLevel->op = OP_Prev;
      }else{
        
        sqlite3VdbeAddOp(v, OP_MoveGe, iIdxCur, brk);
        start = sqlite3VdbeAddOp(v, OP_MemLoad, pLevel->iMem, 0);
        sqlite3VdbeOp3(v, OP_IdxGE, iIdxCur, brk, "+", P3_STATIC);
        pLevel->op = OP_Next;
      }
      sqlite3VdbeAddOp(v, OP_RowKey, iIdxCur, 0);
      sqlite3VdbeAddOp(v, OP_IdxIsNull, nEq, cont);
      if( !omitTable ){
        sqlite3VdbeAddOp(v, OP_IdxRowid, iIdxCur, 0);
        sqlite3VdbeAddOp(v, OP_MoveGe, iCur, 0);
      }
      pLevel->p1 = iIdxCur;
      pLevel->p2 = start;
    }else{
      


      assert( omitTable==0 );
      assert( bRev==0 );
      pLevel->op = OP_Next;
      pLevel->p1 = iCur;
      pLevel->p2 = 1 + sqlite3VdbeAddOp(v, OP_Rewind, iCur, brk);
    }
    notReady &= ~getMask(&maskSet, iCur);

    


    for(pTerm=wc.a, j=wc.nTerm; j>0; j--, pTerm++){
      Expr *pE;
      if( pTerm->flags & (TERM_VIRTUAL|TERM_CODED) ) continue;
      if( (pTerm->prereqAll & notReady)!=0 ) continue;
      pE = pTerm->pExpr;
      assert( pE!=0 );
      if( pLevel->iLeftJoin && !ExprHasProperty(pE, EP_FromJoin) ){
        continue;
      }
      sqlite3ExprIfFalse(pParse, pE, cont, 1);
      pTerm->flags |= TERM_CODED;
    }

    


    if( pLevel->iLeftJoin ){
      pLevel->top = sqlite3VdbeCurrentAddr(v);
      sqlite3VdbeAddOp(v, OP_MemInt, 1, pLevel->iLeftJoin);
      VdbeComment((v, "# record LEFT JOIN hit"));
      for(pTerm=wc.a, j=0; j<wc.nTerm; j++, pTerm++){
        if( pTerm->flags & (TERM_VIRTUAL|TERM_CODED) ) continue;
        if( (pTerm->prereqAll & notReady)!=0 ) continue;
        assert( pTerm->pExpr );
        sqlite3ExprIfFalse(pParse, pTerm->pExpr, cont, 1);
        pTerm->flags |= TERM_CODED;
      }
    }
  }

#ifdef SQLITE_TEST  
  





  for(i=0; i<pTabList->nSrc; i++){
    char *z;
    int n;
    pLevel = &pWInfo->a[i];
    pTabItem = &pTabList->a[pLevel->iFrom];
    z = pTabItem->zAlias;
    if( z==0 ) z = pTabItem->pTab->zName;
    n = strlen(z);
    if( n+nQPlan < sizeof(sqlite3_query_plan)-10 ){
      if( pLevel->flags & WHERE_IDX_ONLY ){
        strcpy(&sqlite3_query_plan[nQPlan], "{}");
        nQPlan += 2;
      }else{
        strcpy(&sqlite3_query_plan[nQPlan], z);
        nQPlan += n;
      }
      sqlite3_query_plan[nQPlan++] = ' ';
    }
    if( pLevel->flags & (WHERE_ROWID_EQ|WHERE_ROWID_RANGE) ){
      strcpy(&sqlite3_query_plan[nQPlan], "* ");
      nQPlan += 2;
    }else if( pLevel->pIdx==0 ){
      strcpy(&sqlite3_query_plan[nQPlan], "{} ");
      nQPlan += 3;
    }else{
      n = strlen(pLevel->pIdx->zName);
      if( n+nQPlan < sizeof(sqlite3_query_plan)-2 ){
        strcpy(&sqlite3_query_plan[nQPlan], pLevel->pIdx->zName);
        nQPlan += n;
        sqlite3_query_plan[nQPlan++] = ' ';
      }
    }
  }
  while( nQPlan>0 && sqlite3_query_plan[nQPlan-1]==' ' ){
    sqlite3_query_plan[--nQPlan] = 0;
  }
  sqlite3_query_plan[nQPlan] = 0;
  nQPlan = 0;
#endif 

  


  pWInfo->iContinue = cont;
  whereClauseClear(&wc);
  return pWInfo;

  
whereBeginNoMem:
  whereClauseClear(&wc);
  sqliteFree(pWInfo);
  return 0;
}





void sqlite3WhereEnd(WhereInfo *pWInfo){
  Vdbe *v = pWInfo->pParse->pVdbe;
  int i;
  WhereLevel *pLevel;
  SrcList *pTabList = pWInfo->pTabList;

  

  for(i=pTabList->nSrc-1; i>=0; i--){
    pLevel = &pWInfo->a[i];
    sqlite3VdbeResolveLabel(v, pLevel->cont);
    if( pLevel->op!=OP_Noop ){
      sqlite3VdbeAddOp(v, pLevel->op, pLevel->p1, pLevel->p2);
    }
    sqlite3VdbeResolveLabel(v, pLevel->brk);
    if( pLevel->nIn ){
      int *a;
      int j;
      for(j=pLevel->nIn, a=&pLevel->aInLoop[j*2-2]; j>0; j--, a-=2){
        sqlite3VdbeAddOp(v, OP_Next, a[0], a[1]);
        sqlite3VdbeJumpHere(v, a[1]-1);
      }
      sqliteFree(pLevel->aInLoop);
    }
    if( pLevel->iLeftJoin ){
      int addr;
      addr = sqlite3VdbeAddOp(v, OP_IfMemPos, pLevel->iLeftJoin, 0);
      sqlite3VdbeAddOp(v, OP_NullRow, pTabList->a[i].iCursor, 0);
      if( pLevel->iIdxCur>=0 ){
        sqlite3VdbeAddOp(v, OP_NullRow, pLevel->iIdxCur, 0);
      }
      sqlite3VdbeAddOp(v, OP_Goto, 0, pLevel->top);
      sqlite3VdbeJumpHere(v, addr);
    }
  }

  


  sqlite3VdbeResolveLabel(v, pWInfo->iBreak);

  

  for(i=0, pLevel=pWInfo->a; i<pTabList->nSrc; i++, pLevel++){
    struct SrcList_item *pTabItem = &pTabList->a[pLevel->iFrom];
    Table *pTab = pTabItem->pTab;
    assert( pTab!=0 );
    if( pTab->isTransient || pTab->pSelect ) continue;
    if( (pLevel->flags & WHERE_IDX_ONLY)==0 ){
      sqlite3VdbeAddOp(v, OP_Close, pTabItem->iCursor, 0);
    }
    if( pLevel->pIdx!=0 ){
      sqlite3VdbeAddOp(v, OP_Close, pLevel->iIdxCur, 0);
    }

    








    if( pLevel->flags & WHERE_IDX_ONLY ){
      int k, j, last;
      VdbeOp *pOp;
      Index *pIdx = pLevel->pIdx;

      assert( pIdx!=0 );
      pOp = sqlite3VdbeGetOp(v, pWInfo->iTop);
      last = sqlite3VdbeCurrentAddr(v);
      for(k=pWInfo->iTop; k<last; k++, pOp++){
        if( pOp->p1!=pLevel->iTabCur ) continue;
        if( pOp->opcode==OP_Column ){
          pOp->p1 = pLevel->iIdxCur;
          for(j=0; j<pIdx->nColumn; j++){
            if( pOp->p2==pIdx->aiColumn[j] ){
              pOp->p2 = j;
              break;
            }
          }
        }else if( pOp->opcode==OP_Rowid ){
          pOp->p1 = pLevel->iIdxCur;
          pOp->opcode = OP_IdxRowid;
        }else if( pOp->opcode==OP_NullRow ){
          pOp->opcode = OP_Noop;
        }
      }
    }
  }

  

  sqliteFree(pWInfo);
  return;
}
