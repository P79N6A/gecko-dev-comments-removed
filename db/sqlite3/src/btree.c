














































































































































































































#include "sqliteInt.h"
#include "pager.h"
#include "btree.h"
#include "os.h"
#include <assert.h>




#define ROUND8(x)   ((x+7)&~7)





#define MX_CELL_SIZE(pBt)  (pBt->pageSize-8)





#define MX_CELL(pBt) ((pBt->pageSize-8)/3)


typedef struct MemPage MemPage;
typedef struct BtLock BtLock;













#ifndef SQLITE_FILE_HEADER 
#  define SQLITE_FILE_HEADER "SQLite format 3"
#endif
static const char zMagicHeader[] = SQLITE_FILE_HEADER;





#define PTF_INTKEY    0x01
#define PTF_ZERODATA  0x02
#define PTF_LEAFDATA  0x04
#define PTF_LEAF      0x08











struct MemPage {
  u8 isInit;           
  u8 idxShift;         
  u8 nOverflow;        
  u8 intKey;           
  u8 leaf;             
  u8 zeroData;         
  u8 leafData;         
  u8 hasData;          
  u8 hdrOffset;        
  u8 childPtrSize;     
  u16 maxLocal;        
  u16 minLocal;        
  u16 cellOffset;      
  u16 idxParent;       
  u16 nFree;           
  u16 nCell;           
  struct _OvflCell {   
    u8 *pCell;          
    u16 idx;            
  } aOvfl[5];
  BtShared *pBt;       
  u8 *aData;           
  Pgno pgno;           
  MemPage *pParent;    
};






#define EXTRA_SIZE sizeof(MemPage)


struct Btree {
  sqlite3 *pSqlite;
  BtShared *pBt;
  u8 inTrans;            
};









#define TRANS_NONE  0
#define TRANS_READ  1
#define TRANS_WRITE 2




struct BtShared {
  Pager *pPager;        
  BtCursor *pCursor;    
  MemPage *pPage1;      
  u8 inStmt;            
  u8 readOnly;          
  u8 maxEmbedFrac;      
  u8 minEmbedFrac;      
  u8 minLeafFrac;       
  u8 pageSizeFixed;     
#ifndef SQLITE_OMIT_AUTOVACUUM
  u8 autoVacuum;        
#endif
  u16 pageSize;         
  u16 usableSize;       
  int maxLocal;         
  int minLocal;         
  int maxLeaf;          
  int minLeaf;          
  BusyHandler *pBusyHandler;   
  u8 inTransaction;     
  int nRef;             
  int nTransaction;     
  void *pSchema;        
  void (*xFreeSchema)(void*);  
#ifndef SQLITE_OMIT_SHARED_CACHE
  BtLock *pLock;        
  BtShared *pNext;      
#endif
};






typedef struct CellInfo CellInfo;
struct CellInfo {
  u8 *pCell;     
  i64 nKey;      
  u32 nData;     
  u16 nHeader;   
  u16 nLocal;    
  u16 iOverflow; 
  u16 nSize;     
};






struct BtCursor {
  Btree *pBtree;            
  BtCursor *pNext, *pPrev;  
  int (*xCompare)(void*,int,const void*,int,const void*); 
  void *pArg;               
  Pgno pgnoRoot;            
  MemPage *pPage;           
  int idx;                  
  CellInfo info;            
  u8 wrFlag;                
  u8 eState;                
#ifndef SQLITE_OMIT_SHARED_CACHE
  void *pKey;      
  i64 nKey;        
  int skip;        
#endif
};





















#define CURSOR_INVALID           0
#define CURSOR_VALID             1
#define CURSOR_REQUIRESEEK       2






#if SQLITE_TEST
# define TRACE(X)   if( sqlite3_btree_trace )\
                        { sqlite3DebugPrintf X; fflush(stdout); }
#else
# define TRACE(X)
#endif
int sqlite3_btree_trace=0;  




static int checkReadLocks(BtShared*,Pgno,BtCursor*);




static u32 get2byte(unsigned char *p){
  return (p[0]<<8) | p[1];
}
static u32 get4byte(unsigned char *p){
  return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}
static void put2byte(unsigned char *p, u32 v){
  p[0] = v>>8;
  p[1] = v;
}
static void put4byte(unsigned char *p, u32 v){
  p[0] = v>>24;
  p[1] = v>>16;
  p[2] = v>>8;
  p[3] = v;
}






#define getVarint    sqlite3GetVarint

#define getVarint32(A,B)  ((*B=*(A))<=0x7f?1:sqlite3GetVarint32(A,B))
#define putVarint    sqlite3PutVarint








#ifdef SQLITE_OMIT_DISKIO
# define PENDING_BYTE_PAGE(pBt)  0x7fffffff
#else
# define PENDING_BYTE_PAGE(pBt) ((PENDING_BYTE/(pBt)->pageSize)+1)
#endif








struct BtLock {
  Btree *pBtree;        
  Pgno iTable;          
  u8 eLock;             
  BtLock *pNext;        
};


#define READ_LOCK     1
#define WRITE_LOCK    2

#ifdef SQLITE_OMIT_SHARED_CACHE
  







  #define queryTableLock(a,b,c) SQLITE_OK
  #define lockTable(a,b,c) SQLITE_OK
  #define unlockAllTables(a)
  #define restoreOrClearCursorPosition(a,b) SQLITE_OK
  #define saveAllCursors(a,b,c) SQLITE_OK

#else

static void releasePage(MemPage *pPage);





static int saveCursorPosition(BtCursor *pCur){
  int rc;

  assert( CURSOR_VALID==pCur->eState );
  assert( 0==pCur->pKey );

  rc = sqlite3BtreeKeySize(pCur, &pCur->nKey);

  





  if( rc==SQLITE_OK && 0==pCur->pPage->intKey){
    void *pKey = sqliteMalloc(pCur->nKey);
    if( pKey ){
      rc = sqlite3BtreeKey(pCur, 0, pCur->nKey, pKey);
      if( rc==SQLITE_OK ){
        pCur->pKey = pKey;
      }else{
        sqliteFree(pKey);
      }
    }else{
      rc = SQLITE_NOMEM;
    }
  }
  assert( !pCur->pPage->intKey || !pCur->pKey );

  if( rc==SQLITE_OK ){
    releasePage(pCur->pPage);
    pCur->pPage = 0;
    pCur->eState = CURSOR_REQUIRESEEK;
  }

  return rc;
}






static int saveAllCursors(BtShared *pBt, Pgno iRoot, BtCursor *pExcept){
  BtCursor *p;
  if( sqlite3ThreadDataReadOnly()->useSharedData ){
    for(p=pBt->pCursor; p; p=p->pNext){
      if( p!=pExcept && (0==iRoot || p->pgnoRoot==iRoot) && 
          p->eState==CURSOR_VALID ){
        int rc = saveCursorPosition(p);
        if( SQLITE_OK!=rc ){
          return rc;
        }
      }
    }
  }
  return SQLITE_OK;
}












static int restoreOrClearCursorPositionX(BtCursor *pCur, int doSeek){
  int rc = SQLITE_OK;
  assert( sqlite3ThreadDataReadOnly()->useSharedData );
  assert( pCur->eState==CURSOR_REQUIRESEEK );
  pCur->eState = CURSOR_INVALID;
  if( doSeek ){
    rc = sqlite3BtreeMoveto(pCur, pCur->pKey, pCur->nKey, &pCur->skip);
  }
  if( rc==SQLITE_OK ){
    sqliteFree(pCur->pKey);
    pCur->pKey = 0;
    assert( CURSOR_VALID==pCur->eState || CURSOR_INVALID==pCur->eState );
  }
  return rc;
}

#define restoreOrClearCursorPosition(p,x) \
  (p->eState==CURSOR_REQUIRESEEK?restoreOrClearCursorPositionX(p,x):SQLITE_OK)







static int queryTableLock(Btree *p, Pgno iTab, u8 eLock){
  BtShared *pBt = p->pBt;
  BtLock *pIter;

  
  if( 0==sqlite3ThreadDataReadOnly()->useSharedData ){
    return SQLITE_OK;
  }

  













  if( 
    !p->pSqlite || 
    0==(p->pSqlite->flags&SQLITE_ReadUncommitted) || 
    eLock==WRITE_LOCK ||
    iTab==MASTER_ROOT
  ){
    for(pIter=pBt->pLock; pIter; pIter=pIter->pNext){
      if( pIter->pBtree!=p && pIter->iTable==iTab && 
          (pIter->eLock!=eLock || eLock!=READ_LOCK) ){
        return SQLITE_LOCKED;
      }
    }
  }
  return SQLITE_OK;
}









static int lockTable(Btree *p, Pgno iTable, u8 eLock){
  BtShared *pBt = p->pBt;
  BtLock *pLock = 0;
  BtLock *pIter;

  
  if( 0==sqlite3ThreadDataReadOnly()->useSharedData ){
    return SQLITE_OK;
  }

  assert( SQLITE_OK==queryTableLock(p, iTable, eLock) );

  




  if( 
    (p->pSqlite) && 
    (p->pSqlite->flags&SQLITE_ReadUncommitted) && 
    (eLock==READ_LOCK) &&
    iTable!=MASTER_ROOT
  ){
    return SQLITE_OK;
  }

  
  for(pIter=pBt->pLock; pIter; pIter=pIter->pNext){
    if( pIter->iTable==iTable && pIter->pBtree==p ){
      pLock = pIter;
      break;
    }
  }

  


  if( !pLock ){
    pLock = (BtLock *)sqliteMalloc(sizeof(BtLock));
    if( !pLock ){
      return SQLITE_NOMEM;
    }
    pLock->iTable = iTable;
    pLock->pBtree = p;
    pLock->pNext = pBt->pLock;
    pBt->pLock = pLock;
  }

  



  assert( WRITE_LOCK>READ_LOCK );
  if( eLock>pLock->eLock ){
    pLock->eLock = eLock;
  }

  return SQLITE_OK;
}





static void unlockAllTables(Btree *p){
  BtLock **ppIter = &p->pBt->pLock;

  



  assert( sqlite3ThreadDataReadOnly()->useSharedData || 0==*ppIter );

  while( *ppIter ){
    BtLock *pLock = *ppIter;
    if( pLock->pBtree==p ){
      *ppIter = pLock->pNext;
      sqliteFree(pLock);
    }else{
      ppIter = &pLock->pNext;
    }
  }
}
#endif 

#ifndef SQLITE_OMIT_AUTOVACUUM















#define PTRMAP_PAGENO(pBt, pgno) ptrmapPageno(pBt, pgno)
#define PTRMAP_PTROFFSET(pBt, pgno) (5*(pgno-ptrmapPageno(pBt, pgno)-1))
#define PTRMAP_ISPAGE(pBt, pgno) (PTRMAP_PAGENO((pBt),(pgno))==(pgno))

static Pgno ptrmapPageno(BtShared *pBt, Pgno pgno){
  int nPagesPerMapPage = (pBt->usableSize/5)+1;
  int iPtrMap = (pgno-2)/nPagesPerMapPage;
  int ret = (iPtrMap*nPagesPerMapPage) + 2; 
  if( ret==PENDING_BYTE_PAGE(pBt) ){
    ret++;
  }
  return ret;
}
































#define PTRMAP_ROOTPAGE 1
#define PTRMAP_FREEPAGE 2
#define PTRMAP_OVERFLOW1 3
#define PTRMAP_OVERFLOW2 4
#define PTRMAP_BTREE 5








static int ptrmapPut(BtShared *pBt, Pgno key, u8 eType, Pgno parent){
  u8 *pPtrmap;    
  Pgno iPtrmap;   
  int offset;     
  int rc;

  
  assert( 0==PTRMAP_ISPAGE(pBt, PENDING_BYTE_PAGE(pBt)) );

  assert( pBt->autoVacuum );
  if( key==0 ){
    return SQLITE_CORRUPT_BKPT;
  }
  iPtrmap = PTRMAP_PAGENO(pBt, key);
  rc = sqlite3pager_get(pBt->pPager, iPtrmap, (void **)&pPtrmap);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  offset = PTRMAP_PTROFFSET(pBt, key);

  if( eType!=pPtrmap[offset] || get4byte(&pPtrmap[offset+1])!=parent ){
    TRACE(("PTRMAP_UPDATE: %d->(%d,%d)\n", key, eType, parent));
    rc = sqlite3pager_write(pPtrmap);
    if( rc==SQLITE_OK ){
      pPtrmap[offset] = eType;
      put4byte(&pPtrmap[offset+1], parent);
    }
  }

  sqlite3pager_unref(pPtrmap);
  return rc;
}








static int ptrmapGet(BtShared *pBt, Pgno key, u8 *pEType, Pgno *pPgno){
  int iPtrmap;       
  u8 *pPtrmap;       
  int offset;        
  int rc;

  iPtrmap = PTRMAP_PAGENO(pBt, key);
  rc = sqlite3pager_get(pBt->pPager, iPtrmap, (void **)&pPtrmap);
  if( rc!=0 ){
    return rc;
  }

  offset = PTRMAP_PTROFFSET(pBt, key);
  assert( pEType!=0 );
  *pEType = pPtrmap[offset];
  if( pPgno ) *pPgno = get4byte(&pPtrmap[offset+1]);

  sqlite3pager_unref(pPtrmap);
  if( *pEType<1 || *pEType>5 ) return SQLITE_CORRUPT_BKPT;
  return SQLITE_OK;
}

#endif 








static u8 *findCell(MemPage *pPage, int iCell){
  u8 *data = pPage->aData;
  assert( iCell>=0 );
  assert( iCell<get2byte(&data[pPage->hdrOffset+3]) );
  return data + get2byte(&data[pPage->cellOffset+2*iCell]);
}





static u8 *findOverflowCell(MemPage *pPage, int iCell){
  int i;
  for(i=pPage->nOverflow-1; i>=0; i--){
    int k;
    struct _OvflCell *pOvfl;
    pOvfl = &pPage->aOvfl[i];
    k = pOvfl->idx;
    if( k<=iCell ){
      if( k==iCell ){
        return pOvfl->pCell;
      }
      iCell--;
    }
  }
  return findCell(pPage, iCell);
}







static void parseCellPtr(
  MemPage *pPage,         
  u8 *pCell,              
  CellInfo *pInfo         
){
  int n;                  
  u32 nPayload;           

  pInfo->pCell = pCell;
  assert( pPage->leaf==0 || pPage->leaf==1 );
  n = pPage->childPtrSize;
  assert( n==4-4*pPage->leaf );
  if( pPage->hasData ){
    n += getVarint32(&pCell[n], &nPayload);
  }else{
    nPayload = 0;
  }
  pInfo->nData = nPayload;
  if( pPage->intKey ){
    n += getVarint(&pCell[n], (u64 *)&pInfo->nKey);
  }else{
    u32 x;
    n += getVarint32(&pCell[n], &x);
    pInfo->nKey = x;
    nPayload += x;
  }
  pInfo->nHeader = n;
  if( nPayload<=pPage->maxLocal ){
    


    int nSize;          
    pInfo->nLocal = nPayload;
    pInfo->iOverflow = 0;
    nSize = nPayload + n;
    if( nSize<4 ){
      nSize = 4;        
    }
    pInfo->nSize = nSize;
  }else{
    








    int minLocal;  
    int maxLocal;  
    int surplus;   

    minLocal = pPage->minLocal;
    maxLocal = pPage->maxLocal;
    surplus = minLocal + (nPayload - minLocal)%(pPage->pBt->usableSize - 4);
    if( surplus <= maxLocal ){
      pInfo->nLocal = surplus;
    }else{
      pInfo->nLocal = minLocal;
    }
    pInfo->iOverflow = pInfo->nLocal + n;
    pInfo->nSize = pInfo->iOverflow + 4;
  }
}
static void parseCell(
  MemPage *pPage,         
  int iCell,              
  CellInfo *pInfo         
){
  parseCellPtr(pPage, findCell(pPage, iCell), pInfo);
}







#ifndef NDEBUG
static int cellSize(MemPage *pPage, int iCell){
  CellInfo info;
  parseCell(pPage, iCell, &info);
  return info.nSize;
}
#endif
static int cellSizePtr(MemPage *pPage, u8 *pCell){
  CellInfo info;
  parseCellPtr(pPage, pCell, &info);
  return info.nSize;
}

#ifndef SQLITE_OMIT_AUTOVACUUM





static int ptrmapPutOvflPtr(MemPage *pPage, u8 *pCell){
  if( pCell ){
    CellInfo info;
    parseCellPtr(pPage, pCell, &info);
    if( (info.nData+(pPage->intKey?0:info.nKey))>info.nLocal ){
      Pgno ovfl = get4byte(&pCell[info.iOverflow]);
      return ptrmapPut(pPage->pBt, ovfl, PTRMAP_OVERFLOW1, pPage->pgno);
    }
  }
  return SQLITE_OK;
}





static int ptrmapPutOvfl(MemPage *pPage, int iCell){
  u8 *pCell;
  pCell = findOverflowCell(pPage, iCell);
  return ptrmapPutOvflPtr(pPage, pCell);
}
#endif









#if defined(BTREE_DEBUG) && !defined(NDEBUG) && 0
static void _pageIntegrity(MemPage *pPage){
  int usableSize;
  u8 *data;
  int i, j, idx, c, pc, hdr, nFree;
  int cellOffset;
  int nCell, cellLimit;
  u8 *used;

  used = sqliteMallocRaw( pPage->pBt->pageSize );
  if( used==0 ) return;
  usableSize = pPage->pBt->usableSize;
  assert( pPage->aData==&((unsigned char*)pPage)[-pPage->pBt->pageSize] );
  hdr = pPage->hdrOffset;
  assert( hdr==(pPage->pgno==1 ? 100 : 0) );
  assert( pPage->pgno==sqlite3pager_pagenumber(pPage->aData) );
  c = pPage->aData[hdr];
  if( pPage->isInit ){
    assert( pPage->leaf == ((c & PTF_LEAF)!=0) );
    assert( pPage->zeroData == ((c & PTF_ZERODATA)!=0) );
    assert( pPage->leafData == ((c & PTF_LEAFDATA)!=0) );
    assert( pPage->intKey == ((c & (PTF_INTKEY|PTF_LEAFDATA))!=0) );
    assert( pPage->hasData ==
             !(pPage->zeroData || (!pPage->leaf && pPage->leafData)) );
    assert( pPage->cellOffset==pPage->hdrOffset+12-4*pPage->leaf );
    assert( pPage->nCell = get2byte(&pPage->aData[hdr+3]) );
  }
  data = pPage->aData;
  memset(used, 0, usableSize);
  for(i=0; i<hdr+10-pPage->leaf*4; i++) used[i] = 1;
  nFree = 0;
  pc = get2byte(&data[hdr+1]);
  while( pc ){
    int size;
    assert( pc>0 && pc<usableSize-4 );
    size = get2byte(&data[pc+2]);
    assert( pc+size<=usableSize );
    nFree += size;
    for(i=pc; i<pc+size; i++){
      assert( used[i]==0 );
      used[i] = 1;
    }
    pc = get2byte(&data[pc]);
  }
  idx = 0;
  nCell = get2byte(&data[hdr+3]);
  cellLimit = get2byte(&data[hdr+5]);
  assert( pPage->isInit==0 
         || pPage->nFree==nFree+data[hdr+7]+cellLimit-(cellOffset+2*nCell) );
  cellOffset = pPage->cellOffset;
  for(i=0; i<nCell; i++){
    int size;
    pc = get2byte(&data[cellOffset+2*i]);
    assert( pc>0 && pc<usableSize-4 );
    size = cellSize(pPage, &data[pc]);
    assert( pc+size<=usableSize );
    for(j=pc; j<pc+size; j++){
      assert( used[j]==0 );
      used[j] = 1;
    }
  }
  for(i=cellOffset+2*nCell; i<cellimit; i++){
    assert( used[i]==0 );
    used[i] = 1;
  }
  nFree = 0;
  for(i=0; i<usableSize; i++){
    assert( used[i]<=1 );
    if( used[i]==0 ) nFree++;
  }
  assert( nFree==data[hdr+7] );
  sqliteFree(used);
}
#define pageIntegrity(X) _pageIntegrity(X)
#else
# define pageIntegrity(X)
#endif




#define btreeIntegrity(p) \
  assert( p->inTrans!=TRANS_NONE || p->pBt->nTransaction<p->pBt->nRef ); \
  assert( p->pBt->nTransaction<=p->pBt->nRef ); \
  assert( p->pBt->inTransaction!=TRANS_NONE || p->pBt->nTransaction==0 ); \
  assert( p->pBt->inTransaction>=p->inTrans ); 







static int defragmentPage(MemPage *pPage){
  int i;                     
  int pc;                    
  int addr;                  
  int hdr;                   
  int size;                  
  int usableSize;            
  int cellOffset;            
  int brk;                   
  int nCell;                 
  unsigned char *data;       
  unsigned char *temp;       

  assert( sqlite3pager_iswriteable(pPage->aData) );
  assert( pPage->pBt!=0 );
  assert( pPage->pBt->usableSize <= SQLITE_MAX_PAGE_SIZE );
  assert( pPage->nOverflow==0 );
  temp = sqliteMalloc( pPage->pBt->pageSize );
  if( temp==0 ) return SQLITE_NOMEM;
  data = pPage->aData;
  hdr = pPage->hdrOffset;
  cellOffset = pPage->cellOffset;
  nCell = pPage->nCell;
  assert( nCell==get2byte(&data[hdr+3]) );
  usableSize = pPage->pBt->usableSize;
  brk = get2byte(&data[hdr+5]);
  memcpy(&temp[brk], &data[brk], usableSize - brk);
  brk = usableSize;
  for(i=0; i<nCell; i++){
    u8 *pAddr;     
    pAddr = &data[cellOffset + i*2];
    pc = get2byte(pAddr);
    assert( pc<pPage->pBt->usableSize );
    size = cellSizePtr(pPage, &temp[pc]);
    brk -= size;
    memcpy(&data[brk], &temp[pc], size);
    put2byte(pAddr, brk);
  }
  assert( brk>=cellOffset+2*nCell );
  put2byte(&data[hdr+5], brk);
  data[hdr+1] = 0;
  data[hdr+2] = 0;
  data[hdr+7] = 0;
  addr = cellOffset+2*nCell;
  memset(&data[addr], 0, brk-addr);
  sqliteFree(temp);
  return SQLITE_OK;
}













static int allocateSpace(MemPage *pPage, int nByte){
  int addr, pc, hdr;
  int size;
  int nFrag;
  int top;
  int nCell;
  int cellOffset;
  unsigned char *data;
  
  data = pPage->aData;
  assert( sqlite3pager_iswriteable(data) );
  assert( pPage->pBt );
  if( nByte<4 ) nByte = 4;
  if( pPage->nFree<nByte || pPage->nOverflow>0 ) return 0;
  pPage->nFree -= nByte;
  hdr = pPage->hdrOffset;

  nFrag = data[hdr+7];
  if( nFrag<60 ){
    

    addr = hdr+1;
    while( (pc = get2byte(&data[addr]))>0 ){
      size = get2byte(&data[pc+2]);
      if( size>=nByte ){
        if( size<nByte+4 ){
          memcpy(&data[addr], &data[pc], 2);
          data[hdr+7] = nFrag + size - nByte;
          return pc;
        }else{
          put2byte(&data[pc+2], size-nByte);
          return pc + size - nByte;
        }
      }
      addr = pc;
    }
  }

  


  top = get2byte(&data[hdr+5]);
  nCell = get2byte(&data[hdr+3]);
  cellOffset = pPage->cellOffset;
  if( nFrag>=60 || cellOffset + 2*nCell > top - nByte ){
    if( defragmentPage(pPage) ) return 0;
    top = get2byte(&data[hdr+5]);
  }
  top -= nByte;
  assert( cellOffset + 2*nCell <= top );
  put2byte(&data[hdr+5], top);
  return top;
}









static void freeSpace(MemPage *pPage, int start, int size){
  int addr, pbegin, hdr;
  unsigned char *data = pPage->aData;

  assert( pPage->pBt!=0 );
  assert( sqlite3pager_iswriteable(data) );
  assert( start>=pPage->hdrOffset+6+(pPage->leaf?0:4) );
  assert( (start + size)<=pPage->pBt->usableSize );
  if( size<4 ) size = 4;

#ifdef SQLITE_SECURE_DELETE
  

  memset(&data[start], 0, size);
#endif

  
  hdr = pPage->hdrOffset;
  addr = hdr + 1;
  while( (pbegin = get2byte(&data[addr]))<start && pbegin>0 ){
    assert( pbegin<=pPage->pBt->usableSize-4 );
    assert( pbegin>addr );
    addr = pbegin;
  }
  assert( pbegin<=pPage->pBt->usableSize-4 );
  assert( pbegin>addr || pbegin==0 );
  put2byte(&data[addr], start);
  put2byte(&data[start], pbegin);
  put2byte(&data[start+2], size);
  pPage->nFree += size;

  
  addr = pPage->hdrOffset + 1;
  while( (pbegin = get2byte(&data[addr]))>0 ){
    int pnext, psize;
    assert( pbegin>addr );
    assert( pbegin<=pPage->pBt->usableSize-4 );
    pnext = get2byte(&data[pbegin]);
    psize = get2byte(&data[pbegin+2]);
    if( pbegin + psize + 3 >= pnext && pnext>0 ){
      int frag = pnext - (pbegin+psize);
      assert( frag<=data[pPage->hdrOffset+7] );
      data[pPage->hdrOffset+7] -= frag;
      put2byte(&data[pbegin], get2byte(&data[pnext]));
      put2byte(&data[pbegin+2], pnext+get2byte(&data[pnext+2])-pbegin);
    }else{
      addr = pbegin;
    }
  }

  
  if( data[hdr+1]==data[hdr+5] && data[hdr+2]==data[hdr+6] ){
    int top;
    pbegin = get2byte(&data[hdr+1]);
    memcpy(&data[hdr+1], &data[pbegin], 2);
    top = get2byte(&data[hdr+5]);
    put2byte(&data[hdr+5], top + get2byte(&data[pbegin+2]));
  }
}





static void decodeFlags(MemPage *pPage, int flagByte){
  BtShared *pBt;     

  assert( pPage->hdrOffset==(pPage->pgno==1 ? 100 : 0) );
  pPage->intKey = (flagByte & (PTF_INTKEY|PTF_LEAFDATA))!=0;
  pPage->zeroData = (flagByte & PTF_ZERODATA)!=0;
  pPage->leaf = (flagByte & PTF_LEAF)!=0;
  pPage->childPtrSize = 4*(pPage->leaf==0);
  pBt = pPage->pBt;
  if( flagByte & PTF_LEAFDATA ){
    pPage->leafData = 1;
    pPage->maxLocal = pBt->maxLeaf;
    pPage->minLocal = pBt->minLeaf;
  }else{
    pPage->leafData = 0;
    pPage->maxLocal = pBt->maxLocal;
    pPage->minLocal = pBt->minLocal;
  }
  pPage->hasData = !(pPage->zeroData || (!pPage->leaf && pPage->leafData));
}














static int initPage(
  MemPage *pPage,        
  MemPage *pParent       
){
  int pc;            
  int hdr;           
  u8 *data;          
  BtShared *pBt;        
  int usableSize;    
  int cellOffset;    
  int nFree;         
  int top;           

  pBt = pPage->pBt;
  assert( pBt!=0 );
  assert( pParent==0 || pParent->pBt==pBt );
  assert( pPage->pgno==sqlite3pager_pagenumber(pPage->aData) );
  assert( pPage->aData == &((unsigned char*)pPage)[-pBt->pageSize] );
  if( pPage->pParent!=pParent && (pPage->pParent!=0 || pPage->isInit) ){
    
    return SQLITE_CORRUPT_BKPT;
  }
  if( pPage->isInit ) return SQLITE_OK;
  if( pPage->pParent==0 && pParent!=0 ){
    pPage->pParent = pParent;
    sqlite3pager_ref(pParent->aData);
  }
  hdr = pPage->hdrOffset;
  data = pPage->aData;
  decodeFlags(pPage, data[hdr]);
  pPage->nOverflow = 0;
  pPage->idxShift = 0;
  usableSize = pBt->usableSize;
  pPage->cellOffset = cellOffset = hdr + 12 - 4*pPage->leaf;
  top = get2byte(&data[hdr+5]);
  pPage->nCell = get2byte(&data[hdr+3]);
  if( pPage->nCell>MX_CELL(pBt) ){
    
    return SQLITE_CORRUPT_BKPT;
  }
  if( pPage->nCell==0 && pParent!=0 && pParent->pgno!=1 ){
    
    return SQLITE_CORRUPT_BKPT;
  }

  
  pc = get2byte(&data[hdr+1]);
  nFree = data[hdr+7] + top - (cellOffset + 2*pPage->nCell);
  while( pc>0 ){
    int next, size;
    if( pc>usableSize-4 ){
      
      return SQLITE_CORRUPT_BKPT; 
    }
    next = get2byte(&data[pc]);
    size = get2byte(&data[pc+2]);
    if( next>0 && next<=pc+size+3 ){
      
      return SQLITE_CORRUPT_BKPT; 
    }
    nFree += size;
    pc = next;
  }
  pPage->nFree = nFree;
  if( nFree>=usableSize ){
    
    return SQLITE_CORRUPT_BKPT; 
  }

  pPage->isInit = 1;
  pageIntegrity(pPage);
  return SQLITE_OK;
}





static void zeroPage(MemPage *pPage, int flags){
  unsigned char *data = pPage->aData;
  BtShared *pBt = pPage->pBt;
  int hdr = pPage->hdrOffset;
  int first;

  assert( sqlite3pager_pagenumber(data)==pPage->pgno );
  assert( &data[pBt->pageSize] == (unsigned char*)pPage );
  assert( sqlite3pager_iswriteable(data) );
  memset(&data[hdr], 0, pBt->usableSize - hdr);
  data[hdr] = flags;
  first = hdr + 8 + 4*((flags&PTF_LEAF)==0);
  memset(&data[hdr+1], 0, 4);
  data[hdr+7] = 0;
  put2byte(&data[hdr+5], pBt->usableSize);
  pPage->nFree = pBt->usableSize - first;
  decodeFlags(pPage, flags);
  pPage->hdrOffset = hdr;
  pPage->cellOffset = first;
  pPage->nOverflow = 0;
  pPage->idxShift = 0;
  pPage->nCell = 0;
  pPage->isInit = 1;
  pageIntegrity(pPage);
}





static int getPage(BtShared *pBt, Pgno pgno, MemPage **ppPage){
  int rc;
  unsigned char *aData;
  MemPage *pPage;
  rc = sqlite3pager_get(pBt->pPager, pgno, (void**)&aData);
  if( rc ) return rc;
  pPage = (MemPage*)&aData[pBt->pageSize];
  pPage->aData = aData;
  pPage->pBt = pBt;
  pPage->pgno = pgno;
  pPage->hdrOffset = pPage->pgno==1 ? 100 : 0;
  *ppPage = pPage;
  return SQLITE_OK;
}






static int getAndInitPage(
  BtShared *pBt,          
  Pgno pgno,           
  MemPage **ppPage,    
  MemPage *pParent     
){
  int rc;
  if( pgno==0 ){
    return SQLITE_CORRUPT_BKPT; 
  }
  rc = getPage(pBt, pgno, ppPage);
  if( rc==SQLITE_OK && (*ppPage)->isInit==0 ){
    rc = initPage(*ppPage, pParent);
  }
  return rc;
}





static void releasePage(MemPage *pPage){
  if( pPage ){
    assert( pPage->aData );
    assert( pPage->pBt );
    assert( &pPage->aData[pPage->pBt->pageSize]==(unsigned char*)pPage );
    sqlite3pager_unref(pPage->aData);
  }
}






static void pageDestructor(void *pData, int pageSize){
  MemPage *pPage;
  assert( (pageSize & 7)==0 );
  pPage = (MemPage*)&((char*)pData)[pageSize];
  if( pPage->pParent ){
    MemPage *pParent = pPage->pParent;
    pPage->pParent = 0;
    releasePage(pParent);
  }
  pPage->isInit = 0;
}









static void pageReinit(void *pData, int pageSize){
  MemPage *pPage;
  assert( (pageSize & 7)==0 );
  pPage = (MemPage*)&((char*)pData)[pageSize];
  if( pPage->isInit ){
    pPage->isInit = 0;
    initPage(pPage, pPage->pParent);
  }
}








int sqlite3BtreeOpen(
  const char *zFilename,  
  sqlite3 *pSqlite,       
  Btree **ppBtree,        
  int flags               
){
  BtShared *pBt;          
  Btree *p;               
  int rc;
  int nReserve;
  unsigned char zDbHeader[100];
#if !defined(SQLITE_OMIT_SHARED_CACHE) && !defined(SQLITE_OMIT_DISKIO)
  const ThreadData *pTsdro;
#endif

  




#if !defined(SQLITE_OMIT_SHARED_CACHE) || !defined(SQLITE_OMIT_AUTOVACUUM)
  #ifdef SQLITE_OMIT_MEMORYDB
  const int isMemdb = !zFilename;
  #else
  const int isMemdb = !zFilename || (strcmp(zFilename, ":memory:")?0:1);
  #endif
#endif

  p = sqliteMalloc(sizeof(Btree));
  if( !p ){
    return SQLITE_NOMEM;
  }
  p->inTrans = TRANS_NONE;
  p->pSqlite = pSqlite;

  
#if !defined(SQLITE_OMIT_SHARED_CACHE) && !defined(SQLITE_OMIT_DISKIO)
  pTsdro = sqlite3ThreadDataReadOnly();
  if( pTsdro->useSharedData && zFilename && !isMemdb ){
    char *zFullPathname = sqlite3OsFullPathname(zFilename);
    if( !zFullPathname ){
      sqliteFree(p);
      return SQLITE_NOMEM;
    }
    for(pBt=pTsdro->pBtree; pBt; pBt=pBt->pNext){
      assert( pBt->nRef>0 );
      if( 0==strcmp(zFullPathname, sqlite3pager_filename(pBt->pPager)) ){
        p->pBt = pBt;
        *ppBtree = p;
        pBt->nRef++;
        sqliteFree(zFullPathname);
        return SQLITE_OK;
      }
    }
    sqliteFree(zFullPathname);
  }
#endif

  




  assert( sizeof(i64)==8 || sizeof(i64)==4 );
  assert( sizeof(u64)==8 || sizeof(u64)==4 );
  assert( sizeof(u32)==4 );
  assert( sizeof(u16)==2 );
  assert( sizeof(Pgno)==4 );

  pBt = sqliteMalloc( sizeof(*pBt) );
  if( pBt==0 ){
    *ppBtree = 0;
    sqliteFree(p);
    return SQLITE_NOMEM;
  }
  rc = sqlite3pager_open(&pBt->pPager, zFilename, EXTRA_SIZE, flags);
  if( rc!=SQLITE_OK ){
    if( pBt->pPager ) sqlite3pager_close(pBt->pPager);
    sqliteFree(pBt);
    sqliteFree(p);
    *ppBtree = 0;
    return rc;
  }
  p->pBt = pBt;

  sqlite3pager_set_destructor(pBt->pPager, pageDestructor);
  sqlite3pager_set_reiniter(pBt->pPager, pageReinit);
  pBt->pCursor = 0;
  pBt->pPage1 = 0;
  pBt->readOnly = sqlite3pager_isreadonly(pBt->pPager);
  sqlite3pager_read_fileheader(pBt->pPager, sizeof(zDbHeader), zDbHeader);
  pBt->pageSize = get2byte(&zDbHeader[16]);
  if( pBt->pageSize<512 || pBt->pageSize>SQLITE_MAX_PAGE_SIZE
       || ((pBt->pageSize-1)&pBt->pageSize)!=0 ){
    pBt->pageSize = SQLITE_DEFAULT_PAGE_SIZE;
    pBt->maxEmbedFrac = 64;   
    pBt->minEmbedFrac = 32;   
    pBt->minLeafFrac = 32;    
#ifndef SQLITE_OMIT_AUTOVACUUM
    





    if( zFilename && !isMemdb ){
      pBt->autoVacuum = SQLITE_DEFAULT_AUTOVACUUM;
    }
#endif
    nReserve = 0;
  }else{
    nReserve = zDbHeader[20];
    pBt->maxEmbedFrac = zDbHeader[21];
    pBt->minEmbedFrac = zDbHeader[22];
    pBt->minLeafFrac = zDbHeader[23];
    pBt->pageSizeFixed = 1;
#ifndef SQLITE_OMIT_AUTOVACUUM
    pBt->autoVacuum = (get4byte(&zDbHeader[36 + 4*4])?1:0);
#endif
  }
  pBt->usableSize = pBt->pageSize - nReserve;
  assert( (pBt->pageSize & 7)==0 );  
  sqlite3pager_set_pagesize(pBt->pPager, pBt->pageSize);

#if !defined(SQLITE_OMIT_SHARED_CACHE) && !defined(SQLITE_OMIT_DISKIO)
  




  if( pTsdro->useSharedData && zFilename && !isMemdb ){
    pBt->pNext = pTsdro->pBtree;
    sqlite3ThreadData()->pBtree = pBt;
  }
#endif
  pBt->nRef = 1;
  *ppBtree = p;
  return SQLITE_OK;
}




int sqlite3BtreeClose(Btree *p){
  BtShared *pBt = p->pBt;
  BtCursor *pCur;

#ifndef SQLITE_OMIT_SHARED_CACHE
  ThreadData *pTsd;
#endif

  
  pCur = pBt->pCursor;
  while( pCur ){
    BtCursor *pTmp = pCur;
    pCur = pCur->pNext;
    if( pTmp->pBtree==p ){
      sqlite3BtreeCloseCursor(pTmp);
    }
  }

  



  sqlite3BtreeRollback(p);
  sqliteFree(p);

#ifndef SQLITE_OMIT_SHARED_CACHE
  



  assert( pBt->nRef>0 );
  pBt->nRef--;
  if( pBt->nRef ){
    return SQLITE_OK;
  }

  



  pTsd = (ThreadData *)sqlite3ThreadDataReadOnly();
  if( pTsd->pBtree==pBt ){
    assert( pTsd==sqlite3ThreadData() );
    pTsd->pBtree = pBt->pNext;
  }else{
    BtShared *pPrev;
    for(pPrev=pTsd->pBtree; pPrev && pPrev->pNext!=pBt; pPrev=pPrev->pNext){}
    if( pPrev ){
      assert( pTsd==sqlite3ThreadData() );
      pPrev->pNext = pBt->pNext;
    }
  }
#endif

  
  assert( !pBt->pCursor );
  sqlite3pager_close(pBt->pPager);
  if( pBt->xFreeSchema && pBt->pSchema ){
    pBt->xFreeSchema(pBt->pSchema);
  }
  sqliteFree(pBt->pSchema);
  sqliteFree(pBt);
  return SQLITE_OK;
}




int sqlite3BtreeSetBusyHandler(Btree *p, BusyHandler *pHandler){
  BtShared *pBt = p->pBt;
  pBt->pBusyHandler = pHandler;
  sqlite3pager_set_busyhandler(pBt->pPager, pHandler);
  return SQLITE_OK;
}
















int sqlite3BtreeSetCacheSize(Btree *p, int mxPage){
  BtShared *pBt = p->pBt;
  sqlite3pager_set_cachesize(pBt->pPager, mxPage);
  return SQLITE_OK;
}









#ifndef SQLITE_OMIT_PAGER_PRAGMAS
int sqlite3BtreeSetSafetyLevel(Btree *p, int level, int fullSync){
  BtShared *pBt = p->pBt;
  sqlite3pager_set_safety_level(pBt->pPager, level, fullSync);
  return SQLITE_OK;
}
#endif





int sqlite3BtreeSyncDisabled(Btree *p){
  BtShared *pBt = p->pBt;
  assert( pBt && pBt->pPager );
  return sqlite3pager_nosync(pBt->pPager);
}

#if !defined(SQLITE_OMIT_PAGER_PRAGMAS) || !defined(SQLITE_OMIT_VACUUM)















int sqlite3BtreeSetPageSize(Btree *p, int pageSize, int nReserve){
  BtShared *pBt = p->pBt;
  if( pBt->pageSizeFixed ){
    return SQLITE_READONLY;
  }
  if( nReserve<0 ){
    nReserve = pBt->pageSize - pBt->usableSize;
  }
  if( pageSize>=512 && pageSize<=SQLITE_MAX_PAGE_SIZE &&
        ((pageSize-1)&pageSize)==0 ){
    assert( (pageSize & 7)==0 );
    assert( !pBt->pPage1 && !pBt->pCursor );
    pBt->pageSize = sqlite3pager_set_pagesize(pBt->pPager, pageSize);
  }
  pBt->usableSize = pBt->pageSize - nReserve;
  return SQLITE_OK;
}




int sqlite3BtreeGetPageSize(Btree *p){
  return p->pBt->pageSize;
}
int sqlite3BtreeGetReserve(Btree *p){
  return p->pBt->pageSize - p->pBt->usableSize;
}
#endif 







int sqlite3BtreeSetAutoVacuum(Btree *p, int autoVacuum){
  BtShared *pBt = p->pBt;;
#ifdef SQLITE_OMIT_AUTOVACUUM
  return SQLITE_READONLY;
#else
  if( pBt->pageSizeFixed ){
    return SQLITE_READONLY;
  }
  pBt->autoVacuum = (autoVacuum?1:0);
  return SQLITE_OK;
#endif
}





int sqlite3BtreeGetAutoVacuum(Btree *p){
#ifdef SQLITE_OMIT_AUTOVACUUM
  return 0;
#else
  return p->pBt->autoVacuum;
#endif
}












static int lockBtree(BtShared *pBt){
  int rc, pageSize;
  MemPage *pPage1;
  if( pBt->pPage1 ) return SQLITE_OK;
  rc = getPage(pBt, 1, &pPage1);
  if( rc!=SQLITE_OK ) return rc;
  

  


  rc = SQLITE_NOTADB;
  if( sqlite3pager_pagecount(pBt->pPager)>0 ){
    u8 *page1 = pPage1->aData;
    if( memcmp(page1, zMagicHeader, 16)!=0 ){
      goto page1_init_failed;
    }
    if( page1[18]>1 || page1[19]>1 ){
      goto page1_init_failed;
    }
    pageSize = get2byte(&page1[16]);
    if( ((pageSize-1)&pageSize)!=0 ){
      goto page1_init_failed;
    }
    assert( (pageSize & 7)==0 );
    pBt->pageSize = pageSize;
    pBt->usableSize = pageSize - page1[20];
    if( pBt->usableSize<500 ){
      goto page1_init_failed;
    }
    pBt->maxEmbedFrac = page1[21];
    pBt->minEmbedFrac = page1[22];
    pBt->minLeafFrac = page1[23];
#ifndef SQLITE_OMIT_AUTOVACUUM
    pBt->autoVacuum = (get4byte(&page1[36 + 4*4])?1:0);
#endif
  }

  












  pBt->maxLocal = (pBt->usableSize-12)*pBt->maxEmbedFrac/255 - 23;
  pBt->minLocal = (pBt->usableSize-12)*pBt->minEmbedFrac/255 - 23;
  pBt->maxLeaf = pBt->usableSize - 35;
  pBt->minLeaf = (pBt->usableSize-12)*pBt->minLeafFrac/255 - 23;
  if( pBt->minLocal>pBt->maxLocal || pBt->maxLocal<0 ){
    goto page1_init_failed;
  }
  assert( pBt->maxLeaf + 23 <= MX_CELL_SIZE(pBt) );
  pBt->pPage1 = pPage1;
  return SQLITE_OK;

page1_init_failed:
  releasePage(pPage1);
  pBt->pPage1 = 0;
  return rc;
}





static int lockBtreeWithRetry(Btree *pRef){
  int rc = SQLITE_OK;
  if( pRef->inTrans==TRANS_NONE ){
    u8 inTransaction = pRef->pBt->inTransaction;
    btreeIntegrity(pRef);
    rc = sqlite3BtreeBeginTrans(pRef, 0);
    pRef->pBt->inTransaction = inTransaction;
    pRef->inTrans = TRANS_NONE;
    if( rc==SQLITE_OK ){
      pRef->pBt->nTransaction--;
    }
    btreeIntegrity(pRef);
  }
  return rc;
}
       











static void unlockBtreeIfUnused(BtShared *pBt){
  if( pBt->inTransaction==TRANS_NONE && pBt->pCursor==0 && pBt->pPage1!=0 ){
    if( pBt->pPage1->aData==0 ){
      MemPage *pPage = pBt->pPage1;
      pPage->aData = &((u8*)pPage)[-pBt->pageSize];
      pPage->pBt = pBt;
      pPage->pgno = 1;
    }
    releasePage(pBt->pPage1);
    pBt->pPage1 = 0;
    pBt->inStmt = 0;
  }
}





static int newDatabase(BtShared *pBt){
  MemPage *pP1;
  unsigned char *data;
  int rc;
  if( sqlite3pager_pagecount(pBt->pPager)>0 ) return SQLITE_OK;
  pP1 = pBt->pPage1;
  assert( pP1!=0 );
  data = pP1->aData;
  rc = sqlite3pager_write(data);
  if( rc ) return rc;
  memcpy(data, zMagicHeader, sizeof(zMagicHeader));
  assert( sizeof(zMagicHeader)==16 );
  put2byte(&data[16], pBt->pageSize);
  data[18] = 1;
  data[19] = 1;
  data[20] = pBt->pageSize - pBt->usableSize;
  data[21] = pBt->maxEmbedFrac;
  data[22] = pBt->minEmbedFrac;
  data[23] = pBt->minLeafFrac;
  memset(&data[24], 0, 100-24);
  zeroPage(pP1, PTF_INTKEY|PTF_LEAF|PTF_LEAFDATA );
  pBt->pageSizeFixed = 1;
#ifndef SQLITE_OMIT_AUTOVACUUM
  if( pBt->autoVacuum ){
    put4byte(&data[36 + 4*4], 1);
  }
#endif
  return SQLITE_OK;
}




































int sqlite3BtreeBeginTrans(Btree *p, int wrflag){
  BtShared *pBt = p->pBt;
  int rc = SQLITE_OK;

  btreeIntegrity(p);

  



  if( p->inTrans==TRANS_WRITE || (p->inTrans==TRANS_READ && !wrflag) ){
    return SQLITE_OK;
  }

  
  if( pBt->readOnly && wrflag ){
    return SQLITE_READONLY;
  }

  



  if( pBt->inTransaction==TRANS_WRITE && wrflag ){
    return SQLITE_BUSY;
  }

  do {
    if( pBt->pPage1==0 ){
      rc = lockBtree(pBt);
    }
  
    if( rc==SQLITE_OK && wrflag ){
      rc = sqlite3pager_begin(pBt->pPage1->aData, wrflag>1);
      if( rc==SQLITE_OK ){
        rc = newDatabase(pBt);
      }
    }
  
    if( rc==SQLITE_OK ){
      if( wrflag ) pBt->inStmt = 0;
    }else{
      unlockBtreeIfUnused(pBt);
    }
  }while( rc==SQLITE_BUSY && pBt->inTransaction==TRANS_NONE &&
          sqlite3InvokeBusyHandler(pBt->pBusyHandler) );

  if( rc==SQLITE_OK ){
    if( p->inTrans==TRANS_NONE ){
      pBt->nTransaction++;
    }
    p->inTrans = (wrflag?TRANS_WRITE:TRANS_READ);
    if( p->inTrans>pBt->inTransaction ){
      pBt->inTransaction = p->inTrans;
    }
  }

  btreeIntegrity(p);
  return rc;
}

#ifndef SQLITE_OMIT_AUTOVACUUM






static int setChildPtrmaps(MemPage *pPage){
  int i;                             
  int nCell;                         
  int rc = SQLITE_OK;                
  BtShared *pBt = pPage->pBt;
  int isInitOrig = pPage->isInit;
  Pgno pgno = pPage->pgno;

  initPage(pPage, 0);
  nCell = pPage->nCell;

  for(i=0; i<nCell; i++){
    u8 *pCell = findCell(pPage, i);

    rc = ptrmapPutOvflPtr(pPage, pCell);
    if( rc!=SQLITE_OK ){
      goto set_child_ptrmaps_out;
    }

    if( !pPage->leaf ){
      Pgno childPgno = get4byte(pCell);
      rc = ptrmapPut(pBt, childPgno, PTRMAP_BTREE, pgno);
      if( rc!=SQLITE_OK ) goto set_child_ptrmaps_out;
    }
  }

  if( !pPage->leaf ){
    Pgno childPgno = get4byte(&pPage->aData[pPage->hdrOffset+8]);
    rc = ptrmapPut(pBt, childPgno, PTRMAP_BTREE, pgno);
  }

set_child_ptrmaps_out:
  pPage->isInit = isInitOrig;
  return rc;
}
















static int modifyPagePointer(MemPage *pPage, Pgno iFrom, Pgno iTo, u8 eType){
  if( eType==PTRMAP_OVERFLOW2 ){
    
    if( get4byte(pPage->aData)!=iFrom ){
      return SQLITE_CORRUPT_BKPT;
    }
    put4byte(pPage->aData, iTo);
  }else{
    int isInitOrig = pPage->isInit;
    int i;
    int nCell;

    initPage(pPage, 0);
    nCell = pPage->nCell;

    for(i=0; i<nCell; i++){
      u8 *pCell = findCell(pPage, i);
      if( eType==PTRMAP_OVERFLOW1 ){
        CellInfo info;
        parseCellPtr(pPage, pCell, &info);
        if( info.iOverflow ){
          if( iFrom==get4byte(&pCell[info.iOverflow]) ){
            put4byte(&pCell[info.iOverflow], iTo);
            break;
          }
        }
      }else{
        if( get4byte(pCell)==iFrom ){
          put4byte(pCell, iTo);
          break;
        }
      }
    }
  
    if( i==nCell ){
      if( eType!=PTRMAP_BTREE || 
          get4byte(&pPage->aData[pPage->hdrOffset+8])!=iFrom ){
        return SQLITE_CORRUPT_BKPT;
      }
      put4byte(&pPage->aData[pPage->hdrOffset+8], iTo);
    }

    pPage->isInit = isInitOrig;
  }
  return SQLITE_OK;
}






static int relocatePage(
  BtShared *pBt,           
  MemPage *pDbPage,        
  u8 eType,                
  Pgno iPtrPage,           
  Pgno iFreePage           
){
  MemPage *pPtrPage;   
  Pgno iDbPage = pDbPage->pgno;
  Pager *pPager = pBt->pPager;
  int rc;

  assert( eType==PTRMAP_OVERFLOW2 || eType==PTRMAP_OVERFLOW1 || 
      eType==PTRMAP_BTREE || eType==PTRMAP_ROOTPAGE );

  
  TRACE(("AUTOVACUUM: Moving %d to free page %d (ptr page %d type %d)\n", 
      iDbPage, iFreePage, iPtrPage, eType));
  rc = sqlite3pager_movepage(pPager, pDbPage->aData, iFreePage);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  pDbPage->pgno = iFreePage;

  







  if( eType==PTRMAP_BTREE || eType==PTRMAP_ROOTPAGE ){
    rc = setChildPtrmaps(pDbPage);
    if( rc!=SQLITE_OK ){
      return rc;
    }
  }else{
    Pgno nextOvfl = get4byte(pDbPage->aData);
    if( nextOvfl!=0 ){
      rc = ptrmapPut(pBt, nextOvfl, PTRMAP_OVERFLOW2, iFreePage);
      if( rc!=SQLITE_OK ){
        return rc;
      }
    }
  }

  



  if( eType!=PTRMAP_ROOTPAGE ){
    rc = getPage(pBt, iPtrPage, &pPtrPage);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    rc = sqlite3pager_write(pPtrPage->aData);
    if( rc!=SQLITE_OK ){
      releasePage(pPtrPage);
      return rc;
    }
    rc = modifyPagePointer(pPtrPage, iDbPage, iFreePage, eType);
    releasePage(pPtrPage);
    if( rc==SQLITE_OK ){
      rc = ptrmapPut(pBt, iFreePage, eType, iPtrPage);
    }
  }
  return rc;
}


static int allocatePage(BtShared *, MemPage **, Pgno *, Pgno, u8);





static int autoVacuumCommit(BtShared *pBt, Pgno *nTrunc){
  Pager *pPager = pBt->pPager;
  Pgno nFreeList;            
  int nPtrMap;               
  Pgno origSize;             
  Pgno finSize;              
  int rc;                    
  u8 eType;
  int pgsz = pBt->pageSize;  
  Pgno iDbPage;              
  MemPage *pDbMemPage = 0;   
  Pgno iPtrPage;             
  Pgno iFreePage;            
  MemPage *pFreeMemPage = 0; 

#ifndef NDEBUG
  int nRef = *sqlite3pager_stats(pPager);
#endif

  assert( pBt->autoVacuum );
  if( PTRMAP_ISPAGE(pBt, sqlite3pager_pagecount(pPager)) ){
    return SQLITE_CORRUPT_BKPT;
  }

  


  nFreeList = get4byte(&pBt->pPage1->aData[36]);
  if( nFreeList==0 ){
    *nTrunc = 0;
    return SQLITE_OK;
  }

  








  origSize = sqlite3pager_pagecount(pPager);
  if( origSize==PENDING_BYTE_PAGE(pBt) ){
    origSize--;
  }
  nPtrMap = (nFreeList-origSize+PTRMAP_PAGENO(pBt, origSize)+pgsz/5)/(pgsz/5);
  finSize = origSize - nFreeList - nPtrMap;
  if( origSize>PENDING_BYTE_PAGE(pBt) && finSize<=PENDING_BYTE_PAGE(pBt) ){
    finSize--;
  }
  while( PTRMAP_ISPAGE(pBt, finSize) || finSize==PENDING_BYTE_PAGE(pBt) ){
    finSize--;
  }
  TRACE(("AUTOVACUUM: Begin (db size %d->%d)\n", origSize, finSize));

  





  for( iDbPage=finSize+1; iDbPage<=origSize; iDbPage++ ){
    
    if( PTRMAP_ISPAGE(pBt, iDbPage) || iDbPage==PENDING_BYTE_PAGE(pBt) ){
      continue;
    }

    rc = ptrmapGet(pBt, iDbPage, &eType, &iPtrPage);
    if( rc!=SQLITE_OK ) goto autovacuum_out;
    if( eType==PTRMAP_ROOTPAGE ){
      rc = SQLITE_CORRUPT_BKPT;
      goto autovacuum_out;
    }

    
    if( eType==PTRMAP_FREEPAGE ){
      continue;
    }
    rc = getPage(pBt, iDbPage, &pDbMemPage);
    if( rc!=SQLITE_OK ) goto autovacuum_out;

    



    do{
      if( pFreeMemPage ){
        releasePage(pFreeMemPage);
        pFreeMemPage = 0;
      }
      rc = allocatePage(pBt, &pFreeMemPage, &iFreePage, 0, 0);
      if( rc!=SQLITE_OK ){
        releasePage(pDbMemPage);
        goto autovacuum_out;
      }
      assert( iFreePage<=origSize );
    }while( iFreePage>finSize );
    releasePage(pFreeMemPage);
    pFreeMemPage = 0;

    





    rc = relocatePage(pBt, pDbMemPage, eType, iPtrPage, iFreePage);
    releasePage(pDbMemPage);
    if( rc!=SQLITE_OK ) goto autovacuum_out;
  }

  



  rc = sqlite3pager_write(pBt->pPage1->aData);
  if( rc!=SQLITE_OK ) goto autovacuum_out;
  put4byte(&pBt->pPage1->aData[32], 0);
  put4byte(&pBt->pPage1->aData[36], 0);
  *nTrunc = finSize;
  assert( finSize!=PENDING_BYTE_PAGE(pBt) );

autovacuum_out:
  assert( nRef==*sqlite3pager_stats(pPager) );
  if( rc!=SQLITE_OK ){
    sqlite3pager_rollback(pPager);
  }
  return rc;
}
#endif







int sqlite3BtreeCommit(Btree *p){
  BtShared *pBt = p->pBt;

  btreeIntegrity(p);

  


  if( p->inTrans==TRANS_WRITE ){
    int rc;
    assert( pBt->inTransaction==TRANS_WRITE );
    assert( pBt->nTransaction>0 );
    rc = sqlite3pager_commit(pBt->pPager);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    pBt->inTransaction = TRANS_READ;
    pBt->inStmt = 0;
  }
  unlockAllTables(p);

  




  if( p->inTrans!=TRANS_NONE ){
    pBt->nTransaction--;
    if( 0==pBt->nTransaction ){
      pBt->inTransaction = TRANS_NONE;
    }
  }

  


  p->inTrans = TRANS_NONE;
  unlockBtreeIfUnused(pBt);

  btreeIntegrity(p);
  return SQLITE_OK;
}

#ifndef NDEBUG





static int countWriteCursors(BtShared *pBt){
  BtCursor *pCur;
  int r = 0;
  for(pCur=pBt->pCursor; pCur; pCur=pCur->pNext){
    if( pCur->wrFlag ) r++; 
  }
  return r;
}
#endif

#if defined(SQLITE_TEST) && defined(SQLITE_DEBUG)



void sqlite3BtreeCursorList(Btree *p){
  BtCursor *pCur;
  BtShared *pBt = p->pBt;
  for(pCur=pBt->pCursor; pCur; pCur=pCur->pNext){
    MemPage *pPage = pCur->pPage;
    char *zMode = pCur->wrFlag ? "rw" : "ro";
    sqlite3DebugPrintf("CURSOR %p rooted at %4d(%s) currently at %d.%d%s\n",
       pCur, pCur->pgnoRoot, zMode,
       pPage ? pPage->pgno : 0, pCur->idx,
       (pCur->eState==CURSOR_VALID) ? "" : " eof"
    );
  }
}
#endif










int sqlite3BtreeRollback(Btree *p){
  int rc;
  BtShared *pBt = p->pBt;
  MemPage *pPage1;

  rc = saveAllCursors(pBt, 0, 0);
#ifndef SQLITE_OMIT_SHARED_CACHE
  if( rc!=SQLITE_OK ){
    






    while( pBt->pCursor ){
      sqlite3 *db = pBt->pCursor->pBtree->pSqlite;
      if( db ){
        sqlite3AbortOtherActiveVdbes(db, 0);
      }
    }
  }
#endif
  btreeIntegrity(p);
  unlockAllTables(p);

  if( p->inTrans==TRANS_WRITE ){
    int rc2;

    assert( TRANS_WRITE==pBt->inTransaction );
    rc2 = sqlite3pager_rollback(pBt->pPager);
    if( rc2!=SQLITE_OK ){
      rc = rc2;
    }

    


    if( getPage(pBt, 1, &pPage1)==SQLITE_OK ){
      releasePage(pPage1);
    }
    assert( countWriteCursors(pBt)==0 );
    pBt->inTransaction = TRANS_READ;
  }

  if( p->inTrans!=TRANS_NONE ){
    assert( pBt->nTransaction>0 );
    pBt->nTransaction--;
    if( 0==pBt->nTransaction ){
      pBt->inTransaction = TRANS_NONE;
    }
  }

  p->inTrans = TRANS_NONE;
  pBt->inStmt = 0;
  unlockBtreeIfUnused(pBt);

  btreeIntegrity(p);
  return rc;
}
















int sqlite3BtreeBeginStmt(Btree *p){
  int rc;
  BtShared *pBt = p->pBt;
  if( (p->inTrans!=TRANS_WRITE) || pBt->inStmt ){
    return pBt->readOnly ? SQLITE_READONLY : SQLITE_ERROR;
  }
  assert( pBt->inTransaction==TRANS_WRITE );
  rc = pBt->readOnly ? SQLITE_OK : sqlite3pager_stmt_begin(pBt->pPager);
  pBt->inStmt = 1;
  return rc;
}






int sqlite3BtreeCommitStmt(Btree *p){
  int rc;
  BtShared *pBt = p->pBt;
  if( pBt->inStmt && !pBt->readOnly ){
    rc = sqlite3pager_stmt_commit(pBt->pPager);
  }else{
    rc = SQLITE_OK;
  }
  pBt->inStmt = 0;
  return rc;
}









int sqlite3BtreeRollbackStmt(Btree *p){
  int rc = SQLITE_OK;
  BtShared *pBt = p->pBt;
  sqlite3MallocDisallow();
  if( pBt->inStmt && !pBt->readOnly ){
    rc = sqlite3pager_stmt_rollback(pBt->pPager);
    assert( countWriteCursors(pBt)==0 );
    pBt->inStmt = 0;
  }
  sqlite3MallocAllow();
  return rc;
}





static int dfltCompare(
  void *NotUsed,             
  int n1, const void *p1,    
  int n2, const void *p2     
){
  int c;
  c = memcmp(p1, p2, n1<n2 ? n1 : n2);
  if( c==0 ){
    c = n1 - n2;
  }
  return c;
}











































int sqlite3BtreeCursor(
  Btree *p,                                   
  int iTable,                                 
  int wrFlag,                                 
  int (*xCmp)(void*,int,const void*,int,const void*), 
  void *pArg,                                 
  BtCursor **ppCur                            
){
  int rc;
  BtCursor *pCur;
  BtShared *pBt = p->pBt;

  *ppCur = 0;
  if( wrFlag ){
    if( pBt->readOnly ){
      return SQLITE_READONLY;
    }
    if( checkReadLocks(pBt, iTable, 0) ){
      return SQLITE_LOCKED;
    }
  }

  if( pBt->pPage1==0 ){
    rc = lockBtreeWithRetry(p);
    if( rc!=SQLITE_OK ){
      return rc;
    }
  }
  pCur = sqliteMalloc( sizeof(*pCur) );
  if( pCur==0 ){
    rc = SQLITE_NOMEM;
    goto create_cursor_exception;
  }
  pCur->pgnoRoot = (Pgno)iTable;
  if( iTable==1 && sqlite3pager_pagecount(pBt->pPager)==0 ){
    rc = SQLITE_EMPTY;
    goto create_cursor_exception;
  }
  rc = getAndInitPage(pBt, pCur->pgnoRoot, &pCur->pPage, 0);
  if( rc!=SQLITE_OK ){
    goto create_cursor_exception;
  }

  



  pCur->xCompare = xCmp ? xCmp : dfltCompare;
  pCur->pArg = pArg;
  pCur->pBtree = p;
  pCur->wrFlag = wrFlag;
  pCur->pNext = pBt->pCursor;
  if( pCur->pNext ){
    pCur->pNext->pPrev = pCur;
  }
  pBt->pCursor = pCur;
  pCur->eState = CURSOR_INVALID;
  *ppCur = pCur;

  return SQLITE_OK;
create_cursor_exception:
  if( pCur ){
    releasePage(pCur->pPage);
    sqliteFree(pCur);
  }
  unlockBtreeIfUnused(pBt);
  return rc;
}

#if 0  



void sqlite3BtreeSetCompare(
  BtCursor *pCur,     
  int(*xCmp)(void*,int,const void*,int,const void*), 
  void *pArg          
){
  pCur->xCompare = xCmp ? xCmp : dfltCompare;
  pCur->pArg = pArg;
}
#endif





int sqlite3BtreeCloseCursor(BtCursor *pCur){
  BtShared *pBt = pCur->pBtree->pBt;
  restoreOrClearCursorPosition(pCur, 0);
  if( pCur->pPrev ){
    pCur->pPrev->pNext = pCur->pNext;
  }else{
    pBt->pCursor = pCur->pNext;
  }
  if( pCur->pNext ){
    pCur->pNext->pPrev = pCur->pPrev;
  }
  releasePage(pCur->pPage);
  unlockBtreeIfUnused(pBt);
  sqliteFree(pCur);
  return SQLITE_OK;
}





static void getTempCursor(BtCursor *pCur, BtCursor *pTempCur){
  memcpy(pTempCur, pCur, sizeof(*pCur));
  pTempCur->pNext = 0;
  pTempCur->pPrev = 0;
  if( pTempCur->pPage ){
    sqlite3pager_ref(pTempCur->pPage->aData);
  }
}





static void releaseTempCursor(BtCursor *pCur){
  if( pCur->pPage ){
    sqlite3pager_unref(pCur->pPage->aData);
  }
}








static void getCellInfo(BtCursor *pCur){
  if( pCur->info.nSize==0 ){
    parseCell(pCur->pPage, pCur->idx, &pCur->info);
  }else{
#ifndef NDEBUG
    CellInfo info;
    memset(&info, 0, sizeof(info));
    parseCell(pCur->pPage, pCur->idx, &info);
    assert( memcmp(&info, &pCur->info, sizeof(info))==0 );
#endif
  }
}









int sqlite3BtreeKeySize(BtCursor *pCur, i64 *pSize){
  int rc = restoreOrClearCursorPosition(pCur, 1);
  if( rc==SQLITE_OK ){
    assert( pCur->eState==CURSOR_INVALID || pCur->eState==CURSOR_VALID );
    if( pCur->eState==CURSOR_INVALID ){
      *pSize = 0;
    }else{
      getCellInfo(pCur);
      *pSize = pCur->info.nKey;
    }
  }
  return rc;
}








int sqlite3BtreeDataSize(BtCursor *pCur, u32 *pSize){
  int rc = restoreOrClearCursorPosition(pCur, 1);
  if( rc==SQLITE_OK ){
    assert( pCur->eState==CURSOR_INVALID || pCur->eState==CURSOR_VALID );
    if( pCur->eState==CURSOR_INVALID ){
      
      *pSize = 0;
    }else{
      getCellInfo(pCur);
      *pSize = pCur->info.nData;
    }
  }
  return rc;
}










static int getPayload(
  BtCursor *pCur,      
  int offset,          
  int amt,             
  unsigned char *pBuf,  
  int skipKey          
){
  unsigned char *aPayload;
  Pgno nextPage;
  int rc;
  MemPage *pPage;
  BtShared *pBt;
  int ovflSize;
  u32 nKey;

  assert( pCur!=0 && pCur->pPage!=0 );
  assert( pCur->eState==CURSOR_VALID );
  pBt = pCur->pBtree->pBt;
  pPage = pCur->pPage;
  pageIntegrity(pPage);
  assert( pCur->idx>=0 && pCur->idx<pPage->nCell );
  getCellInfo(pCur);
  aPayload = pCur->info.pCell + pCur->info.nHeader;
  if( pPage->intKey ){
    nKey = 0;
  }else{
    nKey = pCur->info.nKey;
  }
  assert( offset>=0 );
  if( skipKey ){
    offset += nKey;
  }
  if( offset+amt > nKey+pCur->info.nData ){
    return SQLITE_ERROR;
  }
  if( offset<pCur->info.nLocal ){
    int a = amt;
    if( a+offset>pCur->info.nLocal ){
      a = pCur->info.nLocal - offset;
    }
    memcpy(pBuf, &aPayload[offset], a);
    if( a==amt ){
      return SQLITE_OK;
    }
    offset = 0;
    pBuf += a;
    amt -= a;
  }else{
    offset -= pCur->info.nLocal;
  }
  ovflSize = pBt->usableSize - 4;
  if( amt>0 ){
    nextPage = get4byte(&aPayload[pCur->info.nLocal]);
    while( amt>0 && nextPage ){
      rc = sqlite3pager_get(pBt->pPager, nextPage, (void**)&aPayload);
      if( rc!=0 ){
        return rc;
      }
      nextPage = get4byte(aPayload);
      if( offset<ovflSize ){
        int a = amt;
        if( a + offset > ovflSize ){
          a = ovflSize - offset;
        }
        memcpy(pBuf, &aPayload[offset+4], a);
        offset = 0;
        amt -= a;
        pBuf += a;
      }else{
        offset -= ovflSize;
      }
      sqlite3pager_unref(aPayload);
    }
  }

  if( amt>0 ){
    return SQLITE_CORRUPT_BKPT;
  }
  return SQLITE_OK;
}










int sqlite3BtreeKey(BtCursor *pCur, u32 offset, u32 amt, void *pBuf){
  int rc = restoreOrClearCursorPosition(pCur, 1);
  if( rc==SQLITE_OK ){
    assert( pCur->eState==CURSOR_VALID );
    assert( pCur->pPage!=0 );
    if( pCur->pPage->intKey ){
      return SQLITE_CORRUPT_BKPT;
    }
    assert( pCur->pPage->intKey==0 );
    assert( pCur->idx>=0 && pCur->idx<pCur->pPage->nCell );
    rc = getPayload(pCur, offset, amt, (unsigned char*)pBuf, 0);
  }
  return rc;
}










int sqlite3BtreeData(BtCursor *pCur, u32 offset, u32 amt, void *pBuf){
  int rc = restoreOrClearCursorPosition(pCur, 1);
  if( rc==SQLITE_OK ){
    assert( pCur->eState==CURSOR_VALID );
    assert( pCur->pPage!=0 );
    assert( pCur->idx>=0 && pCur->idx<pCur->pPage->nCell );
    rc = getPayload(pCur, offset, amt, pBuf, 1);
  }
  return rc;
}




















static const unsigned char *fetchPayload(
  BtCursor *pCur,      
  int *pAmt,           
  int skipKey          
){
  unsigned char *aPayload;
  MemPage *pPage;
  u32 nKey;
  int nLocal;

  assert( pCur!=0 && pCur->pPage!=0 );
  assert( pCur->eState==CURSOR_VALID );
  pPage = pCur->pPage;
  pageIntegrity(pPage);
  assert( pCur->idx>=0 && pCur->idx<pPage->nCell );
  getCellInfo(pCur);
  aPayload = pCur->info.pCell;
  aPayload += pCur->info.nHeader;
  if( pPage->intKey ){
    nKey = 0;
  }else{
    nKey = pCur->info.nKey;
  }
  if( skipKey ){
    aPayload += nKey;
    nLocal = pCur->info.nLocal - nKey;
  }else{
    nLocal = pCur->info.nLocal;
    if( nLocal>nKey ){
      nLocal = nKey;
    }
  }
  *pAmt = nLocal;
  return aPayload;
}













const void *sqlite3BtreeKeyFetch(BtCursor *pCur, int *pAmt){
  if( pCur->eState==CURSOR_VALID ){
    return (const void*)fetchPayload(pCur, pAmt, 0);
  }
  return 0;
}
const void *sqlite3BtreeDataFetch(BtCursor *pCur, int *pAmt){
  if( pCur->eState==CURSOR_VALID ){
    return (const void*)fetchPayload(pCur, pAmt, 1);
  }
  return 0;
}






static int moveToChild(BtCursor *pCur, u32 newPgno){
  int rc;
  MemPage *pNewPage;
  MemPage *pOldPage;
  BtShared *pBt = pCur->pBtree->pBt;

  assert( pCur->eState==CURSOR_VALID );
  rc = getAndInitPage(pBt, newPgno, &pNewPage, pCur->pPage);
  if( rc ) return rc;
  pageIntegrity(pNewPage);
  pNewPage->idxParent = pCur->idx;
  pOldPage = pCur->pPage;
  pOldPage->idxShift = 0;
  releasePage(pOldPage);
  pCur->pPage = pNewPage;
  pCur->idx = 0;
  pCur->info.nSize = 0;
  if( pNewPage->nCell<1 ){
    return SQLITE_CORRUPT_BKPT;
  }
  return SQLITE_OK;
}










static int isRootPage(MemPage *pPage){
  MemPage *pParent = pPage->pParent;
  if( pParent==0 ) return 1;
  if( pParent->pgno>1 ) return 0;
  if( get2byte(&pParent->aData[pParent->hdrOffset+3])==0 ) return 1;
  return 0;
}









static void moveToParent(BtCursor *pCur){
  MemPage *pParent;
  MemPage *pPage;
  int idxParent;

  assert( pCur->eState==CURSOR_VALID );
  pPage = pCur->pPage;
  assert( pPage!=0 );
  assert( !isRootPage(pPage) );
  pageIntegrity(pPage);
  pParent = pPage->pParent;
  assert( pParent!=0 );
  pageIntegrity(pParent);
  idxParent = pPage->idxParent;
  sqlite3pager_ref(pParent->aData);
  releasePage(pPage);
  pCur->pPage = pParent;
  pCur->info.nSize = 0;
  assert( pParent->idxShift==0 );
  pCur->idx = idxParent;
}




static int moveToRoot(BtCursor *pCur){
  MemPage *pRoot;
  int rc = SQLITE_OK;
  BtShared *pBt = pCur->pBtree->pBt;

  restoreOrClearCursorPosition(pCur, 0);
  pRoot = pCur->pPage;
  if( pRoot && pRoot->pgno==pCur->pgnoRoot ){
    assert( pRoot->isInit );
  }else{
    if( 
      SQLITE_OK!=(rc = getAndInitPage(pBt, pCur->pgnoRoot, &pRoot, 0))
    ){
      pCur->eState = CURSOR_INVALID;
      return rc;
    }
    releasePage(pCur->pPage);
    pageIntegrity(pRoot);
    pCur->pPage = pRoot;
  }
  pCur->idx = 0;
  pCur->info.nSize = 0;
  if( pRoot->nCell==0 && !pRoot->leaf ){
    Pgno subpage;
    assert( pRoot->pgno==1 );
    subpage = get4byte(&pRoot->aData[pRoot->hdrOffset+8]);
    assert( subpage>0 );
    pCur->eState = CURSOR_VALID;
    rc = moveToChild(pCur, subpage);
  }
  pCur->eState = ((pCur->pPage->nCell>0)?CURSOR_VALID:CURSOR_INVALID);
  return rc;
}








static int moveToLeftmost(BtCursor *pCur){
  Pgno pgno;
  int rc;
  MemPage *pPage;

  assert( pCur->eState==CURSOR_VALID );
  while( !(pPage = pCur->pPage)->leaf ){
    assert( pCur->idx>=0 && pCur->idx<pPage->nCell );
    pgno = get4byte(findCell(pPage, pCur->idx));
    rc = moveToChild(pCur, pgno);
    if( rc ) return rc;
  }
  return SQLITE_OK;
}











static int moveToRightmost(BtCursor *pCur){
  Pgno pgno;
  int rc;
  MemPage *pPage;

  assert( pCur->eState==CURSOR_VALID );
  while( !(pPage = pCur->pPage)->leaf ){
    pgno = get4byte(&pPage->aData[pPage->hdrOffset+8]);
    pCur->idx = pPage->nCell;
    rc = moveToChild(pCur, pgno);
    if( rc ) return rc;
  }
  pCur->idx = pPage->nCell - 1;
  pCur->info.nSize = 0;
  return SQLITE_OK;
}





int sqlite3BtreeFirst(BtCursor *pCur, int *pRes){
  int rc;
  rc = moveToRoot(pCur);
  if( rc ) return rc;
  if( pCur->eState==CURSOR_INVALID ){
    assert( pCur->pPage->nCell==0 );
    *pRes = 1;
    return SQLITE_OK;
  }
  assert( pCur->pPage->nCell>0 );
  *pRes = 0;
  rc = moveToLeftmost(pCur);
  return rc;
}





int sqlite3BtreeLast(BtCursor *pCur, int *pRes){
  int rc;
  rc = moveToRoot(pCur);
  if( rc ) return rc;
  if( CURSOR_INVALID==pCur->eState ){
    assert( pCur->pPage->nCell==0 );
    *pRes = 1;
    return SQLITE_OK;
  }
  assert( pCur->eState==CURSOR_VALID );
  *pRes = 0;
  rc = moveToRightmost(pCur);
  return rc;
}




























int sqlite3BtreeMoveto(BtCursor *pCur, const void *pKey, i64 nKey, int *pRes){
  int rc;
  int tryRightmost;
  rc = moveToRoot(pCur);
  if( rc ) return rc;
  assert( pCur->pPage );
  assert( pCur->pPage->isInit );
  tryRightmost = pCur->pPage->intKey;
  if( pCur->eState==CURSOR_INVALID ){
    *pRes = -1;
    assert( pCur->pPage->nCell==0 );
    return SQLITE_OK;
  }
   for(;;){
    int lwr, upr;
    Pgno chldPg;
    MemPage *pPage = pCur->pPage;
    int c = -1;  
    lwr = 0;
    upr = pPage->nCell-1;
    if( !pPage->intKey && pKey==0 ){
      return SQLITE_CORRUPT_BKPT;
    }
    pageIntegrity(pPage);
    while( lwr<=upr ){
      void *pCellKey;
      i64 nCellKey;
      pCur->idx = (lwr+upr)/2;
      pCur->info.nSize = 0;
      if( pPage->intKey ){
        u8 *pCell;
        if( tryRightmost ){
          pCur->idx = upr;
        }
        pCell = findCell(pPage, pCur->idx) + pPage->childPtrSize;
        if( pPage->hasData ){
          u32 dummy;
          pCell += getVarint32(pCell, &dummy);
        }
        getVarint(pCell, (u64 *)&nCellKey);
        if( nCellKey<nKey ){
          c = -1;
        }else if( nCellKey>nKey ){
          c = +1;
          tryRightmost = 0;
        }else{
          c = 0;
        }
      }else{
        int available;
        pCellKey = (void *)fetchPayload(pCur, &available, 0);
        nCellKey = pCur->info.nKey;
        if( available>=nCellKey ){
          c = pCur->xCompare(pCur->pArg, nCellKey, pCellKey, nKey, pKey);
        }else{
          pCellKey = sqliteMallocRaw( nCellKey );
          if( pCellKey==0 ) return SQLITE_NOMEM;
          rc = sqlite3BtreeKey(pCur, 0, nCellKey, (void *)pCellKey);
          c = pCur->xCompare(pCur->pArg, nCellKey, pCellKey, nKey, pKey);
          sqliteFree(pCellKey);
          if( rc ) return rc;
        }
      }
      if( c==0 ){
        if( pPage->leafData && !pPage->leaf ){
          lwr = pCur->idx;
          upr = lwr - 1;
          break;
        }else{
          if( pRes ) *pRes = 0;
          return SQLITE_OK;
        }
      }
      if( c<0 ){
        lwr = pCur->idx+1;
      }else{
        upr = pCur->idx-1;
      }
    }
    assert( lwr==upr+1 );
    assert( pPage->isInit );
    if( pPage->leaf ){
      chldPg = 0;
    }else if( lwr>=pPage->nCell ){
      chldPg = get4byte(&pPage->aData[pPage->hdrOffset+8]);
    }else{
      chldPg = get4byte(findCell(pPage, lwr));
    }
    if( chldPg==0 ){
      assert( pCur->idx>=0 && pCur->idx<pCur->pPage->nCell );
      if( pRes ) *pRes = c;
      return SQLITE_OK;
    }
    pCur->idx = lwr;
    pCur->info.nSize = 0;
    rc = moveToChild(pCur, chldPg);
    if( rc ){
      return rc;
    }
  }
  
}








int sqlite3BtreeEof(BtCursor *pCur){
  



  return (CURSOR_VALID!=pCur->eState);
}







int sqlite3BtreeNext(BtCursor *pCur, int *pRes){
  int rc;
  MemPage *pPage;

#ifndef SQLITE_OMIT_SHARED_CACHE
  rc = restoreOrClearCursorPosition(pCur, 1);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  if( pCur->skip>0 ){
    pCur->skip = 0;
    *pRes = 0;
    return SQLITE_OK;
  }
  pCur->skip = 0;
#endif 

  assert( pRes!=0 );
  pPage = pCur->pPage;
  if( CURSOR_INVALID==pCur->eState ){
    *pRes = 1;
    return SQLITE_OK;
  }
  assert( pPage->isInit );
  assert( pCur->idx<pPage->nCell );

  pCur->idx++;
  pCur->info.nSize = 0;
  if( pCur->idx>=pPage->nCell ){
    if( !pPage->leaf ){
      rc = moveToChild(pCur, get4byte(&pPage->aData[pPage->hdrOffset+8]));
      if( rc ) return rc;
      rc = moveToLeftmost(pCur);
      *pRes = 0;
      return rc;
    }
    do{
      if( isRootPage(pPage) ){
        *pRes = 1;
        pCur->eState = CURSOR_INVALID;
        return SQLITE_OK;
      }
      moveToParent(pCur);
      pPage = pCur->pPage;
    }while( pCur->idx>=pPage->nCell );
    *pRes = 0;
    if( pPage->leafData ){
      rc = sqlite3BtreeNext(pCur, pRes);
    }else{
      rc = SQLITE_OK;
    }
    return rc;
  }
  *pRes = 0;
  if( pPage->leaf ){
    return SQLITE_OK;
  }
  rc = moveToLeftmost(pCur);
  return rc;
}







int sqlite3BtreePrevious(BtCursor *pCur, int *pRes){
  int rc;
  Pgno pgno;
  MemPage *pPage;

#ifndef SQLITE_OMIT_SHARED_CACHE
  rc = restoreOrClearCursorPosition(pCur, 1);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  if( pCur->skip<0 ){
    pCur->skip = 0;
    *pRes = 0;
    return SQLITE_OK;
  }
  pCur->skip = 0;
#endif

  if( CURSOR_INVALID==pCur->eState ){
    *pRes = 1;
    return SQLITE_OK;
  }

  pPage = pCur->pPage;
  assert( pPage->isInit );
  assert( pCur->idx>=0 );
  if( !pPage->leaf ){
    pgno = get4byte( findCell(pPage, pCur->idx) );
    rc = moveToChild(pCur, pgno);
    if( rc ) return rc;
    rc = moveToRightmost(pCur);
  }else{
    while( pCur->idx==0 ){
      if( isRootPage(pPage) ){
        pCur->eState = CURSOR_INVALID;
        *pRes = 1;
        return SQLITE_OK;
      }
      moveToParent(pCur);
      pPage = pCur->pPage;
    }
    pCur->idx--;
    pCur->info.nSize = 0;
    if( pPage->leafData && !pPage->leaf ){
      rc = sqlite3BtreePrevious(pCur, pRes);
    }else{
      rc = SQLITE_OK;
    }
  }
  *pRes = 0;
  return rc;
}






















static int allocatePage(
  BtShared *pBt, 
  MemPage **ppPage, 
  Pgno *pPgno, 
  Pgno nearby,
  u8 exact
){
  MemPage *pPage1;
  int rc;
  int n;     
  int k;     

  pPage1 = pBt->pPage1;
  n = get4byte(&pPage1->aData[36]);
  if( n>0 ){
    
    MemPage *pTrunk = 0;
    Pgno iTrunk;
    MemPage *pPrevTrunk = 0;
    u8 searchList = 0; 
    
    



#ifndef SQLITE_OMIT_AUTOVACUUM
    if( exact ){
      u8 eType;
      assert( nearby>0 );
      assert( pBt->autoVacuum );
      rc = ptrmapGet(pBt, nearby, &eType, 0);
      if( rc ) return rc;
      if( eType==PTRMAP_FREEPAGE ){
        searchList = 1;
      }
      *pPgno = nearby;
    }
#endif

    


    rc = sqlite3pager_write(pPage1->aData);
    if( rc ) return rc;
    put4byte(&pPage1->aData[36], n-1);

    



    do {
      pPrevTrunk = pTrunk;
      if( pPrevTrunk ){
        iTrunk = get4byte(&pPrevTrunk->aData[0]);
      }else{
        iTrunk = get4byte(&pPage1->aData[32]);
      }
      rc = getPage(pBt, iTrunk, &pTrunk);
      if( rc ){
        releasePage(pPrevTrunk);
        return rc;
      }

      
      rc = sqlite3pager_write(pTrunk->aData);
      if( rc ){
        releasePage(pTrunk);
        releasePage(pPrevTrunk);
        return rc;
      }

      k = get4byte(&pTrunk->aData[4]);
      if( k==0 && !searchList ){
        


        assert( pPrevTrunk==0 );
        *pPgno = iTrunk;
        memcpy(&pPage1->aData[32], &pTrunk->aData[0], 4);
        *ppPage = pTrunk;
        pTrunk = 0;
        TRACE(("ALLOCATE: %d trunk - %d free pages left\n", *pPgno, n-1));
      }else if( k>pBt->usableSize/4 - 8 ){
        
        return SQLITE_CORRUPT_BKPT;
#ifndef SQLITE_OMIT_AUTOVACUUM
      }else if( searchList && nearby==iTrunk ){
        


        assert( *pPgno==iTrunk );
        *ppPage = pTrunk;
        searchList = 0;
        if( k==0 ){
          if( !pPrevTrunk ){
            memcpy(&pPage1->aData[32], &pTrunk->aData[0], 4);
          }else{
            memcpy(&pPrevTrunk->aData[0], &pTrunk->aData[0], 4);
          }
        }else{
          



          MemPage *pNewTrunk;
          Pgno iNewTrunk = get4byte(&pTrunk->aData[8]);
          rc = getPage(pBt, iNewTrunk, &pNewTrunk);
          if( rc!=SQLITE_OK ){
            releasePage(pTrunk);
            releasePage(pPrevTrunk);
            return rc;
          }
          rc = sqlite3pager_write(pNewTrunk->aData);
          if( rc!=SQLITE_OK ){
            releasePage(pNewTrunk);
            releasePage(pTrunk);
            releasePage(pPrevTrunk);
            return rc;
          }
          memcpy(&pNewTrunk->aData[0], &pTrunk->aData[0], 4);
          put4byte(&pNewTrunk->aData[4], k-1);
          memcpy(&pNewTrunk->aData[8], &pTrunk->aData[12], (k-1)*4);
          if( !pPrevTrunk ){
            put4byte(&pPage1->aData[32], iNewTrunk);
          }else{
            put4byte(&pPrevTrunk->aData[0], iNewTrunk);
          }
          releasePage(pNewTrunk);
        }
        pTrunk = 0;
        TRACE(("ALLOCATE: %d trunk - %d free pages left\n", *pPgno, n-1));
#endif
      }else{
        
        int closest;
        Pgno iPage;
        unsigned char *aData = pTrunk->aData;
        if( nearby>0 ){
          int i, dist;
          closest = 0;
          dist = get4byte(&aData[8]) - nearby;
          if( dist<0 ) dist = -dist;
          for(i=1; i<k; i++){
            int d2 = get4byte(&aData[8+i*4]) - nearby;
            if( d2<0 ) d2 = -d2;
            if( d2<dist ){
              closest = i;
              dist = d2;
            }
          }
        }else{
          closest = 0;
        }

        iPage = get4byte(&aData[8+closest*4]);
        if( !searchList || iPage==nearby ){
          *pPgno = iPage;
          if( *pPgno>sqlite3pager_pagecount(pBt->pPager) ){
            
            return SQLITE_CORRUPT_BKPT;
          }
          TRACE(("ALLOCATE: %d was leaf %d of %d on trunk %d"
                 ": %d more free pages\n",
                 *pPgno, closest+1, k, pTrunk->pgno, n-1));
          if( closest<k-1 ){
            memcpy(&aData[8+closest*4], &aData[4+k*4], 4);
          }
          put4byte(&aData[4], k-1);
          rc = getPage(pBt, *pPgno, ppPage);
          if( rc==SQLITE_OK ){
            sqlite3pager_dont_rollback((*ppPage)->aData);
            rc = sqlite3pager_write((*ppPage)->aData);
            if( rc!=SQLITE_OK ){
              releasePage(*ppPage);
            }
          }
          searchList = 0;
        }
      }
      releasePage(pPrevTrunk);
    }while( searchList );
    releasePage(pTrunk);
  }else{
    

    *pPgno = sqlite3pager_pagecount(pBt->pPager) + 1;

#ifndef SQLITE_OMIT_AUTOVACUUM
    if( pBt->autoVacuum && PTRMAP_ISPAGE(pBt, *pPgno) ){
      



      TRACE(("ALLOCATE: %d from end of file (pointer-map page)\n", *pPgno));
      assert( *pPgno!=PENDING_BYTE_PAGE(pBt) );
      (*pPgno)++;
    }
#endif

    assert( *pPgno!=PENDING_BYTE_PAGE(pBt) );
    rc = getPage(pBt, *pPgno, ppPage);
    if( rc ) return rc;
    rc = sqlite3pager_write((*ppPage)->aData);
    if( rc!=SQLITE_OK ){
      releasePage(*ppPage);
    }
    TRACE(("ALLOCATE: %d from end of file\n", *pPgno));
  }

  assert( *pPgno!=PENDING_BYTE_PAGE(pBt) );
  return rc;
}






static int freePage(MemPage *pPage){
  BtShared *pBt = pPage->pBt;
  MemPage *pPage1 = pBt->pPage1;
  int rc, n, k;

  
  assert( pPage->pgno>1 );
  pPage->isInit = 0;
  releasePage(pPage->pParent);
  pPage->pParent = 0;

  
  rc = sqlite3pager_write(pPage1->aData);
  if( rc ) return rc;
  n = get4byte(&pPage1->aData[36]);
  put4byte(&pPage1->aData[36], n+1);

#ifdef SQLITE_SECURE_DELETE
  


  rc = sqlite3pager_write(pPage->aData);
  if( rc ) return rc;
  memset(pPage->aData, 0, pPage->pBt->pageSize);
#endif

#ifndef SQLITE_OMIT_AUTOVACUUM
  


  if( pBt->autoVacuum ){
    rc = ptrmapPut(pBt, pPage->pgno, PTRMAP_FREEPAGE, 0);
    if( rc ) return rc;
  }
#endif

  if( n==0 ){
    
    rc = sqlite3pager_write(pPage->aData);
    if( rc ) return rc;
    memset(pPage->aData, 0, 8);
    put4byte(&pPage1->aData[32], pPage->pgno);
    TRACE(("FREE-PAGE: %d first\n", pPage->pgno));
  }else{
    

    MemPage *pTrunk;
    rc = getPage(pBt, get4byte(&pPage1->aData[32]), &pTrunk);
    if( rc ) return rc;
    k = get4byte(&pTrunk->aData[4]);
    if( k>=pBt->usableSize/4 - 8 ){
      

      rc = sqlite3pager_write(pPage->aData);
      if( rc ) return rc;
      put4byte(pPage->aData, pTrunk->pgno);
      put4byte(&pPage->aData[4], 0);
      put4byte(&pPage1->aData[32], pPage->pgno);
      TRACE(("FREE-PAGE: %d new trunk page replacing %d\n",
              pPage->pgno, pTrunk->pgno));
    }else{
      
      rc = sqlite3pager_write(pTrunk->aData);
      if( rc ) return rc;
      put4byte(&pTrunk->aData[4], k+1);
      put4byte(&pTrunk->aData[8+k*4], pPage->pgno);
#ifndef SQLITE_SECURE_DELETE
      sqlite3pager_dont_write(pBt->pPager, pPage->pgno);
#endif
      TRACE(("FREE-PAGE: %d leaf on trunk page %d\n",pPage->pgno,pTrunk->pgno));
    }
    releasePage(pTrunk);
  }
  return rc;
}




static int clearCell(MemPage *pPage, unsigned char *pCell){
  BtShared *pBt = pPage->pBt;
  CellInfo info;
  Pgno ovflPgno;
  int rc;

  parseCellPtr(pPage, pCell, &info);
  if( info.iOverflow==0 ){
    return SQLITE_OK;  
  }
  ovflPgno = get4byte(&pCell[info.iOverflow]);
  while( ovflPgno!=0 ){
    MemPage *pOvfl;
    if( ovflPgno>sqlite3pager_pagecount(pBt->pPager) ){
      return SQLITE_CORRUPT_BKPT;
    }
    rc = getPage(pBt, ovflPgno, &pOvfl);
    if( rc ) return rc;
    ovflPgno = get4byte(pOvfl->aData);
    rc = freePage(pOvfl);
    sqlite3pager_unref(pOvfl->aData);
    if( rc ) return rc;
  }
  return SQLITE_OK;
}













static int fillInCell(
  MemPage *pPage,                
  unsigned char *pCell,          
  const void *pKey, i64 nKey,    
  const void *pData,int nData,   
  int *pnSize                    
){
  int nPayload;
  const u8 *pSrc;
  int nSrc, n, rc;
  int spaceLeft;
  MemPage *pOvfl = 0;
  MemPage *pToRelease = 0;
  unsigned char *pPrior;
  unsigned char *pPayload;
  BtShared *pBt = pPage->pBt;
  Pgno pgnoOvfl = 0;
  int nHeader;
  CellInfo info;

  
  nHeader = 0;
  if( !pPage->leaf ){
    nHeader += 4;
  }
  if( pPage->hasData ){
    nHeader += putVarint(&pCell[nHeader], nData);
  }else{
    nData = 0;
  }
  nHeader += putVarint(&pCell[nHeader], *(u64*)&nKey);
  parseCellPtr(pPage, pCell, &info);
  assert( info.nHeader==nHeader );
  assert( info.nKey==nKey );
  assert( info.nData==nData );
  
  
  nPayload = nData;
  if( pPage->intKey ){
    pSrc = pData;
    nSrc = nData;
    nData = 0;
  }else{
    nPayload += nKey;
    pSrc = pKey;
    nSrc = nKey;
  }
  *pnSize = info.nSize;
  spaceLeft = info.nLocal;
  pPayload = &pCell[nHeader];
  pPrior = &pCell[info.iOverflow];

  while( nPayload>0 ){
    if( spaceLeft==0 ){
#ifndef SQLITE_OMIT_AUTOVACUUM
      Pgno pgnoPtrmap = pgnoOvfl; 
#endif
      rc = allocatePage(pBt, &pOvfl, &pgnoOvfl, pgnoOvfl, 0);
#ifndef SQLITE_OMIT_AUTOVACUUM
      




      if( pBt->autoVacuum && pgnoPtrmap!=0 && rc==SQLITE_OK ){
        rc = ptrmapPut(pBt, pgnoOvfl, PTRMAP_OVERFLOW2, pgnoPtrmap);
      }
#endif
      if( rc ){
        releasePage(pToRelease);
        
        return rc;
      }
      put4byte(pPrior, pgnoOvfl);
      releasePage(pToRelease);
      pToRelease = pOvfl;
      pPrior = pOvfl->aData;
      put4byte(pPrior, 0);
      pPayload = &pOvfl->aData[4];
      spaceLeft = pBt->usableSize - 4;
    }
    n = nPayload;
    if( n>spaceLeft ) n = spaceLeft;
    if( n>nSrc ) n = nSrc;
    assert( pSrc );
    memcpy(pPayload, pSrc, n);
    nPayload -= n;
    pPayload += n;
    pSrc += n;
    nSrc -= n;
    spaceLeft -= n;
    if( nSrc==0 ){
      nSrc = nData;
      pSrc = pData;
    }
  }
  releasePage(pToRelease);
  return SQLITE_OK;
}






static int reparentPage(BtShared *pBt, Pgno pgno, MemPage *pNewParent, int idx){
  MemPage *pThis;
  unsigned char *aData;

  assert( pNewParent!=0 );
  if( pgno==0 ) return SQLITE_OK;
  assert( pBt->pPager!=0 );
  aData = sqlite3pager_lookup(pBt->pPager, pgno);
  if( aData ){
    pThis = (MemPage*)&aData[pBt->pageSize];
    assert( pThis->aData==aData );
    if( pThis->isInit ){
      if( pThis->pParent!=pNewParent ){
        if( pThis->pParent ) sqlite3pager_unref(pThis->pParent->aData);
        pThis->pParent = pNewParent;
        sqlite3pager_ref(pNewParent->aData);
      }
      pThis->idxParent = idx;
    }
    sqlite3pager_unref(aData);
  }

#ifndef SQLITE_OMIT_AUTOVACUUM
  if( pBt->autoVacuum ){
    return ptrmapPut(pBt, pgno, PTRMAP_BTREE, pNewParent->pgno);
  }
#endif
  return SQLITE_OK;
}













static int reparentChildPages(MemPage *pPage){
  int i;
  BtShared *pBt = pPage->pBt;
  int rc = SQLITE_OK;

  if( pPage->leaf ) return SQLITE_OK;

  for(i=0; i<pPage->nCell; i++){
    u8 *pCell = findCell(pPage, i);
    if( !pPage->leaf ){
      rc = reparentPage(pBt, get4byte(pCell), pPage, i);
      if( rc!=SQLITE_OK ) return rc;
    }
  }
  if( !pPage->leaf ){
    rc = reparentPage(pBt, get4byte(&pPage->aData[pPage->hdrOffset+8]), 
       pPage, i);
    pPage->idxShift = 0;
  }
  return rc;
}









static void dropCell(MemPage *pPage, int idx, int sz){
  int i;          
  int pc;         
  u8 *data;       
  u8 *ptr;        

  assert( idx>=0 && idx<pPage->nCell );
  assert( sz==cellSize(pPage, idx) );
  assert( sqlite3pager_iswriteable(pPage->aData) );
  data = pPage->aData;
  ptr = &data[pPage->cellOffset + 2*idx];
  pc = get2byte(ptr);
  assert( pc>10 && pc+sz<=pPage->pBt->usableSize );
  freeSpace(pPage, pc, sz);
  for(i=idx+1; i<pPage->nCell; i++, ptr+=2){
    ptr[0] = ptr[2];
    ptr[1] = ptr[3];
  }
  pPage->nCell--;
  put2byte(&data[pPage->hdrOffset+3], pPage->nCell);
  pPage->nFree += 2;
  pPage->idxShift = 1;
}


















static int insertCell(
  MemPage *pPage,   
  int i,            
  u8 *pCell,        
  int sz,           
  u8 *pTemp,        
  u8 nSkip          
){
  int idx;          
  int j;            
  int top;          
  int end;          
  int ins;          
  int hdr;          
  int cellOffset;   
  u8 *data;         
  u8 *ptr;          

  assert( i>=0 && i<=pPage->nCell+pPage->nOverflow );
  assert( sz==cellSizePtr(pPage, pCell) );
  assert( sqlite3pager_iswriteable(pPage->aData) );
  if( pPage->nOverflow || sz+2>pPage->nFree ){
    if( pTemp ){
      memcpy(pTemp+nSkip, pCell+nSkip, sz-nSkip);
      pCell = pTemp;
    }
    j = pPage->nOverflow++;
    assert( j<sizeof(pPage->aOvfl)/sizeof(pPage->aOvfl[0]) );
    pPage->aOvfl[j].pCell = pCell;
    pPage->aOvfl[j].idx = i;
    pPage->nFree = 0;
  }else{
    data = pPage->aData;
    hdr = pPage->hdrOffset;
    top = get2byte(&data[hdr+5]);
    cellOffset = pPage->cellOffset;
    end = cellOffset + 2*pPage->nCell + 2;
    ins = cellOffset + 2*i;
    if( end > top - sz ){
      int rc = defragmentPage(pPage);
      if( rc!=SQLITE_OK ) return rc;
      top = get2byte(&data[hdr+5]);
      assert( end + sz <= top );
    }
    idx = allocateSpace(pPage, sz);
    assert( idx>0 );
    assert( end <= get2byte(&data[hdr+5]) );
    pPage->nCell++;
    pPage->nFree -= 2;
    memcpy(&data[idx+nSkip], pCell+nSkip, sz-nSkip);
    for(j=end-2, ptr=&data[j]; j>ins; j-=2, ptr-=2){
      ptr[0] = ptr[-2];
      ptr[1] = ptr[-1];
    }
    put2byte(&data[ins], idx);
    put2byte(&data[hdr+3], pPage->nCell);
    pPage->idxShift = 1;
    pageIntegrity(pPage);
#ifndef SQLITE_OMIT_AUTOVACUUM
    if( pPage->pBt->autoVacuum ){
      


      CellInfo info;
      parseCellPtr(pPage, pCell, &info);
      if( (info.nData+(pPage->intKey?0:info.nKey))>info.nLocal ){
        Pgno pgnoOvfl = get4byte(&pCell[info.iOverflow]);
        int rc = ptrmapPut(pPage->pBt, pgnoOvfl, PTRMAP_OVERFLOW1, pPage->pgno);
        if( rc!=SQLITE_OK ) return rc;
      }
    }
#endif
  }

  return SQLITE_OK;
}





static void assemblePage(
  MemPage *pPage,   
  int nCell,        
  u8 **apCell,      
  int *aSize        
){
  int i;            
  int totalSize;    
  int hdr;          
  int cellptr;      
  int cellbody;     
  u8 *data;         

  assert( pPage->nOverflow==0 );
  totalSize = 0;
  for(i=0; i<nCell; i++){
    totalSize += aSize[i];
  }
  assert( totalSize+2*nCell<=pPage->nFree );
  assert( pPage->nCell==0 );
  cellptr = pPage->cellOffset;
  data = pPage->aData;
  hdr = pPage->hdrOffset;
  put2byte(&data[hdr+3], nCell);
  if( nCell ){
    cellbody = allocateSpace(pPage, totalSize);
    assert( cellbody>0 );
    assert( pPage->nFree >= 2*nCell );
    pPage->nFree -= 2*nCell;
    for(i=0; i<nCell; i++){
      put2byte(&data[cellptr], cellbody);
      memcpy(&data[cellbody], apCell[i], aSize[i]);
      cellptr += 2;
      cellbody += aSize[i];
    }
    assert( cellbody==pPage->pBt->usableSize );
  }
  pPage->nCell = nCell;
}













#define NN 1             /* Number of neighbors on either side of pPage */
#define NB (NN*2+1)      /* Total pages involved in the balance */


static int balance(MemPage*, int);

#ifndef SQLITE_OMIT_QUICKBALANCE

















static int balance_quick(MemPage *pPage, MemPage *pParent){
  int rc;
  MemPage *pNew;
  Pgno pgnoNew;
  u8 *pCell;
  int szCell;
  CellInfo info;
  BtShared *pBt = pPage->pBt;
  int parentIdx = pParent->nCell;   
  int parentSize;                   
  u8 parentCell[64];                

  


  rc = allocatePage(pBt, &pNew, &pgnoNew, 0, 0);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  pCell = pPage->aOvfl[0].pCell;
  szCell = cellSizePtr(pPage, pCell);
  zeroPage(pNew, pPage->aData[0]);
  assemblePage(pNew, 1, &pCell, &szCell);
  pPage->nOverflow = 0;

  
  pNew->pParent = pParent;
  sqlite3pager_ref(pParent->aData);

  



  assert( pPage->nCell>0 );
  parseCellPtr(pPage, findCell(pPage, pPage->nCell-1), &info);
  rc = fillInCell(pParent, parentCell, 0, info.nKey, 0, 0, &parentSize);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  assert( parentSize<64 );
  rc = insertCell(pParent, parentIdx, parentCell, parentSize, 0, 4);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  put4byte(findOverflowCell(pParent,parentIdx), pPage->pgno);
  put4byte(&pParent->aData[pParent->hdrOffset+8], pgnoNew);

#ifndef SQLITE_OMIT_AUTOVACUUM
  



  if( pBt->autoVacuum ){
    rc = ptrmapPut(pBt, pgnoNew, PTRMAP_BTREE, pParent->pgno);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    rc = ptrmapPutOvfl(pNew, 0);
    if( rc!=SQLITE_OK ){
      return rc;
    }
  }
#endif

  


  releasePage(pNew);
  return balance(pParent, 0);
}
#endif 








#ifndef SQLITE_OMIT_AUTOVACUUM
#define ISAUTOVACUUM (pBt->autoVacuum)
#else
#define ISAUTOVACUUM 0
#endif






























static int balance_nonroot(MemPage *pPage){
  MemPage *pParent;            
  BtShared *pBt;                  
  int nCell = 0;               
  int nMaxCells = 0;           
  int nOld;                    
  int nNew;                    
  int nDiv;                    
  int i, j, k;                 
  int idx;                     
  int nxDiv;                   
  int rc;                      
  int leafCorrection;          
  int leafData;                
  int usableSpace;             
  int pageFlags;               
  int subtotal;                
  int iSpace = 0;              
  MemPage *apOld[NB];          
  Pgno pgnoOld[NB];            
  MemPage *apCopy[NB];         
  MemPage *apNew[NB+2];        
  Pgno pgnoNew[NB+2];          
  u8 *apDiv[NB];               
  int cntNew[NB+2];            
  int szNew[NB+2];             
  u8 **apCell = 0;             
  int *szCell;                 
  u8 *aCopy[NB];               
  u8 *aSpace;                  
#ifndef SQLITE_OMIT_AUTOVACUUM
  u8 *aFrom = 0;
#endif

  


  assert( pPage->isInit );
  assert( sqlite3pager_iswriteable(pPage->aData) );
  pBt = pPage->pBt;
  pParent = pPage->pParent;
  assert( pParent );
  if( SQLITE_OK!=(rc = sqlite3pager_write(pParent->aData)) ){
    return rc;
  }
  TRACE(("BALANCE: begin page %d child of %d\n", pPage->pgno, pParent->pgno));

#ifndef SQLITE_OMIT_QUICKBALANCE
  







  if( pPage->leaf &&
      pPage->intKey &&
      pPage->leafData &&
      pPage->nOverflow==1 &&
      pPage->aOvfl[0].idx==pPage->nCell &&
      pPage->pParent->pgno!=1 &&
      get4byte(&pParent->aData[pParent->hdrOffset+8])==pPage->pgno
  ){
    



    return balance_quick(pPage, pParent);
  }
#endif

  




  if( pParent->idxShift ){
    Pgno pgno;
    pgno = pPage->pgno;
    assert( pgno==sqlite3pager_pagenumber(pPage->aData) );
    for(idx=0; idx<pParent->nCell; idx++){
      if( get4byte(findCell(pParent, idx))==pgno ){
        break;
      }
    }
    assert( idx<pParent->nCell
             || get4byte(&pParent->aData[pParent->hdrOffset+8])==pgno );
  }else{
    idx = pPage->idxParent;
  }

  



  nOld = nNew = 0;
  sqlite3pager_ref(pParent->aData);

  






  nxDiv = idx - NN;
  if( nxDiv + NB > pParent->nCell ){
    nxDiv = pParent->nCell - NB + 1;
  }
  if( nxDiv<0 ){
    nxDiv = 0;
  }
  nDiv = 0;
  for(i=0, k=nxDiv; i<NB; i++, k++){
    if( k<pParent->nCell ){
      apDiv[i] = findCell(pParent, k);
      nDiv++;
      assert( !pParent->leaf );
      pgnoOld[i] = get4byte(apDiv[i]);
    }else if( k==pParent->nCell ){
      pgnoOld[i] = get4byte(&pParent->aData[pParent->hdrOffset+8]);
    }else{
      break;
    }
    rc = getAndInitPage(pBt, pgnoOld[i], &apOld[i], pParent);
    if( rc ) goto balance_cleanup;
    apOld[i]->idxParent = k;
    apCopy[i] = 0;
    assert( i==nOld );
    nOld++;
    nMaxCells += 1+apOld[i]->nCell+apOld[i]->nOverflow;
  }

  

  nMaxCells = (nMaxCells + 1)&~1;

  


  apCell = sqliteMallocRaw( 
       nMaxCells*sizeof(u8*)                           
     + nMaxCells*sizeof(int)                           
     + ROUND8(sizeof(MemPage))*NB                      
     + pBt->pageSize*(5+NB)                            
     + (ISAUTOVACUUM ? nMaxCells : 0)                  
  );
  if( apCell==0 ){
    rc = SQLITE_NOMEM;
    goto balance_cleanup;
  }
  szCell = (int*)&apCell[nMaxCells];
  aCopy[0] = (u8*)&szCell[nMaxCells];
  assert( ((aCopy[0] - (u8*)apCell) & 7)==0 ); 
  for(i=1; i<NB; i++){
    aCopy[i] = &aCopy[i-1][pBt->pageSize+ROUND8(sizeof(MemPage))];
    assert( ((aCopy[i] - (u8*)apCell) & 7)==0 ); 
  }
  aSpace = &aCopy[NB-1][pBt->pageSize+ROUND8(sizeof(MemPage))];
  assert( ((aSpace - (u8*)apCell) & 7)==0 ); 
#ifndef SQLITE_OMIT_AUTOVACUUM
  if( pBt->autoVacuum ){
    aFrom = &aSpace[5*pBt->pageSize];
  }
#endif
  
  





  for(i=0; i<nOld; i++){
    MemPage *p = apCopy[i] = (MemPage*)&aCopy[i][pBt->pageSize];
    p->aData = &((u8*)p)[-pBt->pageSize];
    memcpy(p->aData, apOld[i]->aData, pBt->pageSize + sizeof(MemPage));
    

    p->aData = &((u8*)p)[-pBt->pageSize];
  }

  















  nCell = 0;
  leafCorrection = pPage->leaf*4;
  leafData = pPage->leafData && pPage->leaf;
  for(i=0; i<nOld; i++){
    MemPage *pOld = apCopy[i];
    int limit = pOld->nCell+pOld->nOverflow;
    for(j=0; j<limit; j++){
      assert( nCell<nMaxCells );
      apCell[nCell] = findOverflowCell(pOld, j);
      szCell[nCell] = cellSizePtr(pOld, apCell[nCell]);
#ifndef SQLITE_OMIT_AUTOVACUUM
      if( pBt->autoVacuum ){
        int a;
        aFrom[nCell] = i;
        for(a=0; a<pOld->nOverflow; a++){
          if( pOld->aOvfl[a].pCell==apCell[nCell] ){
            aFrom[nCell] = 0xFF;
            break;
          }
        }
      }
#endif
      nCell++;
    }
    if( i<nOld-1 ){
      int sz = cellSizePtr(pParent, apDiv[i]);
      if( leafData ){
        




        dropCell(pParent, nxDiv, sz);
      }else{
        u8 *pTemp;
        assert( nCell<nMaxCells );
        szCell[nCell] = sz;
        pTemp = &aSpace[iSpace];
        iSpace += sz;
        assert( iSpace<=pBt->pageSize*5 );
        memcpy(pTemp, apDiv[i], sz);
        apCell[nCell] = pTemp+leafCorrection;
#ifndef SQLITE_OMIT_AUTOVACUUM
        if( pBt->autoVacuum ){
          aFrom[nCell] = 0xFF;
        }
#endif
        dropCell(pParent, nxDiv, sz);
        szCell[nCell] -= leafCorrection;
        assert( get4byte(pTemp)==pgnoOld[i] );
        if( !pOld->leaf ){
          assert( leafCorrection==0 );
          

          memcpy(apCell[nCell], &pOld->aData[pOld->hdrOffset+8], 4);
        }else{
          assert( leafCorrection==4 );
        }
        nCell++;
      }
    }
  }

  















  usableSpace = pBt->usableSize - 12 + leafCorrection;
  for(subtotal=k=i=0; i<nCell; i++){
    assert( i<nMaxCells );
    subtotal += szCell[i] + 2;
    if( subtotal > usableSpace ){
      szNew[k] = subtotal - szCell[i];
      cntNew[k] = i;
      if( leafData ){ i--; }
      subtotal = 0;
      k++;
    }
  }
  szNew[k] = subtotal;
  cntNew[k] = nCell;
  k++;

  









  for(i=k-1; i>0; i--){
    int szRight = szNew[i];  
    int szLeft = szNew[i-1]; 
    int r;              
    int d;              

    r = cntNew[i-1] - 1;
    d = r + 1 - leafData;
    assert( d<nMaxCells );
    assert( r<nMaxCells );
    while( szRight==0 || szRight+szCell[d]+2<=szLeft-(szCell[r]+2) ){
      szRight += szCell[d] + 2;
      szLeft -= szCell[r] + 2;
      cntNew[i-1]--;
      r = cntNew[i-1] - 1;
      d = r + 1 - leafData;
    }
    szNew[i] = szRight;
    szNew[i-1] = szLeft;
  }

  



  assert( cntNew[0]>0 || (pParent->pgno==1 && pParent->nCell==0) );

  


  assert( pPage->pgno>1 );
  pageFlags = pPage->aData[0];
  for(i=0; i<k; i++){
    MemPage *pNew;
    if( i<nOld ){
      pNew = apNew[i] = apOld[i];
      pgnoNew[i] = pgnoOld[i];
      apOld[i] = 0;
      rc = sqlite3pager_write(pNew->aData);
      if( rc ) goto balance_cleanup;
    }else{
      assert( i>0 );
      rc = allocatePage(pBt, &pNew, &pgnoNew[i], pgnoNew[i-1], 0);
      if( rc ) goto balance_cleanup;
      apNew[i] = pNew;
    }
    nNew++;
    zeroPage(pNew, pageFlags);
  }

  

  while( i<nOld ){
    rc = freePage(apOld[i]);
    if( rc ) goto balance_cleanup;
    releasePage(apOld[i]);
    apOld[i] = 0;
    i++;
  }

  













  for(i=0; i<k-1; i++){
    int minV = pgnoNew[i];
    int minI = i;
    for(j=i+1; j<k; j++){
      if( pgnoNew[j]<(unsigned)minV ){
        minI = j;
        minV = pgnoNew[j];
      }
    }
    if( minI>i ){
      int t;
      MemPage *pT;
      t = pgnoNew[i];
      pT = apNew[i];
      pgnoNew[i] = pgnoNew[minI];
      apNew[i] = apNew[minI];
      pgnoNew[minI] = t;
      apNew[minI] = pT;
    }
  }
  TRACE(("BALANCE: old: %d %d %d  new: %d(%d) %d(%d) %d(%d) %d(%d) %d(%d)\n",
    pgnoOld[0], 
    nOld>=2 ? pgnoOld[1] : 0,
    nOld>=3 ? pgnoOld[2] : 0,
    pgnoNew[0], szNew[0],
    nNew>=2 ? pgnoNew[1] : 0, nNew>=2 ? szNew[1] : 0,
    nNew>=3 ? pgnoNew[2] : 0, nNew>=3 ? szNew[2] : 0,
    nNew>=4 ? pgnoNew[3] : 0, nNew>=4 ? szNew[3] : 0,
    nNew>=5 ? pgnoNew[4] : 0, nNew>=5 ? szNew[4] : 0));

  



  j = 0;
  for(i=0; i<nNew; i++){
    
    MemPage *pNew = apNew[i];
    assert( j<nMaxCells );
    assert( pNew->pgno==pgnoNew[i] );
    assemblePage(pNew, cntNew[i]-j, &apCell[j], &szCell[j]);
    assert( pNew->nCell>0 || (nNew==1 && cntNew[0]==0) );
    assert( pNew->nOverflow==0 );

#ifndef SQLITE_OMIT_AUTOVACUUM
    




    if( pBt->autoVacuum ){
      for(k=j; k<cntNew[i]; k++){
        assert( k<nMaxCells );
        if( aFrom[k]==0xFF || apCopy[aFrom[k]]->pgno!=pNew->pgno ){
          rc = ptrmapPutOvfl(pNew, k-j);
          if( rc!=SQLITE_OK ){
            goto balance_cleanup;
          }
        }
      }
    }
#endif

    j = cntNew[i];

    


    if( i<nNew-1 && j<nCell ){
      u8 *pCell;
      u8 *pTemp;
      int sz;

      assert( j<nMaxCells );
      pCell = apCell[j];
      sz = szCell[j] + leafCorrection;
      if( !pNew->leaf ){
        memcpy(&pNew->aData[8], pCell, 4);
        pTemp = 0;
      }else if( leafData ){
	




        CellInfo info;
        j--;
        parseCellPtr(pNew, apCell[j], &info);
        pCell = &aSpace[iSpace];
        fillInCell(pParent, pCell, 0, info.nKey, 0, 0, &sz);
        iSpace += sz;
        assert( iSpace<=pBt->pageSize*5 );
        pTemp = 0;
      }else{
        pCell -= 4;
        pTemp = &aSpace[iSpace];
        iSpace += sz;
        assert( iSpace<=pBt->pageSize*5 );
      }
      rc = insertCell(pParent, nxDiv, pCell, sz, pTemp, 4);
      if( rc!=SQLITE_OK ) goto balance_cleanup;
      put4byte(findOverflowCell(pParent,nxDiv), pNew->pgno);
#ifndef SQLITE_OMIT_AUTOVACUUM
      



      if( pBt->autoVacuum && !leafData ){
        rc = ptrmapPutOvfl(pParent, nxDiv);
        if( rc!=SQLITE_OK ){
          goto balance_cleanup;
        }
      }
#endif
      j++;
      nxDiv++;
    }
  }
  assert( j==nCell );
  assert( nOld>0 );
  assert( nNew>0 );
  if( (pageFlags & PTF_LEAF)==0 ){
    memcpy(&apNew[nNew-1]->aData[8], &apCopy[nOld-1]->aData[8], 4);
  }
  if( nxDiv==pParent->nCell+pParent->nOverflow ){
    
    put4byte(&pParent->aData[pParent->hdrOffset+8], pgnoNew[nNew-1]);
  }else{
    

    put4byte(findOverflowCell(pParent, nxDiv), pgnoNew[nNew-1]);
  }

  


  for(i=0; i<nNew; i++){
    rc = reparentChildPages(apNew[i]);
    if( rc!=SQLITE_OK ) goto balance_cleanup;
  }
  rc = reparentChildPages(pParent);
  if( rc!=SQLITE_OK ) goto balance_cleanup;

  




  assert( pParent->isInit );
  
   
  rc = balance(pParent, 0);
  
  


balance_cleanup:
  sqliteFree(apCell);
  for(i=0; i<nOld; i++){
    releasePage(apOld[i]);
  }
  for(i=0; i<nNew; i++){
    releasePage(apNew[i]);
  }
  releasePage(pParent);
  TRACE(("BALANCE: finished with %d: old=%d new=%d cells=%d\n",
          pPage->pgno, nOld, nNew, nCell));
  return rc;
}






static int balance_shallower(MemPage *pPage){
  MemPage *pChild;             
  Pgno pgnoChild;              
  int rc = SQLITE_OK;          
  BtShared *pBt;                  
  int mxCellPerPage;           
  u8 **apCell;                 
  int *szCell;                 

  assert( pPage->pParent==0 );
  assert( pPage->nCell==0 );
  pBt = pPage->pBt;
  mxCellPerPage = MX_CELL(pBt);
  apCell = sqliteMallocRaw( mxCellPerPage*(sizeof(u8*)+sizeof(int)) );
  if( apCell==0 ) return SQLITE_NOMEM;
  szCell = (int*)&apCell[mxCellPerPage];
  if( pPage->leaf ){
    
    TRACE(("BALANCE: empty table %d\n", pPage->pgno));
  }else{
    











    pgnoChild = get4byte(&pPage->aData[pPage->hdrOffset+8]);
    assert( pgnoChild>0 );
    assert( pgnoChild<=sqlite3pager_pagecount(pPage->pBt->pPager) );
    rc = getPage(pPage->pBt, pgnoChild, &pChild);
    if( rc ) goto end_shallow_balance;
    if( pPage->pgno==1 ){
      rc = initPage(pChild, pPage);
      if( rc ) goto end_shallow_balance;
      assert( pChild->nOverflow==0 );
      if( pChild->nFree>=100 ){
        

        int i;
        zeroPage(pPage, pChild->aData[0]);
        for(i=0; i<pChild->nCell; i++){
          apCell[i] = findCell(pChild,i);
          szCell[i] = cellSizePtr(pChild, apCell[i]);
        }
        assemblePage(pPage, pChild->nCell, apCell, szCell);
        
        put4byte(&pPage->aData[pPage->hdrOffset+8], 
            get4byte(&pChild->aData[pChild->hdrOffset+8]));
        freePage(pChild);
        TRACE(("BALANCE: child %d transfer to page 1\n", pChild->pgno));
      }else{
        

        TRACE(("BALANCE: child %d will not fit on page 1\n", pChild->pgno));
      }
    }else{
      memcpy(pPage->aData, pChild->aData, pPage->pBt->usableSize);
      pPage->isInit = 0;
      pPage->pParent = 0;
      rc = initPage(pPage, 0);
      assert( rc==SQLITE_OK );
      freePage(pChild);
      TRACE(("BALANCE: transfer child %d into root %d\n",
              pChild->pgno, pPage->pgno));
    }
    rc = reparentChildPages(pPage);
    assert( pPage->nOverflow==0 );
#ifndef SQLITE_OMIT_AUTOVACUUM
    if( pBt->autoVacuum ){
      int i;
      for(i=0; i<pPage->nCell; i++){ 
        rc = ptrmapPutOvfl(pPage, i);
        if( rc!=SQLITE_OK ){
          goto end_shallow_balance;
        }
      }
    }
#endif
    if( rc!=SQLITE_OK ) goto end_shallow_balance;
    releasePage(pChild);
  }
end_shallow_balance:
  sqliteFree(apCell);
  return rc;
}











static int balance_deeper(MemPage *pPage){
  int rc;             
  MemPage *pChild;    
  Pgno pgnoChild;     
  BtShared *pBt;         
  int usableSize;     
  u8 *data;           
  u8 *cdata;          
  int hdr;            
  int brk;            

  assert( pPage->pParent==0 );
  assert( pPage->nOverflow>0 );
  pBt = pPage->pBt;
  rc = allocatePage(pBt, &pChild, &pgnoChild, pPage->pgno, 0);
  if( rc ) return rc;
  assert( sqlite3pager_iswriteable(pChild->aData) );
  usableSize = pBt->usableSize;
  data = pPage->aData;
  hdr = pPage->hdrOffset;
  brk = get2byte(&data[hdr+5]);
  cdata = pChild->aData;
  memcpy(cdata, &data[hdr], pPage->cellOffset+2*pPage->nCell-hdr);
  memcpy(&cdata[brk], &data[brk], usableSize-brk);
  assert( pChild->isInit==0 );
  rc = initPage(pChild, pPage);
  if( rc ) goto balancedeeper_out;
  memcpy(pChild->aOvfl, pPage->aOvfl, pPage->nOverflow*sizeof(pPage->aOvfl[0]));
  pChild->nOverflow = pPage->nOverflow;
  if( pChild->nOverflow ){
    pChild->nFree = 0;
  }
  assert( pChild->nCell==pPage->nCell );
  zeroPage(pPage, pChild->aData[0] & ~PTF_LEAF);
  put4byte(&pPage->aData[pPage->hdrOffset+8], pgnoChild);
  TRACE(("BALANCE: copy root %d into %d\n", pPage->pgno, pChild->pgno));
#ifndef SQLITE_OMIT_AUTOVACUUM
  if( pBt->autoVacuum ){
    int i;
    rc = ptrmapPut(pBt, pChild->pgno, PTRMAP_BTREE, pPage->pgno);
    if( rc ) goto balancedeeper_out;
    for(i=0; i<pChild->nCell; i++){
      rc = ptrmapPutOvfl(pChild, i);
      if( rc!=SQLITE_OK ){
        return rc;
      }
    }
  }
#endif
  rc = balance_nonroot(pChild);

balancedeeper_out:
  releasePage(pChild);
  return rc;
}





static int balance(MemPage *pPage, int insert){
  int rc = SQLITE_OK;
  if( pPage->pParent==0 ){
    if( pPage->nOverflow>0 ){
      rc = balance_deeper(pPage);
    }
    if( rc==SQLITE_OK && pPage->nCell==0 ){
      rc = balance_shallower(pPage);
    }
  }else{
    if( pPage->nOverflow>0 || 
        (!insert && pPage->nFree>pPage->pBt->usableSize*2/3) ){
      rc = balance_nonroot(pPage);
    }
  }
  return rc;
}
















static int checkReadLocks(BtShared *pBt, Pgno pgnoRoot, BtCursor *pExclude){
  BtCursor *p;
  for(p=pBt->pCursor; p; p=p->pNext){
    u32 flags = (p->pBtree->pSqlite ? p->pBtree->pSqlite->flags : 0);
    if( p->pgnoRoot!=pgnoRoot || p==pExclude ) continue;
    if( p->wrFlag==0 && flags&SQLITE_ReadUncommitted ) continue;
    if( p->wrFlag==0 ) return SQLITE_LOCKED;
    if( p->pPage->pgno!=p->pgnoRoot ){
      moveToRoot(p);
    }
  }
  return SQLITE_OK;
}










int sqlite3BtreeInsert(
  BtCursor *pCur,                
  const void *pKey, i64 nKey,    
  const void *pData, int nData   
){
  int rc;
  int loc;
  int szNew;
  MemPage *pPage;
  BtShared *pBt = pCur->pBtree->pBt;
  unsigned char *oldCell;
  unsigned char *newCell = 0;

  if( pBt->inTransaction!=TRANS_WRITE ){
    
    return pBt->readOnly ? SQLITE_READONLY : SQLITE_ERROR;
  }
  assert( !pBt->readOnly );
  if( !pCur->wrFlag ){
    return SQLITE_PERM;   
  }
  if( checkReadLocks(pBt, pCur->pgnoRoot, pCur) ){
    return SQLITE_LOCKED; 
  }

  
  restoreOrClearCursorPosition(pCur, 0);
  if( 
    SQLITE_OK!=(rc = saveAllCursors(pBt, pCur->pgnoRoot, pCur)) ||
    SQLITE_OK!=(rc = sqlite3BtreeMoveto(pCur, pKey, nKey, &loc))
  ){
    return rc;
  }

  pPage = pCur->pPage;
  assert( pPage->intKey || nKey>=0 );
  assert( pPage->leaf || !pPage->leafData );
  TRACE(("INSERT: table=%d nkey=%lld ndata=%d page=%d %s\n",
          pCur->pgnoRoot, nKey, nData, pPage->pgno,
          loc==0 ? "overwrite" : "new entry"));
  assert( pPage->isInit );
  rc = sqlite3pager_write(pPage->aData);
  if( rc ) return rc;
  newCell = sqliteMallocRaw( MX_CELL_SIZE(pBt) );
  if( newCell==0 ) return SQLITE_NOMEM;
  rc = fillInCell(pPage, newCell, pKey, nKey, pData, nData, &szNew);
  if( rc ) goto end_insert;
  assert( szNew==cellSizePtr(pPage, newCell) );
  assert( szNew<=MX_CELL_SIZE(pBt) );
  if( loc==0 && CURSOR_VALID==pCur->eState ){
    int szOld;
    assert( pCur->idx>=0 && pCur->idx<pPage->nCell );
    oldCell = findCell(pPage, pCur->idx);
    if( !pPage->leaf ){
      memcpy(newCell, oldCell, 4);
    }
    szOld = cellSizePtr(pPage, oldCell);
    rc = clearCell(pPage, oldCell);
    if( rc ) goto end_insert;
    dropCell(pPage, pCur->idx, szOld);
  }else if( loc<0 && pPage->nCell>0 ){
    assert( pPage->leaf );
    pCur->idx++;
    pCur->info.nSize = 0;
  }else{
    assert( pPage->leaf );
  }
  rc = insertCell(pPage, pCur->idx, newCell, szNew, 0, 0);
  if( rc!=SQLITE_OK ) goto end_insert;
  rc = balance(pPage, 1);
  
  
  if( rc==SQLITE_OK ){
    moveToRoot(pCur);
  }
end_insert:
  sqliteFree(newCell);
  return rc;
}





int sqlite3BtreeDelete(BtCursor *pCur){
  MemPage *pPage = pCur->pPage;
  unsigned char *pCell;
  int rc;
  Pgno pgnoChild = 0;
  BtShared *pBt = pCur->pBtree->pBt;

  assert( pPage->isInit );
  if( pBt->inTransaction!=TRANS_WRITE ){
    
    return pBt->readOnly ? SQLITE_READONLY : SQLITE_ERROR;
  }
  assert( !pBt->readOnly );
  if( pCur->idx >= pPage->nCell ){
    return SQLITE_ERROR;  
  }
  if( !pCur->wrFlag ){
    return SQLITE_PERM;   
  }
  if( checkReadLocks(pBt, pCur->pgnoRoot, pCur) ){
    return SQLITE_LOCKED; 
  }

  




  if( 
    (rc = restoreOrClearCursorPosition(pCur, 1))!=0 ||
    (rc = saveAllCursors(pBt, pCur->pgnoRoot, pCur))!=0 ||
    (rc = sqlite3pager_write(pPage->aData))!=0
  ){
    return rc;
  }

  



  pCell = findCell(pPage, pCur->idx);
  if( !pPage->leaf ){
    pgnoChild = get4byte(pCell);
  }
  rc = clearCell(pPage, pCell);
  if( rc ) return rc;

  if( !pPage->leaf ){
    






    BtCursor leafCur;
    unsigned char *pNext;
    int szNext;  


    int notUsed;
    unsigned char *tempCell = 0;
    assert( !pPage->leafData );
    getTempCursor(pCur, &leafCur);
    rc = sqlite3BtreeNext(&leafCur, &notUsed);
    if( rc!=SQLITE_OK ){
      if( rc!=SQLITE_NOMEM ){
        rc = SQLITE_CORRUPT_BKPT; 
      }
    }
    if( rc==SQLITE_OK ){
      rc = sqlite3pager_write(leafCur.pPage->aData);
    }
    if( rc==SQLITE_OK ){
      TRACE(("DELETE: table=%d delete internal from %d replace from leaf %d\n",
         pCur->pgnoRoot, pPage->pgno, leafCur.pPage->pgno));
      dropCell(pPage, pCur->idx, cellSizePtr(pPage, pCell));
      pNext = findCell(leafCur.pPage, leafCur.idx);
      szNext = cellSizePtr(leafCur.pPage, pNext);
      assert( MX_CELL_SIZE(pBt)>=szNext+4 );
      tempCell = sqliteMallocRaw( MX_CELL_SIZE(pBt) );
      if( tempCell==0 ){
        rc = SQLITE_NOMEM;
      }
    }
    if( rc==SQLITE_OK ){
      rc = insertCell(pPage, pCur->idx, pNext-4, szNext+4, tempCell, 0);
    }
    if( rc==SQLITE_OK ){
      put4byte(findOverflowCell(pPage, pCur->idx), pgnoChild);
      rc = balance(pPage, 0);
    }
    if( rc==SQLITE_OK ){
      dropCell(leafCur.pPage, leafCur.idx, szNext);
      rc = balance(leafCur.pPage, 0);
    }
    sqliteFree(tempCell);
    releaseTempCursor(&leafCur);
  }else{
    TRACE(("DELETE: table=%d delete from leaf %d\n",
       pCur->pgnoRoot, pPage->pgno));
    dropCell(pPage, pCur->idx, cellSizePtr(pPage, pCell));
    rc = balance(pPage, 0);
  }
  if( rc==SQLITE_OK ){
    moveToRoot(pCur);
  }
  return rc;
}












int sqlite3BtreeCreateTable(Btree *p, int *piTable, int flags){
  BtShared *pBt = p->pBt;
  MemPage *pRoot;
  Pgno pgnoRoot;
  int rc;
  if( pBt->inTransaction!=TRANS_WRITE ){
    
    return pBt->readOnly ? SQLITE_READONLY : SQLITE_ERROR;
  }
  assert( !pBt->readOnly );

  




  if( pBt->pCursor ){
    return SQLITE_LOCKED;
  }

#ifdef SQLITE_OMIT_AUTOVACUUM
  rc = allocatePage(pBt, &pRoot, &pgnoRoot, 1, 0);
  if( rc ) return rc;
#else
  if( pBt->autoVacuum ){
    Pgno pgnoMove;      
    MemPage *pPageMove; 

    



    rc = sqlite3BtreeGetMeta(p, 4, &pgnoRoot);
    if( rc!=SQLITE_OK ) return rc;
    pgnoRoot++;

    


    if( pgnoRoot==PTRMAP_PAGENO(pBt, pgnoRoot) ||
        pgnoRoot==PENDING_BYTE_PAGE(pBt) ){
      pgnoRoot++;
    }
    assert( pgnoRoot>=3 );

    



    rc = allocatePage(pBt, &pPageMove, &pgnoMove, pgnoRoot, 1);
    if( rc!=SQLITE_OK ){
      return rc;
    }

    if( pgnoMove!=pgnoRoot ){
      u8 eType;
      Pgno iPtrPage;

      releasePage(pPageMove);
      rc = getPage(pBt, pgnoRoot, &pRoot);
      if( rc!=SQLITE_OK ){
        return rc;
      }
      rc = ptrmapGet(pBt, pgnoRoot, &eType, &iPtrPage);
      if( rc!=SQLITE_OK || eType==PTRMAP_ROOTPAGE || eType==PTRMAP_FREEPAGE ){
        releasePage(pRoot);
        return rc;
      }
      assert( eType!=PTRMAP_ROOTPAGE );
      assert( eType!=PTRMAP_FREEPAGE );
      rc = sqlite3pager_write(pRoot->aData);
      if( rc!=SQLITE_OK ){
        releasePage(pRoot);
        return rc;
      }
      rc = relocatePage(pBt, pRoot, eType, iPtrPage, pgnoMove);
      releasePage(pRoot);
      if( rc!=SQLITE_OK ){
        return rc;
      }
      rc = getPage(pBt, pgnoRoot, &pRoot);
      if( rc!=SQLITE_OK ){
        return rc;
      }
      rc = sqlite3pager_write(pRoot->aData);
      if( rc!=SQLITE_OK ){
        releasePage(pRoot);
        return rc;
      }
    }else{
      pRoot = pPageMove;
    } 

    
    rc = ptrmapPut(pBt, pgnoRoot, PTRMAP_ROOTPAGE, 0);
    if( rc ){
      releasePage(pRoot);
      return rc;
    }
    rc = sqlite3BtreeUpdateMeta(p, 4, pgnoRoot);
    if( rc ){
      releasePage(pRoot);
      return rc;
    }

  }else{
    rc = allocatePage(pBt, &pRoot, &pgnoRoot, 1, 0);
    if( rc ) return rc;
  }
#endif
  assert( sqlite3pager_iswriteable(pRoot->aData) );
  zeroPage(pRoot, flags | PTF_LEAF);
  sqlite3pager_unref(pRoot->aData);
  *piTable = (int)pgnoRoot;
  return SQLITE_OK;
}





static int clearDatabasePage(
  BtShared *pBt,           
  Pgno pgno,            
  MemPage *pParent,     
  int freePageFlag      
){
  MemPage *pPage = 0;
  int rc;
  unsigned char *pCell;
  int i;

  if( pgno>sqlite3pager_pagecount(pBt->pPager) ){
    return SQLITE_CORRUPT_BKPT;
  }

  rc = getAndInitPage(pBt, pgno, &pPage, pParent);
  if( rc ) goto cleardatabasepage_out;
  rc = sqlite3pager_write(pPage->aData);
  if( rc ) goto cleardatabasepage_out;
  for(i=0; i<pPage->nCell; i++){
    pCell = findCell(pPage, i);
    if( !pPage->leaf ){
      rc = clearDatabasePage(pBt, get4byte(pCell), pPage->pParent, 1);
      if( rc ) goto cleardatabasepage_out;
    }
    rc = clearCell(pPage, pCell);
    if( rc ) goto cleardatabasepage_out;
  }
  if( !pPage->leaf ){
    rc = clearDatabasePage(pBt, get4byte(&pPage->aData[8]), pPage->pParent, 1);
    if( rc ) goto cleardatabasepage_out;
  }
  if( freePageFlag ){
    rc = freePage(pPage);
  }else{
    zeroPage(pPage, pPage->aData[0] | PTF_LEAF);
  }

cleardatabasepage_out:
  releasePage(pPage);
  return rc;
}










int sqlite3BtreeClearTable(Btree *p, int iTable){
  int rc;
  BtCursor *pCur;
  BtShared *pBt = p->pBt;
  sqlite3 *db = p->pSqlite;
  if( p->inTrans!=TRANS_WRITE ){
    return pBt->readOnly ? SQLITE_READONLY : SQLITE_ERROR;
  }

  


  if( 0==db || 0==(db->flags&SQLITE_ReadUncommitted) ){
    for(pCur=pBt->pCursor; pCur; pCur=pCur->pNext){
      if( pCur->pBtree==p && pCur->pgnoRoot==(Pgno)iTable ){
        if( 0==pCur->wrFlag ){
          return SQLITE_LOCKED;
        }
        moveToRoot(pCur);
      }
    }
  }

  
  if( SQLITE_OK!=(rc = saveAllCursors(pBt, iTable, 0)) ){
    return rc;
  }

  return clearDatabasePage(pBt, (Pgno)iTable, 0, 0);
}





















int sqlite3BtreeDropTable(Btree *p, int iTable, int *piMoved){
  int rc;
  MemPage *pPage = 0;
  BtShared *pBt = p->pBt;

  if( p->inTrans!=TRANS_WRITE ){
    return pBt->readOnly ? SQLITE_READONLY : SQLITE_ERROR;
  }

  





  if( pBt->pCursor ){
    return SQLITE_LOCKED;
  }

  rc = getPage(pBt, (Pgno)iTable, &pPage);
  if( rc ) return rc;
  rc = sqlite3BtreeClearTable(p, iTable);
  if( rc ){
    releasePage(pPage);
    return rc;
  }

  *piMoved = 0;

  if( iTable>1 ){
#ifdef SQLITE_OMIT_AUTOVACUUM
    rc = freePage(pPage);
    releasePage(pPage);
#else
    if( pBt->autoVacuum ){
      Pgno maxRootPgno;
      rc = sqlite3BtreeGetMeta(p, 4, &maxRootPgno);
      if( rc!=SQLITE_OK ){
        releasePage(pPage);
        return rc;
      }

      if( iTable==maxRootPgno ){
        


        rc = freePage(pPage);
        releasePage(pPage);
        if( rc!=SQLITE_OK ){
          return rc;
        }
      }else{
        



        MemPage *pMove;
        releasePage(pPage);
        rc = getPage(pBt, maxRootPgno, &pMove);
        if( rc!=SQLITE_OK ){
          return rc;
        }
        rc = relocatePage(pBt, pMove, PTRMAP_ROOTPAGE, 0, iTable);
        releasePage(pMove);
        if( rc!=SQLITE_OK ){
          return rc;
        }
        rc = getPage(pBt, maxRootPgno, &pMove);
        if( rc!=SQLITE_OK ){
          return rc;
        }
        rc = freePage(pMove);
        releasePage(pMove);
        if( rc!=SQLITE_OK ){
          return rc;
        }
        *piMoved = maxRootPgno;
      }

      




      maxRootPgno--;
      if( maxRootPgno==PENDING_BYTE_PAGE(pBt) ){
        maxRootPgno--;
      }
      if( maxRootPgno==PTRMAP_PAGENO(pBt, maxRootPgno) ){
        maxRootPgno--;
      }
      assert( maxRootPgno!=PENDING_BYTE_PAGE(pBt) );

      rc = sqlite3BtreeUpdateMeta(p, 4, maxRootPgno);
    }else{
      rc = freePage(pPage);
      releasePage(pPage);
    }
#endif
  }else{
    
    zeroPage(pPage, PTF_INTKEY|PTF_LEAF );
    releasePage(pPage);
  }
  return rc;  
}












int sqlite3BtreeGetMeta(Btree *p, int idx, u32 *pMeta){
  int rc;
  unsigned char *pP1;
  BtShared *pBt = p->pBt;

  




  rc = queryTableLock(p, 1, READ_LOCK);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  assert( idx>=0 && idx<=15 );
  rc = sqlite3pager_get(pBt->pPager, 1, (void**)&pP1);
  if( rc ) return rc;
  *pMeta = get4byte(&pP1[36 + idx*4]);
  sqlite3pager_unref(pP1);

  


#ifdef SQLITE_OMIT_AUTOVACUUM
  if( idx==4 && *pMeta>0 ) pBt->readOnly = 1;
#endif

  
  rc = lockTable(p, 1, READ_LOCK);
  return rc;
}





int sqlite3BtreeUpdateMeta(Btree *p, int idx, u32 iMeta){
  BtShared *pBt = p->pBt;
  unsigned char *pP1;
  int rc;
  assert( idx>=1 && idx<=15 );
  if( p->inTrans!=TRANS_WRITE ){
    return pBt->readOnly ? SQLITE_READONLY : SQLITE_ERROR;
  }
  assert( pBt->pPage1!=0 );
  pP1 = pBt->pPage1->aData;
  rc = sqlite3pager_write(pP1);
  if( rc ) return rc;
  put4byte(&pP1[36 + idx*4], iMeta);
  return SQLITE_OK;
}





int sqlite3BtreeFlags(BtCursor *pCur){
  


  MemPage *pPage = pCur->pPage;
  return pPage ? pPage->aData[pPage->hdrOffset] : 0;
}

#ifdef SQLITE_DEBUG




static int btreePageDump(BtShared *pBt, int pgno, int recursive, MemPage *pParent){
  int rc;
  MemPage *pPage;
  int i, j, c;
  int nFree;
  u16 idx;
  int hdr;
  int nCell;
  int isInit;
  unsigned char *data;
  char range[20];
  unsigned char payload[20];

  rc = getPage(pBt, (Pgno)pgno, &pPage);
  isInit = pPage->isInit;
  if( pPage->isInit==0 ){
    initPage(pPage, pParent);
  }
  if( rc ){
    return rc;
  }
  hdr = pPage->hdrOffset;
  data = pPage->aData;
  c = data[hdr];
  pPage->intKey = (c & (PTF_INTKEY|PTF_LEAFDATA))!=0;
  pPage->zeroData = (c & PTF_ZERODATA)!=0;
  pPage->leafData = (c & PTF_LEAFDATA)!=0;
  pPage->leaf = (c & PTF_LEAF)!=0;
  pPage->hasData = !(pPage->zeroData || (!pPage->leaf && pPage->leafData));
  nCell = get2byte(&data[hdr+3]);
  sqlite3DebugPrintf("PAGE %d:  flags=0x%02x  frag=%d   parent=%d\n", pgno,
    data[hdr], data[hdr+7], 
    (pPage->isInit && pPage->pParent) ? pPage->pParent->pgno : 0);
  assert( hdr == (pgno==1 ? 100 : 0) );
  idx = hdr + 12 - pPage->leaf*4;
  for(i=0; i<nCell; i++){
    CellInfo info;
    Pgno child;
    unsigned char *pCell;
    int sz;
    int addr;

    addr = get2byte(&data[idx + 2*i]);
    pCell = &data[addr];
    parseCellPtr(pPage, pCell, &info);
    sz = info.nSize;
    sprintf(range,"%d..%d", addr, addr+sz-1);
    if( pPage->leaf ){
      child = 0;
    }else{
      child = get4byte(pCell);
    }
    sz = info.nData;
    if( !pPage->intKey ) sz += info.nKey;
    if( sz>sizeof(payload)-1 ) sz = sizeof(payload)-1;
    memcpy(payload, &pCell[info.nHeader], sz);
    for(j=0; j<sz; j++){
      if( payload[j]<0x20 || payload[j]>0x7f ) payload[j] = '.';
    }
    payload[sz] = 0;
    sqlite3DebugPrintf(
      "cell %2d: i=%-10s chld=%-4d nk=%-4lld nd=%-4d payload=%s\n",
      i, range, child, info.nKey, info.nData, payload
    );
  }
  if( !pPage->leaf ){
    sqlite3DebugPrintf("right_child: %d\n", get4byte(&data[hdr+8]));
  }
  nFree = 0;
  i = 0;
  idx = get2byte(&data[hdr+1]);
  while( idx>0 && idx<pPage->pBt->usableSize ){
    int sz = get2byte(&data[idx+2]);
    sprintf(range,"%d..%d", idx, idx+sz-1);
    nFree += sz;
    sqlite3DebugPrintf("freeblock %2d: i=%-10s size=%-4d total=%d\n",
       i, range, sz, nFree);
    idx = get2byte(&data[idx]);
    i++;
  }
  if( idx!=0 ){
    sqlite3DebugPrintf("ERROR: next freeblock index out of range: %d\n", idx);
  }
  if( recursive && !pPage->leaf ){
    for(i=0; i<nCell; i++){
      unsigned char *pCell = findCell(pPage, i);
      btreePageDump(pBt, get4byte(pCell), 1, pPage);
      idx = get2byte(pCell);
    }
    btreePageDump(pBt, get4byte(&data[hdr+8]), 1, pPage);
  }
  pPage->isInit = isInit;
  sqlite3pager_unref(data);
  fflush(stdout);
  return SQLITE_OK;
}
int sqlite3BtreePageDump(Btree *p, int pgno, int recursive){
  return btreePageDump(p->pBt, pgno, recursive, 0);
}
#endif

#if defined(SQLITE_TEST) && defined(SQLITE_DEBUG)

















int sqlite3BtreeCursorInfo(BtCursor *pCur, int *aResult, int upCnt){
  int cnt, idx;
  MemPage *pPage = pCur->pPage;
  BtCursor tmpCur;

  int rc = restoreOrClearCursorPosition(pCur, 1);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  pageIntegrity(pPage);
  assert( pPage->isInit );
  getTempCursor(pCur, &tmpCur);
  while( upCnt-- ){
    moveToParent(&tmpCur);
  }
  pPage = tmpCur.pPage;
  pageIntegrity(pPage);
  aResult[0] = sqlite3pager_pagenumber(pPage->aData);
  assert( aResult[0]==pPage->pgno );
  aResult[1] = tmpCur.idx;
  aResult[2] = pPage->nCell;
  if( tmpCur.idx>=0 && tmpCur.idx<pPage->nCell ){
    getCellInfo(&tmpCur);
    aResult[3] = tmpCur.info.nSize;
    aResult[6] = tmpCur.info.nData;
    aResult[7] = tmpCur.info.nHeader;
    aResult[8] = tmpCur.info.nLocal;
  }else{
    aResult[3] = 0;
    aResult[6] = 0;
    aResult[7] = 0;
    aResult[8] = 0;
  }
  aResult[4] = pPage->nFree;
  cnt = 0;
  idx = get2byte(&pPage->aData[pPage->hdrOffset+1]);
  while( idx>0 && idx<pPage->pBt->usableSize ){
    cnt++;
    idx = get2byte(&pPage->aData[idx]);
  }
  aResult[5] = cnt;
  if( pPage->pParent==0 || isRootPage(pPage) ){
    aResult[9] = 0;
  }else{
    aResult[9] = pPage->pParent->pgno;
  }
  releaseTempCursor(&tmpCur);
  return SQLITE_OK;
}
#endif





Pager *sqlite3BtreePager(Btree *p){
  return p->pBt->pPager;
}





typedef struct IntegrityCk IntegrityCk;
struct IntegrityCk {
  BtShared *pBt;    
  Pager *pPager; 
  int nPage;     
  int *anRef;    
  char *zErrMsg; 
};

#ifndef SQLITE_OMIT_INTEGRITY_CHECK



static void checkAppendMsg(
  IntegrityCk *pCheck,
  char *zMsg1,
  const char *zFormat,
  ...
){
  va_list ap;
  char *zMsg2;
  va_start(ap, zFormat);
  zMsg2 = sqlite3VMPrintf(zFormat, ap);
  va_end(ap);
  if( zMsg1==0 ) zMsg1 = "";
  if( pCheck->zErrMsg ){
    char *zOld = pCheck->zErrMsg;
    pCheck->zErrMsg = 0;
    sqlite3SetString(&pCheck->zErrMsg, zOld, "\n", zMsg1, zMsg2, (char*)0);
    sqliteFree(zOld);
  }else{
    sqlite3SetString(&pCheck->zErrMsg, zMsg1, zMsg2, (char*)0);
  }
  sqliteFree(zMsg2);
}
#endif 

#ifndef SQLITE_OMIT_INTEGRITY_CHECK








static int checkRef(IntegrityCk *pCheck, int iPage, char *zContext){
  if( iPage==0 ) return 1;
  if( iPage>pCheck->nPage || iPage<0 ){
    checkAppendMsg(pCheck, zContext, "invalid page number %d", iPage);
    return 1;
  }
  if( pCheck->anRef[iPage]==1 ){
    checkAppendMsg(pCheck, zContext, "2nd reference to page %d", iPage);
    return 1;
  }
  return  (pCheck->anRef[iPage]++)>1;
}

#ifndef SQLITE_OMIT_AUTOVACUUM





static void checkPtrmap(
  IntegrityCk *pCheck,   
  Pgno iChild,           
  u8 eType,              
  Pgno iParent,          
  char *zContext         
){
  int rc;
  u8 ePtrmapType;
  Pgno iPtrmapParent;

  rc = ptrmapGet(pCheck->pBt, iChild, &ePtrmapType, &iPtrmapParent);
  if( rc!=SQLITE_OK ){
    checkAppendMsg(pCheck, zContext, "Failed to read ptrmap key=%d", iChild);
    return;
  }

  if( ePtrmapType!=eType || iPtrmapParent!=iParent ){
    checkAppendMsg(pCheck, zContext, 
      "Bad ptr map entry key=%d expected=(%d,%d) got=(%d,%d)", 
      iChild, eType, iParent, ePtrmapType, iPtrmapParent);
  }
}
#endif





static void checkList(
  IntegrityCk *pCheck,  
  int isFreeList,       
  int iPage,            
  int N,                
  char *zContext        
){
  int i;
  int expected = N;
  int iFirst = iPage;
  while( N-- > 0 ){
    unsigned char *pOvfl;
    if( iPage<1 ){
      checkAppendMsg(pCheck, zContext,
         "%d of %d pages missing from overflow list starting at %d",
          N+1, expected, iFirst);
      break;
    }
    if( checkRef(pCheck, iPage, zContext) ) break;
    if( sqlite3pager_get(pCheck->pPager, (Pgno)iPage, (void**)&pOvfl) ){
      checkAppendMsg(pCheck, zContext, "failed to get page %d", iPage);
      break;
    }
    if( isFreeList ){
      int n = get4byte(&pOvfl[4]);
#ifndef SQLITE_OMIT_AUTOVACUUM
      if( pCheck->pBt->autoVacuum ){
        checkPtrmap(pCheck, iPage, PTRMAP_FREEPAGE, 0, zContext);
      }
#endif
      if( n>pCheck->pBt->usableSize/4-8 ){
        checkAppendMsg(pCheck, zContext,
           "freelist leaf count too big on page %d", iPage);
        N--;
      }else{
        for(i=0; i<n; i++){
          Pgno iFreePage = get4byte(&pOvfl[8+i*4]);
#ifndef SQLITE_OMIT_AUTOVACUUM
          if( pCheck->pBt->autoVacuum ){
            checkPtrmap(pCheck, iFreePage, PTRMAP_FREEPAGE, 0, zContext);
          }
#endif
          checkRef(pCheck, iFreePage, zContext);
        }
        N -= n;
      }
    }
#ifndef SQLITE_OMIT_AUTOVACUUM
    else{
      



      if( pCheck->pBt->autoVacuum && N>0 ){
        i = get4byte(pOvfl);
        checkPtrmap(pCheck, i, PTRMAP_OVERFLOW2, iPage, zContext);
      }
    }
#endif
    iPage = get4byte(pOvfl);
    sqlite3pager_unref(pOvfl);
  }
}
#endif 

#ifndef SQLITE_OMIT_INTEGRITY_CHECK


















static int checkTreePage(
  IntegrityCk *pCheck,  
  int iPage,            
  MemPage *pParent,     
  char *zParentContext  
){
  MemPage *pPage;
  int i, rc, depth, d2, pgno, cnt;
  int hdr, cellStart;
  int nCell;
  u8 *data;
  BtShared *pBt;
  int usableSize;
  char zContext[100];
  char *hit;

  sprintf(zContext, "Page %d: ", iPage);

  

  pBt = pCheck->pBt;
  usableSize = pBt->usableSize;
  if( iPage==0 ) return 0;
  if( checkRef(pCheck, iPage, zParentContext) ) return 0;
  if( (rc = getPage(pBt, (Pgno)iPage, &pPage))!=0 ){
    checkAppendMsg(pCheck, zContext,
       "unable to get the page. error code=%d", rc);
    return 0;
  }
  if( (rc = initPage(pPage, pParent))!=0 ){
    checkAppendMsg(pCheck, zContext, "initPage() returns error code %d", rc);
    releasePage(pPage);
    return 0;
  }

  

  depth = 0;
  for(i=0; i<pPage->nCell; i++){
    u8 *pCell;
    int sz;
    CellInfo info;

    

    sprintf(zContext, "On tree page %d cell %d: ", iPage, i);
    pCell = findCell(pPage,i);
    parseCellPtr(pPage, pCell, &info);
    sz = info.nData;
    if( !pPage->intKey ) sz += info.nKey;
    if( sz>info.nLocal ){
      int nPage = (sz - info.nLocal + usableSize - 5)/(usableSize - 4);
      Pgno pgnoOvfl = get4byte(&pCell[info.iOverflow]);
#ifndef SQLITE_OMIT_AUTOVACUUM
      if( pBt->autoVacuum ){
        checkPtrmap(pCheck, pgnoOvfl, PTRMAP_OVERFLOW1, iPage, zContext);
      }
#endif
      checkList(pCheck, 0, pgnoOvfl, nPage, zContext);
    }

    

    if( !pPage->leaf ){
      pgno = get4byte(pCell);
#ifndef SQLITE_OMIT_AUTOVACUUM
      if( pBt->autoVacuum ){
        checkPtrmap(pCheck, pgno, PTRMAP_BTREE, iPage, zContext);
      }
#endif
      d2 = checkTreePage(pCheck,pgno,pPage,zContext);
      if( i>0 && d2!=depth ){
        checkAppendMsg(pCheck, zContext, "Child page depth differs");
      }
      depth = d2;
    }
  }
  if( !pPage->leaf ){
    pgno = get4byte(&pPage->aData[pPage->hdrOffset+8]);
    sprintf(zContext, "On page %d at right child: ", iPage);
#ifndef SQLITE_OMIT_AUTOVACUUM
    if( pBt->autoVacuum ){
      checkPtrmap(pCheck, pgno, PTRMAP_BTREE, iPage, 0);
    }
#endif
    checkTreePage(pCheck, pgno, pPage, zContext);
  }
 
  

  data = pPage->aData;
  hdr = pPage->hdrOffset;
  hit = sqliteMalloc( usableSize );
  if( hit ){
    memset(hit, 1, get2byte(&data[hdr+5]));
    nCell = get2byte(&data[hdr+3]);
    cellStart = hdr + 12 - 4*pPage->leaf;
    for(i=0; i<nCell; i++){
      int pc = get2byte(&data[cellStart+i*2]);
      int size = cellSizePtr(pPage, &data[pc]);
      int j;
      if( (pc+size-1)>=usableSize || pc<0 ){
        checkAppendMsg(pCheck, 0, 
            "Corruption detected in cell %d on page %d",i,iPage,0);
      }else{
        for(j=pc+size-1; j>=pc; j--) hit[j]++;
      }
    }
    for(cnt=0, i=get2byte(&data[hdr+1]); i>0 && i<usableSize && cnt<10000; 
           cnt++){
      int size = get2byte(&data[i+2]);
      int j;
      if( (i+size-1)>=usableSize || i<0 ){
        checkAppendMsg(pCheck, 0,  
            "Corruption detected in cell %d on page %d",i,iPage,0);
      }else{
        for(j=i+size-1; j>=i; j--) hit[j]++;
      }
      i = get2byte(&data[i]);
    }
    for(i=cnt=0; i<usableSize; i++){
      if( hit[i]==0 ){
        cnt++;
      }else if( hit[i]>1 ){
        checkAppendMsg(pCheck, 0,
          "Multiple uses for byte %d of page %d", i, iPage);
        break;
      }
    }
    if( cnt!=data[hdr+7] ){
      checkAppendMsg(pCheck, 0, 
          "Fragmented space is %d byte reported as %d on page %d",
          cnt, data[hdr+7], iPage);
    }
  }
  sqliteFree(hit);

  releasePage(pPage);
  return depth+1;
}
#endif 

#ifndef SQLITE_OMIT_INTEGRITY_CHECK










char *sqlite3BtreeIntegrityCheck(Btree *p, int *aRoot, int nRoot){
  int i;
  int nRef;
  IntegrityCk sCheck;
  BtShared *pBt = p->pBt;

  nRef = *sqlite3pager_stats(pBt->pPager);
  if( lockBtreeWithRetry(p)!=SQLITE_OK ){
    return sqliteStrDup("Unable to acquire a read lock on the database");
  }
  sCheck.pBt = pBt;
  sCheck.pPager = pBt->pPager;
  sCheck.nPage = sqlite3pager_pagecount(sCheck.pPager);
  if( sCheck.nPage==0 ){
    unlockBtreeIfUnused(pBt);
    return 0;
  }
  sCheck.anRef = sqliteMallocRaw( (sCheck.nPage+1)*sizeof(sCheck.anRef[0]) );
  if( !sCheck.anRef ){
    unlockBtreeIfUnused(pBt);
    return sqlite3MPrintf("Unable to malloc %d bytes", 
        (sCheck.nPage+1)*sizeof(sCheck.anRef[0]));
  }
  for(i=0; i<=sCheck.nPage; i++){ sCheck.anRef[i] = 0; }
  i = PENDING_BYTE_PAGE(pBt);
  if( i<=sCheck.nPage ){
    sCheck.anRef[i] = 1;
  }
  sCheck.zErrMsg = 0;

  

  checkList(&sCheck, 1, get4byte(&pBt->pPage1->aData[32]),
            get4byte(&pBt->pPage1->aData[36]), "Main freelist: ");

  

  for(i=0; i<nRoot; i++){
    if( aRoot[i]==0 ) continue;
#ifndef SQLITE_OMIT_AUTOVACUUM
    if( pBt->autoVacuum && aRoot[i]>1 ){
      checkPtrmap(&sCheck, aRoot[i], PTRMAP_ROOTPAGE, 0, 0);
    }
#endif
    checkTreePage(&sCheck, aRoot[i], 0, "List of tree roots: ");
  }

  

  for(i=1; i<=sCheck.nPage; i++){
#ifdef SQLITE_OMIT_AUTOVACUUM
    if( sCheck.anRef[i]==0 ){
      checkAppendMsg(&sCheck, 0, "Page %d is never used", i);
    }
#else
    


    if( sCheck.anRef[i]==0 && 
       (PTRMAP_PAGENO(pBt, i)!=i || !pBt->autoVacuum) ){
      checkAppendMsg(&sCheck, 0, "Page %d is never used", i);
    }
    if( sCheck.anRef[i]!=0 && 
       (PTRMAP_PAGENO(pBt, i)==i && pBt->autoVacuum) ){
      checkAppendMsg(&sCheck, 0, "Pointer map page %d is referenced", i);
    }
#endif
  }

  

  unlockBtreeIfUnused(pBt);
  if( nRef != *sqlite3pager_stats(pBt->pPager) ){
    checkAppendMsg(&sCheck, 0, 
      "Outstanding page count goes from %d to %d during this analysis",
      nRef, *sqlite3pager_stats(pBt->pPager)
    );
  }

  

  sqliteFree(sCheck.anRef);
  return sCheck.zErrMsg;
}
#endif 




const char *sqlite3BtreeGetFilename(Btree *p){
  assert( p->pBt->pPager!=0 );
  return sqlite3pager_filename(p->pBt->pPager);
}




const char *sqlite3BtreeGetDirname(Btree *p){
  assert( p->pBt->pPager!=0 );
  return sqlite3pager_dirname(p->pBt->pPager);
}






const char *sqlite3BtreeGetJournalname(Btree *p){
  assert( p->pBt->pPager!=0 );
  return sqlite3pager_journalname(p->pBt->pPager);
}

#ifndef SQLITE_OMIT_VACUUM







int sqlite3BtreeCopyFile(Btree *pTo, Btree *pFrom){
  int rc = SQLITE_OK;
  Pgno i, nPage, nToPage, iSkip;

  BtShared *pBtTo = pTo->pBt;
  BtShared *pBtFrom = pFrom->pBt;

  if( pTo->inTrans!=TRANS_WRITE || pFrom->inTrans!=TRANS_WRITE ){
    return SQLITE_ERROR;
  }
  if( pBtTo->pCursor ) return SQLITE_BUSY;
  nToPage = sqlite3pager_pagecount(pBtTo->pPager);
  nPage = sqlite3pager_pagecount(pBtFrom->pPager);
  iSkip = PENDING_BYTE_PAGE(pBtTo);
  for(i=1; rc==SQLITE_OK && i<=nPage; i++){
    void *pPage;
    if( i==iSkip ) continue;
    rc = sqlite3pager_get(pBtFrom->pPager, i, &pPage);
    if( rc ) break;
    rc = sqlite3pager_overwrite(pBtTo->pPager, i, pPage);
    if( rc ) break;
    sqlite3pager_unref(pPage);
  }
  for(i=nPage+1; rc==SQLITE_OK && i<=nToPage; i++){
    void *pPage;
    if( i==iSkip ) continue;
    rc = sqlite3pager_get(pBtTo->pPager, i, &pPage);
    if( rc ) break;
    rc = sqlite3pager_write(pPage);
    sqlite3pager_unref(pPage);
    sqlite3pager_dont_write(pBtTo->pPager, i);
  }
  if( !rc && nPage<nToPage ){
    rc = sqlite3pager_truncate(pBtTo->pPager, nPage);
  }
  if( rc ){
    sqlite3BtreeRollback(pTo);
  }
  return rc;  
}
#endif 




int sqlite3BtreeIsInTrans(Btree *p){
  return (p && (p->inTrans==TRANS_WRITE));
}




int sqlite3BtreeIsInStmt(Btree *p){
  return (p->pBt && p->pBt->inStmt);
}















int sqlite3BtreeSync(Btree *p, const char *zMaster){
  int rc = SQLITE_OK;
  if( p->inTrans==TRANS_WRITE ){
    BtShared *pBt = p->pBt;
    Pgno nTrunc = 0;
#ifndef SQLITE_OMIT_AUTOVACUUM
    if( pBt->autoVacuum ){
      rc = autoVacuumCommit(pBt, &nTrunc); 
      if( rc!=SQLITE_OK ){
        return rc;
      }
    }
#endif
    rc = sqlite3pager_sync(pBt->pPager, zMaster, nTrunc);
  }
  return rc;
}

















void *sqlite3BtreeSchema(Btree *p, int nBytes, void(*xFree)(void *)){
  BtShared *pBt = p->pBt;
  if( !pBt->pSchema ){
    pBt->pSchema = sqliteMalloc(nBytes);
    pBt->xFreeSchema = xFree;
  }
  return pBt->pSchema;
}





int sqlite3BtreeSchemaLocked(Btree *p){
  return (queryTableLock(p, MASTER_ROOT, READ_LOCK)!=SQLITE_OK);
}


#ifndef SQLITE_OMIT_SHARED_CACHE





int sqlite3BtreeLockTable(Btree *p, int iTab, u8 isWriteLock){
  int rc = SQLITE_OK;
  u8 lockType = (isWriteLock?WRITE_LOCK:READ_LOCK);
  rc = queryTableLock(p, iTab, lockType);
  if( rc==SQLITE_OK ){
    rc = lockTable(p, iTab, lockType);
  }
  return rc;
}
#endif






#if defined(SQLITE_DEBUG) && defined(TCLSH)
#include <tcl.h>
int sqlite3_shared_cache_report(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
#ifndef SQLITE_OMIT_SHARED_CACHE
  const ThreadData *pTd = sqlite3ThreadDataReadOnly();
  if( pTd->useSharedData ){
    BtShared *pBt;
    Tcl_Obj *pRet = Tcl_NewObj();
    for(pBt=pTd->pBtree; pBt; pBt=pBt->pNext){
      const char *zFile = sqlite3pager_filename(pBt->pPager);
      Tcl_ListObjAppendElement(interp, pRet, Tcl_NewStringObj(zFile, -1));
      Tcl_ListObjAppendElement(interp, pRet, Tcl_NewIntObj(pBt->nRef));
    }
    Tcl_SetObjResult(interp, pRet);
  }
#endif
  return TCL_OK;
}
#endif
