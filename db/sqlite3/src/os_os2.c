














#if (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 3) && defined(OS2_HIGH_MEMORY)

#include <os2safe.h>
#endif

#include "sqliteInt.h"
#include "os.h"

#if OS_OS2




#if defined(THREADSAFE) && THREADSAFE
# define SQLITE_OS2_THREADS 1
#endif




#include "os_common.h"





typedef struct os2File os2File;
struct os2File {
  IoMethod const *pMethod;  
  HFILE h;                  
  int delOnClose;           
  char* pathToDel;          
  unsigned char locktype;   
};






#ifndef SQLITE_OMIT_DISKIO




int sqlite3Os2Delete( const char *zFilename ){
  DosDelete( (PSZ)zFilename );
  TRACE2( "DELETE \"%s\"\n", zFilename );
  return SQLITE_OK;
}




int sqlite3Os2FileExists( const char *zFilename ){
  FILESTATUS3 fsts3ConfigInfo;
  memset(&fsts3ConfigInfo, 0, sizeof(fsts3ConfigInfo));
  return DosQueryPathInfo( (PSZ)zFilename, FIL_STANDARD,
        &fsts3ConfigInfo, sizeof(FILESTATUS3) ) == NO_ERROR;
}


int allocateOs2File( os2File *pInit, OsFile **pld );














int sqlite3Os2OpenReadWrite(
  const char *zFilename,
  OsFile **pld,
  int *pReadonly
){
  os2File  f;
  HFILE    hf;
  ULONG    ulAction;
  APIRET   rc;

  assert( *pld == 0 );
  rc = DosOpen( (PSZ)zFilename, &hf, &ulAction, 0L,
            FILE_ARCHIVED | FILE_NORMAL,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_RANDOM |
                    OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, (PEAOP2)NULL );
  if( rc != NO_ERROR ){
    rc = DosOpen( (PSZ)zFilename, &hf, &ulAction, 0L,
            FILE_ARCHIVED | FILE_NORMAL,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_RANDOM |
                        OPEN_SHARE_DENYWRITE | OPEN_ACCESS_READONLY, (PEAOP2)NULL );
    if( rc != NO_ERROR ){
        return SQLITE_CANTOPEN;
    }
    *pReadonly = 1;
  }
  else{
    *pReadonly = 0;
  }
  f.h = hf;
  f.locktype = NO_LOCK;
  f.delOnClose = 0;
  f.pathToDel = NULL;
  OpenCounter(+1);
  TRACE3( "OPEN R/W %d \"%s\"\n", hf, zFilename );
  return allocateOs2File( &f, pld );
}
















int sqlite3Os2OpenExclusive( const char *zFilename, OsFile **pld, int delFlag ){
  os2File  f;
  HFILE    hf;
  ULONG    ulAction;
  APIRET   rc;

  assert( *pld == 0 );
  rc = DosOpen( (PSZ)zFilename, &hf, &ulAction, 0L, FILE_NORMAL,
            OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
            OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_RANDOM |
                OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE, (PEAOP2)NULL );
  if( rc != NO_ERROR ){
    return SQLITE_CANTOPEN;
  }

  f.h = hf;
  f.locktype = NO_LOCK;
  f.delOnClose = delFlag ? 1 : 0;
  f.pathToDel = delFlag ? sqlite3OsFullPathname( zFilename ) : NULL;
  OpenCounter( +1 );
  if( delFlag ) DosForceDelete( sqlite3OsFullPathname( zFilename ) );
  TRACE3( "OPEN EX %d \"%s\"\n", hf, sqlite3OsFullPathname ( zFilename ) );
  return allocateOs2File( &f, pld );
}








int sqlite3Os2OpenReadOnly( const char *zFilename, OsFile **pld ){
  os2File  f;
  HFILE    hf;
  ULONG    ulAction;
  APIRET   rc;

  assert( *pld == 0 );
  rc = DosOpen( (PSZ)zFilename, &hf, &ulAction, 0L,
            FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
            OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_RANDOM |
                OPEN_SHARE_DENYWRITE | OPEN_ACCESS_READONLY, (PEAOP2)NULL );
  if( rc != NO_ERROR ){
    return SQLITE_CANTOPEN;
  }
  f.h = hf;
  f.locktype = NO_LOCK;
  f.delOnClose = 0;
  f.pathToDel = NULL;
  OpenCounter( +1 );
  TRACE3( "OPEN RO %d \"%s\"\n", hf, zFilename );
  return allocateOs2File( &f, pld );
}

















int os2OpenDirectory(
  OsFile *id,
  const char *zDirname
){
  return SQLITE_OK;
}






char *sqlite3_temp_directory = 0;





int sqlite3Os2TempFileName( char *zBuf ){
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  int i, j;
  PSZ zTempPath = 0;
  if( DosScanEnv( "TEMP", &zTempPath ) ){
    if( DosScanEnv( "TMP", &zTempPath ) ){
      if( DosScanEnv( "TMPDIR", &zTempPath ) ){
           ULONG ulDriveNum = 0, ulDriveMap = 0;
           DosQueryCurrentDisk( &ulDriveNum, &ulDriveMap );
           sprintf( zTempPath, "%c:", (char)( 'A' + ulDriveNum - 1 ) );
      }
    }
  }
  for(;;){
      sprintf( zBuf, "%s\\"TEMP_FILE_PREFIX, zTempPath );
      j = strlen( zBuf );
      sqlite3Randomness( 15, &zBuf[j] );
      for( i = 0; i < 15; i++, j++ ){
        zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
      }
      zBuf[j] = 0;
      if( !sqlite3OsFileExists( zBuf ) ) break;
  }
  TRACE2( "TEMP FILENAME: %s\n", zBuf );
  return SQLITE_OK;
}




int os2Close( OsFile **pld ){
  os2File *pFile;
  if( pld && (pFile = (os2File*)*pld)!=0 ){
    TRACE2( "CLOSE %d\n", pFile->h );
    DosClose( pFile->h );
    pFile->locktype = NO_LOCK;
    if( pFile->delOnClose != 0 ){
        DosForceDelete( pFile->pathToDel );
    }
    *pld = 0;
    OpenCounter( -1 );
  }

  return SQLITE_OK;
}






int os2Read( OsFile *id, void *pBuf, int amt ){
  ULONG got;
  assert( id!=0 );
  SimulateIOError( SQLITE_IOERR );
  TRACE3( "READ %d lock=%d\n", ((os2File*)id)->h, ((os2File*)id)->locktype );
  DosRead( ((os2File*)id)->h, pBuf, amt, &got );
  return (got == (ULONG)amt) ? SQLITE_OK : SQLITE_IOERR;
}





int os2Write( OsFile *id, const void *pBuf, int amt ){
  APIRET rc=NO_ERROR;
  ULONG wrote;
  assert( id!=0 );
  SimulateIOError( SQLITE_IOERR );
  SimulateDiskfullError;
  TRACE3( "WRITE %d lock=%d\n", ((os2File*)id)->h, ((os2File*)id)->locktype );
  while( amt > 0 &&
      (rc = DosWrite( ((os2File*)id)->h, (PVOID)pBuf, amt, &wrote )) && wrote > 0 ){
      amt -= wrote;
      pBuf = &((char*)pBuf)[wrote];
  }

  return ( rc != NO_ERROR || amt > (int)wrote ) ? SQLITE_FULL : SQLITE_OK;
}




int os2Seek( OsFile *id, i64 offset ){
  APIRET rc;
  ULONG filePointer = 0L;
  assert( id!=0 );
  rc = DosSetFilePtr( ((os2File*)id)->h, offset, FILE_BEGIN, &filePointer );
  TRACE3( "SEEK %d %lld\n", ((os2File*)id)->h, offset );
  return rc == NO_ERROR ? SQLITE_OK : SQLITE_IOERR;
}




int os2Sync( OsFile *id, int dataOnly ){
  assert( id!=0 );
  TRACE3( "SYNC %d lock=%d\n", ((os2File*)id)->h, ((os2File*)id)->locktype );
  return DosResetBuffer( ((os2File*)id)->h ) ? SQLITE_IOERR : SQLITE_OK;
}





int sqlite3Os2SyncDirectory( const char *zDirname ){
  SimulateIOError( SQLITE_IOERR );
  return SQLITE_OK;
}




int os2Truncate( OsFile *id, i64 nByte ){
  APIRET rc;
  ULONG upperBits = nByte>>32;
  assert( id!=0 );
  TRACE3( "TRUNCATE %d %lld\n", ((os2File*)id)->h, nByte );
  SimulateIOError( SQLITE_IOERR );
  rc = DosSetFilePtr( ((os2File*)id)->h, nByte, FILE_BEGIN, &upperBits );
  if( rc != NO_ERROR ){
    return SQLITE_IOERR;
  }
  rc = DosSetFilePtr( ((os2File*)id)->h, 0L, FILE_END, &upperBits );
  return rc == NO_ERROR ? SQLITE_OK : SQLITE_IOERR;
}




int os2FileSize( OsFile *id, i64 *pSize ){
  APIRET rc;
  FILESTATUS3 fsts3FileInfo;
  memset(&fsts3FileInfo, 0, sizeof(fsts3FileInfo));
  assert( id!=0 );
  SimulateIOError( SQLITE_IOERR );
  rc = DosQueryFileInfo( ((os2File*)id)->h, FIL_STANDARD, &fsts3FileInfo, sizeof(FILESTATUS3) );
  if( rc == NO_ERROR ){
    *pSize = fsts3FileInfo.cbFile;
    return SQLITE_OK;
  }
  else{
    return SQLITE_IOERR;
  }
}




static int getReadLock( os2File *id ){
  FILELOCK  LockArea,
            UnlockArea;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  LockArea.lOffset = SHARED_FIRST;
  LockArea.lRange = SHARED_SIZE;
  UnlockArea.lOffset = 0L;
  UnlockArea.lRange = 0L;
  return DosSetFileLocks( id->h, &UnlockArea, &LockArea, 2000L, 1L );
}




static int unlockReadLock( os2File *id ){
  FILELOCK  LockArea,
            UnlockArea;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  LockArea.lOffset = 0L;
  LockArea.lRange = 0L;
  UnlockArea.lOffset = SHARED_FIRST;
  UnlockArea.lRange = SHARED_SIZE;
  return DosSetFileLocks( id->h, &UnlockArea, &LockArea, 2000L, 1L );
}

#ifndef SQLITE_OMIT_PAGER_PRAGMAS




int sqlite3Os2IsDirWritable( char *zDirname ){
  FILESTATUS3 fsts3ConfigInfo;
  APIRET rc = NO_ERROR;
  memset(&fsts3ConfigInfo, 0, sizeof(fsts3ConfigInfo));
  if( zDirname==0 ) return 0;
  if( strlen(zDirname)>CCHMAXPATH ) return 0;
  rc = DosQueryPathInfo( (PSZ)zDirname, FIL_STANDARD, &fsts3ConfigInfo, sizeof(FILESTATUS3) );
  if( rc != NO_ERROR ) return 0;
  if( (fsts3ConfigInfo.attrFile & FILE_DIRECTORY) != FILE_DIRECTORY ) return 0;

  return 1;
}
#endif 



























int os2Lock( OsFile *id, int locktype ){
  APIRET rc = SQLITE_OK;    
  APIRET res = 1;           
  int newLocktype;       
  int gotPendingLock = 0;
  FILELOCK  LockArea,
            UnlockArea;
  os2File *pFile = (os2File*)id;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  assert( pFile!=0 );
  TRACE4( "LOCK %d %d was %d\n", pFile->h, locktype, pFile->locktype );

  



  if( pFile->locktype>=locktype ){
    return SQLITE_OK;
  }

  

  assert( pFile->locktype!=NO_LOCK || locktype==SHARED_LOCK );
  assert( locktype!=PENDING_LOCK );
  assert( locktype!=RESERVED_LOCK || pFile->locktype==SHARED_LOCK );

  



  newLocktype = pFile->locktype;
  if( pFile->locktype==NO_LOCK
   || (locktype==EXCLUSIVE_LOCK && pFile->locktype==RESERVED_LOCK)
  ){
    int cnt = 3;

    LockArea.lOffset = PENDING_BYTE;
    LockArea.lRange = 1L;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;

    while( cnt-->0 && (res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L) )!=NO_ERROR ){
      


      TRACE2( "could not get a PENDING lock. cnt=%d\n", cnt );
      DosSleep(1);
    }
    gotPendingLock = res;
  }

  

  if( locktype==SHARED_LOCK && res ){
    assert( pFile->locktype==NO_LOCK );
    res = getReadLock(pFile);
    if( res == NO_ERROR ){
      newLocktype = SHARED_LOCK;
    }
  }

  

  if( locktype==RESERVED_LOCK && res ){
    assert( pFile->locktype==SHARED_LOCK );
    LockArea.lOffset = RESERVED_BYTE;
    LockArea.lRange = 1L;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
    if( res == NO_ERROR ){
      newLocktype = RESERVED_LOCK;
    }
  }

  

  if( locktype==EXCLUSIVE_LOCK && res ){
    newLocktype = PENDING_LOCK;
    gotPendingLock = 0;
  }

  

  if( locktype==EXCLUSIVE_LOCK && res ){
    assert( pFile->locktype>=SHARED_LOCK );
    res = unlockReadLock(pFile);
    TRACE2( "unreadlock = %d\n", res );
    LockArea.lOffset = SHARED_FIRST;
    LockArea.lRange = SHARED_SIZE;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
    if( res == NO_ERROR ){
      newLocktype = EXCLUSIVE_LOCK;
    }else{
      TRACE2( "error-code = %d\n", res );
    }
  }

  


  if( gotPendingLock && locktype==SHARED_LOCK ){
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = PENDING_BYTE;
    UnlockArea.lRange = 1L;
    DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
  }

  


  if( res == NO_ERROR ){
    rc = SQLITE_OK;
  }else{
    TRACE4( "LOCK FAILED %d trying for %d but got %d\n", pFile->h,
           locktype, newLocktype );
    rc = SQLITE_BUSY;
  }
  pFile->locktype = newLocktype;
  return rc;
}






int os2CheckReservedLock( OsFile *id ){
  APIRET rc;
  os2File *pFile = (os2File*)id;
  assert( pFile!=0 );
  if( pFile->locktype>=RESERVED_LOCK ){
    rc = 1;
    TRACE3( "TEST WR-LOCK %d %d (local)\n", pFile->h, rc );
  }else{
    FILELOCK  LockArea,
              UnlockArea;
    memset(&LockArea, 0, sizeof(LockArea));
    memset(&UnlockArea, 0, sizeof(UnlockArea));
    LockArea.lOffset = RESERVED_BYTE;
    LockArea.lRange = 1L;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;
    rc = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
    if( rc == NO_ERROR ){
      LockArea.lOffset = 0L;
      LockArea.lRange = 0L;
      UnlockArea.lOffset = RESERVED_BYTE;
      UnlockArea.lRange = 1L;
      rc = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
    }
    TRACE3( "TEST WR-LOCK %d %d (remote)\n", pFile->h, rc );
  }
  return rc;
}












int os2Unlock( OsFile *id, int locktype ){
  int type;
  APIRET rc = SQLITE_OK;
  os2File *pFile = (os2File*)id;
  FILELOCK  LockArea,
            UnlockArea;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  assert( pFile!=0 );
  assert( locktype<=SHARED_LOCK );
  TRACE4( "UNLOCK %d to %d was %d\n", pFile->h, locktype, pFile->locktype );
  type = pFile->locktype;
  if( type>=EXCLUSIVE_LOCK ){
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = SHARED_FIRST;
    UnlockArea.lRange = SHARED_SIZE;
    DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
    if( locktype==SHARED_LOCK && getReadLock(pFile) != NO_ERROR ){
      

      rc = SQLITE_IOERR;
    }
  }
  if( type>=RESERVED_LOCK ){
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = RESERVED_BYTE;
    UnlockArea.lRange = 1L;
    DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
  }
  if( locktype==NO_LOCK && type>=SHARED_LOCK ){
    unlockReadLock(pFile);
  }
  if( type>=PENDING_LOCK ){
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = PENDING_BYTE;
    UnlockArea.lRange = 1L;
    DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 2000L, 1L );
  }
  pFile->locktype = locktype;
  return rc;
}







char *sqlite3Os2FullPathname( const char *zRelative ){
  char *zFull = 0;
  if( strchr(zRelative, ':') ){
    sqlite3SetString( &zFull, zRelative, (char*)0 );
  }else{
    char zBuff[SQLITE_TEMPNAME_SIZE - 2] = {0};
    char zDrive[1] = {0};
    ULONG cbzFullLen = SQLITE_TEMPNAME_SIZE;
    ULONG ulDriveNum = 0;
    ULONG ulDriveMap = 0;
    DosQueryCurrentDisk( &ulDriveNum, &ulDriveMap );
    DosQueryCurrentDir( 0L, zBuff, &cbzFullLen );
    zFull = sqliteMalloc( cbzFullLen );
    sprintf( zDrive, "%c", (char)('A' + ulDriveNum - 1) );
    sqlite3SetString( &zFull, zDrive, ":\\", zBuff, "\\", zRelative, (char*)0 );
  }
  return zFull;
}






static void os2SetFullSync( OsFile *id, int v ){
  return;
}




static int os2FileHandle( OsFile *id ){
  return (int)((os2File*)id)->h;
}





static int os2LockState( OsFile *id ){
  return ((os2File*)id)->locktype;
}





static const IoMethod sqlite3Os2IoMethod = {
  os2Close,
  os2OpenDirectory,
  os2Read,
  os2Write,
  os2Seek,
  os2Truncate,
  os2Sync,
  os2SetFullSync,
  os2FileHandle,
  os2FileSize,
  os2Lock,
  os2Unlock,
  os2LockState,
  os2CheckReservedLock,
};






int allocateOs2File( os2File *pInit, OsFile **pld ){
  os2File *pNew;
  pNew = sqliteMalloc( sizeof(*pNew) );
  if( pNew==0 ){
    DosClose( pInit->h );
    *pld = 0;
    return SQLITE_NOMEM;
  }else{
    *pNew = *pInit;
    pNew->pMethod = &sqlite3Os2IoMethod;
    pNew->locktype = NO_LOCK;
    *pld = (OsFile*)pNew;
    OpenCounter(+1);
    return SQLITE_OK;
  }
}

#endif 










int sqlite3Os2RandomSeed( char *zBuf ){
  











  memset( zBuf, 0, 256 );
  DosGetDateTime( (PDATETIME)zBuf );
  return SQLITE_OK;
}




int sqlite3Os2Sleep( int ms ){
  DosSleep( ms );
  return ms;
}




static int inMutex = 0;
#ifdef SQLITE_OS2_THREADS
static ULONG mutexOwner;
#endif









void sqlite3Os2EnterMutex(){
  PTIB ptib;
#ifdef SQLITE_OS2_THREADS
  DosEnterCritSec();
  DosGetInfoBlocks( &ptib, NULL );
  mutexOwner = ptib->tib_ptib2->tib2_ultid;
#endif
  assert( !inMutex );
  inMutex = 1;
}
void sqlite3Os2LeaveMutex(){
  PTIB ptib;
  assert( inMutex );
  inMutex = 0;
#ifdef SQLITE_OS2_THREADS
  DosGetInfoBlocks( &ptib, NULL );
  assert( mutexOwner == ptib->tib_ptib2->tib2_ultid );
  DosExitCritSec();
#endif
}








int sqlite3Os2InMutex( int thisThreadOnly ){
#ifdef SQLITE_OS2_THREADS
  PTIB ptib;
  DosGetInfoBlocks( &ptib, NULL );
  return inMutex>0 && (thisThreadOnly==0 || mutexOwner==ptib->tib_ptib2->tib2_ultid);
#else
  return inMutex>0;
#endif
}





#ifdef SQLITE_TEST
int sqlite3_current_time = 0;
#endif






int sqlite3Os2CurrentTime( double *prNow ){
  double now;
  USHORT second, minute, hour,
         day, month, year;
  DATETIME dt;
  DosGetDateTime( &dt );
  second = (USHORT)dt.seconds;
  minute = (USHORT)dt.minutes + dt.timezone;
  hour = (USHORT)dt.hours;
  day = (USHORT)dt.day;
  month = (USHORT)dt.month;
  year = (USHORT)dt.year;

  

  
  now = day - 32076 +
    1461*(year + 4800 + (month - 14)/12)/4 +
    367*(month - 2 - (month - 14)/12*12)/12 -
    3*((year + 4900 + (month - 14)/12)/100)/4;

  
  now += (hour + 12.0)/24.0;
  now += minute/1440.0;
  now += second/86400.0;
  *prNow = now;
#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *prNow = sqlite3_current_time/86400.0 + 2440587.5;
  }
#endif
  return 0;
}






#ifdef SQLITE_TEST
int sqlite3_tsd_count = 0;
# define TSD_COUNTER_INCR InterlockedIncrement( &sqlite3_tsd_count )
# define TSD_COUNTER_DECR InterlockedDecrement( &sqlite3_tsd_count )
#else
# define TSD_COUNTER_INCR
# define TSD_COUNTER_DECR
#endif















ThreadData *sqlite3Os2ThreadSpecificData( int allocateFlag ){
  static ThreadData **s_ppTsd = NULL;
  static const ThreadData zeroData = {0, 0, 0};
  ThreadData *pTsd;

  if( !s_ppTsd ){
    sqlite3OsEnterMutex();
    if( !s_ppTsd ){
      PULONG pul;
      APIRET rc = DosAllocThreadLocalMemory(1, &pul);
      if( rc != NO_ERROR ){
        sqlite3OsLeaveMutex();
        return 0;
      }
      s_ppTsd = (ThreadData **)pul;
    }
    sqlite3OsLeaveMutex();
  }
  pTsd = *s_ppTsd;
  if( allocateFlag>0 ){
    if( !pTsd ){
      pTsd = sqlite3OsMalloc( sizeof(zeroData) );
      if( pTsd ){
        *pTsd = zeroData;
        *s_ppTsd = pTsd;
        TSD_COUNTER_INCR;
      }
    }
  }else if( pTsd!=0 && allocateFlag<0
              && memcmp( pTsd, &zeroData, sizeof(ThreadData) )==0 ){
    sqlite3OsFree(pTsd);
    *s_ppTsd = NULL;
    TSD_COUNTER_DECR;
    pTsd = 0;
  }
  return pTsd;
}
#endif 
