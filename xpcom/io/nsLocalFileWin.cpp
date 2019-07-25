 




#include "mozilla/Util.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
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
#include <shlwapi.h>
#include <aclapi.h>

#include "shellapi.h"
#include "shlguid.h"

#include  <io.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <mbstring.h>

#include "nsXPIDLString.h"
#include "prproces.h"

#include "mozilla/Mutex.h"
#include "SpecialSystemDirectory.h"

#include "nsTraceRefcntImpl.h"
#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"

using namespace mozilla;

#define CHECK_mWorkingPath()                    \
    PR_BEGIN_MACRO                              \
        if (mWorkingPath.IsEmpty())             \
            return NS_ERROR_NOT_INITIALIZED;    \
    PR_END_MACRO


#ifndef COPY_FILE_NO_BUFFERING
#define COPY_FILE_NO_BUFFERING 0x00001000
#endif

#ifndef FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#endif

#ifndef DRIVE_REMOTE
#define DRIVE_REMOTE 4
#endif





class AsyncLocalFileWinDone : public nsRunnable
{
public:
    AsyncLocalFileWinDone() :
        mWorkerThread(do_GetCurrentThread())
    {
        
        MOZ_ASSERT(!NS_IsMainThread()); 
    }

    NS_IMETHOD Run() {
        
        MOZ_ASSERT(NS_IsMainThread());

        
        
        mWorkerThread->Shutdown();
        return NS_OK;
    }

private:
    nsCOMPtr<nsIThread> mWorkerThread;
};





class AsyncLocalFileWinOperation : public nsRunnable
{
public:
    enum FileOp { RevealOp, LaunchOp };

    AsyncLocalFileWinOperation(AsyncLocalFileWinOperation::FileOp aOperation,
                               const nsAString &aResolvedPath) : 
        mOperation(aOperation),
        mResolvedPath(aResolvedPath)
    {
    }

    NS_IMETHOD Run() {
        NS_ASSERTION(!NS_IsMainThread(),
            "AsyncLocalFileWinOperation should not be run on the main thread!");

        CoInitialize(NULL);
        switch(mOperation) {
        case RevealOp: {
            Reveal();
        }
        break;
        case LaunchOp: {
            Launch();
        }
        break;
        }
        CoUninitialize();

        
        nsCOMPtr<nsIRunnable> resultrunnable = new AsyncLocalFileWinDone();
        NS_DispatchToMainThread(resultrunnable);
        return NS_OK;
    }

private:
    
    nsresult Reveal() 
    {
        DWORD attributes = GetFileAttributesW(mResolvedPath.get());
        if (INVALID_FILE_ATTRIBUTES == attributes) {
            return NS_ERROR_FILE_INVALID_PATH;
        }

        HRESULT hr;
        if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
            
            ITEMIDLIST *dir = ILCreateFromPathW(mResolvedPath.get());
            if (!dir) {
              return NS_ERROR_FAILURE;
            }

            const ITEMIDLIST* selection[] = { dir };
            UINT count = ArrayLength(selection);

            
            hr = SHOpenFolderAndSelectItems(dir, count, selection, 0);
            CoTaskMemFree(dir);
        } else {
            int32_t len = mResolvedPath.Length();
            
            
            if (len > MAX_PATH) {
                return NS_ERROR_FILE_INVALID_PATH;
            }
            WCHAR parentDirectoryPath[MAX_PATH + 1] = { 0 };
            wcsncpy(parentDirectoryPath, mResolvedPath.get(), MAX_PATH);
            PathRemoveFileSpecW(parentDirectoryPath);

            
            ITEMIDLIST *dir = ILCreateFromPathW(parentDirectoryPath);
            if (!dir) {
                return NS_ERROR_FAILURE;
            }

            
            ITEMIDLIST *item = ILCreateFromPathW(mResolvedPath.get());
            if (!item) {
                CoTaskMemFree(dir);
                return NS_ERROR_FAILURE;
            }
            
            const ITEMIDLIST* selection[] = { item };
            UINT count = ArrayLength(selection);

            
            hr = SHOpenFolderAndSelectItems(dir, count, selection, 0);

            CoTaskMemFree(dir);
            CoTaskMemFree(item);
        }
        
        return SUCCEEDED(hr) ? NS_OK : NS_ERROR_FAILURE;
    }
    
    
    nsresult Launch()
    {
        
        SHELLEXECUTEINFOW seinfo;
        memset(&seinfo, 0, sizeof(seinfo));
        seinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
        seinfo.fMask  = 0;
        seinfo.hwnd   = NULL;
        seinfo.lpVerb = NULL;
        seinfo.lpFile = mResolvedPath.get();
        seinfo.lpParameters =  NULL;
        seinfo.lpDirectory  = NULL;
        seinfo.nShow  = SW_SHOWNORMAL;

        
        
        
        WCHAR workingDirectory[MAX_PATH + 1] = { L'\0' };
        wcsncpy(workingDirectory,  mResolvedPath.get(), MAX_PATH);
        if (PathRemoveFileSpecW(workingDirectory)) {
            seinfo.lpDirectory = workingDirectory;
        } else {
            NS_WARNING("Could not set working directory for launched file.");
        }
        
        if (ShellExecuteExW(&seinfo)) {
            return NS_OK;
        }
        DWORD r = GetLastError();
        
        
        if (r == SE_ERR_NOASSOC) {
            nsAutoString shellArg;
            shellArg.Assign(NS_LITERAL_STRING("shell32.dll,OpenAs_RunDLL ") + 
                            mResolvedPath);
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

    
    AsyncLocalFileWinOperation::FileOp mOperation;

    
    nsString mResolvedPath;
};

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




class ShortcutResolver
{
public:
    ShortcutResolver();
    
    ~ShortcutResolver();

    nsresult Init();
    nsresult Resolve(const WCHAR* in, WCHAR* out);
    nsresult SetShortcut(bool updateExisting,
                         const WCHAR* shortcutPath,
                         const WCHAR* targetPath,
                         const WCHAR* workingDir,
                         const WCHAR* args,
                         const WCHAR* description,
                         const WCHAR* iconFile,
                         int32_t iconIndex);

private:
    Mutex                  mLock;
    nsRefPtr<IPersistFile> mPersistFile;
    nsRefPtr<IShellLinkW>  mShellLink;
};

ShortcutResolver::ShortcutResolver() :
    mLock("ShortcutResolver.mLock")
{
    CoInitialize(NULL);
}

ShortcutResolver::~ShortcutResolver()
{
    CoUninitialize();
}

nsresult
ShortcutResolver::Init()
{
    
    if (FAILED(CoCreateInstance(CLSID_ShellLink,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IShellLinkW,
                                getter_AddRefs(mShellLink))) ||
        FAILED(mShellLink->QueryInterface(IID_IPersistFile,
                                          getter_AddRefs(mPersistFile)))) {
        mShellLink = nullptr;
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}


nsresult
ShortcutResolver::Resolve(const WCHAR* in, WCHAR* out)
{
    if (!mShellLink)
        return NS_ERROR_FAILURE;

    MutexAutoLock lock(mLock);

    if (FAILED(mPersistFile->Load(in, STGM_READ)) ||
        FAILED(mShellLink->Resolve(nullptr, SLR_NO_UI)) ||
        FAILED(mShellLink->GetPath(out, MAX_PATH, NULL, SLGP_UNCPRIORITY)))
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsresult
ShortcutResolver::SetShortcut(bool updateExisting,
                              const WCHAR* shortcutPath,
                              const WCHAR* targetPath,
                              const WCHAR* workingDir,
                              const WCHAR* args,
                              const WCHAR* description,
                              const WCHAR* iconPath,
                              int32_t iconIndex)
{
    if (!mShellLink) {
      return NS_ERROR_FAILURE;
    }

    if (!shortcutPath) {
      return NS_ERROR_FAILURE;
    }

    MutexAutoLock lock(mLock);

    if (updateExisting) {
      if (FAILED(mPersistFile->Load(shortcutPath, STGM_READWRITE))) {
        return NS_ERROR_FAILURE;
      }
    } else {
      if (!targetPath) {
        return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
      }

      
      
      if (FAILED(mShellLink->SetWorkingDirectory(L""))
       || FAILED(mShellLink->SetArguments(L""))
       || FAILED(mShellLink->SetDescription(L""))
       || FAILED(mShellLink->SetIconLocation(L"", 0))) {
        return NS_ERROR_FAILURE;
      }
    }

    if (targetPath && FAILED(mShellLink->SetPath(targetPath))) {
      return NS_ERROR_FAILURE;
    }

    if (workingDir && FAILED(mShellLink->SetWorkingDirectory(workingDir))) {
      return NS_ERROR_FAILURE;
    }

    if (args && FAILED(mShellLink->SetArguments(args))) {
      return NS_ERROR_FAILURE;
    }

    if (description && FAILED(mShellLink->SetDescription(description))) {
      return NS_ERROR_FAILURE;
    }

    if (iconPath && FAILED(mShellLink->SetIconLocation(iconPath, iconIndex))) {
      return NS_ERROR_FAILURE;
    }

    if (FAILED(mPersistFile->Save(shortcutPath,
                                  TRUE))) {
      
      
      
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

static ShortcutResolver * gResolver = nullptr;

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
    gResolver = nullptr;
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
        case ERROR_SHARING_VIOLATION: 
        case ERROR_LOCK_VIOLATION: 
            rv = NS_ERROR_FILE_IS_LOCKED;
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
        case ERROR_DIRECTORY:
            rv = NS_ERROR_FILE_NOT_DIRECTORY;
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

static bool
IsShortcutPath(const nsAString &path)
{
    
    
    
    NS_ABORT_IF_FALSE(!path.IsEmpty(), "don't pass an empty string");
    int32_t len = path.Length();
    return len >= 4 && (StringTail(path, 4).LowerCaseEqualsASCII(".lnk"));
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
    int32_t state;
    bool nonblocking;
    _PRTriStateBool inheritable;
    PRFileDesc *next;
    int lockCount;      


    bool    appendMode; 
    _MDFileDesc md;
};











static nsresult
OpenFile(const nsAFlatString &name, int osflags, int mode,
         PRFileDesc **fd)
{
    
    int32_t access = 0;
    int32_t flags = 0;
    int32_t flag6 = 0;

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

    if (osflags & nsIFile::DELETE_ON_CLOSE) {
      flag6 |= FILE_FLAG_DELETE_ON_CLOSE;
    }

    if (osflags & nsIFile::OS_READAHEAD) {
      flag6 |= FILE_FLAG_SEQUENTIAL_SCAN;
    }

    HANDLE file = ::CreateFileW(name.get(), access,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                NULL, flags, flag6, NULL);

    if (file == INVALID_HANDLE_VALUE) { 
        *fd = nullptr;
        return ConvertWinError(GetLastError());
    }

    *fd = PR_ImportFile((PROsfd) file); 
    if (*fd) {
        
        
        (*fd)->secret->appendMode = (PR_APPEND & osflags) ? true : false;
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
    bool     firstEntry;
};

static nsresult
OpenDir(const nsAFlatString &name, nsDir * *dir)
{
    NS_ENSURE_ARG_POINTER(dir);

    *dir = nullptr;
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

    if (d->handle == INVALID_HANDLE_VALUE) {
        PR_Free(d);
        return ConvertWinError(GetLastError());
    }
    d->firstEntry = true;

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
            dir->firstEntry = false;
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
CloseDir(nsDir *&d)
{
    NS_ENSURE_ARG(d);

    BOOL isOk = FindClose(d->handle);
    
    PR_DELETE(d);
    return isOk ? NS_OK : ConvertWinError(GetLastError());
}





class nsDirEnumerator MOZ_FINAL : public nsISimpleEnumerator,
                                  public nsIDirectoryEnumerator
{
    public:

        NS_DECL_ISUPPORTS

        nsDirEnumerator() : mDir(nullptr)
        {
        }

        nsresult Init(nsIFile* parent)
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

        NS_IMETHOD HasMoreElements(bool *result)
        {
            nsresult rv;
            if (mNext == nullptr && mDir)
            {
                nsString name;
                rv = ReadDir(mDir, PR_SKIP_BOTH, name);
                if (NS_FAILED(rv))
                    return rv;
                if (name.IsEmpty()) 
                {
                    
                    if (NS_FAILED(CloseDir(mDir)))
                        return NS_ERROR_FAILURE;

                    *result = false;
                    return NS_OK;
                }

                nsCOMPtr<nsIFile> file;
                rv = mParent->Clone(getter_AddRefs(file));
                if (NS_FAILED(rv))
                    return rv;

                rv = file->Append(name);
                if (NS_FAILED(rv))
                    return rv;

                mNext = do_QueryInterface(file);
            }
            *result = mNext != nullptr;
            if (!*result) 
                Close();
            return NS_OK;
        }

        NS_IMETHOD GetNext(nsISupports **result)
        {
            nsresult rv;
            bool hasMore;
            rv = HasMoreElements(&hasMore);
            if (NS_FAILED(rv)) return rv;

            *result = mNext;        
            NS_IF_ADDREF(*result);

            mNext = nullptr;
            return NS_OK;
        }

        NS_IMETHOD GetNextFile(nsIFile **result)
        {
            *result = nullptr;
            bool hasMore = false;
            nsresult rv = HasMoreElements(&hasMore);
            if (NS_FAILED(rv) || !hasMore)
                return rv;
            *result = mNext;
            NS_IF_ADDREF(*result);
            mNext = nullptr;
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
            }
            return NS_OK;
        }

        
        
        ~nsDirEnumerator()
        {
            Close();
        }

    protected:
        nsDir*             mDir;
        nsCOMPtr<nsIFile>  mParent;
        nsCOMPtr<nsIFile>  mNext;
};

NS_IMPL_ISUPPORTS2(nsDirEnumerator, nsISimpleEnumerator, nsIDirectoryEnumerator)






nsLocalFile::nsLocalFile()
  : mDirty(true)
  , mResolveDirty(true)
  , mFollowSymlinks(false)
{
}

nsresult
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
  : mDirty(true)
  , mResolveDirty(true)
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

    
    
    nsresult rv = GetFileInfo(nsprPath, &mFileInfo64);
    if (NS_FAILED(rv))
        return rv;

    
    if (!mFollowSymlinks 
        || mFileInfo64.type != PR_FILE_FILE 
        || !IsShortcutPath(mWorkingPath))
    {
        mDirty = false;
        mResolveDirty = false;
        return NS_OK;
    }

    
    
    
    
    rv = ResolveShortcut();
    if (NS_FAILED(rv))
    {
        mResolvedPath.Assign(mWorkingPath);
        return rv;
    }
    mResolveDirty = false;

    
    rv = GetFileInfo(mResolvedPath, &mFileInfo64);
    if (NS_FAILED(rv))
        return rv;

    mDirty = false;
    return NS_OK;
}








nsresult
nsLocalFile::Resolve()
{
  
  if (!mResolveDirty) {
    return NS_OK;
  }

  
  if (mWorkingPath.IsEmpty()) {
    return NS_ERROR_FILE_INVALID_PATH;
  }
  
  
  mResolvedPath.Assign(mWorkingPath);

  
  
  if (!mFollowSymlinks || 
      !IsShortcutPath(mWorkingPath)) {
    mResolveDirty = false;
    return NS_OK;
  }

  
  
  
  
  nsresult rv = ResolveShortcut();
  if (NS_FAILED(rv)) {
    mResolvedPath.Assign(mWorkingPath);
    return rv;
  }

  mResolveDirty = false;
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
nsLocalFile::InitWithFile(nsIFile *aFile)
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

    if (secondChar != L':' && (secondChar != L'\\' || firstChar != L'\\'))
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    if (secondChar == L':') {
        
        
        if (PathGetDriveNumberW(filePath.Data()) == -1) {
            return NS_ERROR_FILE_UNRECOGNIZED_PATH;
        }
    }

    mWorkingPath = filePath;
    
    if (mWorkingPath.Last() == L'\\')
        mWorkingPath.Truncate(mWorkingPath.Length() - 1);

    return NS_OK;

}

NS_IMETHODIMP
nsLocalFile::OpenNSPRFileDesc(int32_t flags, int32_t mode, PRFileDesc **_retval)
{
    nsresult rv = Resolve();
    if (NS_FAILED(rv))
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
nsLocalFile::Create(uint32_t type, uint32_t attributes)
{
    if (type != NORMAL_FILE_TYPE && type != DIRECTORY_TYPE)
        return NS_ERROR_FILE_UNKNOWN_TYPE;

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;

    
    
    
    
    
    
    
    
    
    
    
    

    PRUnichar* path = mResolvedPath.BeginWriting();

    if (path[0] == L'\\' && path[1] == L'\\')
    {
        
        path = wcschr(path + 2, L'\\');
        if (!path)
            return NS_ERROR_FILE_INVALID_PATH;
        ++path;
    }

    
    PRUnichar* slash = wcschr(path, L'\\');

    nsresult directoryCreateError = NS_OK;
    if (slash)
    {
        
        ++slash;
        slash = wcschr(slash, L'\\');

        while (slash)
        {
            *slash = L'\0';

            if (!::CreateDirectoryW(mResolvedPath.get(), NULL)) {
                rv = ConvertWinError(GetLastError());
                if (NS_ERROR_FILE_NOT_FOUND == rv &&
                    NS_ERROR_FILE_ACCESS_DENIED == directoryCreateError) {
                    
                    return NS_ERROR_FILE_ACCESS_DENIED;
                }
                
                
                
                else if (NS_ERROR_FILE_ALREADY_EXISTS != rv &&
                         NS_ERROR_FILE_ACCESS_DENIED != rv) {
                    return rv;
                }

                directoryCreateError = rv;
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
            
            bool isdir;
            if (NS_SUCCEEDED(IsDirectory(&isdir)) && isdir)
                rv = NS_ERROR_FILE_ALREADY_EXISTS;
        } else if (NS_ERROR_FILE_NOT_FOUND == rv && 
                   NS_ERROR_FILE_ACCESS_DENIED == directoryCreateError) {
            
            return NS_ERROR_FILE_ACCESS_DENIED;
        }
        return rv;
    }

    if (type == DIRECTORY_TYPE)
    {
        if (!::CreateDirectoryW(mResolvedPath.get(), NULL)) {
          rv = ConvertWinError(GetLastError());
          if (NS_ERROR_FILE_NOT_FOUND == rv && 
              NS_ERROR_FILE_ACCESS_DENIED == directoryCreateError) {
              
              return NS_ERROR_FILE_ACCESS_DENIED;
          } else {
              return rv;
          }
        }
        else
            return NS_OK;
    }

    return NS_ERROR_FILE_UNKNOWN_TYPE;
}


NS_IMETHODIMP
nsLocalFile::Append(const nsAString &node)
{
    
    return AppendInternal(PromiseFlatString(node), false);
}

NS_IMETHODIMP
nsLocalFile::AppendRelativePath(const nsAString &node)
{
    
    return AppendInternal(PromiseFlatString(node), true);
}


nsresult
nsLocalFile::AppendInternal(const nsAFlatString &node, bool multipleComponents)
{
    if (node.IsEmpty())
        return NS_OK;

    
    if (node.First() == L'\\'                                   
        || node.FindChar(L'/') != kNotFound                     
        || node.EqualsASCII(".."))                              
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

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

    MakeDirty();
    
    mWorkingPath.Append(NS_LITERAL_STRING("\\") + node);
    
    return NS_OK;
}

#define TOUPPER(u) (((u) >= L'a' && (u) <= L'z') ? \
                    (u) - (L'a' - L'A') : (u))

NS_IMETHODIMP
nsLocalFile::Normalize()
{
    
    
    if (mWorkingPath.IsEmpty())
        return NS_OK;

    nsAutoString path(mWorkingPath);

    
    
    
    
    
    int32_t rootIdx = 2;        
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
    NS_POSTCONDITION(0 < rootIdx && rootIdx < (int32_t)path.Length(), "rootIdx is invalid");
    NS_POSTCONDITION(path.CharAt(rootIdx) == '\\', "rootIdx is invalid");

    
    if (rootIdx + 1 == (int32_t)path.Length())
        return NS_OK;

    
    const PRUnichar * pathBuffer = path.get();  
    mWorkingPath.SetCapacity(path.Length()); 
    mWorkingPath.Assign(pathBuffer, rootIdx);

    
    
    
    
    
    
    
    
    
    
    int32_t len, begin, end = rootIdx;
    while (end < (int32_t)path.Length())
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
                    int32_t prev = mWorkingPath.RFindChar('\\');
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

    
    int32_t filePathLen = mWorkingPath.Length() - 1;
    while(filePathLen > 0 && (mWorkingPath[filePathLen] == L' ' ||
          mWorkingPath[filePathLen] == L'.'))
    {
        mWorkingPath.Truncate(filePathLen--);
    } 

    MakeDirty();
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetLeafName(nsAString &aLeafName)
{
    aLeafName.Truncate();

    if (mWorkingPath.IsEmpty())
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    int32_t offset = mWorkingPath.RFindChar(L'\\');

    
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

    if (mWorkingPath.IsEmpty())
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    
    int32_t offset = mWorkingPath.RFindChar(L'\\');
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
        LANGANDCODEPAGE* translate = nullptr;
        UINT pageCount;
        BOOL queryResult = ::VerQueryValueW(ver, L"\\VarFileInfo\\Translation", 
                                            (void**)&translate, &pageCount);
        if (queryResult && translate) 
        {
            for (int32_t i = 0; i < 2; ++i) 
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
                LPVOID value = nullptr;
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

NS_IMETHODIMP
nsLocalFile::SetShortcut(nsIFile* targetFile,
                         nsIFile* workingDir,
                         const PRUnichar* args,
                         const PRUnichar* description,
                         nsIFile* iconFile,
                         int32_t iconIndex)
{
    bool exists;
    nsresult rv = this->Exists(&exists);
    if (NS_FAILED(rv)) {
      return rv;
    }

    const WCHAR* targetFilePath = NULL;
    const WCHAR* workingDirPath = NULL;
    const WCHAR* iconFilePath = NULL;

    nsAutoString targetFilePathAuto;
    if (targetFile) {
        rv = targetFile->GetPath(targetFilePathAuto);
        if (NS_FAILED(rv)) {
          return rv;
        }
        targetFilePath = targetFilePathAuto.get();
    }

    nsAutoString workingDirPathAuto;
    if (workingDir) {
        rv = workingDir->GetPath(workingDirPathAuto);
        if (NS_FAILED(rv)) {
          return rv;
        }
        workingDirPath = workingDirPathAuto.get();
    }

    nsAutoString iconPathAuto;
    if (iconFile) {
        rv = iconFile->GetPath(iconPathAuto);
        if (NS_FAILED(rv)) {
          return rv;
        }
        iconFilePath = iconPathAuto.get();
    }

    rv = gResolver->SetShortcut(exists,
                                mWorkingPath.get(),
                                targetFilePath,
                                workingDirPath,
                                args,
                                description,
                                iconFilePath,
                                iconFilePath? iconIndex : 0);
    if (targetFilePath && NS_SUCCEEDED(rv)) {
      MakeDirty();
    }

    return rv;
}










static bool
IsRemoteFilePath(LPCWSTR path, bool &remote)
{
  
  
  WCHAR dirPath[MAX_PATH + 1] = { 0 };
  wcsncpy(dirPath, path, MAX_PATH);
  if (!PathRemoveFileSpecW(dirPath)) {
    return false;
  }
  size_t len = wcslen(dirPath);
  
  
  
  if (len >= MAX_PATH) {
    return false;
  }

  dirPath[len] = L'\\';
  dirPath[len + 1] = L'\0';
  UINT driveType = GetDriveTypeW(dirPath);
  remote = driveType == DRIVE_REMOTE;
  return true;
}

nsresult
nsLocalFile::CopySingleFile(nsIFile *sourceFile, nsIFile *destParent,
                            const nsAString &newName, 
                            bool followSymlinks, bool move,
                            bool skipNtfsAclReset)
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
    DWORD dwVersion = GetVersion();
    DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    DWORD dwCopyFlags = 0;
    if (dwMajorVersion > 5) {
        bool path1Remote, path2Remote;
        if (!IsRemoteFilePath(filePath.get(), path1Remote) || 
            !IsRemoteFilePath(destPath.get(), path2Remote) ||
            path1Remote || path2Remote) {
            dwCopyFlags = COPY_FILE_NO_BUFFERING;
        }
    }
    
    if (!move)
        copyOK = ::CopyFileExW(filePath.get(), destPath.get(), NULL, NULL, NULL, dwCopyFlags);
    else {
        DWORD status;
        if (FileEncryptionStatusW(filePath.get(), &status)
            && status == FILE_IS_ENCRYPTED)
        {
            dwCopyFlags |= COPY_FILE_ALLOW_DECRYPTED_DESTINATION;
            copyOK = CopyFileExW(filePath.get(), destPath.get(), NULL, NULL, NULL, dwCopyFlags);

            if (copyOK)
                DeleteFileW(filePath.get());
        }
        else
        {
            copyOK = ::MoveFileExW(filePath.get(), destPath.get(),
                                   MOVEFILE_REPLACE_EXISTING);
            
            
            
            if (!copyOK && GetLastError() == ERROR_NOT_SAME_DEVICE)
            {
                copyOK = CopyFileExW(filePath.get(), destPath.get(), NULL, NULL, NULL, dwCopyFlags);
            
                if (copyOK)
                    DeleteFile(filePath.get());
            }
        }
    }

    if (!copyOK)  
        rv = ConvertWinError(GetLastError());
    else if (move && !skipNtfsAclReset)
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

    return rv;
}

nsresult
nsLocalFile::CopyMove(nsIFile *aParentDir, const nsAString &newName, bool followSymlinks, bool move)
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

    
    bool exists;
    newParentDir->Exists(&exists);
    if (!exists)
    {
        rv = newParentDir->Create(DIRECTORY_TYPE, 0644);  
        if (NS_FAILED(rv))
            return rv;
    }
    else
    {
        bool isDir;
        newParentDir->IsDirectory(&isDir);
        if (!isDir)
        {
            if (followSymlinks)
            {
                bool isLink;
                newParentDir->IsSymlink(&isLink);
                if (isLink)
                {
                    nsAutoString target;
                    newParentDir->GetTarget(target);

                    nsCOMPtr<nsIFile> realDest = new nsLocalFile();
                    if (realDest == nullptr)
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

    
    bool done = false;
    bool isDir;
    IsDirectory(&isDir);
    bool isSymlink;
    IsSymlink(&isSymlink);

    
    if (move || !isDir || (isSymlink && !followSymlinks))
    {
        
        rv = CopySingleFile(this, newParentDir, newName, followSymlinks, move,
                            !aParentDir);
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
            bool isLink;
            IsSymlink(&isLink);
            if (isLink)
            {
                nsAutoString temp;
                GetTarget(temp);
                int32_t offset = temp.RFindChar(L'\\'); 
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
            
            bool isWritable;

            target->IsWritable(&isWritable);
            if (!isWritable)
                return NS_ERROR_FILE_ACCESS_DENIED;

            nsCOMPtr<nsISimpleEnumerator> targetIterator;
            rv = target->GetDirectoryEntries(getter_AddRefs(targetIterator));
            if (NS_FAILED(rv))
                return rv;

            bool more;
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

        bool more = false;
        while (NS_SUCCEEDED(dirEnum.HasMoreElements(&more)) && more)
        {
            nsCOMPtr<nsISupports> item;
            nsCOMPtr<nsIFile> file;
            dirEnum.GetNext(getter_AddRefs(item));
            file = do_QueryInterface(item);
            if (file)
            {
                bool isDir, isLink;

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
          rv = Remove(false );
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
    return CopyMove(newParentDir, newName, false, false);
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinks(nsIFile *newParentDir, const nsAString &newName)
{
    return CopyMove(newParentDir, newName, true, false);
}

NS_IMETHODIMP
nsLocalFile::MoveTo(nsIFile *newParentDir, const nsAString &newName)
{
    return CopyMove(newParentDir, newName, false, true);
}


NS_IMETHODIMP
nsLocalFile::Load(PRLibrary * *_retval)
{
    
    CHECK_mWorkingPath();

    bool isFile;
    nsresult rv = IsFile(&isFile);

    if (NS_FAILED(rv))
        return rv;

    if (! isFile)
        return NS_ERROR_FILE_IS_DIRECTORY;

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(false);
#endif

    PRLibSpec libSpec;
    libSpec.value.pathname_u = mResolvedPath.get();
    libSpec.type = PR_LibSpec_PathnameU;
    *_retval =  PR_LoadLibraryWithFlags(libSpec, 0);

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(true);
#endif

    if (*_retval)
        return NS_OK;
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsLocalFile::Remove(bool recursive)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    CHECK_mWorkingPath();

    bool isDir, isLink;
    nsresult rv;
    
    isDir = false;
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

            bool more = false;
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
nsLocalFile::GetLastModifiedTime(int64_t *aLastModifiedTime)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aLastModifiedTime);
 
    
    
    
    

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    
    int64_t usecPerMsec;
    LL_I2L(usecPerMsec, PR_USEC_PER_MSEC);
    LL_DIV(*aLastModifiedTime, mFileInfo64.modifyTime, usecPerMsec);
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetLastModifiedTimeOfLink(int64_t *aLastModifiedTime)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aLastModifiedTime);
 
    
    

    PRFileInfo64 info;
    nsresult rv = GetFileInfo(mWorkingPath, &info);
    if (NS_FAILED(rv)) 
        return rv;

    
    int64_t usecPerMsec;
    LL_I2L(usecPerMsec, PR_USEC_PER_MSEC);
    LL_DIV(*aLastModifiedTime, info.modifyTime, usecPerMsec);
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::SetLastModifiedTime(int64_t aLastModifiedTime)
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
nsLocalFile::SetLastModifiedTimeOfLink(int64_t aLastModifiedTime)
{
    
    

    nsresult rv = SetModDate(aLastModifiedTime, mWorkingPath.get());
    if (NS_SUCCEEDED(rv))
        MakeDirty();

    return rv;
}

nsresult
nsLocalFile::SetModDate(int64_t aLastModifiedTime, const PRUnichar *filePath)
{
    
    
    HANDLE file = ::CreateFileW(filePath,          
                                GENERIC_WRITE,     
                                0,                 
                                NULL,              
                                OPEN_EXISTING,     
                                FILE_FLAG_BACKUP_SEMANTICS,  
                                NULL);

    if (file == INVALID_HANDLE_VALUE)
    {
        return ConvertWinError(GetLastError());
    }

    FILETIME ft;
    SYSTEMTIME st;
    PRExplodedTime pret;

    
    PR_ExplodeTime(aLastModifiedTime * PR_USEC_PER_MSEC, PR_GMTParameters, &pret);
    st.wYear            = pret.tm_year;
    st.wMonth           = pret.tm_month + 1; 
    st.wDayOfWeek       = pret.tm_wday;
    st.wDay             = pret.tm_mday;
    st.wHour            = pret.tm_hour;
    st.wMinute          = pret.tm_min;
    st.wSecond          = pret.tm_sec;
    st.wMilliseconds    = pret.tm_usec/1000;

    nsresult rv = NS_OK;
    
    if (!(SystemTimeToFileTime(&st, &ft) != 0 &&
          SetFileTime(file, NULL, &ft, &ft) != 0))
    {
      rv = ConvertWinError(GetLastError());
    }

    CloseHandle(file);
    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetPermissions(uint32_t *aPermissions)
{
    NS_ENSURE_ARG(aPermissions);

    
    
    
    
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    bool isWritable, isExecutable;
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
nsLocalFile::GetPermissionsOfLink(uint32_t *aPermissions)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aPermissions);

    
    
    

    DWORD word = ::GetFileAttributesW(mWorkingPath.get());
    if (word == INVALID_FILE_ATTRIBUTES)
        return NS_ERROR_FILE_INVALID_PATH;

    bool isWritable = !(word & FILE_ATTRIBUTE_READONLY);
    *aPermissions = PR_IRUSR|PR_IRGRP|PR_IROTH;         
    if (isWritable)
        *aPermissions |= PR_IWUSR|PR_IWGRP|PR_IWOTH;    

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::SetPermissions(uint32_t aPermissions)
{
    
    CHECK_mWorkingPath();

    
    
    
    
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    
    int mode = 0;
    if (aPermissions & (PR_IRUSR|PR_IRGRP|PR_IROTH))    
        mode |= _S_IREAD;
    if (aPermissions & (PR_IWUSR|PR_IWGRP|PR_IWOTH))    
        mode |= _S_IWRITE;

    if (_wchmod(mResolvedPath.get(), mode) == -1)
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetPermissionsOfLink(uint32_t aPermissions)
{
    
    

    
    int mode = 0;
    if (aPermissions & (PR_IRUSR|PR_IRGRP|PR_IROTH))    
        mode |= _S_IREAD;
    if (aPermissions & (PR_IWUSR|PR_IWGRP|PR_IWOTH))    
        mode |= _S_IWRITE;

    if (_wchmod(mWorkingPath.get(), mode) == -1)
        return NS_ERROR_FAILURE;

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetFileSize(int64_t *aFileSize)
{
    NS_ENSURE_ARG(aFileSize);

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    *aFileSize = mFileInfo64.size;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetFileSizeOfLink(int64_t *aFileSize)
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
nsLocalFile::SetFileSize(int64_t aFileSize)
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
nsLocalFile::GetDiskSpaceAvailable(int64_t *aDiskSpaceAvailable)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aDiskSpaceAvailable);

    ResolveAndStat();

    if (mFileInfo64.type == PR_FILE_FILE) {
      
      nsCOMPtr<nsIFile> parent;
      if (NS_SUCCEEDED(GetParent(getter_AddRefs(parent))) && parent) {
        return parent->GetDiskSpaceAvailable(aDiskSpaceAvailable);
      }
    }

    ULARGE_INTEGER liFreeBytesAvailableToCaller, liTotalNumberOfBytes;
    if (::GetDiskFreeSpaceExW(mResolvedPath.get(), &liFreeBytesAvailableToCaller, 
                              &liTotalNumberOfBytes, NULL))
    {
        *aDiskSpaceAvailable = liFreeBytesAvailableToCaller.QuadPart;
        return NS_OK;
    }
    *aDiskSpaceAvailable = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetParent(nsIFile * *aParent)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG_POINTER(aParent);

    
    if (mWorkingPath.Length() == 2) {
        *aParent = nullptr;
        return NS_OK;
    }

    int32_t offset = mWorkingPath.RFindChar(PRUnichar('\\'));
    
    
    
    
    if (offset == kNotFound)
      return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    
    if (offset == 1 && mWorkingPath[0] == L'\\') {
        *aParent = nullptr;
        return NS_OK;
    }

    nsAutoString parentPath(mWorkingPath);

    if (offset > 0)
        parentPath.Truncate(offset);
    else
        parentPath.AssignLiteral("\\\\.");

    nsCOMPtr<nsIFile> localFile;
    nsresult rv = NS_NewLocalFile(parentPath, mFollowSymlinks, getter_AddRefs(localFile));

    if (NS_SUCCEEDED(rv) && localFile) {
        return CallQueryInterface(localFile, aParent);
    }
    return rv;
}

NS_IMETHODIMP
nsLocalFile::Exists(bool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = false;

    MakeDirty();
    nsresult rv = ResolveAndStat();
    *_retval = NS_SUCCEEDED(rv) || rv == NS_ERROR_FILE_IS_LOCKED;

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsWritable(bool *aIsWritable)
{
    
    CHECK_mWorkingPath();

    
    
    nsresult rv = IsDirectory(aIsWritable);
    if (rv == NS_ERROR_FILE_ACCESS_DENIED) {
      *aIsWritable = true;
      return NS_OK;
    } else if (rv == NS_ERROR_FILE_IS_LOCKED) {
      
      
    } else if (NS_FAILED(rv)) {
        return rv;
    }
    if (*aIsWritable)
        return NS_OK;

    
    rv = HasFileAttribute(FILE_ATTRIBUTE_READONLY, aIsWritable);
    if (rv == NS_ERROR_FILE_ACCESS_DENIED) {
        *aIsWritable = false;
        return NS_OK;
    } else if (rv == NS_ERROR_FILE_IS_LOCKED) {
      
      
    } else if (NS_FAILED(rv)) {
        return rv;
    }
    *aIsWritable = !*aIsWritable;

    
    
    if (*aIsWritable) {
        PRFileDesc* file;
        rv = OpenFile(mResolvedPath, PR_WRONLY, 0, &file);
        if (NS_SUCCEEDED(rv)) {
            PR_Close(file);
        } else if (rv == NS_ERROR_FILE_ACCESS_DENIED) {
          *aIsWritable = false;
        } else if (rv == NS_ERROR_FILE_IS_LOCKED) {
            
            
            *aIsWritable = true; 
        } else {
            return rv;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsReadable(bool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = false;

    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv))
        return rv;

    *_retval = true;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsExecutable(bool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = false;
    
    nsresult rv;

    
    bool isFile;
    rv = IsFile(&isFile);
    if (NS_FAILED(rv))
        return rv;
    if (!isFile)
        return NS_OK;

    
    bool symLink;
    rv = IsSymlink(&symLink);
    if (NS_FAILED(rv))
        return rv;

    nsAutoString path;
    if (symLink)
        GetTarget(path);
    else
        GetPath(path);

    
    int32_t filePathLen = path.Length() - 1;
    while(filePathLen > 0 && (path[filePathLen] == L' ' || path[filePathLen] == L'.'))
    {
        path.Truncate(filePathLen--);
    } 

    
    int32_t dotIdx = path.RFindChar(PRUnichar('.'));
    if ( dotIdx != kNotFound ) {
        
        PRUnichar *p = path.BeginWriting();
        for( p+= dotIdx + 1; *p; p++ )
            *p +=  (*p >= L'A' && *p <= L'Z') ? 'a' - 'A' : 0; 
        
        
        static const char * const executableExts[] = {
            "ad",
            "ade",         
            "adp",
            "air",         
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
            "jar",         
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
        for ( size_t i = 0; i < ArrayLength(executableExts); i++ ) {
            if ( ext.EqualsASCII(executableExts[i])) {
                
                *_retval = true;
                break;
            }
        }
    }

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsDirectory(bool *_retval)
{
    return HasFileAttribute(FILE_ATTRIBUTE_DIRECTORY, _retval);
}

NS_IMETHODIMP
nsLocalFile::IsFile(bool *_retval)
{
    nsresult rv = HasFileAttribute(FILE_ATTRIBUTE_DIRECTORY, _retval);
    if (NS_SUCCEEDED(rv)) {
        *_retval = !*_retval;
    }
    return rv;
}

NS_IMETHODIMP
nsLocalFile::IsHidden(bool *_retval)
{
    return HasFileAttribute(FILE_ATTRIBUTE_HIDDEN, _retval);
}

nsresult
nsLocalFile::HasFileAttribute(DWORD fileAttrib, bool *_retval)
{
    NS_ENSURE_ARG(_retval);

    nsresult rv = Resolve();
    if (NS_FAILED(rv)) {
        return rv;
    }

    DWORD attributes = GetFileAttributesW(mResolvedPath.get());
    if (INVALID_FILE_ATTRIBUTES == attributes) {
        return ConvertWinError(GetLastError());
    }

    *_retval = ((attributes & fileAttrib) != 0);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSymlink(bool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);

    
    if (!IsShortcutPath(mWorkingPath)) {
        *_retval = false;
        return NS_OK;
    }

    
    nsresult rv = ResolveAndStat();
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    
    
    
    *_retval = true;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSpecial(bool *_retval)
{
    return HasFileAttribute(FILE_ATTRIBUTE_SYSTEM, _retval);
}

NS_IMETHODIMP
nsLocalFile::Equals(nsIFile *inFile, bool *_retval)
{
    NS_ENSURE_ARG(inFile);
    NS_ENSURE_ARG(_retval);

    EnsureShortPath();

    nsCOMPtr<nsILocalFileWin> lf(do_QueryInterface(inFile));
    if (!lf) {
        *_retval = false;
        return NS_OK;
    }

    nsAutoString inFilePath;
    lf->GetCanonicalPath(inFilePath);

    
    *_retval = _wcsicmp(mShortWorkingPath.get(), inFilePath.get()) == 0; 

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::Contains(nsIFile *inFile, bool recur, bool *_retval)
{
    
    CHECK_mWorkingPath();

    *_retval = false;

    nsAutoString myFilePath;
    if (NS_FAILED(GetTarget(myFilePath)))
        GetPath(myFilePath);

    uint32_t myFilePathLen = myFilePath.Length();

    nsAutoString inFilePath;
    if (NS_FAILED(inFile->GetTarget(inFilePath)))
        inFile->GetPath(inFilePath);

    
    if (inFilePath.Length() >= myFilePathLen && inFilePath[myFilePathLen] == L'\\')
    {
        if (_wcsnicmp(myFilePath.get(), inFilePath.get(), myFilePathLen) == 0)
        {
            *_retval = true;
        }

    }

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetTarget(nsAString &_retval)
{
    _retval.Truncate();
#if STRICT_FAKE_SYMLINKS
    bool symLink;

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
nsLocalFile::GetFollowLinks(bool *aFollowLinks)
{
    *aFollowLinks = mFollowSymlinks;
    return NS_OK;
}
NS_IMETHODIMP
nsLocalFile::SetFollowLinks(bool aFollowLinks)
{
    MakeDirty();
    mFollowSymlinks = aFollowLinks;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetDirectoryEntries(nsISimpleEnumerator * *entries)
{
    nsresult rv;

    *entries = nullptr;
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

    nsDirEnumerator* dirEnum = new nsDirEnumerator();
    if (dirEnum == nullptr)
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


NS_IMETHODIMP
nsLocalFile::GetFileAttributesWin(uint32_t *aAttribs)
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
nsLocalFile::SetFileAttributesWin(uint32_t aAttribs)
{
    DWORD dwAttrs = GetFileAttributesW(mWorkingPath.get());
    if (dwAttrs == INVALID_FILE_ATTRIBUTES)
      return NS_ERROR_FILE_INVALID_PATH;

    if (aAttribs & WFA_SEARCH_INDEXED) {
        dwAttrs &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    } else {
        dwAttrs |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    }

    if (aAttribs & WFA_READONLY) {
      dwAttrs |= FILE_ATTRIBUTE_READONLY;
    } else if ((aAttribs & WFA_READWRITE) &&
               (dwAttrs & FILE_ATTRIBUTE_READONLY)) {
      dwAttrs &= ~FILE_ATTRIBUTE_READONLY;
    }

    if (SetFileAttributesW(mWorkingPath.get(), dwAttrs) == 0)
      return NS_ERROR_FAILURE;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::Reveal()
{
    
    MOZ_ASSERT(NS_IsMainThread()); 

    
    nsresult rv = Resolve();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND) {
        return rv;
    }

    
    nsCOMPtr<nsIThreadManager> tm = do_GetService(NS_THREADMANAGER_CONTRACTID);
    nsCOMPtr<nsIThread> mythread;
    rv = tm->NewThread(0, 0, getter_AddRefs(mythread));
    if (NS_FAILED(rv)) {
        return rv;
    }

    nsCOMPtr<nsIRunnable> runnable = 
        new AsyncLocalFileWinOperation(AsyncLocalFileWinOperation::RevealOp,
                                       mResolvedPath);

    
    
    mythread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Launch()
{
    
    MOZ_ASSERT(NS_IsMainThread()); 

    
    nsresult rv = Resolve();
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIThreadManager> tm = do_GetService(NS_THREADMANAGER_CONTRACTID);
    nsCOMPtr<nsIThread> mythread;
    rv = tm->NewThread(0, 0, getter_AddRefs(mythread));
    if (NS_FAILED(rv)) {
        return rv;
    }

    nsCOMPtr<nsIRunnable> runnable = 
        new AsyncLocalFileWinOperation(AsyncLocalFileWinOperation::LaunchOp,
                                       mResolvedPath);

    
    
    mythread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    return NS_OK;
}

nsresult
NS_NewLocalFile(const nsAString &path, bool followLinks, nsIFile* *result)
{
    nsLocalFile* file = new nsLocalFile();
    if (file == nullptr)
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
NS_NewNativeLocalFile(const nsACString &path, bool followLinks, nsIFile* *result)
{
    nsAutoString buf;
    nsresult rv = NS_CopyNativeToUnicode(path, buf);
    if (NS_FAILED(rv)) {
        *result = nullptr;
        return rv;
    }
    return NS_NewLocalFile(buf, followLinks, result);
}

void
nsLocalFile::EnsureShortPath()
{
    if (!mShortWorkingPath.IsEmpty())
        return;

    WCHAR shortPath[MAX_PATH + 1];
    DWORD lengthNeeded = ::GetShortPathNameW(mWorkingPath.get(), shortPath,
                                             ArrayLength(shortPath));
    
    
    
    if (lengthNeeded != 0 && lengthNeeded < ArrayLength(shortPath))
        mShortWorkingPath.Assign(shortPath);
    else
        mShortWorkingPath.Assign(mWorkingPath);
}



NS_IMETHODIMP
nsLocalFile::Equals(nsIHashable* aOther, bool *aResult)
{
    nsCOMPtr<nsIFile> otherfile(do_QueryInterface(aOther));
    if (!otherfile) {
        *aResult = false;
        return NS_OK;
    }

    return Equals(otherfile, aResult);
}

NS_IMETHODIMP
nsLocalFile::GetHashCode(uint32_t *aResult)
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

NS_IMETHODIMP nsDriveEnumerator::HasMoreElements(bool *aHasMore)
{
    *aHasMore = *mStartOfCurrentDrive != L'\0';
    return NS_OK;
}

NS_IMETHODIMP nsDriveEnumerator::GetNext(nsISupports **aNext)
{
    



    if (*mStartOfCurrentDrive == L'\0') {
        *aNext = nullptr;
        return NS_OK;
    }

    nsAString::const_iterator driveEnd = mStartOfCurrentDrive;
    FindCharInReadable(L'\0', driveEnd, mEndOfDrivesString);
    nsString drive(Substring(mStartOfCurrentDrive, driveEnd));
    mStartOfCurrentDrive = ++driveEnd;

    nsIFile *file;
    nsresult rv = NS_NewLocalFile(drive, false, &file);

    *aNext = file;
    return rv;
}
