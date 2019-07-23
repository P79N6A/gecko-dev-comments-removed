







#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#pragma hdrstop










typedef BOOL (*sh_FileFcn)(
        wchar_t *pathName,
        WIN32_FIND_DATA *fileData,
        void *arg);

static int shellCp (wchar_t **pArgv); 
static int shellNsinstall (wchar_t **pArgv);
static int shellMkdir (wchar_t **pArgv); 
static BOOL sh_EnumerateFiles(const wchar_t *pattern, const wchar_t *where,
        sh_FileFcn fileFcn, void *arg, int *nFiles);
static const char *sh_GetLastErrorMessage(void);
static BOOL sh_DoCopy(wchar_t *srcFileName, DWORD srcFileAttributes,
        wchar_t *dstFileName, DWORD dstFileAttributes,
        int force, int recursive);

#define LONGPATH_PREFIX L"\\\\?\\"
#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))
#define STR_LEN(a) (ARRAY_LEN(a) - 1)


void changeForwardSlashesToBackSlashes ( wchar_t *arg )
{
    if ( arg == NULL )
        return;

    while ( *arg ) {
        if ( *arg == '/' )
            *arg = '\\';
        arg++;
    }
}

int wmain(int argc, wchar_t *argv[ ])
{
    return shellNsinstall ( argv + 1 );
}

static int
shellNsinstall (wchar_t **pArgv)
{
    int retVal = 0;     
    int dirOnly = 0;    
    wchar_t **pSrc;
    wchar_t **pDst;

    





    while ( *pArgv && **pArgv == '-' ) {
        wchar_t c = (*pArgv)[1];  

        if ( c == 'D' ) {
            dirOnly = 1;
        } else if ( c == 'm' ) {
            pArgv++;  
        }
        pArgv++;
    }

    if ( !dirOnly ) {
        
        if ( *pArgv ) {
            pSrc = pArgv++;
        } else {
            fprintf( stderr, "nsinstall: not enough arguments\n");
            return 3;
        }
    }

    
    if ( *pArgv ) {
        pDst = pArgv++;
        if ( dirOnly && *pArgv ) {
            fprintf( stderr, "nsinstall: too many arguments with -D\n");
            return 3;
        }
    } else {
        fprintf( stderr, "nsinstall: not enough arguments\n");
        return 3;
    }
    while ( *pArgv ) 
        pDst = pArgv++;

    retVal = shellMkdir ( pDst );
    if ( retVal )
        return retVal;
    if ( !dirOnly )
        retVal = shellCp ( pSrc );
    return retVal;
}

static int
shellMkdir (wchar_t **pArgv) 
{
    int retVal = 0; 
    wchar_t *arg;
    wchar_t *pArg;
    wchar_t path[_MAX_PATH];
    wchar_t tmpPath[_MAX_PATH];
    wchar_t *pTmpPath = tmpPath;

    
    while ( *pArgv && **pArgv == '-' ) {
        if ( (*pArgv)[1] == 'm' ) {
            pArgv++;  
        }
        pArgv++;
    }

    while ( *pArgv ) {
        arg = *pArgv;
        changeForwardSlashesToBackSlashes ( arg );
        pArg = arg;
        pTmpPath = tmpPath;
        while ( 1 ) {
            
            while ( *pArg ) {
                *pTmpPath++ = *pArg++;
                if ( *pArg == '\\' )
                    break;
            }
            *pTmpPath = '\0';

            
            _wgetcwd ( path, _MAX_PATH );
            if ( _wchdir ( tmpPath ) != -1 ) {
                _wchdir ( path );
            } else {
                if ( _wmkdir ( tmpPath ) == -1 ) {
                    printf ( "%ls: ", tmpPath );
                    perror ( "Could not create the directory" );
                    retVal = 3;
                    break;
                }
            }
            if ( *pArg == '\0' )      
                break;
            
        }

        pArgv++;
    }
    return retVal;
}

static const char *
sh_GetLastErrorMessage()
{
    static char buf[128];

    FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  
            buf,
            sizeof(buf),
            NULL
    );
    return buf;
}








struct sh_FileData {
    wchar_t pathName[_MAX_PATH];
    DWORD dwFileAttributes;
};














static BOOL
sh_RecordFileData(wchar_t *pathName, WIN32_FIND_DATA *findData, void *arg)
{
    struct sh_FileData *fData = (struct sh_FileData *) arg;

    wcscpy(fData->pathName, pathName);
    fData->dwFileAttributes = findData->dwFileAttributes;
    return TRUE;
}

static BOOL
sh_DoCopy(wchar_t *srcFileName,
          DWORD srcFileAttributes,
          wchar_t *dstFileName,
          DWORD dstFileAttributes,
          int force,
          int recursive
)
{
    if (dstFileAttributes != 0xFFFFFFFF) {
        if ((dstFileAttributes & FILE_ATTRIBUTE_READONLY) && force) {
            dstFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(dstFileName, dstFileAttributes);
        }
    }

    if (srcFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        fprintf(stderr, "nsinstall: %ls is a directory\n",
                srcFileName);
        return FALSE;
    } else {
        DWORD r;
        wchar_t longSrc[1004] = LONGPATH_PREFIX;
        wchar_t longDst[1004] = LONGPATH_PREFIX;
        r = GetFullPathName(srcFileName, 1000, longSrc + STR_LEN(LONGPATH_PREFIX), NULL);
        if (!r) {
            fprintf(stderr, "nsinstall: couldn't get full path of %ls: %s\n",
                    srcFileName, sh_GetLastErrorMessage());
            return FALSE;
        }
        r = GetFullPathName(dstFileName, 1000, longDst + ARRAY_LEN(LONGPATH_PREFIX) - 1, NULL);
        if (!r) {
            fprintf(stderr, "nsinstall: couldn't get full path of %ls: %s\n",
                    dstFileName, sh_GetLastErrorMessage());
            return FALSE;
        }

        if (!CopyFile(longSrc, longDst, FALSE)) {
            fprintf(stderr, "nsinstall: cannot copy %ls to %ls: %s\n",
                    srcFileName, dstFileName, sh_GetLastErrorMessage());
            return FALSE;
        }
    }
    return TRUE;
}









struct sh_CpCmdArg {
    int force;                

    int recursive;            


    wchar_t *dstFileName;        

    wchar_t *dstFileNameMarker;  


};













static BOOL
sh_CpFileCmd(wchar_t *pathName, WIN32_FIND_DATA *findData, void *cpArg)
{
    BOOL retVal = TRUE;
    struct sh_CpCmdArg *arg = (struct sh_CpCmdArg *) cpArg;

    wcscpy(arg->dstFileNameMarker, findData->cFileName);
    return sh_DoCopy(pathName, findData->dwFileAttributes,
            arg->dstFileName, GetFileAttributes(arg->dstFileName),
            arg->force, arg->recursive);
}

static int
shellCp (wchar_t **pArgv) 
{
    int retVal = 0;
    wchar_t **pSrc;
    wchar_t **pDst;
    struct sh_CpCmdArg arg;
    struct sh_FileData dstData;
    int dstIsDir = 0;
    int n;

    arg.force = 0;
    arg.recursive = 0;
    arg.dstFileName = dstData.pathName;
    arg.dstFileNameMarker = 0;

    while (*pArgv && **pArgv == '-') {
        wchar_t *p = *pArgv;

        while (*(++p)) {
            if (*p == 'f') {
                arg.force = 1;
            }
        }
        pArgv++;
    }

    
    if (*pArgv) {
        pSrc = pArgv++;
    } else {
        fprintf(stderr, "nsinstall: not enough arguments\n");
        return 3;
    }

    
    if (*pArgv) {
        pDst = pArgv++;
    } else {
        fprintf(stderr, "nsinstall: not enough arguments\n");
        return 3;
    }
    while (*pArgv) {
        pDst = pArgv++;
    }

    




    changeForwardSlashesToBackSlashes(*pDst);
    sh_EnumerateFiles(*pDst, *pDst, sh_RecordFileData, &dstData, &n);
    assert(n >= 0);
    if (n == 1) {
        



        if (dstData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            dstIsDir = 1;
        }
    } else if (n > 1) {
        fprintf(stderr, "nsinstall: %ls: ambiguous destination file "
                "or directory\n", *pDst);
        return 3;
    } else {
        





        wchar_t *p;

        for (p = *pDst; *p; p++) {
            if (*p == '*' || *p == '?') {
                fprintf(stderr, "nsinstall: %ls: No such file or directory\n",
                        *pDst);
                return 3;
            }
        }

        




        if (p > *pDst && p[-1] == '\\' && p != *pDst + 1 && p[-2] != ':') {
            p[-1] = '\0';
        }
        wcscpy(dstData.pathName, *pDst);
        dstData.dwFileAttributes = 0xFFFFFFFF;
    }

    




    if (pDst - pSrc > 1 && !dstIsDir) {
        fprintf(stderr, "nsinstall: cannot copy more than"
                " one file to the same destination file\n");
        return 3;
    }

    if (dstIsDir) {
        arg.dstFileNameMarker = arg.dstFileName + wcslen(arg.dstFileName);

        






        if (arg.dstFileNameMarker[-1] != '\\') {
            *(arg.dstFileNameMarker++) = '\\';
        }
    }
    
    if (!dstIsDir) {
        struct sh_FileData srcData;

        assert(pDst - pSrc == 1);
        changeForwardSlashesToBackSlashes(*pSrc);
        sh_EnumerateFiles(*pSrc, *pSrc, sh_RecordFileData, &srcData, &n);
        if (n == 0) {
            fprintf(stderr, "nsinstall: %ls: No such file or directory\n",
                    *pSrc);
            retVal = 3;
        } else if (n > 1) {
            fprintf(stderr, "nsinstall: cannot copy more than one file or "
                    "directory to the same destination\n");
            retVal = 3;
        } else {
            assert(n == 1);
            if (sh_DoCopy(srcData.pathName, srcData.dwFileAttributes,
                    dstData.pathName, dstData.dwFileAttributes,
                    arg.force, arg.recursive) == FALSE) {
                retVal = 3;
            }
        }
        return retVal;
    }

    for ( ; *pSrc != *pDst; pSrc++) {
        BOOL rv;

        changeForwardSlashesToBackSlashes(*pSrc);
        rv = sh_EnumerateFiles(*pSrc, *pSrc, sh_CpFileCmd, &arg, &n);
        if (rv == FALSE) {
            retVal = 3;
        } else {
            if (n == 0) {
                fprintf(stderr, "nsinstall: %ls: No such file or directory\n",
                        *pSrc);
                retVal = 3;
            }
        }
    }

    return retVal;
}





















static BOOL sh_EnumerateFiles(
        const wchar_t *pattern,
        const wchar_t *where,
        sh_FileFcn fileFcn,
        void *arg,
        int *nFiles
        )
{
    WIN32_FIND_DATA fileData;
    HANDLE hSearch;
    const wchar_t *src;
    wchar_t *dst;
    wchar_t fileName[_MAX_PATH];
    wchar_t *fileNameMarker = fileName;
    wchar_t *oldFileNameMarker;
    BOOL hasWildcard = FALSE;
    BOOL retVal = TRUE;
    BOOL patternEndsInDotStar = FALSE;
    BOOL patternEndsInDot = FALSE;  

    int numDotsInPattern;
    int len;
    
    








    len = wcslen(pattern);
    if (len >= 2) {
        
        const wchar_t *p = &pattern[len - 1];

        
        while (p >= pattern && *p == '*') {
            p--;
        }
        if (p >= pattern && *p == '.') {
            patternEndsInDotStar = TRUE;
            if (p == &pattern[len - 1]) {
                patternEndsInDot = TRUE;
            }
            p--;
            numDotsInPattern = 1;
            while (p >= pattern && *p != '\\') {
                if (*p == '.') {
                    numDotsInPattern++;
                }
                p--;
            }
        }
    }

    *nFiles = 0;

    










    dst = fileName;
    src = pattern;
    while (src < where) {
        if (*src == '\\') {
            oldFileNameMarker = fileNameMarker;
            fileNameMarker = dst + 1;
        }
        *(dst++) = *(src++);
    }

    while (*src && *src != '*' && *src != '?') {
        if (*src == '\\') {
            oldFileNameMarker = fileNameMarker;
            fileNameMarker = dst + 1;
        }
        *(dst++) = *(src++);
    }

    if (*src) {
        



        hasWildcard = TRUE;
        while (*src && *src != '\\') {
            *(dst++) = *(src++);
        }
    }
    
    

    assert(*src == '\0' || *src == '\\');
    assert(hasWildcard || *src == '\0');
    *dst = '\0';

    




    if (!hasWildcard) {
        



        assert(!wcscmp(fileName, pattern));
        assert(wcslen(fileName) >= 1);
        if (dst[-1] == '\\' && (dst == fileName + 1 || dst[-2] == ':')) {
            fileData.cFileName[0] = '\0';
        } else {
            



            if (dst[-1] == '\\') {
                assert(*fileNameMarker == '\0');
                dst[-1] = '\0';
                fileNameMarker = oldFileNameMarker;
            } 
            wcscpy(fileData.cFileName, fileNameMarker);
        }
        fileData.dwFileAttributes = GetFileAttributes(fileName);
        if (fileData.dwFileAttributes == 0xFFFFFFFF) {
            return TRUE;
        }
        *nFiles = 1;
        return (*fileFcn)(fileName, &fileData, arg);
    }

    hSearch = FindFirstFile(fileName, &fileData);
    if (hSearch == INVALID_HANDLE_VALUE) {
        return retVal;
    }

    do {
        if (!wcscmp(fileData.cFileName, L".")
                || !wcscmp(fileData.cFileName, L"..")) {
            



            continue;
        }

        if (patternEndsInDotStar) {
            int nDots = 0;
            wchar_t *p = fileData.cFileName;
            while (*p) {
                if (*p == '.') {
                    nDots++;
                }
                p++;
            }
            
            if (patternEndsInDot && (p == fileData.cFileName
                    || p[-1] != '.')) {
                




                continue;
            }
            if (nDots < numDotsInPattern) {
                



                continue;
            }
        }

        wcscpy(fileNameMarker, fileData.cFileName);
        if (*src && *(src + 1)) {
            



            int n;

            assert(*src == '\\');
            where = fileName + wcslen(fileName);
            wcscat(fileName, src);
            sh_EnumerateFiles(fileName, where, fileFcn, arg, &n);
            *nFiles += n;
        } else {
            assert(wcschr(fileName, '*') == NULL);
            assert(wcschr(fileName, '?') == NULL);
            (*nFiles)++;
            if ((*fileFcn)(fileName, &fileData, arg) == FALSE) {
                retVal = FALSE;
            }
        }
    } while (FindNextFile(hSearch, &fileData));

    FindClose(hSearch);
    return retVal;
}
