



#include "mozilla/DebugOnly.h"

#include "fcntl.h"
#include "errno.h"

#include "prsystem.h"

#if defined(XP_UNIX)
#include "unistd.h"
#include "dirent.h"
#include "sys/stat.h"
#endif 

#if defined(XP_LINUX)
#include <linux/fadvise.h>
#endif 

#if defined(XP_MACOSX)
#include "copyfile.h"
#endif 

#if defined(XP_WIN)
#include <windows.h>
#endif 

#include "jsapi.h"
#include "jsfriendapi.h"
#include "BindingUtils.h"



#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIXULRuntime.h"
#include "nsXPCOMCIDInternal.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "mozJSComponentLoader.h"

#include "OSFileConstants.h"
#include "nsIOSFileConstantsService.h"
#include "nsZipArchive.h"

#if defined(__DragonFly__) || defined(__FreeBSD__) \
  || defined(__NetBSD__) || defined(__OpenBSD__)
#define __dd_fd dd_fd
#endif







namespace mozilla {



namespace {



bool gInitialized = false;

struct Paths {
  


  nsString libDir;
  nsString tmpDir;
  nsString profileDir;
  nsString localProfileDir;
  


  nsString homeDir;
  



  nsString desktopDir;

#if defined(XP_WIN)
  


  nsString winAppDataDir;
  


  nsString winStartMenuProgsDir;
#endif 

#if defined(XP_MACOSX)
  


  nsString macUserLibDir;
  



  nsString macLocalApplicationsDir;
#endif 

  Paths()
  {
    libDir.SetIsVoid(true);
    tmpDir.SetIsVoid(true);
    profileDir.SetIsVoid(true);
    localProfileDir.SetIsVoid(true);
    homeDir.SetIsVoid(true);
    desktopDir.SetIsVoid(true);

#if defined(XP_WIN)
    winAppDataDir.SetIsVoid(true);
    winStartMenuProgsDir.SetIsVoid(true);
#endif 

#if defined(XP_MACOSX)
    macUserLibDir.SetIsVoid(true);
    macLocalApplicationsDir.SetIsVoid(true);
#endif 
  }
};




Paths* gPaths = nullptr;

}








nsresult GetPathToSpecialDir(const char *aKey, nsString& aOutPath)
{
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory(aKey, getter_AddRefs(file));
  if (NS_FAILED(rv) || !file) {
    return rv;
  }

  return file->GetPath(aOutPath);
}











class DelayedPathSetter MOZ_FINAL: public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  DelayedPathSetter() {}
};

NS_IMPL_ISUPPORTS1(DelayedPathSetter, nsIObserver)

NS_IMETHODIMP
DelayedPathSetter::Observe(nsISupports*, const char * aTopic, const PRUnichar*)
{
  if (gPaths == nullptr) {
    
    
    return NS_OK;
  }
  nsresult rv = GetPathToSpecialDir(NS_APP_USER_PROFILE_50_DIR, gPaths->profileDir);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = GetPathToSpecialDir(NS_APP_USER_PROFILE_LOCAL_50_DIR, gPaths->localProfileDir);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}





nsresult InitOSFileConstants()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (gInitialized) {
    return NS_OK;
  }

  gInitialized = true;

  nsAutoPtr<Paths> paths(new Paths);

  
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory("XpcomLib", getter_AddRefs(file));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIFile> libDir;
  rv = file->GetParent(getter_AddRefs(libDir));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = libDir->GetPath(paths->libDir);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  rv = GetPathToSpecialDir(NS_APP_USER_PROFILE_50_DIR, paths->profileDir);
  if (NS_SUCCEEDED(rv)) {
    rv = GetPathToSpecialDir(NS_APP_USER_PROFILE_LOCAL_50_DIR, paths->localProfileDir);
  }

  
  
  if (NS_FAILED(rv)) {
    nsCOMPtr<nsIObserverService> obsService = do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
    nsRefPtr<DelayedPathSetter> pathSetter = new DelayedPathSetter();
    rv = obsService->AddObserver(pathSetter, "profile-do-change", false);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  
  

  GetPathToSpecialDir(NS_OS_TEMP_DIR, paths->tmpDir);
  GetPathToSpecialDir(NS_OS_HOME_DIR, paths->homeDir);
  GetPathToSpecialDir(NS_OS_DESKTOP_DIR, paths->desktopDir);

#if defined(XP_WIN)
  GetPathToSpecialDir(NS_WIN_APPDATA_DIR, paths->winAppDataDir);
  GetPathToSpecialDir(NS_WIN_PROGRAMS_DIR, paths->winStartMenuProgsDir);
#endif 

#if defined(XP_MACOSX)
  GetPathToSpecialDir(NS_MAC_USER_LIB_DIR, paths->macUserLibDir);
  GetPathToSpecialDir(NS_OSX_LOCAL_APPLICATIONS_DIR, paths->macLocalApplicationsDir);
#endif 

  gPaths = paths.forget();
  return NS_OK;
}




void CleanupOSFileConstants()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!gInitialized) {
    return;
  }

  gInitialized = false;
  delete gPaths;
}










#define INT_CONSTANT(name)      \
  { #name, INT_TO_JSVAL(name) }




#define PROP_END { nullptr, JS::UndefinedValue() }



#if !defined(S_IRGRP)
#define S_IXOTH 0001
#define S_IWOTH 0002
#define S_IROTH 0004
#define S_IRWXO 0007
#define S_IXGRP 0010
#define S_IWGRP 0020
#define S_IRGRP 0040
#define S_IRWXG 0070
#define S_IXUSR 0100
#define S_IWUSR 0200
#define S_IRUSR 0400
#define S_IRWXU 0700
#endif 









static const dom::ConstantSpec gLibcProperties[] =
{
  
  INT_CONSTANT(O_APPEND),
  INT_CONSTANT(O_CREAT),
#if defined(O_DIRECTORY)
  INT_CONSTANT(O_DIRECTORY),
#endif 
#if defined(O_EVTONLY)
  INT_CONSTANT(O_EVTONLY),
#endif 
  INT_CONSTANT(O_EXCL),
#if defined(O_EXLOCK)
  INT_CONSTANT(O_EXLOCK),
#endif 
#if defined(O_LARGEFILE)
  INT_CONSTANT(O_LARGEFILE),
#endif 
#if defined(O_NOFOLLOW)
  INT_CONSTANT(O_NOFOLLOW),
#endif 
#if defined(O_NONBLOCK)
  INT_CONSTANT(O_NONBLOCK),
#endif 
  INT_CONSTANT(O_RDONLY),
  INT_CONSTANT(O_RDWR),
#if defined(O_RSYNC)
  INT_CONSTANT(O_RSYNC),
#endif 
#if defined(O_SHLOCK)
  INT_CONSTANT(O_SHLOCK),
#endif 
#if defined(O_SYMLINK)
  INT_CONSTANT(O_SYMLINK),
#endif 
#if defined(O_SYNC)
  INT_CONSTANT(O_SYNC),
#endif 
  INT_CONSTANT(O_TRUNC),
  INT_CONSTANT(O_WRONLY),

#if defined(AT_EACCESS)
  INT_CONSTANT(AT_EACCESS),
#endif 
#if defined(AT_FDCWD)
  INT_CONSTANT(AT_FDCWD),
#endif 
#if defined(AT_SYMLINK_NOFOLLOW)
  INT_CONSTANT(AT_SYMLINK_NOFOLLOW),
#endif 

#if defined(POSIX_FADV_SEQUENTIAL)
  INT_CONSTANT(POSIX_FADV_SEQUENTIAL),
#endif 

  
#if defined(F_OK)
  INT_CONSTANT(F_OK),
  INT_CONSTANT(R_OK),
  INT_CONSTANT(W_OK),
  INT_CONSTANT(X_OK),
#endif 

  
  INT_CONSTANT(S_IRGRP),
  INT_CONSTANT(S_IROTH),
  INT_CONSTANT(S_IRUSR),
  INT_CONSTANT(S_IRWXG),
  INT_CONSTANT(S_IRWXO),
  INT_CONSTANT(S_IRWXU),
  INT_CONSTANT(S_IWGRP),
  INT_CONSTANT(S_IWOTH),
  INT_CONSTANT(S_IWUSR),
  INT_CONSTANT(S_IXOTH),
  INT_CONSTANT(S_IXGRP),
  INT_CONSTANT(S_IXUSR),

  
  INT_CONSTANT(SEEK_CUR),
  INT_CONSTANT(SEEK_END),
  INT_CONSTANT(SEEK_SET),

  
#if defined(COPYFILE_DATA)
  INT_CONSTANT(COPYFILE_DATA),
  INT_CONSTANT(COPYFILE_EXCL),
  INT_CONSTANT(COPYFILE_XATTR),
  INT_CONSTANT(COPYFILE_STAT),
  INT_CONSTANT(COPYFILE_ACL),
  INT_CONSTANT(COPYFILE_MOVE),
#endif 

  
  INT_CONSTANT(EACCES),
  INT_CONSTANT(EAGAIN),
  INT_CONSTANT(EBADF),
  INT_CONSTANT(EEXIST),
  INT_CONSTANT(EFAULT),
  INT_CONSTANT(EFBIG),
  INT_CONSTANT(EINVAL),
  INT_CONSTANT(EIO),
  INT_CONSTANT(EISDIR),
#if defined(ELOOP) 
  INT_CONSTANT(ELOOP),
#endif 
  INT_CONSTANT(EMFILE),
  INT_CONSTANT(ENAMETOOLONG),
  INT_CONSTANT(ENFILE),
  INT_CONSTANT(ENOENT),
  INT_CONSTANT(ENOMEM),
  INT_CONSTANT(ENOSPC),
  INT_CONSTANT(ENOTDIR),
  INT_CONSTANT(ENXIO),
#if defined(EOPNOTSUPP) 
  INT_CONSTANT(EOPNOTSUPP),
#endif 
#if defined(EOVERFLOW) 
  INT_CONSTANT(EOVERFLOW),
#endif 
  INT_CONSTANT(EPERM),
  INT_CONSTANT(ERANGE),
#if defined(ETIMEDOUT) 
  INT_CONSTANT(ETIMEDOUT),
#endif 
#if defined(EWOULDBLOCK) 
  INT_CONSTANT(EWOULDBLOCK),
#endif 
  INT_CONSTANT(EXDEV),

#if defined(DT_UNKNOWN)
  
  INT_CONSTANT(DT_UNKNOWN),
  INT_CONSTANT(DT_FIFO),
  INT_CONSTANT(DT_CHR),
  INT_CONSTANT(DT_DIR),
  INT_CONSTANT(DT_BLK),
  INT_CONSTANT(DT_REG),
  INT_CONSTANT(DT_LNK),
  INT_CONSTANT(DT_SOCK),
#endif 

#if defined(S_IFIFO)
  
  INT_CONSTANT(S_IFMT),
  INT_CONSTANT(S_IFIFO),
  INT_CONSTANT(S_IFCHR),
  INT_CONSTANT(S_IFDIR),
  INT_CONSTANT(S_IFBLK),
  INT_CONSTANT(S_IFREG),
  INT_CONSTANT(S_IFLNK),
  INT_CONSTANT(S_IFSOCK),
#endif 

  
  
  
  
  
  
  

#if defined(XP_UNIX)
  
  { "OSFILE_SIZEOF_MODE_T", INT_TO_JSVAL(sizeof (mode_t)) },

  
  { "OSFILE_SIZEOF_GID_T", INT_TO_JSVAL(sizeof (gid_t)) },

  
  { "OSFILE_SIZEOF_UID_T", INT_TO_JSVAL(sizeof (uid_t)) },

  
  { "OSFILE_SIZEOF_TIME_T", INT_TO_JSVAL(sizeof (time_t)) },

  
  
  { "OSFILE_SIZEOF_DIRENT", INT_TO_JSVAL(sizeof (dirent)) },

  
  { "OSFILE_OFFSETOF_DIRENT_D_NAME", INT_TO_JSVAL(offsetof (struct dirent, d_name)) },
  
  
  { "OSFILE_SIZEOF_DIRENT_D_NAME", INT_TO_JSVAL(sizeof (struct dirent) - offsetof (struct dirent, d_name)) },

  
  { "OSFILE_SIZEOF_TIMEVAL", INT_TO_JSVAL(sizeof (struct timeval)) },
  { "OSFILE_OFFSETOF_TIMEVAL_TV_SEC", INT_TO_JSVAL(offsetof (struct timeval, tv_sec)) },
  { "OSFILE_OFFSETOF_TIMEVAL_TV_USEC", INT_TO_JSVAL(offsetof (struct timeval, tv_usec)) },

#if defined(DT_UNKNOWN)
  
  
  
  { "OSFILE_OFFSETOF_DIRENT_D_TYPE", INT_TO_JSVAL(offsetof (struct dirent, d_type)) },
#endif 

  
  
#if defined(dirfd)
  { "OSFILE_SIZEOF_DIR", INT_TO_JSVAL(sizeof (DIR)) },

  { "OSFILE_OFFSETOF_DIR_DD_FD", INT_TO_JSVAL(offsetof (DIR, __dd_fd)) },
#endif

  

  { "OSFILE_SIZEOF_STAT", INT_TO_JSVAL(sizeof (struct stat)) },

  { "OSFILE_OFFSETOF_STAT_ST_MODE", INT_TO_JSVAL(offsetof (struct stat, st_mode)) },
  { "OSFILE_OFFSETOF_STAT_ST_UID", INT_TO_JSVAL(offsetof (struct stat, st_uid)) },
  { "OSFILE_OFFSETOF_STAT_ST_GID", INT_TO_JSVAL(offsetof (struct stat, st_gid)) },
  { "OSFILE_OFFSETOF_STAT_ST_SIZE", INT_TO_JSVAL(offsetof (struct stat, st_size)) },

#if defined(HAVE_ST_ATIMESPEC)
  { "OSFILE_OFFSETOF_STAT_ST_ATIME", INT_TO_JSVAL(offsetof (struct stat, st_atimespec)) },
  { "OSFILE_OFFSETOF_STAT_ST_MTIME", INT_TO_JSVAL(offsetof (struct stat, st_mtimespec)) },
  { "OSFILE_OFFSETOF_STAT_ST_CTIME", INT_TO_JSVAL(offsetof (struct stat, st_ctimespec)) },
#else
  { "OSFILE_OFFSETOF_STAT_ST_ATIME", INT_TO_JSVAL(offsetof (struct stat, st_atime)) },
  { "OSFILE_OFFSETOF_STAT_ST_MTIME", INT_TO_JSVAL(offsetof (struct stat, st_mtime)) },
  { "OSFILE_OFFSETOF_STAT_ST_CTIME", INT_TO_JSVAL(offsetof (struct stat, st_ctime)) },
#endif 

  
#if defined(_DARWIN_FEATURE_64_BIT_INODE)
  { "OSFILE_OFFSETOF_STAT_ST_BIRTHTIME", INT_TO_JSVAL(offsetof (struct stat, st_birthtime)) },
#endif 

#endif 



  

  
  
  
  
  
  
#if defined(_DARWIN_FEATURE_64_BIT_INODE)
   { "_DARWIN_FEATURE_64_BIT_INODE", INT_TO_JSVAL(1) },
#endif 

  
#if defined(_STAT_VER)
  INT_CONSTANT(_STAT_VER),
#endif 

  PROP_END
};


#if defined(XP_WIN)








static const dom::ConstantSpec gWinProperties[] =
{
  
  INT_CONSTANT(FORMAT_MESSAGE_FROM_SYSTEM),
  INT_CONSTANT(FORMAT_MESSAGE_IGNORE_INSERTS),

  
  INT_CONSTANT(MAX_PATH),

  
  INT_CONSTANT(GENERIC_ALL),
  INT_CONSTANT(GENERIC_EXECUTE),
  INT_CONSTANT(GENERIC_READ),
  INT_CONSTANT(GENERIC_WRITE),

  
  INT_CONSTANT(FILE_SHARE_DELETE),
  INT_CONSTANT(FILE_SHARE_READ),
  INT_CONSTANT(FILE_SHARE_WRITE),

  
  INT_CONSTANT(CREATE_ALWAYS),
  INT_CONSTANT(CREATE_NEW),
  INT_CONSTANT(OPEN_ALWAYS),
  INT_CONSTANT(OPEN_EXISTING),
  INT_CONSTANT(TRUNCATE_EXISTING),

  
  INT_CONSTANT(FILE_ATTRIBUTE_ARCHIVE),
  INT_CONSTANT(FILE_ATTRIBUTE_DIRECTORY),
  INT_CONSTANT(FILE_ATTRIBUTE_NORMAL),
  INT_CONSTANT(FILE_ATTRIBUTE_READONLY),
  INT_CONSTANT(FILE_ATTRIBUTE_REPARSE_POINT),
  INT_CONSTANT(FILE_ATTRIBUTE_TEMPORARY),
  INT_CONSTANT(FILE_FLAG_BACKUP_SEMANTICS),

  
  { "INVALID_HANDLE_VALUE", INT_TO_JSVAL(INT_PTR(INVALID_HANDLE_VALUE)) },


  
  INT_CONSTANT(FILE_FLAG_DELETE_ON_CLOSE),

  
  INT_CONSTANT(FILE_BEGIN),
  INT_CONSTANT(FILE_CURRENT),
  INT_CONSTANT(FILE_END),

  
  INT_CONSTANT(INVALID_SET_FILE_POINTER),

  
  INT_CONSTANT(FILE_ATTRIBUTE_DIRECTORY),


  
  INT_CONSTANT(MOVEFILE_COPY_ALLOWED),
  INT_CONSTANT(MOVEFILE_REPLACE_EXISTING),

  
  INT_CONSTANT(INVALID_FILE_ATTRIBUTES),

  
  INT_CONSTANT(ERROR_ACCESS_DENIED),
  INT_CONSTANT(ERROR_DIR_NOT_EMPTY),
  INT_CONSTANT(ERROR_FILE_EXISTS),
  INT_CONSTANT(ERROR_ALREADY_EXISTS),
  INT_CONSTANT(ERROR_FILE_NOT_FOUND),
  INT_CONSTANT(ERROR_NO_MORE_FILES),
  INT_CONSTANT(ERROR_PATH_NOT_FOUND),

  PROP_END
};
#endif 








JSObject *GetOrCreateObjectProperty(JSContext *cx, JS::Handle<JSObject*> aObject,
                                    const char *aProperty)
{
  JS::Rooted<JS::Value> val(cx);
  if (!JS_GetProperty(cx, aObject, aProperty, &val)) {
    return nullptr;
  }
  if (!val.isUndefined()) {
    if (val.isObject()) {
      return &val.toObject();
    }

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
      JSMSG_UNEXPECTED_TYPE, aProperty, "not an object");
    return nullptr;
  }
  return JS_DefineObject(cx, aObject, aProperty, nullptr, nullptr,
                         JSPROP_ENUMERATE);
}






bool SetStringProperty(JSContext *cx, JS::Handle<JSObject*> aObject, const char *aProperty,
                       const nsString aValue)
{
  if (aValue.IsVoid()) {
    return true;
  }
  JSString* strValue = JS_NewUCStringCopyZ(cx, aValue.get());
  NS_ENSURE_TRUE(strValue, false);
  JS::Rooted<JS::Value> valValue(cx, STRING_TO_JSVAL(strValue));
  return JS_SetProperty(cx, aObject, aProperty, valValue);
}







bool DefineOSFileConstants(JSContext *cx, JS::Handle<JSObject*> global)
{
  MOZ_ASSERT(gInitialized);

  if (gPaths == nullptr) {
    
    
    
    
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
      JSMSG_CANT_OPEN, "OSFileConstants", "initialization has failed");
    return false;
  }

  JS::Rooted<JSObject*> objOS(cx);
  if (!(objOS = GetOrCreateObjectProperty(cx, global, "OS"))) {
    return false;
  }
  JS::Rooted<JSObject*> objConstants(cx);
  if (!(objConstants = GetOrCreateObjectProperty(cx, objOS, "Constants"))) {
    return false;
  }

  

  JS::Rooted<JSObject*> objLibc(cx);
  if (!(objLibc = GetOrCreateObjectProperty(cx, objConstants, "libc"))) {
    return false;
  }
  if (!dom::DefineConstants(cx, objLibc, gLibcProperties)) {
    return false;
  }

#if defined(XP_WIN)
  

  JS::Rooted<JSObject*> objWin(cx);
  if (!(objWin = GetOrCreateObjectProperty(cx, objConstants, "Win"))) {
    return false;
  }
  if (!dom::DefineConstants(cx, objWin, gWinProperties)) {
    return false;
  }
#endif 

  

  JS::Rooted<JSObject*> objSys(cx);
  if (!(objSys = GetOrCreateObjectProperty(cx, objConstants, "Sys"))) {
    return false;
  }

  nsCOMPtr<nsIXULRuntime> runtime = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  if (runtime) {
    nsAutoCString os;
    DebugOnly<nsresult> rv = runtime->GetOS(os);
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    JSString* strVersion = JS_NewStringCopyZ(cx, os.get());
    if (!strVersion) {
      return false;
    }

    JS::Rooted<JS::Value> valVersion(cx, STRING_TO_JSVAL(strVersion));
    if (!JS_SetProperty(cx, objSys, "Name", valVersion)) {
      return false;
    }
  }

#if defined(DEBUG)
  JS::Rooted<JS::Value> valDebug(cx, JSVAL_TRUE);
  if (!JS_SetProperty(cx, objSys, "DEBUG", valDebug)) {
    return false;
  }
#endif

  

  JS::Rooted<JSObject*> objPath(cx);
  if (!(objPath = GetOrCreateObjectProperty(cx, objConstants, "Path"))) {
    return false;
  }

  
  {
    nsAutoString xulPath(gPaths->libDir);

    xulPath.Append(PR_GetDirectorySeparator());

#if defined(XP_MACOSX)
    
    xulPath.Append(NS_LITERAL_STRING("XUL"));
#else
    
    
    xulPath.Append(NS_LITERAL_STRING(DLL_PREFIX));
    xulPath.Append(NS_LITERAL_STRING("xul"));
    xulPath.Append(NS_LITERAL_STRING(DLL_SUFFIX));
#endif 

    if (!SetStringProperty(cx, objPath, "libxul", xulPath)) {
      return false;
    }
  }

  if (!SetStringProperty(cx, objPath, "libDir", gPaths->libDir)) {
    return false;
  }

  if (!SetStringProperty(cx, objPath, "tmpDir", gPaths->tmpDir)) {
    return false;
  }

  
  if (!gPaths->profileDir.IsVoid()
    && !SetStringProperty(cx, objPath, "profileDir", gPaths->profileDir)) {
    return false;
  }

  
  if (!gPaths->localProfileDir.IsVoid()
    && !SetStringProperty(cx, objPath, "localProfileDir", gPaths->localProfileDir)) {
    return false;
  }

  if (!SetStringProperty(cx, objPath, "homeDir", gPaths->homeDir)) {
    return false;
  }

  if (!SetStringProperty(cx, objPath, "desktopDir", gPaths->desktopDir)) {
    return false;
  }

#if defined(XP_WIN)
  if (!SetStringProperty(cx, objPath, "winAppDataDir", gPaths->winAppDataDir)) {
    return false;
  }

  if (!SetStringProperty(cx, objPath, "winStartMenuProgsDir", gPaths->winStartMenuProgsDir)) {
    return false;
  }
#endif 

#if defined(XP_MACOSX)
  if (!SetStringProperty(cx, objPath, "macUserLibDir", gPaths->macUserLibDir)) {
    return false;
  }

  if (!SetStringProperty(cx, objPath, "macLocalApplicationsDir", gPaths->macLocalApplicationsDir)) {
    return false;
  }
#endif 

  return true;
}

NS_IMPL_ISUPPORTS1(OSFileConstantsService, nsIOSFileConstantsService)

OSFileConstantsService::OSFileConstantsService()
{
  MOZ_ASSERT(NS_IsMainThread());
}

OSFileConstantsService::~OSFileConstantsService()
{
  mozilla::CleanupOSFileConstants();
}


NS_IMETHODIMP
OSFileConstantsService::Init(JSContext *aCx)
{
  nsresult rv = mozilla::InitOSFileConstants();
  if (NS_FAILED(rv)) {
    return rv;
  }

  mozJSComponentLoader* loader = mozJSComponentLoader::Get();
  JS::Rooted<JSObject*> targetObj(aCx);
  rv = loader->FindTargetObject(aCx, &targetObj);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mozilla::DefineOSFileConstants(aCx, targetObj)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

} 
