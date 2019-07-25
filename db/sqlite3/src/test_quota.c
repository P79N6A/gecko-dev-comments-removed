




























#include "sqlite3.h"
#include <string.h>
#include <assert.h>




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
  quotaFile *pNext, **ppPrev;     
};







struct quotaConn {
  sqlite3_file base;              
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





static void quotaGroupDeref(quotaGroup *pGroup){
  if( pGroup->pFiles==0 && pGroup->iLimit==0 ){
    *pGroup->ppPrev = pGroup->pNext;
    if( pGroup->pNext ) pGroup->pNext->ppPrev = pGroup->ppPrev;
    if( pGroup->xDestroy ) pGroup->xDestroy(pGroup->pArg);
    sqlite3_free(pGroup);
  }
}
















static int quotaStrglob(const char *zGlob, const char *z){
  int c, c2;
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
      while( (c2 = (*(z++)))!=0 ){
        while( c2!=c ){
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
      for(pFile=pGroup->pFiles; pFile && strcmp(pFile->zFilename, zName);
          pFile=pFile->pNext){}
      if( pFile==0 ){
        int nName = strlen(zName);
        pFile = (quotaFile *)sqlite3_malloc( sizeof(*pFile) + nName + 1 );
        if( pFile==0 ){
          quotaLeave();
          pSubOpen->pMethods->xClose(pSubOpen);
          return SQLITE_NOMEM;
        }
        memset(pFile, 0, sizeof(*pFile));
        pFile->zFilename = (char*)&pFile[1];
        memcpy(pFile->zFilename, zName, nName+1);
        pFile->pNext = pGroup->pFiles;
        if( pGroup->pFiles ) pGroup->pFiles->ppPrev = &pFile->pNext;
        pFile->ppPrev = &pGroup->pFiles;
        pGroup->pFiles = pFile;
        pFile->pGroup = pGroup;
      }
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
    pGroup->iSize -= pFile->iSize;
    if( pFile->pNext ) pFile->pNext->ppPrev = pFile->ppPrev;
    *pFile->ppPrev = pFile->pNext;
    quotaGroupDeref(pGroup);
    sqlite3_free(pFile);
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
  return pSubOpen->pMethods->xFileControl(pSubOpen, op, pArg);
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
    if( pGroup->pFiles ) return SQLITE_MISUSE;
  }
  while( gQuota.pGroup ){
    pGroup = gQuota.pGroup;
    gQuota.pGroup = pGroup->pNext;
    pGroup->iLimit = 0;
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
      pFileTerm = Tcl_NewObj();
      Tcl_ListObjAppendElement(interp, pFileTerm,
            Tcl_NewStringObj(pFile->zFilename, -1));
      Tcl_ListObjAppendElement(interp, pFileTerm,
            Tcl_NewWideIntObj(pFile->iSize));
      Tcl_ListObjAppendElement(interp, pFileTerm,
            Tcl_NewWideIntObj(pFile->nRef));
      Tcl_ListObjAppendElement(interp, pGroupTerm, pFileTerm);
    }
    Tcl_ListObjAppendElement(interp, pResult, pGroupTerm);
  }
  quotaLeave();
  Tcl_SetObjResult(interp, pResult);
  return TCL_OK;
}






int Sqlitequota_Init(Tcl_Interp *interp){
  static struct {
     char *zName;
     Tcl_ObjCmdProc *xProc;
  } aCmd[] = {
    { "sqlite3_quota_initialize", test_quota_initialize },
    { "sqlite3_quota_shutdown", test_quota_shutdown },
    { "sqlite3_quota_set", test_quota_set },
    { "sqlite3_quota_dump", test_quota_dump },
  };
  int i;

  for(i=0; i<sizeof(aCmd)/sizeof(aCmd[0]); i++){
    Tcl_CreateObjCommand(interp, aCmd[i].zName, aCmd[i].xProc, 0, 0);
  }

  return TCL_OK;
}
#endif
