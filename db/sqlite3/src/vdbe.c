














































#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>
#include "vdbeInt.h"








int sqlite3_search_count = 0;









int sqlite3_interrupt_count = 0;








int sqlite3_sort_count = 0;





#define Release(P) if((P)->flags&MEM_Dyn){ sqlite3VdbeMemRelease(P); }





#define Stringify(P, enc) \
   if(((P)->flags&(MEM_Str|MEM_Blob))==0 && sqlite3VdbeMemStringify(P,enc)) \
     { goto no_mem; }








#define Dynamicify(P,enc) sqlite3VdbeMemDynamicify(P)
















#define GetVarint(A,B)  ((B = *(A))<=0x7f ? 1 : sqlite3GetVarint32(A, &B))












#define Deephemeralize(P) \
   if( ((P)->flags&MEM_Ephem)!=0 \
       && sqlite3VdbeMemMakeWriteable(P) ){ goto no_mem;}








#define storeTypeInfo(A,B) _storeTypeInfo(A)
static void _storeTypeInfo(Mem *pMem){
  int flags = pMem->flags;
  if( flags & MEM_Null ){
    pMem->type = SQLITE_NULL;
  }
  else if( flags & MEM_Int ){
    pMem->type = SQLITE_INTEGER;
  }
  else if( flags & MEM_Real ){
    pMem->type = SQLITE_FLOAT;
  }
  else if( flags & MEM_Str ){
    pMem->type = SQLITE_TEXT;
  }else{
    pMem->type = SQLITE_BLOB;
  }
}




static void popStack(Mem **ppTos, int N){
  Mem *pTos = *ppTos;
  while( N>0 ){
    N--;
    Release(pTos);
    pTos--;
  }
  *ppTos = pTos;
}





static Cursor *allocateCursor(Vdbe *p, int iCur, int iDb){
  Cursor *pCx;
  assert( iCur<p->nCursor );
  if( p->apCsr[iCur] ){
    sqlite3VdbeFreeCursor(p->apCsr[iCur]);
  }
  p->apCsr[iCur] = pCx = sqliteMalloc( sizeof(Cursor) );
  if( pCx ){
    pCx->iDb = iDb;
  }
  return pCx;
}







static void applyNumericAffinity(Mem *pRec){
  if( (pRec->flags & (MEM_Real|MEM_Int))==0 ){
    int realnum;
    sqlite3VdbeMemNulTerminate(pRec);
    if( (pRec->flags&MEM_Str)
         && sqlite3IsNumber(pRec->z, &realnum, pRec->enc) ){
      i64 value;
      sqlite3VdbeChangeEncoding(pRec, SQLITE_UTF8);
      if( !realnum && sqlite3atoi64(pRec->z, &value) ){
        sqlite3VdbeMemRelease(pRec);
        pRec->i = value;
        pRec->flags = MEM_Int;
      }else{
        sqlite3VdbeMemRealify(pRec);
      }
    }
  }
}



















static void applyAffinity(Mem *pRec, char affinity, u8 enc){
  if( affinity==SQLITE_AFF_TEXT ){
    



    if( 0==(pRec->flags&MEM_Str) && (pRec->flags&(MEM_Real|MEM_Int)) ){
      sqlite3VdbeMemStringify(pRec, enc);
    }
    pRec->flags &= ~(MEM_Real|MEM_Int);
  }else if( affinity!=SQLITE_AFF_NONE ){
    assert( affinity==SQLITE_AFF_INTEGER || affinity==SQLITE_AFF_REAL
             || affinity==SQLITE_AFF_NUMERIC );
    applyNumericAffinity(pRec);
    if( pRec->flags & MEM_Real ){
      sqlite3VdbeIntegerAffinity(pRec);
    }
  }
}









int sqlite3_value_numeric_type(sqlite3_value *pVal){
  Mem *pMem = (Mem*)pVal;
  applyNumericAffinity(pMem);
  storeTypeInfo(pMem, 0);
  return pMem->type;
}





void sqlite3ValueApplyAffinity(sqlite3_value *pVal, u8 affinity, u8 enc){
  applyAffinity((Mem *)pVal, affinity, enc);
}

#ifdef SQLITE_DEBUG




void sqlite3VdbeMemPrettyPrint(Mem *pMem, char *zBuf){
  char *zCsr = zBuf;
  int f = pMem->flags;

  static const char *const encnames[] = {"(X)", "(8)", "(16LE)", "(16BE)"};

  if( f&MEM_Blob ){
    int i;
    char c;
    if( f & MEM_Dyn ){
      c = 'z';
      assert( (f & (MEM_Static|MEM_Ephem))==0 );
    }else if( f & MEM_Static ){
      c = 't';
      assert( (f & (MEM_Dyn|MEM_Ephem))==0 );
    }else if( f & MEM_Ephem ){
      c = 'e';
      assert( (f & (MEM_Static|MEM_Dyn))==0 );
    }else{
      c = 's';
    }

    zCsr += sprintf(zCsr, "%c", c);
    zCsr += sprintf(zCsr, "%d[", pMem->n);
    for(i=0; i<16 && i<pMem->n; i++){
      zCsr += sprintf(zCsr, "%02X ", ((int)pMem->z[i] & 0xFF));
    }
    for(i=0; i<16 && i<pMem->n; i++){
      char z = pMem->z[i];
      if( z<32 || z>126 ) *zCsr++ = '.';
      else *zCsr++ = z;
    }

    zCsr += sprintf(zCsr, "]");
    *zCsr = '\0';
  }else if( f & MEM_Str ){
    int j, k;
    zBuf[0] = ' ';
    if( f & MEM_Dyn ){
      zBuf[1] = 'z';
      assert( (f & (MEM_Static|MEM_Ephem))==0 );
    }else if( f & MEM_Static ){
      zBuf[1] = 't';
      assert( (f & (MEM_Dyn|MEM_Ephem))==0 );
    }else if( f & MEM_Ephem ){
      zBuf[1] = 'e';
      assert( (f & (MEM_Static|MEM_Dyn))==0 );
    }else{
      zBuf[1] = 's';
    }
    k = 2;
    k += sprintf(&zBuf[k], "%d", pMem->n);
    zBuf[k++] = '[';
    for(j=0; j<15 && j<pMem->n; j++){
      u8 c = pMem->z[j];
      if( c>=0x20 && c<0x7f ){
        zBuf[k++] = c;
      }else{
        zBuf[k++] = '.';
      }
    }
    zBuf[k++] = ']';
    k += sprintf(&zBuf[k], encnames[pMem->enc]);
    zBuf[k++] = 0;
  }
}
#endif


#ifdef VDBE_PROFILE






__inline__ unsigned long long int hwtime(void){
  unsigned long long int x;
  __asm__("rdtsc\n\t"
          "mov %%edx, %%ecx\n\t"
          :"=A" (x));
  return x;
}
#endif











#define CHECK_FOR_INTERRUPT \
   if( db->flags & SQLITE_Interrupt ) goto abort_due_to_interrupt;

































int sqlite3VdbeExec(
  Vdbe *p                    
){
  int pc;                    
  Op *pOp;                   
  int rc = SQLITE_OK;        
  sqlite3 *db = p->db;       
  u8 encoding = ENC(db);     
  Mem *pTos;                 
#ifdef VDBE_PROFILE
  unsigned long long start;  
  int origPc;                
#endif
#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
  int nProgressOps = 0;      
#endif
#ifndef NDEBUG
  Mem *pStackLimit;
#endif

  if( p->magic!=VDBE_MAGIC_RUN ) return SQLITE_MISUSE;
  assert( db->magic==SQLITE_MAGIC_BUSY );
  pTos = p->pTos;
  if( p->rc==SQLITE_NOMEM ){
    

    goto no_mem;
  }
  assert( p->rc==SQLITE_OK || p->rc==SQLITE_BUSY );
  p->rc = SQLITE_OK;
  assert( p->explain==0 );
  if( p->popStack ){
    popStack(&pTos, p->popStack);
    p->popStack = 0;
  }
  p->resOnStack = 0;
  db->busyHandler.nBusy = 0;
  CHECK_FOR_INTERRUPT;
  for(pc=p->pc; rc==SQLITE_OK; pc++){
    assert( pc>=0 && pc<p->nOp );
    assert( pTos<=&p->aStack[pc] );
    if( sqlite3MallocFailed() ) goto no_mem;
#ifdef VDBE_PROFILE
    origPc = pc;
    start = hwtime();
#endif
    pOp = &p->aOp[pc];

    

#ifdef SQLITE_DEBUG
    if( p->trace ){
      if( pc==0 ){
        printf("VDBE Execution Trace:\n");
        sqlite3VdbePrintSql(p);
      }
      sqlite3VdbePrintOp(p->trace, pc, pOp);
    }
    if( p->trace==0 && pc==0 && sqlite3OsFileExists("vdbe_sqltrace") ){
      sqlite3VdbePrintSql(p);
    }
#endif
      

    


#ifdef SQLITE_TEST
    if( sqlite3_interrupt_count>0 ){
      sqlite3_interrupt_count--;
      if( sqlite3_interrupt_count==0 ){
        sqlite3_interrupt(db);
      }
    }
#endif

#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
    





    if( db->xProgress ){
      if( db->nProgressOps==nProgressOps ){
        if( db->xProgress(db->pProgressArg)!=0 ){
          rc = SQLITE_ABORT;
          continue; 
        }
        nProgressOps = 0;
      }
      nProgressOps++;
    }
#endif

#ifndef NDEBUG
    








 
    pStackLimit = pTos;
    if( !sqlite3VdbeOpcodeNoPush(pOp->opcode) ){
      pStackLimit++;
    }
#endif

    switch( pOp->opcode ){











































case OP_Goto: {             
  CHECK_FOR_INTERRUPT;
  pc = pOp->p2 - 1;
  break;
}











case OP_Gosub: {            
  assert( p->returnDepth<sizeof(p->returnStack)/sizeof(p->returnStack[0]) );
  p->returnStack[p->returnDepth++] = pc+1;
  pc = pOp->p2 - 1;
  break;
}







case OP_Return: {           
  assert( p->returnDepth>0 );
  p->returnDepth--;
  pc = p->returnStack[p->returnDepth] - 1;
  break;
}




















case OP_Halt: {            
  p->pTos = pTos;
  p->rc = pOp->p1;
  p->pc = pc;
  p->errorAction = pOp->p2;
  if( pOp->p3 ){
    sqlite3SetString(&p->zErrMsg, pOp->p3, (char*)0);
  }
  rc = sqlite3VdbeHalt(p);
  assert( rc==SQLITE_BUSY || rc==SQLITE_OK );
  if( rc==SQLITE_BUSY ){
    p->rc = SQLITE_BUSY;
    return SQLITE_BUSY;
  }
  return p->rc ? SQLITE_ERROR : SQLITE_DONE;
}





case OP_Integer: {
  pTos++;
  pTos->flags = MEM_Int;
  pTos->i = pOp->p1;
  break;
}






case OP_Int64: {
  pTos++;
  assert( pOp->p3!=0 );
  pTos->flags = MEM_Str|MEM_Static|MEM_Term;
  pTos->z = pOp->p3;
  pTos->n = strlen(pTos->z);
  pTos->enc = SQLITE_UTF8;
  pTos->i = sqlite3VdbeIntValue(pTos);
  pTos->flags |= MEM_Int;
  break;
}





case OP_Real: {            
  pTos++;
  pTos->flags = MEM_Str|MEM_Static|MEM_Term;
  pTos->z = pOp->p3;
  pTos->n = strlen(pTos->z);
  pTos->enc = SQLITE_UTF8;
  pTos->r = sqlite3VdbeRealValue(pTos);
  pTos->flags |= MEM_Real;
  sqlite3VdbeChangeEncoding(pTos, encoding);
  break;
}






case OP_String8: {         
  assert( pOp->p3!=0 );
  pOp->opcode = OP_String;
  pOp->p1 = strlen(pOp->p3);

#ifndef SQLITE_OMIT_UTF16
  if( encoding!=SQLITE_UTF8 ){
    pTos++;
    sqlite3VdbeMemSetStr(pTos, pOp->p3, -1, SQLITE_UTF8, SQLITE_STATIC);
    if( SQLITE_OK!=sqlite3VdbeChangeEncoding(pTos, encoding) ) goto no_mem;
    if( SQLITE_OK!=sqlite3VdbeMemDynamicify(pTos) ) goto no_mem;
    pTos->flags &= ~(MEM_Dyn);
    pTos->flags |= MEM_Static;
    if( pOp->p3type==P3_DYNAMIC ){
      sqliteFree(pOp->p3);
    }
    pOp->p3type = P3_DYNAMIC;
    pOp->p3 = pTos->z;
    pOp->p1 = pTos->n;
    break;
  }
#endif
  
}
  




case OP_String: {
  pTos++;
  assert( pOp->p3!=0 );
  pTos->flags = MEM_Str|MEM_Static|MEM_Term;
  pTos->z = pOp->p3;
  pTos->n = pOp->p1;
  pTos->enc = encoding;
  break;
}





case OP_Null: {
  pTos++;
  pTos->flags = MEM_Null;
  pTos->n = 0;
  break;
}


#ifndef SQLITE_OMIT_BLOB_LITERAL








case OP_HexBlob: {            
  pOp->opcode = OP_Blob;
  pOp->p1 = strlen(pOp->p3)/2;
  if( pOp->p1 ){
    char *zBlob = sqlite3HexToBlob(pOp->p3);
    if( !zBlob ) goto no_mem;
    if( pOp->p3type==P3_DYNAMIC ){
      sqliteFree(pOp->p3);
    }
    pOp->p3 = zBlob;
    pOp->p3type = P3_DYNAMIC;
  }else{
    if( pOp->p3type==P3_DYNAMIC ){
      sqliteFree(pOp->p3);
    }
    pOp->p3type = P3_STATIC;
    pOp->p3 = "";
  }

  
}










case OP_Blob: {
  pTos++;
  sqlite3VdbeMemSetStr(pTos, pOp->p3, pOp->p1, 0, 0);
  break;
}
#endif 










case OP_Variable: {
  int j = pOp->p1 - 1;
  assert( j>=0 && j<p->nVar );

  pTos++;
  sqlite3VdbeMemShallowCopy(pTos, &p->aVar[j], MEM_Static);
  break;
}





case OP_Pop: {            
  assert( pOp->p1>=0 );
  popStack(&pTos, pOp->p1);
  assert( pTos>=&p->aStack[-1] );
  break;
}
















case OP_Dup: {
  Mem *pFrom = &pTos[-pOp->p1];
  assert( pFrom<=pTos && pFrom>=p->aStack );
  pTos++;
  sqlite3VdbeMemShallowCopy(pTos, pFrom, MEM_Ephem);
  if( pOp->p2 ){
    Deephemeralize(pTos);
  }
  break;
}











case OP_Pull: {            
  Mem *pFrom = &pTos[-pOp->p1];
  int i;
  Mem ts;

  ts = *pFrom;
  Deephemeralize(pTos);
  for(i=0; i<pOp->p1; i++, pFrom++){
    Deephemeralize(&pFrom[1]);
    assert( (pFrom->flags & MEM_Ephem)==0 );
    *pFrom = pFrom[1];
    if( pFrom->flags & MEM_Short ){
      assert( pFrom->flags & (MEM_Str|MEM_Blob) );
      assert( pFrom->z==pFrom[1].zShort );
      pFrom->z = pFrom->zShort;
    }
  }
  *pTos = ts;
  if( pTos->flags & MEM_Short ){
    assert( pTos->flags & (MEM_Str|MEM_Blob) );
    assert( pTos->z==pTos[-pOp->p1].zShort );
    pTos->z = pTos->zShort;
  }
  break;
}







case OP_Push: {            
  Mem *pTo = &pTos[-pOp->p1];

  assert( pTo>=p->aStack );
  sqlite3VdbeMemMove(pTo, pTos);
  pTos--;
  break;
}











case OP_Callback: {            
  Mem *pMem;
  Mem *pFirstColumn;
  assert( p->nResColumn==pOp->p1 );

  





  pFirstColumn = &pTos[0-pOp->p1];
  for(pMem = p->aStack; pMem<pFirstColumn; pMem++){
    Deephemeralize(pMem);
  }

  
  p->cacheCtr = (p->cacheCtr + 2)|1;

  



  for(; pMem<=pTos; pMem++ ){
    sqlite3VdbeMemNulTerminate(pMem);
    storeTypeInfo(pMem, encoding);
  }

  


  p->resOnStack = 1;
  p->nCallback++;
  p->popStack = pOp->p1;
  p->pc = pc + 1;
  p->pTos = pTos;
  return SQLITE_ROW;
}











case OP_Concat: {           
  char *zNew;
  int nByte;
  int nField;
  int i, j;
  Mem *pTerm;

  
  nField = pOp->p1 + 2;
  pTerm = &pTos[1-nField];
  nByte = 0;
  for(i=0; i<nField; i++, pTerm++){
    assert( pOp->p2==0 || (pTerm->flags&MEM_Str) );
    if( pTerm->flags&MEM_Null ){
      nByte = -1;
      break;
    }
    Stringify(pTerm, encoding);
    nByte += pTerm->n;
  }

  if( nByte<0 ){
    



    if( pOp->p2==0 ){
      popStack(&pTos, nField);
    }
    pTos++;
    pTos->flags = MEM_Null;
  }else{
    


    zNew = sqliteMallocRaw( nByte+2 );
    if( zNew==0 ) goto no_mem;
    j = 0;
    pTerm = &pTos[1-nField];
    for(i=j=0; i<nField; i++, pTerm++){
      int n = pTerm->n;
      assert( pTerm->flags & (MEM_Str|MEM_Blob) );
      memcpy(&zNew[j], pTerm->z, n);
      j += n;
    }
    zNew[j] = 0;
    zNew[j+1] = 0;
    assert( j==nByte );

    if( pOp->p2==0 ){
      popStack(&pTos, nField);
    }
    pTos++;
    pTos->n = j;
    pTos->flags = MEM_Str|MEM_Dyn|MEM_Term;
    pTos->xDel = 0;
    pTos->enc = encoding;
    pTos->z = zNew;
  }
  break;
}















































case OP_Add:                   
case OP_Subtract:              
case OP_Multiply:              
case OP_Divide:                
case OP_Remainder: {           
  Mem *pNos = &pTos[-1];
  int flags;
  assert( pNos>=p->aStack );
  flags = pTos->flags | pNos->flags;
  if( (flags & MEM_Null)!=0 ){
    Release(pTos);
    pTos--;
    Release(pTos);
    pTos->flags = MEM_Null;
  }else if( (pTos->flags & pNos->flags & MEM_Int)==MEM_Int ){
    i64 a, b;
    a = pTos->i;
    b = pNos->i;
    switch( pOp->opcode ){
      case OP_Add:         b += a;       break;
      case OP_Subtract:    b -= a;       break;
      case OP_Multiply:    b *= a;       break;
      case OP_Divide: {
        if( a==0 ) goto divide_by_zero;
        b /= a;
        break;
      }
      default: {
        if( a==0 ) goto divide_by_zero;
        b %= a;
        break;
      }
    }
    Release(pTos);
    pTos--;
    Release(pTos);
    pTos->i = b;
    pTos->flags = MEM_Int;
  }else{
    double a, b;
    a = sqlite3VdbeRealValue(pTos);
    b = sqlite3VdbeRealValue(pNos);
    switch( pOp->opcode ){
      case OP_Add:         b += a;       break;
      case OP_Subtract:    b -= a;       break;
      case OP_Multiply:    b *= a;       break;
      case OP_Divide: {
        if( a==0.0 ) goto divide_by_zero;
        b /= a;
        break;
      }
      default: {
        int ia = (int)a;
        int ib = (int)b;
        if( ia==0.0 ) goto divide_by_zero;
        b = ib % ia;
        break;
      }
    }
    Release(pTos);
    pTos--;
    Release(pTos);
    pTos->r = b;
    pTos->flags = MEM_Real;
    if( (flags & MEM_Real)==0 ){
      sqlite3VdbeIntegerAffinity(pTos);
    }
  }
  break;

divide_by_zero:
  Release(pTos);
  pTos--;
  Release(pTos);
  pTos->flags = MEM_Null;
  break;
}












case OP_CollSeq: {             
  assert( pOp->p3type==P3_COLLSEQ );
  break;
}
















case OP_Function: {
  int i;
  Mem *pArg;
  sqlite3_context ctx;
  sqlite3_value **apVal;
  int n = pOp->p2;

  apVal = p->apArg;
  assert( apVal || n==0 );

  pArg = &pTos[1-n];
  for(i=0; i<n; i++, pArg++){
    apVal[i] = pArg;
    storeTypeInfo(pArg, encoding);
  }

  assert( pOp->p3type==P3_FUNCDEF || pOp->p3type==P3_VDBEFUNC );
  if( pOp->p3type==P3_FUNCDEF ){
    ctx.pFunc = (FuncDef*)pOp->p3;
    ctx.pVdbeFunc = 0;
  }else{
    ctx.pVdbeFunc = (VdbeFunc*)pOp->p3;
    ctx.pFunc = ctx.pVdbeFunc->pFunc;
  }

  ctx.s.flags = MEM_Null;
  ctx.s.z = 0;
  ctx.s.xDel = 0;
  ctx.isError = 0;
  if( ctx.pFunc->needCollSeq ){
    assert( pOp>p->aOp );
    assert( pOp[-1].p3type==P3_COLLSEQ );
    assert( pOp[-1].opcode==OP_CollSeq );
    ctx.pColl = (CollSeq *)pOp[-1].p3;
  }
  if( sqlite3SafetyOff(db) ) goto abort_due_to_misuse;
  (*ctx.pFunc->xFunc)(&ctx, n, apVal);
  if( sqlite3SafetyOn(db) ) goto abort_due_to_misuse;
  if( sqlite3MallocFailed() ) goto no_mem;
  popStack(&pTos, n);

  


  if( ctx.pVdbeFunc ){
    sqlite3VdbeDeleteAuxData(ctx.pVdbeFunc, pOp->p1);
    pOp->p3 = (char *)ctx.pVdbeFunc;
    pOp->p3type = P3_VDBEFUNC;
  }

  
  if( ctx.isError ){
    sqlite3SetString(&p->zErrMsg, sqlite3_value_text(&ctx.s), (char*)0);
    rc = SQLITE_ERROR;
  }

  
  sqlite3VdbeChangeEncoding(&ctx.s, encoding);
  pTos++;
  pTos->flags = 0;
  sqlite3VdbeMemMove(pTos, &ctx.s);
  break;
}





























case OP_BitAnd:                 
case OP_BitOr:                  
case OP_ShiftLeft:              
case OP_ShiftRight: {           
  Mem *pNos = &pTos[-1];
  i64 a, b;

  assert( pNos>=p->aStack );
  if( (pTos->flags | pNos->flags) & MEM_Null ){
    popStack(&pTos, 2);
    pTos++;
    pTos->flags = MEM_Null;
    break;
  }
  a = sqlite3VdbeIntValue(pNos);
  b = sqlite3VdbeIntValue(pTos);
  switch( pOp->opcode ){
    case OP_BitAnd:      a &= b;     break;
    case OP_BitOr:       a |= b;     break;
    case OP_ShiftLeft:   a <<= b;    break;
    case OP_ShiftRight:  a >>= b;    break;
    default:        break;
  }
  Release(pTos);
  pTos--;
  Release(pTos);
  pTos->i = a;
  pTos->flags = MEM_Int;
  break;
}








case OP_AddImm: {            
  assert( pTos>=p->aStack );
  sqlite3VdbeMemIntegerify(pTos);
  pTos->i += pOp->p1;
  break;
}











case OP_ForceInt: {            
  i64 v;
  assert( pTos>=p->aStack );
  applyAffinity(pTos, SQLITE_AFF_NUMERIC, encoding);
  if( (pTos->flags & (MEM_Int|MEM_Real))==0 ){
    Release(pTos);
    pTos--;
    pc = pOp->p2 - 1;
    break;
  }
  if( pTos->flags & MEM_Int ){
    v = pTos->i + (pOp->p1!=0);
  }else{
    
    sqlite3VdbeMemRealify(pTos);
    v = (int)pTos->r;
    if( pTos->r>(double)v ) v++;
    if( pOp->p1 && pTos->r==(double)v ) v++;
  }
  Release(pTos);
  pTos->i = v;
  pTos->flags = MEM_Int;
  break;
}












case OP_MustBeInt: {            
  assert( pTos>=p->aStack );
  applyAffinity(pTos, SQLITE_AFF_NUMERIC, encoding);
  if( (pTos->flags & MEM_Int)==0 ){
    if( pOp->p2==0 ){
      rc = SQLITE_MISMATCH;
      goto abort_due_to_error;
    }else{
      if( pOp->p1 ) popStack(&pTos, 1);
      pc = pOp->p2 - 1;
    }
  }else{
    Release(pTos);
    pTos->flags = MEM_Int;
  }
  break;
}










case OP_RealAffinity: {                  
  assert( pTos>=p->aStack );
  if( pTos->flags & MEM_Int ){
    sqlite3VdbeMemRealify(pTos);
  }
  break;
}

#ifndef SQLITE_OMIT_CAST









case OP_ToText: {                  
  assert( pTos>=p->aStack );
  if( pTos->flags & MEM_Null ) break;
  assert( MEM_Str==(MEM_Blob>>3) );
  pTos->flags |= (pTos->flags&MEM_Blob)>>3;
  applyAffinity(pTos, SQLITE_AFF_TEXT, encoding);
  assert( pTos->flags & MEM_Str );
  pTos->flags &= ~(MEM_Int|MEM_Real|MEM_Blob);
  break;
}










case OP_ToBlob: {                  
  assert( pTos>=p->aStack );
  if( pTos->flags & MEM_Null ) break;
  if( (pTos->flags & MEM_Blob)==0 ){
    applyAffinity(pTos, SQLITE_AFF_TEXT, encoding);
    assert( pTos->flags & MEM_Str );
    pTos->flags |= MEM_Blob;
  }
  pTos->flags &= ~(MEM_Int|MEM_Real|MEM_Str);
  break;
}











case OP_ToNumeric: {                  
  assert( pTos>=p->aStack );
  if( (pTos->flags & MEM_Null)==0 ){
    sqlite3VdbeMemNumerify(pTos);
  }
  break;
}
#endif 










case OP_ToInt: {                  
  assert( pTos>=p->aStack );
  if( (pTos->flags & MEM_Null)==0 ){
    sqlite3VdbeMemIntegerify(pTos);
  }
  break;
}

#ifndef SQLITE_OMIT_CAST









case OP_ToReal: {                  
  assert( pTos>=p->aStack );
  if( (pTos->flags & MEM_Null)==0 ){
    sqlite3VdbeMemRealify(pTos);
  }
  break;
}
#endif 


































































case OP_Eq:               
case OP_Ne:               
case OP_Lt:               
case OP_Le:               
case OP_Gt:               
case OP_Ge: {             
  Mem *pNos;
  int flags;
  int res;
  char affinity;

  pNos = &pTos[-1];
  flags = pTos->flags|pNos->flags;

  



  if( flags&MEM_Null ){
    if( (pOp->p1 & 0x200)!=0 ){
      










      sqlite3VdbeMemSetInt64(pTos, (pTos->flags & MEM_Null)==0);
      sqlite3VdbeMemSetInt64(pNos, (pNos->flags & MEM_Null)==0);
    }else{
      



      popStack(&pTos, 2);
      if( pOp->p2 ){
        if( pOp->p1 & 0x100 ){
          pc = pOp->p2-1;
        }
      }else{
        pTos++;
        pTos->flags = MEM_Null;
      }
      break;
    }
  }

  affinity = pOp->p1 & 0xFF;
  if( affinity ){
    applyAffinity(pNos, affinity, encoding);
    applyAffinity(pTos, affinity, encoding);
  }

  assert( pOp->p3type==P3_COLLSEQ || pOp->p3==0 );
  res = sqlite3MemCompare(pNos, pTos, (CollSeq*)pOp->p3);
  switch( pOp->opcode ){
    case OP_Eq:    res = res==0;     break;
    case OP_Ne:    res = res!=0;     break;
    case OP_Lt:    res = res<0;      break;
    case OP_Le:    res = res<=0;     break;
    case OP_Gt:    res = res>0;      break;
    default:       res = res>=0;     break;
  }

  popStack(&pTos, 2);
  if( pOp->p2 ){
    if( res ){
      pc = pOp->p2-1;
    }
  }else{
    pTos++;
    pTos->flags = MEM_Int;
    pTos->i = res;
  }
  break;
}













case OP_And:              
case OP_Or: {             
  Mem *pNos = &pTos[-1];
  int v1, v2;    

  assert( pNos>=p->aStack );
  if( pTos->flags & MEM_Null ){
    v1 = 2;
  }else{
    sqlite3VdbeMemIntegerify(pTos);
    v1 = pTos->i==0;
  }
  if( pNos->flags & MEM_Null ){
    v2 = 2;
  }else{
    sqlite3VdbeMemIntegerify(pNos);
    v2 = pNos->i==0;
  }
  if( pOp->opcode==OP_And ){
    static const unsigned char and_logic[] = { 0, 1, 2, 1, 1, 1, 2, 1, 2 };
    v1 = and_logic[v1*3+v2];
  }else{
    static const unsigned char or_logic[] = { 0, 0, 0, 0, 1, 2, 0, 2, 2 };
    v1 = or_logic[v1*3+v2];
  }
  popStack(&pTos, 2);
  pTos++;
  if( v1==2 ){
    pTos->flags = MEM_Null;
  }else{
    pTos->i = v1==0;
    pTos->flags = MEM_Int;
  }
  break;
}













case OP_Negative:              
case OP_AbsValue: {
  assert( pTos>=p->aStack );
  if( pTos->flags & MEM_Real ){
    neg_abs_real_case:
    Release(pTos);
    if( pOp->opcode==OP_Negative || pTos->r<0.0 ){
      pTos->r = -pTos->r;
    }
    pTos->flags = MEM_Real;
  }else if( pTos->flags & MEM_Int ){
    Release(pTos);
    if( pOp->opcode==OP_Negative || pTos->i<0 ){
      pTos->i = -pTos->i;
    }
    pTos->flags = MEM_Int;
  }else if( pTos->flags & MEM_Null ){
    
  }else{
    sqlite3VdbeMemNumerify(pTos);
    goto neg_abs_real_case;
  }
  break;
}







case OP_Not: {                
  assert( pTos>=p->aStack );
  if( pTos->flags & MEM_Null ) break;  
  sqlite3VdbeMemIntegerify(pTos);
  assert( (pTos->flags & MEM_Dyn)==0 );
  pTos->i = !pTos->i;
  pTos->flags = MEM_Int;
  break;
}







case OP_BitNot: {             
  assert( pTos>=p->aStack );
  if( pTos->flags & MEM_Null ) break;  
  sqlite3VdbeMemIntegerify(pTos);
  assert( (pTos->flags & MEM_Dyn)==0 );
  pTos->i = ~pTos->i;
  pTos->flags = MEM_Int;
  break;
}












case OP_Explain:
case OP_Noop: {            
  break;
}





















case OP_If:                 
case OP_IfNot: {            
  int c;
  assert( pTos>=p->aStack );
  if( pTos->flags & MEM_Null ){
    c = pOp->p1;
  }else{
#ifdef SQLITE_OMIT_FLOATING_POINT
    c = sqlite3VdbeIntValue(pTos);
#else
    c = sqlite3VdbeRealValue(pTos)!=0.0;
#endif
    if( pOp->opcode==OP_IfNot ) c = !c;
  }
  Release(pTos);
  pTos--;
  if( c ) pc = pOp->p2-1;
  break;
}







case OP_IsNull: {            
  int i, cnt;
  Mem *pTerm;
  cnt = pOp->p1;
  if( cnt<0 ) cnt = -cnt;
  pTerm = &pTos[1-cnt];
  assert( pTerm>=p->aStack );
  for(i=0; i<cnt; i++, pTerm++){
    if( pTerm->flags & MEM_Null ){
      pc = pOp->p2-1;
      break;
    }
  }
  if( pOp->p1>0 ) popStack(&pTos, cnt);
  break;
}







case OP_NotNull: {            
  int i, cnt;
  cnt = pOp->p1;
  if( cnt<0 ) cnt = -cnt;
  assert( &pTos[1-cnt] >= p->aStack );
  for(i=0; i<cnt && (pTos[1+i-cnt].flags & MEM_Null)==0; i++){}
  if( i>=cnt ) pc = pOp->p2-1;
  if( pOp->p1>0 ) popStack(&pTos, cnt);
  break;
}











case OP_SetNumColumns: {       
  Cursor *pC;
  assert( (pOp->p1)<p->nCursor );
  assert( p->apCsr[pOp->p1]!=0 );
  pC = p->apCsr[pOp->p1];
  pC->nField = pOp->p2;
  break;
}



















case OP_Column: {
  u32 payloadSize;   
  int p1 = pOp->p1;  
  int p2 = pOp->p2;  
  Cursor *pC = 0;    
  char *zRec;        
  BtCursor *pCrsr;   
  u32 *aType;        
  u32 *aOffset;      
  u32 nField;        
  int len;           
  int i;             
  char *zData;       
  Mem sMem;          

  sMem.flags = 0;
  assert( p1<p->nCursor );
  pTos++;
  pTos->flags = MEM_Null;

  













  pC = p->apCsr[p1];
  assert( pC!=0 );
  if( pC->pCursor!=0 ){
    
    rc = sqlite3VdbeCursorMoveto(pC);
    if( rc ) goto abort_due_to_error;
    zRec = 0;
    pCrsr = pC->pCursor;
    if( pC->nullRow ){
      payloadSize = 0;
    }else if( pC->cacheStatus==p->cacheCtr ){
      payloadSize = pC->payloadSize;
      zRec = (char*)pC->aRow;
    }else if( pC->isIndex ){
      i64 payloadSize64;
      sqlite3BtreeKeySize(pCrsr, &payloadSize64);
      payloadSize = payloadSize64;
    }else{
      sqlite3BtreeDataSize(pCrsr, &payloadSize);
    }
    nField = pC->nField;
  }else if( pC->pseudoTable ){
    
    payloadSize = pC->nData;
    zRec = pC->pData;
    pC->cacheStatus = CACHE_STALE;
    assert( payloadSize==0 || zRec!=0 );
    nField = pC->nField;
    pCrsr = 0;
  }else{
    zRec = 0;
    payloadSize = 0;
    pCrsr = 0;
    nField = 0;
  }

  
  if( payloadSize==0 ){
    assert( pTos->flags==MEM_Null );
    break;
  }

  assert( p2<nField );

  


  if( pC && pC->cacheStatus==p->cacheCtr ){
    aType = pC->aType;
    aOffset = pC->aOffset;
  }else{
    u8 *zIdx;        
    u8 *zEndHdr;     
    u32 offset;      
    int szHdrSz;     
    int avail;       

    aType = pC->aType;
    if( aType==0 ){
      pC->aType = aType = sqliteMallocRaw( 2*nField*sizeof(aType) );
    }
    if( aType==0 ){
      goto no_mem;
    }
    pC->aOffset = aOffset = &aType[nField];
    pC->payloadSize = payloadSize;
    pC->cacheStatus = p->cacheCtr;

    
    if( zRec ){
      zData = zRec;
    }else{
      if( pC->isIndex ){
        zData = (char*)sqlite3BtreeKeyFetch(pCrsr, &avail);
      }else{
        zData = (char*)sqlite3BtreeDataFetch(pCrsr, &avail);
      }
      




      if( avail>=payloadSize ){
        zRec = zData;
        pC->aRow = (u8*)zData;
      }else{
        pC->aRow = 0;
      }
    }
    assert( zRec!=0 || avail>=payloadSize || avail>=9 );
    szHdrSz = GetVarint((u8*)zData, offset);

    





    if( !zRec && avail<offset ){
      rc = sqlite3VdbeMemFromBtree(pCrsr, 0, offset, pC->isIndex, &sMem);
      if( rc!=SQLITE_OK ){
        goto op_column_out;
      }
      zData = sMem.z;
    }
    zEndHdr = (u8 *)&zData[offset];
    zIdx = (u8 *)&zData[szHdrSz];

    




    for(i=0; i<nField; i++){
      if( zIdx<zEndHdr ){
        aOffset[i] = offset;
        zIdx += GetVarint(zIdx, aType[i]);
        offset += sqlite3VdbeSerialTypeLen(aType[i]);
      }else{
        





        aOffset[i] = 0;
      }
    }
    Release(&sMem);
    sMem.flags = MEM_Null;

    



    if( zIdx>zEndHdr || offset>payloadSize ){
      rc = SQLITE_CORRUPT_BKPT;
      goto op_column_out;
    }
  }

  





  if( aOffset[p2] ){
    assert( rc==SQLITE_OK );
    if( zRec ){
      zData = &zRec[aOffset[p2]];
    }else{
      len = sqlite3VdbeSerialTypeLen(aType[p2]);
      rc = sqlite3VdbeMemFromBtree(pCrsr, aOffset[p2], len, pC->isIndex,&sMem);
      if( rc!=SQLITE_OK ){
        goto op_column_out;
      }
      zData = sMem.z;
    }
    sqlite3VdbeSerialGet((u8*)zData, aType[p2], pTos);
    pTos->enc = encoding;
  }else{
    if( pOp->p3type==P3_MEM ){
      sqlite3VdbeMemShallowCopy(pTos, (Mem *)(pOp->p3), MEM_Static);
    }else{
      pTos->flags = MEM_Null;
    }
  }

  




  if( (sMem.flags & MEM_Dyn)!=0 ){
    assert( pTos->flags & MEM_Ephem );
    assert( pTos->flags & (MEM_Str|MEM_Blob) );
    assert( pTos->z==sMem.z );
    assert( sMem.flags & MEM_Term );
    pTos->flags &= ~MEM_Ephem;
    pTos->flags |= MEM_Dyn|MEM_Term;
  }

  

  rc = sqlite3VdbeMemMakeWriteable(pTos);

op_column_out:
  break;
}





































case OP_MakeIdxRec:
case OP_MakeRecord: {
  














  unsigned char *zNewRecord;
  unsigned char *zCsr;
  Mem *pRec;
  Mem *pRowid = 0;
  int nData = 0;         
  int nHdr = 0;          
  int nByte = 0;         
  int nVarint;           
  u32 serial_type;       
  int containsNull = 0;  
  char zTemp[NBFS];      
  Mem *pData0;

  int leaveOnStack;      
  int nField;            
  int jumpIfNull;        
  int addRowid;          
  char *zAffinity;       
  int file_format;       

  leaveOnStack = ((pOp->p1<0)?1:0);
  nField = pOp->p1 * (leaveOnStack?-1:1);
  jumpIfNull = pOp->p2;
  addRowid = pOp->opcode==OP_MakeIdxRec;
  zAffinity = pOp->p3;

  pData0 = &pTos[1-nField];
  assert( pData0>=p->aStack );
  containsNull = 0;
  file_format = p->minWriteFileFormat;

  


  for(pRec=pData0; pRec<=pTos; pRec++){
    if( zAffinity ){
      applyAffinity(pRec, zAffinity[pRec-pData0], encoding);
    }
    if( pRec->flags&MEM_Null ){
      containsNull = 1;
    }
    serial_type = sqlite3VdbeSerialType(pRec, file_format);
    nData += sqlite3VdbeSerialTypeLen(serial_type);
    nHdr += sqlite3VarintLen(serial_type);
  }

  



  if( addRowid ){
    pRowid = &pTos[0-nField];
    assert( pRowid>=p->aStack );
    sqlite3VdbeMemIntegerify(pRowid);
    serial_type = sqlite3VdbeSerialType(pRowid, 0);
    nData += sqlite3VdbeSerialTypeLen(serial_type);
    nHdr += sqlite3VarintLen(serial_type);
  }

  
  nHdr += nVarint = sqlite3VarintLen(nHdr);
  if( nVarint<sqlite3VarintLen(nHdr) ){
    nHdr++;
  }
  nByte = nHdr+nData;

  
  if( nByte>sizeof(zTemp) ){
    zNewRecord = sqliteMallocRaw(nByte);
    if( !zNewRecord ){
      goto no_mem;
    }
  }else{
    zNewRecord = (u8*)zTemp;
  }

  
  zCsr = zNewRecord;
  zCsr += sqlite3PutVarint(zCsr, nHdr);
  for(pRec=pData0; pRec<=pTos; pRec++){
    serial_type = sqlite3VdbeSerialType(pRec, file_format);
    zCsr += sqlite3PutVarint(zCsr, serial_type);      
  }
  if( addRowid ){
    zCsr += sqlite3PutVarint(zCsr, sqlite3VdbeSerialType(pRowid, 0));
  }
  for(pRec=pData0; pRec<=pTos; pRec++){
    zCsr += sqlite3VdbeSerialPut(zCsr, pRec, file_format);  
  }
  if( addRowid ){
    zCsr += sqlite3VdbeSerialPut(zCsr, pRowid, 0);
  }
  assert( zCsr==(zNewRecord+nByte) );

  
  if( !leaveOnStack ){
    popStack(&pTos, nField+addRowid);
  }
  pTos++;
  pTos->n = nByte;
  if( nByte<=sizeof(zTemp) ){
    assert( zNewRecord==(unsigned char *)zTemp );
    pTos->z = pTos->zShort;
    memcpy(pTos->zShort, zTemp, nByte);
    pTos->flags = MEM_Blob | MEM_Short;
  }else{
    assert( zNewRecord!=(unsigned char *)zTemp );
    pTos->z = (char*)zNewRecord;
    pTos->flags = MEM_Blob | MEM_Dyn;
    pTos->xDel = 0;
  }
  pTos->enc = SQLITE_UTF8;  

  
  if( jumpIfNull && containsNull ){
    pc = jumpIfNull - 1;
  }
  break;
}













case OP_Statement: {       
  int i = pOp->p1;
  Btree *pBt;
  if( i>=0 && i<db->nDb && (pBt = db->aDb[i].pBt)!=0 && !(db->autoCommit) ){
    assert( sqlite3BtreeIsInTrans(pBt) );
    if( !sqlite3BtreeIsInStmt(pBt) ){
      rc = sqlite3BtreeBeginStmt(pBt);
    }
  }
  break;
}









case OP_AutoCommit: {       
  u8 i = pOp->p1;
  u8 rollback = pOp->p2;

  assert( i==1 || i==0 );
  assert( i==1 || rollback==0 );

  assert( db->activeVdbeCnt>0 );  

  if( db->activeVdbeCnt>1 && i && !db->autoCommit ){
    



    sqlite3SetString(&p->zErrMsg, "cannot ", rollback?"rollback":"commit", 
        " transaction - SQL statements in progress", (char*)0);
    rc = SQLITE_ERROR;
  }else if( i!=db->autoCommit ){
    if( pOp->p2 ){
      assert( i==1 );
      sqlite3RollbackAll(db);
      db->autoCommit = 1;
    }else{
      db->autoCommit = i;
      if( sqlite3VdbeHalt(p)==SQLITE_BUSY ){
        p->pTos = pTos;
        p->pc = pc;
        db->autoCommit = 1-i;
        p->rc = SQLITE_BUSY;
        return SQLITE_BUSY;
      }
    }
    return SQLITE_DONE;
  }else{
    sqlite3SetString(&p->zErrMsg,
        (!i)?"cannot start a transaction within a transaction":(
        (rollback)?"cannot rollback - no transaction is active":
                   "cannot commit - no transaction is active"), (char*)0);
         
    rc = SQLITE_ERROR;
  }
  break;
}





















case OP_Transaction: {       
  int i = pOp->p1;
  Btree *pBt;

  assert( i>=0 && i<db->nDb );
  pBt = db->aDb[i].pBt;

  if( pBt ){
    rc = sqlite3BtreeBeginTrans(pBt, pOp->p2);
    if( rc==SQLITE_BUSY ){
      p->pc = pc;
      p->rc = SQLITE_BUSY;
      p->pTos = pTos;
      return SQLITE_BUSY;
    }
    if( rc!=SQLITE_OK && rc!=SQLITE_READONLY  ){
      goto abort_due_to_error;
    }
  }
  break;
}













case OP_ReadCookie: {
  int iMeta;
  assert( pOp->p2<SQLITE_N_BTREE_META );
  assert( pOp->p1>=0 && pOp->p1<db->nDb );
  assert( db->aDb[pOp->p1].pBt!=0 );
  






  rc = sqlite3BtreeGetMeta(db->aDb[pOp->p1].pBt, 1 + pOp->p2, (u32 *)&iMeta);
  pTos++;
  pTos->i = iMeta;
  pTos->flags = MEM_Int;
  break;
}











case OP_SetCookie: {       
  Db *pDb;
  assert( pOp->p2<SQLITE_N_BTREE_META );
  assert( pOp->p1>=0 && pOp->p1<db->nDb );
  pDb = &db->aDb[pOp->p1];
  assert( pDb->pBt!=0 );
  assert( pTos>=p->aStack );
  sqlite3VdbeMemIntegerify(pTos);
  
  rc = sqlite3BtreeUpdateMeta(pDb->pBt, 1+pOp->p2, (int)pTos->i);
  if( pOp->p2==0 ){
    
    pDb->pSchema->schema_cookie = pTos->i;
    db->flags |= SQLITE_InternChanges;
  }else if( pOp->p2==1 ){
    
    pDb->pSchema->file_format = pTos->i;
  }
  assert( (pTos->flags & MEM_Dyn)==0 );
  pTos--;
  if( pOp->p1==1 ){
    

    sqlite3ExpirePreparedStatements(db);
  }
  break;
}

















case OP_VerifyCookie: {       
  int iMeta;
  Btree *pBt;
  assert( pOp->p1>=0 && pOp->p1<db->nDb );
  pBt = db->aDb[pOp->p1].pBt;
  if( pBt ){
    rc = sqlite3BtreeGetMeta(pBt, 1, (u32 *)&iMeta);
  }else{
    rc = SQLITE_OK;
    iMeta = 0;
  }
  if( rc==SQLITE_OK && iMeta!=pOp->p2 ){
    sqlite3SetString(&p->zErrMsg, "database schema has changed", (char*)0);
    rc = SQLITE_SCHEMA;
  }
  break;
}











































case OP_OpenRead:          
case OP_OpenWrite: {       
  int i = pOp->p1;
  int p2 = pOp->p2;
  int wrFlag;
  Btree *pX;
  int iDb;
  Cursor *pCur;
  Db *pDb;
  
  assert( pTos>=p->aStack );
  sqlite3VdbeMemIntegerify(pTos);
  iDb = pTos->i;
  assert( (pTos->flags & MEM_Dyn)==0 );
  pTos--;
  assert( iDb>=0 && iDb<db->nDb );
  pDb = &db->aDb[iDb];
  pX = pDb->pBt;
  assert( pX!=0 );
  if( pOp->opcode==OP_OpenWrite ){
    wrFlag = 1;
    if( pDb->pSchema->file_format < p->minWriteFileFormat ){
      p->minWriteFileFormat = pDb->pSchema->file_format;
    }
  }else{
    wrFlag = 0;
  }
  if( p2<=0 ){
    assert( pTos>=p->aStack );
    sqlite3VdbeMemIntegerify(pTos);
    p2 = pTos->i;
    assert( (pTos->flags & MEM_Dyn)==0 );
    pTos--;
    assert( p2>=2 );
  }
  assert( i>=0 );
  pCur = allocateCursor(p, i, iDb);
  if( pCur==0 ) goto no_mem;
  pCur->nullRow = 1;
  if( pX==0 ) break;
  

  rc = sqlite3BtreeCursor(pX, p2, wrFlag,
           sqlite3VdbeRecordCompare, pOp->p3,
           &pCur->pCursor);
  if( pOp->p3type==P3_KEYINFO ){
    pCur->pKeyInfo = (KeyInfo*)pOp->p3;
    pCur->pIncrKey = &pCur->pKeyInfo->incrKey;
    pCur->pKeyInfo->enc = ENC(p->db);
  }else{
    pCur->pKeyInfo = 0;
    pCur->pIncrKey = &pCur->bogusIncrKey;
  }
  switch( rc ){
    case SQLITE_BUSY: {
      p->pc = pc;
      p->rc = SQLITE_BUSY;
      p->pTos = &pTos[1 + (pOp->p2<=0)]; 
      return SQLITE_BUSY;
    }
    case SQLITE_OK: {
      int flags = sqlite3BtreeFlags(pCur->pCursor);
      





      if( (flags & 0xf0)!=0 || ((flags & 0x07)!=5 && (flags & 0x07)!=2) ){
        rc = SQLITE_CORRUPT_BKPT;
        goto abort_due_to_error;
      }
      pCur->isTable = (flags & BTREE_INTKEY)!=0;
      pCur->isIndex = (flags & BTREE_ZERODATA)!=0;
      



      if( (pCur->isTable && pOp->p3type==P3_KEYINFO)
       || (pCur->isIndex && pOp->p3type!=P3_KEYINFO) ){
        rc = SQLITE_CORRUPT_BKPT;
        goto abort_due_to_error;
      }
      break;
    }
    case SQLITE_EMPTY: {
      pCur->isTable = pOp->p3type!=P3_KEYINFO;
      pCur->isIndex = !pCur->isTable;
      rc = SQLITE_OK;
      break;
    }
    default: {
      goto abort_due_to_error;
    }
  }
  break;
}













case OP_OpenVirtual: {       
  int i = pOp->p1;
  Cursor *pCx;
  assert( i>=0 );
  pCx = allocateCursor(p, i, -1);
  if( pCx==0 ) goto no_mem;
  pCx->nullRow = 1;
  rc = sqlite3BtreeFactory(db, 0, 1, TEMP_PAGES, &pCx->pBt);
  if( rc==SQLITE_OK ){
    rc = sqlite3BtreeBeginTrans(pCx->pBt, 1);
  }
  if( rc==SQLITE_OK ){
    




    if( pOp->p3 ){
      int pgno;
      assert( pOp->p3type==P3_KEYINFO );
      rc = sqlite3BtreeCreateTable(pCx->pBt, &pgno, BTREE_ZERODATA); 
      if( rc==SQLITE_OK ){
        assert( pgno==MASTER_ROOT+1 );
        rc = sqlite3BtreeCursor(pCx->pBt, pgno, 1, sqlite3VdbeRecordCompare,
            pOp->p3, &pCx->pCursor);
        pCx->pKeyInfo = (KeyInfo*)pOp->p3;
        pCx->pKeyInfo->enc = ENC(p->db);
        pCx->pIncrKey = &pCx->pKeyInfo->incrKey;
      }
      pCx->isTable = 0;
    }else{
      rc = sqlite3BtreeCursor(pCx->pBt, MASTER_ROOT, 1, 0, 0, &pCx->pCursor);
      pCx->isTable = 1;
      pCx->pIncrKey = &pCx->bogusIncrKey;
    }
  }
  pCx->nField = pOp->p2;
  pCx->isIndex = !pCx->isTable;
  break;
}













case OP_OpenPseudo: {       
  int i = pOp->p1;
  Cursor *pCx;
  assert( i>=0 );
  pCx = allocateCursor(p, i, -1);
  if( pCx==0 ) goto no_mem;
  pCx->nullRow = 1;
  pCx->pseudoTable = 1;
  pCx->pIncrKey = &pCx->bogusIncrKey;
  pCx->isTable = 1;
  pCx->isIndex = 0;
  break;
}






case OP_Close: {       
  int i = pOp->p1;
  if( i>=0 && i<p->nCursor ){
    sqlite3VdbeFreeCursor(p->apCsr[i]);
    p->apCsr[i] = 0;
  }
  break;
}









































case OP_MoveLt:         
case OP_MoveLe:         
case OP_MoveGe:         
case OP_MoveGt: {       
  int i = pOp->p1;
  Cursor *pC;

  assert( pTos>=p->aStack );
  assert( i>=0 && i<p->nCursor );
  pC = p->apCsr[i];
  assert( pC!=0 );
  if( pC->pCursor!=0 ){
    int res, oc;
    oc = pOp->opcode;
    pC->nullRow = 0;
    *pC->pIncrKey = oc==OP_MoveGt || oc==OP_MoveLe;
    if( pC->isTable ){
      i64 iKey;
      sqlite3VdbeMemIntegerify(pTos);
      iKey = intToKey(pTos->i);
      if( pOp->p2==0 && pOp->opcode==OP_MoveGe ){
        pC->movetoTarget = iKey;
        pC->deferredMoveto = 1;
        assert( (pTos->flags & MEM_Dyn)==0 );
        pTos--;
        break;
      }
      rc = sqlite3BtreeMoveto(pC->pCursor, 0, (u64)iKey, &res);
      if( rc!=SQLITE_OK ){
        goto abort_due_to_error;
      }
      pC->lastRowid = pTos->i;
      pC->rowidIsValid = res==0;
    }else{
      assert( pTos->flags & MEM_Blob );
      
      rc = sqlite3BtreeMoveto(pC->pCursor, pTos->z, pTos->n, &res);
      if( rc!=SQLITE_OK ){
        goto abort_due_to_error;
      }
      pC->rowidIsValid = 0;
    }
    pC->deferredMoveto = 0;
    pC->cacheStatus = CACHE_STALE;
    *pC->pIncrKey = 0;
    sqlite3_search_count++;
    if( oc==OP_MoveGe || oc==OP_MoveGt ){
      if( res<0 ){
        rc = sqlite3BtreeNext(pC->pCursor, &res);
        if( rc!=SQLITE_OK ) goto abort_due_to_error;
        pC->rowidIsValid = 0;
      }else{
        res = 0;
      }
    }else{
      assert( oc==OP_MoveLt || oc==OP_MoveLe );
      if( res>=0 ){
        rc = sqlite3BtreePrevious(pC->pCursor, &res);
        if( rc!=SQLITE_OK ) goto abort_due_to_error;
        pC->rowidIsValid = 0;
      }else{
        


        res = sqlite3BtreeEof(pC->pCursor);
      }
    }
    if( res ){
      if( pOp->p2>0 ){
        pc = pOp->p2 - 1;
      }else{
        pC->nullRow = 1;
      }
    }
  }
  Release(pTos);
  pTos--;
  break;
}

















































case OP_Distinct:       
case OP_NotFound:       
case OP_Found: {        
  int i = pOp->p1;
  int alreadyExists = 0;
  Cursor *pC;
  assert( pTos>=p->aStack );
  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  if( (pC = p->apCsr[i])->pCursor!=0 ){
    int res, rx;
    assert( pC->isTable==0 );
    Stringify(pTos, encoding);
    rx = sqlite3BtreeMoveto(pC->pCursor, pTos->z, pTos->n, &res);
    alreadyExists = rx==SQLITE_OK && res==0;
    pC->deferredMoveto = 0;
    pC->cacheStatus = CACHE_STALE;
  }
  if( pOp->opcode==OP_Found ){
    if( alreadyExists ) pc = pOp->p2 - 1;
  }else{
    if( !alreadyExists ) pc = pOp->p2 - 1;
  }
  if( pOp->opcode!=OP_Distinct ){
    Release(pTos);
    pTos--;
  }
  break;
}






















case OP_IsUnique: {        
  int i = pOp->p1;
  Mem *pNos = &pTos[-1];
  Cursor *pCx;
  BtCursor *pCrsr;
  i64 R;

  

  assert( pNos>=p->aStack );
  sqlite3VdbeMemIntegerify(pTos);
  R = pTos->i;
  assert( (pTos->flags & MEM_Dyn)==0 );
  pTos--;
  assert( i>=0 && i<=p->nCursor );
  pCx = p->apCsr[i];
  assert( pCx!=0 );
  pCrsr = pCx->pCursor;
  if( pCrsr!=0 ){
    int res;
    i64 v;         
    char *zKey;    
    int nKey;      
    int len;       
    int szRowid;   

    

    Stringify(pNos, encoding);
    zKey = pNos->z;
    nKey = pNos->n;

    szRowid = sqlite3VdbeIdxRowidLen((u8*)zKey);
    len = nKey-szRowid;

    


    assert( pCx->deferredMoveto==0 );
    pCx->cacheStatus = CACHE_STALE;
    rc = sqlite3BtreeMoveto(pCrsr, zKey, len, &res);
    if( rc!=SQLITE_OK ){
      goto abort_due_to_error;
    }
    if( res<0 ){
      rc = sqlite3BtreeNext(pCrsr, &res);
      if( res ){
        pc = pOp->p2 - 1;
        break;
      }
    }
    rc = sqlite3VdbeIdxKeyCompare(pCx, len, (u8*)zKey, &res); 
    if( rc!=SQLITE_OK ) goto abort_due_to_error;
    if( res>0 ){
      pc = pOp->p2 - 1;
      break;
    }

    




    rc = sqlite3VdbeIdxRowid(pCrsr, &v);
    if( rc!=SQLITE_OK ){
      goto abort_due_to_error;
    }
    if( v==R ){
      pc = pOp->p2 - 1;
      break;
    }

    



    pTos++;
    pTos->i = v;
    pTos->flags = MEM_Int;
  }
  break;
}















case OP_NotExists: {        
  int i = pOp->p1;
  Cursor *pC;
  BtCursor *pCrsr;
  assert( pTos>=p->aStack );
  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  if( (pCrsr = (pC = p->apCsr[i])->pCursor)!=0 ){
    int res;
    u64 iKey;
    assert( pTos->flags & MEM_Int );
    assert( p->apCsr[i]->isTable );
    iKey = intToKey(pTos->i);
    rc = sqlite3BtreeMoveto(pCrsr, 0, iKey, &res);
    pC->lastRowid = pTos->i;
    pC->rowidIsValid = res==0;
    pC->nullRow = 0;
    pC->cacheStatus = CACHE_STALE;
    if( res!=0 ){
      pc = pOp->p2 - 1;
      pC->rowidIsValid = 0;
    }
  }
  Release(pTos);
  pTos--;
  break;
}







case OP_Sequence: {
  int i = pOp->p1;
  assert( pTos>=p->aStack );
  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  pTos++;
  pTos->i = p->apCsr[i]->seqCount++;
  pTos->flags = MEM_Int;
  break;
}
















case OP_NewRowid: {
  int i = pOp->p1;
  i64 v = 0;
  Cursor *pC;
  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  if( (pC = p->apCsr[i])->pCursor==0 ){
    
  }else{
    






























    int res, rx=SQLITE_OK, cnt;
    i64 x;
    cnt = 0;
    if( (sqlite3BtreeFlags(pC->pCursor)&(BTREE_INTKEY|BTREE_ZERODATA)) !=
          BTREE_INTKEY ){
      rc = SQLITE_CORRUPT_BKPT;
      goto abort_due_to_error;
    }
    assert( (sqlite3BtreeFlags(pC->pCursor) & BTREE_INTKEY)!=0 );
    assert( (sqlite3BtreeFlags(pC->pCursor) & BTREE_ZERODATA)==0 );

#ifdef SQLITE_32BIT_ROWID
#   define MAX_ROWID 0x7fffffff
#else
    



#   define MAX_ROWID  ( (((u64)0x7fffffff)<<32) | (u64)0xffffffff )
#endif

    if( !pC->useRandomRowid ){
      if( pC->nextRowidValid ){
        v = pC->nextRowid;
      }else{
        rc = sqlite3BtreeLast(pC->pCursor, &res);
        if( rc!=SQLITE_OK ){
          goto abort_due_to_error;
        }
        if( res ){
          v = 1;
        }else{
          sqlite3BtreeKeySize(pC->pCursor, &v);
          v = keyToInt(v);
          if( v==MAX_ROWID ){
            pC->useRandomRowid = 1;
          }else{
            v++;
          }
        }
      }

#ifndef SQLITE_OMIT_AUTOINCREMENT
      if( pOp->p2 ){
        Mem *pMem;
        assert( pOp->p2>0 && pOp->p2<p->nMem );  
        pMem = &p->aMem[pOp->p2];
        sqlite3VdbeMemIntegerify(pMem);
        assert( (pMem->flags & MEM_Int)!=0 );  
        if( pMem->i==MAX_ROWID || pC->useRandomRowid ){
          rc = SQLITE_FULL;
          goto abort_due_to_error;
        }
        if( v<pMem->i+1 ){
          v = pMem->i + 1;
        }
        pMem->i = v;
      }
#endif

      if( v<MAX_ROWID ){
        pC->nextRowidValid = 1;
        pC->nextRowid = v+1;
      }else{
        pC->nextRowidValid = 0;
      }
    }
    if( pC->useRandomRowid ){
      assert( pOp->p2==0 );  
      v = db->priorNewRowid;
      cnt = 0;
      do{
        if( v==0 || cnt>2 ){
          sqlite3Randomness(sizeof(v), &v);
          if( cnt<5 ) v &= 0xffffff;
        }else{
          unsigned char r;
          sqlite3Randomness(1, &r);
          v += r + 1;
        }
        if( v==0 ) continue;
        x = intToKey(v);
        rx = sqlite3BtreeMoveto(pC->pCursor, 0, (u64)x, &res);
        cnt++;
      }while( cnt<1000 && rx==SQLITE_OK && res==0 );
      db->priorNewRowid = v;
      if( rx==SQLITE_OK && res==0 ){
        rc = SQLITE_FULL;
        goto abort_due_to_error;
      }
    }
    pC->rowidIsValid = 0;
    pC->deferredMoveto = 0;
    pC->cacheStatus = CACHE_STALE;
  }
  pTos++;
  pTos->i = v;
  pTos->flags = MEM_Int;
  break;
}

















case OP_Insert: {         
  Mem *pNos = &pTos[-1];
  int i = pOp->p1;
  Cursor *pC;
  assert( pNos>=p->aStack );
  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  if( ((pC = p->apCsr[i])->pCursor!=0 || pC->pseudoTable) ){
    i64 iKey;   

    assert( pNos->flags & MEM_Int );
    assert( pC->isTable );
    iKey = intToKey(pNos->i);

    if( pOp->p2 & OPFLAG_NCHANGE ) p->nChange++;
    if( pOp->p2 & OPFLAG_LASTROWID ) db->lastRowid = pNos->i;
    if( pC->nextRowidValid && pNos->i>=pC->nextRowid ){
      pC->nextRowidValid = 0;
    }
    if( pTos->flags & MEM_Null ){
      pTos->z = 0;
      pTos->n = 0;
    }else{
      assert( pTos->flags & (MEM_Blob|MEM_Str) );
    }
    if( pC->pseudoTable ){
      sqliteFree(pC->pData);
      pC->iKey = iKey;
      pC->nData = pTos->n;
      if( pTos->flags & MEM_Dyn ){
        pC->pData = pTos->z;
        pTos->flags = MEM_Null;
      }else{
        pC->pData = sqliteMallocRaw( pC->nData+2 );
        if( !pC->pData ) goto no_mem;
        memcpy(pC->pData, pTos->z, pC->nData);
        pC->pData[pC->nData] = 0;
        pC->pData[pC->nData+1] = 0;
      }
      pC->nullRow = 0;
    }else{
      rc = sqlite3BtreeInsert(pC->pCursor, 0, iKey, pTos->z, pTos->n);
    }
    
    pC->rowidIsValid = 0;
    pC->deferredMoveto = 0;
    pC->cacheStatus = CACHE_STALE;

    
    if( rc==SQLITE_OK && db->xUpdateCallback && pOp->p3 ){
      const char *zDb = db->aDb[pC->iDb].zName;
      const char *zTbl = pOp->p3;
      int op = ((pOp->p2 & OPFLAG_ISUPDATE) ? SQLITE_UPDATE : SQLITE_INSERT);
      assert( pC->isTable );
      db->xUpdateCallback(db->pUpdateArg, op, zDb, zTbl, iKey);
      assert( pC->iDb>=0 );
    }
  }
  popStack(&pTos, 2);

  break;
}















case OP_Delete: {        
  int i = pOp->p1;
  Cursor *pC;
  assert( i>=0 && i<p->nCursor );
  pC = p->apCsr[i];
  assert( pC!=0 );
  if( pC->pCursor!=0 ){
    i64 iKey;

    


    if( db->xUpdateCallback && pOp->p3 ){
      assert( pC->isTable );
      if( pC->rowidIsValid ){
        iKey = pC->lastRowid;
      }else{
        rc = sqlite3BtreeKeySize(pC->pCursor, &iKey);
        if( rc ){
          goto abort_due_to_error;
        }
        iKey = keyToInt(iKey);
      }
    }

    rc = sqlite3VdbeCursorMoveto(pC);
    if( rc ) goto abort_due_to_error;
    rc = sqlite3BtreeDelete(pC->pCursor);
    pC->nextRowidValid = 0;
    pC->cacheStatus = CACHE_STALE;

    
    if( rc==SQLITE_OK && db->xUpdateCallback && pOp->p3 ){
      const char *zDb = db->aDb[pC->iDb].zName;
      const char *zTbl = pOp->p3;
      db->xUpdateCallback(db->pUpdateArg, SQLITE_DELETE, zDb, zTbl, iKey);
      assert( pC->iDb>=0 );
    }
  }
  if( pOp->p2 & OPFLAG_NCHANGE ) p->nChange++;
  break;
}








case OP_ResetCount: {        
  if( pOp->p1 ){
    sqlite3VdbeSetChanges(db, p->nChange);
  }
  p->nChange = 0;
  break;
}



















case OP_RowKey:
case OP_RowData: {
  int i = pOp->p1;
  Cursor *pC;
  u32 n;

  
  pTos++;
  assert( i>=0 && i<p->nCursor );
  pC = p->apCsr[i];
  assert( pC->isTable || pOp->opcode==OP_RowKey );
  assert( pC->isIndex || pOp->opcode==OP_RowData );
  assert( pC!=0 );
  if( pC->nullRow ){
    pTos->flags = MEM_Null;
  }else if( pC->pCursor!=0 ){
    BtCursor *pCrsr = pC->pCursor;
    rc = sqlite3VdbeCursorMoveto(pC);
    if( rc ) goto abort_due_to_error;
    if( pC->nullRow ){
      pTos->flags = MEM_Null;
      break;
    }else if( pC->isIndex ){
      i64 n64;
      assert( !pC->isTable );
      sqlite3BtreeKeySize(pCrsr, &n64);
      n = n64;
    }else{
      sqlite3BtreeDataSize(pCrsr, &n);
    }
    pTos->n = n;
    if( n<=NBFS ){
      pTos->flags = MEM_Blob | MEM_Short;
      pTos->z = pTos->zShort;
    }else{
      char *z = sqliteMallocRaw( n );
      if( z==0 ) goto no_mem;
      pTos->flags = MEM_Blob | MEM_Dyn;
      pTos->xDel = 0;
      pTos->z = z;
    }
    if( pC->isIndex ){
      sqlite3BtreeKey(pCrsr, 0, n, pTos->z);
    }else{
      sqlite3BtreeData(pCrsr, 0, n, pTos->z);
    }
  }else if( pC->pseudoTable ){
    pTos->n = pC->nData;
    pTos->z = pC->pData;
    pTos->flags = MEM_Blob|MEM_Ephem;
  }else{
    pTos->flags = MEM_Null;
  }
  pTos->enc = SQLITE_UTF8;  
  break;
}






case OP_Rowid: {
  int i = pOp->p1;
  Cursor *pC;
  i64 v;

  assert( i>=0 && i<p->nCursor );
  pC = p->apCsr[i];
  assert( pC!=0 );
  rc = sqlite3VdbeCursorMoveto(pC);
  if( rc ) goto abort_due_to_error;
  pTos++;
  if( pC->rowidIsValid ){
    v = pC->lastRowid;
  }else if( pC->pseudoTable ){
    v = keyToInt(pC->iKey);
  }else if( pC->nullRow || pC->pCursor==0 ){
    pTos->flags = MEM_Null;
    break;
  }else{
    assert( pC->pCursor!=0 );
    sqlite3BtreeKeySize(pC->pCursor, &v);
    v = keyToInt(v);
  }
  pTos->i = v;
  pTos->flags = MEM_Int;
  break;
}







case OP_NullRow: {        
  int i = pOp->p1;
  Cursor *pC;

  assert( i>=0 && i<p->nCursor );
  pC = p->apCsr[i];
  assert( pC!=0 );
  pC->nullRow = 1;
  pC->rowidIsValid = 0;
  break;
}









case OP_Last: {        
  int i = pOp->p1;
  Cursor *pC;
  BtCursor *pCrsr;

  assert( i>=0 && i<p->nCursor );
  pC = p->apCsr[i];
  assert( pC!=0 );
  if( (pCrsr = pC->pCursor)!=0 ){
    int res;
    rc = sqlite3BtreeLast(pCrsr, &res);
    pC->nullRow = res;
    pC->deferredMoveto = 0;
    pC->cacheStatus = CACHE_STALE;
    if( res && pOp->p2>0 ){
      pc = pOp->p2 - 1;
    }
  }else{
    pC->nullRow = 0;
  }
  break;
}














case OP_Sort: {        
  sqlite3_sort_count++;
  sqlite3_search_count--;
  
}








case OP_Rewind: {        
  int i = pOp->p1;
  Cursor *pC;
  BtCursor *pCrsr;
  int res;

  assert( i>=0 && i<p->nCursor );
  pC = p->apCsr[i];
  assert( pC!=0 );
  if( (pCrsr = pC->pCursor)!=0 ){
    rc = sqlite3BtreeFirst(pCrsr, &res);
    pC->atFirst = res==0;
    pC->deferredMoveto = 0;
    pC->cacheStatus = CACHE_STALE;
  }else{
    res = 1;
  }
  pC->nullRow = res;
  if( res && pOp->p2>0 ){
    pc = pOp->p2 - 1;
  }
  break;
}

















case OP_Prev:          
case OP_Next: {        
  Cursor *pC;
  BtCursor *pCrsr;

  CHECK_FOR_INTERRUPT;
  assert( pOp->p1>=0 && pOp->p1<p->nCursor );
  pC = p->apCsr[pOp->p1];
  assert( pC!=0 );
  if( (pCrsr = pC->pCursor)!=0 ){
    int res;
    if( pC->nullRow ){
      res = 1;
    }else{
      assert( pC->deferredMoveto==0 );
      rc = pOp->opcode==OP_Next ? sqlite3BtreeNext(pCrsr, &res) :
                                  sqlite3BtreePrevious(pCrsr, &res);
      pC->nullRow = res;
      pC->cacheStatus = CACHE_STALE;
    }
    if( res==0 ){
      pc = pOp->p2 - 1;
      sqlite3_search_count++;
    }
  }else{
    pC->nullRow = 1;
  }
  pC->rowidIsValid = 0;
  break;
}










case OP_IdxInsert: {        
  int i = pOp->p1;
  Cursor *pC;
  BtCursor *pCrsr;
  assert( pTos>=p->aStack );
  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  assert( pTos->flags & MEM_Blob );
  assert( pOp->p2==0 );
  if( (pCrsr = (pC = p->apCsr[i])->pCursor)!=0 ){
    int nKey = pTos->n;
    const char *zKey = pTos->z;
    assert( pC->isTable==0 );
    rc = sqlite3BtreeInsert(pCrsr, zKey, nKey, "", 0);
    assert( pC->deferredMoveto==0 );
    pC->cacheStatus = CACHE_STALE;
  }
  Release(pTos);
  pTos--;
  break;
}







case OP_IdxDelete: {        
  int i = pOp->p1;
  Cursor *pC;
  BtCursor *pCrsr;
  assert( pTos>=p->aStack );
  assert( pTos->flags & MEM_Blob );
  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  if( (pCrsr = (pC = p->apCsr[i])->pCursor)!=0 ){
    int res;
    rc = sqlite3BtreeMoveto(pCrsr, pTos->z, pTos->n, &res);
    if( rc==SQLITE_OK && res==0 ){
      rc = sqlite3BtreeDelete(pCrsr);
    }
    assert( pC->deferredMoveto==0 );
    pC->cacheStatus = CACHE_STALE;
  }
  Release(pTos);
  pTos--;
  break;
}









case OP_IdxRowid: {
  int i = pOp->p1;
  BtCursor *pCrsr;
  Cursor *pC;

  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  pTos++;
  pTos->flags = MEM_Null;
  if( (pCrsr = (pC = p->apCsr[i])->pCursor)!=0 ){
    i64 rowid;

    assert( pC->deferredMoveto==0 );
    assert( pC->isTable==0 );
    if( pC->nullRow ){
      pTos->flags = MEM_Null;
    }else{
      rc = sqlite3VdbeIdxRowid(pCrsr, &rowid);
      if( rc!=SQLITE_OK ){
        goto abort_due_to_error;
      }
      pTos->flags = MEM_Int;
      pTos->i = rowid;
    }
  }
  break;
}













































case OP_IdxLT:          
case OP_IdxGT:          
case OP_IdxGE: {        
  int i= pOp->p1;
  Cursor *pC;

  assert( i>=0 && i<p->nCursor );
  assert( p->apCsr[i]!=0 );
  assert( pTos>=p->aStack );
  if( (pC = p->apCsr[i])->pCursor!=0 ){
    int res;
 
    assert( pTos->flags & MEM_Blob );  
    Stringify(pTos, encoding);
    assert( pC->deferredMoveto==0 );
    *pC->pIncrKey = pOp->p3!=0;
    assert( pOp->p3==0 || pOp->opcode!=OP_IdxGT );
    rc = sqlite3VdbeIdxKeyCompare(pC, pTos->n, (u8*)pTos->z, &res);
    *pC->pIncrKey = 0;
    if( rc!=SQLITE_OK ){
      break;
    }
    if( pOp->opcode==OP_IdxLT ){
      res = -res;
    }else if( pOp->opcode==OP_IdxGE ){
      res++;
    }
    if( res>0 ){
      pc = pOp->p2 - 1 ;
    }
  }
  Release(pTos);
  pTos--;
  break;
}










case OP_IdxIsNull: {        
  int i = pOp->p1;
  int k, n;
  const char *z;
  u32 serial_type;

  assert( pTos>=p->aStack );
  assert( pTos->flags & MEM_Blob );
  z = pTos->z;
  n = pTos->n;
  k = sqlite3GetVarint32((u8*)z, &serial_type);
  for(; k<n && i>0; i--){
    k += sqlite3GetVarint32((u8*)&z[k], &serial_type);
    if( serial_type==0 ){   
      pc = pOp->p2-1;
      break;
    }
  }
  Release(pTos);
  pTos--;
  break;
}





















case OP_Destroy: {
  int iMoved;
  if( db->activeVdbeCnt>1 ){
    rc = SQLITE_LOCKED;
  }else{
    assert( db->activeVdbeCnt==1 );
    rc = sqlite3BtreeDropTable(db->aDb[pOp->p2].pBt, pOp->p1, &iMoved);
    pTos++;
    pTos->flags = MEM_Int;
    pTos->i = iMoved;
  #ifndef SQLITE_OMIT_AUTOVACUUM
    if( rc==SQLITE_OK && iMoved!=0 ){
      sqlite3RootPageMoved(&db->aDb[pOp->p2], iMoved, pOp->p1);
    }
  #endif
  }
  break;
}













case OP_Clear: {        

  


#if 0
  Btree *pBt = db->aDb[pOp->p2].pBt;
  if( db->xUpdateCallback && pOp->p3 ){
    const char *zDb = db->aDb[pOp->p2].zName;
    const char *zTbl = pOp->p3;
    BtCursor *pCur = 0;
    int fin = 0;

    rc = sqlite3BtreeCursor(pBt, pOp->p1, 0, 0, 0, &pCur);
    if( rc!=SQLITE_OK ){
      goto abort_due_to_error;
    }
    for(
      rc=sqlite3BtreeFirst(pCur, &fin); 
      rc==SQLITE_OK && !fin; 
      rc=sqlite3BtreeNext(pCur, &fin)
    ){
      i64 iKey;
      rc = sqlite3BtreeKeySize(pCur, &iKey);
      if( rc ){
        break;
      }
      iKey = keyToInt(iKey);
      db->xUpdateCallback(db->pUpdateArg, SQLITE_DELETE, zDb, zTbl, iKey);
    }
    sqlite3BtreeCloseCursor(pCur);
    if( rc!=SQLITE_OK ){
      goto abort_due_to_error;
    }
  }
#endif
  rc = sqlite3BtreeClearTable(db->aDb[pOp->p2].pBt, pOp->p1);
  break;
}





















case OP_CreateIndex:
case OP_CreateTable: {
  int pgno;
  int flags;
  Db *pDb;
  assert( pOp->p1>=0 && pOp->p1<db->nDb );
  pDb = &db->aDb[pOp->p1];
  assert( pDb->pBt!=0 );
  if( pOp->opcode==OP_CreateTable ){
    
    flags = BTREE_LEAFDATA|BTREE_INTKEY;
  }else{
    flags = BTREE_ZERODATA;
  }
  rc = sqlite3BtreeCreateTable(pDb->pBt, &pgno, flags);
  pTos++;
  if( rc==SQLITE_OK ){
    pTos->i = pgno;
    pTos->flags = MEM_Int;
  }else{
    pTos->flags = MEM_Null;
  }
  break;
}









case OP_ParseSchema: {        
  char *zSql;
  int iDb = pOp->p1;
  const char *zMaster;
  InitData initData;

  assert( iDb>=0 && iDb<db->nDb );
  if( !DbHasProperty(db, iDb, DB_SchemaLoaded) ) break;
  zMaster = SCHEMA_TABLE(iDb);
  initData.db = db;
  initData.pzErrMsg = &p->zErrMsg;
  zSql = sqlite3MPrintf(
     "SELECT name, rootpage, sql, %d FROM '%q'.%s WHERE %s",
     pOp->p1, db->aDb[iDb].zName, zMaster, pOp->p3);
  if( zSql==0 ) goto no_mem;
  sqlite3SafetyOff(db);
  assert( db->init.busy==0 );
  db->init.busy = 1;
  assert( !sqlite3MallocFailed() );
  rc = sqlite3_exec(db, zSql, sqlite3InitCallback, &initData, 0);
  sqliteFree(zSql);
  db->init.busy = 0;
  sqlite3SafetyOn(db);
  if( rc==SQLITE_NOMEM ){
    sqlite3FailedMalloc();
    goto no_mem;
  }
  break;  
}

#if !defined(SQLITE_OMIT_ANALYZE) && !defined(SQLITE_OMIT_PARSER)






case OP_LoadAnalysis: {        
  int iDb = pOp->p1;
  assert( iDb>=0 && iDb<db->nDb );
  sqlite3AnalysisLoad(db, iDb);
  break;  
}
#endif 








case OP_DropTable: {        
  sqlite3UnlinkAndDeleteTable(db, pOp->p1, pOp->p3);
  break;
}








case OP_DropIndex: {        
  sqlite3UnlinkAndDeleteIndex(db, pOp->p1, pOp->p3);
  break;
}








case OP_DropTrigger: {        
  sqlite3UnlinkAndDeleteTrigger(db, pOp->p1, pOp->p3);
  break;
}


#ifndef SQLITE_OMIT_INTEGRITY_CHECK















case OP_IntegrityCk: {
  int nRoot;
  int *aRoot;
  int j;
  char *z;

  for(nRoot=0; &pTos[-nRoot]>=p->aStack; nRoot++){
    if( (pTos[-nRoot].flags & MEM_Int)==0 ) break;
  }
  assert( nRoot>0 );
  aRoot = sqliteMallocRaw( sizeof(int*)*(nRoot+1) );
  if( aRoot==0 ) goto no_mem;
  for(j=0; j<nRoot; j++){
    Mem *pMem = &pTos[-j];
    aRoot[j] = pMem->i;
  }
  aRoot[j] = 0;
  popStack(&pTos, nRoot);
  pTos++;
  z = sqlite3BtreeIntegrityCheck(db->aDb[pOp->p2].pBt, aRoot, nRoot);
  if( z==0 || z[0]==0 ){
    if( z ) sqliteFree(z);
    pTos->z = "ok";
    pTos->n = 2;
    pTos->flags = MEM_Str | MEM_Static | MEM_Term;
  }else{
    pTos->z = z;
    pTos->n = strlen(z);
    pTos->flags = MEM_Str | MEM_Dyn | MEM_Term;
    pTos->xDel = 0;
  }
  pTos->enc = SQLITE_UTF8;
  sqlite3VdbeChangeEncoding(pTos, encoding);
  sqliteFree(aRoot);
  break;
}
#endif 






case OP_FifoWrite: {        
  assert( pTos>=p->aStack );
  sqlite3VdbeMemIntegerify(pTos);
  sqlite3VdbeFifoPush(&p->sFifo, pTos->i);
  assert( (pTos->flags & MEM_Dyn)==0 );
  pTos--;
  break;
}







case OP_FifoRead: {
  i64 v;
  CHECK_FOR_INTERRUPT;
  if( sqlite3VdbeFifoPop(&p->sFifo, &v)==SQLITE_DONE ){
    pc = pOp->p2 - 1;
  }else{
    pTos++;
    pTos->i = v;
    pTos->flags = MEM_Int;
  }
  break;
}

#ifndef SQLITE_OMIT_TRIGGER






case OP_ContextPush: {        
  int i = p->contextStackTop++;
  Context *pContext;

  assert( i>=0 );
  
  if( i>=p->contextStackDepth ){
    p->contextStackDepth = i+1;
    sqliteReallocOrFree((void**)&p->contextStack, sizeof(Context)*(i+1));
    if( p->contextStack==0 ) goto no_mem;
  }
  pContext = &p->contextStack[i];
  pContext->lastRowid = db->lastRowid;
  pContext->nChange = p->nChange;
  pContext->sFifo = p->sFifo;
  sqlite3VdbeFifoInit(&p->sFifo);
  break;
}







case OP_ContextPop: {        
  Context *pContext = &p->contextStack[--p->contextStackTop];
  assert( p->contextStackTop>=0 );
  db->lastRowid = pContext->lastRowid;
  p->nChange = pContext->nChange;
  sqlite3VdbeFifoClear(&p->sFifo);
  p->sFifo = pContext->sFifo;
  break;
}
#endif 











case OP_MemStore: {        
  assert( pTos>=p->aStack );
  assert( pOp->p1>=0 && pOp->p1<p->nMem );
  rc = sqlite3VdbeMemMove(&p->aMem[pOp->p1], pTos);
  pTos--;

  


  if( pOp->p2 ){
    break;
  }
}









case OP_MemLoad: {
  int i = pOp->p1;
  assert( i>=0 && i<p->nMem );
  pTos++;
  sqlite3VdbeMemShallowCopy(pTos, &p->aMem[i], MEM_Ephem);
  break;
}

#ifndef SQLITE_OMIT_AUTOINCREMENT








case OP_MemMax: {        
  int i = pOp->p1;
  Mem *pMem;
  assert( pTos>=p->aStack );
  assert( i>=0 && i<p->nMem );
  pMem = &p->aMem[i];
  sqlite3VdbeMemIntegerify(pMem);
  sqlite3VdbeMemIntegerify(pTos);
  if( pMem->i<pTos->i){
    pMem->i = pTos->i;
  }
  break;
}
#endif 








case OP_MemIncr: {        
  int i = pOp->p2;
  Mem *pMem;
  assert( i>=0 && i<p->nMem );
  pMem = &p->aMem[i];
  assert( pMem->flags==MEM_Int );
  pMem->i += pOp->p1;
  break;
}








case OP_IfMemPos: {        
  int i = pOp->p1;
  Mem *pMem;
  assert( i>=0 && i<p->nMem );
  pMem = &p->aMem[i];
  assert( pMem->flags==MEM_Int );
  if( pMem->i>0 ){
     pc = pOp->p2 - 1;
  }
  break;
}








case OP_IfMemNeg: {        
  int i = pOp->p1;
  Mem *pMem;
  assert( i>=0 && i<p->nMem );
  pMem = &p->aMem[i];
  assert( pMem->flags==MEM_Int );
  if( pMem->i<0 ){
     pc = pOp->p2 - 1;
  }
  break;
}








case OP_IfMemZero: {        
  int i = pOp->p1;
  Mem *pMem;
  assert( i>=0 && i<p->nMem );
  pMem = &p->aMem[i];
  assert( pMem->flags==MEM_Int );
  if( pMem->i==0 ){
     pc = pOp->p2 - 1;
  }
  break;
}





case OP_MemNull: {
  assert( pOp->p1>=0 && pOp->p1<p->nMem );
  sqlite3VdbeMemSetNull(&p->aMem[pOp->p1]);
  break;
}





case OP_MemInt: {
  assert( pOp->p2>=0 && pOp->p2<p->nMem );
  sqlite3VdbeMemSetInt64(&p->aMem[pOp->p2], pOp->p1);
  break;
}







case OP_MemMove: {
  assert( pOp->p1>=0 && pOp->p1<p->nMem );
  assert( pOp->p2>=0 && pOp->p2<p->nMem );
  rc = sqlite3VdbeMemMove(&p->aMem[pOp->p1], &p->aMem[pOp->p2]);
  break;
}










case OP_AggStep: {        
  int n = pOp->p2;
  int i;
  Mem *pMem, *pRec;
  sqlite3_context ctx;
  sqlite3_value **apVal;

  assert( n>=0 );
  pRec = &pTos[1-n];
  assert( pRec>=p->aStack );
  apVal = p->apArg;
  assert( apVal || n==0 );
  for(i=0; i<n; i++, pRec++){
    apVal[i] = pRec;
    storeTypeInfo(pRec, encoding);
  }
  ctx.pFunc = (FuncDef*)pOp->p3;
  assert( pOp->p1>=0 && pOp->p1<p->nMem );
  ctx.pMem = pMem = &p->aMem[pOp->p1];
  pMem->n++;
  ctx.s.flags = MEM_Null;
  ctx.s.z = 0;
  ctx.s.xDel = 0;
  ctx.isError = 0;
  ctx.pColl = 0;
  if( ctx.pFunc->needCollSeq ){
    assert( pOp>p->aOp );
    assert( pOp[-1].p3type==P3_COLLSEQ );
    assert( pOp[-1].opcode==OP_CollSeq );
    ctx.pColl = (CollSeq *)pOp[-1].p3;
  }
  (ctx.pFunc->xStep)(&ctx, n, apVal);
  popStack(&pTos, n);
  if( ctx.isError ){
    sqlite3SetString(&p->zErrMsg, sqlite3_value_text(&ctx.s), (char*)0);
    rc = SQLITE_ERROR;
  }
  sqlite3VdbeMemRelease(&ctx.s);
  break;
}













case OP_AggFinal: {        
  Mem *pMem;
  assert( pOp->p1>=0 && pOp->p1<p->nMem );
  pMem = &p->aMem[pOp->p1];
  assert( (pMem->flags & ~(MEM_Null|MEM_Agg))==0 );
  rc = sqlite3VdbeMemFinalize(pMem, (FuncDef*)pOp->p3);
  if( rc==SQLITE_ERROR ){
    sqlite3SetString(&p->zErrMsg, sqlite3_value_text(pMem), (char*)0);
  }
  break;
}








case OP_Vacuum: {        
  if( sqlite3SafetyOff(db) ) goto abort_due_to_misuse; 
  rc = sqlite3RunVacuum(&p->zErrMsg, db);
  if( sqlite3SafetyOn(db) ) goto abort_due_to_misuse;
  break;
}










case OP_Expire: {        
  if( !pOp->p1 ){
    sqlite3ExpirePreparedStatements(db);
  }else{
    p->expired = 1;
  }
  break;
}

#ifndef SQLITE_OMIT_SHARED_CACHE
















case OP_TableLock: {        
  int p1 = pOp->p1; 
  u8 isWriteLock = (p1<0);
  if( isWriteLock ){
    p1 = (-1*p1)-1;
  }
  rc = sqlite3BtreeLockTable(db->aDb[p1].pBt, pOp->p2, isWriteLock);
  if( rc==SQLITE_LOCKED ){
    const char *z = (const char *)pOp->p3;
    sqlite3SetString(&p->zErrMsg, "database table is locked: ", z, (char*)0);
  }
  break;
}
#endif 



default: {
  assert( 0 );
  break;
}







    }

    
    assert( pTos<=pStackLimit );

#ifdef VDBE_PROFILE
    {
      long long elapse = hwtime() - start;
      pOp->cycles += elapse;
      pOp->cnt++;
#if 0
        fprintf(stdout, "%10lld ", elapse);
        sqlite3VdbePrintOp(stdout, origPc, &p->aOp[origPc]);
#endif
    }
#endif

    




#ifndef NDEBUG
    
    if( pTos>=p->aStack ){
      sqlite3VdbeMemSanity(pTos);
    }
    assert( pc>=-1 && pc<p->nOp );
#ifdef SQLITE_DEBUG
    
    if( p->trace && pTos>=p->aStack ){
      int i;
      fprintf(p->trace, "Stack:");
      for(i=0; i>-5 && &pTos[i]>=p->aStack; i--){
        if( pTos[i].flags & MEM_Null ){
          fprintf(p->trace, " NULL");
        }else if( (pTos[i].flags & (MEM_Int|MEM_Str))==(MEM_Int|MEM_Str) ){
          fprintf(p->trace, " si:%lld", pTos[i].i);
        }else if( pTos[i].flags & MEM_Int ){
          fprintf(p->trace, " i:%lld", pTos[i].i);
        }else if( pTos[i].flags & MEM_Real ){
          fprintf(p->trace, " r:%g", pTos[i].r);
        }else{
          char zBuf[100];
          sqlite3VdbeMemPrettyPrint(&pTos[i], zBuf);
          fprintf(p->trace, " ");
          fprintf(p->trace, "%s", zBuf);
        }
      }
      if( rc!=0 ) fprintf(p->trace," rc=%d",rc);
      fprintf(p->trace,"\n");
    }
#endif  
#endif  
  }  

  

vdbe_halt:
  if( rc ){
    p->rc = rc;
    rc = SQLITE_ERROR;
  }else{
    rc = SQLITE_DONE;
  }
  sqlite3VdbeHalt(p);
  p->pTos = pTos;
  return rc;

  


no_mem:
  sqlite3SetString(&p->zErrMsg, "out of memory", (char*)0);
  rc = SQLITE_NOMEM;
  goto vdbe_halt;

  

abort_due_to_misuse:
  rc = SQLITE_MISUSE;
  

  


abort_due_to_error:
  if( p->zErrMsg==0 ){
    if( sqlite3MallocFailed() ) rc = SQLITE_NOMEM;
    sqlite3SetString(&p->zErrMsg, sqlite3ErrStr(rc), (char*)0);
  }
  goto vdbe_halt;

  


abort_due_to_interrupt:
  assert( db->flags & SQLITE_Interrupt );
  db->flags &= ~SQLITE_Interrupt;
  if( db->magic!=SQLITE_MAGIC_BUSY ){
    rc = SQLITE_MISUSE;
  }else{
    rc = SQLITE_INTERRUPT;
  }
  p->rc = rc;
  sqlite3SetString(&p->zErrMsg, sqlite3ErrStr(rc), (char*)0);
  goto vdbe_halt;
}
