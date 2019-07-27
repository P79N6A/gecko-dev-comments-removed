





#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/WindowsVersion.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"

#include "nsLocalFile.h"
#include "nsIDirectoryEnumerator.h"
#include "nsNativeCharsetUtils.h"

#include "nsISimpleEnumerator.h"
#include "nsIComponentManager.h"
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
#include "prlink.h"

#include "mozilla/Mutex.h"
#include "SpecialSystemDirectory.h"

#include "nsTraceRefcnt.h"
#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

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

  NS_IMETHOD Run()
  {
    
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
                             const nsAString& aResolvedPath) :
    mOperation(aOperation),
    mResolvedPath(aResolvedPath)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread(),
               "AsyncLocalFileWinOperation should not be run on the main thread!");

    CoInitialize(nullptr);
    switch (mOperation) {
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
      
      ITEMIDLIST* dir =
        static_cast<ITEMIDLIST*>(ILCreateFromPathW(mResolvedPath.get()));
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

      
      ITEMIDLIST* dir =
        static_cast<ITEMIDLIST*>(ILCreateFromPathW(parentDirectoryPath));
      if (!dir) {
        return NS_ERROR_FAILURE;
      }

      
      ITEMIDLIST* item =
        static_cast<ITEMIDLIST*>(ILCreateFromPathW(mResolvedPath.get()));
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
    seinfo.hwnd   = nullptr;
    seinfo.lpVerb = nullptr;
    seinfo.lpFile = mResolvedPath.get();
    seinfo.lpParameters =  nullptr;
    seinfo.lpDirectory  = nullptr;
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
      shellArg.AssignLiteral(MOZ_UTF16("shell32.dll,OpenAs_RunDLL "));
      shellArg.Append(mResolvedPath);
      seinfo.lpFile = L"RUNDLL32.EXE";
      seinfo.lpParameters = shellArg.get();
      if (ShellExecuteExW(&seinfo)) {
        return NS_OK;
      }
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
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR
  nsresult Init();
private:
  virtual ~nsDriveEnumerator();

  




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
  nsresult Resolve(const WCHAR* aIn, WCHAR* aOut);
  nsresult SetShortcut(bool aUpdateExisting,
                       const WCHAR* aShortcutPath,
                       const WCHAR* aTargetPath,
                       const WCHAR* aWorkingDir,
                       const WCHAR* aArgs,
                       const WCHAR* aDescription,
                       const WCHAR* aIconFile,
                       int32_t aIconIndex);

private:
  Mutex                  mLock;
  nsRefPtr<IPersistFile> mPersistFile;
  nsRefPtr<IShellLinkW>  mShellLink;
};

ShortcutResolver::ShortcutResolver() :
  mLock("ShortcutResolver.mLock")
{
  CoInitialize(nullptr);
}

ShortcutResolver::~ShortcutResolver()
{
  CoUninitialize();
}

nsresult
ShortcutResolver::Init()
{
  
  if (FAILED(CoCreateInstance(CLSID_ShellLink,
                              nullptr,
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
ShortcutResolver::Resolve(const WCHAR* aIn, WCHAR* aOut)
{
  if (!mShellLink) {
    return NS_ERROR_FAILURE;
  }

  MutexAutoLock lock(mLock);

  if (FAILED(mPersistFile->Load(aIn, STGM_READ)) ||
      FAILED(mShellLink->Resolve(nullptr, SLR_NO_UI)) ||
      FAILED(mShellLink->GetPath(aOut, MAX_PATH, nullptr, SLGP_UNCPRIORITY))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
ShortcutResolver::SetShortcut(bool aUpdateExisting,
                              const WCHAR* aShortcutPath,
                              const WCHAR* aTargetPath,
                              const WCHAR* aWorkingDir,
                              const WCHAR* aArgs,
                              const WCHAR* aDescription,
                              const WCHAR* aIconPath,
                              int32_t aIconIndex)
{
  if (!mShellLink) {
    return NS_ERROR_FAILURE;
  }

  if (!aShortcutPath) {
    return NS_ERROR_FAILURE;
  }

  MutexAutoLock lock(mLock);

  if (aUpdateExisting) {
    if (FAILED(mPersistFile->Load(aShortcutPath, STGM_READWRITE))) {
      return NS_ERROR_FAILURE;
    }
  } else {
    if (!aTargetPath) {
      return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
    }

    
    
    if (FAILED(mShellLink->SetWorkingDirectory(L"")) ||
        FAILED(mShellLink->SetArguments(L"")) ||
        FAILED(mShellLink->SetDescription(L"")) ||
        FAILED(mShellLink->SetIconLocation(L"", 0))) {
      return NS_ERROR_FAILURE;
    }
  }

  if (aTargetPath && FAILED(mShellLink->SetPath(aTargetPath))) {
    return NS_ERROR_FAILURE;
  }

  if (aWorkingDir && FAILED(mShellLink->SetWorkingDirectory(aWorkingDir))) {
    return NS_ERROR_FAILURE;
  }

  if (aArgs && FAILED(mShellLink->SetArguments(aArgs))) {
    return NS_ERROR_FAILURE;
  }

  if (aDescription && FAILED(mShellLink->SetDescription(aDescription))) {
    return NS_ERROR_FAILURE;
  }

  if (aIconPath && FAILED(mShellLink->SetIconLocation(aIconPath, aIconIndex))) {
    return NS_ERROR_FAILURE;
  }

  if (FAILED(mPersistFile->Save(aShortcutPath,
                                TRUE))) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

static ShortcutResolver* gResolver = nullptr;

static nsresult
NS_CreateShortcutResolver()
{
  gResolver = new ShortcutResolver();
  return gResolver->Init();
}

static void
NS_DestroyShortcutResolver()
{
  delete gResolver;
  gResolver = nullptr;
}








static nsresult
ConvertWinError(DWORD aWinErr)
{
  nsresult rv;

  switch (aWinErr) {
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
  if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
    li.QuadPart = -1;
  }

  return li.QuadPart;
}

static bool
IsShortcutPath(const nsAString& aPath)
{
  
  
  
  MOZ_ASSERT(!aPath.IsEmpty(), "don't pass an empty string");
  int32_t len = aPath.Length();
  return len >= 4 && (StringTail(aPath, 4).LowerCaseEqualsASCII(".lnk"));
}














typedef enum
{
  _PR_TRI_TRUE = 1,
  _PR_TRI_FALSE = 0,
  _PR_TRI_UNKNOWN = -1
} _PRTriStateBool;

struct _MDFileDesc
{
  PROsfd osfd;
};

struct PRFilePrivate
{
  int32_t state;
  bool nonblocking;
  _PRTriStateBool inheritable;
  PRFileDesc* next;
  int lockCount;      


  bool    appendMode;
  _MDFileDesc md;
};











nsresult
OpenFile(const nsAFlatString& aName,
         int aOsflags,
         int aMode,
         bool aShareDelete,
         PRFileDesc** aFd)
{
  int32_t access = 0;

  int32_t shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  int32_t disposition = 0;
  int32_t attributes = 0;

  if (aShareDelete) {
    shareMode |= FILE_SHARE_DELETE;
  }

  if (aOsflags & PR_SYNC) {
    attributes = FILE_FLAG_WRITE_THROUGH;
  }
  if (aOsflags & PR_RDONLY || aOsflags & PR_RDWR) {
    access |= GENERIC_READ;
  }
  if (aOsflags & PR_WRONLY || aOsflags & PR_RDWR) {
    access |= GENERIC_WRITE;
  }

  if (aOsflags & PR_CREATE_FILE && aOsflags & PR_EXCL) {
    disposition = CREATE_NEW;
  } else if (aOsflags & PR_CREATE_FILE) {
    if (aOsflags & PR_TRUNCATE) {
      disposition = CREATE_ALWAYS;
    } else {
      disposition = OPEN_ALWAYS;
    }
  } else {
    if (aOsflags & PR_TRUNCATE) {
      disposition = TRUNCATE_EXISTING;
    } else {
      disposition = OPEN_EXISTING;
    }
  }

  if (aOsflags & nsIFile::DELETE_ON_CLOSE) {
    attributes |= FILE_FLAG_DELETE_ON_CLOSE;
  }

  if (aOsflags & nsIFile::OS_READAHEAD) {
    attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
  }

  
  
  
  if (!(aMode & (PR_IWUSR | PR_IWGRP | PR_IWOTH)) &&
      disposition != OPEN_EXISTING) {
    attributes |= FILE_ATTRIBUTE_READONLY;
  }

  HANDLE file = ::CreateFileW(aName.get(), access, shareMode,
                              nullptr, disposition, attributes, nullptr);

  if (file == INVALID_HANDLE_VALUE) {
    *aFd = nullptr;
    return ConvertWinError(GetLastError());
  }

  *aFd = PR_ImportFile((PROsfd) file);
  if (*aFd) {
    
    
    (*aFd)->secret->appendMode = (PR_APPEND & aOsflags) ? true : false;
    return NS_OK;
  }

  nsresult rv = NS_ErrorAccordingToNSPR();

  CloseHandle(file);

  return rv;
}



static void
FileTimeToPRTime(const FILETIME* aFiletime, PRTime* aPrtm)
{
#ifdef __GNUC__
  const PRTime _pr_filetime_offset = 116444736000000000LL;
#else
  const PRTime _pr_filetime_offset = 116444736000000000i64;
#endif

  PR_ASSERT(sizeof(FILETIME) == sizeof(PRTime));
  ::CopyMemory(aPrtm, aFiletime, sizeof(PRTime));
#ifdef __GNUC__
  *aPrtm = (*aPrtm - _pr_filetime_offset) / 10LL;
#else
  *aPrtm = (*aPrtm - _pr_filetime_offset) / 10i64;
#endif
}



static nsresult
GetFileInfo(const nsAFlatString& aName, PRFileInfo64* aInfo)
{
  WIN32_FILE_ATTRIBUTE_DATA fileData;

  if (aName.IsEmpty() || aName.FindCharInSet(MOZ_UTF16("?*")) != kNotFound) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!::GetFileAttributesExW(aName.get(), GetFileExInfoStandard, &fileData)) {
    return ConvertWinError(GetLastError());
  }

  if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    aInfo->type = PR_FILE_DIRECTORY;
  } else {
    aInfo->type = PR_FILE_FILE;
  }

  aInfo->size = fileData.nFileSizeHigh;
  aInfo->size = (aInfo->size << 32) + fileData.nFileSizeLow;

  FileTimeToPRTime(&fileData.ftLastWriteTime, &aInfo->modifyTime);

  if (0 == fileData.ftCreationTime.dwLowDateTime &&
      0 == fileData.ftCreationTime.dwHighDateTime) {
    aInfo->creationTime = aInfo->modifyTime;
  } else {
    FileTimeToPRTime(&fileData.ftCreationTime, &aInfo->creationTime);
  }

  return NS_OK;
}

struct nsDir
{
  HANDLE handle;
  WIN32_FIND_DATAW data;
  bool firstEntry;
};

static nsresult
OpenDir(const nsAFlatString& aName, nsDir** aDir)
{
  if (NS_WARN_IF(!aDir)) {
    return NS_ERROR_INVALID_ARG;
  }

  *aDir = nullptr;
  if (aName.Length() + 3 >= MAX_PATH) {
    return NS_ERROR_FILE_NAME_TOO_LONG;
  }

  nsDir* d  = new nsDir();
  nsAutoString filename(aName);

  
  if (filename.Last() == L'/' || filename.Last() == L'\\') {
    filename.Append('*');
  } else {
    filename.AppendLiteral("\\*");
  }

  filename.ReplaceChar(L'/', L'\\');

  
  
  
  d->handle = ::FindFirstFileW(filename.get(), &(d->data));

  if (d->handle == INVALID_HANDLE_VALUE) {
    delete d;
    return ConvertWinError(GetLastError());
  }
  d->firstEntry = true;

  *aDir = d;
  return NS_OK;
}

static nsresult
ReadDir(nsDir* aDir, PRDirFlags aFlags, nsString& aName)
{
  aName.Truncate();
  if (NS_WARN_IF(!aDir)) {
    return NS_ERROR_INVALID_ARG;
  }

  while (1) {
    BOOL rv;
    if (aDir->firstEntry) {
      aDir->firstEntry = false;
      rv = 1;
    } else {
      rv = ::FindNextFileW(aDir->handle, &(aDir->data));
    }

    if (rv == 0) {
      break;
    }

    const wchar_t* fileName;
    nsString tmp;
    fileName = (aDir)->data.cFileName;

    if ((aFlags & PR_SKIP_DOT) &&
        (fileName[0] == L'.') && (fileName[1] == L'\0')) {
      continue;
    }
    if ((aFlags & PR_SKIP_DOT_DOT) &&
        (fileName[0] == L'.') && (fileName[1] == L'.') &&
        (fileName[2] == L'\0')) {
      continue;
    }

    DWORD attrib =  aDir->data.dwFileAttributes;
    if ((aFlags & PR_SKIP_HIDDEN) && (attrib & FILE_ATTRIBUTE_HIDDEN)) {
      continue;
    }

    if (fileName == tmp.get()) {
      aName = tmp;
    } else {
      aName = fileName;
    }
    return NS_OK;
  }

  DWORD err = GetLastError();
  return err == ERROR_NO_MORE_FILES ? NS_OK : ConvertWinError(err);
}

static nsresult
CloseDir(nsDir*& aDir)
{
  if (NS_WARN_IF(!aDir)) {
    return NS_ERROR_INVALID_ARG;
  }

  BOOL isOk = FindClose(aDir->handle);
  delete aDir;
  aDir = nullptr;
  return isOk ? NS_OK : ConvertWinError(GetLastError());
}





class nsDirEnumerator final
  : public nsISimpleEnumerator
  , public nsIDirectoryEnumerator
{
private:
  ~nsDirEnumerator()
  {
    Close();
  }

public:
  NS_DECL_ISUPPORTS

  nsDirEnumerator() : mDir(nullptr)
  {
  }

  nsresult Init(nsIFile* aParent)
  {
    nsAutoString filepath;
    aParent->GetTarget(filepath);

    if (filepath.IsEmpty()) {
      aParent->GetPath(filepath);
    }

    if (filepath.IsEmpty()) {
      return NS_ERROR_UNEXPECTED;
    }

    
    
    nsresult rv = OpenDir(filepath, &mDir);
    if (NS_FAILED(rv)) {
      return rv;
    }

    mParent = aParent;
    return NS_OK;
  }

  NS_IMETHOD HasMoreElements(bool* aResult)
  {
    nsresult rv;
    if (!mNext && mDir) {
      nsString name;
      rv = ReadDir(mDir, PR_SKIP_BOTH, name);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (name.IsEmpty()) {
        
        if (NS_FAILED(CloseDir(mDir))) {
          return NS_ERROR_FAILURE;
        }

        *aResult = false;
        return NS_OK;
      }

      nsCOMPtr<nsIFile> file;
      rv = mParent->Clone(getter_AddRefs(file));
      if (NS_FAILED(rv)) {
        return rv;
      }

      rv = file->Append(name);
      if (NS_FAILED(rv)) {
        return rv;
      }

      mNext = do_QueryInterface(file);
    }
    *aResult = mNext != nullptr;
    if (!*aResult) {
      Close();
    }
    return NS_OK;
  }

  NS_IMETHOD GetNext(nsISupports** aResult)
  {
    nsresult rv;
    bool hasMore;
    rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) {
      return rv;
    }

    *aResult = mNext;        
    NS_IF_ADDREF(*aResult);

    mNext = nullptr;
    return NS_OK;
  }

  NS_IMETHOD GetNextFile(nsIFile** aResult)
  {
    *aResult = nullptr;
    bool hasMore = false;
    nsresult rv = HasMoreElements(&hasMore);
    if (NS_FAILED(rv) || !hasMore) {
      return rv;
    }
    *aResult = mNext;
    NS_IF_ADDREF(*aResult);
    mNext = nullptr;
    return NS_OK;
  }

  NS_IMETHOD Close()
  {
    if (mDir) {
      nsresult rv = CloseDir(mDir);
      NS_ASSERTION(NS_SUCCEEDED(rv), "close failed");
      if (NS_FAILED(rv)) {
        return NS_ERROR_FAILURE;
      }
    }
    return NS_OK;
  }

protected:
  nsDir*             mDir;
  nsCOMPtr<nsIFile>  mParent;
  nsCOMPtr<nsIFile>  mNext;
};

NS_IMPL_ISUPPORTS(nsDirEnumerator, nsISimpleEnumerator, nsIDirectoryEnumerator)






nsLocalFile::nsLocalFile()
  : mDirty(true)
  , mResolveDirty(true)
  , mFollowSymlinks(false)
{
}

nsresult
nsLocalFile::nsLocalFileConstructor(nsISupports* aOuter, const nsIID& aIID,
                                    void** aInstancePtr)
{
  if (NS_WARN_IF(!aInstancePtr)) {
    return NS_ERROR_INVALID_ARG;
  }
  if (NS_WARN_IF(aOuter)) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsLocalFile* inst = new nsLocalFile();
  nsresult rv = inst->QueryInterface(aIID, aInstancePtr);
  if (NS_FAILED(rv)) {
    delete inst;
    return rv;
  }
  return NS_OK;
}






NS_IMPL_ISUPPORTS(nsLocalFile,
                  nsILocalFile,
                  nsIFile,
                  nsILocalFileWin,
                  nsIHashable)






nsLocalFile::nsLocalFile(const nsLocalFile& aOther)
  : mDirty(true)
  , mResolveDirty(true)
  , mFollowSymlinks(aOther.mFollowSymlinks)
  , mWorkingPath(aOther.mWorkingPath)
{
}



nsresult
nsLocalFile::ResolveShortcut()
{
  
  if (!gResolver) {
    return NS_ERROR_FAILURE;
  }

  mResolvedPath.SetLength(MAX_PATH);
  if (mResolvedPath.Length() != MAX_PATH) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  wchar_t* resolvedPath = wwc(mResolvedPath.BeginWriting());

  
  nsresult rv = gResolver->Resolve(mWorkingPath.get(), resolvedPath);

  size_t len = NS_FAILED(rv) ? 0 : wcslen(resolvedPath);
  mResolvedPath.SetLength(len);

  return rv;
}



nsresult
nsLocalFile::ResolveAndStat()
{
  
  if (!mDirty) {
    return NS_OK;
  }

  
  if (mWorkingPath.IsEmpty()) {
    return NS_ERROR_FILE_INVALID_PATH;
  }

  
  mResolvedPath.Assign(mWorkingPath);

  
  nsAutoString nsprPath(mWorkingPath.get());
  if (mWorkingPath.Length() == 2 && mWorkingPath.CharAt(1) == L':') {
    nsprPath.Append('\\');
  }

  
  
  nsresult rv = GetFileInfo(nsprPath, &mFileInfo64);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  if (!mFollowSymlinks ||
      mFileInfo64.type != PR_FILE_FILE ||
      !IsShortcutPath(mWorkingPath)) {
    mDirty = false;
    mResolveDirty = false;
    return NS_OK;
  }

  
  
  
  
  rv = ResolveShortcut();
  if (NS_FAILED(rv)) {
    mResolvedPath.Assign(mWorkingPath);
    return rv;
  }
  mResolveDirty = false;

  
  rv = GetFileInfo(mResolvedPath, &mFileInfo64);
  if (NS_FAILED(rv)) {
    return rv;
  }

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
nsLocalFile::Clone(nsIFile** aFile)
{
  
  *aFile = new nsLocalFile(*this);
  NS_ADDREF(*aFile);

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::InitWithFile(nsIFile* aFile)
{
  if (NS_WARN_IF(!aFile)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsAutoString path;
  aFile->GetPath(path);
  if (path.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }
  return InitWithPath(path);
}

NS_IMETHODIMP
nsLocalFile::InitWithPath(const nsAString& aFilePath)
{
  MakeDirty();

  nsAString::const_iterator begin, end;
  aFilePath.BeginReading(begin);
  aFilePath.EndReading(end);

  
  if (begin == end) {
    return NS_ERROR_FAILURE;
  }

  char16_t firstChar = *begin;
  char16_t secondChar = *(++begin);

  
  
  if (FindCharInReadable(L'/', begin, end)) {
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }

  if (secondChar != L':' && (secondChar != L'\\' || firstChar != L'\\')) {
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }

  if (secondChar == L':') {
    
    
    if (PathGetDriveNumberW(aFilePath.Data()) == -1) {
      return NS_ERROR_FILE_UNRECOGNIZED_PATH;
    }
  }

  mWorkingPath = aFilePath;
  
  if (mWorkingPath.Last() == L'\\') {
    mWorkingPath.Truncate(mWorkingPath.Length() - 1);
  }

  return NS_OK;

}

NS_IMETHODIMP
nsLocalFile::OpenNSPRFileDesc(int32_t aFlags, int32_t aMode,
                              PRFileDesc** aResult)
{
  nsresult rv = OpenNSPRFileDescMaybeShareDelete(aFlags, aMode, false, aResult);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::OpenANSIFileDesc(const char* aMode, FILE** aResult)
{
  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND) {
    return rv;
  }

  *aResult = _wfopen(mResolvedPath.get(), NS_ConvertASCIItoUTF16(aMode).get());
  if (*aResult) {
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsLocalFile::Create(uint32_t aType, uint32_t aAttributes)
{
  if (aType != NORMAL_FILE_TYPE && aType != DIRECTORY_TYPE) {
    return NS_ERROR_FILE_UNKNOWN_TYPE;
  }

  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND) {
    return rv;
  }

  
  
  
  
  
  
  
  
  
  
  
  

  wchar_t* path = wwc(mResolvedPath.BeginWriting());

  if (path[0] == L'\\' && path[1] == L'\\') {
    
    path = wcschr(path + 2, L'\\');
    if (!path) {
      return NS_ERROR_FILE_INVALID_PATH;
    }
    ++path;
  }

  
  wchar_t* slash = wcschr(path, L'\\');

  nsresult directoryCreateError = NS_OK;
  if (slash) {
    
    ++slash;
    slash = wcschr(slash, L'\\');

    while (slash) {
      *slash = L'\0';

      if (!::CreateDirectoryW(mResolvedPath.get(), nullptr)) {
        rv = ConvertWinError(GetLastError());
        if (NS_ERROR_FILE_NOT_FOUND == rv &&
            NS_ERROR_FILE_ACCESS_DENIED == directoryCreateError) {
          
          return NS_ERROR_FILE_ACCESS_DENIED;
        }
        
        
        
        else if (rv != NS_ERROR_FILE_ALREADY_EXISTS &&
                 rv != NS_ERROR_FILE_ACCESS_DENIED) {
          return rv;
        }

        directoryCreateError = rv;
      }
      *slash = L'\\';
      ++slash;
      slash = wcschr(slash, L'\\');
    }
  }

  if (aType == NORMAL_FILE_TYPE) {
    PRFileDesc* file;
    rv = OpenFile(mResolvedPath,
                  PR_RDONLY | PR_CREATE_FILE | PR_APPEND | PR_EXCL,
                  aAttributes, false, &file);
    if (file) {
      PR_Close(file);
    }

    if (rv == NS_ERROR_FILE_ACCESS_DENIED) {
      
      bool isdir;
      if (NS_SUCCEEDED(IsDirectory(&isdir)) && isdir) {
        rv = NS_ERROR_FILE_ALREADY_EXISTS;
      }
    } else if (NS_ERROR_FILE_NOT_FOUND == rv &&
               NS_ERROR_FILE_ACCESS_DENIED == directoryCreateError) {
      
      return NS_ERROR_FILE_ACCESS_DENIED;
    }
    return rv;
  }

  if (aType == DIRECTORY_TYPE) {
    if (!::CreateDirectoryW(mResolvedPath.get(), nullptr)) {
      rv = ConvertWinError(GetLastError());
      if (NS_ERROR_FILE_NOT_FOUND == rv &&
          NS_ERROR_FILE_ACCESS_DENIED == directoryCreateError) {
        
        return NS_ERROR_FILE_ACCESS_DENIED;
      }
      return rv;
    }
    return NS_OK;
  }

  return NS_ERROR_FILE_UNKNOWN_TYPE;
}


NS_IMETHODIMP
nsLocalFile::Append(const nsAString& aNode)
{
  
  return AppendInternal(PromiseFlatString(aNode), false);
}

NS_IMETHODIMP
nsLocalFile::AppendRelativePath(const nsAString& aNode)
{
  
  return AppendInternal(PromiseFlatString(aNode), true);
}


nsresult
nsLocalFile::AppendInternal(const nsAFlatString& aNode,
                            bool aMultipleComponents)
{
  if (aNode.IsEmpty()) {
    return NS_OK;
  }

  
  if (aNode.First() == L'\\' ||               
      aNode.FindChar(L'/') != kNotFound ||    
      aNode.EqualsASCII("..")) {              
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }

  if (aMultipleComponents) {
    
    
    
    
    NS_NAMED_LITERAL_STRING(doubleDot, "\\..");
    nsAString::const_iterator start, end, offset;
    aNode.BeginReading(start);
    aNode.EndReading(end);
    offset = end;
    while (FindInReadable(doubleDot, start, offset)) {
      if (offset == end || *offset == L'\\') {
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;
      }
      start = offset;
      offset = end;
    }

    
    if (StringBeginsWith(aNode, NS_LITERAL_STRING("..\\"))) {
      return NS_ERROR_FILE_UNRECOGNIZED_PATH;
    }
  }
  
  else if (aNode.FindChar(L'\\') != kNotFound) {
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }

  MakeDirty();

  mWorkingPath.Append('\\');
  mWorkingPath.Append(aNode);

  return NS_OK;
}

nsresult
nsLocalFile::OpenNSPRFileDescMaybeShareDelete(int32_t aFlags,
                                              int32_t aMode,
                                              bool aShareDelete,
                                              PRFileDesc** aResult)
{
  nsresult rv = Resolve();
  if (NS_FAILED(rv)) {
    return rv;
  }

  return OpenFile(mResolvedPath, aFlags, aMode, aShareDelete, aResult);
}

#define TOUPPER(u) (((u) >= L'a' && (u) <= L'z') ? \
                    (u) - (L'a' - L'A') : (u))

NS_IMETHODIMP
nsLocalFile::Normalize()
{
  

  if (mWorkingPath.IsEmpty()) {
    return NS_OK;
  }

  nsAutoString path(mWorkingPath);

  
  
  
  
  
  int32_t rootIdx = 2;        
  if (path.First() == L'\\') {  
    rootIdx = path.FindChar(L'\\', 2);   
    if (rootIdx == kNotFound) {
      return NS_OK;  
    }
    rootIdx = path.FindChar(L'\\', rootIdx + 1);
    if (rootIdx == kNotFound) {
      return NS_OK;  
    }
  } else if (path.CharAt(rootIdx) != L'\\') {
    
    
    
    
    WCHAR cwd[MAX_PATH];
    WCHAR* pcwd = cwd;
    int drive = TOUPPER(path.First()) - 'A' + 1;
    















    if (!((1 << (drive - 1)) & _getdrives())) {
      return NS_ERROR_FILE_INVALID_PATH;
    }
    if (!_wgetdcwd(drive, pcwd, MAX_PATH)) {
      pcwd = _wgetdcwd(drive, 0, 0);
    }
    if (!pcwd) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    nsAutoString currentDir(pcwd);
    if (pcwd != cwd) {
      free(pcwd);
    }

    if (currentDir.Last() == '\\') {
      path.Replace(0, 2, currentDir);
    } else {
      path.Replace(0, 2, currentDir + NS_LITERAL_STRING("\\"));
    }
  }
  NS_POSTCONDITION(0 < rootIdx && rootIdx < (int32_t)path.Length(), "rootIdx is invalid");
  NS_POSTCONDITION(path.CharAt(rootIdx) == '\\', "rootIdx is invalid");

  
  if (rootIdx + 1 == (int32_t)path.Length()) {
    return NS_OK;
  }

  
  const char16_t* pathBuffer = path.get();  
  mWorkingPath.SetCapacity(path.Length());  
  mWorkingPath.Assign(pathBuffer, rootIdx);

  
  
  
  
  
  
  
  
  
  
  int32_t len, begin, end = rootIdx;
  while (end < (int32_t)path.Length()) {
    
    
    
    
    
    begin = end + 1;
    end = path.FindChar('\\', begin);
    if (end == kNotFound) {
      end = path.Length();
    }
    len = end - begin;

    
    if (len == 0) {
      continue;
    }

    
    if (pathBuffer[begin] == '.') {
      
      if (len == 1) {
        continue;
      }

      
      if (len >= 2 && pathBuffer[begin + 1] == L'.') {
        
        if (len == 2) {
          int32_t prev = mWorkingPath.RFindChar('\\');
          if (prev >= rootIdx) {
            mWorkingPath.Truncate(prev);
          }
          continue;
        }

        
        
        int idx = len - 1;
        for (; idx >= 2; --idx) {
          if (pathBuffer[begin + idx] != L'.') {
            break;
          }
        }

        
        
        if (idx < 2) {
          continue;
        }
      }
    }

    
    mWorkingPath.Append(pathBuffer + begin - 1, len + 1);
  }

  
  int32_t filePathLen = mWorkingPath.Length() - 1;
  while (filePathLen > 0 && (mWorkingPath[filePathLen] == L' ' ||
                             mWorkingPath[filePathLen] == L'.')) {
    mWorkingPath.Truncate(filePathLen--);
  }

  MakeDirty();
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetLeafName(nsAString& aLeafName)
{
  aLeafName.Truncate();

  if (mWorkingPath.IsEmpty()) {
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }

  int32_t offset = mWorkingPath.RFindChar(L'\\');

  
  if (offset == kNotFound) {
    aLeafName = mWorkingPath;
  } else {
    aLeafName = Substring(mWorkingPath, offset + 1);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetLeafName(const nsAString& aLeafName)
{
  MakeDirty();

  if (mWorkingPath.IsEmpty()) {
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }

  
  int32_t offset = mWorkingPath.RFindChar(L'\\');
  if (offset) {
    mWorkingPath.Truncate(offset + 1);
  }
  mWorkingPath.Append(aLeafName);

  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetPath(nsAString& aResult)
{
  aResult = mWorkingPath;
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetCanonicalPath(nsAString& aResult)
{
  EnsureShortPath();
  aResult.Assign(mShortWorkingPath);
  return NS_OK;
}

typedef struct
{
  WORD wLanguage;
  WORD wCodePage;
} LANGANDCODEPAGE;

NS_IMETHODIMP
nsLocalFile::GetVersionInfoField(const char* aField, nsAString& aResult)
{
  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = NS_ERROR_FAILURE;

  const WCHAR* path =
    mFollowSymlinks ? mResolvedPath.get() : mWorkingPath.get();

  DWORD dummy;
  DWORD size = ::GetFileVersionInfoSizeW(path, &dummy);
  if (!size) {
    return rv;
  }

  void* ver = moz_xcalloc(size, 1);
  if (::GetFileVersionInfoW(path, 0, size, ver)) {
    LANGANDCODEPAGE* translate = nullptr;
    UINT pageCount;
    BOOL queryResult = ::VerQueryValueW(ver, L"\\VarFileInfo\\Translation",
                                        (void**)&translate, &pageCount);
    if (queryResult && translate) {
      for (int32_t i = 0; i < 2; ++i) {
        wchar_t subBlock[MAX_PATH];
        _snwprintf(subBlock, MAX_PATH,
                   L"\\StringFileInfo\\%04x%04x\\%s",
                   (i == 0 ? translate[0].wLanguage : ::GetUserDefaultLangID()),
                   translate[0].wCodePage,
                   NS_ConvertASCIItoUTF16(
                     nsDependentCString(aField)).get());
        subBlock[MAX_PATH - 1] = 0;
        LPVOID value = nullptr;
        UINT size;
        queryResult = ::VerQueryValueW(ver, subBlock, &value, &size);
        if (queryResult && value) {
          aResult.Assign(static_cast<char16_t*>(value));
          if (!aResult.IsEmpty()) {
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
nsLocalFile::SetShortcut(nsIFile* aTargetFile,
                         nsIFile* aWorkingDir,
                         const char16_t* aArgs,
                         const char16_t* aDescription,
                         nsIFile* aIconFile,
                         int32_t aIconIndex)
{
  bool exists;
  nsresult rv = this->Exists(&exists);
  if (NS_FAILED(rv)) {
    return rv;
  }

  const WCHAR* targetFilePath = nullptr;
  const WCHAR* workingDirPath = nullptr;
  const WCHAR* iconFilePath = nullptr;

  nsAutoString targetFilePathAuto;
  if (aTargetFile) {
    rv = aTargetFile->GetPath(targetFilePathAuto);
    if (NS_FAILED(rv)) {
      return rv;
    }
    targetFilePath = targetFilePathAuto.get();
  }

  nsAutoString workingDirPathAuto;
  if (aWorkingDir) {
    rv = aWorkingDir->GetPath(workingDirPathAuto);
    if (NS_FAILED(rv)) {
      return rv;
    }
    workingDirPath = workingDirPathAuto.get();
  }

  nsAutoString iconPathAuto;
  if (aIconFile) {
    rv = aIconFile->GetPath(iconPathAuto);
    if (NS_FAILED(rv)) {
      return rv;
    }
    iconFilePath = iconPathAuto.get();
  }

  rv = gResolver->SetShortcut(exists,
                              mWorkingPath.get(),
                              targetFilePath,
                              workingDirPath,
                              char16ptr_t(aArgs),
                              char16ptr_t(aDescription),
                              iconFilePath,
                              iconFilePath ? aIconIndex : 0);
  if (targetFilePath && NS_SUCCEEDED(rv)) {
    MakeDirty();
  }

  return rv;
}

NS_IMETHODIMP
nsLocalFile::OpenNSPRFileDescShareDelete(int32_t aFlags,
                                         int32_t aMode,
                                         PRFileDesc** aResult)
{
  nsresult rv = OpenNSPRFileDescMaybeShareDelete(aFlags, aMode, true, aResult);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}










static bool
IsRemoteFilePath(LPCWSTR aPath, bool& aRemote)
{
  
  
  WCHAR dirPath[MAX_PATH + 1] = { 0 };
  wcsncpy(dirPath, aPath, MAX_PATH);
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
  aRemote = driveType == DRIVE_REMOTE;
  return true;
}

nsresult
nsLocalFile::CopySingleFile(nsIFile* aSourceFile, nsIFile* aDestParent,
                            const nsAString& aNewName, uint32_t aOptions)
{
  nsresult rv = NS_OK;
  nsAutoString filePath;

  bool move = aOptions & (Move | Rename);

  
  
  
  
  nsAutoString destPath;
  aDestParent->GetTarget(destPath);

  destPath.Append('\\');

  if (aNewName.IsEmpty()) {
    nsAutoString aFileName;
    aSourceFile->GetLeafName(aFileName);
    destPath.Append(aFileName);
  } else {
    destPath.Append(aNewName);
  }


  if (aOptions & FollowSymlinks) {
    rv = aSourceFile->GetTarget(filePath);
    if (filePath.IsEmpty()) {
      rv = aSourceFile->GetPath(filePath);
    }
  } else {
    rv = aSourceFile->GetPath(filePath);
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  
  
  
  
  int copyOK;
  DWORD dwCopyFlags = COPY_FILE_ALLOW_DECRYPTED_DESTINATION;
  if (IsVistaOrLater()) {
    bool path1Remote, path2Remote;
    if (!IsRemoteFilePath(filePath.get(), path1Remote) ||
        !IsRemoteFilePath(destPath.get(), path2Remote) ||
        path1Remote || path2Remote) {
      dwCopyFlags |= COPY_FILE_NO_BUFFERING;
    }
  }

  if (!move) {
    copyOK = ::CopyFileExW(filePath.get(), destPath.get(), nullptr,
                           nullptr, nullptr, dwCopyFlags);
  } else {
    copyOK = ::MoveFileExW(filePath.get(), destPath.get(),
                           MOVEFILE_REPLACE_EXISTING);

    
    
    if (!copyOK && GetLastError() == ERROR_NOT_SAME_DEVICE) {
      if (aOptions & Rename) {
        return NS_ERROR_FILE_ACCESS_DENIED;
      }
      copyOK = CopyFileExW(filePath.get(), destPath.get(), nullptr,
                           nullptr, nullptr, dwCopyFlags);

      if (copyOK) {
        DeleteFileW(filePath.get());
      }
    }
  }

  if (!copyOK) { 
    rv = ConvertWinError(GetLastError());
  } else if (move && !(aOptions & SkipNtfsAclReset)) {
    
    
    PACL pOldDACL = nullptr;
    PSECURITY_DESCRIPTOR pSD = nullptr;
    ::GetNamedSecurityInfoW((LPWSTR)destPath.get(), SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION,
                            nullptr, nullptr, &pOldDACL, nullptr, &pSD);
    if (pOldDACL)
      ::SetNamedSecurityInfoW((LPWSTR)destPath.get(), SE_FILE_OBJECT,
                              DACL_SECURITY_INFORMATION |
                              UNPROTECTED_DACL_SECURITY_INFORMATION,
                              nullptr, nullptr, pOldDACL, nullptr);
    if (pSD) {
      LocalFree((HLOCAL)pSD);
    }
  }

  return rv;
}

nsresult
nsLocalFile::CopyMove(nsIFile* aParentDir, const nsAString& aNewName,
                      uint32_t aOptions)
{
  bool move = aOptions & (Move | Rename);
  bool followSymlinks = aOptions & FollowSymlinks;

  nsCOMPtr<nsIFile> newParentDir = aParentDir;
  
  
  
  
  nsresult rv  = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!newParentDir) {
    
    if (aNewName.IsEmpty()) {
      return NS_ERROR_INVALID_ARG;
    }

    rv = GetParent(getter_AddRefs(newParentDir));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  if (!newParentDir) {
    return NS_ERROR_FILE_DESTINATION_NOT_DIR;
  }

  
  bool exists;
  newParentDir->Exists(&exists);
  if (!exists) {
    rv = newParentDir->Create(DIRECTORY_TYPE, 0644);  
    if (NS_FAILED(rv)) {
      return rv;
    }
  } else {
    bool isDir;
    newParentDir->IsDirectory(&isDir);
    if (!isDir) {
      if (followSymlinks) {
        bool isLink;
        newParentDir->IsSymlink(&isLink);
        if (isLink) {
          nsAutoString target;
          newParentDir->GetTarget(target);

          nsCOMPtr<nsIFile> realDest = new nsLocalFile();
          rv = realDest->InitWithPath(target);

          if (NS_FAILED(rv)) {
            return rv;
          }

          return CopyMove(realDest, aNewName, aOptions);
        }
      } else {
        return NS_ERROR_FILE_DESTINATION_NOT_DIR;
      }
    }
  }

  
  bool done = false;
  bool isDir;
  IsDirectory(&isDir);
  bool isSymlink;
  IsSymlink(&isSymlink);

  
  if (move || !isDir || (isSymlink && !followSymlinks)) {
    
    if (!aParentDir) {
      aOptions |= SkipNtfsAclReset;
    }
    rv = CopySingleFile(this, newParentDir, aNewName, aOptions);
    done = NS_SUCCEEDED(rv);
    
    
    if (!done && !(move && isDir)) {
      return rv;
    }
  }

  
  if (!done) {
    
    nsCOMPtr<nsIFile> target;
    rv = newParentDir->Clone(getter_AddRefs(target));

    if (NS_FAILED(rv)) {
      return rv;
    }

    nsAutoString allocatedNewName;
    if (aNewName.IsEmpty()) {
      bool isLink;
      IsSymlink(&isLink);
      if (isLink) {
        nsAutoString temp;
        GetTarget(temp);
        int32_t offset = temp.RFindChar(L'\\');
        if (offset == kNotFound) {
          allocatedNewName = temp;
        } else {
          allocatedNewName = Substring(temp, offset + 1);
        }
      } else {
        GetLeafName(allocatedNewName);
      }
    } else {
      allocatedNewName = aNewName;
    }

    rv = target->Append(allocatedNewName);
    if (NS_FAILED(rv)) {
      return rv;
    }

    allocatedNewName.Truncate();

    
    target->Exists(&exists);
    if (!exists) {
      
      rv = target->Create(DIRECTORY_TYPE, 0644);  
      if (NS_FAILED(rv)) {
        return rv;
      }
    } else {
      
      bool isWritable;

      target->IsWritable(&isWritable);
      if (!isWritable) {
        return NS_ERROR_FILE_ACCESS_DENIED;
      }

      nsCOMPtr<nsISimpleEnumerator> targetIterator;
      rv = target->GetDirectoryEntries(getter_AddRefs(targetIterator));
      if (NS_FAILED(rv)) {
        return rv;
      }

      bool more;
      targetIterator->HasMoreElements(&more);
      
      if (more) {
        return NS_ERROR_FILE_DIR_NOT_EMPTY;
      }
    }

    nsRefPtr<nsDirEnumerator> dirEnum = new nsDirEnumerator();

    rv = dirEnum->Init(this);
    if (NS_FAILED(rv)) {
      NS_WARNING("dirEnum initialization failed");
      return rv;
    }

    bool more = false;
    while (NS_SUCCEEDED(dirEnum->HasMoreElements(&more)) && more) {
      nsCOMPtr<nsISupports> item;
      nsCOMPtr<nsIFile> file;
      dirEnum->GetNext(getter_AddRefs(item));
      file = do_QueryInterface(item);
      if (file) {
        bool isDir, isLink;

        file->IsDirectory(&isDir);
        file->IsSymlink(&isLink);

        if (move) {
          if (followSymlinks) {
            return NS_ERROR_FAILURE;
          }

          rv = file->MoveTo(target, EmptyString());
          if (NS_FAILED(rv)) {
            return rv;
          }
        } else {
          if (followSymlinks) {
            rv = file->CopyToFollowingLinks(target, EmptyString());
          } else {
            rv = file->CopyTo(target, EmptyString());
          }
          if (NS_FAILED(rv)) {
            return rv;
          }
        }
      }
    }
    
    
    
    
    
    
    if (move) {
      rv = Remove(false );
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }


  
  if (move) {
    MakeDirty();

    nsAutoString newParentPath;
    newParentDir->GetPath(newParentPath);

    if (newParentPath.IsEmpty()) {
      return NS_ERROR_FAILURE;
    }

    if (aNewName.IsEmpty()) {
      nsAutoString aFileName;
      GetLeafName(aFileName);

      InitWithPath(newParentPath);
      Append(aFileName);
    } else {
      InitWithPath(newParentPath);
      Append(aNewName);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::CopyTo(nsIFile* aNewParentDir, const nsAString& aNewName)
{
  return CopyMove(aNewParentDir, aNewName, 0);
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinks(nsIFile* aNewParentDir,
                                  const nsAString& aNewName)
{
  return CopyMove(aNewParentDir, aNewName, FollowSymlinks);
}

NS_IMETHODIMP
nsLocalFile::MoveTo(nsIFile* aNewParentDir, const nsAString& aNewName)
{
  return CopyMove(aNewParentDir, aNewName, Move);
}

NS_IMETHODIMP
nsLocalFile::RenameTo(nsIFile* aNewParentDir, const nsAString& aNewName)
{
  nsCOMPtr<nsIFile> targetParentDir = aNewParentDir;
  
  
  
  
  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!targetParentDir) {
    
    if (aNewName.IsEmpty()) {
      return NS_ERROR_INVALID_ARG;
    }
    rv = GetParent(getter_AddRefs(targetParentDir));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  if (!targetParentDir) {
    return NS_ERROR_FILE_DESTINATION_NOT_DIR;
  }

  
  bool exists;
  targetParentDir->Exists(&exists);
  if (!exists) {
    rv = targetParentDir->Create(DIRECTORY_TYPE, 0644);
    if (NS_FAILED(rv)) {
      return rv;
    }
  } else {
    bool isDir;
    targetParentDir->IsDirectory(&isDir);
    if (!isDir) {
      return NS_ERROR_FILE_DESTINATION_NOT_DIR;
    }
  }

  uint32_t options = Rename;
  if (!aNewParentDir) {
    options |= SkipNtfsAclReset;
  }
  
  return CopySingleFile(this, targetParentDir, aNewName, options);
}

NS_IMETHODIMP
nsLocalFile::Load(PRLibrary** aResult)
{
  
  CHECK_mWorkingPath();

  bool isFile;
  nsresult rv = IsFile(&isFile);

  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!isFile) {
    return NS_ERROR_FILE_IS_DIRECTORY;
  }

#ifdef NS_BUILD_REFCNT_LOGGING
  nsTraceRefcnt::SetActivityIsLegal(false);
#endif

  PRLibSpec libSpec;
  libSpec.value.pathname_u = mResolvedPath.get();
  libSpec.type = PR_LibSpec_PathnameU;
  *aResult =  PR_LoadLibraryWithFlags(libSpec, 0);

#ifdef NS_BUILD_REFCNT_LOGGING
  nsTraceRefcnt::SetActivityIsLegal(true);
#endif

  if (*aResult) {
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsLocalFile::Remove(bool aRecursive)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  CHECK_mWorkingPath();

  bool isDir, isLink;
  nsresult rv;

  isDir = false;
  rv = IsSymlink(&isLink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  if (!isLink) {
    rv = IsDirectory(&isDir);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  if (isDir) {
    if (aRecursive) {
      nsRefPtr<nsDirEnumerator> dirEnum = new nsDirEnumerator();

      rv = dirEnum->Init(this);
      if (NS_FAILED(rv)) {
        return rv;
      }

      bool more = false;
      while (NS_SUCCEEDED(dirEnum->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsISupports> item;
        dirEnum->GetNext(getter_AddRefs(item));
        nsCOMPtr<nsIFile> file = do_QueryInterface(item);
        if (file) {
          file->Remove(aRecursive);
        }
      }
    }
    if (RemoveDirectoryW(mWorkingPath.get()) == 0) {
      return ConvertWinError(GetLastError());
    }
  } else {
    if (DeleteFileW(mWorkingPath.get()) == 0) {
      return ConvertWinError(GetLastError());
    }
  }

  MakeDirty();
  return rv;
}

NS_IMETHODIMP
nsLocalFile::GetLastModifiedTime(PRTime* aLastModifiedTime)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aLastModifiedTime)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  
  

  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  *aLastModifiedTime = mFileInfo64.modifyTime / PR_USEC_PER_MSEC;
  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetLastModifiedTimeOfLink(PRTime* aLastModifiedTime)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aLastModifiedTime)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  

  PRFileInfo64 info;
  nsresult rv = GetFileInfo(mWorkingPath, &info);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  *aLastModifiedTime = info.modifyTime / PR_USEC_PER_MSEC;
  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::SetLastModifiedTime(PRTime aLastModifiedTime)
{
  
  CHECK_mWorkingPath();

  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  

  rv = SetModDate(aLastModifiedTime, mResolvedPath.get());
  if (NS_SUCCEEDED(rv)) {
    MakeDirty();
  }

  return rv;
}


NS_IMETHODIMP
nsLocalFile::SetLastModifiedTimeOfLink(PRTime aLastModifiedTime)
{
  
  

  nsresult rv = SetModDate(aLastModifiedTime, mWorkingPath.get());
  if (NS_SUCCEEDED(rv)) {
    MakeDirty();
  }

  return rv;
}

nsresult
nsLocalFile::SetModDate(PRTime aLastModifiedTime, const wchar_t* aFilePath)
{
  
  
  HANDLE file = ::CreateFileW(aFilePath,         
                              GENERIC_WRITE,     
                              0,                 
                              nullptr,           
                              OPEN_EXISTING,     
                              FILE_FLAG_BACKUP_SEMANTICS,  
                              nullptr);

  if (file == INVALID_HANDLE_VALUE) {
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
  st.wMilliseconds    = pret.tm_usec / 1000;

  nsresult rv = NS_OK;
  
  if (!(SystemTimeToFileTime(&st, &ft) != 0 &&
        SetFileTime(file, nullptr, &ft, &ft) != 0)) {
    rv = ConvertWinError(GetLastError());
  }

  CloseHandle(file);
  return rv;
}

NS_IMETHODIMP
nsLocalFile::GetPermissions(uint32_t* aPermissions)
{
  if (NS_WARN_IF(!aPermissions)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  
  
  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  bool isWritable, isExecutable;
  IsWritable(&isWritable);
  IsExecutable(&isExecutable);

  *aPermissions = PR_IRUSR | PR_IRGRP | PR_IROTH;     
  if (isWritable) {
    *aPermissions |= PR_IWUSR | PR_IWGRP | PR_IWOTH;  
  }
  if (isExecutable) {
    *aPermissions |= PR_IXUSR | PR_IXGRP | PR_IXOTH;  
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPermissionsOfLink(uint32_t* aPermissions)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aPermissions)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  

  DWORD word = ::GetFileAttributesW(mWorkingPath.get());
  if (word == INVALID_FILE_ATTRIBUTES) {
    return NS_ERROR_FILE_INVALID_PATH;
  }

  bool isWritable = !(word & FILE_ATTRIBUTE_READONLY);
  *aPermissions = PR_IRUSR | PR_IRGRP | PR_IROTH;     
  if (isWritable) {
    *aPermissions |= PR_IWUSR | PR_IWGRP | PR_IWOTH;  
  }

  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::SetPermissions(uint32_t aPermissions)
{
  
  CHECK_mWorkingPath();

  
  
  
  
  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  int mode = 0;
  if (aPermissions & (PR_IRUSR | PR_IRGRP | PR_IROTH)) { 
    mode |= _S_IREAD;
  }
  if (aPermissions & (PR_IWUSR | PR_IWGRP | PR_IWOTH)) { 
    mode |= _S_IWRITE;
  }

  if (_wchmod(mResolvedPath.get(), mode) == -1) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetPermissionsOfLink(uint32_t aPermissions)
{
  
  

  
  int mode = 0;
  if (aPermissions & (PR_IRUSR | PR_IRGRP | PR_IROTH)) { 
    mode |= _S_IREAD;
  }
  if (aPermissions & (PR_IWUSR | PR_IWGRP | PR_IWOTH)) { 
    mode |= _S_IWRITE;
  }

  if (_wchmod(mWorkingPath.get(), mode) == -1) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetFileSize(int64_t* aFileSize)
{
  if (NS_WARN_IF(!aFileSize)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  *aFileSize = mFileInfo64.size;
  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetFileSizeOfLink(int64_t* aFileSize)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aFileSize)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  

  PRFileInfo64 info;
  if (NS_FAILED(GetFileInfo(mWorkingPath, &info))) {
    return NS_ERROR_FILE_INVALID_PATH;
  }

  *aFileSize = info.size;
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetFileSize(int64_t aFileSize)
{
  
  CHECK_mWorkingPath();

  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  HANDLE hFile = ::CreateFileW(mResolvedPath.get(),
                               GENERIC_WRITE,      
                               FILE_SHARE_READ,    
                               nullptr,            
                               OPEN_EXISTING,          
                               FILE_ATTRIBUTE_NORMAL,  
                               nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    return ConvertWinError(GetLastError());
  }

  
  
  rv = NS_ERROR_FAILURE;
  aFileSize = MyFileSeek64(hFile, aFileSize, FILE_BEGIN);
  if (aFileSize != -1 && SetEndOfFile(hFile)) {
    MakeDirty();
    rv = NS_OK;
  }

  CloseHandle(hFile);
  return rv;
}

NS_IMETHODIMP
nsLocalFile::GetDiskSpaceAvailable(int64_t* aDiskSpaceAvailable)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aDiskSpaceAvailable)) {
    return NS_ERROR_INVALID_ARG;
  }

  ResolveAndStat();

  if (mFileInfo64.type == PR_FILE_FILE) {
    
    nsCOMPtr<nsIFile> parent;
    if (NS_SUCCEEDED(GetParent(getter_AddRefs(parent))) && parent) {
      return parent->GetDiskSpaceAvailable(aDiskSpaceAvailable);
    }
  }

  ULARGE_INTEGER liFreeBytesAvailableToCaller, liTotalNumberOfBytes;
  if (::GetDiskFreeSpaceExW(mResolvedPath.get(), &liFreeBytesAvailableToCaller,
                            &liTotalNumberOfBytes, nullptr)) {
    *aDiskSpaceAvailable = liFreeBytesAvailableToCaller.QuadPart;
    return NS_OK;
  }
  *aDiskSpaceAvailable = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetParent(nsIFile** aParent)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aParent)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (mWorkingPath.Length() == 2) {
    *aParent = nullptr;
    return NS_OK;
  }

  int32_t offset = mWorkingPath.RFindChar(char16_t('\\'));
  
  
  
  
  if (offset == kNotFound) {
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }

  
  if (offset == 1 && mWorkingPath[0] == L'\\') {
    *aParent = nullptr;
    return NS_OK;
  }

  nsAutoString parentPath(mWorkingPath);

  if (offset > 0) {
    parentPath.Truncate(offset);
  } else {
    parentPath.AssignLiteral("\\\\.");
  }

  nsCOMPtr<nsIFile> localFile;
  nsresult rv = NS_NewLocalFile(parentPath, mFollowSymlinks,
                                getter_AddRefs(localFile));

  if (NS_FAILED(rv)) {
    return rv;
  }

  localFile.forget(aParent);
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Exists(bool* aResult)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }
  *aResult = false;

  MakeDirty();
  nsresult rv = ResolveAndStat();
  *aResult = NS_SUCCEEDED(rv) || rv == NS_ERROR_FILE_IS_LOCKED;

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsWritable(bool* aIsWritable)
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
  if (*aIsWritable) {
    return NS_OK;
  }

  
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
    rv = OpenFile(mResolvedPath, PR_WRONLY, 0, false, &file);
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
nsLocalFile::IsReadable(bool* aResult)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }
  *aResult = false;

  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  *aResult = true;
  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsExecutable(bool* aResult)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }
  *aResult = false;

  nsresult rv;

  
  bool isFile;
  rv = IsFile(&isFile);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!isFile) {
    return NS_OK;
  }

  
  bool symLink;
  rv = IsSymlink(&symLink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsAutoString path;
  if (symLink) {
    GetTarget(path);
  } else {
    GetPath(path);
  }

  
  int32_t filePathLen = path.Length() - 1;
  while (filePathLen > 0 && (path[filePathLen] == L' ' ||
                             path[filePathLen] == L'.')) {
    path.Truncate(filePathLen--);
  }

  
  int32_t dotIdx = path.RFindChar(char16_t('.'));
  if (dotIdx != kNotFound) {
    
    char16_t* p = path.BeginWriting();
    for (p += dotIdx + 1; *p; ++p) {
      *p += (*p >= L'A' && *p <= L'Z') ? 'a' - 'A' : 0;
    }

    
    static const char* const executableExts[] = {
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
      "wsh"
    };
    nsDependentSubstring ext = Substring(path, dotIdx + 1);
    for (size_t i = 0; i < ArrayLength(executableExts); ++i) {
      if (ext.EqualsASCII(executableExts[i])) {
        
        *aResult = true;
        break;
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsDirectory(bool* aResult)
{
  return HasFileAttribute(FILE_ATTRIBUTE_DIRECTORY, aResult);
}

NS_IMETHODIMP
nsLocalFile::IsFile(bool* aResult)
{
  nsresult rv = HasFileAttribute(FILE_ATTRIBUTE_DIRECTORY, aResult);
  if (NS_SUCCEEDED(rv)) {
    *aResult = !*aResult;
  }
  return rv;
}

NS_IMETHODIMP
nsLocalFile::IsHidden(bool* aResult)
{
  return HasFileAttribute(FILE_ATTRIBUTE_HIDDEN, aResult);
}

nsresult
nsLocalFile::HasFileAttribute(DWORD aFileAttrib, bool* aResult)
{
  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv = Resolve();
  if (NS_FAILED(rv)) {
    return rv;
  }

  DWORD attributes = GetFileAttributesW(mResolvedPath.get());
  if (INVALID_FILE_ATTRIBUTES == attributes) {
    return ConvertWinError(GetLastError());
  }

  *aResult = ((attributes & aFileAttrib) != 0);
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSymlink(bool* aResult)
{
  
  CHECK_mWorkingPath();

  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (!IsShortcutPath(mWorkingPath)) {
    *aResult = false;
    return NS_OK;
  }

  
  nsresult rv = ResolveAndStat();
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSpecial(bool* aResult)
{
  return HasFileAttribute(FILE_ATTRIBUTE_SYSTEM, aResult);
}

NS_IMETHODIMP
nsLocalFile::Equals(nsIFile* aInFile, bool* aResult)
{
  if (NS_WARN_IF(!aInFile)) {
    return NS_ERROR_INVALID_ARG;
  }
  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }

  EnsureShortPath();

  nsCOMPtr<nsILocalFileWin> lf(do_QueryInterface(aInFile));
  if (!lf) {
    *aResult = false;
    return NS_OK;
  }

  nsAutoString inFilePath;
  lf->GetCanonicalPath(inFilePath);

  
  *aResult = _wcsicmp(mShortWorkingPath.get(), inFilePath.get()) == 0;

  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::Contains(nsIFile* aInFile, bool* aResult)
{
  
  CHECK_mWorkingPath();

  *aResult = false;

  nsAutoString myFilePath;
  if (NS_FAILED(GetTarget(myFilePath))) {
    GetPath(myFilePath);
  }

  uint32_t myFilePathLen = myFilePath.Length();

  nsAutoString inFilePath;
  if (NS_FAILED(aInFile->GetTarget(inFilePath))) {
    aInFile->GetPath(inFilePath);
  }

  
  if (inFilePath.Length() >= myFilePathLen &&
      inFilePath[myFilePathLen] == L'\\') {
    if (_wcsnicmp(myFilePath.get(), inFilePath.get(), myFilePathLen) == 0) {
      *aResult = true;
    }

  }

  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetTarget(nsAString& aResult)
{
  aResult.Truncate();
#if STRICT_FAKE_SYMLINKS
  bool symLink;

  nsresult rv = IsSymlink(&symLink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!symLink) {
    return NS_ERROR_FILE_INVALID_PATH;
  }
#endif
  ResolveAndStat();

  aResult = mResolvedPath;
  return NS_OK;
}



NS_IMETHODIMP
nsLocalFile::GetFollowLinks(bool* aFollowLinks)
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
nsLocalFile::GetDirectoryEntries(nsISimpleEnumerator** aEntries)
{
  nsresult rv;

  *aEntries = nullptr;
  if (mWorkingPath.EqualsLiteral("\\\\.")) {
    nsDriveEnumerator* drives = new nsDriveEnumerator;
    NS_ADDREF(drives);
    rv = drives->Init();
    if (NS_FAILED(rv)) {
      NS_RELEASE(drives);
      return rv;
    }
    *aEntries = drives;
    return NS_OK;
  }

  nsRefPtr<nsDirEnumerator> dirEnum = new nsDirEnumerator();
  rv = dirEnum->Init(this);
  if (NS_FAILED(rv)) {
    return rv;
  }

  dirEnum.forget(aEntries);

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPersistentDescriptor(nsACString& aPersistentDescriptor)
{
  CopyUTF16toUTF8(mWorkingPath, aPersistentDescriptor);
  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetPersistentDescriptor(const nsACString& aPersistentDescriptor)
{
  if (IsUTF8(aPersistentDescriptor)) {
    return InitWithPath(NS_ConvertUTF8toUTF16(aPersistentDescriptor));
  } else {
    return InitWithNativePath(aPersistentDescriptor);
  }
}


NS_IMETHODIMP
nsLocalFile::GetFileAttributesWin(uint32_t* aAttribs)
{
  *aAttribs = 0;
  DWORD dwAttrs = GetFileAttributesW(mWorkingPath.get());
  if (dwAttrs == INVALID_FILE_ATTRIBUTES) {
    return NS_ERROR_FILE_INVALID_PATH;
  }

  if (!(dwAttrs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)) {
    *aAttribs |= WFA_SEARCH_INDEXED;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetFileAttributesWin(uint32_t aAttribs)
{
  DWORD dwAttrs = GetFileAttributesW(mWorkingPath.get());
  if (dwAttrs == INVALID_FILE_ATTRIBUTES) {
    return NS_ERROR_FILE_INVALID_PATH;
  }

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

  if (SetFileAttributesW(mWorkingPath.get(), dwAttrs) == 0) {
    return NS_ERROR_FAILURE;
  }
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
  if (NS_FAILED(rv)) {
    return rv;
  }

  
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
NS_NewLocalFile(const nsAString& aPath, bool aFollowLinks, nsIFile** aResult)
{
  nsLocalFile* file = new nsLocalFile();
  NS_ADDREF(file);

  file->SetFollowLinks(aFollowLinks);

  if (!aPath.IsEmpty()) {
    nsresult rv = file->InitWithPath(aPath);
    if (NS_FAILED(rv)) {
      NS_RELEASE(file);
      return rv;
    }
  }

  *aResult = file;
  return NS_OK;
}





NS_IMETHODIMP
nsLocalFile::InitWithNativePath(const nsACString& aFilePath)
{
  nsAutoString tmp;
  nsresult rv = NS_CopyNativeToUnicode(aFilePath, tmp);
  if (NS_SUCCEEDED(rv)) {
    return InitWithPath(tmp);
  }

  return rv;
}

NS_IMETHODIMP
nsLocalFile::AppendNative(const nsACString& aNode)
{
  nsAutoString tmp;
  nsresult rv = NS_CopyNativeToUnicode(aNode, tmp);
  if (NS_SUCCEEDED(rv)) {
    return Append(tmp);
  }

  return rv;
}

NS_IMETHODIMP
nsLocalFile::AppendRelativeNativePath(const nsACString& aNode)
{
  nsAutoString tmp;
  nsresult rv = NS_CopyNativeToUnicode(aNode, tmp);
  if (NS_SUCCEEDED(rv)) {
    return AppendRelativePath(tmp);
  }
  return rv;
}


NS_IMETHODIMP
nsLocalFile::GetNativeLeafName(nsACString& aLeafName)
{
  
  nsAutoString tmp;
  nsresult rv = GetLeafName(tmp);
  if (NS_SUCCEEDED(rv)) {
    rv = NS_CopyUnicodeToNative(tmp, aLeafName);
  }

  return rv;
}

NS_IMETHODIMP
nsLocalFile::SetNativeLeafName(const nsACString& aLeafName)
{
  nsAutoString tmp;
  nsresult rv = NS_CopyNativeToUnicode(aLeafName, tmp);
  if (NS_SUCCEEDED(rv)) {
    return SetLeafName(tmp);
  }

  return rv;
}


NS_IMETHODIMP
nsLocalFile::GetNativePath(nsACString& aResult)
{
  
  nsAutoString tmp;
  nsresult rv = GetPath(tmp);
  if (NS_SUCCEEDED(rv)) {
    rv = NS_CopyUnicodeToNative(tmp, aResult);
  }

  return rv;
}


NS_IMETHODIMP
nsLocalFile::GetNativeCanonicalPath(nsACString& aResult)
{
  NS_WARNING("This method is lossy. Use GetCanonicalPath !");
  EnsureShortPath();
  NS_CopyUnicodeToNative(mShortWorkingPath, aResult);
  return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::CopyToNative(nsIFile* aNewParentDir, const nsACString& aNewName)
{
  
  CHECK_mWorkingPath();

  if (aNewName.IsEmpty()) {
    return CopyTo(aNewParentDir, EmptyString());
  }

  nsAutoString tmp;
  nsresult rv = NS_CopyNativeToUnicode(aNewName, tmp);
  if (NS_SUCCEEDED(rv)) {
    return CopyTo(aNewParentDir, tmp);
  }

  return rv;
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinksNative(nsIFile* aNewParentDir,
                                        const nsACString& aNewName)
{
  if (aNewName.IsEmpty()) {
    return CopyToFollowingLinks(aNewParentDir, EmptyString());
  }

  nsAutoString tmp;
  nsresult rv = NS_CopyNativeToUnicode(aNewName, tmp);
  if (NS_SUCCEEDED(rv)) {
    return CopyToFollowingLinks(aNewParentDir, tmp);
  }

  return rv;
}

NS_IMETHODIMP
nsLocalFile::MoveToNative(nsIFile* aNewParentDir, const nsACString& aNewName)
{
  
  CHECK_mWorkingPath();

  if (aNewName.IsEmpty()) {
    return MoveTo(aNewParentDir, EmptyString());
  }

  nsAutoString tmp;
  nsresult rv = NS_CopyNativeToUnicode(aNewName, tmp);
  if (NS_SUCCEEDED(rv)) {
    return MoveTo(aNewParentDir, tmp);
  }

  return rv;
}

NS_IMETHODIMP
nsLocalFile::GetNativeTarget(nsACString& aResult)
{
  
  CHECK_mWorkingPath();

  NS_WARNING("This API is lossy. Use GetTarget !");
  nsAutoString tmp;
  nsresult rv = GetTarget(tmp);
  if (NS_SUCCEEDED(rv)) {
    rv = NS_CopyUnicodeToNative(tmp, aResult);
  }

  return rv;
}

nsresult
NS_NewNativeLocalFile(const nsACString& aPath, bool aFollowLinks,
                      nsIFile** aResult)
{
  nsAutoString buf;
  nsresult rv = NS_CopyNativeToUnicode(aPath, buf);
  if (NS_FAILED(rv)) {
    *aResult = nullptr;
    return rv;
  }
  return NS_NewLocalFile(buf, aFollowLinks, aResult);
}

void
nsLocalFile::EnsureShortPath()
{
  if (!mShortWorkingPath.IsEmpty()) {
    return;
  }

  WCHAR shortPath[MAX_PATH + 1];
  DWORD lengthNeeded = ::GetShortPathNameW(mWorkingPath.get(), shortPath,
                                           ArrayLength(shortPath));
  
  
  
  if (lengthNeeded != 0 && lengthNeeded < ArrayLength(shortPath)) {
    mShortWorkingPath.Assign(shortPath);
  } else {
    mShortWorkingPath.Assign(mWorkingPath);
  }
}



NS_IMETHODIMP
nsLocalFile::Equals(nsIHashable* aOther, bool* aResult)
{
  nsCOMPtr<nsIFile> otherfile(do_QueryInterface(aOther));
  if (!otherfile) {
    *aResult = false;
    return NS_OK;
  }

  return Equals(otherfile, aResult);
}

NS_IMETHODIMP
nsLocalFile::GetHashCode(uint32_t* aResult)
{
  
  
  EnsureShortPath();

  *aResult = HashString(mShortWorkingPath);
  return NS_OK;
}





void
nsLocalFile::GlobalInit()
{
  DebugOnly<nsresult> rv = NS_CreateShortcutResolver();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Shortcut resolver could not be created");
}

void
nsLocalFile::GlobalShutdown()
{
  NS_DestroyShortcutResolver();
}

NS_IMPL_ISUPPORTS(nsDriveEnumerator, nsISimpleEnumerator)

nsDriveEnumerator::nsDriveEnumerator()
{
}

nsDriveEnumerator::~nsDriveEnumerator()
{
}

nsresult
nsDriveEnumerator::Init()
{
  


  DWORD length = GetLogicalDriveStringsW(0, 0);
  
  if (!mDrives.SetLength(length + 1, fallible)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (!GetLogicalDriveStringsW(length, wwc(mDrives.BeginWriting()))) {
    return NS_ERROR_FAILURE;
  }
  mDrives.BeginReading(mStartOfCurrentDrive);
  mDrives.EndReading(mEndOfDrivesString);
  return NS_OK;
}

NS_IMETHODIMP
nsDriveEnumerator::HasMoreElements(bool* aHasMore)
{
  *aHasMore = *mStartOfCurrentDrive != L'\0';
  return NS_OK;
}

NS_IMETHODIMP
nsDriveEnumerator::GetNext(nsISupports** aNext)
{
  



  if (*mStartOfCurrentDrive == L'\0') {
    *aNext = nullptr;
    return NS_OK;
  }

  nsAString::const_iterator driveEnd = mStartOfCurrentDrive;
  FindCharInReadable(L'\0', driveEnd, mEndOfDrivesString);
  nsString drive(Substring(mStartOfCurrentDrive, driveEnd));
  mStartOfCurrentDrive = ++driveEnd;

  nsIFile* file;
  nsresult rv = NS_NewLocalFile(drive, false, &file);

  *aNext = file;
  return rv;
}
