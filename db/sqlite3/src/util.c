

















#include "sqliteInt.h"
#include "os.h"
#include <stdarg.h>
#include <ctype.h>












































#define MAX(x,y) ((x)>(y)?(x):(y))

#if defined(SQLITE_ENABLE_MEMORY_MANAGEMENT) && !defined(SQLITE_OMIT_DISKIO)




void sqlite3_soft_heap_limit(int n){
  ThreadData *pTd = sqlite3ThreadData();
  if( pTd ){
    pTd->nSoftHeapLimit = n;
  }
  sqlite3ReleaseThreadData();
}




int sqlite3_release_memory(int n){
  return sqlite3pager_release_memory(n);
}
#else





#define sqlite3_release_memory(x) 0    /* 0 == no memory freed */
#endif

#ifdef SQLITE_MEMDEBUG















#if defined(__GLIBC__) && SQLITE_MEMDEBUG>2
  extern int backtrace(void **, int);
  #define TESTALLOC_STACKSIZE 128
  #define TESTALLOC_STACKFRAMES ((TESTALLOC_STACKSIZE-8)/sizeof(void*))
#else
  #define backtrace(x, y)
  #define TESTALLOC_STACKSIZE 0
  #define TESTALLOC_STACKFRAMES 0
#endif






#ifndef TESTALLOC_NGUARD
# define TESTALLOC_NGUARD 2
#endif




#define TESTALLOC_FILESIZE 64






#define TESTALLOC_USERSIZE 64
const char *sqlite3_malloc_id = 0;












 

#define TESTALLOC_OFFSET_GUARD1(p)    (sizeof(void *) * 2)
#define TESTALLOC_OFFSET_DATA(p) ( \
  TESTALLOC_OFFSET_GUARD1(p) + sizeof(u32) * TESTALLOC_NGUARD \
)
#define TESTALLOC_OFFSET_GUARD2(p) ( \
  TESTALLOC_OFFSET_DATA(p) + sqlite3OsAllocationSize(p) - TESTALLOC_OVERHEAD \
)
#define TESTALLOC_OFFSET_LINENUMBER(p) ( \
  TESTALLOC_OFFSET_GUARD2(p) + sizeof(u32) * TESTALLOC_NGUARD \
)
#define TESTALLOC_OFFSET_FILENAME(p) ( \
  TESTALLOC_OFFSET_LINENUMBER(p) + sizeof(u32) \
)
#define TESTALLOC_OFFSET_USER(p) ( \
  TESTALLOC_OFFSET_FILENAME(p) + TESTALLOC_FILESIZE \
)
#define TESTALLOC_OFFSET_STACK(p) ( \
  TESTALLOC_OFFSET_USER(p) + TESTALLOC_USERSIZE + 8 - \
  (TESTALLOC_OFFSET_USER(p) % 8) \
)

#define TESTALLOC_OVERHEAD ( \
  sizeof(void *)*2 +                   /* pPrev and pNext pointers */   \
  TESTALLOC_NGUARD*sizeof(u32)*2 +              /* Guard words */       \
  sizeof(u32) + TESTALLOC_FILESIZE +   /* File and line number */       \
  TESTALLOC_USERSIZE +                 /* User string */                \
  TESTALLOC_STACKSIZE                  /* backtrace() stack */          \
)









int sqlite3_nMalloc;         
int sqlite3_nFree;           
int sqlite3_memUsed;         
int sqlite3_memMax;          
int sqlite3_iMallocFail;     
int sqlite3_iMallocReset = -1; 

void *sqlite3_pFirst = 0;         
int sqlite3_nMaxAlloc = 0;        
int sqlite3_mallocDisallowed = 0; 
int sqlite3_isFail = 0;           
const char *sqlite3_zFile = 0;    
int sqlite3_iLine = 0;            





int sqlite3TestMallocFail(){
  if( sqlite3_isFail ){
    return 1;
  }
  if( sqlite3_iMallocFail>=0 ){
    sqlite3_iMallocFail--;
    if( sqlite3_iMallocFail==0 ){
      sqlite3_iMallocFail = sqlite3_iMallocReset;
      sqlite3_isFail = 1;
      return 1;
    }
  }
  return 0;
}






static void checkGuards(u32 *p)
{
  int i;
  char *zAlloc = (char *)p;
  char *z;

  
  z = &zAlloc[TESTALLOC_OFFSET_GUARD1(p)];
  for(i=0; i<TESTALLOC_NGUARD; i++){
    assert(((u32 *)z)[i]==0xdead1122);
  }

  
  z = &zAlloc[TESTALLOC_OFFSET_GUARD2(p)];
  for(i=0; i<TESTALLOC_NGUARD; i++){
    u32 guard = 0;
    memcpy(&guard, &z[i*sizeof(u32)], sizeof(u32));
    assert(guard==0xdead3344);
  }
}






static void applyGuards(u32 *p)
{
  int i;
  char *z;
  char *zAlloc = (char *)p;

  
  z = &zAlloc[TESTALLOC_OFFSET_GUARD1(p)];
  for(i=0; i<TESTALLOC_NGUARD; i++){
    ((u32 *)z)[i] = 0xdead1122;
  }

  
  z = &zAlloc[TESTALLOC_OFFSET_GUARD2(p)];
  for(i=0; i<TESTALLOC_NGUARD; i++){
    static const int guard = 0xdead3344;
    memcpy(&z[i*sizeof(u32)], &guard, sizeof(u32));
  }

  
  z = &((char *)z)[TESTALLOC_NGUARD*sizeof(u32)];             
  z = &zAlloc[TESTALLOC_OFFSET_LINENUMBER(p)];
  memcpy(z, &sqlite3_iLine, sizeof(u32));

  
  z = &zAlloc[TESTALLOC_OFFSET_FILENAME(p)];
  strncpy(z, sqlite3_zFile, TESTALLOC_FILESIZE);
  z[TESTALLOC_FILESIZE - 1] = '\0';

  
  z = &zAlloc[TESTALLOC_OFFSET_USER(p)];
  z[0] = 0;
  if( sqlite3_malloc_id ){
    strncpy(z, sqlite3_malloc_id, TESTALLOC_USERSIZE);
    z[TESTALLOC_USERSIZE-1] = 0;
  }

  
  z = &zAlloc[TESTALLOC_OFFSET_STACK(p)];
  backtrace((void **)z, TESTALLOC_STACKFRAMES);

  
  checkGuards(p);
}





static void *getOsPointer(void *p)
{
  char *z = (char *)p;
  return (void *)(&z[-1 * TESTALLOC_OFFSET_DATA(p)]);
}


#if SQLITE_MEMDEBUG>1




static void linkAlloc(void *p){
  void **pp = (void **)p;
  pp[0] = 0;
  pp[1] = sqlite3_pFirst;
  if( sqlite3_pFirst ){
    ((void **)sqlite3_pFirst)[0] = p;
  }
  sqlite3_pFirst = p;
}





static void unlinkAlloc(void *p)
{
  void **pp = (void **)p;
  if( p==sqlite3_pFirst ){
    assert(!pp[0]);
    assert(!pp[1] || ((void **)(pp[1]))[0]==p);
    sqlite3_pFirst = pp[1];
    if( sqlite3_pFirst ){
      ((void **)sqlite3_pFirst)[0] = 0;
    }
  }else{
    void **pprev = pp[0];
    void **pnext = pp[1];
    assert(pprev);
    assert(pprev[1]==p);
    pprev[1] = (void *)pnext;
    if( pnext ){
      assert(pnext[0]==p);
      pnext[0] = (void *)pprev;
    }
  }
}






static void relinkAlloc(void *p)
{
  void **pp = (void **)p;
  if( pp[0] ){
    ((void **)(pp[0]))[1] = p;
  }else{
    sqlite3_pFirst = p;
  }
  if( pp[1] ){
    ((void **)(pp[1]))[0] = p;
  }
}
#else
#define linkAlloc(x)
#define relinkAlloc(x)
#define unlinkAlloc(x)
#endif
















#if defined(TCLSH) && defined(SQLITE_DEBUG) && SQLITE_MEMDEBUG>1
#include <tcl.h>
int sqlite3OutstandingMallocs(Tcl_Interp *interp){
  void *p;
  Tcl_Obj *pRes = Tcl_NewObj();
  Tcl_IncrRefCount(pRes);


  for(p=sqlite3_pFirst; p; p=((void **)p)[1]){
    Tcl_Obj *pEntry = Tcl_NewObj();
    Tcl_Obj *pStack = Tcl_NewObj();
    char *z;
    u32 iLine;
    int nBytes = sqlite3OsAllocationSize(p) - TESTALLOC_OVERHEAD;
    char *zAlloc = (char *)p;
    int i;

    Tcl_ListObjAppendElement(0, pEntry, Tcl_NewIntObj(nBytes));

    z = &zAlloc[TESTALLOC_OFFSET_FILENAME(p)];
    Tcl_ListObjAppendElement(0, pEntry, Tcl_NewStringObj(z, -1));

    z = &zAlloc[TESTALLOC_OFFSET_LINENUMBER(p)];
    memcpy(&iLine, z, sizeof(u32));
    Tcl_ListObjAppendElement(0, pEntry, Tcl_NewIntObj(iLine));

    z = &zAlloc[TESTALLOC_OFFSET_USER(p)];
    Tcl_ListObjAppendElement(0, pEntry, Tcl_NewStringObj(z, -1));

    z = &zAlloc[TESTALLOC_OFFSET_STACK(p)];
    for(i=0; i<TESTALLOC_STACKFRAMES; i++){
      char zHex[128];
      sprintf(zHex, "%p", ((void **)z)[i]);
      Tcl_ListObjAppendElement(0, pStack, Tcl_NewStringObj(zHex, -1));
    }

    Tcl_ListObjAppendElement(0, pEntry, pStack);
    Tcl_ListObjAppendElement(0, pRes, pEntry);
  }

  Tcl_ResetResult(interp);
  Tcl_SetObjResult(interp, pRes);
  Tcl_DecrRefCount(pRes);
  return TCL_OK;
}
#endif




static void * OSMALLOC(int n){
  sqlite3OsEnterMutex();
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  sqlite3_nMaxAlloc = 
      MAX(sqlite3_nMaxAlloc, sqlite3ThreadDataReadOnly()->nAlloc);
#endif
  assert( !sqlite3_mallocDisallowed );
  if( !sqlite3TestMallocFail() ){
    u32 *p;
    p = (u32 *)sqlite3OsMalloc(n + TESTALLOC_OVERHEAD);
    assert(p);
    sqlite3_nMalloc++;
    applyGuards(p);
    linkAlloc(p);
    sqlite3OsLeaveMutex();
    return (void *)(&p[TESTALLOC_NGUARD + 2*sizeof(void *)/sizeof(u32)]);
  }
  sqlite3OsLeaveMutex();
  return 0;
}

static int OSSIZEOF(void *p){
  if( p ){
    u32 *pOs = (u32 *)getOsPointer(p);
    return sqlite3OsAllocationSize(pOs) - TESTALLOC_OVERHEAD;
  }
  return 0;
}





static void OSFREE(void *pFree){
  u32 *p;         
  sqlite3OsEnterMutex();
  p = (u32 *)getOsPointer(pFree);
  checkGuards(p);
  unlinkAlloc(p);
  memset(pFree, 0x55, OSSIZEOF(pFree));
  sqlite3OsFree(p);
  sqlite3_nFree++;
  sqlite3OsLeaveMutex();
}




static void * OSREALLOC(void *pRealloc, int n){
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  sqlite3_nMaxAlloc = 
      MAX(sqlite3_nMaxAlloc, sqlite3ThreadDataReadOnly()->nAlloc);
#endif
  assert( !sqlite3_mallocDisallowed );
  if( !sqlite3TestMallocFail() ){
    u32 *p = (u32 *)getOsPointer(pRealloc);
    checkGuards(p);
    p = sqlite3OsRealloc(p, n + TESTALLOC_OVERHEAD);
    applyGuards(p);
    relinkAlloc(p);
    return (void *)(&p[TESTALLOC_NGUARD + 2*sizeof(void *)/sizeof(u32)]);
  }
  return 0;
}

static void OSMALLOC_FAILED(){
  sqlite3_isFail = 0;
}

#else



#define OSMALLOC(x)        sqlite3OsMalloc(x)
#define OSREALLOC(x,y)     sqlite3OsRealloc(x,y)
#define OSFREE(x)          sqlite3OsFree(x)
#define OSSIZEOF(x)        sqlite3OsAllocationSize(x)
#define OSMALLOC_FAILED()

#endif  

















 
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
static int enforceSoftLimit(int n){
  ThreadData *pTsd = sqlite3ThreadData();
  if( pTsd==0 ){
    return 0;
  }
  assert( pTsd->nAlloc>=0 );
  if( n>0 && pTsd->nSoftHeapLimit>0 ){
    while( pTsd->nAlloc+n>pTsd->nSoftHeapLimit && sqlite3_release_memory(n) ){}
  }
  return 1;
}
#else
# define enforceSoftLimit(X)  1
#endif









#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
static void updateMemoryUsedCount(int n){
  ThreadData *pTsd = sqlite3ThreadData();
  if( pTsd ){
    pTsd->nAlloc += n;
    assert( pTsd->nAlloc>=0 );
    if( pTsd->nAlloc==0 && pTsd->nSoftHeapLimit==0 ){
      sqlite3ReleaseThreadData();
    }
  }
}
#else
#define updateMemoryUsedCount(x)
#endif






void *sqlite3MallocRaw(int n, int doMemManage){
  void *p = 0;
  if( n>0 && !sqlite3MallocFailed() && (!doMemManage || enforceSoftLimit(n)) ){
    while( (p = OSMALLOC(n))==0 && sqlite3_release_memory(n) ){}
    if( !p ){
      sqlite3FailedMalloc();
      OSMALLOC_FAILED();
    }else if( doMemManage ){
      updateMemoryUsedCount(OSSIZEOF(p));
    }
  }
  return p;
}






void *sqlite3Realloc(void *p, int n){
  if( sqlite3MallocFailed() ){
    return 0;
  }

  if( !p ){
    return sqlite3Malloc(n, 1);
  }else{
    void *np = 0;
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
    int origSize = OSSIZEOF(p);
#endif
    if( enforceSoftLimit(n - origSize) ){
      while( (np = OSREALLOC(p, n))==0 && sqlite3_release_memory(n) ){}
      if( !np ){
        sqlite3FailedMalloc();
        OSMALLOC_FAILED();
      }else{
        updateMemoryUsedCount(OSSIZEOF(np) - origSize);
      }
    }
    return np;
  }
}





void sqlite3FreeX(void *p){
  if( p ){
    updateMemoryUsedCount(0 - OSSIZEOF(p));
    OSFREE(p);
  }
}





void *sqlite3MallocX(int n){
  return sqliteMalloc(n);
}







 
void *sqlite3Malloc(int n, int doMemManage){
  void *p = sqlite3MallocRaw(n, doMemManage);
  if( p ){
    memset(p, 0, n);
  }
  return p;
}
void sqlite3ReallocOrFree(void **pp, int n){
  void *p = sqlite3Realloc(*pp, n);
  if( !p ){
    sqlite3FreeX(*pp);
  }
  *pp = p;
}














#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
void *sqlite3ThreadSafeMalloc(int n){
  ENTER_MALLOC;
  return sqlite3Malloc(n, 0);
}
void sqlite3ThreadSafeFree(void *p){
  ENTER_MALLOC;
  if( p ){
    OSFREE(p);
  }
}
#endif












#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
int sqlite3AllocSize(void *p){
  return OSSIZEOF(p);
}
#endif








char *sqlite3StrDup(const char *z){
  char *zNew;
  if( z==0 ) return 0;
  zNew = sqlite3MallocRaw(strlen(z)+1, 1);
  if( zNew ) strcpy(zNew, z);
  return zNew;
}
char *sqlite3StrNDup(const char *z, int n){
  char *zNew;
  if( z==0 ) return 0;
  zNew = sqlite3MallocRaw(n+1, 1);
  if( zNew ){
    memcpy(zNew, z, n);
    zNew[n] = 0;
  }
  return zNew;
}








void sqlite3SetString(char **pz, ...){
  va_list ap;
  int nByte;
  const char *z;
  char *zResult;

  if( pz==0 ) return;
  nByte = 1;
  va_start(ap, pz);
  while( (z = va_arg(ap, const char*))!=0 ){
    nByte += strlen(z);
  }
  va_end(ap);
  sqliteFree(*pz);
  *pz = zResult = sqliteMallocRaw( nByte );
  if( zResult==0 ){
    return;
  }
  *zResult = 0;
  va_start(ap, pz);
  while( (z = va_arg(ap, const char*))!=0 ){
    strcpy(zResult, z);
    zResult += strlen(zResult);
  }
  va_end(ap);
}






















void sqlite3Error(sqlite3 *db, int err_code, const char *zFormat, ...){
  if( db && (db->pErr || (db->pErr = sqlite3ValueNew())!=0) ){
    db->errCode = err_code;
    if( zFormat ){
      char *z;
      va_list ap;
      va_start(ap, zFormat);
      z = sqlite3VMPrintf(zFormat, ap);
      va_end(ap);
      sqlite3ValueSetStr(db->pErr, -1, z, SQLITE_UTF8, sqlite3FreeX);
    }else{
      sqlite3ValueSetStr(db->pErr, 0, 0, SQLITE_UTF8, SQLITE_STATIC);
    }
  }
}


















void sqlite3ErrorMsg(Parse *pParse, const char *zFormat, ...){
  va_list ap;
  pParse->nErr++;
  sqliteFree(pParse->zErrMsg);
  va_start(ap, zFormat);
  pParse->zErrMsg = sqlite3VMPrintf(zFormat, ap);
  va_end(ap);
}




void sqlite3ErrorClear(Parse *pParse){
  sqliteFree(pParse->zErrMsg);
  pParse->zErrMsg = 0;
  pParse->nErr = 0;
}











void sqlite3Dequote(char *z){
  int quote;
  int i, j;
  if( z==0 ) return;
  quote = z[0];
  switch( quote ){
    case '\'':  break;
    case '"':   break;
    case '`':   break;                
    case '[':   quote = ']';  break;  
    default:    return;
  }
  for(i=1, j=0; z[i]; i++){
    if( z[i]==quote ){
      if( z[i+1]==quote ){
        z[j++] = quote;
        i++;
      }else{
        z[j++] = 0;
        break;
      }
    }else{
      z[j++] = z[i];
    }
  }
}




const unsigned char sqlite3UpperToLower[] = {
#ifdef SQLITE_ASCII
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
     18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
     36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
     54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 97, 98, 99,100,101,102,103,
    104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,
    122, 91, 92, 93, 94, 95, 96, 97, 98, 99,100,101,102,103,104,105,106,107,
    108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,
    126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,
    162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,
    180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,
    198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
    216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,
    234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,
    252,253,254,255
#endif
#ifdef SQLITE_EBCDIC
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 
     64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
     80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 
     96, 97, 66, 67, 68, 69, 70, 71, 72, 73,106,107,108,109,110,111, 
    112, 81, 82, 83, 84, 85, 86, 87, 88, 89,122,123,124,125,126,127, 
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143, 
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,156,159, 
    160,161,162,163,164,165,166,167,168,169,170,171,140,141,142,175, 
    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191, 
    192,129,130,131,132,133,134,135,136,137,202,203,204,205,206,207, 
    208,145,146,147,148,149,150,151,152,153,218,219,220,221,222,223, 
    224,225,162,163,164,165,166,167,168,169,232,203,204,205,206,207, 
    239,240,241,242,243,244,245,246,247,248,249,219,220,221,222,255, 
#endif
};
#define UpperToLower sqlite3UpperToLower





int sqlite3StrICmp(const char *zLeft, const char *zRight){
  register unsigned char *a, *b;
  a = (unsigned char *)zLeft;
  b = (unsigned char *)zRight;
  while( *a!=0 && UpperToLower[*a]==UpperToLower[*b]){ a++; b++; }
  return UpperToLower[*a] - UpperToLower[*b];
}
int sqlite3StrNICmp(const char *zLeft, const char *zRight, int N){
  register unsigned char *a, *b;
  a = (unsigned char *)zLeft;
  b = (unsigned char *)zRight;
  while( N-- > 0 && *a!=0 && UpperToLower[*a]==UpperToLower[*b]){ a++; b++; }
  return N<0 ? 0 : UpperToLower[*a] - UpperToLower[*b];
}









int sqlite3IsNumber(const char *z, int *realnum, u8 enc){
  int incr = (enc==SQLITE_UTF8?1:2);
  if( enc==SQLITE_UTF16BE ) z++;
  if( *z=='-' || *z=='+' ) z += incr;
  if( !isdigit(*(u8*)z) ){
    return 0;
  }
  z += incr;
  if( realnum ) *realnum = 0;
  while( isdigit(*(u8*)z) ){ z += incr; }
  if( *z=='.' ){
    z += incr;
    if( !isdigit(*(u8*)z) ) return 0;
    while( isdigit(*(u8*)z) ){ z += incr; }
    if( realnum ) *realnum = 1;
  }
  if( *z=='e' || *z=='E' ){
    z += incr;
    if( *z=='+' || *z=='-' ) z += incr;
    if( !isdigit(*(u8*)z) ) return 0;
    while( isdigit(*(u8*)z) ){ z += incr; }
    if( realnum ) *realnum = 1;
  }
  return *z==0;
}













int sqlite3AtoF(const char *z, double *pResult){
#ifndef SQLITE_OMIT_FLOATING_POINT
  int sign = 1;
  const char *zBegin = z;
  LONGDOUBLE_TYPE v1 = 0.0;
  while( isspace(*z) ) z++;
  if( *z=='-' ){
    sign = -1;
    z++;
  }else if( *z=='+' ){
    z++;
  }
  while( isdigit(*(u8*)z) ){
    v1 = v1*10.0 + (*z - '0');
    z++;
  }
  if( *z=='.' ){
    LONGDOUBLE_TYPE divisor = 1.0;
    z++;
    while( isdigit(*(u8*)z) ){
      v1 = v1*10.0 + (*z - '0');
      divisor *= 10.0;
      z++;
    }
    v1 /= divisor;
  }
  if( *z=='e' || *z=='E' ){
    int esign = 1;
    int eval = 0;
    LONGDOUBLE_TYPE scale = 1.0;
    z++;
    if( *z=='-' ){
      esign = -1;
      z++;
    }else if( *z=='+' ){
      z++;
    }
    while( isdigit(*(u8*)z) ){
      eval = eval*10 + *z - '0';
      z++;
    }
    while( eval>=64 ){ scale *= 1.0e+64; eval -= 64; }
    while( eval>=16 ){ scale *= 1.0e+16; eval -= 16; }
    while( eval>=4 ){ scale *= 1.0e+4; eval -= 4; }
    while( eval>=1 ){ scale *= 1.0e+1; eval -= 1; }
    if( esign<0 ){
      v1 /= scale;
    }else{
      v1 *= scale;
    }
  }
  *pResult = sign<0 ? -v1 : v1;
  return z - zBegin;
#else
  return sqlite3atoi64(z, pResult);
#endif 
}












int sqlite3atoi64(const char *zNum, i64 *pNum){
  i64 v = 0;
  int neg;
  int i, c;
  while( isspace(*zNum) ) zNum++;
  if( *zNum=='-' ){
    neg = 1;
    zNum++;
  }else if( *zNum=='+' ){
    neg = 0;
    zNum++;
  }else{
    neg = 0;
  }
  for(i=0; (c=zNum[i])>='0' && c<='9'; i++){
    v = v*10 + c - '0';
  }
  *pNum = neg ? -v : v;
  return c==0 && i>0 && 
      (i<19 || (i==19 && memcmp(zNum,"9223372036854775807",19)<=0));
}












static int sqlite3FitsIn32Bits(const char *zNum){
  int i, c;
  if( *zNum=='-' || *zNum=='+' ) zNum++;
  for(i=0; (c=zNum[i])>='0' && c<='9'; i++){}
  return i<10 || (i==10 && memcmp(zNum,"2147483647",10)<=0);
}





int sqlite3GetInt32(const char *zNum, int *pValue){
  if( sqlite3FitsIn32Bits(zNum) ){
    *pValue = atoi(zNum);
    return 1;
  }
  return 0;
}












int sqlite3FitsIn64Bits(const char *zNum){
  int i, c;
  if( *zNum=='-' || *zNum=='+' ) zNum++;
  for(i=0; (c=zNum[i])>='0' && c<='9'; i++){}
  return i<19 || (i==19 && memcmp(zNum,"9223372036854775807",19)<=0);
}




















int sqlite3SafetyOn(sqlite3 *db){
  if( db->magic==SQLITE_MAGIC_OPEN ){
    db->magic = SQLITE_MAGIC_BUSY;
    return 0;
  }else if( db->magic==SQLITE_MAGIC_BUSY ){
    db->magic = SQLITE_MAGIC_ERROR;
    db->flags |= SQLITE_Interrupt;
  }
  return 1;
}






int sqlite3SafetyOff(sqlite3 *db){
  if( db->magic==SQLITE_MAGIC_BUSY ){
    db->magic = SQLITE_MAGIC_OPEN;
    return 0;
  }else if( db->magic==SQLITE_MAGIC_OPEN ){
    db->magic = SQLITE_MAGIC_ERROR;
    db->flags |= SQLITE_Interrupt;
  }
  return 1;
}










int sqlite3SafetyCheck(sqlite3 *db){
  int magic;
  if( db==0 ) return 1;
  magic = db->magic;
  if( magic!=SQLITE_MAGIC_CLOSED &&
         magic!=SQLITE_MAGIC_OPEN &&
         magic!=SQLITE_MAGIC_BUSY ) return 1;
  return 0;
}






























int sqlite3PutVarint(unsigned char *p, u64 v){
  int i, j, n;
  u8 buf[10];
  if( v & (((u64)0xff000000)<<32) ){
    p[8] = v;
    v >>= 8;
    for(i=7; i>=0; i--){
      p[i] = (v & 0x7f) | 0x80;
      v >>= 7;
    }
    return 9;
  }    
  n = 0;
  do{
    buf[n++] = (v & 0x7f) | 0x80;
    v >>= 7;
  }while( v!=0 );
  buf[0] &= 0x7f;
  assert( n<=9 );
  for(i=0, j=n-1; j>=0; j--, i++){
    p[i] = buf[j];
  }
  return n;
}





int sqlite3GetVarint(const unsigned char *p, u64 *v){
  u32 x;
  u64 x64;
  int n;
  unsigned char c;
  if( ((c = p[0]) & 0x80)==0 ){
    *v = c;
    return 1;
  }
  x = c & 0x7f;
  if( ((c = p[1]) & 0x80)==0 ){
    *v = (x<<7) | c;
    return 2;
  }
  x = (x<<7) | (c&0x7f);
  if( ((c = p[2]) & 0x80)==0 ){
    *v = (x<<7) | c;
    return 3;
  }
  x = (x<<7) | (c&0x7f);
  if( ((c = p[3]) & 0x80)==0 ){
    *v = (x<<7) | c;
    return 4;
  }
  x64 = (x<<7) | (c&0x7f);
  n = 4;
  do{
    c = p[n++];
    if( n==9 ){
      x64 = (x64<<8) | c;
      break;
    }
    x64 = (x64<<7) | (c&0x7f);
  }while( (c & 0x80)!=0 );
  *v = x64;
  return n;
}





int sqlite3GetVarint32(const unsigned char *p, u32 *v){
  u32 x;
  int n;
  unsigned char c;
  if( ((signed char*)p)[0]>=0 ){
    *v = p[0];
    return 1;
  }
  x = p[0] & 0x7f;
  if( ((signed char*)p)[1]>=0 ){
    *v = (x<<7) | p[1];
    return 2;
  }
  x = (x<<7) | (p[1] & 0x7f);
  n = 2;
  do{
    x = (x<<7) | ((c = p[n++])&0x7f);
  }while( (c & 0x80)!=0 && n<9 );
  *v = x;
  return n;
}





int sqlite3VarintLen(u64 v){
  int i = 0;
  do{
    i++;
    v >>= 7;
  }while( v!=0 && i<9 );
  return i;
}

#if !defined(SQLITE_OMIT_BLOB_LITERAL) || defined(SQLITE_HAS_CODEC) \
    || defined(SQLITE_TEST)



static int hexToInt(int h){
  if( h>='0' && h<='9' ){
    return h - '0';
  }else if( h>='a' && h<='f' ){
    return h - 'a' + 10;
  }else{
    assert( h>='A' && h<='F' );
    return h - 'A' + 10;
  }
}
#endif 

#if !defined(SQLITE_OMIT_BLOB_LITERAL) || defined(SQLITE_HAS_CODEC)






void *sqlite3HexToBlob(const char *z){
  char *zBlob;
  int i;
  int n = strlen(z);
  if( n%2 ) return 0;

  zBlob = (char *)sqliteMalloc(n/2);
  for(i=0; i<n; i+=2){
    zBlob[i/2] = (hexToInt(z[i])<<4) | hexToInt(z[i+1]);
  }
  return zBlob;
}
#endif 

#if defined(SQLITE_TEST)




void *sqlite3TextToPtr(const char *z){
  void *p;
  u64 v;
  u32 v2;
  if( z[0]=='0' && z[1]=='x' ){
    z += 2;
  }
  v = 0;
  while( *z ){
    v = (v<<4) + hexToInt(*z);
    z++;
  }
  if( sizeof(p)==sizeof(v) ){
    p = *(void**)&v;
  }else{
    assert( sizeof(p)==sizeof(v2) );
    v2 = (u32)v;
    p = *(void**)&v2;
  }
  return p;
}
#endif




ThreadData *sqlite3ThreadData(){
  ThreadData *p = (ThreadData*)sqlite3OsThreadSpecificData(1);
  if( !p ){
    sqlite3FailedMalloc();
  }
  return p;
}






const ThreadData *sqlite3ThreadDataReadOnly(){
  static const ThreadData zeroData = {0};  

  const ThreadData *pTd = sqlite3OsThreadSpecificData(0);
  return pTd ? pTd : &zeroData;
}





void sqlite3ReleaseThreadData(){
  sqlite3OsThreadSpecificData(-1);
}














static int mallocHasFailed = 0;
int sqlite3ApiExit(sqlite3* db, int rc){
  if( sqlite3MallocFailed() ){
    mallocHasFailed = 0;
    sqlite3OsLeaveMutex();
    sqlite3Error(db, SQLITE_NOMEM, 0);
    rc = SQLITE_NOMEM;
  }
  return rc;
}





int sqlite3MallocFailed(){
  return (mallocHasFailed && sqlite3OsInMutex(1));
}




void sqlite3FailedMalloc(){
  sqlite3OsEnterMutex();
  assert( mallocHasFailed==0 );
  mallocHasFailed = 1;
}

#ifdef SQLITE_MEMDEBUG




void sqlite3MallocDisallow(){
  assert( sqlite3_mallocDisallowed>=0 );
  sqlite3_mallocDisallowed++;
}





void sqlite3MallocAllow(){
  assert( sqlite3_mallocDisallowed>0 );
  sqlite3_mallocDisallowed--;
}
#endif
