
















#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>
#include "vdbeInt.h"














int sqlite3VdbeChangeEncoding(Mem *pMem, int desiredEnc){
  int rc;
  if( !(pMem->flags&MEM_Str) || pMem->enc==desiredEnc ){
    return SQLITE_OK;
  }
#ifdef SQLITE_OMIT_UTF16
  return SQLITE_ERROR;
#else


  


  rc = sqlite3VdbeMemTranslate(pMem, desiredEnc);
  assert(rc==SQLITE_OK    || rc==SQLITE_NOMEM);
  assert(rc==SQLITE_OK    || pMem->enc!=desiredEnc);
  assert(rc==SQLITE_NOMEM || pMem->enc==desiredEnc);

  if( rc==SQLITE_NOMEM ){





  }
  return rc;
#endif
}






int sqlite3VdbeMemDynamicify(Mem *pMem){
  int n = pMem->n;
  u8 *z;
  if( (pMem->flags & (MEM_Ephem|MEM_Static|MEM_Short))==0 ){
    return SQLITE_OK;
  }
  assert( (pMem->flags & MEM_Dyn)==0 );
  assert( pMem->flags & (MEM_Str|MEM_Blob) );
  z = sqliteMallocRaw( n+2 );
  if( z==0 ){
    return SQLITE_NOMEM;
  }
  pMem->flags |= MEM_Dyn|MEM_Term;
  pMem->xDel = 0;
  memcpy(z, pMem->z, n );
  z[n] = 0;
  z[n+1] = 0;
  pMem->z = (char*)z;
  pMem->flags &= ~(MEM_Ephem|MEM_Static|MEM_Short);
  return SQLITE_OK;
}







int sqlite3VdbeMemMakeWriteable(Mem *pMem){
  int n;
  u8 *z;
  if( (pMem->flags & (MEM_Ephem|MEM_Static))==0 ){
    return SQLITE_OK;
  }
  assert( (pMem->flags & MEM_Dyn)==0 );
  assert( pMem->flags & (MEM_Str|MEM_Blob) );
  if( (n = pMem->n)+2<sizeof(pMem->zShort) ){
    z = (u8*)pMem->zShort;
    pMem->flags |= MEM_Short|MEM_Term;
  }else{
    z = sqliteMallocRaw( n+2 );
    if( z==0 ){
      return SQLITE_NOMEM;
    }
    pMem->flags |= MEM_Dyn|MEM_Term;
    pMem->xDel = 0;
  }
  memcpy(z, pMem->z, n );
  z[n] = 0;
  z[n+1] = 0;
  pMem->z = (char*)z;
  pMem->flags &= ~(MEM_Ephem|MEM_Static);
  assert(0==(1&(int)pMem->z));
  return SQLITE_OK;
}




int sqlite3VdbeMemNulTerminate(Mem *pMem){
  





  assert(                         
    !(pMem->flags&MEM_Str) ||                
    (pMem->flags&MEM_Term) ||                
    (pMem->flags&(MEM_Ephem|MEM_Static)) ||  
    (pMem->flags&MEM_Dyn && pMem->xDel)      
  );
  if( (pMem->flags & MEM_Term)!=0 || (pMem->flags & MEM_Str)==0 ){
    return SQLITE_OK;   
  }

  if( pMem->flags & (MEM_Static|MEM_Ephem) ){
    return sqlite3VdbeMemMakeWriteable(pMem);
  }else{
    char *z = sqliteMalloc(pMem->n+2);
    if( !z ) return SQLITE_NOMEM;
    memcpy(z, pMem->z, pMem->n);
    z[pMem->n] = 0;
    z[pMem->n+1] = 0;
    pMem->xDel(pMem->z);
    pMem->xDel = 0;
    pMem->z = z;
  }
  return SQLITE_OK;
}














int sqlite3VdbeMemStringify(Mem *pMem, int enc){
  int rc = SQLITE_OK;
  int fg = pMem->flags;
  char *z = pMem->zShort;

  assert( !(fg&(MEM_Str|MEM_Blob)) );
  assert( fg&(MEM_Int|MEM_Real) );

  





  if( fg & MEM_Int ){
    sqlite3_snprintf(NBFS, z, "%lld", pMem->i);
  }else{
    assert( fg & MEM_Real );
    sqlite3_snprintf(NBFS, z, "%!.15g", pMem->r);
  }
  pMem->n = strlen(z);
  pMem->z = z;
  pMem->enc = SQLITE_UTF8;
  pMem->flags |= MEM_Str | MEM_Short | MEM_Term;
  sqlite3VdbeChangeEncoding(pMem, enc);
  return rc;
}









int sqlite3VdbeMemFinalize(Mem *pMem, FuncDef *pFunc){
  int rc = SQLITE_OK;
  if( pFunc && pFunc->xFinalize ){
    sqlite3_context ctx;
    assert( (pMem->flags & MEM_Null)!=0 || pFunc==*(FuncDef**)&pMem->i );
    ctx.s.flags = MEM_Null;
    ctx.s.z = pMem->zShort;
    ctx.pMem = pMem;
    ctx.pFunc = pFunc;
    ctx.isError = 0;
    pFunc->xFinalize(&ctx);
    if( pMem->z && pMem->z!=pMem->zShort ){
      sqliteFree( pMem->z );
    }
    *pMem = ctx.s;
    if( pMem->flags & MEM_Short ){
      pMem->z = pMem->zShort;
    }
    if( ctx.isError ){
      rc = SQLITE_ERROR;
    }
  }
  return rc;
}






void sqlite3VdbeMemRelease(Mem *p){
  if( p->flags & (MEM_Dyn|MEM_Agg) ){
    if( p->xDel ){
      if( p->flags & MEM_Agg ){
        sqlite3VdbeMemFinalize(p, *(FuncDef**)&p->i);
        assert( (p->flags & MEM_Agg)==0 );
        sqlite3VdbeMemRelease(p);
      }else{
        p->xDel((void *)p->z);
      }
    }else{
      sqliteFree(p->z);
    }
    p->z = 0;
    p->xDel = 0;
  }
}











i64 sqlite3VdbeIntValue(Mem *pMem){
  int flags = pMem->flags;
  if( flags & MEM_Int ){
    return pMem->i;
  }else if( flags & MEM_Real ){
    return (i64)pMem->r;
  }else if( flags & (MEM_Str|MEM_Blob) ){
    i64 value;
    if( sqlite3VdbeChangeEncoding(pMem, SQLITE_UTF8)
       || sqlite3VdbeMemNulTerminate(pMem) ){
      return 0;
    }
    assert( pMem->z );
    sqlite3atoi64(pMem->z, &value);
    return value;
  }else{
    return 0;
  }
}







double sqlite3VdbeRealValue(Mem *pMem){
  if( pMem->flags & MEM_Real ){
    return pMem->r;
  }else if( pMem->flags & MEM_Int ){
    return (double)pMem->i;
  }else if( pMem->flags & (MEM_Str|MEM_Blob) ){
    double val = 0.0;
    if( sqlite3VdbeChangeEncoding(pMem, SQLITE_UTF8)
       || sqlite3VdbeMemNulTerminate(pMem) ){
      return 0.0;
    }
    assert( pMem->z );
    sqlite3AtoF(pMem->z, &val);
    return val;
  }else{
    return 0.0;
  }
}





void sqlite3VdbeIntegerAffinity(Mem *pMem){
  assert( pMem->flags & MEM_Real );
  pMem->i = pMem->r;
  if( ((double)pMem->i)==pMem->r ){
    pMem->flags |= MEM_Int;
  }
}




int sqlite3VdbeMemIntegerify(Mem *pMem){
  pMem->i = sqlite3VdbeIntValue(pMem);
  sqlite3VdbeMemRelease(pMem);
  pMem->flags = MEM_Int;
  return SQLITE_OK;
}





int sqlite3VdbeMemRealify(Mem *pMem){
  pMem->r = sqlite3VdbeRealValue(pMem);
  sqlite3VdbeMemRelease(pMem);
  pMem->flags = MEM_Real;
  return SQLITE_OK;
}





int sqlite3VdbeMemNumerify(Mem *pMem){
  sqlite3VdbeMemRealify(pMem);
  sqlite3VdbeIntegerAffinity(pMem);
  return SQLITE_OK;
}




void sqlite3VdbeMemSetNull(Mem *pMem){
  sqlite3VdbeMemRelease(pMem);
  pMem->flags = MEM_Null;
  pMem->type = SQLITE_NULL;
  pMem->n = 0;
}





void sqlite3VdbeMemSetInt64(Mem *pMem, i64 val){
  sqlite3VdbeMemRelease(pMem);
  pMem->i = val;
  pMem->flags = MEM_Int;
  pMem->type = SQLITE_INTEGER;
}





void sqlite3VdbeMemSetDouble(Mem *pMem, double val){
  sqlite3VdbeMemRelease(pMem);
  pMem->r = val;
  pMem->flags = MEM_Real;
  pMem->type = SQLITE_FLOAT;
}







void sqlite3VdbeMemShallowCopy(Mem *pTo, const Mem *pFrom, int srcType){
  memcpy(pTo, pFrom, sizeof(*pFrom)-sizeof(pFrom->zShort));
  pTo->xDel = 0;
  if( pTo->flags & (MEM_Str|MEM_Blob) ){
    pTo->flags &= ~(MEM_Dyn|MEM_Static|MEM_Short|MEM_Ephem);
    assert( srcType==MEM_Ephem || srcType==MEM_Static );
    pTo->flags |= srcType;
  }
}





int sqlite3VdbeMemCopy(Mem *pTo, const Mem *pFrom){
  int rc;
  if( pTo->flags & MEM_Dyn ){
    sqlite3VdbeMemRelease(pTo);
  }
  sqlite3VdbeMemShallowCopy(pTo, pFrom, MEM_Ephem);
  if( pTo->flags & MEM_Ephem ){
    rc = sqlite3VdbeMemMakeWriteable(pTo);
  }else{
    rc = SQLITE_OK;
  }
  return rc;
}









int sqlite3VdbeMemMove(Mem *pTo, Mem *pFrom){
  int rc;
  if( pTo->flags & MEM_Dyn ){
    sqlite3VdbeMemRelease(pTo);
  }
  memcpy(pTo, pFrom, sizeof(Mem));
  if( pFrom->flags & MEM_Short ){
    pTo->z = pTo->zShort;
  }
  pFrom->flags = MEM_Null;
  pFrom->xDel = 0;
  if( pTo->flags & MEM_Ephem ){
    rc = sqlite3VdbeMemMakeWriteable(pTo);
  }else{
    rc = SQLITE_OK;
  }
  return rc;
}




int sqlite3VdbeMemSetStr(
  Mem *pMem,          
  const char *z,      
  int n,              
  u8 enc,             
  void (*xDel)(void*) 
){
  sqlite3VdbeMemRelease(pMem);
  if( !z ){
    pMem->flags = MEM_Null;
    pMem->type = SQLITE_NULL;
    return SQLITE_OK;
  }

  pMem->z = (char *)z;
  if( xDel==SQLITE_STATIC ){
    pMem->flags = MEM_Static;
  }else if( xDel==SQLITE_TRANSIENT ){
    pMem->flags = MEM_Ephem;
  }else{
    pMem->flags = MEM_Dyn;
    pMem->xDel = xDel;
  }

  pMem->enc = enc;
  pMem->type = enc==0 ? SQLITE_BLOB : SQLITE_TEXT;
  pMem->n = n;

  assert( enc==0 || enc==SQLITE_UTF8 || enc==SQLITE_UTF16LE 
      || enc==SQLITE_UTF16BE );
  switch( enc ){
    case 0:
      pMem->flags |= MEM_Blob;
      pMem->enc = SQLITE_UTF8;
      break;

    case SQLITE_UTF8:
      pMem->flags |= MEM_Str;
      if( n<0 ){
        pMem->n = strlen(z);
        pMem->flags |= MEM_Term;
      }
      break;

#ifndef SQLITE_OMIT_UTF16
    case SQLITE_UTF16LE:
    case SQLITE_UTF16BE:
      pMem->flags |= MEM_Str;
      if( pMem->n<0 ){
        pMem->n = sqlite3utf16ByteLen(pMem->z,-1);
        pMem->flags |= MEM_Term;
      }
      if( sqlite3VdbeMemHandleBom(pMem) ){
        return SQLITE_NOMEM;
      }
#endif 
  }
  if( pMem->flags&MEM_Ephem ){
    return sqlite3VdbeMemMakeWriteable(pMem);
  }
  return SQLITE_OK;
}










int sqlite3MemCompare(const Mem *pMem1, const Mem *pMem2, const CollSeq *pColl){
  int rc;
  int f1, f2;
  int combined_flags;

  


  f1 = pMem1->flags;
  f2 = pMem2->flags;
  combined_flags = f1|f2;
 
  


  if( combined_flags&MEM_Null ){
    return (f2&MEM_Null) - (f1&MEM_Null);
  }

  



  if( combined_flags&(MEM_Int|MEM_Real) ){
    if( !(f1&(MEM_Int|MEM_Real)) ){
      return 1;
    }
    if( !(f2&(MEM_Int|MEM_Real)) ){
      return -1;
    }
    if( (f1 & f2 & MEM_Int)==0 ){
      double r1, r2;
      if( (f1&MEM_Real)==0 ){
        r1 = pMem1->i;
      }else{
        r1 = pMem1->r;
      }
      if( (f2&MEM_Real)==0 ){
        r2 = pMem2->i;
      }else{
        r2 = pMem2->r;
      }
      if( r1<r2 ) return -1;
      if( r1>r2 ) return 1;
      return 0;
    }else{
      assert( f1&MEM_Int );
      assert( f2&MEM_Int );
      if( pMem1->i < pMem2->i ) return -1;
      if( pMem1->i > pMem2->i ) return 1;
      return 0;
    }
  }

  


  if( combined_flags&MEM_Str ){
    if( (f1 & MEM_Str)==0 ){
      return 1;
    }
    if( (f2 & MEM_Str)==0 ){
      return -1;
    }

    assert( pMem1->enc==pMem2->enc );
    assert( pMem1->enc==SQLITE_UTF8 || 
            pMem1->enc==SQLITE_UTF16LE || pMem1->enc==SQLITE_UTF16BE );

    



    assert( !pColl || pColl->xCmp );

    if( pColl ){
      if( pMem1->enc==pColl->enc ){
        

        return pColl->xCmp(pColl->pUser,pMem1->n,pMem1->z,pMem2->n,pMem2->z);
      }else{
        u8 origEnc = pMem1->enc;
        const void *v1, *v2;
        int n1, n2;
        

        v1 = sqlite3ValueText((sqlite3_value*)pMem1, pColl->enc);
        n1 = v1==0 ? 0 : pMem1->n;
        assert( n1==sqlite3ValueBytes((sqlite3_value*)pMem1, pColl->enc) );
        v2 = sqlite3ValueText((sqlite3_value*)pMem2, pColl->enc);
        n2 = v2==0 ? 0 : pMem2->n;
        assert( n2==sqlite3ValueBytes((sqlite3_value*)pMem2, pColl->enc) );
        
        rc = pColl->xCmp(pColl->pUser, n1, v1, n2, v2);
        
        sqlite3ValueText((sqlite3_value*)pMem1, origEnc);
        sqlite3ValueText((sqlite3_value*)pMem2, origEnc);
        return rc;
      }
    }
    

  }
 
  
  rc = memcmp(pMem1->z, pMem2->z, (pMem1->n>pMem2->n)?pMem2->n:pMem1->n);
  if( rc==0 ){
    rc = pMem1->n - pMem2->n;
  }
  return rc;
}














int sqlite3VdbeMemFromBtree(
  BtCursor *pCur,   
  int offset,       
  int amt,          
  int key,          
  Mem *pMem         
){
  char *zData;      
  int available;    

  if( key ){
    zData = (char *)sqlite3BtreeKeyFetch(pCur, &available);
  }else{
    zData = (char *)sqlite3BtreeDataFetch(pCur, &available);
  }

  pMem->n = amt;
  if( offset+amt<=available ){
    pMem->z = &zData[offset];
    pMem->flags = MEM_Blob|MEM_Ephem;
  }else{
    int rc;
    if( amt>NBFS-2 ){
      zData = (char *)sqliteMallocRaw(amt+2);
      if( !zData ){
        return SQLITE_NOMEM;
      }
      pMem->flags = MEM_Blob|MEM_Dyn|MEM_Term;
      pMem->xDel = 0;
    }else{
      zData = &(pMem->zShort[0]);
      pMem->flags = MEM_Blob|MEM_Short|MEM_Term;
    }
    pMem->z = zData;
    pMem->enc = 0;
    pMem->type = SQLITE_BLOB;

    if( key ){
      rc = sqlite3BtreeKey(pCur, offset, amt, zData);
    }else{
      rc = sqlite3BtreeData(pCur, offset, amt, zData);
    }
    zData[amt] = 0;
    zData[amt+1] = 0;
    if( rc!=SQLITE_OK ){
      if( amt>NBFS-2 ){
        assert( zData!=pMem->zShort );
        assert( pMem->flags & MEM_Dyn );
        sqliteFree(zData);
      } else {
        assert( zData==pMem->zShort );
        assert( pMem->flags & MEM_Short );
      }
      return rc;
    }
  }

  return SQLITE_OK;
}

#ifndef NDEBUG




void sqlite3VdbeMemSanity(Mem *pMem){
  int flags = pMem->flags;
  assert( flags!=0 );  
  if( pMem->flags & (MEM_Str|MEM_Blob) ){
    int x = pMem->flags & (MEM_Static|MEM_Dyn|MEM_Ephem|MEM_Short);
    assert( x!=0 );            
    assert( (x & (x-1))==0 );  
    assert( pMem->z!=0 );      
    
    assert( (pMem->flags & MEM_Short)==0 || pMem->z==pMem->zShort );
    assert( (pMem->flags & MEM_Short)!=0 || pMem->z!=pMem->zShort );
    
    assert( pMem->xDel==0 || (pMem->flags & MEM_Dyn)!=0 );

    if( (flags & MEM_Str) ){
      assert( pMem->enc==SQLITE_UTF8 || 
              pMem->enc==SQLITE_UTF16BE ||
              pMem->enc==SQLITE_UTF16LE 
      );
      





      if( pMem->enc==SQLITE_UTF8 && (flags & MEM_Term) ){ 
        assert( strlen(pMem->z)<=pMem->n );
        assert( pMem->z[pMem->n]==0 );
      }
    }
  }else{
    
    assert( (pMem->flags & (MEM_Static|MEM_Dyn|MEM_Ephem|MEM_Short))==0 );
    assert( pMem->xDel==0 );
  }
  
  assert( (pMem->flags&(MEM_Str|MEM_Int|MEM_Real|MEM_Blob))==0
          || (pMem->flags&MEM_Null)==0 );
  
  assert( (pMem->flags & (MEM_Int|MEM_Real))!=(MEM_Int|MEM_Real) 
          || pMem->r==pMem->i );
}
#endif











const void *sqlite3ValueText(sqlite3_value* pVal, u8 enc){
  if( !pVal ) return 0;
  assert( (enc&3)==(enc&~SQLITE_UTF16_ALIGNED) );

  if( pVal->flags&MEM_Null ){
    return 0;
  }
  assert( (MEM_Blob>>3) == MEM_Str );
  pVal->flags |= (pVal->flags & MEM_Blob)>>3;
  if( pVal->flags&MEM_Str ){
    sqlite3VdbeChangeEncoding(pVal, enc & ~SQLITE_UTF16_ALIGNED);
    if( (enc & SQLITE_UTF16_ALIGNED)!=0 && 1==(1&(int)pVal->z) ){
      assert( (pVal->flags & (MEM_Ephem|MEM_Static))!=0 );
      if( sqlite3VdbeMemMakeWriteable(pVal)!=SQLITE_OK ){
        return 0;
      }
    }
  }else if( !(pVal->flags&MEM_Blob) ){
    sqlite3VdbeMemStringify(pVal, enc);
    assert( 0==(1&(int)pVal->z) );
  }
  assert(pVal->enc==(enc & ~SQLITE_UTF16_ALIGNED) || sqlite3MallocFailed() );
  if( pVal->enc==(enc & ~SQLITE_UTF16_ALIGNED) ){
    return pVal->z;
  }else{
    return 0;
  }
}




sqlite3_value* sqlite3ValueNew(void){
  Mem *p = sqliteMalloc(sizeof(*p));
  if( p ){
    p->flags = MEM_Null;
    p->type = SQLITE_NULL;
  }
  return p;
}











int sqlite3ValueFromExpr(
  Expr *pExpr, 
  u8 enc, 
  u8 affinity,
  sqlite3_value **ppVal
){
  int op;
  char *zVal = 0;
  sqlite3_value *pVal = 0;

  if( !pExpr ){
    *ppVal = 0;
    return SQLITE_OK;
  }
  op = pExpr->op;

  if( op==TK_STRING || op==TK_FLOAT || op==TK_INTEGER ){
    zVal = sqliteStrNDup((char*)pExpr->token.z, pExpr->token.n);
    pVal = sqlite3ValueNew();
    if( !zVal || !pVal ) goto no_mem;
    sqlite3Dequote(zVal);
    sqlite3ValueSetStr(pVal, -1, zVal, SQLITE_UTF8, sqlite3FreeX);
    if( (op==TK_INTEGER || op==TK_FLOAT ) && affinity==SQLITE_AFF_NONE ){
      sqlite3ValueApplyAffinity(pVal, SQLITE_AFF_NUMERIC, enc);
    }else{
      sqlite3ValueApplyAffinity(pVal, affinity, enc);
    }
  }else if( op==TK_UMINUS ) {
    if( SQLITE_OK==sqlite3ValueFromExpr(pExpr->pLeft, enc, affinity, &pVal) ){
      pVal->i = -1 * pVal->i;
      pVal->r = -1.0 * pVal->r;
    }
  }
#ifndef SQLITE_OMIT_BLOB_LITERAL
  else if( op==TK_BLOB ){
    int nVal;
    pVal = sqlite3ValueNew();
    zVal = sqliteStrNDup((char*)pExpr->token.z+1, pExpr->token.n-1);
    if( !zVal || !pVal ) goto no_mem;
    sqlite3Dequote(zVal);
    nVal = strlen(zVal)/2;
    sqlite3VdbeMemSetStr(pVal, sqlite3HexToBlob(zVal), nVal, 0, sqlite3FreeX);
    sqliteFree(zVal);
  }
#endif

  *ppVal = pVal;
  return SQLITE_OK;

no_mem:
  sqliteFree(zVal);
  sqlite3ValueFree(pVal);
  *ppVal = 0;
  return SQLITE_NOMEM;
}




void sqlite3ValueSetStr(
  sqlite3_value *v, 
  int n, 
  const void *z, 
  u8 enc,
  void (*xDel)(void*)
){
  if( v ) sqlite3VdbeMemSetStr((Mem *)v, z, n, enc, xDel);
}




void sqlite3ValueFree(sqlite3_value *v){
  if( !v ) return;
  sqlite3ValueSetStr(v, 0, 0, SQLITE_UTF8, SQLITE_STATIC);
  sqliteFree(v);
}





int sqlite3ValueBytes(sqlite3_value *pVal, u8 enc){
  Mem *p = (Mem*)pVal;
  if( (p->flags & MEM_Blob)!=0 || sqlite3ValueText(pVal, enc) ){
    return p->n;
  }
  return 0;
}
