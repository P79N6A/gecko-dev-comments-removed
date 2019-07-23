



























































#include "sqliteInt.h"
#include <assert.h>
#include "vdbeInt.h"






static const u8 xtra_utf8_bytes[256]  = {

0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,


255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,


1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,


2, 2, 2, 2, 2, 2, 2, 2,     2, 2, 2, 2, 2, 2, 2, 2,


3, 3, 3, 3, 3, 3, 3, 3,     255, 255, 255, 255, 255, 255, 255, 255,
};







static const int xtra_utf8_bits[4] =  {
0,
12416,          
925824,         
63447168        
};

#define READ_UTF8(zIn, c) { \
  int xtra;                                            \
  c = *(zIn)++;                                        \
  xtra = xtra_utf8_bytes[c];                           \
  switch( xtra ){                                      \
    case 255: c = (int)0xFFFD; break;                  \
    case 3: c = (c<<6) + *(zIn)++;                     \
    case 2: c = (c<<6) + *(zIn)++;                     \
    case 1: c = (c<<6) + *(zIn)++;                     \
    c -= xtra_utf8_bits[xtra];                         \
  }                                                    \
}
int sqlite3ReadUtf8(const unsigned char *z){
  int c;
  READ_UTF8(z, c);
  return c;
}

#define SKIP_UTF8(zIn) {                               \
  zIn += (xtra_utf8_bytes[*(u8 *)zIn] + 1);            \
}

#define WRITE_UTF8(zOut, c) {                          \
  if( c<0x00080 ){                                     \
    *zOut++ = (c&0xFF);                                \
  }                                                    \
  else if( c<0x00800 ){                                \
    *zOut++ = 0xC0 + ((c>>6)&0x1F);                    \
    *zOut++ = 0x80 + (c & 0x3F);                       \
  }                                                    \
  else if( c<0x10000 ){                                \
    *zOut++ = 0xE0 + ((c>>12)&0x0F);                   \
    *zOut++ = 0x80 + ((c>>6) & 0x3F);                  \
    *zOut++ = 0x80 + (c & 0x3F);                       \
  }else{                                               \
    *zOut++ = 0xF0 + ((c>>18) & 0x07);                 \
    *zOut++ = 0x80 + ((c>>12) & 0x3F);                 \
    *zOut++ = 0x80 + ((c>>6) & 0x3F);                  \
    *zOut++ = 0x80 + (c & 0x3F);                       \
  }                                                    \
}

#define WRITE_UTF16LE(zOut, c) {                                \
  if( c<=0xFFFF ){                                              \
    *zOut++ = (c&0x00FF);                                       \
    *zOut++ = ((c>>8)&0x00FF);                                  \
  }else{                                                        \
    *zOut++ = (((c>>10)&0x003F) + (((c-0x10000)>>10)&0x00C0));  \
    *zOut++ = (0x00D8 + (((c-0x10000)>>18)&0x03));              \
    *zOut++ = (c&0x00FF);                                       \
    *zOut++ = (0x00DC + ((c>>8)&0x03));                         \
  }                                                             \
}

#define WRITE_UTF16BE(zOut, c) {                                \
  if( c<=0xFFFF ){                                              \
    *zOut++ = ((c>>8)&0x00FF);                                  \
    *zOut++ = (c&0x00FF);                                       \
  }else{                                                        \
    *zOut++ = (0x00D8 + (((c-0x10000)>>18)&0x03));              \
    *zOut++ = (((c>>10)&0x003F) + (((c-0x10000)>>10)&0x00C0));  \
    *zOut++ = (0x00DC + ((c>>8)&0x03));                         \
    *zOut++ = (c&0x00FF);                                       \
  }                                                             \
}

#define READ_UTF16LE(zIn, c){                                         \
  c = (*zIn++);                                                       \
  c += ((*zIn++)<<8);                                                 \
  if( c>=0xD800 && c<=0xE000 ){                                       \
    int c2 = (*zIn++);                                                \
    c2 += ((*zIn++)<<8);                                              \
    c = (c2&0x03FF) + ((c&0x003F)<<10) + (((c&0x03C0)+0x0040)<<10);   \
  }                                                                   \
}

#define READ_UTF16BE(zIn, c){                                         \
  c = ((*zIn++)<<8);                                                  \
  c += (*zIn++);                                                      \
  if( c>=0xD800 && c<=0xE000 ){                                       \
    int c2 = ((*zIn++)<<8);                                           \
    c2 += (*zIn++);                                                   \
    c = (c2&0x03FF) + ((c&0x003F)<<10) + (((c&0x03C0)+0x0040)<<10);   \
  }                                                                   \
}

#define SKIP_UTF16BE(zIn){                                            \
  if( *zIn>=0xD8 && (*zIn<0xE0 || (*zIn==0xE0 && *(zIn+1)==0x00)) ){  \
    zIn += 4;                                                         \
  }else{                                                              \
    zIn += 2;                                                         \
  }                                                                   \
}
#define SKIP_UTF16LE(zIn){                                            \
  zIn++;                                                              \
  if( *zIn>=0xD8 && (*zIn<0xE0 || (*zIn==0xE0 && *(zIn-1)==0x00)) ){  \
    zIn += 3;                                                         \
  }else{                                                              \
    zIn += 1;                                                         \
  }                                                                   \
}

#define RSKIP_UTF16LE(zIn){                                            \
  if( *zIn>=0xD8 && (*zIn<0xE0 || (*zIn==0xE0 && *(zIn-1)==0x00)) ){  \
    zIn -= 4;                                                         \
  }else{                                                              \
    zIn -= 2;                                                         \
  }                                                                   \
}
#define RSKIP_UTF16BE(zIn){                                            \
  zIn--;                                                              \
  if( *zIn>=0xD8 && (*zIn<0xE0 || (*zIn==0xE0 && *(zIn+1)==0x00)) ){  \
    zIn -= 3;                                                         \
  }else{                                                              \
    zIn -= 1;                                                         \
  }                                                                   \
}




 


#ifndef SQLITE_OMIT_UTF16





int sqlite3VdbeMemTranslate(Mem *pMem, u8 desiredEnc){
  unsigned char zShort[NBFS]; 
  int len;                    
  unsigned char *zOut;                  
  unsigned char *zIn;                   
  unsigned char *zTerm;                 
  unsigned char *z;                     
  int c;

  assert( pMem->flags&MEM_Str );
  assert( pMem->enc!=desiredEnc );
  assert( pMem->enc!=0 );
  assert( pMem->n>=0 );

#if defined(TRANSLATE_TRACE) && defined(SQLITE_DEBUG)
  {
    char zBuf[100];
    sqlite3VdbeMemPrettyPrint(pMem, zBuf);
    fprintf(stderr, "INPUT:  %s\n", zBuf);
  }
#endif

  



  if( pMem->enc!=SQLITE_UTF8 && desiredEnc!=SQLITE_UTF8 ){
    u8 temp;
    int rc;
    rc = sqlite3VdbeMemMakeWriteable(pMem);
    if( rc!=SQLITE_OK ){
      assert( rc==SQLITE_NOMEM );
      return SQLITE_NOMEM;
    }
    zIn = (u8*)pMem->z;
    zTerm = &zIn[pMem->n];
    while( zIn<zTerm ){
      temp = *zIn;
      *zIn = *(zIn+1);
      zIn++;
      *zIn++ = temp;
    }
    pMem->enc = desiredEnc;
    goto translate_out;
  }

  
  if( desiredEnc==SQLITE_UTF8 ){
    




    len = pMem->n * 2 + 1;
  }else{
    




    len = pMem->n * 2 + 2;
  }

  






  zIn = (u8*)pMem->z;
  zTerm = &zIn[pMem->n];
  if( len>NBFS ){
    zOut = sqliteMallocRaw(len);
    if( !zOut ) return SQLITE_NOMEM;
  }else{
    zOut = zShort;
  }
  z = zOut;

  if( pMem->enc==SQLITE_UTF8 ){
    if( desiredEnc==SQLITE_UTF16LE ){
      
      while( zIn<zTerm ){
        READ_UTF8(zIn, c); 
        WRITE_UTF16LE(z, c);
      }
    }else{
      assert( desiredEnc==SQLITE_UTF16BE );
      
      while( zIn<zTerm ){
        READ_UTF8(zIn, c); 
        WRITE_UTF16BE(z, c);
      }
    }
    pMem->n = z - zOut;
    *z++ = 0;
  }else{
    assert( desiredEnc==SQLITE_UTF8 );
    if( pMem->enc==SQLITE_UTF16LE ){
      
      while( zIn<zTerm ){
        READ_UTF16LE(zIn, c); 
        WRITE_UTF8(z, c);
      }
    }else{
      
      while( zIn<zTerm ){
        READ_UTF16BE(zIn, c); 
        WRITE_UTF8(z, c);
      }
    }
    pMem->n = z - zOut;
  }
  *z = 0;
  assert( (pMem->n+(desiredEnc==SQLITE_UTF8?1:2))<=len );

  sqlite3VdbeMemRelease(pMem);
  pMem->flags &= ~(MEM_Static|MEM_Dyn|MEM_Ephem|MEM_Short);
  pMem->enc = desiredEnc;
  if( zOut==zShort ){
    memcpy(pMem->zShort, zOut, len);
    zOut = (u8*)pMem->zShort;
    pMem->flags |= (MEM_Term|MEM_Short);
  }else{
    pMem->flags |= (MEM_Term|MEM_Dyn);
  }
  pMem->z = (char*)zOut;

translate_out:
#if defined(TRANSLATE_TRACE) && defined(SQLITE_DEBUG)
  {
    char zBuf[100];
    sqlite3VdbeMemPrettyPrint(pMem, zBuf);
    fprintf(stderr, "OUTPUT: %s\n", zBuf);
  }
#endif
  return SQLITE_OK;
}










int sqlite3VdbeMemHandleBom(Mem *pMem){
  int rc = SQLITE_OK;
  u8 bom = 0;

  if( pMem->n<0 || pMem->n>1 ){
    u8 b1 = *(u8 *)pMem->z;
    u8 b2 = *(((u8 *)pMem->z) + 1);
    if( b1==0xFE && b2==0xFF ){
      bom = SQLITE_UTF16BE;
    }
    if( b1==0xFF && b2==0xFE ){
      bom = SQLITE_UTF16LE;
    }
  }
  
  if( bom ){
    




    assert( !(pMem->flags&MEM_Short) );
    assert( !(pMem->flags&MEM_Dyn) || pMem->xDel );
    if( pMem->flags & MEM_Dyn ){
      void (*xDel)(void*) = pMem->xDel;
      char *z = pMem->z;
      pMem->z = 0;
      pMem->xDel = 0;
      rc = sqlite3VdbeMemSetStr(pMem, &z[2], pMem->n-2, bom, SQLITE_TRANSIENT);
      xDel(z);
    }else{
      rc = sqlite3VdbeMemSetStr(pMem, &pMem->z[2], pMem->n-2, bom, 
          SQLITE_TRANSIENT);
    }
  }
  return rc;
}
#endif 








int sqlite3utf8CharLen(const char *z, int nByte){
  int r = 0;
  const char *zTerm;
  if( nByte>=0 ){
    zTerm = &z[nByte];
  }else{
    zTerm = (const char *)(-1);
  }
  assert( z<=zTerm );
  while( *z!=0 && z<zTerm ){
    SKIP_UTF8(z);
    r++;
  }
  return r;
}

#ifndef SQLITE_OMIT_UTF16







char *sqlite3utf16to8(const void *z, int nByte){
  Mem m;
  memset(&m, 0, sizeof(m));
  sqlite3VdbeMemSetStr(&m, z, nByte, SQLITE_UTF16NATIVE, SQLITE_STATIC);
  sqlite3VdbeChangeEncoding(&m, SQLITE_UTF8);
  assert( m.flags & MEM_Term );
  assert( m.flags & MEM_Str );
  return (m.flags & MEM_Dyn)!=0 ? m.z : sqliteStrDup(m.z);
}








int sqlite3utf16ByteLen(const void *zIn, int nChar){
  int c = 1;
  char const *z = zIn;
  int n = 0;
  if( SQLITE_UTF16NATIVE==SQLITE_UTF16BE ){
    








    while( c && ((nChar<0) || n<nChar) ){
      READ_UTF16BE(z, c);
      n++;
    }
  }else{
    while( c && ((nChar<0) || n<nChar) ){
      READ_UTF16LE(z, c);
      n++;
    }
  }
  return (z-(char const *)zIn)-((c==0)?2:0);
}




void sqlite3utf16Substr(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  int y, z;
  unsigned char const *zStr;
  unsigned char const *zStrEnd;
  unsigned char const *zStart;
  unsigned char const *zEnd;
  int i;

  zStr = (unsigned char const *)sqlite3_value_text16(argv[0]);
  zStrEnd = &zStr[sqlite3_value_bytes16(argv[0])];
  y = sqlite3_value_int(argv[1]);
  z = sqlite3_value_int(argv[2]);

  if( y>0 ){
    y = y-1;
    zStart = zStr;
    if( SQLITE_UTF16BE==SQLITE_UTF16NATIVE ){
      for(i=0; i<y && zStart<zStrEnd; i++) SKIP_UTF16BE(zStart);
    }else{
      for(i=0; i<y && zStart<zStrEnd; i++) SKIP_UTF16LE(zStart);
    }
  }else{
    zStart = zStrEnd;
    if( SQLITE_UTF16BE==SQLITE_UTF16NATIVE ){
      for(i=y; i<0 && zStart>zStr; i++) RSKIP_UTF16BE(zStart);
    }else{
      for(i=y; i<0 && zStart>zStr; i++) RSKIP_UTF16LE(zStart);
    }
    for(; i<0; i++) z -= 1;
  }

  zEnd = zStart;
  if( SQLITE_UTF16BE==SQLITE_UTF16NATIVE ){
    for(i=0; i<z && zEnd<zStrEnd; i++) SKIP_UTF16BE(zEnd);
  }else{
    for(i=0; i<z && zEnd<zStrEnd; i++) SKIP_UTF16LE(zEnd);
  }

  sqlite3_result_text16(context, zStart, zEnd-zStart, SQLITE_TRANSIENT);
}

#if defined(SQLITE_TEST)





void sqlite3utfSelfTest(){
  int i;
  unsigned char zBuf[20];
  unsigned char *z;
  int n;
  int c;

  for(i=0; i<0x00110000; i++){
    z = zBuf;
    WRITE_UTF8(z, i);
    n = z-zBuf;
    z = zBuf;
    READ_UTF8(z, c);
    assert( c==i );
    assert( (z-zBuf)==n );
  }
  for(i=0; i<0x00110000; i++){
    if( i>=0xD800 && i<=0xE000 ) continue;
    z = zBuf;
    WRITE_UTF16LE(z, i);
    n = z-zBuf;
    z = zBuf;
    READ_UTF16LE(z, c);
    assert( c==i );
    assert( (z-zBuf)==n );
  }
  for(i=0; i<0x00110000; i++){
    if( i>=0xD800 && i<=0xE000 ) continue;
    z = zBuf;
    WRITE_UTF16BE(z, i);
    n = z-zBuf;
    z = zBuf;
    READ_UTF16BE(z, c);
    assert( c==i );
    assert( (z-zBuf)==n );
  }
}
#endif 
#endif 
