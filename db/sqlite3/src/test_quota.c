




























#include "test_quota.h"
#include <string.h>
#include <assert.h>




#if defined(SQLITE_THREADSAFE) && SQLITE_THREADSAFE==0
#define sqlite3_mutex_alloc(X)    ((sqlite3_mutex*)8)
#define sqlite3_mutex_free(X)
#define sqlite3_mutex_enter(X)
#define sqlite3_mutex_try(X)      SQLITE_OK
#define sqlite3_mutex_leave(X)
#define sqlite3_mutex_held(X)     ((void)(X),1)
#define sqlite3_mutex_notheld(X)  ((void)(X),1)
#endif 





typedef struct quotaGroup quotaGroup;
typedef struct quotaConn quotaConn;
typedef struct quotaFile quotaFile;















struct quotaGroup {
  const char *zPattern;          
  sqlite3_int64 iLimit;          
  sqlite3_int64 iSize;           
  void (*xCallback)(             
     const char *zFilename,         
     sqlite3_int64 *piLimit,        
     sqlite3_int64 iSize,           
     void *pArg                     
  );
  void *pArg;                    
  void (*xDestroy)(void*);       
  quotaGroup *pNext, **ppPrev;   
  quotaFile *pFiles;             
};









struct quotaFile {
  char *zFilename;                
  quotaGroup *pGroup;             
  sqlite3_int64 iSize;            
  int nRef;                       
  int deleteOnClose;              
  quotaFile *pNext, **ppPrev;     
};







struct quotaConn {
  sqlite3_file base;              
  quotaFile *pFile;               
  
};






struct quota_FILE {
  FILE *f;                
  sqlite3_int64 iOfst;    
  quotaFile *pFile;       
};







static struct {
  




  sqlite3_vfs *pOrigVfs;

  


  sqlite3_vfs sThisVfs;

  









  sqlite3_io_methods sIoMethodsV1;
  sqlite3_io_methods sIoMethodsV2;

  

  int isInitialized;

  


  sqlite3_mutex *pMutex;

  

  quotaGroup *pGroup;

} gQuota;






static void quotaEnter(void){ sqlite3_mutex_enter(gQuota.pMutex); }
static void quotaLeave(void){ sqlite3_mutex_leave(gQuota.pMutex); }



static int quotaGroupOpenFileCount(quotaGroup *pGroup){
  int N = 0;
  quotaFile *pFile = pGroup->pFiles;
  while( pFile ){
    if( pFile->nRef ) N++;
    pFile = pFile->pNext;
  }
  return N;
}



static void quotaRemoveFile(quotaFile *pFile){
  quotaGroup *pGroup = pFile->pGroup;
  pGroup->iSize -= pFile->iSize;
  *pFile->ppPrev = pFile->pNext;
  if( pFile->pNext ) pFile->pNext->ppPrev = pFile->ppPrev;
  sqlite3_free(pFile);
}




static void quotaRemoveAllFiles(quotaGroup *pGroup){
  while( pGroup->pFiles ){
    assert( pGroup->pFiles->nRef==0 );
    quotaRemoveFile(pGroup->pFiles);
  }
}





static void quotaGroupDeref(quotaGroup *pGroup){
  if( pGroup->iLimit==0 && quotaGroupOpenFileCount(pGroup)==0 ){
    quotaRemoveAllFiles(pGroup);
    *pGroup->ppPrev = pGroup->pNext;
    if( pGroup->pNext ) pGroup->pNext->ppPrev = pGroup->ppPrev;
    if( pGroup->xDestroy ) pGroup->xDestroy(pGroup->pArg);
    sqlite3_free(pGroup);
  }
}


















static int quotaStrglob(const char *zGlob, const char *z){
  int c, c2, cx;
  int invert;
  int seen;

  while( (c = (*(zGlob++)))!=0 ){
    if( c=='*' ){
      while( (c=(*(zGlob++))) == '*' || c=='?' ){
        if( c=='?' && (*(z++))==0 ) return 0;
      }
      if( c==0 ){
        return 1;
      }else if( c=='[' ){
        while( *z && quotaStrglob(zGlob-1,z)==0 ){
          z++;
        }
        return (*z)!=0;
      }
      cx = (c=='/') ? '\\' : c;
      while( (c2 = (*(z++)))!=0 ){
        while( c2!=c && c2!=cx ){
          c2 = *(z++);
          if( c2==0 ) return 0;
        }
        if( quotaStrglob(zGlob,z) ) return 1;
      }
      return 0;
    }else if( c=='?' ){
      if( (*(z++))==0 ) return 0;
    }else if( c=='[' ){
      int prior_c = 0;
      seen = 0;
      invert = 0;
      c = *(z++);
      if( c==0 ) return 0;
      c2 = *(zGlob++);
      if( c2=='^' ){
        invert = 1;
        c2 = *(zGlob++);
      }
      if( c2==']' ){
        if( c==']' ) seen = 1;
        c2 = *(zGlob++);
      }
      while( c2 && c2!=']' ){
        if( c2=='-' && zGlob[0]!=']' && zGlob[0]!=0 && prior_c>0 ){
          c2 = *(zGlob++);
          if( c>=prior_c && c<=c2 ) seen = 1;
          prior_c = 0;
        }else{
          if( c==c2 ){
            seen = 1;
          }
          prior_c = c2;
        }
        c2 = *(zGlob++);
      }
      if( c2==0 || (seen ^ invert)==0 ) return 0;
    }else if( c=='/' ){
      if( z[0]!='/' && z[0]!='\\' ) return 0;
      z++;
    }else{
      if( c!=(*(z++)) ) return 0;
    }
  }
  return *z==0;
}






static quotaGroup *quotaGroupFind(const char *zFilename){
  quotaGroup *p;
  for(p=gQuota.pGroup; p && quotaStrglob(p->zPattern, zFilename)==0;
      p=p->pNext){}
  return p;
}




static sqlite3_file *quotaSubOpen(sqlite3_file *pConn){
  quotaConn *p = (quotaConn*)pConn;
  return (sqlite3_file*)&p[1];
}




static quotaFile *quotaFindFile(
  quotaGroup *pGroup,     
  const char *zName,      
  int createFlag          
){
  quotaFile *pFile = pGroup->pFiles;
  while( pFile && strcmp(pFile->zFilename, zName)!=0 ){
    pFile = pFile->pNext;
  }
  if( pFile==0 && createFlag ){
    int nName = strlen(zName);
    pFile = (quotaFile *)sqlite3_malloc( sizeof(*pFile) + nName + 1 );
    if( pFile ){
      memset(pFile, 0, sizeof(*pFile));
      pFile->zFilename = (char*)&pFile[1];
      memcpy(pFile->zFilename, zName, nName+1);
      pFile->pNext = pGroup->pFiles;
      if( pGroup->pFiles ) pGroup->pFiles->ppPrev = &pFile->pNext;
      pFile->ppPrev = &pGroup->pFiles;
      pGroup->pFiles = pFile;
      pFile->pGroup = pGroup;
    }
  }
  return pFile;
}








#if defined(SQLITE_OS_OTHER)
# if SQLITE_OS_OTHER==1
#   undef SQLITE_OS_UNIX
#   define SQLITE_OS_UNIX 0
#   undef SQLITE_OS_WIN
#   define SQLITE_OS_WIN 0
#   undef SQLITE_OS_OS2
#   define SQLITE_OS_OS2 0
# else
#   undef SQLITE_OS_OTHER
# endif
#endif
#if !defined(SQLITE_OS_UNIX) && !defined(SQLITE_OS_OTHER)
# define SQLITE_OS_OTHER 0
# ifndef SQLITE_OS_WIN
#   if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) \
                       || defined(__MINGW32__) || defined(__BORLANDC__)
#     define SQLITE_OS_WIN 1
#     define SQLITE_OS_UNIX 0
#     define SQLITE_OS_OS2 0
#   elif defined(__EMX__) || defined(_OS2) || defined(OS2) \
                          || defined(_OS2_) || defined(__OS2__)
#     define SQLITE_OS_WIN 0
#     define SQLITE_OS_UNIX 0
#     define SQLITE_OS_OS2 1
#   else
#     define SQLITE_OS_WIN 0
#     define SQLITE_OS_UNIX 1
#     define SQLITE_OS_OS2 0
#  endif
# else
#  define SQLITE_OS_UNIX 0
#  define SQLITE_OS_OS2 0
# endif
#else
# ifndef SQLITE_OS_WIN
#  define SQLITE_OS_WIN 0
# endif
#endif

#if SQLITE_OS_UNIX
# include <unistd.h>
#endif
#if SQLITE_OS_WIN
# include <windows.h>
# include <io.h>
#endif






static char *quota_utf8_to_mbcs(const char *zUtf8){
#if SQLITE_OS_WIN
  int n;             
  int nWide;         
  int nMbcs;         
  LPWSTR zTmpWide;   
  char *zMbcs;       
  int codepage;      

  n = strlen(zUtf8);
  nWide = MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, NULL, 0);
  if( nWide==0 ) return 0;
  zTmpWide = (LPWSTR)sqlite3_malloc( (nWide+1)*sizeof(zTmpWide[0]) );
  if( zTmpWide==0 ) return 0;
  MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, zTmpWide, nWide);
  codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  nMbcs = WideCharToMultiByte(codepage, 0, zTmpWide, nWide, 0, 0, 0, 0);
  zMbcs = nMbcs ? (char*)sqlite3_malloc( nMbcs+1 ) : 0;
  if( zMbcs ){
    WideCharToMultiByte(codepage, 0, zTmpWide, nWide, zMbcs, nMbcs, 0, 0);
  }
  sqlite3_free(zTmpWide);
  return zMbcs;
#else
  return (char*)zUtf8;  
#endif  
}




static void quota_mbcs_free(char *zOld){
#if SQLITE_OS_WIN
  sqlite3_free(zOld);
#else
  
#endif  
}









static int quotaOpen(
  sqlite3_vfs *pVfs,          
  const char *zName,          
  sqlite3_file *pConn,        
  int flags,                  
  int *pOutFlags              
){
  int rc;                                             
  quotaConn *pQuotaOpen;                     
  quotaFile *pFile;                          
  quotaGroup *pGroup;                        
  sqlite3_file *pSubOpen;                    
  sqlite3_vfs *pOrigVfs = gQuota.pOrigVfs;   

  


  if( (flags & (SQLITE_OPEN_MAIN_DB|SQLITE_OPEN_WAL))==0 ){
    return pOrigVfs->xOpen(pOrigVfs, zName, pConn, flags, pOutFlags);
  }

  


  quotaEnter();
  pGroup = quotaGroupFind(zName);
  if( pGroup==0 ){
    rc = pOrigVfs->xOpen(pOrigVfs, zName, pConn, flags, pOutFlags);
  }else{
    

    pQuotaOpen = (quotaConn*)pConn;
    pSubOpen = quotaSubOpen(pConn);
    rc = pOrigVfs->xOpen(pOrigVfs, zName, pSubOpen, flags, pOutFlags);
    if( rc==SQLITE_OK ){
      pFile = quotaFindFile(pGroup, zName, 1);
      if( pFile==0 ){
        quotaLeave();
        pSubOpen->pMethods->xClose(pSubOpen);
        return SQLITE_NOMEM;
      }
      pFile->deleteOnClose = (flags & SQLITE_OPEN_DELETEONCLOSE)!=0;
      pFile->nRef++;
      pQuotaOpen->pFile = pFile;
      if( pSubOpen->pMethods->iVersion==1 ){
        pQuotaOpen->base.pMethods = &gQuota.sIoMethodsV1;
      }else{
        pQuotaOpen->base.pMethods = &gQuota.sIoMethodsV2;
      }
    }
  }
  quotaLeave();
  return rc;
}








static int quotaDelete(
  sqlite3_vfs *pVfs,          
  const char *zName,          
  int syncDir                 
){
  int rc;                                             
  quotaFile *pFile;                          
  quotaGroup *pGroup;                        
  sqlite3_vfs *pOrigVfs = gQuota.pOrigVfs;   

  
  rc = pOrigVfs->xDelete(pOrigVfs, zName, syncDir);

  


  if( rc==SQLITE_OK ){
    quotaEnter();
    pGroup = quotaGroupFind(zName);
    if( pGroup ){
      pFile = quotaFindFile(pGroup, zName, 0);
      if( pFile ){
        if( pFile->nRef ){
          pFile->deleteOnClose = 1;
        }else{
          quotaRemoveFile(pFile);
          quotaGroupDeref(pGroup);
        }
      }
    }
    quotaLeave();
  }
  return rc;
}








static int quotaClose(sqlite3_file *pConn){
  quotaConn *p = (quotaConn*)pConn;
  quotaFile *pFile = p->pFile;
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  int rc;
  rc = pSubOpen->pMethods->xClose(pSubOpen);
  quotaEnter();
  pFile->nRef--;
  if( pFile->nRef==0 ){
    quotaGroup *pGroup = pFile->pGroup;
    if( pFile->deleteOnClose ){
      gQuota.pOrigVfs->xDelete(gQuota.pOrigVfs, pFile->zFilename, 0);
      quotaRemoveFile(pFile);
    }
    quotaGroupDeref(pGroup);
  }
  quotaLeave();
  return rc;
}




static int quotaRead(
  sqlite3_file *pConn,
  void *pBuf,
  int iAmt,
  sqlite3_int64 iOfst
){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xRead(pSubOpen, pBuf, iAmt, iOfst);
}





static int quotaWrite(
  sqlite3_file *pConn,
  const void *pBuf,
  int iAmt,
  sqlite3_int64 iOfst
){
  quotaConn *p = (quotaConn*)pConn;
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  sqlite3_int64 iEnd = iOfst+iAmt;
  quotaGroup *pGroup;
  quotaFile *pFile = p->pFile;
  sqlite3_int64 szNew;

  if( pFile->iSize<iEnd ){
    pGroup = pFile->pGroup;
    quotaEnter();
    szNew = pGroup->iSize - pFile->iSize + iEnd;
    if( szNew>pGroup->iLimit && pGroup->iLimit>0 ){
      if( pGroup->xCallback ){
        pGroup->xCallback(pFile->zFilename, &pGroup->iLimit, szNew, 
                          pGroup->pArg);
      }
      if( szNew>pGroup->iLimit && pGroup->iLimit>0 ){
        quotaLeave();
        return SQLITE_FULL;
      }
    }
    pGroup->iSize = szNew;
    pFile->iSize = iEnd;
    quotaLeave();
  }
  return pSubOpen->pMethods->xWrite(pSubOpen, pBuf, iAmt, iOfst);
}




static int quotaTruncate(sqlite3_file *pConn, sqlite3_int64 size){
  quotaConn *p = (quotaConn*)pConn;
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  int rc = pSubOpen->pMethods->xTruncate(pSubOpen, size);
  quotaFile *pFile = p->pFile;
  quotaGroup *pGroup;
  if( rc==SQLITE_OK ){
    quotaEnter();
    pGroup = pFile->pGroup;
    pGroup->iSize -= pFile->iSize;
    pFile->iSize = size;
    pGroup->iSize += size;
    quotaLeave();
  }
  return rc;
}



static int quotaSync(sqlite3_file *pConn, int flags){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xSync(pSubOpen, flags);
}




static int quotaFileSize(sqlite3_file *pConn, sqlite3_int64 *pSize){
  quotaConn *p = (quotaConn*)pConn;
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  quotaFile *pFile = p->pFile;
  quotaGroup *pGroup;
  sqlite3_int64 sz;
  int rc;

  rc = pSubOpen->pMethods->xFileSize(pSubOpen, &sz);
  if( rc==SQLITE_OK ){
    quotaEnter();
    pGroup = pFile->pGroup;
    pGroup->iSize -= pFile->iSize;
    pFile->iSize = sz;
    pGroup->iSize += sz;
    quotaLeave();
    *pSize = sz;
  }
  return rc;
}



static int quotaLock(sqlite3_file *pConn, int lock){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xLock(pSubOpen, lock);
}



static int quotaUnlock(sqlite3_file *pConn, int lock){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xUnlock(pSubOpen, lock);
}



static int quotaCheckReservedLock(sqlite3_file *pConn, int *pResOut){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xCheckReservedLock(pSubOpen, pResOut);
}



static int quotaFileControl(sqlite3_file *pConn, int op, void *pArg){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  int rc = pSubOpen->pMethods->xFileControl(pSubOpen, op, pArg);
#if defined(SQLITE_FCNTL_VFSNAME)
  if( op==SQLITE_FCNTL_VFSNAME && rc==SQLITE_OK ){
    *(char**)pArg = sqlite3_mprintf("quota/%z", *(char**)pArg);
  }
#endif
  return rc;
}



static int quotaSectorSize(sqlite3_file *pConn){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xSectorSize(pSubOpen);
}



static int quotaDeviceCharacteristics(sqlite3_file *pConn){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xDeviceCharacteristics(pSubOpen);
}



static int quotaShmMap(
  sqlite3_file *pConn,            
  int iRegion,                    
  int szRegion,                   
  int bExtend,                    
  void volatile **pp              
){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xShmMap(pSubOpen, iRegion, szRegion, bExtend, pp);
}



static int quotaShmLock(
  sqlite3_file *pConn,       
  int ofst,                  
  int n,                     
  int flags                  
){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xShmLock(pSubOpen, ofst, n, flags);
}



static void quotaShmBarrier(sqlite3_file *pConn){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  pSubOpen->pMethods->xShmBarrier(pSubOpen);
}



static int quotaShmUnmap(sqlite3_file *pConn, int deleteFlag){
  sqlite3_file *pSubOpen = quotaSubOpen(pConn);
  return pSubOpen->pMethods->xShmUnmap(pSubOpen, deleteFlag);
}













int sqlite3_quota_initialize(const char *zOrigVfsName, int makeDefault){
  sqlite3_vfs *pOrigVfs;
  if( gQuota.isInitialized ) return SQLITE_MISUSE;
  pOrigVfs = sqlite3_vfs_find(zOrigVfsName);
  if( pOrigVfs==0 ) return SQLITE_ERROR;
  assert( pOrigVfs!=&gQuota.sThisVfs );
  gQuota.pMutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
  if( !gQuota.pMutex ){
    return SQLITE_NOMEM;
  }
  gQuota.isInitialized = 1;
  gQuota.pOrigVfs = pOrigVfs;
  gQuota.sThisVfs = *pOrigVfs;
  gQuota.sThisVfs.xOpen = quotaOpen;
  gQuota.sThisVfs.xDelete = quotaDelete;
  gQuota.sThisVfs.szOsFile += sizeof(quotaConn);
  gQuota.sThisVfs.zName = "quota";
  gQuota.sIoMethodsV1.iVersion = 1;
  gQuota.sIoMethodsV1.xClose = quotaClose;
  gQuota.sIoMethodsV1.xRead = quotaRead;
  gQuota.sIoMethodsV1.xWrite = quotaWrite;
  gQuota.sIoMethodsV1.xTruncate = quotaTruncate;
  gQuota.sIoMethodsV1.xSync = quotaSync;
  gQuota.sIoMethodsV1.xFileSize = quotaFileSize;
  gQuota.sIoMethodsV1.xLock = quotaLock;
  gQuota.sIoMethodsV1.xUnlock = quotaUnlock;
  gQuota.sIoMethodsV1.xCheckReservedLock = quotaCheckReservedLock;
  gQuota.sIoMethodsV1.xFileControl = quotaFileControl;
  gQuota.sIoMethodsV1.xSectorSize = quotaSectorSize;
  gQuota.sIoMethodsV1.xDeviceCharacteristics = quotaDeviceCharacteristics;
  gQuota.sIoMethodsV2 = gQuota.sIoMethodsV1;
  gQuota.sIoMethodsV2.iVersion = 2;
  gQuota.sIoMethodsV2.xShmMap = quotaShmMap;
  gQuota.sIoMethodsV2.xShmLock = quotaShmLock;
  gQuota.sIoMethodsV2.xShmBarrier = quotaShmBarrier;
  gQuota.sIoMethodsV2.xShmUnmap = quotaShmUnmap;
  sqlite3_vfs_register(&gQuota.sThisVfs, makeDefault);
  return SQLITE_OK;
}










int sqlite3_quota_shutdown(void){
  quotaGroup *pGroup;
  if( gQuota.isInitialized==0 ) return SQLITE_MISUSE;
  for(pGroup=gQuota.pGroup; pGroup; pGroup=pGroup->pNext){
    if( quotaGroupOpenFileCount(pGroup)>0 ) return SQLITE_MISUSE;
  }
  while( gQuota.pGroup ){
    pGroup = gQuota.pGroup;
    gQuota.pGroup = pGroup->pNext;
    pGroup->iLimit = 0;
    assert( quotaGroupOpenFileCount(pGroup)==0 );
    quotaGroupDeref(pGroup);
  }
  gQuota.isInitialized = 0;
  sqlite3_mutex_free(gQuota.pMutex);
  sqlite3_vfs_unregister(&gQuota.sThisVfs);
  memset(&gQuota, 0, sizeof(gQuota));
  return SQLITE_OK;
}





















int sqlite3_quota_set(
  const char *zPattern,           
  sqlite3_int64 iLimit,           
  void (*xCallback)(              
     const char *zFilename,         
     sqlite3_int64 *piLimit,        
     sqlite3_int64 iSize,           
     void *pArg                     
  ),
  void *pArg,                     
  void (*xDestroy)(void*)         
){
  quotaGroup *pGroup;
  quotaEnter();
  pGroup = gQuota.pGroup;
  while( pGroup && strcmp(pGroup->zPattern, zPattern)!=0 ){
    pGroup = pGroup->pNext;
  }
  if( pGroup==0 ){
    int nPattern = strlen(zPattern);
    if( iLimit<=0 ){
      quotaLeave();
      return SQLITE_OK;
    }
    pGroup = (quotaGroup *)sqlite3_malloc( sizeof(*pGroup) + nPattern + 1 );
    if( pGroup==0 ){
      quotaLeave();
      return SQLITE_NOMEM;
    }
    memset(pGroup, 0, sizeof(*pGroup));
    pGroup->zPattern = (char*)&pGroup[1];
    memcpy((char *)pGroup->zPattern, zPattern, nPattern+1);
    if( gQuota.pGroup ) gQuota.pGroup->ppPrev = &pGroup->pNext;
    pGroup->pNext = gQuota.pGroup;
    pGroup->ppPrev = &gQuota.pGroup;
    gQuota.pGroup = pGroup;
  }
  pGroup->iLimit = iLimit;
  pGroup->xCallback = xCallback;
  if( pGroup->xDestroy && pGroup->pArg!=pArg ){
    pGroup->xDestroy(pGroup->pArg);
  }
  pGroup->pArg = pArg;
  pGroup->xDestroy = xDestroy;
  quotaGroupDeref(pGroup);
  quotaLeave();
  return SQLITE_OK;
}





int sqlite3_quota_file(const char *zFilename){
  char *zFull;
  sqlite3_file *fd;
  int rc;
  int outFlags = 0;
  sqlite3_int64 iSize;
  int nAlloc = gQuota.sThisVfs.szOsFile + gQuota.sThisVfs.mxPathname+2;

  
  fd = (sqlite3_file *)sqlite3_malloc(nAlloc);
  if( fd==0 ){
    rc = SQLITE_NOMEM;
  }else{
    zFull = &((char *)fd)[gQuota.sThisVfs.szOsFile];
    rc = gQuota.pOrigVfs->xFullPathname(gQuota.pOrigVfs, zFilename,
        gQuota.sThisVfs.mxPathname+1, zFull);
  }

  if( rc==SQLITE_OK ){
    zFull[strlen(zFull)+1] = '\0';
    rc = quotaOpen(&gQuota.sThisVfs, zFull, fd, 
                   SQLITE_OPEN_READONLY | SQLITE_OPEN_MAIN_DB, &outFlags);
    if( rc==SQLITE_OK ){
      fd->pMethods->xFileSize(fd, &iSize);
      fd->pMethods->xClose(fd);
    }else if( rc==SQLITE_CANTOPEN ){
      quotaGroup *pGroup;
      quotaFile *pFile;
      quotaEnter();
      pGroup = quotaGroupFind(zFull);
      if( pGroup ){
        pFile = quotaFindFile(pGroup, zFull, 0);
        if( pFile ) quotaRemoveFile(pFile);
      }
      quotaLeave();
    }
  }

  sqlite3_free(fd);
  return rc;
}




quota_FILE *sqlite3_quota_fopen(const char *zFilename, const char *zMode){
  quota_FILE *p = 0;
  char *zFull = 0;
  char *zFullTranslated;
  int rc;
  quotaGroup *pGroup;
  quotaFile *pFile;

  zFull = (char*)sqlite3_malloc(gQuota.sThisVfs.mxPathname + 1);
  if( zFull==0 ) return 0;
  rc = gQuota.pOrigVfs->xFullPathname(gQuota.pOrigVfs, zFilename,
                                      gQuota.sThisVfs.mxPathname+1, zFull);
  if( rc ) goto quota_fopen_error;
  p = (quota_FILE*)sqlite3_malloc(sizeof(*p));
  if( p==0 ) goto quota_fopen_error;
  memset(p, 0, sizeof(*p));
  zFullTranslated = quota_utf8_to_mbcs(zFull);
  if( zFullTranslated==0 ) goto quota_fopen_error;
  p->f = fopen(zFullTranslated, zMode);
  quota_mbcs_free(zFullTranslated);
  if( p->f==0 ) goto quota_fopen_error;
  quotaEnter();
  pGroup = quotaGroupFind(zFull);
  if( pGroup ){
    pFile = quotaFindFile(pGroup, zFull, 1);
    if( pFile==0 ){
      quotaLeave();
      goto quota_fopen_error;
    }
    pFile->nRef++;
    p->pFile = pFile;
  }
  quotaLeave();
  sqlite3_free(zFull);
  return p;

quota_fopen_error:
  sqlite3_free(zFull);
  if( p && p->f ) fclose(p->f);
  sqlite3_free(p);
  return 0;
}




size_t sqlite3_quota_fread(
  void *pBuf,            
  size_t size,           
  size_t nmemb,          
  quota_FILE *p          
){
  return fread(pBuf, size, nmemb, p->f);
}





size_t sqlite3_quota_fwrite(
  void *pBuf,            
  size_t size,           
  size_t nmemb,          
  quota_FILE *p          
){
  sqlite3_int64 iOfst;
  sqlite3_int64 iEnd;
  sqlite3_int64 szNew;
  quotaFile *pFile;
  
  iOfst = ftell(p->f);
  iEnd = iOfst + size*nmemb;
  pFile = p->pFile;
  if( pFile && pFile->iSize<iEnd ){
    quotaGroup *pGroup = pFile->pGroup;
    quotaEnter();
    szNew = pGroup->iSize - pFile->iSize + iEnd;
    if( szNew>pGroup->iLimit && pGroup->iLimit>0 ){
      if( pGroup->xCallback ){
        pGroup->xCallback(pFile->zFilename, &pGroup->iLimit, szNew, 
                          pGroup->pArg);
      }
      if( szNew>pGroup->iLimit && pGroup->iLimit>0 ){
        iEnd = pGroup->iLimit - pGroup->iSize + pFile->iSize;
        nmemb = (iEnd - iOfst)/size;
        iEnd = iOfst + size*nmemb;
        szNew = pGroup->iSize - pFile->iSize + iEnd;
      }
    }
    pGroup->iSize = szNew;
    pFile->iSize = iEnd;
    quotaLeave();
  }
  return fwrite(pBuf, size, nmemb, p->f);
}




int sqlite3_quota_fclose(quota_FILE *p){
  int rc;
  quotaFile *pFile;
  rc = fclose(p->f);
  pFile = p->pFile;
  if( pFile ){
    quotaEnter();
    pFile->nRef--;
    if( pFile->nRef==0 ){
      quotaGroup *pGroup = pFile->pGroup;
      if( pFile->deleteOnClose ){
        gQuota.pOrigVfs->xDelete(gQuota.pOrigVfs, pFile->zFilename, 0);
        quotaRemoveFile(pFile);
      }
      quotaGroupDeref(pGroup);
    }
    quotaLeave();
  }
  sqlite3_free(p);
  return rc;
}




int sqlite3_quota_fflush(quota_FILE *p, int doFsync){
  int rc;
  rc = fflush(p->f);
  if( rc==0 && doFsync ){
#if SQLITE_OS_UNIX
    rc = fsync(fileno(p->f));
#endif
#if SQLITE_OS_WIN
    rc = _commit(_fileno(p->f));
#endif
  }
  return rc!=0;
}




int sqlite3_quota_fseek(quota_FILE *p, long offset, int whence){
  return fseek(p->f, offset, whence);
}




void sqlite3_quota_rewind(quota_FILE *p){
  rewind(p->f);
}




long sqlite3_quota_ftell(quota_FILE *p){
  return ftell(p->f);
}




int sqlite3_quota_remove(const char *zFilename){
  char *zFull;            
  int nFull;              
  int rc;                 
  quotaGroup *pGroup;     
  quotaFile *pFile;       
  quotaFile *pNextFile;   
  int diff;               
  char c;                 

  zFull = (char*)sqlite3_malloc(gQuota.sThisVfs.mxPathname + 1);
  if( zFull==0 ) return SQLITE_NOMEM;
  rc = gQuota.pOrigVfs->xFullPathname(gQuota.pOrigVfs, zFilename,
                                      gQuota.sThisVfs.mxPathname+1, zFull);
  if( rc ){
    sqlite3_free(zFull);
    return rc;
  }

  


  nFull = strlen(zFull);
  if( nFull>0 && (zFull[nFull-1]=='/' || zFull[nFull-1]=='\\') ){
    nFull--;
    zFull[nFull] = 0;
  }

  quotaEnter();
  pGroup = quotaGroupFind(zFull);
  if( pGroup ){
    for(pFile=pGroup->pFiles; pFile && rc==SQLITE_OK; pFile=pNextFile){
      pNextFile = pFile->pNext;
      diff = memcmp(zFull, pFile->zFilename, nFull);
      if( diff==0 && ((c = pFile->zFilename[nFull])==0 || c=='/' || c=='\\') ){
        if( pFile->nRef ){
          pFile->deleteOnClose = 1;
        }else{
          rc = gQuota.pOrigVfs->xDelete(gQuota.pOrigVfs, pFile->zFilename, 0);
          quotaRemoveFile(pFile);
          quotaGroupDeref(pGroup);
        }
      }
    }
  }
  quotaLeave();
  sqlite3_free(zFull);
  return rc;
}
  

#ifdef SQLITE_TEST
#include <tcl.h>




typedef struct TclQuotaCallback TclQuotaCallback;
struct TclQuotaCallback {
  Tcl_Interp *interp;    
  Tcl_Obj *pScript;      
};

extern const char *sqlite3TestErrorName(int);





static void tclQuotaCallback(
  const char *zFilename,          
  sqlite3_int64 *piLimit,         
  sqlite3_int64 iSize,            
  void *pArg                      
){
  TclQuotaCallback *p;            
  Tcl_Obj *pEval;                 
  Tcl_Obj *pVarname;              
  unsigned int rnd;               
  int rc;                         

  p = (TclQuotaCallback *)pArg;
  if( p==0 ) return;

  pVarname = Tcl_NewStringObj("::piLimit_", -1);
  Tcl_IncrRefCount(pVarname);
  sqlite3_randomness(sizeof(rnd), (void *)&rnd);
  Tcl_AppendObjToObj(pVarname, Tcl_NewIntObj((int)(rnd&0x7FFFFFFF)));
  Tcl_ObjSetVar2(p->interp, pVarname, 0, Tcl_NewWideIntObj(*piLimit), 0);

  pEval = Tcl_DuplicateObj(p->pScript);
  Tcl_IncrRefCount(pEval);
  Tcl_ListObjAppendElement(0, pEval, Tcl_NewStringObj(zFilename, -1));
  Tcl_ListObjAppendElement(0, pEval, pVarname);
  Tcl_ListObjAppendElement(0, pEval, Tcl_NewWideIntObj(iSize));
  rc = Tcl_EvalObjEx(p->interp, pEval, TCL_EVAL_GLOBAL);

  if( rc==TCL_OK ){
    Tcl_Obj *pLimit = Tcl_ObjGetVar2(p->interp, pVarname, 0, 0);
    rc = Tcl_GetWideIntFromObj(p->interp, pLimit, piLimit);
    Tcl_UnsetVar(p->interp, Tcl_GetString(pVarname), 0);
  }

  Tcl_DecrRefCount(pEval);
  Tcl_DecrRefCount(pVarname);
  if( rc!=TCL_OK ) Tcl_BackgroundError(p->interp);
}




static void tclCallbackDestructor(void *pObj){
  TclQuotaCallback *p = (TclQuotaCallback*)pObj;
  if( p ){
    Tcl_DecrRefCount(p->pScript);
    sqlite3_free((char *)p);
  }
}




static int test_quota_initialize(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zName;              
  int makeDefault;                
  int rc;                         

  
  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "NAME MAKEDEFAULT");
    return TCL_ERROR;
  }
  zName = Tcl_GetString(objv[1]);
  if( Tcl_GetBooleanFromObj(interp, objv[2], &makeDefault) ) return TCL_ERROR;
  if( zName[0]=='\0' ) zName = 0;

  
  rc = sqlite3_quota_initialize(zName, makeDefault);
  Tcl_SetResult(interp, (char *)sqlite3TestErrorName(rc), TCL_STATIC);

  return TCL_OK;
}




static int test_quota_shutdown(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc;                         

  if( objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }

  
  rc = sqlite3_quota_shutdown();
  Tcl_SetResult(interp, (char *)sqlite3TestErrorName(rc), TCL_STATIC);

  return TCL_OK;
}




static int test_quota_set(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zPattern;           
  sqlite3_int64 iLimit;           
  Tcl_Obj *pScript;               
  int rc;                         
  TclQuotaCallback *p;            
  int nScript;                    
  void (*xDestroy)(void*);        
  void (*xCallback)(const char *, sqlite3_int64 *, sqlite3_int64, void *);

  
  if( objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "PATTERN LIMIT SCRIPT");
    return TCL_ERROR;
  }
  zPattern = Tcl_GetString(objv[1]);
  if( Tcl_GetWideIntFromObj(interp, objv[2], &iLimit) ) return TCL_ERROR;
  pScript = objv[3];
  Tcl_GetStringFromObj(pScript, &nScript);

  if( nScript>0 ){
    
    p = (TclQuotaCallback *)sqlite3_malloc(sizeof(TclQuotaCallback));
    if( !p ){
      Tcl_SetResult(interp, (char *)"SQLITE_NOMEM", TCL_STATIC);
      return TCL_OK;
    }
    memset(p, 0, sizeof(TclQuotaCallback));
    p->interp = interp;
    Tcl_IncrRefCount(pScript);
    p->pScript = pScript;
    xDestroy = tclCallbackDestructor;
    xCallback = tclQuotaCallback;
  }else{
    p = 0;
    xDestroy = 0;
    xCallback = 0;
  }

  
  rc = sqlite3_quota_set(zPattern, iLimit, xCallback, (void*)p, xDestroy);

  Tcl_SetResult(interp, (char *)sqlite3TestErrorName(rc), TCL_STATIC);
  return TCL_OK;
}




static int test_quota_file(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zFilename;          
  int rc;                         

  
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "FILENAME");
    return TCL_ERROR;
  }
  zFilename = Tcl_GetString(objv[1]);

  
  rc = sqlite3_quota_file(zFilename);

  Tcl_SetResult(interp, (char *)sqlite3TestErrorName(rc), TCL_STATIC);
  return TCL_OK;
}




static int test_quota_dump(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  Tcl_Obj *pResult;
  Tcl_Obj *pGroupTerm;
  Tcl_Obj *pFileTerm;
  quotaGroup *pGroup;
  quotaFile *pFile;

  pResult = Tcl_NewObj();
  quotaEnter();
  for(pGroup=gQuota.pGroup; pGroup; pGroup=pGroup->pNext){
    pGroupTerm = Tcl_NewObj();
    Tcl_ListObjAppendElement(interp, pGroupTerm,
          Tcl_NewStringObj(pGroup->zPattern, -1));
    Tcl_ListObjAppendElement(interp, pGroupTerm,
          Tcl_NewWideIntObj(pGroup->iLimit));
    Tcl_ListObjAppendElement(interp, pGroupTerm,
          Tcl_NewWideIntObj(pGroup->iSize));
    for(pFile=pGroup->pFiles; pFile; pFile=pFile->pNext){
      int i;
      char zTemp[1000];
      pFileTerm = Tcl_NewObj();
      sqlite3_snprintf(sizeof(zTemp), zTemp, "%s", pFile->zFilename);
      for(i=0; zTemp[i]; i++){ if( zTemp[i]=='\\' ) zTemp[i] = '/'; }
      Tcl_ListObjAppendElement(interp, pFileTerm,
            Tcl_NewStringObj(zTemp, -1));
      Tcl_ListObjAppendElement(interp, pFileTerm,
            Tcl_NewWideIntObj(pFile->iSize));
      Tcl_ListObjAppendElement(interp, pFileTerm,
            Tcl_NewWideIntObj(pFile->nRef));
      Tcl_ListObjAppendElement(interp, pFileTerm,
            Tcl_NewWideIntObj(pFile->deleteOnClose));
      Tcl_ListObjAppendElement(interp, pGroupTerm, pFileTerm);
    }
    Tcl_ListObjAppendElement(interp, pResult, pGroupTerm);
  }
  quotaLeave();
  Tcl_SetObjResult(interp, pResult);
  return TCL_OK;
}




static int test_quota_fopen(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zFilename;          
  const char *zMode;              
  quota_FILE *p;                  
  char zReturn[50];               

  
  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "FILENAME MODE");
    return TCL_ERROR;
  }
  zFilename = Tcl_GetString(objv[1]);
  zMode = Tcl_GetString(objv[2]);
  p = sqlite3_quota_fopen(zFilename, zMode);
  sqlite3_snprintf(sizeof(zReturn), zReturn, "%p", p);
  Tcl_SetResult(interp, zReturn, TCL_VOLATILE);
  return TCL_OK;
}


extern void *sqlite3TestTextToPtr(const char*);




static int test_quota_fread(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  quota_FILE *p;
  char *zBuf;
  int sz;
  int nElem;
  int got;

  if( objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE SIZE NELEM");
    return TCL_ERROR;
  }
  p = sqlite3TestTextToPtr(Tcl_GetString(objv[1]));
  if( Tcl_GetIntFromObj(interp, objv[2], &sz) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[3], &nElem) ) return TCL_ERROR;
  zBuf = (char*)sqlite3_malloc( sz*nElem + 1 );
  if( zBuf==0 ){
    Tcl_SetResult(interp, "out of memory", TCL_STATIC);
    return TCL_ERROR;
  }
  got = sqlite3_quota_fread(zBuf, sz, nElem, p);
  if( got<0 ) got = 0;
  zBuf[got*sz] = 0;
  Tcl_SetResult(interp, zBuf, TCL_VOLATILE);
  sqlite3_free(zBuf);
  return TCL_OK;
}




static int test_quota_fwrite(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  quota_FILE *p;
  char *zBuf;
  int sz;
  int nElem;
  int got;

  if( objc!=5 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE SIZE NELEM CONTENT");
    return TCL_ERROR;
  }
  p = sqlite3TestTextToPtr(Tcl_GetString(objv[1]));
  if( Tcl_GetIntFromObj(interp, objv[2], &sz) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[3], &nElem) ) return TCL_ERROR;
  zBuf = Tcl_GetString(objv[4]);
  got = sqlite3_quota_fwrite(zBuf, sz, nElem, p);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(got));
  return TCL_OK;
}




static int test_quota_fclose(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  quota_FILE *p;
  int rc;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE");
    return TCL_ERROR;
  }
  p = sqlite3TestTextToPtr(Tcl_GetString(objv[1]));
  rc = sqlite3_quota_fclose(p);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}




static int test_quota_fflush(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  quota_FILE *p;
  int rc;
  int doSync = 0;

  if( objc!=2 && objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE ?HARDSYNC?");
    return TCL_ERROR;
  }
  p = sqlite3TestTextToPtr(Tcl_GetString(objv[1]));
  if( objc==3 ){
    if( Tcl_GetBooleanFromObj(interp, objv[2], &doSync) ) return TCL_ERROR;
  }
  rc = sqlite3_quota_fflush(p, doSync);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}




static int test_quota_fseek(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  quota_FILE *p;
  int ofst;
  const char *zWhence;
  int whence;
  int rc;

  if( objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE OFFSET WHENCE");
    return TCL_ERROR;
  }
  p = sqlite3TestTextToPtr(Tcl_GetString(objv[1]));
  if( Tcl_GetIntFromObj(interp, objv[2], &ofst) ) return TCL_ERROR;
  zWhence = Tcl_GetString(objv[3]);
  if( strcmp(zWhence, "SEEK_SET")==0 ){
    whence = SEEK_SET;
  }else if( strcmp(zWhence, "SEEK_CUR")==0 ){
    whence = SEEK_CUR;
  }else if( strcmp(zWhence, "SEEK_END")==0 ){
    whence = SEEK_END;
  }else{
    Tcl_AppendResult(interp,
           "WHENCE should be SEEK_SET, SEEK_CUR, or SEEK_END", (char*)0);
    return TCL_ERROR;
  }
  rc = sqlite3_quota_fseek(p, ofst, whence);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}




static int test_quota_rewind(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  quota_FILE *p;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE");
    return TCL_ERROR;
  }
  p = sqlite3TestTextToPtr(Tcl_GetString(objv[1]));
  sqlite3_quota_rewind(p);
  return TCL_OK;
}




static int test_quota_ftell(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  quota_FILE *p;
  sqlite3_int64 x;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE");
    return TCL_ERROR;
  }
  p = sqlite3TestTextToPtr(Tcl_GetString(objv[1]));
  x = sqlite3_quota_ftell(p);
  Tcl_SetObjResult(interp, Tcl_NewWideIntObj(x));
  return TCL_OK;
}




static int test_quota_remove(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zFilename;          
  int rc;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "FILENAME");
    return TCL_ERROR;
  }
  zFilename = Tcl_GetString(objv[1]);
  rc = sqlite3_quota_remove(zFilename);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}







static int test_quota_glob(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zPattern;          
  const char *zText;             
  int rc;
  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "PATTERN TEXT");
    return TCL_ERROR;
  }
  zPattern = Tcl_GetString(objv[1]);
  zText = Tcl_GetString(objv[2]);
  rc = quotaStrglob(zPattern, zText);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}






int Sqlitequota_Init(Tcl_Interp *interp){
  static struct {
     char *zName;
     Tcl_ObjCmdProc *xProc;
  } aCmd[] = {
    { "sqlite3_quota_initialize", test_quota_initialize },
    { "sqlite3_quota_shutdown",   test_quota_shutdown },
    { "sqlite3_quota_set",        test_quota_set },
    { "sqlite3_quota_file",       test_quota_file },
    { "sqlite3_quota_dump",       test_quota_dump },
    { "sqlite3_quota_fopen",      test_quota_fopen },
    { "sqlite3_quota_fread",      test_quota_fread },
    { "sqlite3_quota_fwrite",     test_quota_fwrite },
    { "sqlite3_quota_fclose",     test_quota_fclose },
    { "sqlite3_quota_fflush",     test_quota_fflush },
    { "sqlite3_quota_fseek",      test_quota_fseek },
    { "sqlite3_quota_rewind",     test_quota_rewind },
    { "sqlite3_quota_ftell",      test_quota_ftell },
    { "sqlite3_quota_remove",     test_quota_remove },
    { "sqlite3_quota_glob",       test_quota_glob },
  };
  int i;

  for(i=0; i<sizeof(aCmd)/sizeof(aCmd[0]); i++){
    Tcl_CreateObjCommand(interp, aCmd[i].zName, aCmd[i].xProc, 0, 0);
  }

  return TCL_OK;
}
#endif
