 










































#include "nsCOMPtr.h"
#include "nsMemory.h"

#include "nsLocalFile.h"
#include "nsIDirectoryEnumerator.h"
#include "nsNativeCharsetUtils.h"

#include "nsISimpleEnumerator.h"
#include "nsIComponentManager.h"
#include "prtypes.h"
#include "prio.h"
#include "private/pprio.h"  
#include "prprf.h"
#include "prmem.h"
#include "nsHashKeys.h"

#include "nsXPIDLString.h"
#include "nsReadableUtils.h"

#include <direct.h>
#include <windows.h>

#ifndef WINCE
#include <aclapi.h>
#endif

#include "shellapi.h"
#include "shlguid.h"

#include  <io.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <mbstring.h>

#include "nsXPIDLString.h"
#include "prproces.h"
#include "nsITimelineService.h"

#include "nsAutoLock.h"
#include "SpecialSystemDirectory.h"

#include "nsTraceRefcntImpl.h"

#define CHECK_mWorkingPath()                    \
    PR_BEGIN_MACRO                              \
        if (mWorkingPath.IsEmpty())             \
            return NS_ERROR_NOT_INITIALIZED;    \
    PR_END_MACRO


#ifdef __MINGW32__
extern "C" {
unsigned char *_mbsstr( const unsigned char *str,
                        const unsigned char *substr );
}
#endif

#ifndef FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#endif

#ifndef WINCE
class nsDriveEnumerator : public nsISimpleEnumerator
{
public:
    nsDriveEnumerator();
    virtual ~nsDriveEnumerator();
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR
    nsresult Init();
private:
    




    nsString mDrives;
    nsAString::const_iterator mStartOfCurrentDrive;
    nsAString::const_iterator mEndOfDrivesString;
};
#endif




#ifdef WINCE
class ShortcutResolver
{
public:
    ShortcutResolver() {};
    
    ~ShortcutResolver() {};

    nsresult Init() { return NS_OK; }; 
    nsresult Resolve(const WCHAR* in, WCHAR* out);
};


nsresult
ShortcutResolver::Resolve(const WCHAR* in, WCHAR* out)
{
    return SHGetShortcutTarget(in, out, MAX_PATH) ? NS_OK : NS_ERROR_FAILURE;
}
#else 
class ShortcutResolver
{
public:
    ShortcutResolver();
    
    ~ShortcutResolver();

    nsresult Init();
    nsresult Resolve(const WCHAR* in, WCHAR* out);

private:
    PRLock*       mLock;
    IPersistFile* mPersistFile;
    
    IShellLinkW*  mShellLink;
};

ShortcutResolver::ShortcutResolver()
{
    mLock = nsnull;
    mPersistFile = nsnull;
    mShellLink  = nsnull;
}

ShortcutResolver::~ShortcutResolver()
{
    if (mLock)
        PR_DestroyLock(mLock);

    
    if (mPersistFile)
        mPersistFile->Release();

    
    if (mShellLink)
        mShellLink->Release();

    CoUninitialize();
}

nsresult
ShortcutResolver::Init()
{
    CoInitialize(NULL);  

    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_FAILURE;

    HRESULT hres; 
    hres = CoCreateInstance(CLSID_ShellLink,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLinkW,
                            (void**)&(mShellLink));
    if (SUCCEEDED(hres))
    {
        
        hres = mShellLink->QueryInterface(IID_IPersistFile,
                                          (void**)&mPersistFile);
    }

    if (mPersistFile == nsnull || mShellLink == nsnull)
        return NS_ERROR_FAILURE;

    return NS_OK;
}


nsresult
ShortcutResolver::Resolve(const WCHAR* in, WCHAR* out)
{
    nsAutoLock lock(mLock);

    
    HRESULT hres = mPersistFile->Load(in, STGM_READ);

    if (FAILED(hres))
        return NS_ERROR_FAILURE;

    
    hres = mShellLink->Resolve(nsnull, SLR_NO_UI);

    if (FAILED(hres))
        return NS_ERROR_FAILURE;

    
    hres = mShellLink->GetPath(out, MAX_PATH, NULL, SLGP_UNCPRIORITY);

    if (FAILED(hres))
        return NS_ERROR_FAILURE;
    return NS_OK;
}
#endif

static ShortcutResolver * gResolver = nsnull;

static nsresult NS_CreateShortcutResolver()
{
    gResolver = new ShortcutResolver();
    if (!gResolver)
        return NS_ERROR_OUT_OF_MEMORY;

    return gResolver->Init();
}

static void NS_DestroyShortcutResolver()
{
    delete gResolver;
    gResolver = nsnull;
}








static nsresult ConvertWinError(DWORD winErr)
{
    nsresult rv;

    switch (winErr)
    {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_DRIVE:
            rv = NS_ERROR_FILE_NOT_FOUND;
            break;
        case ERROR_ACCESS_DENIED:
        case ERROR_NOT_SAME_DEVICE:
            rv = NS_ERROR_FILE_ACCESS_DENIED;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_INVALID_BLOCK:
        case ERROR_INVALID_HANDLE:
        case ERROR_ARENA_TRASHED:
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
        case ERROR_CURRENT_DIRECTORY:
            rv = NS_ERROR_FILE_DIR_NOT_EMPTY;
            break;
        case ERROR_WRITE_PROTECT:
            rv = NS_ERROR_FILE_READ_ONLY;
            break;
        case ERROR_HANDLE_DISK_FULL:
            rv = NS_ERROR_FILE_TOO_BIG;
            break;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
        case ERROR_CANNOT_MAKE:
            rv = NS_ERROR_FILE_ALREADY_EXISTS;
            break;
        case ERROR_FILENAME_EXCED_RANGE:
            rv = NS_ERROR_FILE_NAME_TOO_LONG;
            break;
        case 0:
            rv = NS_OK;
            break;
        default:
            rv = NS_ERROR_FAILURE;
            break;
    }
    return rv;
}


static __int64 
MyFileSeek64(HANDLE aHandle, __int64 aDistance, DWORD aMoveMethod)
{
    LARGE_INTEGER li;

    li.QuadPart = aDistance;
    li.LowPart = SetFilePointer(aHandle, li.LowPart, &li.HighPart, aMoveMethod);
    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        li.QuadPart = -1;
    }

    return li.QuadPart;
}

static PRBool
IsShortcutPath(const nsAString &path)
{
    
    
    
    NS_ABORT_IF_FALSE(!path.IsEmpty(), "don't pass an empty string");
    PRInt32 len = path.Length();
    return (StringTail(path, 4).LowerCaseEqualsASCII(".lnk"));
}














typedef enum {
    _PR_TRI_TRUE = 1,
    _PR_TRI_FALSE = 0,
    _PR_TRI_UNKNOWN = -1
} _PRTriStateBool;

struct _MDFileDesc {
    PROsfd osfd;
};

struct PRFilePrivate {
    PRInt32 state;
    PRBool nonblocking;
    _PRTriStateBool inheritable;
    PRFileDesc *next;
    PRIntn lockCount;   


    PRBool  appendMode; 
    _MDFileDesc md;
};











static nsresult
OpenFile(const nsAFlatString &name, PRIntn osflags, PRIntn mode,
         PRFileDesc **fd)
{
    
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

    if (osflags & nsILocalFile::DELETE_ON_CLOSE) {
      flag6 |= FILE_FLAG_DELETE_ON_CLOSE;
    }

    HANDLE file = ::CreateFileW(name.get(), access,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                NULL, flags, flag6, NULL);

    if (file == INVALID_HANDLE_VALUE) { 
        *fd = nsnull;
        return ConvertWinError(GetLastError());
    }

    *fd = PR_ImportFile((PROsfd) file); 
    if (*fd) {
        
        
        (*fd)->secret->appendMode = (PR_APPEND & osflags) ? PR_TRUE : PR_FALSE;
        return NS_OK;
    }

    nsresult rv = NS_ErrorAccordingToNSPR();

    CloseHandle(file);

    return rv;
}



static
void FileTimeToPRTime(const FILETIME *filetime, PRTime *prtm)
{
#ifdef __GNUC__
    const PRTime _pr_filetime_offset = 116444736000000000LL;
#else
    const PRTime _pr_filetime_offset = 116444736000000000i64;
#endif

    PR_ASSERT(sizeof(FILETIME) == sizeof(PRTime));
    ::CopyMemory(prtm, filetime, sizeof(PRTime));
#ifdef __GNUC__
    *prtm = (*prtm - _pr_filetime_offset) / 10LL;
#else
    *prtm = (*prtm - _pr_filetime_offset) / 10i64;
#endif
}



static nsresult
GetFileInfo(const nsAFlatString &name, PRFileInfo64 *info)
{
    WIN32_FILE_ATTRIBUTE_DATA fileData;

    if (name.IsEmpty() || name.FindCharInSet(L"?*") != kNotFound)
        return NS_ERROR_INVALID_ARG;

    if (!::GetFileAttributesExW(name.get(), GetFileExInfoStandard, &fileData))
        return ConvertWinError(GetLastError());

    if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        info->type = PR_FILE_DIRECTORY;
    } else {
        info->type = PR_FILE_FILE;
    }

    info->size = fileData.nFileSizeHigh;
    info->size = (info->size << 32) + fileData.nFileSizeLow;

    FileTimeToPRTime(&fileData.ftLastWriteTime, &info->modifyTime);

    if (0 == fileData.ftCreationTime.dwLowDateTime &&
            0 == fileData.ftCreationTime.dwHighDateTime) {
        info->creationTime = info->modifyTime;
    } else {
        FileTimeToPRTime(&fileData.ftCreationTime, &info->creationTime);
    }

    return NS_OK;
}

struct nsDir
{
    HANDLE   handle; 
    WIN32_FIND_DATAW data;
    PRBool   firstEntry;
};

static nsresult
OpenDir(const nsAFlatString &name, nsDir * *dir)
{
    NS_ENSURE_ARG_POINTER(dir);

    *dir = nsnull;
    if (name.Length() + 3 >= MAX_PATH)
        return NS_ERROR_FILE_NAME_TOO_LONG;

    nsDir *d  = PR_NEW(nsDir);
    if (!d)
        return NS_ERROR_OUT_OF_MEMORY;

    nsAutoString filename(name);

     
     
    if (filename.Last() == L'/' || filename.Last() == L'\\')
        filename.AppendASCII("*");
    else 
        filename.AppendASCII("\\*");

    filename.ReplaceChar(L'/', L'\\');

    d->handle = ::FindFirstFileW(filename.get(), &(d->data) );

    if ( d->handle == INVALID_HANDLE_VALUE )
    {
        PR_Free(d);

#ifdef WINCE
        







        if (GetLastError() == ERROR_NO_MORE_FILES)
            return NS_OK;
#endif

        return ConvertWinError(GetLastError());
    }
    d->firstEntry = PR_TRUE;

    *dir = d;
    return NS_OK;
}

static nsresult
ReadDir(nsDir *dir, PRDirFlags flags, nsString& name)
{
    name.Truncate();
    NS_ENSURE_ARG(dir);

    while (1) {
        BOOL rv;
        if (dir->firstEntry)
        {
            dir->firstEntry = PR_FALSE;
            rv = 1;
        } else
            rv = ::FindNextFileW(dir->handle, &(dir->data));

        if (rv == 0)
            break;

        const PRUnichar *fileName;
        nsString tmp;
        fileName = (dir)->data.cFileName;

        if ((flags & PR_SKIP_DOT) &&
            (fileName[0] == L'.') && (fileName[1] == L'\0'))
            continue;
        if ((flags & PR_SKIP_DOT_DOT) &&
            (fileName[0] == L'.') && (fileName[1] == L'.') &&
            (fileName[2] == L'\0'))
            continue;

        DWORD attrib =  dir->data.dwFileAttributes;
        if ((flags & PR_SKIP_HIDDEN) && (attrib & FILE_ATTRIBUTE_HIDDEN))
            continue;

        if (fileName == tmp.get())
            name = tmp;
        else 
            name = fileName;
        return NS_OK;
    }

    DWORD err = GetLastError();
    return err == ERROR_NO_MORE_FILES ? NS_OK : ConvertWinError(err);
}

static nsresult
CloseDir(nsDir *d)
{
    NS_ENSURE_ARG(d);

    BOOL isOk = FindClose(d->handle);
    PR_DELETE(d);
    return isOk ? NS_OK : ConvertWinError(GetLastError());
}





class nsDirEnumerator : public nsISimpleEnumerator,
                        public nsIDirectoryEnumerator
{
    public:

        NS_DECL_ISUPPORTS

        nsDirEnumerator() : mDir(nsnull)
        {
        }

        nsresult Init(nsILocalFile* parent)
        {
            nsAutoString filepath;
            parent->GetTarget(filepath);

            if (filepath.IsEmpty())
            {
                parent->GetPath(filepath);
            }

            if (filepath.IsEmpty())
            {
                return NS_ERROR_UNEXPECTED;
            }

            nsresult rv = OpenDir(filepath, &mDir);
            if (NS_FAILED(rv))
                return rv;

            mParent = parent;
            return NS_OK;
        }

        NS_IMETHOD HasMoreElements(PRBool *result)
        {
            nsresult rv;
            if (mNext == nsnull && mDir)
            {
                nsString name;
                rv = ReadDir(mDir, PR_SKIP_BOTH, name);
                if (NS_FAILED(rv))
                    return rv;
                if (name.IsEmpty()) 
                {
                    
                    if (NS_FAILED(CloseDir(mDir)))
                        return NS_ERROR_FAILURE;

                    mDir = nsnull;

                    *result = PR_FALSE;
                    return NS_OK;
                }

                nsCOMPtr<nsIFile> file;
                rv = mParent->Clone(getter_AddRefs(file));
                if (NS_FAILED(rv))
                    return rv;

                rv = file->Append(name);
                if (NS_FAILED(rv))
                    return rv;

                
                PRBool exists;
                rv = file->Exists(&exists);
                if (NS_FAILED(rv) || !exists)
                {
                    return HasMoreElements(result);
                }

                mNext = do_QueryInterface(file);
            }
            *result = mNext != nsnull;
            if (!*result) 
                Close();
            return NS_OK;
        }

        NS_IMETHOD GetNext(nsISupports **result)
        {
            nsresult rv;
            PRBool hasMore;
            rv = HasMoreElements(&hasMore);
            if (NS_FAILED(rv)) return rv;

            *result = mNext;        
            NS_IF_ADDREF(*result);

            mNext = nsnull;
            return NS_OK;
        }

        NS_IMETHOD GetNextFile(nsIFile **result)
        {
            *result = nsnull;
            PRBool hasMore = PR_FALSE;
            nsresult rv = HasMoreElements(&hasMore);
            if (NS_FAILED(rv) || !hasMore)
                return rv;
            *result = mNext;
            NS_IF_ADDREF(*result);
            mNext = nsnull;
            return NS_OK;
        }

        NS_IMETHOD Close()
        {
            if (mDir)
            {
                nsresult rv = CloseDir(mDir);
                NS_ASSERTION(NS_SUCCEEDED(rv), "close failed");
                if (NS_FAILED(rv))
                    return NS_ERROR_FAILURE;
                mDir = nsnull;
            }
            return NS_OK;
        }

        
        
        ~nsDirEnumerator()
        {
            Close();
        }

    protected:
        nsDir*                  mDir;
        nsCOMPtr<nsILocalFile>  mParent;
        nsCOMPtr<nsILocalFile>  mNext;
};

NS_IMPL_ISUPPORTS2(nsDirEnumerator, nsISimpleEnumerator, nsIDirectoryEnumerator)






nsLocalFile::nsLocalFile()
  : mDirty(PR_TRUE)
  , mFollowSymlinks(PR_FALSE)
{
}

NS_METHOD
nsLocalFile::nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);
    NS_ENSURE_NO_AGGREGATION(outer);

    nsLocalFile* inst = new nsLocalFile();
    if (inst == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = inst->QueryInterface(aIID, aInstancePtr);
    if (NS_FAILED(rv))
    {
        delete inst;
        return rv;
    }
    return NS_OK;
}






NS_IMPL_THREADSAFE_ISUPPORTS4(nsLocalFile,
                              nsILocalFile,
                              nsIFile,
                              nsILocalFileWin,
                              nsIHashable)






nsLocalFile::nsLocalFile(const nsLocalFile& other)
  : mDirty(PR_TRUE)
  , mFollowSymlinks(other.mFollowSymlinks)
  , mWorkingPath(other.mWorkingPath)
{
}



nsresult
nsLocalFile::ResolveShortcut()
{
    
    if (!gResolver)
        return NS_ERROR_FAILURE;

    mResolvedPath.SetLength(MAX_PATH);
    if (mResolvedPath.Length() != MAX_PATH)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUnichar *resolvedPath = mResolvedPath.BeginWriting();

    
    nsresult rv = gResolver->Resolve(mWorkingPath.get(), resolvedPath);

    size_t len = NS_FAILED(rv) ? 0 : wcslen(resolvedPath);
    mResolvedPath.SetLength(len);

    return rv;
}



nsresult
nsLocalFile::ResolveAndStat()
{
    
    if (!mDirty)
        return NS_OK;

    
    if (mWorkingPath.IsEmpty())
        return NS_ERROR_FILE_INVALID_PATH;

    
    mResolvedPath.Assign(mWorkingPath);

    
    nsAutoString nsprPath(mWorkingPath.get());
    if (mWorkingPath.Length() == 2 && mWorkingPath.CharAt(1) == L':') 
        nsprPath.AppendASCII("\\");

    
    
    if (NS_FAILED(GetFileInfo(nsprPath, &mFileInfo64)))
        return NS_ERROR_FILE_NOT_FOUND;

    
    if (!mFollowSymlinks 
        || mFileInfo64.type != PR_FILE_FILE 
        || !IsShortcutPath(mWorkingPath))
    {
        mDirty = PR_FALSE;
        return NS_OK;
    }

    
    
    
    
    nsresult rv = ResolveShortcut();
    if (NS_FAILED(rv))
    {
        mResolvedPath.Assign(mWorkingPath);
        return rv;
    }

    
    if (NS_FAILED(GetFileInfo(mResolvedPath, &mFileInfo64)))
        return NS_ERROR_FILE_NOT_FOUND;

    mDirty = PR_FALSE;
    return NS_OK;
}






NS_IMETHODIMP
nsLocalFile::Clone(nsIFile **file)
{
    
    *file = new nsLocalFile(*this);
    if (!*file)
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*file);
    
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::InitWithFile(nsILocalFile *aFile)
{
    NS_ENSURE_ARG(aFile);
    
    nsAutoString path;
    aFile->GetPath(path);
    if (path.IsEmpty())
        return NS_ERROR_INVALID_ARG;
    return InitWithPath(path); 
}

NS_IMETHODIMP
nsLocalFile::InitWithPath(const nsAString &filePath)
{
    MakeDirty();

    nsAString::const_iterator begin, end;
    filePath.BeginReading(begin);
    filePath.EndReading(end);

    
    if (begin == end)
        return NS_ERROR_FAILURE;

    PRUnichar firstChar = *begin;
    PRUnichar secondChar = *(++begin);

    
    
    if (FindCharInReadable(L'/', begin, end))
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

#ifdef WINCE
    if (firstChar != L'\\')
#else
    if (secondChar != L':' && (secondChar != L'\\' || firstChar != L'\\'))
#endif
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    mWorkingPath = filePath;
    
    if (mWorkingPath.Last() == L'\\')
        mWorkingPath.Truncate(mWorkingPath.Length() - 1);

    return NS_OK;

}

NS_IMETHODIMP
nsLocalFile::OpenNSPRFileDesc(PRInt32 flags, PRInt32 mode, PRFileDesc **_retval)
{
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;

    return OpenFile(mResolvedPath, flags, mode, _retval);
}


NS_IMETHODIMP
nsLocalFile::OpenANSIFileDesc(const char *mode, FILE * *_retval)
{
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;

    *_retval = _wfopen(mResolvedPath.get(), NS_ConvertASCIItoUTF16(mode).get());
    if (*_retval)
        return NS_OK;

    return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsLocalFile::Create(PRUint32 type, PRUint32 attributes)
{
    if (type != NORMAL_FILE_TYPE && type != DIRECTORY_TYPE)
        return NS_ERROR_FILE_UNKNOWN_TYPE;

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;

    
    
    
    
    
    
    
    
    
    
    
    

    PRUnichar* path = mResolvedPath.BeginWriting();

    if (path[0] == L'\\' && path[1] == L'\\')
    {
#ifdef WINCE
        ++path;
#else
        
        path = wcschr(path + 2, L'\\');
        if (!path)
            return NS_ERROR_FILE_INVALID_PATH;
        ++path;
#endif
    }

    
    PRUnichar* slash = wcschr(path, L'\\');

    if (slash)
    {
        
        ++slash;
        slash = wcschr(slash, L'\\');

        while (slash)
        {
            *slash = L'\0';

            if (!::CreateDirectoryW(mResolvedPath.get(), NULL)) {
                rv = ConvertWinError(GetLastError());
                
                
                
                if (rv != NS_ERROR_FILE_ALREADY_EXISTS &&
                    rv != NS_ERROR_FILE_ACCESS_DENIED)
                    return rv;
            }
            *slash = L'\\';
            ++slash;
            slash = wcschr(slash, L'\\');
        }
    }

    if (type == NORMAL_FILE_TYPE)
    {
        PRFileDesc* file;
        rv = OpenFile(mResolvedPath,
                      PR_RDONLY | PR_CREATE_FILE | PR_APPEND | PR_EXCL, attributes,
                      &file);
        if (file)
            PR_Close(file);

        if (rv == NS_ERROR_FILE_ACCESS_DENIED)
        {
            
            PRBool isdir;
            if (NS_SUCCEEDED(IsDirectory(&isdir)) && isdir)
                rv = NS_ERROR_FILE_ALREADY_EXISTS;
        }
        return rv;
    }

    if (type == DIRECTORY_TYPE)
    {
        if (!::CreateDirectoryW(mResolvedPath.get(), NULL))
            return ConvertWinError(GetLastError());
        else
            return NS_OK;
    }

    return NS_ERROR_FILE_UNKNOWN_TYPE;
}


NS_IMETHODIMP
nsLocalFile::Append(const nsAString &node)
{
    
    return AppendInternal(PromiseFlatString(node), PR_FALSE);
}

NS_IMETHODIMP
nsLocalFile::AppendRelativePath(const nsAString &node)
{
    
    return AppendInternal(PromiseFlatString(node), PR_TRUE);
}


nsresult
nsLocalFile::AppendInternal(const nsAFlatString &node, PRBool multipleComponents)
{
    if (node.IsEmpty())
        return NS_OK;

    
    if (node.First() == L'\\'                                   
        || node.FindChar(L'/') != kNotFound                     
        || node.EqualsASCII(".."))                              
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

#ifndef WINCE  
    if (multipleComponents)
    {
        
        
        
        
        NS_NAMED_LITERAL_STRING(doubleDot, "\\.."); 
        nsAString::const_iterator start, end, offset;
        node.BeginReading(start);
        node.EndReading(end);
        offset = end; 
        while (FindInReadable(doubleDot, start, offset))
        {
            if (offset == end || *offset == L'\\')
                return NS_ERROR_FILE_UNRECOGNIZED_PATH;
            start = offset;
            offset = end;
        }
        
        
        if (StringBeginsWith(node, NS_LITERAL_STRING("..\\")))
            return NS_ERROR_FILE_UNRECOGNIZED_PATH;
    }
    
    else if (node.FindChar(L'\\') != kNotFound)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;
#endif

    MakeDirty();
    
    mWorkingPath.Append(NS_LITERAL_STRING("\\") + node);
    
    return NS_OK;
}

#define TOUPPER(u) (((u) >= L'a' && (u) <= L'z') ? \
                    (u) - (L'a' - L'A') : (u))

NS_IMETHODIMP
nsLocalFile::Normalize()
{
#ifndef WINCE
    
    
    if (mWorkingPath.IsEmpty())
        return NS_OK;

    nsAutoString path(mWorkingPath);

    
    
    
    
    
    PRInt32 rootIdx = 2;        
    if (path.First() == L'\\')   
    {
        rootIdx = path.FindChar(L'\\', 2);   
        if (rootIdx == kNotFound)
            return NS_OK;                   
        rootIdx = path.FindChar(L'\\', rootIdx+1);
        if (rootIdx == kNotFound)
            return NS_OK;                   
    }
    else if (path.CharAt(rootIdx) != L'\\')
    {
        
        
        
        
        WCHAR cwd[MAX_PATH];
        WCHAR * pcwd = cwd;
        int drive = TOUPPER(path.First()) - 'A' + 1;
        















        if (!((1 << (drive - 1)) & _getdrives()))
            return NS_ERROR_FILE_INVALID_PATH;
        if (!_wgetdcwd(drive, pcwd, MAX_PATH))
            pcwd = _wgetdcwd(drive, 0, 0);
        if (!pcwd)
            return NS_ERROR_OUT_OF_MEMORY;

        nsAutoString currentDir(pcwd);
        if (pcwd != cwd)
            free(pcwd);

        if (currentDir.Last() == '\\')
            path.Replace(0, 2, currentDir);
        else
            path.Replace(0, 2, currentDir + NS_LITERAL_STRING("\\"));
    }
    NS_POSTCONDITION(0 < rootIdx && rootIdx < (PRInt32)path.Length(), "rootIdx is invalid");
    NS_POSTCONDITION(path.CharAt(rootIdx) == '\\', "rootIdx is invalid");

    
    if (rootIdx + 1 == (PRInt32)path.Length())
        return NS_OK;

    
    const PRUnichar * pathBuffer = path.get();  
    mWorkingPath.SetCapacity(path.Length()); 
    mWorkingPath.Assign(pathBuffer, rootIdx);

    
    
    
    
    
    
    
    
    
    
    PRInt32 len, begin, end = rootIdx;
    while (end < (PRInt32)path.Length())
    {
        
        
        
        
        
        begin = end + 1;
        end = path.FindChar('\\', begin);
        if (end == kNotFound)
            end = path.Length();
        len = end - begin;
        
        
        if (len == 0)
            continue;
        
        
        if (pathBuffer[begin] == '.')
        {
            
            if (len == 1)
                continue;   

            
            if (len >= 2 && pathBuffer[begin+1] == L'.')
            {
                
                if (len == 2)
                {
                    PRInt32 prev = mWorkingPath.RFindChar('\\');
                    if (prev >= rootIdx)
                        mWorkingPath.Truncate(prev);
                    continue;
                }

                
                
                int idx = len - 1;
                for (; idx >= 2; --idx) 
                {
                    if (pathBuffer[begin+idx] != L'.')
                        break;
                }

                
                
                if (idx < 2) 
                    continue;
            }
        }

        
        mWorkingPath.Append(pathBuffer + begin - 1, len + 1);
    }

    
    PRInt32 filePathLen = mWorkingPath.Length() - 1;
    while(filePathLen > 0 && (mWorkingPath[filePathLen] == L' ' ||
          mWorkingPath[filePathLen] == L'.'))
    {
        mWorkingPath.Truncate(filePathLen--);
    } 

    MakeDirty();
#else 
    
#endif 
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetLeafName(nsAString &aLeafName)
{
    aLeafName.Truncate();

    if(mWorkingPath.IsEmpty())
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    PRInt32 offset = mWorkingPath.RFindChar(L'\\');

    
    if (offset == kNotFound)
        aLeafName = mWorkingPath;
    else
        aLeafName = Substring(mWorkingPath, offset + 1);

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetLeafName(const nsAString &aLeafName)
{
    MakeDirty();

    if(mWorkingPath.IsEmpty())
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    
    PRInt32 offset = mWorkingPath.RFindChar(L'\\');
    if (offset)
    {
        mWorkingPath.Truncate(offset+1);
    }
    mWorkingPath.Append(aLeafName);

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetPath(nsAString &_retval)
{
    _retval = mWorkingPath;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetCanonicalPath(nsAString &aResult)
{
    EnsureShortPath();
    aResult.Assign(mShortWorkingPath);
    return NS_OK;
}

typedef struct {
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCODEPAGE;

NS_IMETHODIMP
nsLocalFile::GetVersionInfoField(const char* aField, nsAString& _retval)
{
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    rv = NS_ERROR_FAILURE;

    
    
    WCHAR *path = const_cast<WCHAR*>(mFollowSymlinks ? mResolvedPath.get() 
                                                        : mWorkingPath.get());

    DWORD dummy;
    DWORD size = ::GetFileVersionInfoSizeW(path, &dummy);
    if (!size)
        return rv;

    void* ver = calloc(size, 1);
    if (!ver)
        return NS_ERROR_OUT_OF_MEMORY;

    if (::GetFileVersionInfoW(path, 0, size, ver)) 
    {
        LANGANDCODEPAGE* translate = nsnull;
        UINT pageCount;
        BOOL queryResult = ::VerQueryValueW(ver, L"\\VarFileInfo\\Translation", 
                                            (void**)&translate, &pageCount);
        if (queryResult && translate) 
        {
            for (PRInt32 i = 0; i < 2; ++i) 
            { 
                PRUnichar subBlock[MAX_PATH];
                _snwprintf(subBlock, MAX_PATH,
                           L"\\StringFileInfo\\%04x%04x\\%s", 
                           (i == 0 ? translate[0].wLanguage 
                                   : ::GetUserDefaultLangID()),
                           translate[0].wCodePage,
                           NS_ConvertASCIItoUTF16(
                               nsDependentCString(aField)).get());
                subBlock[MAX_PATH - 1] = 0;
                LPVOID value = nsnull;
                UINT size;
                queryResult = ::VerQueryValueW(ver, subBlock, &value, &size);
                if (queryResult && value)
                {
                    _retval.Assign(static_cast<PRUnichar*>(value));
                    if (!_retval.IsEmpty()) 
                    {
                        rv = NS_OK;
                        break;
                    }
                }
            }
        }
    }
    free(ver);
    
    return rv;
}

nsresult
nsLocalFile::CopySingleFile(nsIFile *sourceFile, nsIFile *destParent,
                            const nsAString &newName, 
                            PRBool followSymlinks, PRBool move)
{
    nsresult rv;
    nsAutoString filePath;

    
    
    
    
    nsAutoString destPath;
    destParent->GetTarget(destPath);

    destPath.AppendASCII("\\");

    if (newName.IsEmpty())
    {
        nsAutoString aFileName;
        sourceFile->GetLeafName(aFileName);
        destPath.Append(aFileName);
    }
    else
    {
        destPath.Append(newName);
    }


    if (followSymlinks)
    {
        rv = sourceFile->GetTarget(filePath);
        if (filePath.IsEmpty())
            rv = sourceFile->GetPath(filePath);
    }
    else
    {
        rv = sourceFile->GetPath(filePath);
    }

    if (NS_FAILED(rv))
        return rv;

    int copyOK;

    if (!move)
        copyOK = ::CopyFileW(filePath.get(), destPath.get(), PR_TRUE);
    else {
#ifndef WINCE
        copyOK = ::MoveFileExW(filePath.get(), destPath.get(),
                               MOVEFILE_REPLACE_EXISTING |
                               MOVEFILE_COPY_ALLOWED |
                               MOVEFILE_WRITE_THROUGH);
#else
        DeleteFile(destPath.get());
        copyOK = :: MoveFileW(filePath.get(), destPath.get());
#endif
    }

    if (!copyOK)  
        rv = ConvertWinError(GetLastError());

#ifndef WINCE
    else if (move) 
    {
        PACL pOldDACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        ::GetNamedSecurityInfoW((LPWSTR)destPath.get(), SE_FILE_OBJECT,
                                DACL_SECURITY_INFORMATION,
                                NULL, NULL, &pOldDACL, NULL, &pSD);
        if (pOldDACL)
            ::SetNamedSecurityInfoW((LPWSTR)destPath.get(), SE_FILE_OBJECT,
                                    DACL_SECURITY_INFORMATION |
                                    UNPROTECTED_DACL_SECURITY_INFORMATION,
                                    NULL, NULL, pOldDACL, NULL);
        if (pSD)
            LocalFree((HLOCAL)pSD);
    }
#endif
    return rv;
}

nsresult
nsLocalFile::CopyMove(nsIFile *aParentDir, const nsAString &newName, PRBool followSymlinks, PRBool move)
{
    nsCOMPtr<nsIFile> newParentDir = aParentDir;
    
    
    
    
    nsresult rv  = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    if (!newParentDir)
    {
        

        if (newName.IsEmpty())
            return NS_ERROR_INVALID_ARG;

        rv = GetParent(getter_AddRefs(newParentDir));
        if (NS_FAILED(rv))
            return rv;
    }

    if (!newParentDir)
        return NS_ERROR_FILE_DESTINATION_NOT_DIR;

    
    PRBool exists;
    newParentDir->Exists(&exists);
    if (!exists)
    {
        rv = newParentDir->Create(DIRECTORY_TYPE, 0644);  
        if (NS_FAILED(rv))
            return rv;
    }
    else
    {
        PRBool isDir;
        newParentDir->IsDirectory(&isDir);
        if (isDir == PR_FALSE)
        {
            if (followSymlinks)
            {
                PRBool isLink;
                newParentDir->IsSymlink(&isLink);
                if (isLink)
                {
                    nsAutoString target;
                    newParentDir->GetTarget(target);

                    nsCOMPtr<nsILocalFile> realDest = new nsLocalFile();
                    if (realDest == nsnull)
                        return NS_ERROR_OUT_OF_MEMORY;

                    rv = realDest->InitWithPath(target);

                    if (NS_FAILED(rv))
                        return rv;

                    return CopyMove(realDest, newName, followSymlinks, move);
                }
            }
            else
            {
                return NS_ERROR_FILE_DESTINATION_NOT_DIR;
            }
        }
    }

    
    PRBool done = PR_FALSE;
    PRBool isDir;
    IsDirectory(&isDir);
    PRBool isSymlink;
    IsSymlink(&isSymlink);

    
    if (move || !isDir || (isSymlink && !followSymlinks))
    {
        
        rv = CopySingleFile(this, newParentDir, newName, followSymlinks, move);
        done = NS_SUCCEEDED(rv);
        
        
        if (!done && !(move && isDir))
            return rv;  
    }
    
    
    if (!done)
    {
        
        nsCOMPtr<nsIFile> target;
        rv = newParentDir->Clone(getter_AddRefs(target));

        if (NS_FAILED(rv))
            return rv;

        nsAutoString allocatedNewName;
        if (newName.IsEmpty())
        {
            PRBool isLink;
            IsSymlink(&isLink);
            if (isLink)
            {
                nsAutoString temp;
                GetTarget(temp);
                PRInt32 offset = temp.RFindChar(L'\\'); 
                if (offset == kNotFound)
                    allocatedNewName = temp;
                else 
                    allocatedNewName = Substring(temp, offset + 1);
            }
            else
            {
                GetLeafName(allocatedNewName);
            }
        }
        else
        {
            allocatedNewName = newName;
        }

        rv = target->Append(allocatedNewName);
        if (NS_FAILED(rv))
            return rv;

        allocatedNewName.Truncate();

        
        target->Exists(&exists);
        if (!exists)
        {
            
            rv = target->Create(DIRECTORY_TYPE, 0644);  
            if (NS_FAILED(rv))
                return rv;
        }
        else
        {
            
            PRBool isWritable;

            target->IsWritable(&isWritable);
            if (!isWritable)
                return NS_ERROR_FILE_ACCESS_DENIED;

            nsCOMPtr<nsISimpleEnumerator> targetIterator;
            rv = target->GetDirectoryEntries(getter_AddRefs(targetIterator));

            PRBool more;
            targetIterator->HasMoreElements(&more);
            
            if (more)
                return NS_ERROR_FILE_DIR_NOT_EMPTY;
        }

        nsDirEnumerator dirEnum;

        rv = dirEnum.Init(this);
        if (NS_FAILED(rv)) {
            NS_WARNING("dirEnum initialization failed");
            return rv;
        }

        PRBool more;
        while (NS_SUCCEEDED(dirEnum.HasMoreElements(&more)) && more)
        {
            nsCOMPtr<nsISupports> item;
            nsCOMPtr<nsIFile> file;
            dirEnum.GetNext(getter_AddRefs(item));
            file = do_QueryInterface(item);
            if (file)
            {
                PRBool isDir, isLink;

                file->IsDirectory(&isDir);
                file->IsSymlink(&isLink);

                if (move)
                {
                    if (followSymlinks)
                        return NS_ERROR_FAILURE;

                    rv = file->MoveTo(target, EmptyString());
                    NS_ENSURE_SUCCESS(rv,rv);
                }
                else
                {
                    if (followSymlinks)
                        rv = file->CopyToFollowingLinks(target, EmptyString());
                    else
                        rv = file->CopyTo(target, EmptyString());
                    NS_ENSURE_SUCCESS(rv,rv);
                }
            }
        }
        
        
        
        
        
        
        if (move)
        {
          rv = Remove(PR_FALSE );
          NS_ENSURE_SUCCESS(rv,rv);
        }
    }


    
    if (move)
    {
        MakeDirty();

        nsAutoString newParentPath;
        newParentDir->GetPath(newParentPath);

        if (newParentPath.IsEmpty())
            return NS_ERROR_FAILURE;

        if (newName.IsEmpty())
        {
            nsAutoString aFileName;
            GetLeafName(aFileName);

            InitWithPath(newParentPath);
            Append(aFileName);
        }
        else
        {
            InitWithPath(newParentPath);
            Append(newName);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::CopyTo(nsIFile *newParentDir, const nsAString &newName)
{
    return CopyMove(newParentDir, newName, PR_FALSE, PR_FALSE);
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinks(nsIFile *newParentDir, const nsAString &newName)
{
    return CopyMove(newParentDir, newName, PR_TRUE, PR_FALSE);
}

NS_IMETHODIMP
nsLocalFile::MoveTo(nsIFile *newParentDir, const nsAString &newName)
{
    return CopyMove(newParentDir, newName, PR_FALSE, PR_TRUE);
}


NS_IMETHODIMP
nsLocalFile::Load(PRLibrary * *_retval)
{
    
    CHECK_mWorkingPath();

    PRBool isFile;
    nsresult rv = IsFile(&isFile);

    if (NS_FAILED(rv))
        return rv;

    if (! isFile)
        return NS_ERROR_FILE_IS_DIRECTORY;

    NS_TIMELINE_START_TIMER("PR_LoadLibraryWithFlags");

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(PR_FALSE);
#endif

    PRLibSpec libSpec;
    libSpec.value.pathname_u = mResolvedPath.get();
    libSpec.type = PR_LibSpec_PathnameU;
    *_retval =  PR_LoadLibraryWithFlags(libSpec, 0);

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(PR_TRUE);
#endif

    NS_TIMELINE_STOP_TIMER("PR_LoadLibraryWithFlags");
    NS_TIMELINE_MARK_TIMER1("PR_LoadLibraryWithFlags",
                            NS_ConvertUTF16toUTF8(mResolvedPath).get());

    if (*_retval)
        return NS_OK;
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsLocalFile::Remove(PRBool recursive)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    CHECK_mWorkingPath();

    PRBool isDir, isLink;
    nsresult rv;
    
    isDir = PR_FALSE;
    rv = IsSymlink(&isLink);
    if (NS_FAILED(rv))
        return rv;

    
    if (!isLink)
    {
        rv = IsDirectory(&isDir);
        if (NS_FAILED(rv))
            return rv;
    }

    if (isDir)
    {
        if (recursive)
        {
            nsDirEnumerator dirEnum;

            rv = dirEnum.Init(this);
            if (NS_FAILED(rv))
                return rv;

            PRBool more;
            while (NS_SUCCEEDED(dirEnum.HasMoreElements(&more)) && more)
            {
                nsCOMPtr<nsISupports> item;
                dirEnum.GetNext(getter_AddRefs(item));
                nsCOMPtr<nsIFile> file = do_QueryInterface(item);
                if (file)
                    file->Remove(recursive);
            }
        }
        if (RemoveDirectoryW(mWorkingPath.get()) == 0)
            return ConvertWinError(GetLastError());
    }
    else
    {
        if (DeleteFileW(mWorkingPath.get()) == 0)
            return ConvertWinError(GetLastError());
    }

    MakeDirty();
    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetLastModifiedTime(PRInt64 *aLastModifiedTime)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aLastModifiedTime);
 
    
    
    
    

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    
    PRInt64 usecPerMsec;
    LL_I2L(usecPerMsec, PR_USEC_PER_MSEC);
    LL_DIV(*aLastModifiedTime, mFileInfo64.modifyTime, usecPerMsec);
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetLastModifiedTimeOfLink(PRInt64 *aLastModifiedTime)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aLastModifiedTime);
 
    
    

    PRFileInfo64 info;
    nsresult rv = GetFileInfo(mWorkingPath, &info);
    if (NS_FAILED(rv)) 
        return rv;

    
    PRInt64 usecPerMsec;
    LL_I2L(usecPerMsec, PR_USEC_PER_MSEC);
    LL_DIV(*aLastModifiedTime, info.modifyTime, usecPerMsec);
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::SetLastModifiedTime(PRInt64 aLastModifiedTime)
{
    
    CHECK_mWorkingPath();

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    
    
    
    

    rv = SetModDate(aLastModifiedTime, mResolvedPath.get());
    if (NS_SUCCEEDED(rv))
        MakeDirty();

    return rv;
}


NS_IMETHODIMP
nsLocalFile::SetLastModifiedTimeOfLink(PRInt64 aLastModifiedTime)
{
    
    

    nsresult rv = SetModDate(aLastModifiedTime, mWorkingPath.get());
    if (NS_SUCCEEDED(rv))
        MakeDirty();

    return rv;
}

nsresult
nsLocalFile::SetModDate(PRInt64 aLastModifiedTime, const PRUnichar *filePath)
{
    HANDLE file = ::CreateFileW(filePath,          
                                GENERIC_WRITE,     
                                0,                 
                                NULL,              
                                OPEN_EXISTING,     
                                0,                 
                                NULL);

    if (file == INVALID_HANDLE_VALUE)
    {
        return ConvertWinError(GetLastError());
    }

    FILETIME lft, ft;
    SYSTEMTIME st;
    PRExplodedTime pret;

    
    PR_ExplodeTime(aLastModifiedTime * PR_USEC_PER_MSEC, PR_LocalTimeParameters, &pret);
    st.wYear            = pret.tm_year;
    st.wMonth           = pret.tm_month + 1; 
    st.wDayOfWeek       = pret.tm_wday;
    st.wDay             = pret.tm_mday;
    st.wHour            = pret.tm_hour;
    st.wMinute          = pret.tm_min;
    st.wSecond          = pret.tm_sec;
    st.wMilliseconds    = pret.tm_usec/1000;

    nsresult rv = NS_OK;
    
    if (!(SystemTimeToFileTime(&st, &lft) != 0 &&
          LocalFileTimeToFileTime(&lft, &ft) != 0 &&
          SetFileTime(file, NULL, &ft, &ft) != 0))
    {
      rv = ConvertWinError(GetLastError());
    }

    CloseHandle(file);
    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetPermissions(PRUint32 *aPermissions)
{
    NS_ENSURE_ARG(aPermissions);

    
    
    
    
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    PRBool isWritable, isExecutable;
    IsWritable(&isWritable);
    IsExecutable(&isExecutable);

    *aPermissions = PR_IRUSR|PR_IRGRP|PR_IROTH;         
    if (isWritable)
        *aPermissions |= PR_IWUSR|PR_IWGRP|PR_IWOTH;    
    if (isExecutable)
        *aPermissions |= PR_IXUSR|PR_IXGRP|PR_IXOTH;    

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPermissionsOfLink(PRUint32 *aPermissions)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aPermissions);

    
    
    

    DWORD word = ::GetFileAttributesW(mWorkingPath.get());
    if (word == INVALID_FILE_ATTRIBUTES)
        return NS_ERROR_FILE_INVALID_PATH;

    PRBool isWritable = !(word & FILE_ATTRIBUTE_READONLY);
    *aPermissions = PR_IRUSR|PR_IRGRP|PR_IROTH;         
    if (isWritable)
        *aPermissions |= PR_IWUSR|PR_IWGRP|PR_IWOTH;    

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::SetPermissions(PRUint32 aPermissions)
{
    
    CHECK_mWorkingPath();

    
    
    
    
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;
#ifndef WINCE

    
    int mode = 0;
    if (aPermissions & (PR_IRUSR|PR_IRGRP|PR_IROTH))    
        mode |= _S_IREAD;
    if (aPermissions & (PR_IWUSR|PR_IWGRP|PR_IWOTH))    
        mode |= _S_IWRITE;

    if (_wchmod(mResolvedPath.get(), mode) == -1)
        return NS_ERROR_FAILURE;

    return NS_OK;
#else

    
    DWORD mode = 0;
    if (!(aPermissions & (PR_IWUSR|PR_IWGRP|PR_IWOTH)))    
        mode = FILE_ATTRIBUTE_READONLY;
	else
		mode = FILE_ATTRIBUTE_NORMAL;

    if (SetFileAttributesW(mResolvedPath.get(), mode) == 0)
        return NS_ERROR_FAILURE;

    return NS_OK;
#endif
}

NS_IMETHODIMP
nsLocalFile::SetPermissionsOfLink(PRUint32 aPermissions)
{
#ifndef WINCE
    
    

    
    int mode = 0;
    if (aPermissions & (PR_IRUSR|PR_IRGRP|PR_IROTH))    
        mode |= _S_IREAD;
    if (aPermissions & (PR_IWUSR|PR_IWGRP|PR_IWOTH))    
        mode |= _S_IWRITE;

    if (_wchmod(mWorkingPath.get(), mode) == -1)
        return NS_ERROR_FAILURE;

    return NS_OK;
#else
    return NS_ERROR_NOT_AVAILABLE;
#endif
}


NS_IMETHODIMP
nsLocalFile::GetFileSize(PRInt64 *aFileSize)
{
    NS_ENSURE_ARG(aFileSize);

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    *aFileSize = mFileInfo64.size;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetFileSizeOfLink(PRInt64 *aFileSize)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aFileSize);

    
    

    PRFileInfo64 info;
    if (NS_FAILED(GetFileInfo(mWorkingPath, &info)))
        return NS_ERROR_FILE_INVALID_PATH;

    *aFileSize = info.size;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetFileSize(PRInt64 aFileSize)
{
    
    CHECK_mWorkingPath();

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    HANDLE hFile = ::CreateFileW(mResolvedPath.get(),
                                 GENERIC_WRITE,      
                                 FILE_SHARE_READ,    
                                 NULL,               
                                 OPEN_EXISTING,          
                                 FILE_ATTRIBUTE_NORMAL,  
                                 NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return ConvertWinError(GetLastError());
    }

    
    
    rv = NS_ERROR_FAILURE;
    aFileSize = MyFileSeek64(hFile, aFileSize, FILE_BEGIN);
    if (aFileSize != -1 && SetEndOfFile(hFile))
    {
        MakeDirty();
        rv = NS_OK;
    }

    CloseHandle(hFile);
    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetDiskSpaceAvailable(PRInt64 *aDiskSpaceAvailable)
{
    
    CHECK_mWorkingPath();

#ifndef WINCE
    NS_ENSURE_ARG(aDiskSpaceAvailable);

    ResolveAndStat();

    ULARGE_INTEGER liFreeBytesAvailableToCaller, liTotalNumberOfBytes;
    if (::GetDiskFreeSpaceExW(mResolvedPath.get(), &liFreeBytesAvailableToCaller, 
                              &liTotalNumberOfBytes, NULL))
    {
        *aDiskSpaceAvailable = liFreeBytesAvailableToCaller.QuadPart;
        return NS_OK;
    }
#endif
    
    *aDiskSpaceAvailable = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetParent(nsIFile * *aParent)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG_POINTER(aParent);

    
    if (mWorkingPath.Length() == 2) {
        *aParent = nsnull;
        return NS_OK;
    }

    PRInt32 offset = mWorkingPath.RFindChar(PRUnichar('\\'));
    
    
    
    
    if (offset == kNotFound)
      return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    
    if (offset == 1 && mWorkingPath[0] == L'\\') {
        *aParent = nsnull;
        return NS_OK;
    }

    nsAutoString parentPath(mWorkingPath);

    if (offset > 0)
        parentPath.Truncate(offset);
    else
        parentPath.AssignLiteral("\\\\.");

    nsCOMPtr<nsILocalFile> localFile;
    nsresult rv = NS_NewLocalFile(parentPath, mFollowSymlinks, getter_AddRefs(localFile));

    if(NS_SUCCEEDED(rv) && localFile)
    {
        return CallQueryInterface(localFile, aParent);
    }
    return rv;
}

NS_IMETHODIMP
nsLocalFile::Exists(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    MakeDirty();
    nsresult rv = ResolveAndStat();
    *_retval = NS_SUCCEEDED(rv);

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsWritable(PRBool *aIsWritable)
{
    
    CHECK_mWorkingPath();

    

    
    
    nsresult rv = IsDirectory(aIsWritable);
    if (NS_FAILED(rv))
        return rv;
    if (*aIsWritable)
        return NS_OK;

    
    rv = HasFileAttribute(FILE_ATTRIBUTE_READONLY, aIsWritable);
    if (NS_FAILED(rv))
        return rv;
    *aIsWritable = !*aIsWritable;

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsReadable(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsExecutable(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;
    
    nsresult rv;

    
    PRBool isFile;
    rv = IsFile(&isFile);
    if (NS_FAILED(rv))
        return rv;
    if (!isFile)
        return NS_OK;

    
    PRBool symLink;
    rv = IsSymlink(&symLink);
    if (NS_FAILED(rv))
        return rv;

    nsAutoString path;
    if (symLink)
        GetTarget(path);
    else
        GetPath(path);

    
    PRInt32 filePathLen = path.Length() - 1;
    while(filePathLen > 0 && (path[filePathLen] == L' ' || path[filePathLen] == L'.'))
    {
        path.Truncate(filePathLen--);
    } 

    
    PRInt32 dotIdx = path.RFindChar(PRUnichar('.'));
    if ( dotIdx != kNotFound ) {
        
        PRUnichar *p = path.BeginWriting();
        for( p+= dotIdx + 1; *p; p++ )
            *p +=  (*p >= L'A' && *p <= L'Z') ? 'a' - 'A' : 0; 
        
        
        static const char * const executableExts[] = {
            "ad",
            "ade",         
            "adp",
            "app",         
            "application", 
            "asp",
            "bas",
            "bat",
            "chm",
            "cmd",
            "com",
            "cpl",
            "crt",
            "exe",
            "fxp",         
            "hlp",
            "hta",
            "inf",
            "ins",
            "isp",
            "js",
            "jse",
            "lnk",
            "mad",         
            "maf",         
            "mag",         
            "mam",         
            "maq",         
            "mar",         
            "mas",         
            "mat",         
            "mau",         
            "mav",         
            "maw",         
            "mda",         
            "mdb",
            "mde",
            "mdt",         
            "mdw",         
            "mdz",         
            "msc",
            "msh",         
            "mshxml",      
            "msi",
            "msp",
            "mst",
            "ops",         
            "pcd",
            "pif",
            "plg",         
            "prf",         
            "prg",
            "pst",
            "reg",
            "scf",         
            "scr",
            "sct",
            "shb",
            "shs",
            "url",
            "vb",
            "vbe",
            "vbs",
            "vsd",
            "vsmacros",    
            "vss",
            "vst",
            "vsw",
            "ws",
            "wsc",
            "wsf",
            "wsh"};
        nsDependentSubstring ext = Substring(path, dotIdx + 1);
        for ( int i = 0; i < NS_ARRAY_LENGTH(executableExts); i++ ) {
            if ( ext.EqualsASCII(executableExts[i])) {
                
                *_retval = PR_TRUE;
                break;
            }
        }
    }

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsDirectory(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    *_retval = (mFileInfo64.type == PR_FILE_DIRECTORY); 
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsFile(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    *_retval = (mFileInfo64.type == PR_FILE_FILE); 
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsHidden(PRBool *_retval)
{
    return HasFileAttribute(FILE_ATTRIBUTE_HIDDEN, _retval);
}

nsresult
nsLocalFile::HasFileAttribute(DWORD fileAttrib, PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    
    const PRUnichar *filePath = mFollowSymlinks ? 
                                mResolvedPath.get() : mWorkingPath.get();
    DWORD word = ::GetFileAttributesW(filePath);

    *_retval = ((word & fileAttrib) != 0);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSymlink(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);

    
    if (!IsShortcutPath(mWorkingPath))
    {
        *_retval = PR_FALSE;
        return NS_OK;
    }

    
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    
    *_retval = (mFileInfo64.type == PR_FILE_FILE);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSpecial(PRBool *_retval)
{
    return HasFileAttribute(FILE_ATTRIBUTE_SYSTEM, _retval);
}

NS_IMETHODIMP
nsLocalFile::Equals(nsIFile *inFile, PRBool *_retval)
{
    NS_ENSURE_ARG(inFile);
    NS_ENSURE_ARG(_retval);

    EnsureShortPath();

    nsCOMPtr<nsILocalFileWin> lf(do_QueryInterface(inFile));
    if (!lf) {
        *_retval = PR_FALSE;
        return NS_OK;
    }

    nsAutoString inFilePath;
    lf->GetCanonicalPath(inFilePath);

    
    *_retval = _wcsicmp(mShortWorkingPath.get(), inFilePath.get()) == 0; 

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::Contains(nsIFile *inFile, PRBool recur, PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    *_retval = PR_FALSE;

    nsAutoString myFilePath;
    if (NS_FAILED(GetTarget(myFilePath)))
        GetPath(myFilePath);

    PRUint32 myFilePathLen = myFilePath.Length();

    nsAutoString inFilePath;
    if (NS_FAILED(inFile->GetTarget(inFilePath)))
        inFile->GetPath(inFilePath);

    
    if (inFilePath.Length() >= myFilePathLen && inFilePath[myFilePathLen] == L'\\')
    {
        if (_wcsnicmp(myFilePath.get(), inFilePath.get(), myFilePathLen) == 0)
        {
            *_retval = PR_TRUE;
        }

    }

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetTarget(nsAString &_retval)
{
    _retval.Truncate();
#if STRICT_FAKE_SYMLINKS
    PRBool symLink;

    nsresult rv = IsSymlink(&symLink);
    if (NS_FAILED(rv))
        return rv;

    if (!symLink)
    {
        return NS_ERROR_FILE_INVALID_PATH;
    }
#endif
    ResolveAndStat();

    _retval = mResolvedPath;
    return NS_OK;
}



NS_IMETHODIMP
nsLocalFile::GetFollowLinks(PRBool *aFollowLinks)
{
    *aFollowLinks = mFollowSymlinks;
    return NS_OK;
}
NS_IMETHODIMP
nsLocalFile::SetFollowLinks(PRBool aFollowLinks)
{
    MakeDirty();
    mFollowSymlinks = aFollowLinks;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetDirectoryEntries(nsISimpleEnumerator * *entries)
{
    nsresult rv;

    *entries = nsnull;
#ifndef WINCE
    if (mWorkingPath.EqualsLiteral("\\\\.")) {
        nsDriveEnumerator *drives = new nsDriveEnumerator;
        if (!drives)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(drives);
        rv = drives->Init();
        if (NS_FAILED(rv)) {
            NS_RELEASE(drives);
            return rv;
        }
        *entries = drives;
        return NS_OK;
    }
#endif

    PRBool isDir;
    rv = IsDirectory(&isDir);
    if (NS_FAILED(rv))
        return rv;
    if (!isDir)
        return NS_ERROR_FILE_NOT_DIRECTORY;

    nsDirEnumerator* dirEnum = new nsDirEnumerator();
    if (dirEnum == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(dirEnum);
    rv = dirEnum->Init(this);
    if (NS_FAILED(rv))
    {
        NS_RELEASE(dirEnum);
        return rv;
    }

    *entries = dirEnum;

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPersistentDescriptor(nsACString &aPersistentDescriptor)
{
    CopyUTF16toUTF8(mWorkingPath, aPersistentDescriptor);
    return NS_OK;
}   
    
NS_IMETHODIMP
nsLocalFile::SetPersistentDescriptor(const nsACString &aPersistentDescriptor)
{
    if (IsUTF8(aPersistentDescriptor))
        return InitWithPath(NS_ConvertUTF8toUTF16(aPersistentDescriptor));
    else
        return InitWithNativePath(aPersistentDescriptor);
}   


static PRBool IsXPOrGreater()
{
#ifdef WINCE
    return PR_FALSE;
#endif
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    return ((osvi.dwMajorVersion > 5) ||
       ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >= 1)));
}

NS_IMETHODIMP
nsLocalFile::GetFileAttributesWin(PRUint32 *aAttribs)
{
    *aAttribs = 0;
    DWORD dwAttrs = GetFileAttributesW(mWorkingPath.get());
    if (dwAttrs == INVALID_FILE_ATTRIBUTES)
      return NS_ERROR_FILE_INVALID_PATH;

    if (!(dwAttrs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
        *aAttribs |= WFA_SEARCH_INDEXED;

    return NS_OK;
}   
    
NS_IMETHODIMP
nsLocalFile::SetFileAttributesWin(PRUint32 aAttribs)
{
    DWORD dwAttrs = GetFileAttributesW(mWorkingPath.get());
    if (dwAttrs == INVALID_FILE_ATTRIBUTES)
      return NS_ERROR_FILE_INVALID_PATH;

    if (IsXPOrGreater()) {
      if (aAttribs & WFA_SEARCH_INDEXED) {
          dwAttrs &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
      } else {
          dwAttrs |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
      }
    }

    if (SetFileAttributesW(mWorkingPath.get(), dwAttrs) == 0)
      return NS_ERROR_FAILURE;
    return NS_OK;
}   


NS_IMETHODIMP
nsLocalFile::Reveal()
{
#ifndef WINCE
    
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;
    
    
    nsCOMPtr<nsILocalFile> winDir;
    rv = GetSpecialSystemDirectory(Win_WindowsDirectory, getter_AddRefs(winDir));
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoString explorerPath;
    rv = winDir->GetPath(explorerPath);  
    NS_ENSURE_SUCCESS(rv, rv);
    explorerPath.Append(L"\\explorer.exe");
    
    
    
    
    
    nsAutoString explorerParams;
    if (mFileInfo64.type != PR_FILE_DIRECTORY) 
        explorerParams.Append(L"/n,/select,");
    explorerParams.Append(L'\"');
    explorerParams.Append(mResolvedPath);
    explorerParams.Append(L'\"');
    
    if (::ShellExecuteW(NULL, L"open", explorerPath.get(), explorerParams.get(),
                        NULL, SW_SHOWNORMAL) <= (HINSTANCE) 32)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
#else
    return NS_ERROR_NOT_AVAILABLE;
#endif
}
#ifdef WINCE 
#ifndef UNICODE
#error "we don't support narrow char wince"
#endif

#define SHELLEXECUTEINFOW SHELLEXECUTEINFO
#define ShellExecuteExW ShellExecuteEx

#endif

NS_IMETHODIMP
nsLocalFile::Launch()
{
    const nsString &path = mWorkingPath;
    
    
    SHELLEXECUTEINFOW seinfo;
    memset(&seinfo, 0, sizeof(seinfo));
    seinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    seinfo.fMask  = NULL;
    seinfo.hwnd   = NULL;
    seinfo.lpVerb = NULL;
    seinfo.lpFile = path.get();
    seinfo.lpParameters =  NULL;
    seinfo.lpDirectory  = NULL;
    seinfo.nShow  = SW_SHOWNORMAL;
    
    if (ShellExecuteExW(&seinfo))
        return NS_OK;
    DWORD r = GetLastError();
    
    if (r == SE_ERR_NOASSOC) {
        nsAutoString shellArg;
        shellArg.Assign(NS_LITERAL_STRING("shell32.dll,OpenAs_RunDLL ") + path);
        seinfo.lpFile = L"RUNDLL32.EXE";
        seinfo.lpParameters = shellArg.get();
        if (ShellExecuteExW(&seinfo))
            return NS_OK;
        r = GetLastError();
    }
    if (r < 32) {
        switch (r) {
          case 0:
          case SE_ERR_OOM:
              return NS_ERROR_OUT_OF_MEMORY;
          case ERROR_FILE_NOT_FOUND:
              return NS_ERROR_FILE_NOT_FOUND;
          case ERROR_PATH_NOT_FOUND:
              return NS_ERROR_FILE_UNRECOGNIZED_PATH;
          case ERROR_BAD_FORMAT:
              return NS_ERROR_FILE_CORRUPTED;
          case SE_ERR_ACCESSDENIED:
              return NS_ERROR_FILE_ACCESS_DENIED;
          case SE_ERR_ASSOCINCOMPLETE:
          case SE_ERR_NOASSOC:
              return NS_ERROR_UNEXPECTED;
          case SE_ERR_DDEBUSY:
          case SE_ERR_DDEFAIL:
          case SE_ERR_DDETIMEOUT:
              return NS_ERROR_NOT_AVAILABLE;
          case SE_ERR_DLLNOTFOUND:
              return NS_ERROR_FAILURE;
          case SE_ERR_SHARE:
              return NS_ERROR_FILE_IS_LOCKED;
          default:
              return NS_ERROR_FILE_EXECUTION_FAILED;
        }
    }
    return NS_OK;
}


nsresult
NS_NewLocalFile(const nsAString &path, PRBool followLinks, nsILocalFile* *result)
{
    nsLocalFile* file = new nsLocalFile();
    if (file == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(file);
    
    file->SetFollowLinks(followLinks);

    if (!path.IsEmpty()) {
        nsresult rv = file->InitWithPath(path);
        if (NS_FAILED(rv)) {
            NS_RELEASE(file);
            return rv;
        }
    }

    *result = file;
    return NS_OK;
}




  
NS_IMETHODIMP
nsLocalFile::InitWithNativePath(const nsACString &filePath)
{
   nsAutoString tmp;
   nsresult rv = NS_CopyNativeToUnicode(filePath, tmp);
   if (NS_SUCCEEDED(rv))
       return InitWithPath(tmp);

   return rv;
}

NS_IMETHODIMP
nsLocalFile::AppendNative(const nsACString &node)
{
    nsAutoString tmp;
    nsresult rv = NS_CopyNativeToUnicode(node, tmp);
    if (NS_SUCCEEDED(rv))
        return Append(tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::AppendRelativeNativePath(const nsACString &node)
{
    nsAutoString tmp;
    nsresult rv = NS_CopyNativeToUnicode(node, tmp);
    if (NS_SUCCEEDED(rv))
        return AppendRelativePath(tmp);
    return rv;
}


NS_IMETHODIMP
nsLocalFile::GetNativeLeafName(nsACString &aLeafName)
{
    
    nsAutoString tmp;
    nsresult rv = GetLeafName(tmp);
    if (NS_SUCCEEDED(rv))
        rv = NS_CopyUnicodeToNative(tmp, aLeafName);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::SetNativeLeafName(const nsACString &aLeafName)
{
    nsAutoString tmp;
    nsresult rv = NS_CopyNativeToUnicode(aLeafName, tmp);
    if (NS_SUCCEEDED(rv))
        return SetLeafName(tmp);

    return rv;
}


NS_IMETHODIMP
nsLocalFile::GetNativePath(nsACString &_retval)
{
    
    nsAutoString tmp;
    nsresult rv = GetPath(tmp);
    if (NS_SUCCEEDED(rv))
        rv = NS_CopyUnicodeToNative(tmp, _retval);

    return rv;
}


NS_IMETHODIMP
nsLocalFile::GetNativeCanonicalPath(nsACString &aResult)
{
    NS_WARNING("This method is lossy. Use GetCanonicalPath !");
    EnsureShortPath();
    NS_CopyUnicodeToNative(mShortWorkingPath, aResult);
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::CopyToNative(nsIFile *newParentDir, const nsACString &newName)
{
    
    CHECK_mWorkingPath();

    if (newName.IsEmpty())
        return CopyTo(newParentDir, EmptyString());

    nsAutoString tmp;
    nsresult rv = NS_CopyNativeToUnicode(newName, tmp);
    if (NS_SUCCEEDED(rv))
        return CopyTo(newParentDir, tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinksNative(nsIFile *newParentDir, const nsACString &newName)
{
    if (newName.IsEmpty())
        return CopyToFollowingLinks(newParentDir, EmptyString());

    nsAutoString tmp;
    nsresult rv = NS_CopyNativeToUnicode(newName, tmp);
    if (NS_SUCCEEDED(rv))
        return CopyToFollowingLinks(newParentDir, tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::MoveToNative(nsIFile *newParentDir, const nsACString &newName)
{
    
    CHECK_mWorkingPath();

    if (newName.IsEmpty())
        return MoveTo(newParentDir, EmptyString());

    nsAutoString tmp;
    nsresult rv = NS_CopyNativeToUnicode(newName, tmp);
    if (NS_SUCCEEDED(rv))
        return MoveTo(newParentDir, tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetNativeTarget(nsACString &_retval)
{
    
    CHECK_mWorkingPath();

    NS_WARNING("This API is lossy. Use GetTarget !");
    nsAutoString tmp;
    nsresult rv = GetTarget(tmp);
    if (NS_SUCCEEDED(rv))
        rv = NS_CopyUnicodeToNative(tmp, _retval);

    return rv;
}

nsresult
NS_NewNativeLocalFile(const nsACString &path, PRBool followLinks, nsILocalFile* *result)
{
    nsAutoString buf;
    nsresult rv = NS_CopyNativeToUnicode(path, buf);
    if (NS_FAILED(rv)) {
        *result = nsnull;
        return rv;
    }
    return NS_NewLocalFile(buf, followLinks, result);
}

void
nsLocalFile::EnsureShortPath()
{
    if (!mShortWorkingPath.IsEmpty())
        return;
#ifdef WINCE
	 mShortWorkingPath.Assign(mWorkingPath);
#else
    WCHAR thisshort[MAX_PATH];
    DWORD thisr = ::GetShortPathNameW(mWorkingPath.get(), thisshort,
                                      sizeof(thisshort));
    
    if (thisr != 0 && thisr < sizeof(thisshort))
        mShortWorkingPath.Assign(thisshort);
    else
        mShortWorkingPath.Assign(mWorkingPath);
#endif
}



NS_IMETHODIMP
nsLocalFile::Equals(nsIHashable* aOther, PRBool *aResult)
{
    nsCOMPtr<nsIFile> otherfile(do_QueryInterface(aOther));
    if (!otherfile) {
        *aResult = PR_FALSE;
        return NS_OK;
    }

    return Equals(otherfile, aResult);
}

NS_IMETHODIMP
nsLocalFile::GetHashCode(PRUint32 *aResult)
{
    
    
    EnsureShortPath();

    *aResult = HashString(mShortWorkingPath);
    return NS_OK;
}





void
nsLocalFile::GlobalInit()
{
    nsresult rv = NS_CreateShortcutResolver();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Shortcut resolver could not be created");
}

void
nsLocalFile::GlobalShutdown()
{
    NS_DestroyShortcutResolver();
}

#ifndef WINCE
NS_IMPL_ISUPPORTS1(nsDriveEnumerator, nsISimpleEnumerator)

nsDriveEnumerator::nsDriveEnumerator()
{
}

nsDriveEnumerator::~nsDriveEnumerator()
{
}

nsresult nsDriveEnumerator::Init()
{
    


    DWORD length = GetLogicalDriveStringsW(0, 0);
    
    if (!EnsureStringLength(mDrives, length+1))
        return NS_ERROR_OUT_OF_MEMORY;
    if (!GetLogicalDriveStringsW(length, mDrives.BeginWriting()))
        return NS_ERROR_FAILURE;
    mDrives.BeginReading(mStartOfCurrentDrive);
    mDrives.EndReading(mEndOfDrivesString);
    return NS_OK;
}

NS_IMETHODIMP nsDriveEnumerator::HasMoreElements(PRBool *aHasMore)
{
    *aHasMore = *mStartOfCurrentDrive != L'\0';
    return NS_OK;
}

NS_IMETHODIMP nsDriveEnumerator::GetNext(nsISupports **aNext)
{
    



    if (*mStartOfCurrentDrive == L'\0') {
        *aNext = nsnull;
        return NS_OK;
    }

    nsAString::const_iterator driveEnd = mStartOfCurrentDrive;
    FindCharInReadable(L'\0', driveEnd, mEndOfDrivesString);
    nsString drive(Substring(mStartOfCurrentDrive, driveEnd));
    mStartOfCurrentDrive = ++driveEnd;

    nsILocalFile *file;
    nsresult rv = NS_NewLocalFile(drive, PR_FALSE, &file);

    *aNext = file;
    return rv;
}
#endif

