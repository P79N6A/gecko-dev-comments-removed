





















#ifndef SQLITE_OMIT_DISKIO
#include "sqliteInt.h"
#include "os.h"
#include "pager.h"
#include <assert.h>
#include <string.h>




#if 0
#define TRACE1(X)       sqlite3DebugPrintf(X)
#define TRACE2(X,Y)     sqlite3DebugPrintf(X,Y)
#define TRACE3(X,Y,Z)   sqlite3DebugPrintf(X,Y,Z)
#define TRACE4(X,Y,Z,W) sqlite3DebugPrintf(X,Y,Z,W)
#define TRACE5(X,Y,Z,W,V) sqlite3DebugPrintf(X,Y,Z,W,V)
#else
#define TRACE1(X)
#define TRACE2(X,Y)
#define TRACE3(X,Y,Z)
#define TRACE4(X,Y,Z,W)
#define TRACE5(X,Y,Z,W,V)
#endif









#define PAGERID(p) ((int)(p->fd))
#define FILEHANDLEID(fd) ((int)fd)














































#define PAGER_UNLOCK      0
#define PAGER_SHARED      1   /* same as SHARED_LOCK */
#define PAGER_RESERVED    2   /* same as RESERVED_LOCK */
#define PAGER_EXCLUSIVE   4   /* same as EXCLUSIVE_LOCK */
#define PAGER_SYNCED      5














#ifndef SQLITE_BUSY_RESERVED_LOCK
# define SQLITE_BUSY_RESERVED_LOCK 0
#endif





#define FORCE_ALIGNMENT(X)   (((X)+7)&~7)



















typedef struct PgHdr PgHdr;
struct PgHdr {
  Pager *pPager;                 
  Pgno pgno;                     
  PgHdr *pNextHash, *pPrevHash;  
  PgHdr *pNextFree, *pPrevFree;  
  PgHdr *pNextAll;               
  PgHdr *pNextStmt, *pPrevStmt;  
  u8 inJournal;                  
  u8 inStmt;                     
  u8 dirty;                      
  u8 needSync;                   
  u8 alwaysRollback;             
  short int nRef;                
  PgHdr *pDirty, *pPrevDirty;    
  u32 notUsed;                   
#ifdef SQLITE_CHECK_PAGES
  u32 pageHash;
#endif
  
  
};












typedef struct PgHistory PgHistory;
struct PgHistory {
  u8 *pOrig;     
  u8 *pStmt;     
};




#ifdef SQLITE_HAS_CODEC
# define CODEC1(P,D,N,X) if( P->xCodec!=0 ){ P->xCodec(P->pCodecArg,D,N,X); }
# define CODEC2(P,D,N,X) ((char*)(P->xCodec!=0?P->xCodec(P->pCodecArg,D,N,X):D))
#else
# define CODEC1(P,D,N,X)
# define CODEC2(P,D,N,X) ((char*)D)
#endif





#define PGHDR_TO_DATA(P)  ((void*)(&(P)[1]))
#define DATA_TO_PGHDR(D)  (&((PgHdr*)(D))[-1])
#define PGHDR_TO_EXTRA(G,P) ((void*)&((char*)(&(G)[1]))[(P)->pageSize])
#define PGHDR_TO_HIST(P,PGR)  \
            ((PgHistory*)&((char*)(&(P)[1]))[(PGR)->pageSize+(PGR)->nExtra])












struct Pager {
  u8 journalOpen;             
  u8 journalStarted;          
  u8 useJournal;              
  u8 noReadlock;              
  u8 stmtOpen;                
  u8 stmtInUse;               
  u8 stmtAutoopen;            
  u8 noSync;                  
  u8 fullSync;                
  u8 full_fsync;              
  u8 state;                   
  u8 errCode;                 
  u8 tempFile;                
  u8 readOnly;                
  u8 needSync;                
  u8 dirtyCache;              
  u8 alwaysRollback;          
  u8 memDb;                   
  u8 setMaster;               
  int dbSize;                 
  int origDbSize;             
  int stmtSize;               
  int nRec;                   
  u32 cksumInit;              
  int stmtNRec;               
  int nExtra;                 
  int pageSize;               
  int nPage;                  
  int nMaxPage;               
  int nRef;                   
  int mxPage;                 
  u8 *aInJournal;             
  u8 *aInStmt;                
  char *zFilename;            
  char *zJournal;             
  char *zDirectory;           
  OsFile *fd, *jfd;           
  OsFile *stfd;               
  BusyHandler *pBusyHandler;  
  PgHdr *pFirst, *pLast;      
  PgHdr *pFirstSynced;        
  PgHdr *pAll;                
  PgHdr *pStmt;               
  PgHdr *pDirty;              
  i64 journalOff;             
  i64 journalHdr;             
  i64 stmtHdrOff;             
  i64 stmtCksum;              
  i64 stmtJSize;              
  int sectorSize;             
#ifdef SQLITE_TEST
  int nHit, nMiss, nOvfl;     
  int nRead,nWrite;           
#endif
  void (*xDestructor)(void*,int); 
  void (*xReiniter)(void*,int);   
  void *(*xCodec)(void*,void*,Pgno,int); 
  void *pCodecArg;            
  int nHash;                  
  PgHdr **aHash;              
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  Pager *pNext;               
#endif
};





#ifdef SQLITE_TEST
# define TEST_INCR(x)  x++
#else
# define TEST_INCR(x)
#endif
























static const unsigned char aJournalMagic[] = {
  0xd9, 0xd5, 0x05, 0xf9, 0x20, 0xa1, 0x63, 0xd7,
};





#define JOURNAL_PG_SZ(pPager)  ((pPager->pageSize) + 8)






#define JOURNAL_HDR_SZ(pPager) (pPager->sectorSize)







#ifdef SQLITE_OMIT_MEMORYDB
# define MEMDB 0
#else
# define MEMDB pPager->memDb
#endif




#define PAGER_SECTOR_SIZE 512









#define PAGER_MJ_PGNO(x) ((PENDING_BYTE/((x)->pageSize))+1)




#define PAGER_MAX_PGNO 2147483647




#ifdef SQLITE_DEBUG
  int pager3_refinfo_enable = 0;
  static void pager_refinfo(PgHdr *p){
    static int cnt = 0;
    if( !pager3_refinfo_enable ) return;
    sqlite3DebugPrintf(
       "REFCNT: %4d addr=%p nRef=%d\n",
       p->pgno, PGHDR_TO_DATA(p), p->nRef
    );
    cnt++;   
  }
# define REFINFO(X)  pager_refinfo(X)
#else
# define REFINFO(X)
#endif






static void pager_resize_hash_table(Pager *pPager, int N){
  PgHdr **aHash, *pPg;
  assert( N>0 && (N&(N-1))==0 );
  aHash = sqliteMalloc( sizeof(aHash[0])*N );
  if( aHash==0 ){
    
    return;
  }
  sqliteFree(pPager->aHash);
  pPager->nHash = N;
  pPager->aHash = aHash;
  for(pPg=pPager->pAll; pPg; pPg=pPg->pNextAll){
    int h = pPg->pgno & (N-1);
    pPg->pNextHash = aHash[h];
    if( aHash[h] ){
      aHash[h]->pPrevHash = pPg;
    }
    aHash[h] = pPg;
    pPg->pPrevHash = 0;
  }
}








static int read32bits(OsFile *fd, u32 *pRes){
  unsigned char ac[4];
  int rc = sqlite3OsRead(fd, ac, sizeof(ac));
  if( rc==SQLITE_OK ){
    *pRes = (ac[0]<<24) | (ac[1]<<16) | (ac[2]<<8) | ac[3];
  }
  return rc;
}




static void put32bits(char *ac, u32 val){
  ac[0] = (val>>24) & 0xff;
  ac[1] = (val>>16) & 0xff;
  ac[2] = (val>>8) & 0xff;
  ac[3] = val & 0xff;
}





static int write32bits(OsFile *fd, u32 val){
  char ac[4];
  put32bits(ac, val);
  return sqlite3OsWrite(fd, ac, 4);
}





static u32 retrieve32bits(PgHdr *p, int offset){
  unsigned char *ac;
  ac = &((unsigned char*)PGHDR_TO_DATA(p))[offset];
  return (ac[0]<<24) | (ac[1]<<16) | (ac[2]<<8) | ac[3];
}












static int pager_error(Pager *pPager, int rc){
  assert( pPager->errCode==SQLITE_FULL || pPager->errCode==SQLITE_OK );
  if( 
    rc==SQLITE_FULL ||
    rc==SQLITE_IOERR ||
    rc==SQLITE_CORRUPT ||
    rc==SQLITE_PROTOCOL
  ){
    pPager->errCode = rc;
  }
  return rc;
}

#ifdef SQLITE_CHECK_PAGES



static u32 pager_pagehash(PgHdr *pPage){
  u32 hash = 0;
  int i;
  unsigned char *pData = (unsigned char *)PGHDR_TO_DATA(pPage);
  for(i=0; i<pPage->pPager->pageSize; i++){
    hash = (hash+i)^pData[i];
  }
  return hash;
}






#define CHECK_PAGE(x) checkPage(x)
static void checkPage(PgHdr *pPg){
  Pager *pPager = pPg->pPager;
  assert( !pPg->pageHash || pPager->errCode || MEMDB || pPg->dirty || 
      pPg->pageHash==pager_pagehash(pPg) );
}

#else
#define CHECK_PAGE(x)
#endif











static int readMasterJournal(OsFile *pJrnl, char **pzMaster){
  int rc;
  u32 len;
  i64 szJ;
  u32 cksum;
  int i;
  unsigned char aMagic[8]; 

  *pzMaster = 0;

  rc = sqlite3OsFileSize(pJrnl, &szJ);
  if( rc!=SQLITE_OK || szJ<16 ) return rc;

  rc = sqlite3OsSeek(pJrnl, szJ-16);
  if( rc!=SQLITE_OK ) return rc;
 
  rc = read32bits(pJrnl, &len);
  if( rc!=SQLITE_OK ) return rc;

  rc = read32bits(pJrnl, &cksum);
  if( rc!=SQLITE_OK ) return rc;

  rc = sqlite3OsRead(pJrnl, aMagic, 8);
  if( rc!=SQLITE_OK || memcmp(aMagic, aJournalMagic, 8) ) return rc;

  rc = sqlite3OsSeek(pJrnl, szJ-16-len);
  if( rc!=SQLITE_OK ) return rc;

  *pzMaster = (char *)sqliteMalloc(len+1);
  if( !*pzMaster ){
    return SQLITE_NOMEM;
  }
  rc = sqlite3OsRead(pJrnl, *pzMaster, len);
  if( rc!=SQLITE_OK ){
    sqliteFree(*pzMaster);
    *pzMaster = 0;
    return rc;
  }

  
  for(i=0; i<len; i++){
    cksum -= (*pzMaster)[i];
  }
  if( cksum ){
    




    sqliteFree(*pzMaster);
    *pzMaster = 0;
  }else{
    (*pzMaster)[len] = '\0';
  }
   
  return SQLITE_OK;
}
















static int seekJournalHdr(Pager *pPager){
  i64 offset = 0;
  i64 c = pPager->journalOff;
  if( c ){
    offset = ((c-1)/JOURNAL_HDR_SZ(pPager) + 1) * JOURNAL_HDR_SZ(pPager);
  }
  assert( offset%JOURNAL_HDR_SZ(pPager)==0 );
  assert( offset>=c );
  assert( (offset-c)<JOURNAL_HDR_SZ(pPager) );
  pPager->journalOff = offset;
  return sqlite3OsSeek(pPager->jfd, pPager->journalOff);
}















static int writeJournalHdr(Pager *pPager){
  char zHeader[sizeof(aJournalMagic)+16];

  int rc = seekJournalHdr(pPager);
  if( rc ) return rc;

  pPager->journalHdr = pPager->journalOff;
  if( pPager->stmtHdrOff==0 ){
    pPager->stmtHdrOff = pPager->journalHdr;
  }
  pPager->journalOff += JOURNAL_HDR_SZ(pPager);

  







  memcpy(zHeader, aJournalMagic, sizeof(aJournalMagic));
  
  put32bits(&zHeader[sizeof(aJournalMagic)], pPager->noSync ? 0xffffffff : 0);
   
  sqlite3Randomness(sizeof(pPager->cksumInit), &pPager->cksumInit);
  put32bits(&zHeader[sizeof(aJournalMagic)+4], pPager->cksumInit);
  
  put32bits(&zHeader[sizeof(aJournalMagic)+8], pPager->dbSize);
  
  put32bits(&zHeader[sizeof(aJournalMagic)+12], pPager->sectorSize);
  rc = sqlite3OsWrite(pPager->jfd, zHeader, sizeof(zHeader));

  


  if( rc==SQLITE_OK ){
    rc = sqlite3OsSeek(pPager->jfd, pPager->journalOff-1);
    if( rc==SQLITE_OK ){
      rc = sqlite3OsWrite(pPager->jfd, "\000", 1);
    }
  }
  return rc;
}

















static int readJournalHdr(
  Pager *pPager, 
  i64 journalSize,
  u32 *pNRec, 
  u32 *pDbSize
){
  int rc;
  unsigned char aMagic[8]; 

  rc = seekJournalHdr(pPager);
  if( rc ) return rc;

  if( pPager->journalOff+JOURNAL_HDR_SZ(pPager) > journalSize ){
    return SQLITE_DONE;
  }

  rc = sqlite3OsRead(pPager->jfd, aMagic, sizeof(aMagic));
  if( rc ) return rc;

  if( memcmp(aMagic, aJournalMagic, sizeof(aMagic))!=0 ){
    return SQLITE_DONE;
  }

  rc = read32bits(pPager->jfd, pNRec);
  if( rc ) return rc;

  rc = read32bits(pPager->jfd, &pPager->cksumInit);
  if( rc ) return rc;

  rc = read32bits(pPager->jfd, pDbSize);
  if( rc ) return rc;

  





  rc = read32bits(pPager->jfd, (u32 *)&pPager->sectorSize);
  if( rc ) return rc;

  pPager->journalOff += JOURNAL_HDR_SZ(pPager);
  rc = sqlite3OsSeek(pPager->jfd, pPager->journalOff);
  return rc;
}





















static int writeMasterJournal(Pager *pPager, const char *zMaster){
  int rc;
  int len; 
  int i; 
  u32 cksum = 0;
  char zBuf[sizeof(aJournalMagic)+2*4];

  if( !zMaster || pPager->setMaster) return SQLITE_OK;
  pPager->setMaster = 1;

  len = strlen(zMaster);
  for(i=0; i<len; i++){
    cksum += zMaster[i];
  }

  



  if( pPager->fullSync ){
    rc = seekJournalHdr(pPager);
    if( rc!=SQLITE_OK ) return rc;
  }
  pPager->journalOff += (len+20);

  rc = write32bits(pPager->jfd, PAGER_MJ_PGNO(pPager));
  if( rc!=SQLITE_OK ) return rc;

  rc = sqlite3OsWrite(pPager->jfd, zMaster, len);
  if( rc!=SQLITE_OK ) return rc;

  put32bits(zBuf, len);
  put32bits(&zBuf[4], cksum);
  memcpy(&zBuf[8], aJournalMagic, sizeof(aJournalMagic));
  rc = sqlite3OsWrite(pPager->jfd, zBuf, 8+sizeof(aJournalMagic));
  pPager->needSync = !pPager->noSync;
  return rc;
}










static void page_add_to_stmt_list(PgHdr *pPg){
  Pager *pPager = pPg->pPager;
  if( pPg->inStmt ) return;
  assert( pPg->pPrevStmt==0 && pPg->pNextStmt==0 );
  pPg->pPrevStmt = 0;
  if( pPager->pStmt ){
    pPager->pStmt->pPrevStmt = pPg;
  }
  pPg->pNextStmt = pPager->pStmt;
  pPager->pStmt = pPg;
  pPg->inStmt = 1;
}
static void page_remove_from_stmt_list(PgHdr *pPg){
  if( !pPg->inStmt ) return;
  if( pPg->pPrevStmt ){
    assert( pPg->pPrevStmt->pNextStmt==pPg );
    pPg->pPrevStmt->pNextStmt = pPg->pNextStmt;
  }else{
    assert( pPg->pPager->pStmt==pPg );
    pPg->pPager->pStmt = pPg->pNextStmt;
  }
  if( pPg->pNextStmt ){
    assert( pPg->pNextStmt->pPrevStmt==pPg );
    pPg->pNextStmt->pPrevStmt = pPg->pPrevStmt;
  }
  pPg->pNextStmt = 0;
  pPg->pPrevStmt = 0;
  pPg->inStmt = 0;
}





static PgHdr *pager_lookup(Pager *pPager, Pgno pgno){
  PgHdr *p;
  if( pPager->aHash==0 ) return 0;
  p = pPager->aHash[pgno & (pPager->nHash-1)];
  while( p && p->pgno!=pgno ){
    p = p->pNextHash;
  }
  return p;
}







static void pager_reset(Pager *pPager){
  PgHdr *pPg, *pNext;
  if( pPager->errCode ) return;
  for(pPg=pPager->pAll; pPg; pPg=pNext){
    pNext = pPg->pNextAll;
    sqliteFree(pPg);
  }
  pPager->pFirst = 0;
  pPager->pFirstSynced = 0;
  pPager->pLast = 0;
  pPager->pAll = 0;
  pPager->nHash = 0;
  sqliteFree(pPager->aHash);
  pPager->nPage = 0;
  pPager->aHash = 0;
  if( pPager->state>=PAGER_RESERVED ){
    sqlite3pager_rollback(pPager);
  }
  sqlite3OsUnlock(pPager->fd, NO_LOCK);
  pPager->state = PAGER_UNLOCK;
  pPager->dbSize = -1;
  pPager->nRef = 0;
  assert( pPager->journalOpen==0 );
}











static int pager_unwritelock(Pager *pPager){
  PgHdr *pPg;
  int rc;
  assert( !MEMDB );
  if( pPager->state<PAGER_RESERVED ){
    return SQLITE_OK;
  }
  sqlite3pager_stmt_commit(pPager);
  if( pPager->stmtOpen ){
    sqlite3OsClose(&pPager->stfd);
    pPager->stmtOpen = 0;
  }
  if( pPager->journalOpen ){
    sqlite3OsClose(&pPager->jfd);
    pPager->journalOpen = 0;
    sqlite3OsDelete(pPager->zJournal);
    sqliteFree( pPager->aInJournal );
    pPager->aInJournal = 0;
    for(pPg=pPager->pAll; pPg; pPg=pPg->pNextAll){
      pPg->inJournal = 0;
      pPg->dirty = 0;
      pPg->needSync = 0;
#ifdef SQLITE_CHECK_PAGES
      pPg->pageHash = pager_pagehash(pPg);
#endif
    }
    pPager->pDirty = 0;
    pPager->dirtyCache = 0;
    pPager->nRec = 0;
  }else{
    assert( pPager->aInJournal==0 );
    assert( pPager->dirtyCache==0 || pPager->useJournal==0 );
  }
  rc = sqlite3OsUnlock(pPager->fd, SHARED_LOCK);
  pPager->state = PAGER_SHARED;
  pPager->origDbSize = 0;
  pPager->setMaster = 0;
  pPager->needSync = 0;
  pPager->pFirstSynced = pPager->pFirst;
  return rc;
}





















static u32 pager_cksum(Pager *pPager, const u8 *aData){
  u32 cksum = pPager->cksumInit;
  int i = pPager->pageSize-200;
  while( i>0 ){
    cksum += aData[i];
    i -= 200;
  }
  return cksum;
}


static void makeClean(PgHdr*);









static int pager_playback_one_page(Pager *pPager, OsFile *jfd, int useCksum){
  int rc;
  PgHdr *pPg;                   
  Pgno pgno;                    
  u32 cksum;                    
  u8 aData[SQLITE_MAX_PAGE_SIZE];  

  


  assert( jfd == (useCksum ? pPager->jfd : pPager->stfd) );


  rc = read32bits(jfd, &pgno);
  if( rc!=SQLITE_OK ) return rc;
  rc = sqlite3OsRead(jfd, &aData, pPager->pageSize);
  if( rc!=SQLITE_OK ) return rc;
  pPager->journalOff += pPager->pageSize + 4;

  




  if( pgno==0 || pgno==PAGER_MJ_PGNO(pPager) ){
    return SQLITE_DONE;
  }
  if( pgno>(unsigned)pPager->dbSize ){
    return SQLITE_OK;
  }
  if( useCksum ){
    rc = read32bits(jfd, &cksum);
    if( rc ) return rc;
    pPager->journalOff += 4;
    if( pager_cksum(pPager, aData)!=cksum ){
      return SQLITE_DONE;
    }
  }

  assert( pPager->state==PAGER_RESERVED || pPager->state>=PAGER_EXCLUSIVE );

  



















  pPg = pager_lookup(pPager, pgno);
  assert( pPager->state>=PAGER_EXCLUSIVE || pPg!=0 );
  TRACE3("PLAYBACK %d page %d\n", PAGERID(pPager), pgno);
  if( pPager->state>=PAGER_EXCLUSIVE && (pPg==0 || pPg->needSync==0) ){
    rc = sqlite3OsSeek(pPager->fd, (pgno-1)*(i64)pPager->pageSize);
    if( rc==SQLITE_OK ){
      rc = sqlite3OsWrite(pPager->fd, aData, pPager->pageSize);
    }
    if( pPg ){
      makeClean(pPg);
    }
  }
  if( pPg ){
    





    void *pData;
    
    pData = PGHDR_TO_DATA(pPg);
    memcpy(pData, aData, pPager->pageSize);
    if( pPager->xDestructor ){  
      pPager->xDestructor(pData, pPager->pageSize);
    }
#ifdef SQLITE_CHECK_PAGES
    pPg->pageHash = pager_pagehash(pPg);
#endif
    CODEC1(pPager, pData, pPg->pgno, 3);
  }
  return rc;
}












static int pager_delmaster(const char *zMaster){
  int rc;
  int master_open = 0;
  OsFile *master = 0;
  char *zMasterJournal = 0; 
  i64 nMasterJournal;       

  


  rc = sqlite3OsOpenReadOnly(zMaster, &master);
  if( rc!=SQLITE_OK ) goto delmaster_out;
  master_open = 1;
  rc = sqlite3OsFileSize(master, &nMasterJournal);
  if( rc!=SQLITE_OK ) goto delmaster_out;

  if( nMasterJournal>0 ){
    char *zJournal;
    char *zMasterPtr = 0;

    


    zMasterJournal = (char *)sqliteMalloc(nMasterJournal);
    if( !zMasterJournal ){
      rc = SQLITE_NOMEM;
      goto delmaster_out;
    }
    rc = sqlite3OsRead(master, zMasterJournal, nMasterJournal);
    if( rc!=SQLITE_OK ) goto delmaster_out;

    zJournal = zMasterJournal;
    while( (zJournal-zMasterJournal)<nMasterJournal ){
      if( sqlite3OsFileExists(zJournal) ){
        



        OsFile *journal = 0;
        int c;

        rc = sqlite3OsOpenReadOnly(zJournal, &journal);
        if( rc!=SQLITE_OK ){
          goto delmaster_out;
        }

        rc = readMasterJournal(journal, &zMasterPtr);
        sqlite3OsClose(&journal);
        if( rc!=SQLITE_OK ){
          goto delmaster_out;
        }

        c = zMasterPtr!=0 && strcmp(zMasterPtr, zMaster)==0;
        sqliteFree(zMasterPtr);
        if( c ){
          
          goto delmaster_out;
        }
      }
      zJournal += (strlen(zJournal)+1);
    }
  }
  
  sqlite3OsDelete(zMaster);

delmaster_out:
  if( zMasterJournal ){
    sqliteFree(zMasterJournal);
  }  
  if( master_open ){
    sqlite3OsClose(&master);
  }
  return rc;
}










static int pager_reload_cache(Pager *pPager){
  PgHdr *pPg;
  int rc = SQLITE_OK;
  for(pPg=pPager->pAll; pPg; pPg=pPg->pNextAll){
    char zBuf[SQLITE_MAX_PAGE_SIZE];
    if( !pPg->dirty ) continue;
    if( (int)pPg->pgno <= pPager->origDbSize ){
      rc = sqlite3OsSeek(pPager->fd, pPager->pageSize*(i64)(pPg->pgno-1));
      if( rc==SQLITE_OK ){
        rc = sqlite3OsRead(pPager->fd, zBuf, pPager->pageSize);
      }
      TRACE3("REFETCH %d page %d\n", PAGERID(pPager), pPg->pgno);
      if( rc ) break;
      CODEC1(pPager, zBuf, pPg->pgno, 2);
    }else{
      memset(zBuf, 0, pPager->pageSize);
    }
    if( pPg->nRef==0 || memcmp(zBuf, PGHDR_TO_DATA(pPg), pPager->pageSize) ){
      memcpy(PGHDR_TO_DATA(pPg), zBuf, pPager->pageSize);
      if( pPager->xReiniter ){
        pPager->xReiniter(PGHDR_TO_DATA(pPg), pPager->pageSize);
      }else{
        memset(PGHDR_TO_EXTRA(pPg, pPager), 0, pPager->nExtra);
      }
    }
    pPg->needSync = 0;
    pPg->dirty = 0;
#ifdef SQLITE_CHECK_PAGES
    pPg->pageHash = pager_pagehash(pPg);
#endif
  }
  pPager->pDirty = 0;
  return rc;
}





static int pager_truncate(Pager *pPager, int nPage){
  assert( pPager->state>=PAGER_EXCLUSIVE );
  return sqlite3OsTruncate(pPager->fd, pPager->pageSize*(i64)nPage);
}






















































static int pager_playback(Pager *pPager){
  i64 szJ;                 
  u32 nRec;                
  int i;                   
  Pgno mxPg = 0;           
  int rc;                  
  char *zMaster = 0;       

  


  assert( pPager->journalOpen );
  rc = sqlite3OsFileSize(pPager->jfd, &szJ);
  if( rc!=SQLITE_OK ){
    goto end_playback;
  }

  




  rc = readMasterJournal(pPager->jfd, &zMaster);
  assert( rc!=SQLITE_DONE );
  if( rc!=SQLITE_OK || (zMaster && !sqlite3OsFileExists(zMaster)) ){
    sqliteFree(zMaster);
    zMaster = 0;
    if( rc==SQLITE_DONE ) rc = SQLITE_OK;
    goto end_playback;
  }
  sqlite3OsSeek(pPager->jfd, 0);
  pPager->journalOff = 0;

  

  while( 1 ){

    




    rc = readJournalHdr(pPager, szJ, &nRec, &mxPg);
    if( rc!=SQLITE_OK ){ 
      if( rc==SQLITE_DONE ){
        rc = SQLITE_OK;
      }
      goto end_playback;
    }

    




    if( nRec==0xffffffff ){
      assert( pPager->journalOff==JOURNAL_HDR_SZ(pPager) );
      nRec = (szJ - JOURNAL_HDR_SZ(pPager))/JOURNAL_PG_SZ(pPager);
    }

    


    if( pPager->state>=PAGER_EXCLUSIVE && 
        pPager->journalOff==JOURNAL_HDR_SZ(pPager) ){
      assert( pPager->origDbSize==0 || pPager->origDbSize==mxPg );
      rc = pager_truncate(pPager, mxPg);
      if( rc!=SQLITE_OK ){
        goto end_playback;
      }
      pPager->dbSize = mxPg;
    }

    

    for(i=0; i<nRec; i++){
      rc = pager_playback_one_page(pPager, pPager->jfd, 1);
      if( rc!=SQLITE_OK ){
        if( rc==SQLITE_DONE ){
          rc = SQLITE_OK;
          pPager->journalOff = szJ;
          break;
        }else{
          goto end_playback;
        }
      }
    }
  }
  
  assert( 0 );

end_playback:
  if( rc==SQLITE_OK ){
    rc = pager_unwritelock(pPager);
  }
  if( zMaster ){
    


    if( rc==SQLITE_OK ){
      rc = pager_delmaster(zMaster);
    }
    sqliteFree(zMaster);
  }

  



  pPager->sectorSize = PAGER_SECTOR_SIZE;
  return rc;
}















static int pager_stmt_playback(Pager *pPager){
  i64 szJ;                 
  i64 hdrOff;
  int nRec;                
  int i;                   
  int rc;

  szJ = pPager->journalOff;
#ifndef NDEBUG 
  {
    i64 os_szJ;
    rc = sqlite3OsFileSize(pPager->jfd, &os_szJ);
    if( rc!=SQLITE_OK ) return rc;
    assert( szJ==os_szJ );
  }
#endif

  



  hdrOff = pPager->stmtHdrOff;
  assert( pPager->fullSync || !hdrOff );
  if( !hdrOff ){
    hdrOff = szJ;
  }
  
  

  if( pPager->state>=PAGER_EXCLUSIVE ){
    rc = pager_truncate(pPager, pPager->stmtSize);
  }
  pPager->dbSize = pPager->stmtSize;

  

  assert( pPager->stmtInUse && pPager->journalOpen );
  sqlite3OsSeek(pPager->stfd, 0);
  nRec = pPager->stmtNRec;
  
  




  for(i=nRec-1; i>=0; i--){
    rc = pager_playback_one_page(pPager, pPager->stfd, 0);
    assert( rc!=SQLITE_DONE );
    if( rc!=SQLITE_OK ) goto end_stmt_playback;
  }

  







  rc = sqlite3OsSeek(pPager->jfd, pPager->stmtJSize);
  if( rc!=SQLITE_OK ){
    goto end_stmt_playback;
  }
  pPager->journalOff = pPager->stmtJSize;
  pPager->cksumInit = pPager->stmtCksum;
  assert( JOURNAL_HDR_SZ(pPager)<(pPager->pageSize+8) );
  while( pPager->journalOff <= (hdrOff-(pPager->pageSize+8)) ){
    rc = pager_playback_one_page(pPager, pPager->jfd, 1);
    assert( rc!=SQLITE_DONE );
    if( rc!=SQLITE_OK ) goto end_stmt_playback;
  }

  while( pPager->journalOff < szJ ){
    u32 nJRec;         
    u32 dummy;
    rc = readJournalHdr(pPager, szJ, &nJRec, &dummy);
    if( rc!=SQLITE_OK ){
      assert( rc!=SQLITE_DONE );
      goto end_stmt_playback;
    }
    if( nJRec==0 ){
      nJRec = (szJ - pPager->journalOff) / (pPager->pageSize+8);
    }
    for(i=nJRec-1; i>=0 && pPager->journalOff < szJ; i--){
      rc = pager_playback_one_page(pPager, pPager->jfd, 1);
      assert( rc!=SQLITE_DONE );
      if( rc!=SQLITE_OK ) goto end_stmt_playback;
    }
  }

  pPager->journalOff = szJ;
  
end_stmt_playback:
  if( rc==SQLITE_OK) {
    pPager->journalOff = szJ;
    
  }
  return rc;
}




void sqlite3pager_set_cachesize(Pager *pPager, int mxPage){
  if( mxPage>10 ){
    pPager->mxPage = mxPage;
  }else{
    pPager->mxPage = 10;
  }
}



























#ifndef SQLITE_OMIT_PAGER_PRAGMAS
void sqlite3pager_set_safety_level(Pager *pPager, int level, int full_fsync){
  pPager->noSync =  level==1 || pPager->tempFile;
  pPager->fullSync = level==3 && !pPager->tempFile;
  pPager->full_fsync = full_fsync;
  if( pPager->noSync ) pPager->needSync = 0;
}
#endif






int sqlite3_opentemp_count = 0;










static int sqlite3pager_opentemp(char *zFile, OsFile **pFd){
  int cnt = 8;
  int rc;
  sqlite3_opentemp_count++;  
  do{
    cnt--;
    sqlite3OsTempFileName(zFile);
    rc = sqlite3OsOpenExclusive(zFile, pFd, 1);
  }while( cnt>0 && rc!=SQLITE_OK && rc!=SQLITE_NOMEM );
  return rc;
}















int sqlite3pager_open(
  Pager **ppPager,         
  const char *zFilename,   
  int nExtra,              
  int flags                
){
  Pager *pPager = 0;
  char *zFullPathname = 0;
  int nameLen;  
  OsFile *fd;
  int rc = SQLITE_OK;
  int i;
  int tempFile = 0;
  int memDb = 0;
  int readOnly = 0;
  int useJournal = (flags & PAGER_OMIT_JOURNAL)==0;
  int noReadlock = (flags & PAGER_NO_READLOCK)!=0;
  char zTemp[SQLITE_TEMPNAME_SIZE];
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  






  ThreadData *pTsd = sqlite3ThreadData();
  assert( pTsd );
#endif

  



  *ppPager = 0;
  if( sqlite3MallocFailed() ){
    return SQLITE_NOMEM;
  }
  memset(&fd, 0, sizeof(fd));

  


  if( zFilename && zFilename[0] ){
#ifndef SQLITE_OMIT_MEMORYDB
    if( strcmp(zFilename,":memory:")==0 ){
      memDb = 1;
      zFullPathname = sqliteStrDup("");
    }else
#endif
    {
      zFullPathname = sqlite3OsFullPathname(zFilename);
      if( zFullPathname ){
        rc = sqlite3OsOpenReadWrite(zFullPathname, &fd, &readOnly);
      }
    }
  }else{
    rc = sqlite3pager_opentemp(zTemp, &fd);
    zFilename = zTemp;
    zFullPathname = sqlite3OsFullPathname(zFilename);
    if( rc==SQLITE_OK ){
      tempFile = 1;
    }
  }

  



  if( zFullPathname ){
    nameLen = strlen(zFullPathname);
    pPager = sqliteMalloc( sizeof(*pPager) + nameLen*3 + 30 );
  }

  




  if( !pPager || !zFullPathname || rc!=SQLITE_OK ){
    sqlite3OsClose(&fd);
    sqliteFree(zFullPathname);
    sqliteFree(pPager);
    return ((rc==SQLITE_OK)?SQLITE_NOMEM:rc);
  }

  TRACE3("OPEN %d %s\n", FILEHANDLEID(fd), zFullPathname);
  pPager->zFilename = (char*)&pPager[1];
  pPager->zDirectory = &pPager->zFilename[nameLen+1];
  pPager->zJournal = &pPager->zDirectory[nameLen+1];
  strcpy(pPager->zFilename, zFullPathname);
  strcpy(pPager->zDirectory, zFullPathname);

  for(i=nameLen; i>0 && pPager->zDirectory[i-1]!='/'; i--){}
  if( i>0 ) pPager->zDirectory[i-1] = 0;
  strcpy(pPager->zJournal, zFullPathname);
  sqliteFree(zFullPathname);
  strcpy(&pPager->zJournal[nameLen], "-journal");
  pPager->fd = fd;
  
  pPager->useJournal = useJournal && !memDb;
  pPager->noReadlock = noReadlock && readOnly;
  
  
  
  pPager->dbSize = memDb-1;
  pPager->pageSize = SQLITE_DEFAULT_PAGE_SIZE;
  
  
  
  
  pPager->mxPage = 100;
  assert( PAGER_UNLOCK==0 );
  
  
  pPager->tempFile = tempFile;
  pPager->memDb = memDb;
  pPager->readOnly = readOnly;
  
  pPager->noSync = pPager->tempFile || !useJournal;
  pPager->fullSync = (pPager->noSync?0:1);
  
  
  
  pPager->nExtra = FORCE_ALIGNMENT(nExtra);
  pPager->sectorSize = PAGER_SECTOR_SIZE;
  
  
  *ppPager = pPager;
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  pPager->pNext = pTsd->pPager;
  pTsd->pPager = pPager;
#endif
  return SQLITE_OK;
}




void sqlite3pager_set_busyhandler(Pager *pPager, BusyHandler *pBusyHandler){
  pPager->pBusyHandler = pBusyHandler;
}









void sqlite3pager_set_destructor(Pager *pPager, void (*xDesc)(void*,int)){
  pPager->xDestructor = xDesc;
}








void sqlite3pager_set_reiniter(Pager *pPager, void (*xReinit)(void*,int)){
  pPager->xReiniter = xReinit;
}






int sqlite3pager_set_pagesize(Pager *pPager, int pageSize){
  assert( pageSize>=512 && pageSize<=SQLITE_MAX_PAGE_SIZE );
  if( !pPager->memDb ){
    pPager->pageSize = pageSize;
  }
  return pPager->pageSize;
}









#ifdef SQLITE_TEST
extern int sqlite3_io_error_pending;
extern int sqlite3_io_error_hit;
static int saved_cnt;
void clear_simulated_io_error(){
  sqlite3_io_error_hit = 0;
}
void disable_simulated_io_errors(void){
  saved_cnt = sqlite3_io_error_pending;
  sqlite3_io_error_pending = -1;
}
void enable_simulated_io_errors(void){
  sqlite3_io_error_pending = saved_cnt;
}
#else
# define clear_simulated_io_error()
# define disable_simulated_io_errors()
# define enable_simulated_io_errors()
#endif











void sqlite3pager_read_fileheader(Pager *pPager, int N, unsigned char *pDest){
  memset(pDest, 0, N);
  if( MEMDB==0 ){
    disable_simulated_io_errors();
    sqlite3OsSeek(pPager->fd, 0);
    sqlite3OsRead(pPager->fd, pDest, N);
    enable_simulated_io_errors();
  }
}










int sqlite3pager_pagecount(Pager *pPager){
  i64 n;
  assert( pPager!=0 );
  if( pPager->dbSize>=0 ){
    n = pPager->dbSize;
  } else {
    if( sqlite3OsFileSize(pPager->fd, &n)!=SQLITE_OK ){
      pager_error(pPager, SQLITE_IOERR);
      return 0;
    }
    if( n>0 && n<pPager->pageSize ){
      n = 1;
    }else{
      n /= pPager->pageSize;
    }
    if( pPager->state!=PAGER_UNLOCK ){
      pPager->dbSize = n;
    }
  }
  if( n==(PENDING_BYTE/pPager->pageSize) ){
    n++;
  }
  return n;
}


#ifndef SQLITE_OMIT_MEMORYDB



static void clearHistory(PgHistory *pHist){
  sqliteFree(pHist->pOrig);
  sqliteFree(pHist->pStmt);
  pHist->pOrig = 0;
  pHist->pStmt = 0;
}
#else
#define clearHistory(x)
#endif




static int syncJournal(Pager*);







static void unlinkHashChain(Pager *pPager, PgHdr *pPg){
  if( pPg->pgno==0 ){
    
    return;
  }
  if( pPg->pNextHash ){
    pPg->pNextHash->pPrevHash = pPg->pPrevHash;
  }
  if( pPg->pPrevHash ){
    assert( pPager->aHash[pPg->pgno & (pPager->nHash-1)]!=pPg );
    pPg->pPrevHash->pNextHash = pPg->pNextHash;
  }else{
    int h = pPg->pgno & (pPager->nHash-1);
    assert( pPager->aHash[h]==pPg );
    pPager->aHash[h] = pPg->pNextHash;
  }
  if( MEMDB ){
    clearHistory(PGHDR_TO_HIST(pPg, pPager));
  }
  pPg->pgno = 0;
  pPg->pNextHash = pPg->pPrevHash = 0;
}





static void unlinkPage(PgHdr *pPg){
  Pager *pPager = pPg->pPager;

  
  if( pPg==pPager->pFirstSynced ){
    PgHdr *p = pPg->pNextFree;
    while( p && p->needSync ){ p = p->pNextFree; }
    pPager->pFirstSynced = p;
  }

  
  if( pPg->pPrevFree ){
    pPg->pPrevFree->pNextFree = pPg->pNextFree;
  }else{
    assert( pPager->pFirst==pPg );
    pPager->pFirst = pPg->pNextFree;
  }
  if( pPg->pNextFree ){
    pPg->pNextFree->pPrevFree = pPg->pPrevFree;
  }else{
    assert( pPager->pLast==pPg );
    pPager->pLast = pPg->pPrevFree;
  }
  pPg->pNextFree = pPg->pPrevFree = 0;

  
  unlinkHashChain(pPager, pPg);
}

#ifndef SQLITE_OMIT_MEMORYDB





static void memoryTruncate(Pager *pPager){
  PgHdr *pPg;
  PgHdr **ppPg;
  int dbSize = pPager->dbSize;

  ppPg = &pPager->pAll;
  while( (pPg = *ppPg)!=0 ){
    if( pPg->pgno<=dbSize ){
      ppPg = &pPg->pNextAll;
    }else if( pPg->nRef>0 ){
      memset(PGHDR_TO_DATA(pPg), 0, pPager->pageSize);
      ppPg = &pPg->pNextAll;
    }else{
      *ppPg = pPg->pNextAll;
      unlinkPage(pPg);
      makeClean(pPg);
      sqliteFree(pPg);
      pPager->nPage--;
    }
  }
}
#else
#define memoryTruncate(p)
#endif









static int pager_wait_on_lock(Pager *pPager, int locktype){
  int rc;
  assert( PAGER_SHARED==SHARED_LOCK );
  assert( PAGER_RESERVED==RESERVED_LOCK );
  assert( PAGER_EXCLUSIVE==EXCLUSIVE_LOCK );
  if( pPager->state>=locktype ){
    rc = SQLITE_OK;
  }else{
    do {
      rc = sqlite3OsLock(pPager->fd, locktype);
    }while( rc==SQLITE_BUSY && sqlite3InvokeBusyHandler(pPager->pBusyHandler) );
    if( rc==SQLITE_OK ){
      pPager->state = locktype;
    }
  }
  return rc;
}




int sqlite3pager_truncate(Pager *pPager, Pgno nPage){
  int rc;
  sqlite3pager_pagecount(pPager);
  if( pPager->errCode ){
    rc = pPager->errCode;
    return rc;
  }
  if( nPage>=(unsigned)pPager->dbSize ){
    return SQLITE_OK;
  }
  if( MEMDB ){
    pPager->dbSize = nPage;
    memoryTruncate(pPager);
    return SQLITE_OK;
  }
  rc = syncJournal(pPager);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  
  rc = pager_wait_on_lock(pPager, EXCLUSIVE_LOCK);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  rc = pager_truncate(pPager, nPage);
  if( rc==SQLITE_OK ){
    pPager->dbSize = nPage;
  }
  return rc;
}















int sqlite3pager_close(Pager *pPager){
  PgHdr *pPg, *pNext;
#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  




  ThreadData *pTsd = sqlite3ThreadData();
  assert( pPager );
  assert( pTsd && pTsd->nAlloc );
#endif

  switch( pPager->state ){
    case PAGER_RESERVED:
    case PAGER_SYNCED: 
    case PAGER_EXCLUSIVE: {
      



      disable_simulated_io_errors();
      sqlite3pager_rollback(pPager);
      enable_simulated_io_errors();
      if( !MEMDB ){
        sqlite3OsUnlock(pPager->fd, NO_LOCK);
      }
      assert( pPager->errCode || pPager->journalOpen==0 );
      break;
    }
    case PAGER_SHARED: {
      if( !MEMDB ){
        sqlite3OsUnlock(pPager->fd, NO_LOCK);
      }
      break;
    }
    default: {
      
      break;
    }
  }
  for(pPg=pPager->pAll; pPg; pPg=pNext){
#ifndef NDEBUG
    if( MEMDB ){
      PgHistory *pHist = PGHDR_TO_HIST(pPg, pPager);
      assert( !pPg->alwaysRollback );
      assert( !pHist->pOrig );
      assert( !pHist->pStmt );
    }
#endif
    pNext = pPg->pNextAll;
    sqliteFree(pPg);
  }
  TRACE2("CLOSE %d\n", PAGERID(pPager));
  assert( pPager->errCode || (pPager->journalOpen==0 && pPager->stmtOpen==0) );
  if( pPager->journalOpen ){
    sqlite3OsClose(&pPager->jfd);
  }
  sqliteFree(pPager->aInJournal);
  if( pPager->stmtOpen ){
    sqlite3OsClose(&pPager->stfd);
  }
  sqlite3OsClose(&pPager->fd);
  





#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  


  if( pPager==pTsd->pPager ){
    pTsd->pPager = pPager->pNext;
  }else{
    Pager *pTmp;
    for(pTmp = pTsd->pPager; pTmp->pNext!=pPager; pTmp=pTmp->pNext){}
    pTmp->pNext = pPager->pNext;
  }
#endif
  sqliteFree(pPager->aHash);
  sqliteFree(pPager);
  return SQLITE_OK;
}




Pgno sqlite3pager_pagenumber(void *pData){
  PgHdr *p = DATA_TO_PGHDR(pData);
  return p->pgno;
}










static void _page_ref(PgHdr *pPg){
  if( pPg->nRef==0 ){
    
    if( pPg==pPg->pPager->pFirstSynced ){
      PgHdr *p = pPg->pNextFree;
      while( p && p->needSync ){ p = p->pNextFree; }
      pPg->pPager->pFirstSynced = p;
    }
    if( pPg->pPrevFree ){
      pPg->pPrevFree->pNextFree = pPg->pNextFree;
    }else{
      pPg->pPager->pFirst = pPg->pNextFree;
    }
    if( pPg->pNextFree ){
      pPg->pNextFree->pPrevFree = pPg->pPrevFree;
    }else{
      pPg->pPager->pLast = pPg->pPrevFree;
    }
    pPg->pPager->nRef++;
  }
  pPg->nRef++;
  REFINFO(pPg);
}
#ifdef SQLITE_DEBUG
  static void page_ref(PgHdr *pPg){
    if( pPg->nRef==0 ){
      _page_ref(pPg);
    }else{
      pPg->nRef++;
      REFINFO(pPg);
    }
  }
#else
# define page_ref(P)   ((P)->nRef==0?_page_ref(P):(void)(P)->nRef++)
#endif





int sqlite3pager_ref(void *pData){
  PgHdr *pPg = DATA_TO_PGHDR(pData);
  page_ref(pPg);
  return SQLITE_OK;
}





















static int syncJournal(Pager *pPager){
  PgHdr *pPg;
  int rc = SQLITE_OK;

  


  if( pPager->needSync ){
    if( !pPager->tempFile ){
      assert( pPager->journalOpen );
      

#ifndef NDEBUG
      {
        


        i64 jSz;
        rc = sqlite3OsFileSize(pPager->jfd, &jSz);
        if( rc!=0 ) return rc;
        assert( pPager->journalOff==jSz );
      }
#endif
      {
        




        if( pPager->fullSync ){
          TRACE2("SYNC journal of %d\n", PAGERID(pPager));
          rc = sqlite3OsSync(pPager->jfd, 0);
          if( rc!=0 ) return rc;
        }
        rc = sqlite3OsSeek(pPager->jfd,
                           pPager->journalHdr + sizeof(aJournalMagic));
        if( rc ) return rc;
        rc = write32bits(pPager->jfd, pPager->nRec);
        if( rc ) return rc;

        rc = sqlite3OsSeek(pPager->jfd, pPager->journalOff);
        if( rc ) return rc;
      }
      TRACE2("SYNC journal of %d\n", PAGERID(pPager));
      rc = sqlite3OsSync(pPager->jfd, pPager->full_fsync);
      if( rc!=0 ) return rc;
      pPager->journalStarted = 1;
    }
    pPager->needSync = 0;

    

    for(pPg=pPager->pAll; pPg; pPg=pPg->pNextAll){
      pPg->needSync = 0;
    }
    pPager->pFirstSynced = pPager->pFirst;
  }

#ifndef NDEBUG
  



  else{
    for(pPg=pPager->pAll; pPg; pPg=pPg->pNextAll){
      assert( pPg->needSync==0 );
    }
    assert( pPager->pFirstSynced==pPager->pFirst );
  }
#endif

  return rc;
}






static int pager_write_pagelist(PgHdr *pList){
  Pager *pPager;
  int rc;

  if( pList==0 ) return SQLITE_OK;
  pPager = pList->pPager;

  















  rc = pager_wait_on_lock(pPager, EXCLUSIVE_LOCK);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  while( pList ){
    assert( pList->dirty );
    rc = sqlite3OsSeek(pPager->fd, (pList->pgno-1)*(i64)pPager->pageSize);
    if( rc ) return rc;
    




    if( pList->pgno<=pPager->dbSize ){
      char *pData = CODEC2(pPager, PGHDR_TO_DATA(pList), pList->pgno, 6);
      TRACE3("STORE %d page %d\n", PAGERID(pPager), pList->pgno);
      rc = sqlite3OsWrite(pPager->fd, pData, pPager->pageSize);
      TEST_INCR(pPager->nWrite);
    }
#ifndef NDEBUG
    else{
      TRACE3("NOSTORE %d page %d\n", PAGERID(pPager), pList->pgno);
    }
#endif
    if( rc ) return rc;
    pList->dirty = 0;
#ifdef SQLITE_CHECK_PAGES
    pList->pageHash = pager_pagehash(pList);
#endif
    pList = pList->pDirty;
  }
  return SQLITE_OK;
}






static PgHdr *pager_get_all_dirty_pages(Pager *pPager){
  return pPager->pDirty;
}









static int hasHotJournal(Pager *pPager){
  if( !pPager->useJournal ) return 0;
  if( !sqlite3OsFileExists(pPager->zJournal) ) return 0;
  if( sqlite3OsCheckReservedLock(pPager->fd) ) return 0;
  if( sqlite3pager_pagecount(pPager)==0 ){
    sqlite3OsDelete(pPager->zJournal);
    return 0;
  }else{
    return 1;
  }
}







static int pager_recycle(Pager *pPager, int syncOk, PgHdr **ppPg){
  PgHdr *pPg;
  *ppPg = 0;

  


  pPg = pPager->pFirstSynced;

  




  if( pPg==0 && pPager->pFirst && syncOk && !MEMDB){
    int rc = syncJournal(pPager);
    if( rc!=0 ){
      return rc;
    }
    if( pPager->fullSync ){
      





      pPager->nRec = 0;
      assert( pPager->journalOff > 0 );
      rc = writeJournalHdr(pPager);
      if( rc!=0 ){
        return rc;
      }
    }
    pPg = pPager->pFirst;
  }
  if( pPg==0 ){
    return SQLITE_OK;
  }

  assert( pPg->nRef==0 );

  

  if( pPg->dirty ){
    int rc;
    assert( pPg->needSync==0 );
    makeClean(pPg);
    pPg->dirty = 1;
    pPg->pDirty = 0;
    rc = pager_write_pagelist( pPg );
    if( rc!=SQLITE_OK ){
      return rc;
    }
  }
  assert( pPg->dirty==0 );

  







  if( pPg->alwaysRollback ){
    pPager->alwaysRollback = 1;
  }

  

  unlinkPage(pPg);
  TEST_INCR(pPager->nOvfl);

  *ppPg = pPg;
  return SQLITE_OK;
}











#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
int sqlite3pager_release_memory(int nReq){
  const ThreadData *pTsdro = sqlite3ThreadDataReadOnly();
  Pager *p;
  int nReleased = 0;
  int i;

  




  if( sqlite3OsInMutex(0) ){
    return 0;
  }

  





  for(i=0; i<=1; i++){

    
    for(p=pTsdro->pPager; p && (nReq<0 || nReleased<nReq); p=p->pNext){
      PgHdr *pPg;
      int rc;

      



      while( SQLITE_OK==(rc = pager_recycle(p, i, &pPg)) && pPg) {
        







        PgHdr *pTmp;
        assert( pPg );
        page_remove_from_stmt_list(pPg);
        if( pPg==p->pAll ){
           p->pAll = pPg->pNextAll;
        }else{
          for( pTmp=p->pAll; pTmp->pNextAll!=pPg; pTmp=pTmp->pNextAll ){}
          pTmp->pNextAll = pPg->pNextAll;
        }
        nReleased += sqliteAllocSize(pPg);
        sqliteFree(pPg);
      }

      if( rc!=SQLITE_OK ){
        





        assert( rc==SQLITE_IOERR || rc==SQLITE_FULL );
        assert( p->state>=PAGER_RESERVED );
        pager_error(p, rc);
      }
    }
  }

  return nReleased;
}
#endif 
























int sqlite3pager_get(Pager *pPager, Pgno pgno, void **ppPage){
  
  return sqlite3pager_get2(pPager, pgno, ppPage, 0);
}









int sqlite3pager_get2(Pager *pPager, Pgno pgno, void **ppPage,
                      unsigned char* pDataToFill) {
  PgHdr *pPg;
  int rc;

  


  if( pgno>PAGER_MAX_PGNO || pgno==0 || pgno==PAGER_MJ_PGNO(pPager) ){
    return SQLITE_CORRUPT_BKPT;
  }

  
 
  assert( pPager!=0 );
  *ppPage = 0;
  if( pPager->errCode && pPager->errCode!=SQLITE_FULL ){
    return pPager->errCode;
  }

  


  if( pPager->nRef==0 && !MEMDB ){
    if( !pPager->noReadlock ){
      rc = pager_wait_on_lock(pPager, SHARED_LOCK);
      if( rc!=SQLITE_OK ){
        return pager_error(pPager, rc);
      }
    }

    


    if( hasHotJournal(pPager) ){
       










       rc = sqlite3OsLock(pPager->fd, EXCLUSIVE_LOCK);
       if( rc!=SQLITE_OK ){
         sqlite3OsUnlock(pPager->fd, NO_LOCK);
         pPager->state = PAGER_UNLOCK;
         return pager_error(pPager, rc);
       }
       pPager->state = PAGER_EXCLUSIVE;

       







       rc = sqlite3OsOpenReadOnly(pPager->zJournal, &pPager->jfd);
       if( rc!=SQLITE_OK ){
         sqlite3OsUnlock(pPager->fd, NO_LOCK);
         pPager->state = PAGER_UNLOCK;
         return SQLITE_BUSY;
       }
       pPager->journalOpen = 1;
       pPager->journalStarted = 0;
       pPager->journalOff = 0;
       pPager->setMaster = 0;
       pPager->journalHdr = 0;

       


       rc = pager_playback(pPager);
       if( rc!=SQLITE_OK ){
         return pager_error(pPager, rc);
       }
    }
    pPg = 0;
  }else{
    
    pPg = pager_lookup(pPager, pgno);
    if( MEMDB && pPager->state==PAGER_UNLOCK ){
      pPager->state = PAGER_SHARED;
    }
  }
  if( pPg==0 ){
    
    int h;
    TEST_INCR(pPager->nMiss);
    if( pPager->nPage<pPager->mxPage || pPager->pFirst==0 || MEMDB ){
      
      if( pPager->nPage>=pPager->nHash ){
        pager_resize_hash_table(pPager,
           pPager->nHash<256 ? 256 : pPager->nHash*2);
        if( pPager->nHash==0 ){
          return SQLITE_NOMEM;
        }
      }
      pPg = sqliteMallocRaw( sizeof(*pPg) + pPager->pageSize
                              + sizeof(u32) + pPager->nExtra
                              + MEMDB*sizeof(PgHistory) );
      if( pPg==0 ){
        return SQLITE_NOMEM;
      }
      memset(pPg, 0, sizeof(*pPg));
      if( MEMDB ){
        memset(PGHDR_TO_HIST(pPg, pPager), 0, sizeof(PgHistory));
      }
      pPg->pPager = pPager;
      pPg->pNextAll = pPager->pAll;
      pPager->pAll = pPg;
      pPager->nPage++;
      if( pPager->nPage>pPager->nMaxPage ){
        assert( pPager->nMaxPage==(pPager->nPage-1) );
        pPager->nMaxPage++;
      }
    }else{
      rc = pager_recycle(pPager, 1, &pPg);
      if( rc!=SQLITE_OK ){
        return rc;
      }
      assert(pPg) ;
    }
    pPg->pgno = pgno;
    if( pPager->aInJournal && (int)pgno<=pPager->origDbSize ){
      sqlite3CheckMemory(pPager->aInJournal, pgno/8);
      assert( pPager->journalOpen );
      pPg->inJournal = (pPager->aInJournal[pgno/8] & (1<<(pgno&7)))!=0;
      pPg->needSync = 0;
    }else{
      pPg->inJournal = 0;
      pPg->needSync = 0;
    }
    if( pPager->aInStmt && (int)pgno<=pPager->stmtSize
             && (pPager->aInStmt[pgno/8] & (1<<(pgno&7)))!=0 ){
      page_add_to_stmt_list(pPg);
    }else{
      page_remove_from_stmt_list(pPg);
    }
    makeClean(pPg);
    pPg->nRef = 1;
    REFINFO(pPg);

    pPager->nRef++;
    if( pPager->nExtra>0 ){
      memset(PGHDR_TO_EXTRA(pPg, pPager), 0, pPager->nExtra);
    }
    if( pPager->errCode ){
      sqlite3pager_unref(PGHDR_TO_DATA(pPg));
      rc = pPager->errCode;
      return rc;
    }

    


    if( sqlite3pager_pagecount(pPager)<(int)pgno || MEMDB ){
      memset(PGHDR_TO_DATA(pPg), 0, pPager->pageSize);
    }else{
      if (pDataToFill) {
        
        memcpy(PGHDR_TO_DATA(pPg), pDataToFill, pPager->pageSize);
        CODEC1(pPager, PGHDR_TO_DATA(pPg), pPg->pgno, 3);
      } else {
        
        assert( MEMDB==0 );
        rc = sqlite3OsSeek(pPager->fd, (pgno-1)*(i64)pPager->pageSize);
        if( rc==SQLITE_OK ){
          rc = sqlite3OsRead(pPager->fd, PGHDR_TO_DATA(pPg),
                                pPager->pageSize);
        }
        TRACE3("FETCH %d page %d\n", PAGERID(pPager), pPg->pgno);
        CODEC1(pPager, PGHDR_TO_DATA(pPg), pPg->pgno, 3);
        if( rc!=SQLITE_OK ){
          i64 fileSize;
          int rc2 = sqlite3OsFileSize(pPager->fd, &fileSize);
          if( rc2!=SQLITE_OK || fileSize>=pgno*pPager->pageSize ){
            

            pPg->pgno = 0;
            sqlite3pager_unref(PGHDR_TO_DATA(pPg));
            return rc;
          }else{
            clear_simulated_io_error();
            memset(PGHDR_TO_DATA(pPg), 0, pPager->pageSize);
          }
        }else{
          TEST_INCR(pPager->nRead);
        }
      }
    }

    
    h = pgno & (pPager->nHash-1);
    pPg->pNextHash = pPager->aHash[h];
    pPager->aHash[h] = pPg;
    if( pPg->pNextHash ){
      assert( pPg->pNextHash->pPrevHash==0 );
      pPg->pNextHash->pPrevHash = pPg;
    }

#ifdef SQLITE_CHECK_PAGES
    pPg->pageHash = pager_pagehash(pPg);
#endif
  }else{
    
    TEST_INCR(pPager->nHit);
    page_ref(pPg);
  }
  *ppPage = PGHDR_TO_DATA(pPg);
  return SQLITE_OK;
}












void *sqlite3pager_lookup(Pager *pPager, Pgno pgno){
  PgHdr *pPg;

  assert( pPager!=0 );
  assert( pgno!=0 );
  if( pPager->errCode && pPager->errCode!=SQLITE_FULL ){
    return 0;
  }
  pPg = pager_lookup(pPager, pgno);
  if( pPg==0 ) return 0;
  page_ref(pPg);
  return PGHDR_TO_DATA(pPg);
}









int sqlite3pager_unref(void *pData){
  PgHdr *pPg;

  

  pPg = DATA_TO_PGHDR(pData);
  assert( pPg->nRef>0 );
  pPg->nRef--;
  REFINFO(pPg);

  CHECK_PAGE(pPg);

  


  if( pPg->nRef==0 ){
    Pager *pPager;
    pPager = pPg->pPager;
    pPg->pNextFree = 0;
    pPg->pPrevFree = pPager->pLast;
    pPager->pLast = pPg;
    if( pPg->pPrevFree ){
      pPg->pPrevFree->pNextFree = pPg;
    }else{
      pPager->pFirst = pPg;
    }
    if( pPg->needSync==0 && pPager->pFirstSynced==0 ){
      pPager->pFirstSynced = pPg;
    }
    if( pPager->xDestructor ){
      pPager->xDestructor(pData, pPager->pageSize);
    }
  
    


    pPager->nRef--;
    assert( pPager->nRef>=0 );
    if( pPager->nRef==0 && !MEMDB ){
      pager_reset(pPager);
    }
  }
  return SQLITE_OK;
}








static int pager_open_journal(Pager *pPager){
  int rc;
  assert( !MEMDB );
  assert( pPager->state>=PAGER_RESERVED );
  assert( pPager->journalOpen==0 );
  assert( pPager->useJournal );
  assert( pPager->aInJournal==0 );
  sqlite3pager_pagecount(pPager);
  pPager->aInJournal = sqliteMalloc( pPager->dbSize/8 + 1 );
  if( pPager->aInJournal==0 ){
    rc = SQLITE_NOMEM;
    goto failed_to_open_journal;
  }
  rc = sqlite3OsOpenExclusive(pPager->zJournal, &pPager->jfd,
                                 pPager->tempFile);
  pPager->journalOff = 0;
  pPager->setMaster = 0;
  pPager->journalHdr = 0;
  if( rc!=SQLITE_OK ){
    goto failed_to_open_journal;
  }
  sqlite3OsSetFullSync(pPager->jfd, pPager->full_fsync);
  sqlite3OsSetFullSync(pPager->fd, pPager->full_fsync);
  sqlite3OsOpenDirectory(pPager->jfd, pPager->zDirectory);
  pPager->journalOpen = 1;
  pPager->journalStarted = 0;
  pPager->needSync = 0;
  pPager->alwaysRollback = 0;
  pPager->nRec = 0;
  if( pPager->errCode ){
    rc = pPager->errCode;
    goto failed_to_open_journal;
  }
  pPager->origDbSize = pPager->dbSize;

  rc = writeJournalHdr(pPager);

  if( pPager->stmtAutoopen && rc==SQLITE_OK ){
    rc = sqlite3pager_stmt_begin(pPager);
  }
  if( rc!=SQLITE_OK && rc!=SQLITE_NOMEM ){
    rc = pager_unwritelock(pPager);
    if( rc==SQLITE_OK ){
      rc = SQLITE_FULL;
    }
  }
  return rc;

failed_to_open_journal:
  sqliteFree(pPager->aInJournal);
  pPager->aInJournal = 0;
  if( rc==SQLITE_NOMEM ){
    




    sqlite3OsDelete(pPager->zJournal);
  }else{
    sqlite3OsUnlock(pPager->fd, NO_LOCK);
    pPager->state = PAGER_UNLOCK;
  }
  return rc;
}




























int sqlite3pager_begin(void *pData, int exFlag){
  PgHdr *pPg = DATA_TO_PGHDR(pData);
  Pager *pPager = pPg->pPager;
  int rc = SQLITE_OK;
  assert( pPg->nRef>0 );
  assert( pPager->state!=PAGER_UNLOCK );
  if( pPager->state==PAGER_SHARED ){
    assert( pPager->aInJournal==0 );
    if( MEMDB ){
      pPager->state = PAGER_EXCLUSIVE;
      pPager->origDbSize = pPager->dbSize;
    }else{
      rc = sqlite3OsLock(pPager->fd, RESERVED_LOCK);
      if( rc==SQLITE_OK ){
        pPager->state = PAGER_RESERVED;
        if( exFlag ){
          rc = pager_wait_on_lock(pPager, EXCLUSIVE_LOCK);
        }
      }
      if( rc!=SQLITE_OK ){
        return rc;
      }
      pPager->dirtyCache = 0;
      TRACE2("TRANSACTION %d\n", PAGERID(pPager));
      if( pPager->useJournal && !pPager->tempFile ){
        rc = pager_open_journal(pPager);
      }
    }
  }
  return rc;
}





static void makeDirty(PgHdr *pPg){
  if( pPg->dirty==0 ){
    Pager *pPager = pPg->pPager;
    pPg->dirty = 1;
    pPg->pDirty = pPager->pDirty;
    if( pPager->pDirty ){
      pPager->pDirty->pPrevDirty = pPg;
    }
    pPg->pPrevDirty = 0;
    pPager->pDirty = pPg;
  }
}





static void makeClean(PgHdr *pPg){
  if( pPg->dirty ){
    pPg->dirty = 0;
    if( pPg->pDirty ){
      pPg->pDirty->pPrevDirty = pPg->pPrevDirty;
    }
    if( pPg->pPrevDirty ){
      pPg->pPrevDirty->pDirty = pPg->pDirty;
    }else{
      pPg->pPager->pDirty = pPg->pDirty;
    }
  }
}



















int sqlite3pager_write(void *pData){
  PgHdr *pPg = DATA_TO_PGHDR(pData);
  Pager *pPager = pPg->pPager;
  int rc = SQLITE_OK;

  

  if( pPager->errCode ){ 
    return pPager->errCode;
  }
  if( pPager->readOnly ){
    return SQLITE_PERM;
  }

  assert( !pPager->setMaster );

  CHECK_PAGE(pPg);

  


  makeDirty(pPg);
  if( pPg->inJournal && (pPg->inStmt || pPager->stmtInUse==0) ){
    pPager->dirtyCache = 1;
  }else{

    






    assert( pPager->state!=PAGER_UNLOCK );
    rc = sqlite3pager_begin(pData, 0);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    assert( pPager->state>=PAGER_RESERVED );
    if( !pPager->journalOpen && pPager->useJournal ){
      rc = pager_open_journal(pPager);
      if( rc!=SQLITE_OK ) return rc;
    }
    assert( pPager->journalOpen || !pPager->useJournal );
    pPager->dirtyCache = 1;
  
    



    if( !pPg->inJournal && (pPager->useJournal || MEMDB) ){
      if( (int)pPg->pgno <= pPager->origDbSize ){
        int szPg;
        if( MEMDB ){
          PgHistory *pHist = PGHDR_TO_HIST(pPg, pPager);
          TRACE3("JOURNAL %d page %d\n", PAGERID(pPager), pPg->pgno);
          assert( pHist->pOrig==0 );
          pHist->pOrig = sqliteMallocRaw( pPager->pageSize );
          if( pHist->pOrig ){
            memcpy(pHist->pOrig, PGHDR_TO_DATA(pPg), pPager->pageSize);
          }
        }else{
          u32 cksum, saved;
          char *pData2, *pEnd;
          


          assert( pPg->pgno!=PAGER_MJ_PGNO(pPager) );
          pData2 = CODEC2(pPager, pData, pPg->pgno, 7);
          cksum = pager_cksum(pPager, (u8*)pData2);
          pEnd = pData2 + pPager->pageSize;
          pData2 -= 4;
          saved = *(u32*)pEnd;
          put32bits(pEnd, cksum);
          szPg = pPager->pageSize+8;
          put32bits(pData2, pPg->pgno);
          rc = sqlite3OsWrite(pPager->jfd, pData2, szPg);
          pPager->journalOff += szPg;
          TRACE4("JOURNAL %d page %d needSync=%d\n",
                  PAGERID(pPager), pPg->pgno, pPg->needSync);
          *(u32*)pEnd = saved;

	  


          if( rc!=SQLITE_OK ){
            return rc;
          }

          pPager->nRec++;
          assert( pPager->aInJournal!=0 );
          pPager->aInJournal[pPg->pgno/8] |= 1<<(pPg->pgno&7);
          pPg->needSync = !pPager->noSync;
          if( pPager->stmtInUse ){
            pPager->aInStmt[pPg->pgno/8] |= 1<<(pPg->pgno&7);
            page_add_to_stmt_list(pPg);
          }
        }
      }else{
        pPg->needSync = !pPager->journalStarted && !pPager->noSync;
        TRACE4("APPEND %d page %d needSync=%d\n",
                PAGERID(pPager), pPg->pgno, pPg->needSync);
      }
      if( pPg->needSync ){
        pPager->needSync = 1;
      }
      pPg->inJournal = 1;
    }
  
    




    if( pPager->stmtInUse && !pPg->inStmt && (int)pPg->pgno<=pPager->stmtSize ){
      assert( pPg->inJournal || (int)pPg->pgno>pPager->origDbSize );
      if( MEMDB ){
        PgHistory *pHist = PGHDR_TO_HIST(pPg, pPager);
        assert( pHist->pStmt==0 );
        pHist->pStmt = sqliteMallocRaw( pPager->pageSize );
        if( pHist->pStmt ){
          memcpy(pHist->pStmt, PGHDR_TO_DATA(pPg), pPager->pageSize);
        }
        TRACE3("STMT-JOURNAL %d page %d\n", PAGERID(pPager), pPg->pgno);
      }else{
        char *pData2 = CODEC2(pPager, pData, pPg->pgno, 7)-4;
        put32bits(pData2, pPg->pgno);
        rc = sqlite3OsWrite(pPager->stfd, pData2, pPager->pageSize+4);
        TRACE3("STMT-JOURNAL %d page %d\n", PAGERID(pPager), pPg->pgno);
        if( rc!=SQLITE_OK ){
          return rc;
        }
        pPager->stmtNRec++;
        assert( pPager->aInStmt!=0 );
        pPager->aInStmt[pPg->pgno/8] |= 1<<(pPg->pgno&7);
      }
      page_add_to_stmt_list(pPg);
    }
  }

  

  if( pPager->dbSize<(int)pPg->pgno ){
    pPager->dbSize = pPg->pgno;
    if( !MEMDB && pPager->dbSize==PENDING_BYTE/pPager->pageSize ){
      pPager->dbSize++;
    }
  }
  return rc;
}






#ifndef NDEBUG
int sqlite3pager_iswriteable(void *pData){
  PgHdr *pPg = DATA_TO_PGHDR(pData);
  return pPg->dirty;
}
#endif

#ifndef SQLITE_OMIT_VACUUM




int sqlite3pager_overwrite(Pager *pPager, Pgno pgno, void *pData){
  void *pPage;
  int rc;

  rc = sqlite3pager_get(pPager, pgno, &pPage);
  if( rc==SQLITE_OK ){
    rc = sqlite3pager_write(pPage);
    if( rc==SQLITE_OK ){
      memcpy(pPage, pData, pPager->pageSize);
    }
    sqlite3pager_unref(pPage);
  }
  return rc;
}
#endif

























void sqlite3pager_dont_write(Pager *pPager, Pgno pgno){
  PgHdr *pPg;

  if( MEMDB ) return;

  pPg = pager_lookup(pPager, pgno);
  assert( pPg!=0 );  
  pPg->alwaysRollback = 1;
  if( pPg->dirty && !pPager->stmtInUse ){
    if( pPager->dbSize==(int)pPg->pgno && pPager->origDbSize<pPager->dbSize ){
      







    }else{
      TRACE3("DONT_WRITE page %d of %d\n", pgno, PAGERID(pPager));
      makeClean(pPg);
#ifdef SQLITE_CHECK_PAGES
      pPg->pageHash = pager_pagehash(pPg);
#endif
    }
  }
}







void sqlite3pager_dont_rollback(void *pData){
  PgHdr *pPg = DATA_TO_PGHDR(pData);
  Pager *pPager = pPg->pPager;

  if( pPager->state!=PAGER_EXCLUSIVE || pPager->journalOpen==0 ) return;
  if( pPg->alwaysRollback || pPager->alwaysRollback || MEMDB ) return;
  if( !pPg->inJournal && (int)pPg->pgno <= pPager->origDbSize ){
    assert( pPager->aInJournal!=0 );
    pPager->aInJournal[pPg->pgno/8] |= 1<<(pPg->pgno&7);
    pPg->inJournal = 1;
    if( pPager->stmtInUse ){
      pPager->aInStmt[pPg->pgno/8] |= 1<<(pPg->pgno&7);
      page_add_to_stmt_list(pPg);
    }
    TRACE3("DONT_ROLLBACK page %d of %d\n", pPg->pgno, PAGERID(pPager));
  }
  if( pPager->stmtInUse && !pPg->inStmt && (int)pPg->pgno<=pPager->stmtSize ){
    assert( pPg->inJournal || (int)pPg->pgno>pPager->origDbSize );
    assert( pPager->aInStmt!=0 );
    pPager->aInStmt[pPg->pgno/8] |= 1<<(pPg->pgno&7);
    page_add_to_stmt_list(pPg);
  }
}









int sqlite3pager_commit(Pager *pPager){
  int rc;
  PgHdr *pPg;

  if( pPager->errCode ){
    return pPager->errCode;
  }
  if( pPager->state<PAGER_RESERVED ){
    return SQLITE_ERROR;
  }
  TRACE2("COMMIT %d\n", PAGERID(pPager));
  if( MEMDB ){
    pPg = pager_get_all_dirty_pages(pPager);
    while( pPg ){
      clearHistory(PGHDR_TO_HIST(pPg, pPager));
      pPg->dirty = 0;
      pPg->inJournal = 0;
      pPg->inStmt = 0;
      pPg->needSync = 0;
      pPg->pPrevStmt = pPg->pNextStmt = 0;
      pPg = pPg->pDirty;
    }
    pPager->pDirty = 0;
#ifndef NDEBUG
    for(pPg=pPager->pAll; pPg; pPg=pPg->pNextAll){
      PgHistory *pHist = PGHDR_TO_HIST(pPg, pPager);
      assert( !pPg->alwaysRollback );
      assert( !pHist->pOrig );
      assert( !pHist->pStmt );
    }
#endif
    pPager->pStmt = 0;
    pPager->state = PAGER_SHARED;
    return SQLITE_OK;
  }
  if( pPager->dirtyCache==0 ){
    

    assert( pPager->needSync==0 );
    rc = pager_unwritelock(pPager);
    pPager->dbSize = -1;
    return rc;
  }
  assert( pPager->journalOpen );
  rc = sqlite3pager_sync(pPager, 0, 0);
  if( rc==SQLITE_OK ){
    rc = pager_unwritelock(pPager);
    pPager->dbSize = -1;
  }
  return rc;
}













int sqlite3pager_rollback(Pager *pPager){
  int rc;
  TRACE2("ROLLBACK %d\n", PAGERID(pPager));
  if( MEMDB ){
    PgHdr *p;
    for(p=pPager->pAll; p; p=p->pNextAll){
      PgHistory *pHist;
      assert( !p->alwaysRollback );
      if( !p->dirty ){
        assert( !((PgHistory *)PGHDR_TO_HIST(p, pPager))->pOrig );
        assert( !((PgHistory *)PGHDR_TO_HIST(p, pPager))->pStmt );
        continue;
      }

      pHist = PGHDR_TO_HIST(p, pPager);
      if( pHist->pOrig ){
        memcpy(PGHDR_TO_DATA(p), pHist->pOrig, pPager->pageSize);
        TRACE3("ROLLBACK-PAGE %d of %d\n", p->pgno, PAGERID(pPager));
      }else{
        TRACE3("PAGE %d is clean on %d\n", p->pgno, PAGERID(pPager));
      }
      clearHistory(pHist);
      p->dirty = 0;
      p->inJournal = 0;
      p->inStmt = 0;
      p->pPrevStmt = p->pNextStmt = 0;
      if( pPager->xReiniter ){
        pPager->xReiniter(PGHDR_TO_DATA(p), pPager->pageSize);
      }
    }
    pPager->pDirty = 0;
    pPager->pStmt = 0;
    pPager->dbSize = pPager->origDbSize;
    memoryTruncate(pPager);
    pPager->stmtInUse = 0;
    pPager->state = PAGER_SHARED;
    return SQLITE_OK;
  }

  if( !pPager->dirtyCache || !pPager->journalOpen ){
    rc = pager_unwritelock(pPager);
    pPager->dbSize = -1;
    return rc;
  }

  if( pPager->errCode && pPager->errCode!=SQLITE_FULL ){
    if( pPager->state>=PAGER_EXCLUSIVE ){
      pager_playback(pPager);
    }
    return pPager->errCode;
  }
  if( pPager->state==PAGER_RESERVED ){
    int rc2;
    rc = pager_reload_cache(pPager);
    rc2 = pager_unwritelock(pPager);
    if( rc==SQLITE_OK ){
      rc = rc2;
    }
  }else{
    rc = pager_playback(pPager);
  }
  pPager->dbSize = -1;

  



  return pager_error(pPager, rc);
}





int sqlite3pager_isreadonly(Pager *pPager){
  return pPager->readOnly;
}




int *sqlite3pager_stats(Pager *pPager){
  static int a[11];
  a[0] = pPager->nRef;
  a[1] = pPager->nPage;
  a[2] = pPager->mxPage;
  a[3] = pPager->dbSize;
  a[4] = pPager->state;
  a[5] = pPager->errCode;
#ifdef SQLITE_TEST
  a[6] = pPager->nHit;
  a[7] = pPager->nMiss;
  a[8] = pPager->nOvfl;
  a[9] = pPager->nRead;
  a[10] = pPager->nWrite;
#endif
  return a;
}








int sqlite3pager_stmt_begin(Pager *pPager){
  int rc;
  char zTemp[SQLITE_TEMPNAME_SIZE];
  assert( !pPager->stmtInUse );
  assert( pPager->dbSize>=0 );
  TRACE2("STMT-BEGIN %d\n", PAGERID(pPager));
  if( MEMDB ){
    pPager->stmtInUse = 1;
    pPager->stmtSize = pPager->dbSize;
    return SQLITE_OK;
  }
  if( !pPager->journalOpen ){
    pPager->stmtAutoopen = 1;
    return SQLITE_OK;
  }
  assert( pPager->journalOpen );
  pPager->aInStmt = sqliteMalloc( pPager->dbSize/8 + 1 );
  if( pPager->aInStmt==0 ){
    
    return SQLITE_NOMEM;
  }
#ifndef NDEBUG
  rc = sqlite3OsFileSize(pPager->jfd, &pPager->stmtJSize);
  if( rc ) goto stmt_begin_failed;
  assert( pPager->stmtJSize == pPager->journalOff );
#endif
  pPager->stmtJSize = pPager->journalOff;
  pPager->stmtSize = pPager->dbSize;
  pPager->stmtHdrOff = 0;
  pPager->stmtCksum = pPager->cksumInit;
  if( !pPager->stmtOpen ){
    rc = sqlite3pager_opentemp(zTemp, &pPager->stfd);
    if( rc ) goto stmt_begin_failed;
    pPager->stmtOpen = 1;
    pPager->stmtNRec = 0;
  }
  pPager->stmtInUse = 1;
  return SQLITE_OK;
 
stmt_begin_failed:
  if( pPager->aInStmt ){
    sqliteFree(pPager->aInStmt);
    pPager->aInStmt = 0;
  }
  return rc;
}




int sqlite3pager_stmt_commit(Pager *pPager){
  if( pPager->stmtInUse ){
    PgHdr *pPg, *pNext;
    TRACE2("STMT-COMMIT %d\n", PAGERID(pPager));
    if( !MEMDB ){
      sqlite3OsSeek(pPager->stfd, 0);
      
      sqliteFree( pPager->aInStmt );
      pPager->aInStmt = 0;
    }
    for(pPg=pPager->pStmt; pPg; pPg=pNext){
      pNext = pPg->pNextStmt;
      assert( pPg->inStmt );
      pPg->inStmt = 0;
      pPg->pPrevStmt = pPg->pNextStmt = 0;
      if( MEMDB ){
        PgHistory *pHist = PGHDR_TO_HIST(pPg, pPager);
        sqliteFree(pHist->pStmt);
        pHist->pStmt = 0;
      }
    }
    pPager->stmtNRec = 0;
    pPager->stmtInUse = 0;
    pPager->pStmt = 0;
  }
  pPager->stmtAutoopen = 0;
  return SQLITE_OK;
}




int sqlite3pager_stmt_rollback(Pager *pPager){
  int rc;
  if( pPager->stmtInUse ){
    TRACE2("STMT-ROLLBACK %d\n", PAGERID(pPager));
    if( MEMDB ){
      PgHdr *pPg;
      for(pPg=pPager->pStmt; pPg; pPg=pPg->pNextStmt){
        PgHistory *pHist = PGHDR_TO_HIST(pPg, pPager);
        if( pHist->pStmt ){
          memcpy(PGHDR_TO_DATA(pPg), pHist->pStmt, pPager->pageSize);
          sqliteFree(pHist->pStmt);
          pHist->pStmt = 0;
        }
      }
      pPager->dbSize = pPager->stmtSize;
      memoryTruncate(pPager);
      rc = SQLITE_OK;
    }else{
      rc = pager_stmt_playback(pPager);
    }
    sqlite3pager_stmt_commit(pPager);
  }else{
    rc = SQLITE_OK;
  }
  pPager->stmtAutoopen = 0;
  return rc;
}




const char *sqlite3pager_filename(Pager *pPager){
  return pPager->zFilename;
}




const char *sqlite3pager_dirname(Pager *pPager){
  return pPager->zDirectory;
}




const char *sqlite3pager_journalname(Pager *pPager){
  return pPager->zJournal;
}





int sqlite3pager_nosync(Pager *pPager){
  return pPager->noSync;
}




void sqlite3pager_set_codec(
  Pager *pPager,
  void *(*xCodec)(void*,void*,Pgno,int),
  void *pCodecArg
){
  pPager->xCodec = xCodec;
  pPager->pCodecArg = pCodecArg;
}





static int pager_incr_changecounter(Pager *pPager){
  void *pPage;
  PgHdr *pPgHdr;
  u32 change_counter;
  int rc;

  
  rc = sqlite3pager_get(pPager, 1, &pPage);
  if( rc!=SQLITE_OK ) return rc;
  rc = sqlite3pager_write(pPage);
  if( rc!=SQLITE_OK ) return rc;

  
  pPgHdr = DATA_TO_PGHDR(pPage);
  change_counter = retrieve32bits(pPgHdr, 24);

  
  change_counter++;
  put32bits(((char*)PGHDR_TO_DATA(pPgHdr))+24, change_counter);

  
  sqlite3pager_unref(pPage);
  return SQLITE_OK;
}


















int sqlite3pager_sync(Pager *pPager, const char *zMaster, Pgno nTrunc){
  int rc = SQLITE_OK;

  TRACE4("DATABASE SYNC: File=%s zMaster=%s nTrunc=%d\n", 
      pPager->zFilename, zMaster, nTrunc);

  


  if( pPager->state!=PAGER_SYNCED && !MEMDB && pPager->dirtyCache ){
    PgHdr *pPg;
    assert( pPager->journalOpen );

    





    if( !pPager->setMaster ){
      rc = pager_incr_changecounter(pPager);
      if( rc!=SQLITE_OK ) goto sync_exit;
#ifndef SQLITE_OMIT_AUTOVACUUM
      if( nTrunc!=0 ){
        



        Pgno i;
        void *pPage;
        int iSkip = PAGER_MJ_PGNO(pPager);
        for( i=nTrunc+1; i<=pPager->origDbSize; i++ ){
          if( !(pPager->aInJournal[i/8] & (1<<(i&7))) && i!=iSkip ){
            rc = sqlite3pager_get(pPager, i, &pPage);
            if( rc!=SQLITE_OK ) goto sync_exit;
            rc = sqlite3pager_write(pPage);
            sqlite3pager_unref(pPage);
            if( rc!=SQLITE_OK ) goto sync_exit;
          }
        } 
      }
#endif
      rc = writeMasterJournal(pPager, zMaster);
      if( rc!=SQLITE_OK ) goto sync_exit;
      rc = syncJournal(pPager);
      if( rc!=SQLITE_OK ) goto sync_exit;
    }

#ifndef SQLITE_OMIT_AUTOVACUUM
    if( nTrunc!=0 ){
      rc = sqlite3pager_truncate(pPager, nTrunc);
      if( rc!=SQLITE_OK ) goto sync_exit;
    }
#endif

    
    pPg = pager_get_all_dirty_pages(pPager);
    rc = pager_write_pagelist(pPg);
    if( rc!=SQLITE_OK ) goto sync_exit;

    
    if( !pPager->noSync ){
      rc = sqlite3OsSync(pPager->fd, 0);
    }

    pPager->state = PAGER_SYNCED;
  }else if( MEMDB && nTrunc!=0 ){
    rc = sqlite3pager_truncate(pPager, nTrunc);
  }

sync_exit:
  return rc;
}

#ifndef SQLITE_OMIT_AUTOVACUUM

















int sqlite3pager_movepage(Pager *pPager, void *pData, Pgno pgno){
  PgHdr *pPg = DATA_TO_PGHDR(pData);
  PgHdr *pPgOld; 
  int h;
  Pgno needSyncPgno = 0;

  assert( pPg->nRef>0 );

  TRACE5("MOVE %d page %d (needSync=%d) moves to %d\n", 
      PAGERID(pPager), pPg->pgno, pPg->needSync, pgno);

  if( pPg->needSync ){
    needSyncPgno = pPg->pgno;
    assert( pPg->inJournal );
    assert( pPg->dirty );
    assert( pPager->needSync );
  }

  
  unlinkHashChain(pPager, pPg);

  




  pPgOld = pager_lookup(pPager, pgno);
  if( pPgOld ){
    assert( pPgOld->nRef==0 );
    unlinkHashChain(pPager, pPgOld);
    makeClean(pPgOld);
    if( pPgOld->needSync ){
      assert( pPgOld->inJournal );
      pPg->inJournal = 1;
      pPg->needSync = 1;
      assert( pPager->needSync );
    }
  }

  
  pPg->pgno = pgno;
  h = pgno & (pPager->nHash-1);
  if( pPager->aHash[h] ){
    assert( pPager->aHash[h]->pPrevHash==0 );
    pPager->aHash[h]->pPrevHash = pPg;
  }
  pPg->pNextHash = pPager->aHash[h];
  pPager->aHash[h] = pPg;
  pPg->pPrevHash = 0;

  makeDirty(pPg);
  pPager->dirtyCache = 1;

  if( needSyncPgno ){
    








    int rc;
    void *pNeedSync;
    assert( pPager->needSync );
    rc = sqlite3pager_get(pPager, needSyncPgno, &pNeedSync);
    if( rc!=SQLITE_OK ) return rc;
    pPager->needSync = 1;
    DATA_TO_PGHDR(pNeedSync)->needSync = 1;
    DATA_TO_PGHDR(pNeedSync)->inJournal = 1;
    makeDirty(DATA_TO_PGHDR(pNeedSync));
    sqlite3pager_unref(pNeedSync);
  }

  return SQLITE_OK;
}
#endif












int sqlite3pager_loadall(Pager* pPager)
{
  int i;
  int rc;
  int loadSize;
  int loadPages;
  unsigned char* fileData;

  if (pPager->dbSize < 0 || pPager->pageSize < 0) {
    
    return SQLITE_MISUSE;
  }

  
  if (pPager->mxPage < pPager->dbSize)
    loadPages = pPager->mxPage;
  else
    loadPages = pPager->dbSize;
  loadSize = loadPages * pPager->pageSize;

  rc = sqlite3OsSeek(pPager->fd, 0);
  if (rc != SQLITE_OK)
    return rc;

  
  fileData = sqliteMallocRaw(loadSize);
  if (! fileData)
    return SQLITE_NOMEM;
  rc = sqlite3OsRead(pPager->fd, fileData, loadSize);
  if (rc != SQLITE_OK) {
    sqliteFree(fileData);
    return rc;
  }

  





  for (i = 1; i <= loadPages && pPager->nPage < pPager->mxPage; i ++) {
    void *pPage;
    rc = sqlite3pager_get2(pPager, 1, &pPage,
                           &fileData[(i-1)*(i64)pPager->pageSize]);
    if (rc != SQLITE_OK)
      break;
    sqlite3pager_unref(pPage);
  }
  sqliteFree(fileData);
  return SQLITE_OK;
}


#if defined(SQLITE_DEBUG) || defined(SQLITE_TEST)





int sqlite3pager_lockstate(Pager *pPager){
  return sqlite3OsLockState(pPager->fd);
}
#endif

#ifdef SQLITE_DEBUG



void sqlite3pager_refdump(Pager *pPager){
  PgHdr *pPg;
  for(pPg=pPager->pAll; pPg; pPg=pPg->pNextAll){
    if( pPg->nRef<=0 ) continue;
    sqlite3DebugPrintf("PAGE %3d addr=%p nRef=%d\n", 
       pPg->pgno, PGHDR_TO_DATA(pPg), pPg->nRef);
  }
}
#endif

#endif 
