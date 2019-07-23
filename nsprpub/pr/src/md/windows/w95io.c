











































#include "primpl.h"
#include <direct.h>
#include <mbstring.h>
#ifdef MOZ_UNICODE
#include <wchar.h>
#endif 

#ifdef WINCE

static HANDLE CreateFileA(LPCSTR lpFileName,
                          DWORD dwDesiredAccess,
                          DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                          DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes,
                          HANDLE hTemplateFile)
{
    PRUnichar wFileName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wFileName, MAX_PATH);
    return CreateFileW(wFileName, dwDesiredAccess, dwShareMode,
                       lpSecurityAttributes, dwCreationDisposition,
                       dwFlagsAndAttributes, hTemplateFile);
}








static void CopyFindFileDataW2A(LPWIN32_FIND_DATAW from,
                                LPWIN32_FIND_DATAA to)
{
    





    to->dwFileAttributes = from->dwFileAttributes;
    to->ftCreationTime = from->ftCreationTime;
    to->ftLastAccessTime = from->ftLastAccessTime;
    to->ftLastWriteTime = from->ftLastWriteTime;
    to->nFileSizeHigh = from->nFileSizeHigh;
    to->nFileSizeLow = from->nFileSizeLow;
    to->dwReserved0 = 0;
    to->dwReserved1 = 0;
    WideCharToMultiByte(CP_ACP, 0, from->cFileName, -1,
                        to->cFileName, MAX_PATH, NULL, NULL);
    to->cAlternateFileName[0] = '\0';
}

static HANDLE FindFirstFileA(LPCSTR lpFileName,
                             LPWIN32_FIND_DATAA lpFindFileData)
{
    PRUnichar wFileName[MAX_PATH];
    HANDLE hFindFile;
    WIN32_FIND_DATAW wFindFileData;
    
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wFileName, MAX_PATH);
    hFindFile = FindFirstFileW(wFileName, &wFindFileData);
    if (hFindFile != INVALID_HANDLE_VALUE) {
        CopyFindFileDataW2A(&wFindFileData, lpFindFileData);
    }
    return hFindFile;
}

static BOOL FindNextFileA(HANDLE hFindFile,
                          LPWIN32_FIND_DATAA lpFindFileData)
{
    WIN32_FIND_DATAW wFindFileData;
    BOOL rv;

    rv = FindNextFileW(hFindFile, &wFindFileData);
    if (rv) {
        CopyFindFileDataW2A(&wFindFileData, lpFindFileData);
    }
    return rv;
}

static BOOL GetFileAttributesExA(LPCSTR lpFileName,
                                 GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                 LPVOID lpFileInformation)
{
    PRUnichar wFileName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wFileName, MAX_PATH);
    return GetFileAttributesExW(wFileName, fInfoLevelId, lpFileInformation);
}

static BOOL DeleteFileA(LPCSTR lpFileName)
{
    PRUnichar wFileName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wFileName, MAX_PATH);
    return DeleteFileW(wFileName);
}

static BOOL MoveFileA(LPCSTR from, LPCSTR to)
{
    PRUnichar wFrom[MAX_PATH];
    PRUnichar wTo[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, from, -1, wFrom, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, to, -1, wTo, MAX_PATH);
    return MoveFileW(wFrom, wTo);
}

static BOOL CreateDirectoryA(LPCSTR lpPathName,
                             LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PRUnichar wPathName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpPathName, -1, wPathName, MAX_PATH);
    return CreateDirectoryW(wPathName, lpSecurityAttributes);
}

static BOOL RemoveDirectoryA(LPCSTR lpPathName)
{
    PRUnichar wPathName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpPathName, -1, wPathName, MAX_PATH);
    return RemoveDirectoryW(wPathName);
}

static long GetDriveType(const char *lpRootPathName)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return 0; 
}

static DWORD GetFullPathName(const char *lpFileName,
                             DWORD nBufferLength,
                             const char *lpBuffer,
                             const char **lpFilePart)
{
    
    DWORD len = strlen(lpFileName);
    if (len > nBufferLength)
        return len;
  
    strncpy((char *)lpBuffer, lpFileName, len);
    ((char *)lpBuffer)[len] = '\0';
  
    if (lpFilePart) {
        char *sep = strrchr(lpBuffer, '\\');
        if (sep) {
            sep++; 
            *lpFilePart = sep;
        } else {
            *lpFilePart = lpBuffer;
        }
    }
    return len;
}

static BOOL LockFile(HANDLE hFile,
                     DWORD dwFileOffsetLow,
                     DWORD dwFileOffsetHigh,
                     DWORD nNumberOfBytesToLockLow,
                     DWORD nNumberOfBytesToLockHigh)
{
    OVERLAPPED overlapped = {0};
    overlapped.Offset = dwFileOffsetLow;
    overlapped.OffsetHigh = dwFileOffsetHigh;
    return LockFileEx(hFile,
                      LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                      0, 
                      nNumberOfBytesToLockLow,
                      nNumberOfBytesToLockHigh, &overlapped);
}

static BOOL UnlockFile(HANDLE hFile,
                       DWORD dwFileOffsetLow,
                       DWORD dwFileOffsetHigh,
                       DWORD nNumberOfBytesToUnlockLow,
                       DWORD nNumberOfBytesToUnlockHigh)
{
    OVERLAPPED overlapped = {0};
    overlapped.Offset = dwFileOffsetLow;
    overlapped.OffsetHigh = dwFileOffsetHigh;
    return UnlockFileEx(hFile,
                        0, 
                        nNumberOfBytesToUnlockLow,
                        nNumberOfBytesToUnlockHigh, &overlapped);
}

static unsigned char *_mbsdec(const unsigned char *string1,
                              const unsigned char *string2)
{
    
    return NULL;
}

static unsigned char *_mbsinc(const unsigned char *inCurrent)
{
    
    return (unsigned char *)(inCurrent + 1);
}

#endif

struct _MDLock               _pr_ioq_lock;




static DWORD fileAccessTable[] = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE
};




static DWORD dirAccessTable[] = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE|FILE_DELETE_CHILD,
    FILE_GENERIC_EXECUTE
};


#ifndef WINCE
typedef BOOL (WINAPI *GetFileAttributesExFn)(LPCTSTR,
                                             GET_FILEEX_INFO_LEVELS,
                                             LPVOID); 
static GetFileAttributesExFn getFileAttributesEx;
static void InitGetFileInfo(void);
#endif

static void InitUnicodeSupport(void);

static PRBool IsPrevCharSlash(const char *str, const char *current);

void
_PR_MD_INIT_IO()
{
    WORD WSAVersion = 0x0101;
    WSADATA WSAData;
    int err;

    err = WSAStartup( WSAVersion, &WSAData );
    PR_ASSERT(0 == err);

#ifdef DEBUG
    
    {
        SYSTEMTIME systime;
        union {
           PRTime prt;
           FILETIME ft;
        } filetime;
        BOOL rv;

        systime.wYear = 1970;
        systime.wMonth = 1;
        
        systime.wDay = 1;
        systime.wHour = 0;
        systime.wMinute = 0;
        systime.wSecond = 0;
        systime.wMilliseconds = 0;

        rv = SystemTimeToFileTime(&systime, &filetime.ft);
        PR_ASSERT(0 != rv);
        PR_ASSERT(filetime.prt == _pr_filetime_offset);
    }
#endif 

    _PR_NT_InitSids();

#ifndef WINCE
    InitGetFileInfo();
#endif

    InitUnicodeSupport();

    _PR_MD_InitSockets();
}

PRStatus
_PR_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    DWORD rv;

    PRUint32 msecs = (ticks == PR_INTERVAL_NO_TIMEOUT) ?
        INFINITE : PR_IntervalToMilliseconds(ticks);
    rv = WaitForSingleObject(thread->md.blocked_sema, msecs);
    switch(rv) 
    {
        case WAIT_OBJECT_0:
            return PR_SUCCESS;
            break;
        case WAIT_TIMEOUT:
            _PR_THREAD_LOCK(thread);
            if (thread->state == _PR_IO_WAIT) {
			  ;
            } else {
                if (thread->wait.cvar != NULL) {
                    thread->wait.cvar = NULL;
                    _PR_THREAD_UNLOCK(thread);
                } else {
                    



                    _PR_THREAD_UNLOCK(thread);
                    rv = WaitForSingleObject(thread->md.blocked_sema, 0);
                    PR_ASSERT(rv == WAIT_OBJECT_0);
                }
            }
            return PR_SUCCESS;
            break;
        default:
            return PR_FAILURE;
            break;
    }
}
PRStatus
_PR_MD_WAKEUP_WAITER(PRThread *thread)
{
    if ( _PR_IS_NATIVE_THREAD(thread) ) 
    {
        if (ReleaseSemaphore(thread->md.blocked_sema, 1, NULL) == FALSE)
            return PR_FAILURE;
        else
			return PR_SUCCESS;
	}
}














PROsfd
_PR_MD_OPEN(const char *name, PRIntn osflags, int mode)
{
    HANDLE file;
    PRInt32 access = 0;
    PRInt32 flags = 0;
    PRInt32 flag6 = 0;
    
    if (osflags & PR_SYNC) flag6 = FILE_FLAG_WRITE_THROUGH;
 
    if (osflags & PR_RDONLY || osflags & PR_RDWR)
        access |= GENERIC_READ;
    if (osflags & PR_WRONLY || osflags & PR_RDWR)
        access |= GENERIC_WRITE;

    if ( osflags & PR_CREATE_FILE && osflags & PR_EXCL )
        flags = CREATE_NEW;
    else if (osflags & PR_CREATE_FILE) {
        if (osflags & PR_TRUNCATE)
            flags = CREATE_ALWAYS;
        else
            flags = OPEN_ALWAYS;
    } else {
        if (osflags & PR_TRUNCATE)
            flags = TRUNCATE_EXISTING;
        else
            flags = OPEN_EXISTING;
    }

    file = CreateFileA(name,
                       access,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,
                       flags,
                       flag6,
                       NULL);
    if (file == INVALID_HANDLE_VALUE) {
		_PR_MD_MAP_OPEN_ERROR(GetLastError());
        return -1; 
	}

    return (PROsfd)file;
}

PROsfd
_PR_MD_OPEN_FILE(const char *name, PRIntn osflags, int mode)
{
    HANDLE file;
    PRInt32 access = 0;
    PRInt32 flags = 0;
    PRInt32 flag6 = 0;
    SECURITY_ATTRIBUTES sa;
    LPSECURITY_ATTRIBUTES lpSA = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pACL = NULL;

    if (osflags & PR_CREATE_FILE) {
        if (_PR_NT_MakeSecurityDescriptorACL(mode, fileAccessTable,
                &pSD, &pACL) == PR_SUCCESS) {
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
            lpSA = &sa;
        }
    }
    
    if (osflags & PR_SYNC) flag6 = FILE_FLAG_WRITE_THROUGH;
 
    if (osflags & PR_RDONLY || osflags & PR_RDWR)
        access |= GENERIC_READ;
    if (osflags & PR_WRONLY || osflags & PR_RDWR)
        access |= GENERIC_WRITE;

    if ( osflags & PR_CREATE_FILE && osflags & PR_EXCL )
        flags = CREATE_NEW;
    else if (osflags & PR_CREATE_FILE) {
        if (osflags & PR_TRUNCATE)
            flags = CREATE_ALWAYS;
        else
            flags = OPEN_ALWAYS;
    } else {
        if (osflags & PR_TRUNCATE)
            flags = TRUNCATE_EXISTING;
        else
            flags = OPEN_EXISTING;
    }

    file = CreateFileA(name,
                       access,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       lpSA,
                       flags,
                       flag6,
                       NULL);
    if (lpSA != NULL) {
        _PR_NT_FreeSecurityDescriptorACL(pSD, pACL);
    }
    if (file == INVALID_HANDLE_VALUE) {
		_PR_MD_MAP_OPEN_ERROR(GetLastError());
        return -1; 
	}

    return (PROsfd)file;
}

PRInt32
_PR_MD_READ(PRFileDesc *fd, void *buf, PRInt32 len)
{
    PRUint32 bytes;
    int rv, err;

    rv = ReadFile((HANDLE)fd->secret->md.osfd,
            (LPVOID)buf,
            len,
            &bytes,
            NULL);
    
    if (rv == 0) 
    {
        err = GetLastError();
        
        PR_ASSERT(err != ERROR_HANDLE_EOF);
        if (err == ERROR_BROKEN_PIPE)
            return 0;
		else {
			_PR_MD_MAP_READ_ERROR(err);
        return -1;
    }
    }
    return bytes;
}

PRInt32
_PR_MD_WRITE(PRFileDesc *fd, const void *buf, PRInt32 len)
{
    PROsfd f = fd->secret->md.osfd;
    PRInt32 bytes;
    int rv;
    
    rv = WriteFile((HANDLE)f,
            buf,
            len,
            &bytes,
            NULL );
            
    if (rv == 0) 
    {
		_PR_MD_MAP_WRITE_ERROR(GetLastError());
        return -1;
    }
    return bytes;
} 

PROffset32
_PR_MD_LSEEK(PRFileDesc *fd, PROffset32 offset, PRSeekWhence whence)
{
    DWORD moveMethod;
    PROffset32 rv;

    switch (whence) {
        case PR_SEEK_SET:
            moveMethod = FILE_BEGIN;
            break;
        case PR_SEEK_CUR:
            moveMethod = FILE_CURRENT;
            break;
        case PR_SEEK_END:
            moveMethod = FILE_END;
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            return -1;
    }

    rv = SetFilePointer((HANDLE)fd->secret->md.osfd, offset, NULL, moveMethod);

    



    if (-1 == rv) {
        _PR_MD_MAP_LSEEK_ERROR(GetLastError());
    }
    return rv;
}

PROffset64
_PR_MD_LSEEK64(PRFileDesc *fd, PROffset64 offset, PRSeekWhence whence)
{
    DWORD moveMethod;
    LARGE_INTEGER li;
    DWORD err;

    switch (whence) {
        case PR_SEEK_SET:
            moveMethod = FILE_BEGIN;
            break;
        case PR_SEEK_CUR:
            moveMethod = FILE_CURRENT;
            break;
        case PR_SEEK_END:
            moveMethod = FILE_END;
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            return -1;
    }

    li.QuadPart = offset;
    li.LowPart = SetFilePointer((HANDLE)fd->secret->md.osfd,
            li.LowPart, &li.HighPart, moveMethod);

    if (0xffffffff == li.LowPart && (err = GetLastError()) != NO_ERROR) {
        _PR_MD_MAP_LSEEK_ERROR(err);
        li.QuadPart = -1;
    }
    return li.QuadPart;
}







PRInt32
_PR_MD_FSYNC(PRFileDesc *fd)
{
    
















    BOOL ok = FlushFileBuffers((HANDLE)fd->secret->md.osfd);

    if (!ok) {
	DWORD err = GetLastError();
	if (err != ERROR_ACCESS_DENIED) {	
			_PR_MD_MAP_FSYNC_ERROR(err);
	    return -1;
	}
    }
    return 0;
}

PRInt32
_MD_CloseFile(PROsfd osfd)
{
    PRInt32 rv;
    
    rv = (CloseHandle((HANDLE)osfd))?0:-1;
	if (rv == -1)
		_PR_MD_MAP_CLOSE_ERROR(GetLastError());
    return rv;
}



#define GetFileFromDIR(d)       (d)->d_entry.cFileName
#define FileIsHidden(d)	((d)->d_entry.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)

static void FlipSlashes(char *cp, size_t len)
{
    while (len-- > 0) {
        if (cp[0] == '/') {
            cp[0] = PR_DIRECTORY_SEPARATOR;
        }
        cp = _mbsinc(cp);
    }
} 









PRStatus
_PR_MD_CLOSE_DIR(_MDDir *d)
{
    if ( d ) {
        if (FindClose(d->d_hdl)) {
        d->magic = (PRUint32)-1;
        return PR_SUCCESS;
		} else {
			_PR_MD_MAP_CLOSEDIR_ERROR(GetLastError());
        	return PR_FAILURE;
		}
    }
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return PR_FAILURE;
}


PRStatus
_PR_MD_OPEN_DIR(_MDDir *d, const char *name)
{
    char filename[ MAX_PATH ];
    size_t len;

    len = strlen(name);
    
    if (len + 5 > MAX_PATH) {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
        return PR_FAILURE;
    }
    strcpy(filename, name);

    



    if (IsPrevCharSlash(filename, filename + len)) {
        len--;
    }
    strcpy(&filename[len], "\\*.*");
    FlipSlashes( filename, strlen(filename) );

    d->d_hdl = FindFirstFileA( filename, &(d->d_entry) );
    if ( d->d_hdl == INVALID_HANDLE_VALUE ) {
		_PR_MD_MAP_OPENDIR_ERROR(GetLastError());
        return PR_FAILURE;
    }
    d->firstEntry = PR_TRUE;
    d->magic = _MD_MAGIC_DIR;
    return PR_SUCCESS;
}

char *
_PR_MD_READ_DIR(_MDDir *d, PRIntn flags)
{
    PRInt32 err;
    BOOL rv;
    char *fileName;

    if ( d ) {
        while (1) {
            if (d->firstEntry) {
                d->firstEntry = PR_FALSE;
                rv = 1;
            } else {
                rv = FindNextFileA(d->d_hdl, &(d->d_entry));
            }
            if (rv == 0) {
                break;
            }
            fileName = GetFileFromDIR(d);
            if ( (flags & PR_SKIP_DOT) &&
                 (fileName[0] == '.') && (fileName[1] == '\0'))
                 continue;
            if ( (flags & PR_SKIP_DOT_DOT) &&
                 (fileName[0] == '.') && (fileName[1] == '.') &&
                 (fileName[2] == '\0'))
                 continue;
            if ( (flags & PR_SKIP_HIDDEN) && FileIsHidden(d))
                 continue;
            return fileName;
        }
        err = GetLastError();
        PR_ASSERT(NO_ERROR != err);
			_PR_MD_MAP_READDIR_ERROR(err);
        return NULL;
		}
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return NULL;
}

PRInt32
_PR_MD_DELETE(const char *name)
{
    if (DeleteFileA(name)) {
        return 0;
    } else {
		_PR_MD_MAP_DELETE_ERROR(GetLastError());
        return -1;
    }
}

void
_PR_FileTimeToPRTime(const FILETIME *filetime, PRTime *prtm)
{
    PR_ASSERT(sizeof(FILETIME) == sizeof(PRTime));
    CopyMemory(prtm, filetime, sizeof(PRTime));
#if defined(__MINGW32__)
    *prtm = (*prtm - _pr_filetime_offset) / 10LL;
#else
    *prtm = (*prtm - _pr_filetime_offset) / 10i64;
#endif

#ifdef DEBUG
    
    {
        SYSTEMTIME systime;
        PRExplodedTime etm;
        PRTime cmp; 
        BOOL rv;

        rv = FileTimeToSystemTime(filetime, &systime);
        PR_ASSERT(0 != rv);

        


        etm.tm_usec = systime.wMilliseconds * PR_USEC_PER_MSEC;
        etm.tm_sec = systime.wSecond;
        etm.tm_min = systime.wMinute;
        etm.tm_hour = systime.wHour;
        etm.tm_mday = systime.wDay;
        etm.tm_month = systime.wMonth - 1;
        etm.tm_year = systime.wYear;
        





        etm.tm_params.tp_gmt_offset = 0;
        etm.tm_params.tp_dst_offset = 0;
        cmp = PR_ImplodeTime(&etm);

        



        PR_ASSERT((cmp / PR_USEC_PER_MSEC) == (*prtm / PR_USEC_PER_MSEC));
    }
#endif 
}

PRInt32
_PR_MD_STAT(const char *fn, struct stat *info)
{
#ifdef WINCE
    
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return -1;
#else
    PRInt32 rv;

    rv = _stat(fn, (struct _stat *)info);
    if (-1 == rv) {
        











        size_t len = strlen(fn);
        if (len > 0 && len <= _MAX_PATH
                && IsPrevCharSlash(fn, fn + len)) {
            char newfn[_MAX_PATH + 1];

            strcpy(newfn, fn);
            newfn[len - 1] = '\0';
            rv = _stat(newfn, (struct _stat *)info);
        }
    }

    if (-1 == rv) {
        _PR_MD_MAP_STAT_ERROR(errno);
    }
    return rv;
#endif
}

#define _PR_IS_SLASH(ch) ((ch) == '/' || (ch) == '\\')

static PRBool
IsPrevCharSlash(const char *str, const char *current)
{
    const char *prev;

    if (str >= current)
        return PR_FALSE;
    prev = _mbsdec(str, current);
    return (prev == current - 1) && _PR_IS_SLASH(*prev);
}



















static PRBool
IsRootDirectory(char *fn, size_t buflen)
{
    char *p;
    PRBool slashAdded = PR_FALSE;
    PRBool rv = PR_FALSE;

    if (_PR_IS_SLASH(fn[0]) && fn[1] == '\0') {
        return PR_TRUE;
    }

    if (isalpha(fn[0]) && fn[1] == ':' && _PR_IS_SLASH(fn[2])
            && fn[3] == '\0') {
        rv = GetDriveType(fn) > 1 ? PR_TRUE : PR_FALSE;
        return rv;
    }

    

    if (_PR_IS_SLASH(fn[0]) && _PR_IS_SLASH(fn[1])) {
        
        p = &fn[2];
        if (*p == '\0' || _PR_IS_SLASH(*p)) {
            return PR_FALSE;
        }

        
        do {
            p = _mbsinc(p);
        } while (*p != '\0' && !_PR_IS_SLASH(*p));
        if (*p == '\0') {
            return PR_FALSE;
        }

        
        p++;
        if (*p == '\0' || _PR_IS_SLASH(*p)) {
            return PR_FALSE;
        }

        
        do {
            p = _mbsinc(p);
        } while (*p != '\0' && !_PR_IS_SLASH(*p));
        if (_PR_IS_SLASH(*p) && p[1] != '\0') {
            return PR_FALSE;
        }
        if (*p == '\0') {
            




            if ((p + 1) < (fn + buflen)) {
                *p++ = '\\';
                *p = '\0';
                slashAdded = PR_TRUE;
            } else {
                return PR_FALSE; 
            }
        }
        rv = GetDriveType(fn) > 1 ? PR_TRUE : PR_FALSE;
        
        if (slashAdded) {
            *--p = '\0';
        }
    }
    return rv;
}

#ifndef WINCE








static void InitGetFileInfo(void)
{
    HMODULE module;
    module = GetModuleHandle("Kernel32.dll");
    if (!module) {
        PR_LOG(_pr_io_lm, PR_LOG_DEBUG,
                ("InitGetFileInfo: GetModuleHandle() failed: %d",
                GetLastError()));
        return;
    }

    getFileAttributesEx = (GetFileAttributesExFn)
            GetProcAddress(module, "GetFileAttributesExA");
}





static BOOL
GetFileAttributesExFB(const char *fn, WIN32_FIND_DATA *findFileData)
{
    HANDLE hFindFile;

    



    if (NULL != _mbspbrk(fn, "?*")) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    hFindFile = FindFirstFile(fn, findFileData);
    if (INVALID_HANDLE_VALUE == hFindFile) {
        DWORD len;
        char *filePart;
        char pathbuf[MAX_PATH + 1];

        







        



        if (NULL == _mbspbrk(fn, ".\\/")) {
            return FALSE;
        } 
        len = GetFullPathName(fn, sizeof(pathbuf), pathbuf,
                &filePart);
        if (0 == len) {
            return FALSE;
        }
        if (len > sizeof(pathbuf)) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
        if (IsRootDirectory(pathbuf, sizeof(pathbuf))) {
            findFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            
            findFileData->nFileSizeHigh = 0;
            findFileData->nFileSizeLow = 0;
            




            memcpy(&findFileData->ftCreationTime, &_pr_filetime_offset, 8);
            findFileData->ftLastAccessTime = findFileData->ftCreationTime;
            findFileData->ftLastWriteTime = findFileData->ftCreationTime;
            return TRUE;
        }
        if (!IsPrevCharSlash(pathbuf, pathbuf + len)) {
            return FALSE;
        } else {
            pathbuf[len - 1] = '\0';
            hFindFile = FindFirstFile(pathbuf, findFileData);
            if (INVALID_HANDLE_VALUE == hFindFile) {
                return FALSE;
            }
        }
    }

    FindClose(hFindFile);
    return TRUE;
}
#endif

PRInt32
_PR_MD_GETFILEINFO64(const char *fn, PRFileInfo64 *info)
{
#ifdef WINCE
    WIN32_FILE_ATTRIBUTE_DATA findFileData;
#else
    WIN32_FIND_DATA findFileData;
#endif
    BOOL rv;
    
    if (NULL == fn || '\0' == *fn) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return -1;
    }

#ifdef WINCE
    rv = GetFileAttributesExA(fn, GetFileExInfoStandard, &findFileData);
#else
    
    if (getFileAttributesEx) {
        rv = getFileAttributesEx(fn, GetFileExInfoStandard, &findFileData);
    } else {
        rv = GetFileAttributesExFB(fn, &findFileData);
    }
#endif
    if (!rv) {
        _PR_MD_MAP_OPENDIR_ERROR(GetLastError());
        return -1;
    }

    if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        info->type = PR_FILE_DIRECTORY;
    } else {
        info->type = PR_FILE_FILE;
    }

    info->size = findFileData.nFileSizeHigh;
    info->size = (info->size << 32) + findFileData.nFileSizeLow;

    _PR_FileTimeToPRTime(&findFileData.ftLastWriteTime, &info->modifyTime);

    if (0 == findFileData.ftCreationTime.dwLowDateTime &&
            0 == findFileData.ftCreationTime.dwHighDateTime) {
        info->creationTime = info->modifyTime;
    } else {
        _PR_FileTimeToPRTime(&findFileData.ftCreationTime,
                &info->creationTime);
    }

    return 0;
}

PRInt32
_PR_MD_GETFILEINFO(const char *fn, PRFileInfo *info)
{
    PRFileInfo64 info64;
    PRInt32 rv = _PR_MD_GETFILEINFO64(fn, &info64);
    if (0 == rv)
    {
        info->type = info64.type;
        info->size = (PRUint32) info64.size;
        info->modifyTime = info64.modifyTime;
        info->creationTime = info64.creationTime;
    }
    return rv;
}

PRInt32
_PR_MD_GETOPENFILEINFO64(const PRFileDesc *fd, PRFileInfo64 *info)
{
    int rv;

    BY_HANDLE_FILE_INFORMATION hinfo;

    rv = GetFileInformationByHandle((HANDLE)fd->secret->md.osfd, &hinfo);
    if (rv == FALSE) {
		_PR_MD_MAP_FSTAT_ERROR(GetLastError());
        return -1;
	}

    if (hinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        info->type = PR_FILE_DIRECTORY;
    else
        info->type = PR_FILE_FILE;

    info->size = hinfo.nFileSizeHigh;
    info->size = (info->size << 32) + hinfo.nFileSizeLow;

    _PR_FileTimeToPRTime(&hinfo.ftLastWriteTime, &(info->modifyTime) );
    _PR_FileTimeToPRTime(&hinfo.ftCreationTime, &(info->creationTime) );

    return 0;
}

PRInt32
_PR_MD_GETOPENFILEINFO(const PRFileDesc *fd, PRFileInfo *info)
{
    PRFileInfo64 info64;
    int rv = _PR_MD_GETOPENFILEINFO64(fd, &info64);
    if (0 == rv)
    {
        info->type = info64.type;
        info->modifyTime = info64.modifyTime;
        info->creationTime = info64.creationTime;
        LL_L2I(info->size, info64.size);
    }
    return rv;
}

PRStatus
_PR_MD_SET_FD_INHERITABLE(PRFileDesc *fd, PRBool inheritable)
{
#ifdef WINCE
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
#else
    BOOL rv;

    



    rv = SetHandleInformation(
            (HANDLE)fd->secret->md.osfd,
            HANDLE_FLAG_INHERIT,
            inheritable ? HANDLE_FLAG_INHERIT : 0);
    if (0 == rv) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        return PR_FAILURE;
    }
    return PR_SUCCESS;
#endif
} 

void
_PR_MD_INIT_FD_INHERITABLE(PRFileDesc *fd, PRBool imported)
{
    if (imported) {
        fd->secret->inheritable = _PR_TRI_UNKNOWN;
    } else {
        fd->secret->inheritable = _PR_TRI_FALSE;
    }
}

void
_PR_MD_QUERY_FD_INHERITABLE(PRFileDesc *fd)
{
#ifdef WINCE
    fd->secret->inheritable = _PR_TRI_FALSE;
#else
    DWORD flags;

    PR_ASSERT(_PR_TRI_UNKNOWN == fd->secret->inheritable);
    if (GetHandleInformation((HANDLE)fd->secret->md.osfd, &flags)) {
        if (flags & HANDLE_FLAG_INHERIT) {
            fd->secret->inheritable = _PR_TRI_TRUE;
        } else {
            fd->secret->inheritable = _PR_TRI_FALSE;
        }
    }
#endif
}

PRInt32
_PR_MD_RENAME(const char *from, const char *to)
{
    
    if (MoveFileA(from, to)) {
        return 0;
    } else {
		_PR_MD_MAP_RENAME_ERROR(GetLastError());
        return -1;
    }
}

PRInt32
_PR_MD_ACCESS(const char *name, PRAccessHow how)
{
#ifdef WINCE
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return -1;
#else
PRInt32 rv;
    switch (how) {
      case PR_ACCESS_WRITE_OK:
        rv = _access(name, 02);
		break;
      case PR_ACCESS_READ_OK:
        rv = _access(name, 04);
		break;
      case PR_ACCESS_EXISTS:
        return _access(name, 00);
	  	break;
      default:
		PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
		return -1;
    }
	if (rv < 0)
		_PR_MD_MAP_ACCESS_ERROR(errno);
    return rv;
#endif
}

PRInt32
_PR_MD_MKDIR(const char *name, PRIntn mode)
{
    
    if (CreateDirectoryA(name, NULL)) {
        return 0;
    } else {
		_PR_MD_MAP_MKDIR_ERROR(GetLastError());
        return -1;
    }
}

PRInt32
_PR_MD_MAKE_DIR(const char *name, PRIntn mode)
{
    BOOL rv;
    SECURITY_ATTRIBUTES sa;
    LPSECURITY_ATTRIBUTES lpSA = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pACL = NULL;

    if (_PR_NT_MakeSecurityDescriptorACL(mode, dirAccessTable,
            &pSD, &pACL) == PR_SUCCESS) {
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = pSD;
        sa.bInheritHandle = FALSE;
        lpSA = &sa;
    }
    rv = CreateDirectoryA(name, lpSA);
    if (lpSA != NULL) {
        _PR_NT_FreeSecurityDescriptorACL(pSD, pACL);
    }
    if (rv) {
        return 0;
    } else {
        _PR_MD_MAP_MKDIR_ERROR(GetLastError());
        return -1;
    }
}

PRInt32
_PR_MD_RMDIR(const char *name)
{
    if (RemoveDirectoryA(name)) {
        return 0;
    } else {
		_PR_MD_MAP_RMDIR_ERROR(GetLastError());
        return -1;
    }
}

PRStatus
_PR_MD_LOCKFILE(PROsfd f)
{
    PRStatus  rc = PR_SUCCESS;
	DWORD     rv;

	rv = LockFile( (HANDLE)f,
		0l, 0l,
		0x0l, 0xffffffffl ); 
	if ( rv == 0 ) {
        DWORD rc = GetLastError();
        PR_LOG( _pr_io_lm, PR_LOG_ERROR,
            ("_PR_MD_LOCKFILE() failed. Error: %d", rc ));
        rc = PR_FAILURE;
    }

    return rc;
} 

PRStatus
_PR_MD_TLOCKFILE(PROsfd f)
{
    PR_SetError( PR_NOT_IMPLEMENTED_ERROR, 0 );
    return PR_FAILURE;
} 


PRStatus
_PR_MD_UNLOCKFILE(PROsfd f)
{
	PRInt32   rv;
    
    rv = UnlockFile( (HANDLE) f,
    		0l, 0l,
            0x0l, 0xffffffffl ); 
            
    if ( rv )
    {
    	return PR_SUCCESS;
    }
    else
    {
		_PR_MD_MAP_DEFAULT_ERROR(GetLastError());
		return PR_FAILURE;
    }
} 

PRInt32
_PR_MD_PIPEAVAILABLE(PRFileDesc *fd)
{
    if (NULL == fd)
		PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
	else
		PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return -1;
}

#ifdef MOZ_UNICODE

typedef HANDLE (WINAPI *CreateFileWFn) (LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static CreateFileWFn createFileW = CreateFileW;
typedef HANDLE (WINAPI *FindFirstFileWFn) (LPCWSTR, LPWIN32_FIND_DATAW);
static FindFirstFileWFn findFirstFileW = FindFirstFileW;
typedef BOOL (WINAPI *FindNextFileWFn) (HANDLE, LPWIN32_FIND_DATAW);
static FindNextFileWFn findNextFileW = FindNextFileW;
typedef DWORD (WINAPI *GetFullPathNameWFn) (LPCWSTR, DWORD, LPWSTR, LPWSTR *);
static GetFullPathNameWFn getFullPathNameW = GetFullPathNameW;
typedef UINT (WINAPI *GetDriveTypeWFn) (LPCWSTR);
static GetDriveTypeWFn getDriveTypeW = GetDriveTypeW;

#endif 

PRBool _pr_useUnicode = PR_FALSE;

static void InitUnicodeSupport(void)
{
#ifdef WINCE
    
    _pr_useUnicode = PR_TRUE;
#else
    





    
    OSVERSIONINFOA osvi = {0};

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionExA(&osvi)) {
        _pr_useUnicode = (osvi.dwPlatformId >= VER_PLATFORM_WIN32_NT);
    } else {
        _pr_useUnicode = PR_FALSE;
    }
#ifdef DEBUG
    



    if (getenv("WINAPI_USE_ANSI"))
        _pr_useUnicode = PR_FALSE;
#endif
#endif
}

#ifdef MOZ_UNICODE


static void FlipSlashesW(PRUnichar *cp, size_t len)
{
    while (len-- > 0) {
        if (cp[0] == L'/') {
            cp[0] = L'\\';
        }
        cp++;
    }
} 

PROsfd
_PR_MD_OPEN_FILE_UTF16(const PRUnichar *name, PRIntn osflags, int mode)
{
    HANDLE file;
    PRInt32 access = 0;
    PRInt32 flags = 0;
    PRInt32 flag6 = 0;
    SECURITY_ATTRIBUTES sa;
    LPSECURITY_ATTRIBUTES lpSA = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pACL = NULL;

    if (osflags & PR_CREATE_FILE) {
        if (_PR_NT_MakeSecurityDescriptorACL(mode, fileAccessTable,
                &pSD, &pACL) == PR_SUCCESS) {
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
            lpSA = &sa;
        }
    }

    if (osflags & PR_SYNC) flag6 = FILE_FLAG_WRITE_THROUGH;

    if (osflags & PR_RDONLY || osflags & PR_RDWR)
        access |= GENERIC_READ;
    if (osflags & PR_WRONLY || osflags & PR_RDWR)
        access |= GENERIC_WRITE;
 
    if ( osflags & PR_CREATE_FILE && osflags & PR_EXCL )
        flags = CREATE_NEW;
    else if (osflags & PR_CREATE_FILE) {
        if (osflags & PR_TRUNCATE)
            flags = CREATE_ALWAYS;
        else
            flags = OPEN_ALWAYS;
    } else {
        if (osflags & PR_TRUNCATE)
            flags = TRUNCATE_EXISTING;
        else
            flags = OPEN_EXISTING;
    }

    file = createFileW(name,
                       access,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       lpSA,
                       flags,
                       flag6,
                       NULL);
    if (lpSA != NULL) {
        _PR_NT_FreeSecurityDescriptorACL(pSD, pACL);
    }
    if (file == INVALID_HANDLE_VALUE) {
        _PR_MD_MAP_OPEN_ERROR(GetLastError());
        return -1;
    }
 
    return (PROsfd)file;
}
 
PRStatus
_PR_MD_OPEN_DIR_UTF16(_MDDirUTF16 *d, const PRUnichar *name)
{
    PRUnichar filename[ MAX_PATH ];
    int len;

    len = wcslen(name);
    
    if (len + 5 > MAX_PATH) {
        PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
        return PR_FAILURE;
    }
    wcscpy(filename, name);

    



    if (filename[len - 1] == L'/' || filename[len - 1] == L'\\') {
        len--;
    }
    wcscpy(&filename[len], L"\\*.*");
    FlipSlashesW( filename, wcslen(filename) );

    d->d_hdl = findFirstFileW( filename, &(d->d_entry) );
    if ( d->d_hdl == INVALID_HANDLE_VALUE ) {
        _PR_MD_MAP_OPENDIR_ERROR(GetLastError());
        return PR_FAILURE;
    }
    d->firstEntry = PR_TRUE;
    d->magic = _MD_MAGIC_DIR;
    return PR_SUCCESS;
}

PRUnichar *
_PR_MD_READ_DIR_UTF16(_MDDirUTF16 *d, PRIntn flags)
{
    PRInt32 err;
    BOOL rv;
    PRUnichar *fileName;

    if ( d ) {
        while (1) {
            if (d->firstEntry) {
                d->firstEntry = PR_FALSE;
                rv = 1;
            } else {
                rv = findNextFileW(d->d_hdl, &(d->d_entry));
            }
            if (rv == 0) {
                break;
            }
            fileName = GetFileFromDIR(d);
            if ( (flags & PR_SKIP_DOT) &&
                 (fileName[0] == L'.') && (fileName[1] == L'\0'))
                continue;
            if ( (flags & PR_SKIP_DOT_DOT) &&
                 (fileName[0] == L'.') && (fileName[1] == L'.') &&
                 (fileName[2] == L'\0'))
                continue;
            if ( (flags & PR_SKIP_HIDDEN) && FileIsHidden(d))
                continue;
            return fileName;
        }
        err = GetLastError();
        PR_ASSERT(NO_ERROR != err);
        _PR_MD_MAP_READDIR_ERROR(err);
        return NULL;
    }
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return NULL;
}
 
PRStatus
_PR_MD_CLOSE_DIR_UTF16(_MDDirUTF16 *d)
{
    if ( d ) {
        if (FindClose(d->d_hdl)) {
            d->magic = (PRUint32)-1;
            return PR_SUCCESS;
        } else {
            _PR_MD_MAP_CLOSEDIR_ERROR(GetLastError());
            return PR_FAILURE;
        }
    }
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return PR_FAILURE;
}

#define _PR_IS_W_SLASH(ch) ((ch) == L'/' || (ch) == L'\\')



















static PRBool
IsRootDirectoryW(PRUnichar *fn, size_t buflen)
{
    PRUnichar *p;
    PRBool slashAdded = PR_FALSE;
    PRBool rv = PR_FALSE;

    if (_PR_IS_W_SLASH(fn[0]) && fn[1] == L'\0') {
        return PR_TRUE;
    }

    if (iswalpha(fn[0]) && fn[1] == L':' && _PR_IS_W_SLASH(fn[2])
            && fn[3] == L'\0') {
        rv = getDriveTypeW(fn) > 1 ? PR_TRUE : PR_FALSE;
        return rv;
    }

    

    if (_PR_IS_W_SLASH(fn[0]) && _PR_IS_W_SLASH(fn[1])) {
        
        p = &fn[2];
        if (*p == L'\0' || _PR_IS_W_SLASH(*p)) {
            return PR_FALSE;
        }

        
        do {
            p++;
        } while (*p != L'\0' && !_PR_IS_W_SLASH(*p));
        if (*p == L'\0') {
            return PR_FALSE;
        }

        
        p++;
        if (*p == L'\0' || _PR_IS_W_SLASH(*p)) {
            return PR_FALSE;
        }

        
        do {
            p++;
        } while (*p != L'\0' && !_PR_IS_W_SLASH(*p));
        if (_PR_IS_W_SLASH(*p) && p[1] != L'\0') {
            return PR_FALSE;
        }
        if (*p == L'\0') {
            




            if ((p + 1) < (fn + buflen)) {
                *p++ = L'\\';
                *p = L'\0';
                slashAdded = PR_TRUE;
            } else {
                return PR_FALSE; 
            }
        }
        rv = getDriveTypeW(fn) > 1 ? PR_TRUE : PR_FALSE;
        
        if (slashAdded) {
            *--p = L'\0';
        }
    }
    return rv;
}

PRInt32
_PR_MD_GETFILEINFO64_UTF16(const PRUnichar *fn, PRFileInfo64 *info)
{
    HANDLE hFindFile;
    WIN32_FIND_DATAW findFileData;
    PRUnichar pathbuf[MAX_PATH + 1];

    if (NULL == fn || L'\0' == *fn) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return -1;
    }

    



    if (NULL != wcspbrk(fn, L"?*")) {
        PR_SetError(PR_FILE_NOT_FOUND_ERROR, 0);
        return -1;
    }

    hFindFile = findFirstFileW(fn, &findFileData);
    if (INVALID_HANDLE_VALUE == hFindFile) {
        DWORD len;
        PRUnichar *filePart;

        







        



        if (NULL == wcspbrk(fn, L".\\/")) {
            _PR_MD_MAP_OPENDIR_ERROR(GetLastError());
            return -1;
        } 
        len = getFullPathNameW(fn, sizeof(pathbuf)/sizeof(pathbuf[0]), pathbuf,
                &filePart);
        if (0 == len) {
            _PR_MD_MAP_OPENDIR_ERROR(GetLastError());
            return -1;
        }
        if (len > sizeof(pathbuf)/sizeof(pathbuf[0])) {
            PR_SetError(PR_NAME_TOO_LONG_ERROR, 0);
            return -1;
        }
        if (IsRootDirectoryW(pathbuf, sizeof(pathbuf)/sizeof(pathbuf[0]))) {
            info->type = PR_FILE_DIRECTORY;
            info->size = 0;
            


            info->modifyTime = 0;
            info->creationTime = 0;
            return 0;
        }
        if (!_PR_IS_W_SLASH(pathbuf[len - 1])) {
            _PR_MD_MAP_OPENDIR_ERROR(GetLastError());
            return -1;
        } else {
            pathbuf[len - 1] = L'\0';
            hFindFile = findFirstFileW(pathbuf, &findFileData);
            if (INVALID_HANDLE_VALUE == hFindFile) {
                _PR_MD_MAP_OPENDIR_ERROR(GetLastError());
                return -1;
            }
        }
    }

    FindClose(hFindFile);

    if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        info->type = PR_FILE_DIRECTORY;
    } else {
        info->type = PR_FILE_FILE;
    }

    info->size = findFileData.nFileSizeHigh;
    info->size = (info->size << 32) + findFileData.nFileSizeLow;

    _PR_FileTimeToPRTime(&findFileData.ftLastWriteTime, &info->modifyTime);

    if (0 == findFileData.ftCreationTime.dwLowDateTime &&
            0 == findFileData.ftCreationTime.dwHighDateTime) {
        info->creationTime = info->modifyTime;
    } else {
        _PR_FileTimeToPRTime(&findFileData.ftCreationTime,
                &info->creationTime);
    }

    return 0;
}

#endif 
