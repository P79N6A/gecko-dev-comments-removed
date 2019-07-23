















#include "sqliteInt.h"
#include <ctype.h>

















char sqlite3ExprAffinity(Expr *pExpr){
  int op = pExpr->op;
  if( op==TK_AS ){
    return sqlite3ExprAffinity(pExpr->pLeft);
  }
  if( op==TK_SELECT ){
    return sqlite3ExprAffinity(pExpr->pSelect->pEList->a[0].pExpr);
  }
#ifndef SQLITE_OMIT_CAST
  if( op==TK_CAST ){
    return sqlite3AffinityType(&pExpr->token);
  }
#endif
  return pExpr->affinity;
}





CollSeq *sqlite3ExprCollSeq(Parse *pParse, Expr *pExpr){
  CollSeq *pColl = 0;
  if( pExpr ){
    pColl = pExpr->pColl;
    if( (pExpr->op==TK_AS || pExpr->op==TK_CAST) && !pColl ){
      return sqlite3ExprCollSeq(pParse, pExpr->pLeft);
    }
  }
  if( sqlite3CheckCollSeq(pParse, pColl) ){ 
    pColl = 0;
  }
  return pColl;
}






char sqlite3CompareAffinity(Expr *pExpr, char aff2){
  char aff1 = sqlite3ExprAffinity(pExpr);
  if( aff1 && aff2 ){
    


    if( sqlite3IsNumericAffinity(aff1) || sqlite3IsNumericAffinity(aff2) ){
      return SQLITE_AFF_NUMERIC;
    }else{
      return SQLITE_AFF_NONE;
    }
  }else if( !aff1 && !aff2 ){
    


    return SQLITE_AFF_NONE;
  }else{
    
    assert( aff1==0 || aff2==0 );
    return (aff1 + aff2);
  }
}





static char comparisonAffinity(Expr *pExpr){
  char aff;
  assert( pExpr->op==TK_EQ || pExpr->op==TK_IN || pExpr->op==TK_LT ||
          pExpr->op==TK_GT || pExpr->op==TK_GE || pExpr->op==TK_LE ||
          pExpr->op==TK_NE );
  assert( pExpr->pLeft );
  aff = sqlite3ExprAffinity(pExpr->pLeft);
  if( pExpr->pRight ){
    aff = sqlite3CompareAffinity(pExpr->pRight, aff);
  }
  else if( pExpr->pSelect ){
    aff = sqlite3CompareAffinity(pExpr->pSelect->pEList->a[0].pExpr, aff);
  }
  else if( !aff ){
    aff = SQLITE_AFF_NUMERIC;
  }
  return aff;
}







int sqlite3IndexAffinityOk(Expr *pExpr, char idx_affinity){
  char aff = comparisonAffinity(pExpr);
  switch( aff ){
    case SQLITE_AFF_NONE:
      return 1;
    case SQLITE_AFF_TEXT:
      return idx_affinity==SQLITE_AFF_TEXT;
    default:
      return sqlite3IsNumericAffinity(idx_affinity);
  }
}








static int binaryCompareP1(Expr *pExpr1, Expr *pExpr2, int jumpIfNull){
  char aff = sqlite3ExprAffinity(pExpr2);
  return ((int)sqlite3CompareAffinity(pExpr1, aff))+(jumpIfNull?0x100:0);
}










static CollSeq* binaryCompareCollSeq(Parse *pParse, Expr *pLeft, Expr *pRight){
  CollSeq *pColl = sqlite3ExprCollSeq(pParse, pLeft);
  if( !pColl ){
    pColl = sqlite3ExprCollSeq(pParse, pRight);
  }
  return pColl;
}




static int codeCompare(
  Parse *pParse,    
  Expr *pLeft,      
  Expr *pRight,     
  int opcode,       
  int dest,         
  int jumpIfNull    
){
  int p1 = binaryCompareP1(pLeft, pRight, jumpIfNull);
  CollSeq *p3 = binaryCompareCollSeq(pParse, pLeft, pRight);
  return sqlite3VdbeOp3(pParse->pVdbe, opcode, p1, dest, (void*)p3, P3_COLLSEQ);
}






Expr *sqlite3Expr(int op, Expr *pLeft, Expr *pRight, const Token *pToken){
  Expr *pNew;
  pNew = sqliteMalloc( sizeof(Expr) );
  if( pNew==0 ){
    



    sqlite3ExprDelete(pLeft);
    sqlite3ExprDelete(pRight);
    return 0;
  }
  pNew->op = op;
  pNew->pLeft = pLeft;
  pNew->pRight = pRight;
  pNew->iAgg = -1;
  if( pToken ){
    assert( pToken->dyn==0 );
    pNew->span = pNew->token = *pToken;
  }else if( pLeft && pRight ){
    sqlite3ExprSpan(pNew, &pLeft->span, &pRight->span);
  }
  return pNew;
}












Expr *sqlite3RegisterExpr(Parse *pParse, Token *pToken){
  Vdbe *v = pParse->pVdbe;
  Expr *p;
  int depth;
  if( pParse->nested==0 ){
    sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", pToken);
    return 0;
  }
  if( v==0 ) return 0;
  p = sqlite3Expr(TK_REGISTER, 0, 0, pToken);
  if( p==0 ){
    return 0;  
  }
  depth = atoi((char*)&pToken->z[1]);
  p->iTable = pParse->nMem++;
  sqlite3VdbeAddOp(v, OP_Dup, depth, 0);
  sqlite3VdbeAddOp(v, OP_MemStore, p->iTable, 1);
  return p;
}





Expr *sqlite3ExprAnd(Expr *pLeft, Expr *pRight){
  if( pLeft==0 ){
    return pRight;
  }else if( pRight==0 ){
    return pLeft;
  }else{
    return sqlite3Expr(TK_AND, pLeft, pRight, 0);
  }
}





void sqlite3ExprSpan(Expr *pExpr, Token *pLeft, Token *pRight){
  assert( pRight!=0 );
  assert( pLeft!=0 );
  if( !sqlite3MallocFailed() && pRight->z && pLeft->z ){
    assert( pLeft->dyn==0 || pLeft->z[pLeft->n]==0 );
    if( pLeft->dyn==0 && pRight->dyn==0 ){
      pExpr->span.z = pLeft->z;
      pExpr->span.n = pRight->n + (pRight->z - pLeft->z);
    }else{
      pExpr->span.z = 0;
    }
  }
}





Expr *sqlite3ExprFunction(ExprList *pList, Token *pToken){
  Expr *pNew;
  assert( pToken );
  pNew = sqliteMalloc( sizeof(Expr) );
  if( pNew==0 ){
    sqlite3ExprListDelete(pList); 
    return 0;
  }
  pNew->op = TK_FUNCTION;
  pNew->pList = pList;
  assert( pToken->dyn==0 );
  pNew->token = *pToken;
  pNew->span = pNew->token;
  return pNew;
}

















void sqlite3ExprAssignVarNumber(Parse *pParse, Expr *pExpr){
  Token *pToken;
  if( pExpr==0 ) return;
  pToken = &pExpr->token;
  assert( pToken->n>=1 );
  assert( pToken->z!=0 );
  assert( pToken->z[0]!=0 );
  if( pToken->n==1 ){
    
    pExpr->iTable = ++pParse->nVar;
  }else if( pToken->z[0]=='?' ){
    

    int i;
    pExpr->iTable = i = atoi((char*)&pToken->z[1]);
    if( i<1 || i>SQLITE_MAX_VARIABLE_NUMBER ){
      sqlite3ErrorMsg(pParse, "variable number must be between ?1 and ?%d",
          SQLITE_MAX_VARIABLE_NUMBER);
    }
    if( i>pParse->nVar ){
      pParse->nVar = i;
    }
  }else{
    



    int i, n;
    n = pToken->n;
    for(i=0; i<pParse->nVarExpr; i++){
      Expr *pE;
      if( (pE = pParse->apVarExpr[i])!=0
          && pE->token.n==n
          && memcmp(pE->token.z, pToken->z, n)==0 ){
        pExpr->iTable = pE->iTable;
        break;
      }
    }
    if( i>=pParse->nVarExpr ){
      pExpr->iTable = ++pParse->nVar;
      if( pParse->nVarExpr>=pParse->nVarExprAlloc-1 ){
        pParse->nVarExprAlloc += pParse->nVarExprAlloc + 10;
        sqliteReallocOrFree((void**)&pParse->apVarExpr,
                       pParse->nVarExprAlloc*sizeof(pParse->apVarExpr[0]) );
      }
      if( !sqlite3MallocFailed() ){
        assert( pParse->apVarExpr!=0 );
        pParse->apVarExpr[pParse->nVarExpr++] = pExpr;
      }
    }
  } 
}




void sqlite3ExprDelete(Expr *p){
  if( p==0 ) return;
  if( p->span.dyn ) sqliteFree((char*)p->span.z);
  if( p->token.dyn ) sqliteFree((char*)p->token.z);
  sqlite3ExprDelete(p->pLeft);
  sqlite3ExprDelete(p->pRight);
  sqlite3ExprListDelete(p->pList);
  sqlite3SelectDelete(p->pSelect);
  sqliteFree(p);
}





void sqlite3DequoteExpr(Expr *p){
  if( ExprHasAnyProperty(p, EP_Dequoted) ){
    return;
  }
  ExprSetProperty(p, EP_Dequoted);
  if( p->token.dyn==0 ){
    sqlite3TokenCopy(&p->token, &p->token);
  }
  sqlite3Dequote((char*)p->token.z);
}














Expr *sqlite3ExprDup(Expr *p){
  Expr *pNew;
  if( p==0 ) return 0;
  pNew = sqliteMallocRaw( sizeof(*p) );
  if( pNew==0 ) return 0;
  memcpy(pNew, p, sizeof(*pNew));
  if( p->token.z!=0 ){
    pNew->token.z = (u8*)sqliteStrNDup((char*)p->token.z, p->token.n);
    pNew->token.dyn = 1;
  }else{
    assert( pNew->token.z==0 );
  }
  pNew->span.z = 0;
  pNew->pLeft = sqlite3ExprDup(p->pLeft);
  pNew->pRight = sqlite3ExprDup(p->pRight);
  pNew->pList = sqlite3ExprListDup(p->pList);
  pNew->pSelect = sqlite3SelectDup(p->pSelect);
  pNew->pTab = p->pTab;
  return pNew;
}
void sqlite3TokenCopy(Token *pTo, Token *pFrom){
  if( pTo->dyn ) sqliteFree((char*)pTo->z);
  if( pFrom->z ){
    pTo->n = pFrom->n;
    pTo->z = (u8*)sqliteStrNDup((char*)pFrom->z, pFrom->n);
    pTo->dyn = 1;
  }else{
    pTo->z = 0;
  }
}
ExprList *sqlite3ExprListDup(ExprList *p){
  ExprList *pNew;
  struct ExprList_item *pItem, *pOldItem;
  int i;
  if( p==0 ) return 0;
  pNew = sqliteMalloc( sizeof(*pNew) );
  if( pNew==0 ) return 0;
  pNew->nExpr = pNew->nAlloc = p->nExpr;
  pNew->a = pItem = sqliteMalloc( p->nExpr*sizeof(p->a[0]) );
  if( pItem==0 ){
    sqliteFree(pNew);
    return 0;
  } 
  pOldItem = p->a;
  for(i=0; i<p->nExpr; i++, pItem++, pOldItem++){
    Expr *pNewExpr, *pOldExpr;
    pItem->pExpr = pNewExpr = sqlite3ExprDup(pOldExpr = pOldItem->pExpr);
    if( pOldExpr->span.z!=0 && pNewExpr ){
      


      sqlite3TokenCopy(&pNewExpr->span, &pOldExpr->span);
    }
    assert( pNewExpr==0 || pNewExpr->span.z!=0 
            || pOldExpr->span.z==0
            || sqlite3MallocFailed() );
    pItem->zName = sqliteStrDup(pOldItem->zName);
    pItem->sortOrder = pOldItem->sortOrder;
    pItem->isAgg = pOldItem->isAgg;
    pItem->done = 0;
  }
  return pNew;
}







#if !defined(SQLITE_OMIT_VIEW) || !defined(SQLITE_OMIT_TRIGGER) \
 || !defined(SQLITE_OMIT_SUBQUERY)
SrcList *sqlite3SrcListDup(SrcList *p){
  SrcList *pNew;
  int i;
  int nByte;
  if( p==0 ) return 0;
  nByte = sizeof(*p) + (p->nSrc>0 ? sizeof(p->a[0]) * (p->nSrc-1) : 0);
  pNew = sqliteMallocRaw( nByte );
  if( pNew==0 ) return 0;
  pNew->nSrc = pNew->nAlloc = p->nSrc;
  for(i=0; i<p->nSrc; i++){
    struct SrcList_item *pNewItem = &pNew->a[i];
    struct SrcList_item *pOldItem = &p->a[i];
    Table *pTab;
    pNewItem->zDatabase = sqliteStrDup(pOldItem->zDatabase);
    pNewItem->zName = sqliteStrDup(pOldItem->zName);
    pNewItem->zAlias = sqliteStrDup(pOldItem->zAlias);
    pNewItem->jointype = pOldItem->jointype;
    pNewItem->iCursor = pOldItem->iCursor;
    pNewItem->isPopulated = pOldItem->isPopulated;
    pTab = pNewItem->pTab = pOldItem->pTab;
    if( pTab ){
      pTab->nRef++;
    }
    pNewItem->pSelect = sqlite3SelectDup(pOldItem->pSelect);
    pNewItem->pOn = sqlite3ExprDup(pOldItem->pOn);
    pNewItem->pUsing = sqlite3IdListDup(pOldItem->pUsing);
    pNewItem->colUsed = pOldItem->colUsed;
  }
  return pNew;
}
IdList *sqlite3IdListDup(IdList *p){
  IdList *pNew;
  int i;
  if( p==0 ) return 0;
  pNew = sqliteMallocRaw( sizeof(*pNew) );
  if( pNew==0 ) return 0;
  pNew->nId = pNew->nAlloc = p->nId;
  pNew->a = sqliteMallocRaw( p->nId*sizeof(p->a[0]) );
  if( pNew->a==0 ){
    sqliteFree(pNew);
    return 0;
  }
  for(i=0; i<p->nId; i++){
    struct IdList_item *pNewItem = &pNew->a[i];
    struct IdList_item *pOldItem = &p->a[i];
    pNewItem->zName = sqliteStrDup(pOldItem->zName);
    pNewItem->idx = pOldItem->idx;
  }
  return pNew;
}
Select *sqlite3SelectDup(Select *p){
  Select *pNew;
  if( p==0 ) return 0;
  pNew = sqliteMallocRaw( sizeof(*p) );
  if( pNew==0 ) return 0;
  pNew->isDistinct = p->isDistinct;
  pNew->pEList = sqlite3ExprListDup(p->pEList);
  pNew->pSrc = sqlite3SrcListDup(p->pSrc);
  pNew->pWhere = sqlite3ExprDup(p->pWhere);
  pNew->pGroupBy = sqlite3ExprListDup(p->pGroupBy);
  pNew->pHaving = sqlite3ExprDup(p->pHaving);
  pNew->pOrderBy = sqlite3ExprListDup(p->pOrderBy);
  pNew->op = p->op;
  pNew->pPrior = sqlite3SelectDup(p->pPrior);
  pNew->pLimit = sqlite3ExprDup(p->pLimit);
  pNew->pOffset = sqlite3ExprDup(p->pOffset);
  pNew->iLimit = -1;
  pNew->iOffset = -1;
  pNew->isResolved = p->isResolved;
  pNew->isAgg = p->isAgg;
  pNew->usesVirt = 0;
  pNew->disallowOrderBy = 0;
  pNew->pRightmost = 0;
  pNew->addrOpenVirt[0] = -1;
  pNew->addrOpenVirt[1] = -1;
  pNew->addrOpenVirt[2] = -1;
  return pNew;
}
#else
Select *sqlite3SelectDup(Select *p){
  assert( p==0 );
  return 0;
}
#endif






ExprList *sqlite3ExprListAppend(ExprList *pList, Expr *pExpr, Token *pName){
  if( pList==0 ){
    pList = sqliteMalloc( sizeof(ExprList) );
    if( pList==0 ){
      goto no_mem;
    }
    assert( pList->nAlloc==0 );
  }
  if( pList->nAlloc<=pList->nExpr ){
    struct ExprList_item *a;
    int n = pList->nAlloc*2 + 4;
    a = sqliteRealloc(pList->a, n*sizeof(pList->a[0]));
    if( a==0 ){
      goto no_mem;
    }
    pList->a = a;
    pList->nAlloc = n;
  }
  assert( pList->a!=0 );
  if( pExpr || pName ){
    struct ExprList_item *pItem = &pList->a[pList->nExpr++];
    memset(pItem, 0, sizeof(*pItem));
    pItem->zName = sqlite3NameFromToken(pName);
    pItem->pExpr = pExpr;
  }
  return pList;

no_mem:     
  
  sqlite3ExprDelete(pExpr);
  sqlite3ExprListDelete(pList);
  return 0;
}




void sqlite3ExprListDelete(ExprList *pList){
  int i;
  struct ExprList_item *pItem;
  if( pList==0 ) return;
  assert( pList->a!=0 || (pList->nExpr==0 && pList->nAlloc==0) );
  assert( pList->nExpr<=pList->nAlloc );
  for(pItem=pList->a, i=0; i<pList->nExpr; i++, pItem++){
    sqlite3ExprDelete(pItem->pExpr);
    sqliteFree(pItem->zName);
  }
  sqliteFree(pList->a);
  sqliteFree(pList);
}














static int walkExprList(ExprList *, int (*)(void *, Expr*), void *);
static int walkExprTree(Expr *pExpr, int (*xFunc)(void*,Expr*), void *pArg){
  int rc;
  if( pExpr==0 ) return 0;
  rc = (*xFunc)(pArg, pExpr);
  if( rc==0 ){
    if( walkExprTree(pExpr->pLeft, xFunc, pArg) ) return 1;
    if( walkExprTree(pExpr->pRight, xFunc, pArg) ) return 1;
    if( walkExprList(pExpr->pList, xFunc, pArg) ) return 1;
  }
  return rc>1;
}




static int walkExprList(ExprList *p, int (*xFunc)(void *, Expr*), void *pArg){
  int i;
  struct ExprList_item *pItem;
  if( !p ) return 0;
  for(i=p->nExpr, pItem=p->a; i>0; i--, pItem++){
    if( walkExprTree(pItem->pExpr, xFunc, pArg) ) return 1;
  }
  return 0;
}






static int walkSelectExpr(Select *p, int (*xFunc)(void *, Expr*), void *pArg){
  walkExprList(p->pEList, xFunc, pArg);
  walkExprTree(p->pWhere, xFunc, pArg);
  walkExprList(p->pGroupBy, xFunc, pArg);
  walkExprTree(p->pHaving, xFunc, pArg);
  walkExprList(p->pOrderBy, xFunc, pArg);
  return 0;
}















static int exprNodeIsConstant(void *pArg, Expr *pExpr){
  switch( pExpr->op ){
    

    case TK_FUNCTION:
      if( *((int*)pArg)==2 ) return 0;
      
    case TK_ID:
    case TK_COLUMN:
    case TK_DOT:
    case TK_AGG_FUNCTION:
    case TK_AGG_COLUMN:
#ifndef SQLITE_OMIT_SUBQUERY
    case TK_SELECT:
    case TK_EXISTS:
#endif
      *((int*)pArg) = 0;
      return 2;
    case TK_IN:
      if( pExpr->pSelect ){
        *((int*)pArg) = 0;
        return 2;
      }
    default:
      return 0;
  }
}









int sqlite3ExprIsConstant(Expr *p){
  int isConst = 1;
  walkExprTree(p, exprNodeIsConstant, &isConst);
  return isConst;
}










int sqlite3ExprIsConstantOrFunction(Expr *p){
  int isConst = 2;
  walkExprTree(p, exprNodeIsConstant, &isConst);
  return isConst!=0;
}







int sqlite3ExprIsInteger(Expr *p, int *pValue){
  switch( p->op ){
    case TK_INTEGER: {
      if( sqlite3GetInt32((char*)p->token.z, pValue) ){
        return 1;
      }
      break;
    }
    case TK_UPLUS: {
      return sqlite3ExprIsInteger(p->pLeft, pValue);
    }
    case TK_UMINUS: {
      int v;
      if( sqlite3ExprIsInteger(p->pLeft, &v) ){
        *pValue = -v;
        return 1;
      }
      break;
    }
    default: break;
  }
  return 0;
}




int sqlite3IsRowid(const char *z){
  if( sqlite3StrICmp(z, "_ROWID_")==0 ) return 1;
  if( sqlite3StrICmp(z, "ROWID")==0 ) return 1;
  if( sqlite3StrICmp(z, "OID")==0 ) return 1;
  return 0;
}


























static int lookupName(
  Parse *pParse,       
  Token *pDbToken,     
  Token *pTableToken,  
  Token *pColumnToken, 
  NameContext *pNC,    
  Expr *pExpr          
){
  char *zDb = 0;       
  char *zTab = 0;      
  char *zCol = 0;      
  int i, j;            
  int cnt = 0;         
  int cntTab = 0;      
  sqlite3 *db = pParse->db;  
  struct SrcList_item *pItem;       
  struct SrcList_item *pMatch = 0;  
  NameContext *pTopNC = pNC;        

  assert( pColumnToken && pColumnToken->z ); 
  zDb = sqlite3NameFromToken(pDbToken);
  zTab = sqlite3NameFromToken(pTableToken);
  zCol = sqlite3NameFromToken(pColumnToken);
  if( sqlite3MallocFailed() ){
    goto lookupname_end;
  }

  pExpr->iTable = -1;
  while( pNC && cnt==0 ){
    ExprList *pEList;
    SrcList *pSrcList = pNC->pSrcList;

    if( pSrcList ){
      for(i=0, pItem=pSrcList->a; i<pSrcList->nSrc; i++, pItem++){
        Table *pTab;
        int iDb;
        Column *pCol;
  
        pTab = pItem->pTab;
        assert( pTab!=0 );
        iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
        assert( pTab->nCol>0 );
        if( zTab ){
          if( pItem->zAlias ){
            char *zTabName = pItem->zAlias;
            if( sqlite3StrICmp(zTabName, zTab)!=0 ) continue;
          }else{
            char *zTabName = pTab->zName;
            if( zTabName==0 || sqlite3StrICmp(zTabName, zTab)!=0 ) continue;
            if( zDb!=0 && sqlite3StrICmp(db->aDb[iDb].zName, zDb)!=0 ){
              continue;
            }
          }
        }
        if( 0==(cntTab++) ){
          pExpr->iTable = pItem->iCursor;
          pExpr->pSchema = pTab->pSchema;
          pMatch = pItem;
        }
        for(j=0, pCol=pTab->aCol; j<pTab->nCol; j++, pCol++){
          if( sqlite3StrICmp(pCol->zName, zCol)==0 ){
            const char *zColl = pTab->aCol[j].zColl;
            IdList *pUsing;
            cnt++;
            pExpr->iTable = pItem->iCursor;
            pMatch = pItem;
            pExpr->pSchema = pTab->pSchema;
            
            pExpr->iColumn = j==pTab->iPKey ? -1 : j;
            pExpr->affinity = pTab->aCol[j].affinity;
            pExpr->pColl = sqlite3FindCollSeq(db, ENC(db), zColl,-1, 0);
            if( pItem->jointype & JT_NATURAL ){
              

              pItem++;
              i++;
            }
            if( (pUsing = pItem->pUsing)!=0 ){
              


              int k;
              for(k=0; k<pUsing->nId; k++){
                if( sqlite3StrICmp(pUsing->a[k].zName, zCol)==0 ){
                  pItem++;
                  i++;
                  break;
                }
              }
            }
            break;
          }
        }
      }
    }

#ifndef SQLITE_OMIT_TRIGGER
    


    if( zDb==0 && zTab!=0 && cnt==0 && pParse->trigStack!=0 ){
      TriggerStack *pTriggerStack = pParse->trigStack;
      Table *pTab = 0;
      if( pTriggerStack->newIdx != -1 && sqlite3StrICmp("new", zTab) == 0 ){
        pExpr->iTable = pTriggerStack->newIdx;
        assert( pTriggerStack->pTab );
        pTab = pTriggerStack->pTab;
      }else if( pTriggerStack->oldIdx != -1 && sqlite3StrICmp("old", zTab)==0 ){
        pExpr->iTable = pTriggerStack->oldIdx;
        assert( pTriggerStack->pTab );
        pTab = pTriggerStack->pTab;
      }

      if( pTab ){ 
        int iCol;
        Column *pCol = pTab->aCol;

        pExpr->pSchema = pTab->pSchema;
        cntTab++;
        for(iCol=0; iCol < pTab->nCol; iCol++, pCol++) {
          if( sqlite3StrICmp(pCol->zName, zCol)==0 ){
            const char *zColl = pTab->aCol[iCol].zColl;
            cnt++;
            pExpr->iColumn = iCol==pTab->iPKey ? -1 : iCol;
            pExpr->affinity = pTab->aCol[iCol].affinity;
            pExpr->pColl = sqlite3FindCollSeq(db, ENC(db), zColl,-1, 0);
            pExpr->pTab = pTab;
            break;
          }
        }
      }
    }
#endif 

    


    if( cnt==0 && cntTab==1 && sqlite3IsRowid(zCol) ){
      cnt = 1;
      pExpr->iColumn = -1;
      pExpr->affinity = SQLITE_AFF_INTEGER;
    }

    











    if( cnt==0 && (pEList = pNC->pEList)!=0 && zTab==0 ){
      for(j=0; j<pEList->nExpr; j++){
        char *zAs = pEList->a[j].zName;
        if( zAs!=0 && sqlite3StrICmp(zAs, zCol)==0 ){
          assert( pExpr->pLeft==0 && pExpr->pRight==0 );
          pExpr->op = TK_AS;
          pExpr->iColumn = j;
          pExpr->pLeft = sqlite3ExprDup(pEList->a[j].pExpr);
          cnt = 1;
          assert( zTab==0 && zDb==0 );
          goto lookupname_end_2;
        }
      } 
    }

    


    if( cnt==0 ){
      pNC = pNC->pNext;
    }
  }

  









  if( cnt==0 && zTab==0 && pColumnToken->z[0]=='"' ){
    sqliteFree(zCol);
    return 0;
  }

  



  if( cnt!=1 ){
    char *z = 0;
    char *zErr;
    zErr = cnt==0 ? "no such column: %s" : "ambiguous column name: %s";
    if( zDb ){
      sqlite3SetString(&z, zDb, ".", zTab, ".", zCol, (char*)0);
    }else if( zTab ){
      sqlite3SetString(&z, zTab, ".", zCol, (char*)0);
    }else{
      z = sqliteStrDup(zCol);
    }
    sqlite3ErrorMsg(pParse, zErr, z);
    sqliteFree(z);
    pTopNC->nErr++;
  }

  





  if( pExpr->iColumn>=0 && pMatch!=0 ){
    int n = pExpr->iColumn;
    if( n>=sizeof(Bitmask)*8 ){
      n = sizeof(Bitmask)*8-1;
    }
    assert( pMatch->iCursor==pExpr->iTable );
    pMatch->colUsed |= 1<<n;
  }

lookupname_end:
  

  sqliteFree(zDb);
  sqliteFree(zTab);
  sqlite3ExprDelete(pExpr->pLeft);
  pExpr->pLeft = 0;
  sqlite3ExprDelete(pExpr->pRight);
  pExpr->pRight = 0;
  pExpr->op = TK_COLUMN;
lookupname_end_2:
  sqliteFree(zCol);
  if( cnt==1 ){
    assert( pNC!=0 );
    sqlite3AuthRead(pParse, pExpr, pNC->pSrcList);
    if( pMatch && !pMatch->pSelect ){
      pExpr->pTab = pMatch->pTab;
    }
    

    for(;;){
      assert( pTopNC!=0 );
      pTopNC->nRef++;
      if( pTopNC==pNC ) break;
      pTopNC = pTopNC->pNext;
    }
    return 0;
  } else {
    return 1;
  }
}












static int nameResolverStep(void *pArg, Expr *pExpr){
  NameContext *pNC = (NameContext*)pArg;
  Parse *pParse;

  if( pExpr==0 ) return 1;
  assert( pNC!=0 );
  pParse = pNC->pParse;

  if( ExprHasAnyProperty(pExpr, EP_Resolved) ) return 1;
  ExprSetProperty(pExpr, EP_Resolved);
#ifndef NDEBUG
  if( pNC->pSrcList && pNC->pSrcList->nAlloc>0 ){
    SrcList *pSrcList = pNC->pSrcList;
    int i;
    for(i=0; i<pNC->pSrcList->nSrc; i++){
      assert( pSrcList->a[i].iCursor>=0 && pSrcList->a[i].iCursor<pParse->nTab);
    }
  }
#endif
  switch( pExpr->op ){
    



    case TK_STRING: {
      if( pExpr->token.z[0]=='\'' ) break;
      
    }
    

    case TK_ID: {
      lookupName(pParse, 0, 0, &pExpr->token, pNC, pExpr);
      return 1;
    }
  
    


    case TK_DOT: {
      Token *pColumn;
      Token *pTable;
      Token *pDb;
      Expr *pRight;

      
      pRight = pExpr->pRight;
      if( pRight->op==TK_ID ){
        pDb = 0;
        pTable = &pExpr->pLeft->token;
        pColumn = &pRight->token;
      }else{
        assert( pRight->op==TK_DOT );
        pDb = &pExpr->pLeft->token;
        pTable = &pRight->pLeft->token;
        pColumn = &pRight->pRight->token;
      }
      lookupName(pParse, pDb, pTable, pColumn, pNC, pExpr);
      return 1;
    }

    

    case TK_CONST_FUNC:
    case TK_FUNCTION: {
      ExprList *pList = pExpr->pList;    
      int n = pList ? pList->nExpr : 0;  
      int no_such_func = 0;       
      int wrong_num_args = 0;     
      int is_agg = 0;             
      int i;
      int nId;                    
      const char *zId;            
      FuncDef *pDef;              
      int enc = ENC(pParse->db);  

      zId = (char*)pExpr->token.z;
      nId = pExpr->token.n;
      pDef = sqlite3FindFunction(pParse->db, zId, nId, n, enc, 0);
      if( pDef==0 ){
        pDef = sqlite3FindFunction(pParse->db, zId, nId, -1, enc, 0);
        if( pDef==0 ){
          no_such_func = 1;
        }else{
          wrong_num_args = 1;
        }
      }else{
        is_agg = pDef->xFunc==0;
      }
      if( is_agg && !pNC->allowAgg ){
        sqlite3ErrorMsg(pParse, "misuse of aggregate function %.*s()", nId,zId);
        pNC->nErr++;
        is_agg = 0;
      }else if( no_such_func ){
        sqlite3ErrorMsg(pParse, "no such function: %.*s", nId, zId);
        pNC->nErr++;
      }else if( wrong_num_args ){
        sqlite3ErrorMsg(pParse,"wrong number of arguments to function %.*s()",
             nId, zId);
        pNC->nErr++;
      }
      if( is_agg ){
        pExpr->op = TK_AGG_FUNCTION;
        pNC->hasAgg = 1;
      }
      if( is_agg ) pNC->allowAgg = 0;
      for(i=0; pNC->nErr==0 && i<n; i++){
        walkExprTree(pList->a[i].pExpr, nameResolverStep, pNC);
      }
      if( is_agg ) pNC->allowAgg = 1;
      


      return is_agg;
    }
#ifndef SQLITE_OMIT_SUBQUERY
    case TK_SELECT:
    case TK_EXISTS:
#endif
    case TK_IN: {
      if( pExpr->pSelect ){
        int nRef = pNC->nRef;
#ifndef SQLITE_OMIT_CHECK
        if( pNC->isCheck ){
          sqlite3ErrorMsg(pParse,"subqueries prohibited in CHECK constraints");
        }
#endif
        sqlite3SelectResolve(pParse, pExpr->pSelect, pNC);
        assert( pNC->nRef>=nRef );
        if( nRef!=pNC->nRef ){
          ExprSetProperty(pExpr, EP_VarSelect);
        }
      }
      break;
    }
#ifndef SQLITE_OMIT_CHECK
    case TK_VARIABLE: {
      if( pNC->isCheck ){
        sqlite3ErrorMsg(pParse,"parameters prohibited in CHECK constraints");
      }
      break;
    }
#endif
  }
  return 0;
}






















int sqlite3ExprResolveNames(
  NameContext *pNC,       
  Expr *pExpr             
){
  int savedHasAgg;
  if( pExpr==0 ) return 0;
  savedHasAgg = pNC->hasAgg;
  pNC->hasAgg = 0;
  walkExprTree(pExpr, nameResolverStep, pNC);
  if( pNC->nErr>0 ){
    ExprSetProperty(pExpr, EP_Error);
  }
  if( pNC->hasAgg ){
    ExprSetProperty(pExpr, EP_Agg);
  }else if( savedHasAgg ){
    pNC->hasAgg = 1;
  }
  return ExprHasProperty(pExpr, EP_Error);
}





typedef struct QueryCoder QueryCoder;
struct QueryCoder {
  Parse *pParse;       
  NameContext *pNC;    
};














#ifndef SQLITE_OMIT_SUBQUERY
void sqlite3CodeSubselect(Parse *pParse, Expr *pExpr){
  int testAddr = 0;                       
  Vdbe *v = sqlite3GetVdbe(pParse);
  if( v==0 ) return;

  









  if( !ExprHasAnyProperty(pExpr, EP_VarSelect) && !pParse->trigStack ){
    int mem = pParse->nMem++;
    sqlite3VdbeAddOp(v, OP_MemLoad, mem, 0);
    testAddr = sqlite3VdbeAddOp(v, OP_If, 0, 0);
    assert( testAddr>0 || sqlite3MallocFailed() );
    sqlite3VdbeAddOp(v, OP_MemInt, 1, mem);
  }

  switch( pExpr->op ){
    case TK_IN: {
      char affinity;
      KeyInfo keyInfo;
      int addr;        

      affinity = sqlite3ExprAffinity(pExpr->pLeft);

      












      pExpr->iTable = pParse->nTab++;
      addr = sqlite3VdbeAddOp(v, OP_OpenVirtual, pExpr->iTable, 0);
      memset(&keyInfo, 0, sizeof(keyInfo));
      keyInfo.nField = 1;
      sqlite3VdbeAddOp(v, OP_SetNumColumns, pExpr->iTable, 1);

      if( pExpr->pSelect ){
        




        int iParm = pExpr->iTable +  (((int)affinity)<<16);
        ExprList *pEList;
        assert( (pExpr->iTable&0x0000FFFF)==pExpr->iTable );
        sqlite3Select(pParse, pExpr->pSelect, SRT_Set, iParm, 0, 0, 0, 0);
        pEList = pExpr->pSelect->pEList;
        if( pEList && pEList->nExpr>0 ){ 
          keyInfo.aColl[0] = binaryCompareCollSeq(pParse, pExpr->pLeft,
              pEList->a[0].pExpr);
        }
      }else if( pExpr->pList ){
        






        int i;
        ExprList *pList = pExpr->pList;
        struct ExprList_item *pItem;

        if( !affinity ){
          affinity = SQLITE_AFF_NUMERIC;
        }
        keyInfo.aColl[0] = pExpr->pLeft->pColl;

        
        for(i=pList->nExpr, pItem=pList->a; i>0; i--, pItem++){
          Expr *pE2 = pItem->pExpr;

          




          if( testAddr>0 && !sqlite3ExprIsConstant(pE2) ){
            sqlite3VdbeChangeToNoop(v, testAddr-1, 3);
            testAddr = 0;
          }

          
          sqlite3ExprCode(pParse, pE2);
          sqlite3VdbeOp3(v, OP_MakeRecord, 1, 0, &affinity, 1);
          sqlite3VdbeAddOp(v, OP_IdxInsert, pExpr->iTable, 0);
        }
      }
      sqlite3VdbeChangeP3(v, addr, (void *)&keyInfo, P3_KEYINFO);
      break;
    }

    case TK_EXISTS:
    case TK_SELECT: {
      



      static const Token one = { (u8*)"1", 0, 1 };
      Select *pSel;
      int iMem;
      int sop;

      pExpr->iColumn = iMem = pParse->nMem++;
      pSel = pExpr->pSelect;
      if( pExpr->op==TK_SELECT ){
        sop = SRT_Mem;
        sqlite3VdbeAddOp(v, OP_MemNull, iMem, 0);
        VdbeComment((v, "# Init subquery result"));
      }else{
        sop = SRT_Exists;
        sqlite3VdbeAddOp(v, OP_MemInt, 0, iMem);
        VdbeComment((v, "# Init EXISTS result"));
      }
      sqlite3ExprDelete(pSel->pLimit);
      pSel->pLimit = sqlite3Expr(TK_INTEGER, 0, 0, &one);
      sqlite3Select(pParse, pSel, sop, iMem, 0, 0, 0, 0);
      break;
    }
  }

  if( testAddr ){
    sqlite3VdbeJumpHere(v, testAddr);
  }
  return;
}
#endif 





static void codeInteger(Vdbe *v, const char *z, int n){
  int i;
  if( sqlite3GetInt32(z, &i) ){
    sqlite3VdbeAddOp(v, OP_Integer, i, 0);
  }else if( sqlite3FitsIn64Bits(z) ){
    sqlite3VdbeOp3(v, OP_Int64, 0, 0, z, n);
  }else{
    sqlite3VdbeOp3(v, OP_Real, 0, 0, z, n);
  }
}











void sqlite3ExprCode(Parse *pParse, Expr *pExpr){
  Vdbe *v = pParse->pVdbe;
  int op;
  int stackChng = 1;    

  if( v==0 ) return;
  if( pExpr==0 ){
    sqlite3VdbeAddOp(v, OP_Null, 0, 0);
    return;
  }
  op = pExpr->op;
  switch( op ){
    case TK_AGG_COLUMN: {
      AggInfo *pAggInfo = pExpr->pAggInfo;
      struct AggInfo_col *pCol = &pAggInfo->aCol[pExpr->iAgg];
      if( !pAggInfo->directMode ){
        sqlite3VdbeAddOp(v, OP_MemLoad, pCol->iMem, 0);
        break;
      }else if( pAggInfo->useSortingIdx ){
        sqlite3VdbeAddOp(v, OP_Column, pAggInfo->sortingIdx,
                              pCol->iSorterColumn);
        break;
      }
      
    }
    case TK_COLUMN: {
      if( pExpr->iTable<0 ){
        
        assert( pParse->ckOffset>0 );
        sqlite3VdbeAddOp(v, OP_Dup, pParse->ckOffset-pExpr->iColumn-1, 1);
      }else if( pExpr->iColumn>=0 ){
        Table *pTab = pExpr->pTab;
        int iCol = pExpr->iColumn;
        sqlite3VdbeAddOp(v, OP_Column, pExpr->iTable, iCol);
        sqlite3ColumnDefault(v, pTab, iCol);
#ifndef SQLITE_OMIT_FLOATING_POINT
        if( pTab && pTab->aCol[iCol].affinity==SQLITE_AFF_REAL ){
          sqlite3VdbeAddOp(v, OP_RealAffinity, 0, 0);
        }
#endif
      }else{
        sqlite3VdbeAddOp(v, OP_Rowid, pExpr->iTable, 0);
      }
      break;
    }
    case TK_INTEGER: {
      codeInteger(v, (char*)pExpr->token.z, pExpr->token.n);
      break;
    }
    case TK_FLOAT:
    case TK_STRING: {
      assert( TK_FLOAT==OP_Real );
      assert( TK_STRING==OP_String8 );
      sqlite3DequoteExpr(pExpr);
      sqlite3VdbeOp3(v, op, 0, 0, (char*)pExpr->token.z, pExpr->token.n);
      break;
    }
    case TK_NULL: {
      sqlite3VdbeAddOp(v, OP_Null, 0, 0);
      break;
    }
#ifndef SQLITE_OMIT_BLOB_LITERAL
    case TK_BLOB: {
      int n;
      const char *z;
      assert( TK_BLOB==OP_HexBlob );
      n = pExpr->token.n - 3;
      z = (char*)pExpr->token.z + 2;
      assert( n>=0 );
      if( n==0 ){
        z = "";
      }
      sqlite3VdbeOp3(v, op, 0, 0, z, n);
      break;
    }
#endif
    case TK_VARIABLE: {
      sqlite3VdbeAddOp(v, OP_Variable, pExpr->iTable, 0);
      if( pExpr->token.n>1 ){
        sqlite3VdbeChangeP3(v, -1, (char*)pExpr->token.z, pExpr->token.n);
      }
      break;
    }
    case TK_REGISTER: {
      sqlite3VdbeAddOp(v, OP_MemLoad, pExpr->iTable, 0);
      break;
    }
#ifndef SQLITE_OMIT_CAST
    case TK_CAST: {
      
      int aff, to_op;
      sqlite3ExprCode(pParse, pExpr->pLeft);
      aff = sqlite3AffinityType(&pExpr->token);
      to_op = aff - SQLITE_AFF_TEXT + OP_ToText;
      assert( to_op==OP_ToText    || aff!=SQLITE_AFF_TEXT    );
      assert( to_op==OP_ToBlob    || aff!=SQLITE_AFF_NONE    );
      assert( to_op==OP_ToNumeric || aff!=SQLITE_AFF_NUMERIC );
      assert( to_op==OP_ToInt     || aff!=SQLITE_AFF_INTEGER );
      assert( to_op==OP_ToReal    || aff!=SQLITE_AFF_REAL    );
      sqlite3VdbeAddOp(v, to_op, 0, 0);
      stackChng = 0;
      break;
    }
#endif 
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
    case TK_NE:
    case TK_EQ: {
      assert( TK_LT==OP_Lt );
      assert( TK_LE==OP_Le );
      assert( TK_GT==OP_Gt );
      assert( TK_GE==OP_Ge );
      assert( TK_EQ==OP_Eq );
      assert( TK_NE==OP_Ne );
      sqlite3ExprCode(pParse, pExpr->pLeft);
      sqlite3ExprCode(pParse, pExpr->pRight);
      codeCompare(pParse, pExpr->pLeft, pExpr->pRight, op, 0, 0);
      stackChng = -1;
      break;
    }
    case TK_AND:
    case TK_OR:
    case TK_PLUS:
    case TK_STAR:
    case TK_MINUS:
    case TK_REM:
    case TK_BITAND:
    case TK_BITOR:
    case TK_SLASH:
    case TK_LSHIFT:
    case TK_RSHIFT: 
    case TK_CONCAT: {
      assert( TK_AND==OP_And );
      assert( TK_OR==OP_Or );
      assert( TK_PLUS==OP_Add );
      assert( TK_MINUS==OP_Subtract );
      assert( TK_REM==OP_Remainder );
      assert( TK_BITAND==OP_BitAnd );
      assert( TK_BITOR==OP_BitOr );
      assert( TK_SLASH==OP_Divide );
      assert( TK_LSHIFT==OP_ShiftLeft );
      assert( TK_RSHIFT==OP_ShiftRight );
      assert( TK_CONCAT==OP_Concat );
      sqlite3ExprCode(pParse, pExpr->pLeft);
      sqlite3ExprCode(pParse, pExpr->pRight);
      sqlite3VdbeAddOp(v, op, 0, 0);
      stackChng = -1;
      break;
    }
    case TK_UMINUS: {
      Expr *pLeft = pExpr->pLeft;
      assert( pLeft );
      if( pLeft->op==TK_FLOAT || pLeft->op==TK_INTEGER ){
        Token *p = &pLeft->token;
        char *z = sqlite3MPrintf("-%.*s", p->n, p->z);
        if( pLeft->op==TK_FLOAT ){
          sqlite3VdbeOp3(v, OP_Real, 0, 0, z, p->n+1);
        }else{
          codeInteger(v, z, p->n+1);
        }
        sqliteFree(z);
        break;
      }
      
    }
    case TK_BITNOT:
    case TK_NOT: {
      assert( TK_BITNOT==OP_BitNot );
      assert( TK_NOT==OP_Not );
      sqlite3ExprCode(pParse, pExpr->pLeft);
      sqlite3VdbeAddOp(v, op, 0, 0);
      stackChng = 0;
      break;
    }
    case TK_ISNULL:
    case TK_NOTNULL: {
      int dest;
      assert( TK_ISNULL==OP_IsNull );
      assert( TK_NOTNULL==OP_NotNull );
      sqlite3VdbeAddOp(v, OP_Integer, 1, 0);
      sqlite3ExprCode(pParse, pExpr->pLeft);
      dest = sqlite3VdbeCurrentAddr(v) + 2;
      sqlite3VdbeAddOp(v, op, 1, dest);
      sqlite3VdbeAddOp(v, OP_AddImm, -1, 0);
      stackChng = 0;
      break;
    }
    case TK_AGG_FUNCTION: {
      AggInfo *pInfo = pExpr->pAggInfo;
      if( pInfo==0 ){
        sqlite3ErrorMsg(pParse, "misuse of aggregate: %T",
            &pExpr->span);
      }else{
        sqlite3VdbeAddOp(v, OP_MemLoad, pInfo->aFunc[pExpr->iAgg].iMem, 0);
      }
      break;
    }
    case TK_CONST_FUNC:
    case TK_FUNCTION: {
      ExprList *pList = pExpr->pList;
      int nExpr = pList ? pList->nExpr : 0;
      FuncDef *pDef;
      int nId;
      const char *zId;
      int constMask = 0;
      int i;
      u8 enc = ENC(pParse->db);
      CollSeq *pColl = 0;
      zId = (char*)pExpr->token.z;
      nId = pExpr->token.n;
      pDef = sqlite3FindFunction(pParse->db, zId, nId, nExpr, enc, 0);
      assert( pDef!=0 );
      nExpr = sqlite3ExprCodeExprList(pParse, pList);
      for(i=0; i<nExpr && i<32; i++){
        if( sqlite3ExprIsConstant(pList->a[i].pExpr) ){
          constMask |= (1<<i);
        }
        if( pDef->needCollSeq && !pColl ){
          pColl = sqlite3ExprCollSeq(pParse, pList->a[i].pExpr);
        }
      }
      if( pDef->needCollSeq ){
        if( !pColl ) pColl = pParse->db->pDfltColl; 
        sqlite3VdbeOp3(v, OP_CollSeq, 0, 0, (char *)pColl, P3_COLLSEQ);
      }
      sqlite3VdbeOp3(v, OP_Function, constMask, nExpr, (char*)pDef, P3_FUNCDEF);
      stackChng = 1-nExpr;
      break;
    }
#ifndef SQLITE_OMIT_SUBQUERY
    case TK_EXISTS:
    case TK_SELECT: {
      if( pExpr->iColumn==0 ){
        sqlite3CodeSubselect(pParse, pExpr);
      }
      sqlite3VdbeAddOp(v, OP_MemLoad, pExpr->iColumn, 0);
      VdbeComment((v, "# load subquery result"));
      break;
    }
    case TK_IN: {
      int addr;
      char affinity;
      int ckOffset = pParse->ckOffset;
      sqlite3CodeSubselect(pParse, pExpr);

      



      affinity = comparisonAffinity(pExpr);

      sqlite3VdbeAddOp(v, OP_Integer, 1, 0);
      pParse->ckOffset = ckOffset+1;

      


      sqlite3ExprCode(pParse, pExpr->pLeft);
      addr = sqlite3VdbeCurrentAddr(v);
      sqlite3VdbeAddOp(v, OP_NotNull, -1, addr+4);            
      sqlite3VdbeAddOp(v, OP_Pop, 2, 0);
      sqlite3VdbeAddOp(v, OP_Null, 0, 0);
      sqlite3VdbeAddOp(v, OP_Goto, 0, addr+7);
      sqlite3VdbeOp3(v, OP_MakeRecord, 1, 0, &affinity, 1);   
      sqlite3VdbeAddOp(v, OP_Found, pExpr->iTable, addr+7);
      sqlite3VdbeAddOp(v, OP_AddImm, -1, 0);                  

      break;
    }
#endif
    case TK_BETWEEN: {
      Expr *pLeft = pExpr->pLeft;
      struct ExprList_item *pLItem = pExpr->pList->a;
      Expr *pRight = pLItem->pExpr;
      sqlite3ExprCode(pParse, pLeft);
      sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
      sqlite3ExprCode(pParse, pRight);
      codeCompare(pParse, pLeft, pRight, OP_Ge, 0, 0);
      sqlite3VdbeAddOp(v, OP_Pull, 1, 0);
      pLItem++;
      pRight = pLItem->pExpr;
      sqlite3ExprCode(pParse, pRight);
      codeCompare(pParse, pLeft, pRight, OP_Le, 0, 0);
      sqlite3VdbeAddOp(v, OP_And, 0, 0);
      break;
    }
    case TK_UPLUS:
    case TK_AS: {
      sqlite3ExprCode(pParse, pExpr->pLeft);
      stackChng = 0;
      break;
    }
    case TK_CASE: {
      int expr_end_label;
      int jumpInst;
      int nExpr;
      int i;
      ExprList *pEList;
      struct ExprList_item *aListelem;

      assert(pExpr->pList);
      assert((pExpr->pList->nExpr % 2) == 0);
      assert(pExpr->pList->nExpr > 0);
      pEList = pExpr->pList;
      aListelem = pEList->a;
      nExpr = pEList->nExpr;
      expr_end_label = sqlite3VdbeMakeLabel(v);
      if( pExpr->pLeft ){
        sqlite3ExprCode(pParse, pExpr->pLeft);
      }
      for(i=0; i<nExpr; i=i+2){
        sqlite3ExprCode(pParse, aListelem[i].pExpr);
        if( pExpr->pLeft ){
          sqlite3VdbeAddOp(v, OP_Dup, 1, 1);
          jumpInst = codeCompare(pParse, pExpr->pLeft, aListelem[i].pExpr,
                                 OP_Ne, 0, 1);
          sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
        }else{
          jumpInst = sqlite3VdbeAddOp(v, OP_IfNot, 1, 0);
        }
        sqlite3ExprCode(pParse, aListelem[i+1].pExpr);
        sqlite3VdbeAddOp(v, OP_Goto, 0, expr_end_label);
        sqlite3VdbeJumpHere(v, jumpInst);
      }
      if( pExpr->pLeft ){
        sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      }
      if( pExpr->pRight ){
        sqlite3ExprCode(pParse, pExpr->pRight);
      }else{
        sqlite3VdbeAddOp(v, OP_Null, 0, 0);
      }
      sqlite3VdbeResolveLabel(v, expr_end_label);
      break;
    }
#ifndef SQLITE_OMIT_TRIGGER
    case TK_RAISE: {
      if( !pParse->trigStack ){
        sqlite3ErrorMsg(pParse,
                       "RAISE() may only be used within a trigger-program");
	return;
      }
      if( pExpr->iColumn!=OE_Ignore ){
         assert( pExpr->iColumn==OE_Rollback ||
                 pExpr->iColumn == OE_Abort ||
                 pExpr->iColumn == OE_Fail );
         sqlite3DequoteExpr(pExpr);
         sqlite3VdbeOp3(v, OP_Halt, SQLITE_CONSTRAINT, pExpr->iColumn,
                        (char*)pExpr->token.z, pExpr->token.n);
      } else {
         assert( pExpr->iColumn == OE_Ignore );
         sqlite3VdbeAddOp(v, OP_ContextPop, 0, 0);
         sqlite3VdbeAddOp(v, OP_Goto, 0, pParse->trigStack->ignoreJump);
         VdbeComment((v, "# raise(IGNORE)"));
      }
      stackChng = 0;
      break;
    }
#endif
  }

  if( pParse->ckOffset ){
    pParse->ckOffset += stackChng;
    assert( pParse->ckOffset );
  }
}

#ifndef SQLITE_OMIT_TRIGGER










void sqlite3ExprCodeAndCache(Parse *pParse, Expr *pExpr){
  Vdbe *v = pParse->pVdbe;
  int iMem;
  int addr1, addr2;
  if( v==0 ) return;
  addr1 = sqlite3VdbeCurrentAddr(v);
  sqlite3ExprCode(pParse, pExpr);
  addr2 = sqlite3VdbeCurrentAddr(v);
  if( addr2>addr1+1 || sqlite3VdbeGetOp(v, addr1)->opcode==OP_Function ){
    iMem = pExpr->iTable = pParse->nMem++;
    sqlite3VdbeAddOp(v, OP_MemStore, iMem, 0);
    pExpr->op = TK_REGISTER;
  }
}
#endif







int sqlite3ExprCodeExprList(
  Parse *pParse,     
  ExprList *pList    
){
  struct ExprList_item *pItem;
  int i, n;
  if( pList==0 ) return 0;
  n = pList->nExpr;
  for(pItem=pList->a, i=n; i>0; i--, pItem++){
    sqlite3ExprCode(pParse, pItem->pExpr);
  }
  return n;
}















void sqlite3ExprIfTrue(Parse *pParse, Expr *pExpr, int dest, int jumpIfNull){
  Vdbe *v = pParse->pVdbe;
  int op = 0;
  int ckOffset = pParse->ckOffset;
  if( v==0 || pExpr==0 ) return;
  op = pExpr->op;
  switch( op ){
    case TK_AND: {
      int d2 = sqlite3VdbeMakeLabel(v);
      sqlite3ExprIfFalse(pParse, pExpr->pLeft, d2, !jumpIfNull);
      sqlite3ExprIfTrue(pParse, pExpr->pRight, dest, jumpIfNull);
      sqlite3VdbeResolveLabel(v, d2);
      break;
    }
    case TK_OR: {
      sqlite3ExprIfTrue(pParse, pExpr->pLeft, dest, jumpIfNull);
      sqlite3ExprIfTrue(pParse, pExpr->pRight, dest, jumpIfNull);
      break;
    }
    case TK_NOT: {
      sqlite3ExprIfFalse(pParse, pExpr->pLeft, dest, jumpIfNull);
      break;
    }
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
    case TK_NE:
    case TK_EQ: {
      assert( TK_LT==OP_Lt );
      assert( TK_LE==OP_Le );
      assert( TK_GT==OP_Gt );
      assert( TK_GE==OP_Ge );
      assert( TK_EQ==OP_Eq );
      assert( TK_NE==OP_Ne );
      sqlite3ExprCode(pParse, pExpr->pLeft);
      sqlite3ExprCode(pParse, pExpr->pRight);
      codeCompare(pParse, pExpr->pLeft, pExpr->pRight, op, dest, jumpIfNull);
      break;
    }
    case TK_ISNULL:
    case TK_NOTNULL: {
      assert( TK_ISNULL==OP_IsNull );
      assert( TK_NOTNULL==OP_NotNull );
      sqlite3ExprCode(pParse, pExpr->pLeft);
      sqlite3VdbeAddOp(v, op, 1, dest);
      break;
    }
    case TK_BETWEEN: {
      





      int addr;
      Expr *pLeft = pExpr->pLeft;
      Expr *pRight = pExpr->pList->a[0].pExpr;
      sqlite3ExprCode(pParse, pLeft);
      sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
      sqlite3ExprCode(pParse, pRight);
      addr = codeCompare(pParse, pLeft, pRight, OP_Lt, 0, !jumpIfNull);

      pRight = pExpr->pList->a[1].pExpr;
      sqlite3ExprCode(pParse, pRight);
      codeCompare(pParse, pLeft, pRight, OP_Le, dest, jumpIfNull);

      sqlite3VdbeAddOp(v, OP_Integer, 0, 0);
      sqlite3VdbeJumpHere(v, addr);
      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      break;
    }
    default: {
      sqlite3ExprCode(pParse, pExpr);
      sqlite3VdbeAddOp(v, OP_If, jumpIfNull, dest);
      break;
    }
  }
  pParse->ckOffset = ckOffset;
}









void sqlite3ExprIfFalse(Parse *pParse, Expr *pExpr, int dest, int jumpIfNull){
  Vdbe *v = pParse->pVdbe;
  int op = 0;
  int ckOffset = pParse->ckOffset;
  if( v==0 || pExpr==0 ) return;

  

















  op = ((pExpr->op+(TK_ISNULL&1))^1)-(TK_ISNULL&1);

  

  assert( pExpr->op!=TK_ISNULL || op==OP_NotNull );
  assert( pExpr->op!=TK_NOTNULL || op==OP_IsNull );
  assert( pExpr->op!=TK_NE || op==OP_Eq );
  assert( pExpr->op!=TK_EQ || op==OP_Ne );
  assert( pExpr->op!=TK_LT || op==OP_Ge );
  assert( pExpr->op!=TK_LE || op==OP_Gt );
  assert( pExpr->op!=TK_GT || op==OP_Le );
  assert( pExpr->op!=TK_GE || op==OP_Lt );

  switch( pExpr->op ){
    case TK_AND: {
      sqlite3ExprIfFalse(pParse, pExpr->pLeft, dest, jumpIfNull);
      sqlite3ExprIfFalse(pParse, pExpr->pRight, dest, jumpIfNull);
      break;
    }
    case TK_OR: {
      int d2 = sqlite3VdbeMakeLabel(v);
      sqlite3ExprIfTrue(pParse, pExpr->pLeft, d2, !jumpIfNull);
      sqlite3ExprIfFalse(pParse, pExpr->pRight, dest, jumpIfNull);
      sqlite3VdbeResolveLabel(v, d2);
      break;
    }
    case TK_NOT: {
      sqlite3ExprIfTrue(pParse, pExpr->pLeft, dest, jumpIfNull);
      break;
    }
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
    case TK_NE:
    case TK_EQ: {
      sqlite3ExprCode(pParse, pExpr->pLeft);
      sqlite3ExprCode(pParse, pExpr->pRight);
      codeCompare(pParse, pExpr->pLeft, pExpr->pRight, op, dest, jumpIfNull);
      break;
    }
    case TK_ISNULL:
    case TK_NOTNULL: {
      sqlite3ExprCode(pParse, pExpr->pLeft);
      sqlite3VdbeAddOp(v, op, 1, dest);
      break;
    }
    case TK_BETWEEN: {
      





      int addr;
      Expr *pLeft = pExpr->pLeft;
      Expr *pRight = pExpr->pList->a[0].pExpr;
      sqlite3ExprCode(pParse, pLeft);
      sqlite3VdbeAddOp(v, OP_Dup, 0, 0);
      sqlite3ExprCode(pParse, pRight);
      addr = sqlite3VdbeCurrentAddr(v);
      codeCompare(pParse, pLeft, pRight, OP_Ge, addr+3, !jumpIfNull);

      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
      sqlite3VdbeAddOp(v, OP_Goto, 0, dest);
      pRight = pExpr->pList->a[1].pExpr;
      sqlite3ExprCode(pParse, pRight);
      codeCompare(pParse, pLeft, pRight, OP_Gt, dest, jumpIfNull);
      break;
    }
    default: {
      sqlite3ExprCode(pParse, pExpr);
      sqlite3VdbeAddOp(v, OP_IfNot, jumpIfNull, dest);
      break;
    }
  }
  pParse->ckOffset = ckOffset;
}





int sqlite3ExprCompare(Expr *pA, Expr *pB){
  int i;
  if( pA==0||pB==0 ){
    return pB==pA;
  }
  if( pA->op!=pB->op ) return 0;
  if( (pA->flags & EP_Distinct)!=(pB->flags & EP_Distinct) ) return 0;
  if( !sqlite3ExprCompare(pA->pLeft, pB->pLeft) ) return 0;
  if( !sqlite3ExprCompare(pA->pRight, pB->pRight) ) return 0;
  if( pA->pList ){
    if( pB->pList==0 ) return 0;
    if( pA->pList->nExpr!=pB->pList->nExpr ) return 0;
    for(i=0; i<pA->pList->nExpr; i++){
      if( !sqlite3ExprCompare(pA->pList->a[i].pExpr, pB->pList->a[i].pExpr) ){
        return 0;
      }
    }
  }else if( pB->pList ){
    return 0;
  }
  if( pA->pSelect || pB->pSelect ) return 0;
  if( pA->iTable!=pB->iTable || pA->iColumn!=pB->iColumn ) return 0;
  if( pA->token.z ){
    if( pB->token.z==0 ) return 0;
    if( pB->token.n!=pA->token.n ) return 0;
    if( sqlite3StrNICmp((char*)pA->token.z,(char*)pB->token.z,pB->token.n)!=0 ){
      return 0;
    }
  }
  return 1;
}






static int addAggInfoColumn(AggInfo *pInfo){
  int i;
  i = sqlite3ArrayAllocate((void**)&pInfo->aCol, sizeof(pInfo->aCol[0]), 3);
  if( i<0 ){
    return -1;
  }
  return i;
}    





static int addAggInfoFunc(AggInfo *pInfo){
  int i;
  i = sqlite3ArrayAllocate((void**)&pInfo->aFunc, sizeof(pInfo->aFunc[0]), 2);
  if( i<0 ){
    return -1;
  }
  return i;
}    








static int analyzeAggregate(void *pArg, Expr *pExpr){
  int i;
  NameContext *pNC = (NameContext *)pArg;
  Parse *pParse = pNC->pParse;
  SrcList *pSrcList = pNC->pSrcList;
  AggInfo *pAggInfo = pNC->pAggInfo;
  

  switch( pExpr->op ){
    case TK_COLUMN: {
      

      if( pSrcList ){
        struct SrcList_item *pItem = pSrcList->a;
        for(i=0; i<pSrcList->nSrc; i++, pItem++){
          struct AggInfo_col *pCol;
          if( pExpr->iTable==pItem->iCursor ){
            





            pCol = pAggInfo->aCol;
            for(i=0; i<pAggInfo->nColumn; i++, pCol++){
              if( pCol->iTable==pExpr->iTable &&
                  pCol->iColumn==pExpr->iColumn ){
                break;
              }
            }
            if( i>=pAggInfo->nColumn && (i = addAggInfoColumn(pAggInfo))>=0 ){
              pCol = &pAggInfo->aCol[i];
              pCol->iTable = pExpr->iTable;
              pCol->iColumn = pExpr->iColumn;
              pCol->iMem = pParse->nMem++;
              pCol->iSorterColumn = -1;
              pCol->pExpr = pExpr;
              if( pAggInfo->pGroupBy ){
                int j, n;
                ExprList *pGB = pAggInfo->pGroupBy;
                struct ExprList_item *pTerm = pGB->a;
                n = pGB->nExpr;
                for(j=0; j<n; j++, pTerm++){
                  Expr *pE = pTerm->pExpr;
                  if( pE->op==TK_COLUMN && pE->iTable==pExpr->iTable &&
                      pE->iColumn==pExpr->iColumn ){
                    pCol->iSorterColumn = j;
                    break;
                  }
                }
              }
              if( pCol->iSorterColumn<0 ){
                pCol->iSorterColumn = pAggInfo->nSortingColumn++;
              }
            }
            




            pExpr->pAggInfo = pAggInfo;
            pExpr->op = TK_AGG_COLUMN;
            pExpr->iAgg = i;
            break;
          } 
        } 
      }
      return 1;
    }
    case TK_AGG_FUNCTION: {
      

      if( pNC->nDepth==0 ){
        


        struct AggInfo_func *pItem = pAggInfo->aFunc;
        for(i=0; i<pAggInfo->nFunc; i++, pItem++){
          if( sqlite3ExprCompare(pItem->pExpr, pExpr) ){
            break;
          }
        }
        if( i>=pAggInfo->nFunc ){
          

          u8 enc = ENC(pParse->db);
          i = addAggInfoFunc(pAggInfo);
          if( i>=0 ){
            pItem = &pAggInfo->aFunc[i];
            pItem->pExpr = pExpr;
            pItem->iMem = pParse->nMem++;
            pItem->pFunc = sqlite3FindFunction(pParse->db,
                   (char*)pExpr->token.z, pExpr->token.n,
                   pExpr->pList ? pExpr->pList->nExpr : 0, enc, 0);
            if( pExpr->flags & EP_Distinct ){
              pItem->iDistinct = pParse->nTab++;
            }else{
              pItem->iDistinct = -1;
            }
          }
        }
        

        pExpr->iAgg = i;
        pExpr->pAggInfo = pAggInfo;
        return 1;
      }
    }
  }

  



  if( pExpr->pSelect ){
    pNC->nDepth++;
    walkSelectExpr(pExpr->pSelect, analyzeAggregate, pNC);
    pNC->nDepth--;
  }
  return 0;
}












int sqlite3ExprAnalyzeAggregates(NameContext *pNC, Expr *pExpr){
  int nErr = pNC->pParse->nErr;
  walkExprTree(pExpr, analyzeAggregate, pNC);
  return pNC->pParse->nErr - nErr;
}







int sqlite3ExprAnalyzeAggList(NameContext *pNC, ExprList *pList){
  struct ExprList_item *pItem;
  int i;
  int nErr = 0;
  if( pList ){
    for(pItem=pList->a, i=0; nErr==0 && i<pList->nExpr; i++, pItem++){
      nErr += sqlite3ExprAnalyzeAggregates(pNC, pItem->pExpr);
    }
  }
  return nErr;
}
