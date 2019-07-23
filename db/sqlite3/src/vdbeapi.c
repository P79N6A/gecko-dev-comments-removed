














#include "sqliteInt.h"
#include "vdbeInt.h"
#include "os.h"









int sqlite3_expired(sqlite3_stmt *pStmt){
  Vdbe *p = (Vdbe*)pStmt;
  return p==0 || p->expired;
}





const void *sqlite3_value_blob(sqlite3_value *pVal){
  Mem *p = (Mem*)pVal;
  if( p->flags & (MEM_Blob|MEM_Str) ){
    return p->z;
  }else{
    return sqlite3_value_text(pVal);
  }
}
int sqlite3_value_bytes(sqlite3_value *pVal){
  return sqlite3ValueBytes(pVal, SQLITE_UTF8);
}
int sqlite3_value_bytes16(sqlite3_value *pVal){
  return sqlite3ValueBytes(pVal, SQLITE_UTF16NATIVE);
}
double sqlite3_value_double(sqlite3_value *pVal){
  return sqlite3VdbeRealValue((Mem*)pVal);
}
int sqlite3_value_int(sqlite3_value *pVal){
  return sqlite3VdbeIntValue((Mem*)pVal);
}
sqlite_int64 sqlite3_value_int64(sqlite3_value *pVal){
  return sqlite3VdbeIntValue((Mem*)pVal);
}
const unsigned char *sqlite3_value_text(sqlite3_value *pVal){
  return (const unsigned char *)sqlite3ValueText(pVal, SQLITE_UTF8);
}
#ifndef SQLITE_OMIT_UTF16
const void *sqlite3_value_text16(sqlite3_value* pVal){
  return sqlite3ValueText(pVal, SQLITE_UTF16NATIVE);
}
const void *sqlite3_value_text16be(sqlite3_value *pVal){
  return sqlite3ValueText(pVal, SQLITE_UTF16BE);
}
const void *sqlite3_value_text16le(sqlite3_value *pVal){
  return sqlite3ValueText(pVal, SQLITE_UTF16LE);
}
#endif 
int sqlite3_value_type(sqlite3_value* pVal){
  return pVal->type;
}






void sqlite3_result_blob(
  sqlite3_context *pCtx, 
  const void *z, 
  int n, 
  void (*xDel)(void *)
){
  assert( n>=0 );
  sqlite3VdbeMemSetStr(&pCtx->s, z, n, 0, xDel);
}
void sqlite3_result_double(sqlite3_context *pCtx, double rVal){
  sqlite3VdbeMemSetDouble(&pCtx->s, rVal);
}
void sqlite3_result_error(sqlite3_context *pCtx, const char *z, int n){
  pCtx->isError = 1;
  sqlite3VdbeMemSetStr(&pCtx->s, z, n, SQLITE_UTF8, SQLITE_TRANSIENT);
}
#ifndef SQLITE_OMIT_UTF16
void sqlite3_result_error16(sqlite3_context *pCtx, const void *z, int n){
  pCtx->isError = 1;
  sqlite3VdbeMemSetStr(&pCtx->s, z, n, SQLITE_UTF16NATIVE, SQLITE_TRANSIENT);
}
#endif
void sqlite3_result_int(sqlite3_context *pCtx, int iVal){
  sqlite3VdbeMemSetInt64(&pCtx->s, (i64)iVal);
}
void sqlite3_result_int64(sqlite3_context *pCtx, i64 iVal){
  sqlite3VdbeMemSetInt64(&pCtx->s, iVal);
}
void sqlite3_result_null(sqlite3_context *pCtx){
  sqlite3VdbeMemSetNull(&pCtx->s);
}
void sqlite3_result_text(
  sqlite3_context *pCtx, 
  const char *z, 
  int n,
  void (*xDel)(void *)
){
  sqlite3VdbeMemSetStr(&pCtx->s, z, n, SQLITE_UTF8, xDel);
}
#ifndef SQLITE_OMIT_UTF16
void sqlite3_result_text16(
  sqlite3_context *pCtx, 
  const void *z, 
  int n, 
  void (*xDel)(void *)
){
  sqlite3VdbeMemSetStr(&pCtx->s, z, n, SQLITE_UTF16NATIVE, xDel);
}
void sqlite3_result_text16be(
  sqlite3_context *pCtx, 
  const void *z, 
  int n, 
  void (*xDel)(void *)
){
  sqlite3VdbeMemSetStr(&pCtx->s, z, n, SQLITE_UTF16BE, xDel);
}
void sqlite3_result_text16le(
  sqlite3_context *pCtx, 
  const void *z, 
  int n, 
  void (*xDel)(void *)
){
  sqlite3VdbeMemSetStr(&pCtx->s, z, n, SQLITE_UTF16LE, xDel);
}
#endif 
void sqlite3_result_value(sqlite3_context *pCtx, sqlite3_value *pValue){
  sqlite3VdbeMemCopy(&pCtx->s, pValue);
}






int sqlite3_step(sqlite3_stmt *pStmt){
  Vdbe *p = (Vdbe*)pStmt;
  sqlite3 *db;
  int rc;

  
  assert( !sqlite3MallocFailed() );

  if( p==0 || p->magic!=VDBE_MAGIC_RUN ){
    return SQLITE_MISUSE;
  }
  if( p->aborted ){
    return SQLITE_ABORT;
  }
  if( p->pc<=0 && p->expired ){
    if( p->rc==SQLITE_OK ){
      p->rc = SQLITE_SCHEMA;
    }
    return SQLITE_ERROR;
  }
  db = p->db;
  if( sqlite3SafetyOn(db) ){
    p->rc = SQLITE_MISUSE;
    return SQLITE_MISUSE;
  }
  if( p->pc<0 ){
#ifndef SQLITE_OMIT_TRACE
    

    if( db->xTrace && !db->init.busy ){
      assert( p->nOp>0 );
      assert( p->aOp[p->nOp-1].opcode==OP_Noop );
      assert( p->aOp[p->nOp-1].p3!=0 );
      assert( p->aOp[p->nOp-1].p3type==P3_DYNAMIC );
      sqlite3SafetyOff(db);
      db->xTrace(db->pTraceArg, p->aOp[p->nOp-1].p3);
      if( sqlite3SafetyOn(db) ){
        p->rc = SQLITE_MISUSE;
        return SQLITE_MISUSE;
      }
    }
    if( db->xProfile && !db->init.busy ){
      double rNow;
      sqlite3OsCurrentTime(&rNow);
      p->startTime = (rNow - (int)rNow)*3600.0*24.0*1000000000.0;
    }
#endif

    


#ifdef SQLITE_DEBUG
    if( (db->flags & SQLITE_SqlTrace)!=0 ){
      sqlite3DebugPrintf("SQL-trace: %s\n", p->aOp[p->nOp-1].p3);
    }
#endif 

    db->activeVdbeCnt++;
    p->pc = 0;
  }
#ifndef SQLITE_OMIT_EXPLAIN
  if( p->explain ){
    rc = sqlite3VdbeList(p);
  }else
#endif 
  {
    rc = sqlite3VdbeExec(p);
  }

  if( sqlite3SafetyOff(db) ){
    rc = SQLITE_MISUSE;
  }

#ifndef SQLITE_OMIT_TRACE
  

  if( rc!=SQLITE_ROW && db->xProfile && !db->init.busy ){
    double rNow;
    u64 elapseTime;

    sqlite3OsCurrentTime(&rNow);
    elapseTime = (rNow - (int)rNow)*3600.0*24.0*1000000000.0 - p->startTime;
    assert( p->nOp>0 );
    assert( p->aOp[p->nOp-1].opcode==OP_Noop );
    assert( p->aOp[p->nOp-1].p3!=0 );
    assert( p->aOp[p->nOp-1].p3type==P3_DYNAMIC );
    db->xProfile(db->pProfileArg, p->aOp[p->nOp-1].p3, elapseTime);
  }
#endif

  sqlite3Error(p->db, rc, 0);
  p->rc = sqlite3ApiExit(p->db, p->rc);
  return rc;
}





void *sqlite3_user_data(sqlite3_context *p){
  assert( p && p->pFunc );
  return p->pFunc->pUserData;
}






void *sqlite3_aggregate_context(sqlite3_context *p, int nByte){
  Mem *pMem = p->pMem;
  assert( p && p->pFunc && p->pFunc->xStep );
  if( (pMem->flags & MEM_Agg)==0 ){
    if( nByte==0 ){
      assert( pMem->flags==MEM_Null );
      pMem->z = 0;
    }else{
      pMem->flags = MEM_Agg;
      pMem->xDel = sqlite3FreeX;
      *(FuncDef**)&pMem->i = p->pFunc;
      if( nByte<=NBFS ){
        pMem->z = pMem->zShort;
        memset(pMem->z, 0, nByte);
      }else{
        pMem->z = sqliteMalloc( nByte );
      }
    }
  }
  return (void*)pMem->z;
}





void *sqlite3_get_auxdata(sqlite3_context *pCtx, int iArg){
  VdbeFunc *pVdbeFunc = pCtx->pVdbeFunc;
  if( !pVdbeFunc || iArg>=pVdbeFunc->nAux || iArg<0 ){
    return 0;
  }
  return pVdbeFunc->apAux[iArg].pAux;
}






void sqlite3_set_auxdata(
  sqlite3_context *pCtx, 
  int iArg, 
  void *pAux, 
  void (*xDelete)(void*)
){
  struct AuxData *pAuxData;
  VdbeFunc *pVdbeFunc;
  if( iArg<0 ) return;

  pVdbeFunc = pCtx->pVdbeFunc;
  if( !pVdbeFunc || pVdbeFunc->nAux<=iArg ){
    int nMalloc = sizeof(VdbeFunc) + sizeof(struct AuxData)*iArg;
    pVdbeFunc = sqliteRealloc(pVdbeFunc, nMalloc);
    if( !pVdbeFunc ) return;
    pCtx->pVdbeFunc = pVdbeFunc;
    memset(&pVdbeFunc->apAux[pVdbeFunc->nAux], 0, 
             sizeof(struct AuxData)*(iArg+1-pVdbeFunc->nAux));
    pVdbeFunc->nAux = iArg+1;
    pVdbeFunc->pFunc = pCtx->pFunc;
  }

  pAuxData = &pVdbeFunc->apAux[iArg];
  if( pAuxData->pAux && pAuxData->xDelete ){
    pAuxData->xDelete(pAuxData->pAux);
  }
  pAuxData->pAux = pAux;
  pAuxData->xDelete = xDelete;
}










int sqlite3_aggregate_count(sqlite3_context *p){
  assert( p && p->pFunc && p->pFunc->xStep );
  return p->pMem->n;
}




int sqlite3_column_count(sqlite3_stmt *pStmt){
  Vdbe *pVm = (Vdbe *)pStmt;
  return pVm ? pVm->nResColumn : 0;
}





int sqlite3_data_count(sqlite3_stmt *pStmt){
  Vdbe *pVm = (Vdbe *)pStmt;
  if( pVm==0 || !pVm->resOnStack ) return 0;
  return pVm->nResColumn;
}








static Mem *columnMem(sqlite3_stmt *pStmt, int i){
  Vdbe *pVm = (Vdbe *)pStmt;
  int vals = sqlite3_data_count(pStmt);
  if( i>=vals || i<0 ){
    static Mem nullMem;
    if( nullMem.flags==0 ){ nullMem.flags = MEM_Null; }
    sqlite3Error(pVm->db, SQLITE_RANGE, 0);
    return &nullMem;
  }
  return &pVm->pTos[(1-vals)+i];
}




















static void columnMallocFailure(sqlite3_stmt *pStmt)
{
  




  Vdbe *p = (Vdbe *)pStmt;
  p->rc = sqlite3ApiExit(0, p->rc);
}





const void *sqlite3_column_blob(sqlite3_stmt *pStmt, int i){
  const void *val;
  sqlite3MallocDisallow();
  val = sqlite3_value_blob( columnMem(pStmt,i) );
  sqlite3MallocAllow();
  return val;
}
int sqlite3_column_bytes(sqlite3_stmt *pStmt, int i){
  int val = sqlite3_value_bytes( columnMem(pStmt,i) );
  columnMallocFailure(pStmt);
  return val;
}
int sqlite3_column_bytes16(sqlite3_stmt *pStmt, int i){
  int val = sqlite3_value_bytes16( columnMem(pStmt,i) );
  columnMallocFailure(pStmt);
  return val;
}
double sqlite3_column_double(sqlite3_stmt *pStmt, int i){
  double val = sqlite3_value_double( columnMem(pStmt,i) );
  columnMallocFailure(pStmt);
  return val;
}
int sqlite3_column_int(sqlite3_stmt *pStmt, int i){
  int val = sqlite3_value_int( columnMem(pStmt,i) );
  columnMallocFailure(pStmt);
  return val;
}
sqlite_int64 sqlite3_column_int64(sqlite3_stmt *pStmt, int i){
  sqlite_int64 val = sqlite3_value_int64( columnMem(pStmt,i) );
  columnMallocFailure(pStmt);
  return val;
}
const unsigned char *sqlite3_column_text(sqlite3_stmt *pStmt, int i){
  const unsigned char *val = sqlite3_value_text( columnMem(pStmt,i) );
  columnMallocFailure(pStmt);
  return val;
}
#if 0
sqlite3_value *sqlite3_column_value(sqlite3_stmt *pStmt, int i){
  return columnMem(pStmt, i);
}
#endif
#ifndef SQLITE_OMIT_UTF16
const void *sqlite3_column_text16(sqlite3_stmt *pStmt, int i){
  const void *val = sqlite3_value_text16( columnMem(pStmt,i) );
  columnMallocFailure(pStmt);
  return val;
}
#endif 
int sqlite3_column_type(sqlite3_stmt *pStmt, int i){
  return sqlite3_value_type( columnMem(pStmt,i) );
}
























static const void *columnName(
  sqlite3_stmt *pStmt,
  int N,
  const void *(*xFunc)(Mem*),
  int useType
){
  const void *ret;
  Vdbe *p = (Vdbe *)pStmt;
  int n = sqlite3_column_count(pStmt);

  if( p==0 || N>=n || N<0 ){
    return 0;
  }
  N += useType*n;
  ret = xFunc(&p->aColName[N]);

  


  sqlite3ApiExit(0, 0);
  return ret;
}





const char *sqlite3_column_name(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text, COLNAME_NAME);
}
#ifndef SQLITE_OMIT_UTF16
const void *sqlite3_column_name16(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text16, COLNAME_NAME);
}
#endif





const char *sqlite3_column_decltype(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text, COLNAME_DECLTYPE);
}
#ifndef SQLITE_OMIT_UTF16
const void *sqlite3_column_decltype16(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text16, COLNAME_DECLTYPE);
}
#endif 

#ifdef SQLITE_ENABLE_COLUMN_METADATA





const char *sqlite3_column_database_name(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text, COLNAME_DATABASE);
}
#ifndef SQLITE_OMIT_UTF16
const void *sqlite3_column_database_name16(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text16, COLNAME_DATABASE);
}
#endif 






const char *sqlite3_column_table_name(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text, COLNAME_TABLE);
}
#ifndef SQLITE_OMIT_UTF16
const void *sqlite3_column_table_name16(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text16, COLNAME_TABLE);
}
#endif 






const char *sqlite3_column_origin_name(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text, COLNAME_COLUMN);
}
#ifndef SQLITE_OMIT_UTF16
const void *sqlite3_column_origin_name16(sqlite3_stmt *pStmt, int N){
  return columnName(
      pStmt, N, (const void*(*)(Mem*))sqlite3_value_text16, COLNAME_COLUMN);
}
#endif 
#endif 














static int vdbeUnbind(Vdbe *p, int i){
  Mem *pVar;
  if( p==0 || p->magic!=VDBE_MAGIC_RUN || p->pc>=0 ){
    if( p ) sqlite3Error(p->db, SQLITE_MISUSE, 0);
    return SQLITE_MISUSE;
  }
  if( i<1 || i>p->nVar ){
    sqlite3Error(p->db, SQLITE_RANGE, 0);
    return SQLITE_RANGE;
  }
  i--;
  pVar = &p->aVar[i];
  sqlite3VdbeMemRelease(pVar);
  pVar->flags = MEM_Null;
  sqlite3Error(p->db, SQLITE_OK, 0);
  return SQLITE_OK;
}




static int bindText(
  sqlite3_stmt *pStmt, 
  int i, 
  const void *zData, 
  int nData, 
  void (*xDel)(void*),
  int encoding
){
  Vdbe *p = (Vdbe *)pStmt;
  Mem *pVar;
  int rc;

  rc = vdbeUnbind(p, i);
  if( rc || zData==0 ){
    return rc;
  }
  pVar = &p->aVar[i-1];
  rc = sqlite3VdbeMemSetStr(pVar, zData, nData, encoding, xDel);
  if( rc==SQLITE_OK && encoding!=0 ){
    rc = sqlite3VdbeChangeEncoding(pVar, ENC(p->db));
  }

  sqlite3Error(((Vdbe *)pStmt)->db, rc, 0);
  return sqlite3ApiExit(((Vdbe *)pStmt)->db, rc);
}





int sqlite3_bind_blob(
  sqlite3_stmt *pStmt, 
  int i, 
  const void *zData, 
  int nData, 
  void (*xDel)(void*)
){
  return bindText(pStmt, i, zData, nData, xDel, 0);
}
int sqlite3_bind_double(sqlite3_stmt *pStmt, int i, double rValue){
  int rc;
  Vdbe *p = (Vdbe *)pStmt;
  rc = vdbeUnbind(p, i);
  if( rc==SQLITE_OK ){
    sqlite3VdbeMemSetDouble(&p->aVar[i-1], rValue);
  }
  return rc;
}
int sqlite3_bind_int(sqlite3_stmt *p, int i, int iValue){
  return sqlite3_bind_int64(p, i, (i64)iValue);
}
int sqlite3_bind_int64(sqlite3_stmt *pStmt, int i, sqlite_int64 iValue){
  int rc;
  Vdbe *p = (Vdbe *)pStmt;
  rc = vdbeUnbind(p, i);
  if( rc==SQLITE_OK ){
    sqlite3VdbeMemSetInt64(&p->aVar[i-1], iValue);
  }
  return rc;
}
int sqlite3_bind_null(sqlite3_stmt* p, int i){
  return vdbeUnbind((Vdbe *)p, i);
}
int sqlite3_bind_text( 
  sqlite3_stmt *pStmt, 
  int i, 
  const char *zData, 
  int nData, 
  void (*xDel)(void*)
){
  return bindText(pStmt, i, zData, nData, xDel, SQLITE_UTF8);
}
#ifndef SQLITE_OMIT_UTF16
int sqlite3_bind_text16(
  sqlite3_stmt *pStmt, 
  int i, 
  const void *zData, 
  int nData, 
  void (*xDel)(void*)
){
  return bindText(pStmt, i, zData, nData, xDel, SQLITE_UTF16NATIVE);
}
#endif 





int sqlite3_bind_parameter_count(sqlite3_stmt *pStmt){
  Vdbe *p = (Vdbe*)pStmt;
  return p ? p->nVar : 0;
}






static void createVarMap(Vdbe *p){
  if( !p->okVar ){
    int j;
    Op *pOp;
    for(j=0, pOp=p->aOp; j<p->nOp; j++, pOp++){
      if( pOp->opcode==OP_Variable ){
        assert( pOp->p1>0 && pOp->p1<=p->nVar );
        p->azVar[pOp->p1-1] = pOp->p3;
      }
    }
    p->okVar = 1;
  }
}







const char *sqlite3_bind_parameter_name(sqlite3_stmt *pStmt, int i){
  Vdbe *p = (Vdbe*)pStmt;
  if( p==0 || i<1 || i>p->nVar ){
    return 0;
  }
  createVarMap(p);
  return p->azVar[i-1];
}






int sqlite3_bind_parameter_index(sqlite3_stmt *pStmt, const char *zName){
  Vdbe *p = (Vdbe*)pStmt;
  int i;
  if( p==0 ){
    return 0;
  }
  createVarMap(p); 
  if( zName ){
    for(i=0; i<p->nVar; i++){
      const char *z = p->azVar[i];
      if( z && strcmp(z,zName)==0 ){
        return i+1;
      }
    }
  }
  return 0;
}








int sqlite3_bind_parameter_indexes(
    sqlite3_stmt *pStmt,
    const char *zName,
    int **pIndexes
){
  Vdbe *p = (Vdbe*)pStmt;
  int i, j, nVars, *indexes;
  if( p==0 ){
    return 0;
  }
  createVarMap(p);
  if( !zName )
    return 0;
  
  nVars = 0;
  for(i=0; i<p->nVar; i++){
    const char *z = p->azVar[i];
    if( z && strcmp(z,zName)==0 ){
      nVars++;
    }
  }
  indexes = sqliteMalloc( sizeof(int) * nVars );
  j = 0;
  for(i=0; i<p->nVar; i++){
    const char *z = p->azVar[i];
    if( z && strcmp(z,zName)==0 )
      indexes[j++] = i+1;
  }
  *pIndexes = indexes;
  return nVars;
}

void sqlite3_free_parameter_indexes(int *pIndexes)
{
  sqliteFree( pIndexes );
}






int sqlite3_transfer_bindings(sqlite3_stmt *pFromStmt, sqlite3_stmt *pToStmt){
  Vdbe *pFrom = (Vdbe*)pFromStmt;
  Vdbe *pTo = (Vdbe*)pToStmt;
  int i, rc = SQLITE_OK;
  if( (pFrom->magic!=VDBE_MAGIC_RUN && pFrom->magic!=VDBE_MAGIC_HALT)
    || (pTo->magic!=VDBE_MAGIC_RUN && pTo->magic!=VDBE_MAGIC_HALT) ){
    return SQLITE_MISUSE;
  }
  if( pFrom->nVar!=pTo->nVar ){
    return SQLITE_ERROR;
  }
  for(i=0; rc==SQLITE_OK && i<pFrom->nVar; i++){
    sqlite3MallocDisallow();
    rc = sqlite3VdbeMemMove(&pTo->aVar[i], &pFrom->aVar[i]);
    sqlite3MallocAllow();
  }
  return rc;
}







sqlite3 *sqlite3_db_handle(sqlite3_stmt *pStmt){
  return pStmt ? ((Vdbe*)pStmt)->db : 0;
}
