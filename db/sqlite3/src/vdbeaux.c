















#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>
#include "vdbeInt.h"







#ifdef SQLITE_DEBUG
int sqlite3_vdbe_addop_trace = 0;
#endif





Vdbe *sqlite3VdbeCreate(sqlite3 *db){
  Vdbe *p;
  p = sqliteMalloc( sizeof(Vdbe) );
  if( p==0 ) return 0;
  p->db = db;
  if( db->pVdbe ){
    db->pVdbe->pPrev = p;
  }
  p->pNext = db->pVdbe;
  p->pPrev = 0;
  db->pVdbe = p;
  p->magic = VDBE_MAGIC_INIT;
  return p;
}




void sqlite3VdbeTrace(Vdbe *p, FILE *trace){
  p->trace = trace;
}













static void resizeOpArray(Vdbe *p, int N){
  int runMode = p->magic==VDBE_MAGIC_RUN;
  if( runMode || p->nOpAlloc<N ){
    VdbeOp *pNew;
    int nNew = N + 100*(!runMode);
    int oldSize = p->nOpAlloc;
    pNew = sqliteRealloc(p->aOp, nNew*sizeof(Op));
    if( pNew ){
      p->nOpAlloc = nNew;
      p->aOp = pNew;
      if( nNew>oldSize ){
        memset(&p->aOp[oldSize], 0, (nNew-oldSize)*sizeof(Op));
      }
    }
  }
}

















int sqlite3VdbeAddOp(Vdbe *p, int op, int p1, int p2){
  int i;
  VdbeOp *pOp;

  i = p->nOp;
  p->nOp++;
  assert( p->magic==VDBE_MAGIC_INIT );
  if( p->nOpAlloc<=i ){
    resizeOpArray(p, i+1);
    if( sqlite3MallocFailed() ){
      return 0;
    }
  }
  pOp = &p->aOp[i];
  pOp->opcode = op;
  pOp->p1 = p1;
  pOp->p2 = p2;
  pOp->p3 = 0;
  pOp->p3type = P3_NOTUSED;
  p->expired = 0;
#ifdef SQLITE_DEBUG
  if( sqlite3_vdbe_addop_trace ) sqlite3VdbePrintOp(0, i, &p->aOp[i]);
#endif
  return i;
}




int sqlite3VdbeOp3(Vdbe *p, int op, int p1, int p2, const char *zP3,int p3type){
  int addr = sqlite3VdbeAddOp(p, op, p1, p2);
  sqlite3VdbeChangeP3(p, addr, zP3, p3type);
  return addr;
}















int sqlite3VdbeMakeLabel(Vdbe *p){
  int i;
  i = p->nLabel++;
  assert( p->magic==VDBE_MAGIC_INIT );
  if( i>=p->nLabelAlloc ){
    p->nLabelAlloc = p->nLabelAlloc*2 + 10;
    sqliteReallocOrFree((void**)&p->aLabel,
                          p->nLabelAlloc*sizeof(p->aLabel[0]));
  }
  if( p->aLabel ){
    p->aLabel[i] = -1;
  }
  return -1-i;
}






void sqlite3VdbeResolveLabel(Vdbe *p, int x){
  int j = -1-x;
  assert( p->magic==VDBE_MAGIC_INIT );
  assert( j>=0 && j<p->nLabel );
  if( p->aLabel ){
    p->aLabel[j] = p->nOp;
  }
}





static int opcodeNoPush(u8 op){
  


















 
  static const u32 masks[5] = {
    NOPUSH_MASK_0 + (((unsigned)NOPUSH_MASK_1)<<16),
    NOPUSH_MASK_2 + (((unsigned)NOPUSH_MASK_3)<<16),
    NOPUSH_MASK_4 + (((unsigned)NOPUSH_MASK_5)<<16),
    NOPUSH_MASK_6 + (((unsigned)NOPUSH_MASK_7)<<16),
    NOPUSH_MASK_8 + (((unsigned)NOPUSH_MASK_9)<<16)
  };
  assert( op<32*5 );
  return (masks[op>>5] & (1<<(op&0x1F)));
}

#ifndef NDEBUG
int sqlite3VdbeOpcodeNoPush(u8 op){
  return opcodeNoPush(op);
}
#endif





















static void resolveP2Values(Vdbe *p, int *pMaxFuncArgs, int *pMaxStack){
  int i;
  int nMaxArgs = 0;
  int nMaxStack = p->nOp;
  Op *pOp;
  int *aLabel = p->aLabel;
  int doesStatementRollback = 0;
  int hasStatementBegin = 0;
  for(pOp=p->aOp, i=p->nOp-1; i>=0; i--, pOp++){
    u8 opcode = pOp->opcode;

    if( opcode==OP_Function || opcode==OP_AggStep ){
      if( pOp->p2>nMaxArgs ) nMaxArgs = pOp->p2;
    }else if( opcode==OP_Halt ){
      if( pOp->p1==SQLITE_CONSTRAINT && pOp->p2==OE_Abort ){
        doesStatementRollback = 1;
      }
    }else if( opcode==OP_IdxInsert ){
      if( pOp->p2 ){
        doesStatementRollback = 1;
      }
    }else if( opcode==OP_Statement ){
      hasStatementBegin = 1;
    }

    if( opcodeNoPush(opcode) ){
      nMaxStack--;
    }

    if( pOp->p2>=0 ) continue;
    assert( -1-pOp->p2<p->nLabel );
    pOp->p2 = aLabel[-1-pOp->p2];
  }
  sqliteFree(p->aLabel);
  p->aLabel = 0;

  *pMaxFuncArgs = nMaxArgs;
  *pMaxStack = nMaxStack;

  




  if( hasStatementBegin && !doesStatementRollback ){
    for(pOp=p->aOp, i=p->nOp-1; i>=0; i--, pOp++){
      if( pOp->opcode==OP_Statement ){
        pOp->opcode = OP_Noop;
      }
    }
  }
}




int sqlite3VdbeCurrentAddr(Vdbe *p){
  assert( p->magic==VDBE_MAGIC_INIT );
  return p->nOp;
}





int sqlite3VdbeAddOpList(Vdbe *p, int nOp, VdbeOpList const *aOp){
  int addr;
  assert( p->magic==VDBE_MAGIC_INIT );
  resizeOpArray(p, p->nOp + nOp);
  if( sqlite3MallocFailed() ){
    return 0;
  }
  addr = p->nOp;
  if( nOp>0 ){
    int i;
    VdbeOpList const *pIn = aOp;
    for(i=0; i<nOp; i++, pIn++){
      int p2 = pIn->p2;
      VdbeOp *pOut = &p->aOp[i+addr];
      pOut->opcode = pIn->opcode;
      pOut->p1 = pIn->p1;
      pOut->p2 = p2<0 ? addr + ADDR(p2) : p2;
      pOut->p3 = pIn->p3;
      pOut->p3type = pIn->p3 ? P3_STATIC : P3_NOTUSED;
#ifdef SQLITE_DEBUG
      if( sqlite3_vdbe_addop_trace ){
        sqlite3VdbePrintOp(0, i+addr, &p->aOp[i+addr]);
      }
#endif
    }
    p->nOp += nOp;
  }
  return addr;
}







void sqlite3VdbeChangeP1(Vdbe *p, int addr, int val){
  assert( p==0 || p->magic==VDBE_MAGIC_INIT );
  if( p && addr>=0 && p->nOp>addr && p->aOp ){
    p->aOp[addr].p1 = val;
  }
}





void sqlite3VdbeChangeP2(Vdbe *p, int addr, int val){
  assert( val>=0 );
  assert( p==0 || p->magic==VDBE_MAGIC_INIT );
  if( p && addr>=0 && p->nOp>addr && p->aOp ){
    p->aOp[addr].p2 = val;
  }
}





void sqlite3VdbeJumpHere(Vdbe *p, int addr){
  sqlite3VdbeChangeP2(p, addr, p->nOp);
}




static void freeP3(int p3type, void *p3){
  if( p3 ){
    switch( p3type ){
      case P3_DYNAMIC:
      case P3_KEYINFO:
      case P3_KEYINFO_HANDOFF: {
        sqliteFree(p3);
        break;
      }
      case P3_VDBEFUNC: {
        VdbeFunc *pVdbeFunc = (VdbeFunc *)p3;
        sqlite3VdbeDeleteAuxData(pVdbeFunc, 0);
        sqliteFree(pVdbeFunc);
        break;
      }
      case P3_MEM: {
        sqlite3ValueFree((sqlite3_value*)p3);
        break;
      }
    }
  }
}





void sqlite3VdbeChangeToNoop(Vdbe *p, int addr, int N){
  VdbeOp *pOp = &p->aOp[addr];
  while( N-- ){
    freeP3(pOp->p3type, pOp->p3);
    memset(pOp, 0, sizeof(pOp[0]));
    pOp->opcode = OP_Noop;
    pOp++;
  }
}


























void sqlite3VdbeChangeP3(Vdbe *p, int addr, const char *zP3, int n){
  Op *pOp;
  assert( p==0 || p->magic==VDBE_MAGIC_INIT );
  if( p==0 || p->aOp==0 || sqlite3MallocFailed() ){
    if (n != P3_KEYINFO) {
      freeP3(n, (void*)*(char**)&zP3);
    }
    return;
  }
  if( addr<0 || addr>=p->nOp ){
    addr = p->nOp - 1;
    if( addr<0 ) return;
  }
  pOp = &p->aOp[addr];
  freeP3(pOp->p3type, pOp->p3);
  pOp->p3 = 0;
  if( zP3==0 ){
    pOp->p3 = 0;
    pOp->p3type = P3_NOTUSED;
  }else if( n==P3_KEYINFO ){
    KeyInfo *pKeyInfo;
    int nField, nByte;

    nField = ((KeyInfo*)zP3)->nField;
    nByte = sizeof(*pKeyInfo) + (nField-1)*sizeof(pKeyInfo->aColl[0]) + nField;
    pKeyInfo = sqliteMallocRaw( nByte );
    pOp->p3 = (char*)pKeyInfo;
    if( pKeyInfo ){
      unsigned char *aSortOrder;
      memcpy(pKeyInfo, zP3, nByte);
      aSortOrder = pKeyInfo->aSortOrder;
      if( aSortOrder ){
        pKeyInfo->aSortOrder = (unsigned char*)&pKeyInfo->aColl[nField];
        memcpy(pKeyInfo->aSortOrder, aSortOrder, nField);
      }
      pOp->p3type = P3_KEYINFO;
    }else{
      pOp->p3type = P3_NOTUSED;
    }
  }else if( n==P3_KEYINFO_HANDOFF ){
    pOp->p3 = (char*)zP3;
    pOp->p3type = P3_KEYINFO;
  }else if( n<0 ){
    pOp->p3 = (char*)zP3;
    pOp->p3type = n;
  }else{
    if( n==0 ) n = strlen(zP3);
    pOp->p3 = sqliteStrNDup(zP3, n);
    pOp->p3type = P3_DYNAMIC;
  }
}

#ifndef NDEBUG




void sqlite3VdbeComment(Vdbe *p, const char *zFormat, ...){
  va_list ap;
  assert( p->nOp>0 );
  assert( p->aOp==0 || p->aOp[p->nOp-1].p3==0 
          || sqlite3MallocFailed() );
  va_start(ap, zFormat);
  sqlite3VdbeChangeP3(p, -1, sqlite3VMPrintf(zFormat, ap), P3_DYNAMIC);
  va_end(ap);
}
#endif




VdbeOp *sqlite3VdbeGetOp(Vdbe *p, int addr){
  assert( p->magic==VDBE_MAGIC_INIT );
  assert( addr>=0 && addr<p->nOp );
  return &p->aOp[addr];
}

#if !defined(SQLITE_OMIT_EXPLAIN) || !defined(NDEBUG) \
     || defined(VDBE_PROFILE) || defined(SQLITE_DEBUG)




static char *displayP3(Op *pOp, char *zTemp, int nTemp){
  char *zP3;
  assert( nTemp>=20 );
  switch( pOp->p3type ){
    case P3_KEYINFO: {
      int i, j;
      KeyInfo *pKeyInfo = (KeyInfo*)pOp->p3;
      sprintf(zTemp, "keyinfo(%d", pKeyInfo->nField);
      i = strlen(zTemp);
      for(j=0; j<pKeyInfo->nField; j++){
        CollSeq *pColl = pKeyInfo->aColl[j];
        if( pColl ){
          int n = strlen(pColl->zName);
          if( i+n>nTemp-6 ){
            strcpy(&zTemp[i],",...");
            break;
          }
          zTemp[i++] = ',';
          if( pKeyInfo->aSortOrder && pKeyInfo->aSortOrder[j] ){
            zTemp[i++] = '-';
          }
          strcpy(&zTemp[i], pColl->zName);
          i += n;
        }else if( i+4<nTemp-6 ){
          strcpy(&zTemp[i],",nil");
          i += 4;
        }
      }
      zTemp[i++] = ')';
      zTemp[i] = 0;
      assert( i<nTemp );
      zP3 = zTemp;
      break;
    }
    case P3_COLLSEQ: {
      CollSeq *pColl = (CollSeq*)pOp->p3;
      sprintf(zTemp, "collseq(%.20s)", pColl->zName);
      zP3 = zTemp;
      break;
    }
    case P3_FUNCDEF: {
      FuncDef *pDef = (FuncDef*)pOp->p3;
      char zNum[30];
      sprintf(zTemp, "%.*s", nTemp, pDef->zName);
      sprintf(zNum,"(%d)", pDef->nArg);
      if( strlen(zTemp)+strlen(zNum)+1<=nTemp ){
        strcat(zTemp, zNum);
      }
      zP3 = zTemp;
      break;
    }
    default: {
      zP3 = pOp->p3;
      if( zP3==0 || pOp->opcode==OP_Noop ){
        zP3 = "";
      }
    }
  }
  return zP3;
}
#endif


#if defined(VDBE_PROFILE) || defined(SQLITE_DEBUG)



void sqlite3VdbePrintOp(FILE *pOut, int pc, Op *pOp){
  char *zP3;
  char zPtr[50];
  static const char *zFormat1 = "%4d %-13s %4d %4d %s\n";
  if( pOut==0 ) pOut = stdout;
  zP3 = displayP3(pOp, zPtr, sizeof(zPtr));
  fprintf(pOut, zFormat1,
      pc, sqlite3OpcodeNames[pOp->opcode], pOp->p1, pOp->p2, zP3);
  fflush(pOut);
}
#endif




static void releaseMemArray(Mem *p, int N){
  if( p ){
    while( N-->0 ){
      sqlite3VdbeMemRelease(p++);
    }
  }
}

#ifndef SQLITE_OMIT_EXPLAIN







int sqlite3VdbeList(
  Vdbe *p                   
){
  sqlite3 *db = p->db;
  int i;
  int rc = SQLITE_OK;

  assert( p->explain );
  if( p->magic!=VDBE_MAGIC_RUN ) return SQLITE_MISUSE;
  assert( db->magic==SQLITE_MAGIC_BUSY );
  assert( p->rc==SQLITE_OK || p->rc==SQLITE_BUSY );

  



  if( p->pTos==&p->aStack[4] ){
    releaseMemArray(p->aStack, 5);
  }
  p->resOnStack = 0;

  do{
    i = p->pc++;
  }while( i<p->nOp && p->explain==2 && p->aOp[i].opcode!=OP_Explain );
  if( i>=p->nOp ){
    p->rc = SQLITE_OK;
    rc = SQLITE_DONE;
  }else if( db->flags & SQLITE_Interrupt ){
    db->flags &= ~SQLITE_Interrupt;
    p->rc = SQLITE_INTERRUPT;
    rc = SQLITE_ERROR;
    sqlite3SetString(&p->zErrMsg, sqlite3ErrStr(p->rc), (char*)0);
  }else{
    Op *pOp = &p->aOp[i];
    Mem *pMem = p->aStack;
    pMem->flags = MEM_Int;
    pMem->type = SQLITE_INTEGER;
    pMem->i = i;                                
    pMem++;

    pMem->flags = MEM_Static|MEM_Str|MEM_Term;
    pMem->z = sqlite3OpcodeNames[pOp->opcode];  
    pMem->n = strlen(pMem->z);
    pMem->type = SQLITE_TEXT;
    pMem->enc = SQLITE_UTF8;
    pMem++;

    pMem->flags = MEM_Int;
    pMem->i = pOp->p1;                          
    pMem->type = SQLITE_INTEGER;
    pMem++;

    pMem->flags = MEM_Int;
    pMem->i = pOp->p2;                          
    pMem->type = SQLITE_INTEGER;
    pMem++;

    pMem->flags = MEM_Ephem|MEM_Str|MEM_Term;   
    pMem->z = displayP3(pOp, pMem->zShort, sizeof(pMem->zShort));
    pMem->n = strlen(pMem->z);
    pMem->type = SQLITE_TEXT;
    pMem->enc = SQLITE_UTF8;

    p->nResColumn = 5 - 2*(p->explain-1);
    p->pTos = pMem;
    p->rc = SQLITE_OK;
    p->resOnStack = 1;
    rc = SQLITE_ROW;
  }
  return rc;
}
#endif 




void sqlite3VdbePrintSql(Vdbe *p){
#ifdef SQLITE_DEBUG
  int nOp = p->nOp;
  VdbeOp *pOp;
  if( nOp<1 ) return;
  pOp = &p->aOp[nOp-1];
  if( pOp->opcode==OP_Noop && pOp->p3!=0 ){
    const char *z = pOp->p3;
    while( isspace(*(u8*)z) ) z++;
    printf("SQL: [%s]\n", z);
  }
#endif
}










void sqlite3VdbeMakeReady(
  Vdbe *p,                       
  int nVar,                      
  int nMem,                      
  int nCursor,                   
  int isExplain                  
){
  int n;

  assert( p!=0 );
  assert( p->magic==VDBE_MAGIC_INIT );

  

  assert( p->nOp>0 );

  




  p->magic = VDBE_MAGIC_RUN;

  








  if( p->aStack==0 ){
    int nArg;       
    int nStack;     
    resolveP2Values(p, &nArg, &nStack);
    resizeOpArray(p, p->nOp);
    assert( nVar>=0 );
    assert( nStack<p->nOp );
    nStack = isExplain ? 10 : nStack;
    p->aStack = sqliteMalloc(
        nStack*sizeof(p->aStack[0])    
      + nArg*sizeof(Mem*)              
      + nVar*sizeof(Mem)               
      + nVar*sizeof(char*)             
      + nMem*sizeof(Mem)               
      + nCursor*sizeof(Cursor*)        
    );
    if( !sqlite3MallocFailed() ){
      p->aMem = &p->aStack[nStack];
      p->nMem = nMem;
      p->aVar = &p->aMem[nMem];
      p->nVar = nVar;
      p->okVar = 0;
      p->apArg = (Mem**)&p->aVar[nVar];
      p->azVar = (char**)&p->apArg[nArg];
      p->apCsr = (Cursor**)&p->azVar[nVar];
      p->nCursor = nCursor;
      for(n=0; n<nVar; n++){
        p->aVar[n].flags = MEM_Null;
      }
    }
  }
  for(n=0; n<p->nMem; n++){
    p->aMem[n].flags = MEM_Null;
  }

#ifdef SQLITE_DEBUG
  if( (p->db->flags & SQLITE_VdbeListing)!=0
    || sqlite3OsFileExists("vdbe_explain")
  ){
    int i;
    printf("VDBE Program Listing:\n");
    sqlite3VdbePrintSql(p);
    for(i=0; i<p->nOp; i++){
      sqlite3VdbePrintOp(stdout, i, &p->aOp[i]);
    }
  }
  if( sqlite3OsFileExists("vdbe_trace") ){
    p->trace = stdout;
  }
#endif
  p->pTos = &p->aStack[-1];
  p->pc = -1;
  p->rc = SQLITE_OK;
  p->uniqueCnt = 0;
  p->returnDepth = 0;
  p->errorAction = OE_Abort;
  p->popStack =  0;
  p->explain |= isExplain;
  p->magic = VDBE_MAGIC_RUN;
  p->nChange = 0;
  p->cacheCtr = 1;
  p->minWriteFileFormat = 255;
#ifdef VDBE_PROFILE
  {
    int i;
    for(i=0; i<p->nOp; i++){
      p->aOp[i].cnt = 0;
      p->aOp[i].cycles = 0;
    }
  }
#endif
}





void sqlite3VdbeFreeCursor(Cursor *pCx){
  if( pCx==0 ){
    return;
  }
  if( pCx->pCursor ){
    sqlite3BtreeCloseCursor(pCx->pCursor);
  }
  if( pCx->pBt ){
    sqlite3BtreeClose(pCx->pBt);
  }
  sqliteFree(pCx->pData);
  sqliteFree(pCx->aType);
  sqliteFree(pCx);
}




static void closeAllCursors(Vdbe *p){
  int i;
  if( p->apCsr==0 ) return;
  for(i=0; i<p->nCursor; i++){
    sqlite3VdbeFreeCursor(p->apCsr[i]);
    p->apCsr[i] = 0;
  }
}








static void Cleanup(Vdbe *p){
  int i;
  if( p->aStack ){
    releaseMemArray(p->aStack, 1 + (p->pTos - p->aStack));
    p->pTos = &p->aStack[-1];
  }
  closeAllCursors(p);
  releaseMemArray(p->aMem, p->nMem);
  sqlite3VdbeFifoClear(&p->sFifo);
  if( p->contextStack ){
    for(i=0; i<p->contextStackTop; i++){
      sqlite3VdbeFifoClear(&p->contextStack[i].sFifo);
    }
    sqliteFree(p->contextStack);
  }
  p->contextStack = 0;
  p->contextStackDepth = 0;
  p->contextStackTop = 0;
  sqliteFree(p->zErrMsg);
  p->zErrMsg = 0;
}







void sqlite3VdbeSetNumCols(Vdbe *p, int nResColumn){
  Mem *pColName;
  int n;
  releaseMemArray(p->aColName, p->nResColumn*COLNAME_N);
  sqliteFree(p->aColName);
  n = nResColumn*COLNAME_N;
  p->nResColumn = nResColumn;
  p->aColName = pColName = (Mem*)sqliteMalloc( sizeof(Mem)*n );
  if( p->aColName==0 ) return;
  while( n-- > 0 ){
    (pColName++)->flags = MEM_Null;
  }
}












int sqlite3VdbeSetColName(Vdbe *p, int idx, int var, const char *zName, int N){
  int rc;
  Mem *pColName;
  assert( idx<p->nResColumn );
  assert( var<COLNAME_N );
  if( sqlite3MallocFailed() ) return SQLITE_NOMEM;
  assert( p->aColName!=0 );
  pColName = &(p->aColName[idx+var*p->nResColumn]);
  if( N==P3_DYNAMIC || N==P3_STATIC ){
    rc = sqlite3VdbeMemSetStr(pColName, zName, -1, SQLITE_UTF8, SQLITE_STATIC);
  }else{
    rc = sqlite3VdbeMemSetStr(pColName, zName, N, SQLITE_UTF8,SQLITE_TRANSIENT);
  }
  if( rc==SQLITE_OK && N==P3_DYNAMIC ){
    pColName->flags = (pColName->flags&(~MEM_Static))|MEM_Dyn;
    pColName->xDel = 0;
  }
  return rc;
}







static int vdbeCommit(sqlite3 *db){
  int i;
  int nTrans = 0;  
  int rc = SQLITE_OK;
  int needXcommit = 0;

  for(i=0; i<db->nDb; i++){ 
    Btree *pBt = db->aDb[i].pBt;
    if( pBt && sqlite3BtreeIsInTrans(pBt) ){
      needXcommit = 1;
      if( i!=1 ) nTrans++;
    }
  }

  
  if( needXcommit && db->xCommitCallback ){
    sqlite3SafetyOff(db);
    rc = db->xCommitCallback(db->pCommitArg);
    sqlite3SafetyOn(db);
    if( rc ){
      return SQLITE_CONSTRAINT;
    }
  }

  








  if( 0==strlen(sqlite3BtreeGetFilename(db->aDb[0].pBt)) || nTrans<=1 ){
    for(i=0; rc==SQLITE_OK && i<db->nDb; i++){ 
      Btree *pBt = db->aDb[i].pBt;
      if( pBt ){
        rc = sqlite3BtreeSync(pBt, 0);
      }
    }

    
    if( rc==SQLITE_OK ){
      for(i=0; i<db->nDb; i++){
        Btree *pBt = db->aDb[i].pBt;
        if( pBt ){
          sqlite3BtreeCommit(pBt);
        }
      }
    }
  }

  



#ifndef SQLITE_OMIT_DISKIO
  else{
    int needSync = 0;
    char *zMaster = 0;   
    char const *zMainFile = sqlite3BtreeGetFilename(db->aDb[0].pBt);
    OsFile *master = 0;

    
    do {
      u32 random;
      sqliteFree(zMaster);
      sqlite3Randomness(sizeof(random), &random);
      zMaster = sqlite3MPrintf("%s-mj%08X", zMainFile, random&0x7fffffff);
      if( !zMaster ){
        return SQLITE_NOMEM;
      }
    }while( sqlite3OsFileExists(zMaster) );

    
    rc = sqlite3OsOpenExclusive(zMaster, &master, 0);
    if( rc!=SQLITE_OK ){
      sqliteFree(zMaster);
      return rc;
    }
 
    





    for(i=0; i<db->nDb; i++){ 
      Btree *pBt = db->aDb[i].pBt;
      if( i==1 ) continue;   
      if( pBt && sqlite3BtreeIsInTrans(pBt) ){
        char const *zFile = sqlite3BtreeGetJournalname(pBt);
        if( zFile[0]==0 ) continue;  
        if( !needSync && !sqlite3BtreeSyncDisabled(pBt) ){
          needSync = 1;
        }
        rc = sqlite3OsWrite(master, zFile, strlen(zFile)+1);
        if( rc!=SQLITE_OK ){
          sqlite3OsClose(&master);
          sqlite3OsDelete(zMaster);
          sqliteFree(zMaster);
          return rc;
        }
      }
    }


    


    zMainFile = sqlite3BtreeGetDirname(db->aDb[0].pBt);
    rc = sqlite3OsOpenDirectory(master, zMainFile);
    if( rc!=SQLITE_OK ||
          (needSync && (rc=sqlite3OsSync(master,0))!=SQLITE_OK) ){
      sqlite3OsClose(&master);
      sqlite3OsDelete(zMaster);
      sqliteFree(zMaster);
      return rc;
    }

    









    for(i=0; i<db->nDb; i++){ 
      Btree *pBt = db->aDb[i].pBt;
      if( pBt && sqlite3BtreeIsInTrans(pBt) ){
        rc = sqlite3BtreeSync(pBt, zMaster);
        if( rc!=SQLITE_OK ){
          sqlite3OsClose(&master);
          sqliteFree(zMaster);
          return rc;
        }
      }
    }
    sqlite3OsClose(&master);

    



    rc = sqlite3OsDelete(zMaster);
    assert( rc==SQLITE_OK );
    sqliteFree(zMaster);
    zMaster = 0;
    rc = sqlite3OsSyncDirectory(zMainFile);
    if( rc!=SQLITE_OK ){
      






      return rc;
    }

    






    for(i=0; i<db->nDb; i++){ 
      Btree *pBt = db->aDb[i].pBt;
      if( pBt ){
        sqlite3BtreeCommit(pBt);
      }
    }
  }
#endif

  return rc;
}








void sqlite3AbortOtherActiveVdbes(sqlite3 *db, Vdbe *pExcept){
  Vdbe *pOther;
  for(pOther=db->pVdbe; pOther; pOther=pOther->pNext){
    if( pOther==pExcept ) continue;
    if( pOther->magic!=VDBE_MAGIC_RUN || pOther->pc<0 ) continue;
    closeAllCursors(pOther);
    pOther->aborted = 1;
  }
}










#ifndef NDEBUG
static void checkActiveVdbeCnt(sqlite3 *db){
  Vdbe *p;
  int cnt = 0;
  p = db->pVdbe;
  while( p ){
    if( p->magic==VDBE_MAGIC_RUN && p->pc>=0 ){
      cnt++;
    }
    p = p->pNext;
  }
  assert( cnt==db->activeVdbeCnt );
}
#else
#define checkActiveVdbeCnt(x)
#endif













int sqlite3VdbeHalt(Vdbe *p){
  sqlite3 *db = p->db;
  int i;
  int (*xFunc)(Btree *pBt) = 0;  
  int isSpecialError;            

  



























  if( sqlite3MallocFailed() ){
    p->rc = SQLITE_NOMEM;
  }
  if( p->magic!=VDBE_MAGIC_RUN ){
    
    assert( p->magic==VDBE_MAGIC_HALT );
    return SQLITE_OK;
  }
  closeAllCursors(p);
  checkActiveVdbeCnt(db);

  
  if( p->pc>=0 ){

    
    isSpecialError = ((p->rc==SQLITE_NOMEM || p->rc==SQLITE_IOERR)?1:0);
    if( isSpecialError ){
      












      int isReadOnly = 1;
      int isStatement = 0;
      assert(p->aOp || p->nOp==0);
      for(i=0; i<p->nOp; i++){ 
        switch( p->aOp[i].opcode ){
          case OP_Transaction:
            isReadOnly = 0;
            break;
          case OP_Statement:
            isStatement = 1;
            break;
        }
      }
  
      


      if( !isReadOnly ){
        if( p->rc==SQLITE_NOMEM && isStatement ){
          xFunc = sqlite3BtreeRollbackStmt;
        }else{
          


          sqlite3AbortOtherActiveVdbes(db, p);
          sqlite3RollbackAll(db);
          db->autoCommit = 1;
        }
      }
    }
  
    





    if( db->autoCommit && db->activeVdbeCnt==1 ){
      if( p->rc==SQLITE_OK || (p->errorAction==OE_Fail && !isSpecialError) ){
	



        int rc = vdbeCommit(db);
        if( rc==SQLITE_BUSY ){
          return SQLITE_BUSY;
        }else if( rc!=SQLITE_OK ){
          p->rc = rc;
          sqlite3RollbackAll(db);
        }else{
          sqlite3CommitInternalChanges(db);
        }
      }else{
        sqlite3RollbackAll(db);
      }
    }else if( !xFunc ){
      if( p->rc==SQLITE_OK || p->errorAction==OE_Fail ){
        xFunc = sqlite3BtreeCommitStmt;
      }else if( p->errorAction==OE_Abort ){
        xFunc = sqlite3BtreeRollbackStmt;
      }else{
        sqlite3AbortOtherActiveVdbes(db, p);
        sqlite3RollbackAll(db);
        db->autoCommit = 1;
      }
    }
  
    




    assert(!xFunc ||
      xFunc==sqlite3BtreeCommitStmt ||
      xFunc==sqlite3BtreeRollbackStmt
    );
    for(i=0; xFunc && i<db->nDb; i++){ 
      int rc;
      Btree *pBt = db->aDb[i].pBt;
      if( pBt ){
        rc = xFunc(pBt);
        if( rc && (p->rc==SQLITE_OK || p->rc==SQLITE_CONSTRAINT) ){
          p->rc = rc;
          sqlite3SetString(&p->zErrMsg, 0);
        }
      }
    }
  
    


    if( p->changeCntOn && p->pc>=0 ){
      if( !xFunc || xFunc==sqlite3BtreeCommitStmt ){
        sqlite3VdbeSetChanges(db, p->nChange);
      }else{
        sqlite3VdbeSetChanges(db, 0);
      }
      p->nChange = 0;
    }
  
    
    if( p->rc!=SQLITE_OK && db->flags&SQLITE_InternChanges ){
      sqlite3ResetInternalSchema(db, 0);
      db->flags = (db->flags | SQLITE_InternChanges);
    }
  }

  
  if( p->pc>=0 ){
    db->activeVdbeCnt--;
  }
  p->magic = VDBE_MAGIC_HALT;
  checkActiveVdbeCnt(db);

  return SQLITE_OK;
}












int sqlite3VdbeReset(Vdbe *p){
  if( p->magic!=VDBE_MAGIC_RUN && p->magic!=VDBE_MAGIC_HALT ){
    sqlite3Error(p->db, SQLITE_MISUSE, 0);
    return SQLITE_MISUSE;
  }

  



  sqlite3VdbeHalt(p);

  




  if( p->pc>=0 ){
    if( p->zErrMsg ){
      sqlite3* db = p->db;
      sqlite3ValueSetStr(db->pErr, -1, p->zErrMsg, SQLITE_UTF8, sqlite3FreeX);
      db->errCode = p->rc;
      p->zErrMsg = 0;
    }else if( p->rc ){
      sqlite3Error(p->db, p->rc, 0);
    }else{
      sqlite3Error(p->db, SQLITE_OK, 0);
    }
  }else if( p->rc && p->expired ){
    



    sqlite3Error(p->db, p->rc, 0);
  }

  

  Cleanup(p);

  

  assert( p->pTos<&p->aStack[p->pc<0?0:p->pc] || !p->aStack );
#ifdef VDBE_PROFILE
  {
    FILE *out = fopen("vdbe_profile.out", "a");
    if( out ){
      int i;
      fprintf(out, "---- ");
      for(i=0; i<p->nOp; i++){
        fprintf(out, "%02x", p->aOp[i].opcode);
      }
      fprintf(out, "\n");
      for(i=0; i<p->nOp; i++){
        fprintf(out, "%6d %10lld %8lld ",
           p->aOp[i].cnt,
           p->aOp[i].cycles,
           p->aOp[i].cnt>0 ? p->aOp[i].cycles/p->aOp[i].cnt : 0
        );
        sqlite3VdbePrintOp(out, i, &p->aOp[i]);
      }
      fclose(out);
    }
  }
#endif
  p->magic = VDBE_MAGIC_INIT;
  p->aborted = 0;
  if( p->rc==SQLITE_SCHEMA ){
    sqlite3ResetInternalSchema(p->db, 0);
  }
  return p->rc;
}
 




int sqlite3VdbeFinalize(Vdbe *p){
  int rc = SQLITE_OK;

  if( p->magic==VDBE_MAGIC_RUN || p->magic==VDBE_MAGIC_HALT ){
    rc = sqlite3VdbeReset(p);
  }else if( p->magic!=VDBE_MAGIC_INIT ){
    return SQLITE_MISUSE;
  }
  sqlite3VdbeDelete(p);
  return rc;
}







void sqlite3VdbeDeleteAuxData(VdbeFunc *pVdbeFunc, int mask){
  int i;
  for(i=0; i<pVdbeFunc->nAux; i++){
    struct AuxData *pAux = &pVdbeFunc->apAux[i];
    if( (i>31 || !(mask&(1<<i))) && pAux->pAux ){
      if( pAux->xDelete ){
        pAux->xDelete(pAux->pAux);
      }
      pAux->pAux = 0;
    }
  }
}




void sqlite3VdbeDelete(Vdbe *p){
  int i;
  if( p==0 ) return;
  Cleanup(p);
  if( p->pPrev ){
    p->pPrev->pNext = p->pNext;
  }else{
    assert( p->db->pVdbe==p );
    p->db->pVdbe = p->pNext;
  }
  if( p->pNext ){
    p->pNext->pPrev = p->pPrev;
  }
  if( p->aOp ){
    for(i=0; i<p->nOp; i++){
      Op *pOp = &p->aOp[i];
      freeP3(pOp->p3type, pOp->p3);
    }
    sqliteFree(p->aOp);
  }
  releaseMemArray(p->aVar, p->nVar);
  sqliteFree(p->aLabel);
  sqliteFree(p->aStack);
  releaseMemArray(p->aColName, p->nResColumn*COLNAME_N);
  sqliteFree(p->aColName);
  p->magic = VDBE_MAGIC_DEAD;
  sqliteFree(p);
}






int sqlite3VdbeCursorMoveto(Cursor *p){
  if( p->deferredMoveto ){
    int res, rc;
    extern int sqlite3_search_count;
    assert( p->isTable );
    if( p->isTable ){
      rc = sqlite3BtreeMoveto(p->pCursor, 0, p->movetoTarget, &res);
    }else{
      rc = sqlite3BtreeMoveto(p->pCursor,(char*)&p->movetoTarget,
                              sizeof(i64),&res);
    }
    if( rc ) return rc;
    *p->pIncrKey = 0;
    p->lastRowid = keyToInt(p->movetoTarget);
    p->rowidIsValid = res==0;
    if( res<0 ){
      rc = sqlite3BtreeNext(p->pCursor, &res);
      if( rc ) return rc;
    }
    sqlite3_search_count++;
    p->deferredMoveto = 0;
    p->cacheStatus = CACHE_STALE;
  }
  return SQLITE_OK;
}














































u32 sqlite3VdbeSerialType(Mem *pMem, int file_format){
  int flags = pMem->flags;

  if( flags&MEM_Null ){
    return 0;
  }
  if( flags&MEM_Int ){
    
#   define MAX_6BYTE ((((i64)0x00001000)<<32)-1)
    i64 i = pMem->i;
    u64 u;
    if( file_format>=4 && (i&1)==i ){
      return 8+i;
    }
    u = i<0 ? -i : i;
    if( u<=127 ) return 1;
    if( u<=32767 ) return 2;
    if( u<=8388607 ) return 3;
    if( u<=2147483647 ) return 4;
    if( u<=MAX_6BYTE ) return 5;
    return 6;
  }
  if( flags&MEM_Real ){
    return 7;
  }
  if( flags&MEM_Str ){
    int n = pMem->n;
    assert( n>=0 );
    return ((n*2) + 13);
  }
  if( flags&MEM_Blob ){
    return (pMem->n*2 + 12);
  }
  return 0;
}




int sqlite3VdbeSerialTypeLen(u32 serial_type){
  if( serial_type>=12 ){
    return (serial_type-12)/2;
  }else{
    static const u8 aSize[] = { 0, 1, 2, 3, 4, 6, 8, 8, 0, 0, 0, 0 };
    return aSize[serial_type];
  }
}





 
int sqlite3VdbeSerialPut(unsigned char *buf, Mem *pMem, int file_format){
  u32 serial_type = sqlite3VdbeSerialType(pMem, file_format);
  int len;

  
  if( serial_type<=7 && serial_type>0 ){
    u64 v;
    int i;
    if( serial_type==7 ){
      v = *(u64*)&pMem->r;
    }else{
      v = *(u64*)&pMem->i;
    }
    len = i = sqlite3VdbeSerialTypeLen(serial_type);
    while( i-- ){
      buf[i] = (v&0xFF);
      v >>= 8;
    }
    return len;
  }

  
  if( serial_type>=12 ){
    len = sqlite3VdbeSerialTypeLen(serial_type);
    memcpy(buf, pMem->z, len);
    return len;
  }

  
  return 0;
}




 
int sqlite3VdbeSerialGet(
  const unsigned char *buf,     
  u32 serial_type,              
  Mem *pMem                     
){
  switch( serial_type ){
    case 10:   
    case 11:   
    case 0: {  
      pMem->flags = MEM_Null;
      break;
    }
    case 1: { 
      pMem->i = (signed char)buf[0];
      pMem->flags = MEM_Int;
      return 1;
    }
    case 2: { 
      pMem->i = (((signed char)buf[0])<<8) | buf[1];
      pMem->flags = MEM_Int;
      return 2;
    }
    case 3: { 
      pMem->i = (((signed char)buf[0])<<16) | (buf[1]<<8) | buf[2];
      pMem->flags = MEM_Int;
      return 3;
    }
    case 4: { 
      pMem->i = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
      pMem->flags = MEM_Int;
      return 4;
    }
    case 5: { 
      u64 x = (((signed char)buf[0])<<8) | buf[1];
      u32 y = (buf[2]<<24) | (buf[3]<<16) | (buf[4]<<8) | buf[5];
      x = (x<<32) | y;
      pMem->i = *(i64*)&x;
      pMem->flags = MEM_Int;
      return 6;
    }
    case 6:   
    case 7: { 
      u64 x;
      u32 y;
#if !defined(NDEBUG) && !defined(SQLITE_OMIT_FLOATING_POINT)
      


      static const u64 t1 = ((u64)0x3ff00000)<<32;
      assert( 1.0==*(double*)&t1 );
#endif

      x = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
      y = (buf[4]<<24) | (buf[5]<<16) | (buf[6]<<8) | buf[7];
      x = (x<<32) | y;
      if( serial_type==6 ){
        pMem->i = *(i64*)&x;
        pMem->flags = MEM_Int;
      }else{
        pMem->r = *(double*)&x;
        pMem->flags = MEM_Real;
      }
      return 8;
    }
    case 8:    
    case 9: {  
      pMem->i = serial_type-8;
      pMem->flags = MEM_Int;
      return 0;
    }
    default: {
      int len = (serial_type-12)/2;
      pMem->z = (char *)buf;
      pMem->n = len;
      pMem->xDel = 0;
      if( serial_type&0x01 ){
        pMem->flags = MEM_Str | MEM_Ephem;
      }else{
        pMem->flags = MEM_Blob | MEM_Ephem;
      }
      return len;
    }
  }
  return 0;
}
















#define GetVarint(A,B)  ((B = *(A))<=0x7f ? 1 : sqlite3GetVarint32(A, &B))








int sqlite3VdbeRecordCompare(
  void *userData,
  int nKey1, const void *pKey1, 
  int nKey2, const void *pKey2
){
  KeyInfo *pKeyInfo = (KeyInfo*)userData;
  u32 d1, d2;          
  u32 idx1, idx2;      
  u32 szHdr1, szHdr2;  
  int i = 0;
  int nField;
  int rc = 0;
  const unsigned char *aKey1 = (const unsigned char *)pKey1;
  const unsigned char *aKey2 = (const unsigned char *)pKey2;

  Mem mem1;
  Mem mem2;
  mem1.enc = pKeyInfo->enc;
  mem2.enc = pKeyInfo->enc;
  
  idx1 = GetVarint(aKey1, szHdr1);
  d1 = szHdr1;
  idx2 = GetVarint(aKey2, szHdr2);
  d2 = szHdr2;
  nField = pKeyInfo->nField;
  while( idx1<szHdr1 && idx2<szHdr2 ){
    u32 serial_type1;
    u32 serial_type2;

    
    idx1 += GetVarint( aKey1+idx1, serial_type1 );
    if( d1>=nKey1 && sqlite3VdbeSerialTypeLen(serial_type1)>0 ) break;
    idx2 += GetVarint( aKey2+idx2, serial_type2 );
    if( d2>=nKey2 && sqlite3VdbeSerialTypeLen(serial_type2)>0 ) break;

    




    d1 += sqlite3VdbeSerialGet(&aKey1[d1], serial_type1, &mem1);
    d2 += sqlite3VdbeSerialGet(&aKey2[d2], serial_type2, &mem2);

    rc = sqlite3MemCompare(&mem1, &mem2, i<nField ? pKeyInfo->aColl[i] : 0);
    if( mem1.flags & MEM_Dyn ) sqlite3VdbeMemRelease(&mem1);
    if( mem2.flags & MEM_Dyn ) sqlite3VdbeMemRelease(&mem2);
    if( rc!=0 ){
      break;
    }
    i++;
  }

  



  if( rc==0 ){
    if( pKeyInfo->incrKey ){
      rc = -1;
    }else if( d1<nKey1 ){
      rc = 1;
    }else if( d2<nKey2 ){
      rc = -1;
    }
  }else if( pKeyInfo->aSortOrder && i<pKeyInfo->nField
               && pKeyInfo->aSortOrder[i] ){
    rc = -rc;
  }

  return rc;
}







int sqlite3VdbeIdxRowidLen(const u8 *aKey){
  u32 szHdr;        
  u32 typeRowid;    

  sqlite3GetVarint32(aKey, &szHdr);
  sqlite3GetVarint32(&aKey[szHdr-1], &typeRowid);
  return sqlite3VdbeSerialTypeLen(typeRowid);
}
  






int sqlite3VdbeIdxRowid(BtCursor *pCur, i64 *rowid){
  i64 nCellKey;
  int rc;
  u32 szHdr;        
  u32 typeRowid;    
  u32 lenRowid;     
  Mem m, v;

  sqlite3BtreeKeySize(pCur, &nCellKey);
  if( nCellKey<=0 ){
    return SQLITE_CORRUPT_BKPT;
  }
  rc = sqlite3VdbeMemFromBtree(pCur, 0, nCellKey, 1, &m);
  if( rc ){
    return rc;
  }
  sqlite3GetVarint32((u8*)m.z, &szHdr);
  sqlite3GetVarint32((u8*)&m.z[szHdr-1], &typeRowid);
  lenRowid = sqlite3VdbeSerialTypeLen(typeRowid);
  sqlite3VdbeSerialGet((u8*)&m.z[m.n-lenRowid], typeRowid, &v);
  *rowid = v.i;
  sqlite3VdbeMemRelease(&m);
  return SQLITE_OK;
}











int sqlite3VdbeIdxKeyCompare(
  Cursor *pC,                 
  int nKey, const u8 *pKey,   
  int *res                    
){
  i64 nCellKey;
  int rc;
  BtCursor *pCur = pC->pCursor;
  int lenRowid;
  Mem m;

  sqlite3BtreeKeySize(pCur, &nCellKey);
  if( nCellKey<=0 ){
    *res = 0;
    return SQLITE_OK;
  }
  rc = sqlite3VdbeMemFromBtree(pC->pCursor, 0, nCellKey, 1, &m);
  if( rc ){
    return rc;
  }
  lenRowid = sqlite3VdbeIdxRowidLen((u8*)m.z);
  *res = sqlite3VdbeRecordCompare(pC->pKeyInfo, m.n-lenRowid, m.z, nKey, pKey);
  sqlite3VdbeMemRelease(&m);
  return SQLITE_OK;
}





void sqlite3VdbeSetChanges(sqlite3 *db, int nChange){
  db->nChange = nChange;
  db->nTotalChange += nChange;
}





void sqlite3VdbeCountChanges(Vdbe *v){
  v->changeCntOn = 1;
}











void sqlite3ExpirePreparedStatements(sqlite3 *db){
  Vdbe *p;
  for(p = db->pVdbe; p; p=p->pNext){
    p->expired = 1;
  }
}




sqlite3 *sqlite3VdbeDb(Vdbe *v){
  return v->db;
}
