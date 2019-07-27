









#include "webrtc/base/unixfilesystem.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <Carbon/Carbon.h>
#include <IOKit/IOCFBundle.h>
#include <sys/statvfs.h>
#include "webrtc/base/macutils.h"
#endif  

#if defined(WEBRTC_POSIX) && !defined(WEBRTC_MAC) || defined(WEBRTC_IOS)
#include <sys/types.h>
#if defined(WEBRTC_ANDROID)
#include <sys/statfs.h>
#elif !defined(__native_client__)
#include <sys/statvfs.h>
#endif  
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#endif  

#if defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
#include <ctype.h>
#include <algorithm>
#endif

#if defined(__native_client__) && !defined(__GLIBC__)
#include <sys/syslimits.h>
#endif

#include "webrtc/base/fileutils.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"
#include "webrtc/base/stringutils.h"

#if defined(WEBRTC_IOS)



char* IOSDataDirectory();
char* IOSTempDirectory();
void IOSAppName(rtc::Pathname* path);
#endif

namespace rtc {

#if !defined(WEBRTC_ANDROID) && !defined(WEBRTC_IOS)
char* UnixFilesystem::app_temp_path_ = NULL;
#else
char* UnixFilesystem::provided_app_data_folder_ = NULL;
char* UnixFilesystem::provided_app_temp_folder_ = NULL;

void UnixFilesystem::SetAppDataFolder(const std::string& folder) {
  delete [] provided_app_data_folder_;
  provided_app_data_folder_ = CopyString(folder);
}

void UnixFilesystem::SetAppTempFolder(const std::string& folder) {
  delete [] provided_app_temp_folder_;
  provided_app_temp_folder_ = CopyString(folder);
}
#endif

UnixFilesystem::UnixFilesystem() {
#if defined(WEBRTC_IOS)
  if (!provided_app_data_folder_)
    provided_app_data_folder_ = IOSDataDirectory();
  if (!provided_app_temp_folder_)
    provided_app_temp_folder_ = IOSTempDirectory();
#endif
}

UnixFilesystem::~UnixFilesystem() {}

bool UnixFilesystem::CreateFolder(const Pathname &path, mode_t mode) {
  std::string pathname(path.pathname());
  int len = pathname.length();
  if ((len == 0) || (pathname[len - 1] != '/'))
    return false;

  struct stat st;
  int res = ::stat(pathname.c_str(), &st);
  if (res == 0) {
    
    return S_ISDIR(st.st_mode) != 0;
  } else if (errno != ENOENT) {
    
    return false;
  }

  
  do {
    --len;
  } while ((len > 0) && (pathname[len - 1] != '/'));

  if (!CreateFolder(Pathname(pathname.substr(0, len)), mode)) {
    return false;
  }

  LOG(LS_INFO) << "Creating folder: " << pathname;
  return (0 == ::mkdir(pathname.c_str(), mode));
}

bool UnixFilesystem::CreateFolder(const Pathname &path) {
  return CreateFolder(path, 0755);
}

FileStream *UnixFilesystem::OpenFile(const Pathname &filename,
                                     const std::string &mode) {
  FileStream *fs = new FileStream();
  if (fs && !fs->Open(filename.pathname().c_str(), mode.c_str(), NULL)) {
    delete fs;
    fs = NULL;
  }
  return fs;
}

bool UnixFilesystem::CreatePrivateFile(const Pathname &filename) {
  int fd = open(filename.pathname().c_str(),
                O_RDWR | O_CREAT | O_EXCL,
                S_IRUSR | S_IWUSR);
  if (fd < 0) {
    LOG_ERR(LS_ERROR) << "open() failed.";
    return false;
  }
  
  if (close(fd) < 0) {
    LOG_ERR(LS_ERROR) << "close() failed.";
    
  }
  return true;
}

bool UnixFilesystem::DeleteFile(const Pathname &filename) {
  LOG(LS_INFO) << "Deleting file:" << filename.pathname();

  if (!IsFile(filename)) {
    ASSERT(IsFile(filename));
    return false;
  }
  return ::unlink(filename.pathname().c_str()) == 0;
}

bool UnixFilesystem::DeleteEmptyFolder(const Pathname &folder) {
  LOG(LS_INFO) << "Deleting folder" << folder.pathname();

  if (!IsFolder(folder)) {
    ASSERT(IsFolder(folder));
    return false;
  }
  std::string no_slash(folder.pathname(), 0, folder.pathname().length()-1);
  return ::rmdir(no_slash.c_str()) == 0;
}

bool UnixFilesystem::GetTemporaryFolder(Pathname &pathname, bool create,
                                        const std::string *append) {
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  FSRef fr;
  if (0 != FSFindFolder(kOnAppropriateDisk, kTemporaryFolderType,
                        kCreateFolder, &fr))
    return false;
  unsigned char buffer[NAME_MAX+1];
  if (0 != FSRefMakePath(&fr, buffer, ARRAY_SIZE(buffer)))
    return false;
  pathname.SetPathname(reinterpret_cast<char*>(buffer), "");
#elif defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
  ASSERT(provided_app_temp_folder_ != NULL);
  pathname.SetPathname(provided_app_temp_folder_, "");
#else  
  if (const char* tmpdir = getenv("TMPDIR")) {
    pathname.SetPathname(tmpdir, "");
  } else if (const char* tmp = getenv("TMP")) {
    pathname.SetPathname(tmp, "");
  } else {
#ifdef P_tmpdir
    pathname.SetPathname(P_tmpdir, "");
#else  
    pathname.SetPathname("/tmp/", "");
#endif  
  }
#endif  
  if (append) {
    ASSERT(!append->empty());
    pathname.AppendFolder(*append);
  }
  return !create || CreateFolder(pathname);
}

std::string UnixFilesystem::TempFilename(const Pathname &dir,
                                         const std::string &prefix) {
  int len = dir.pathname().size() + prefix.size() + 2 + 6;
  char *tempname = new char[len];

  snprintf(tempname, len, "%s/%sXXXXXX", dir.pathname().c_str(),
           prefix.c_str());
  int fd = ::mkstemp(tempname);
  if (fd != -1)
    ::close(fd);
  std::string ret(tempname);
  delete[] tempname;

  return ret;
}

bool UnixFilesystem::MoveFile(const Pathname &old_path,
                              const Pathname &new_path) {
  if (!IsFile(old_path)) {
    ASSERT(IsFile(old_path));
    return false;
  }
  LOG(LS_VERBOSE) << "Moving " << old_path.pathname()
                  << " to " << new_path.pathname();
  if (rename(old_path.pathname().c_str(), new_path.pathname().c_str()) != 0) {
    if (errno != EXDEV)
      return false;
    if (!CopyFile(old_path, new_path))
      return false;
    if (!DeleteFile(old_path))
      return false;
  }
  return true;
}

bool UnixFilesystem::MoveFolder(const Pathname &old_path,
                                const Pathname &new_path) {
  if (!IsFolder(old_path)) {
    ASSERT(IsFolder(old_path));
    return false;
  }
  LOG(LS_VERBOSE) << "Moving " << old_path.pathname()
                  << " to " << new_path.pathname();
  if (rename(old_path.pathname().c_str(), new_path.pathname().c_str()) != 0) {
    if (errno != EXDEV)
      return false;
    if (!CopyFolder(old_path, new_path))
      return false;
    if (!DeleteFolderAndContents(old_path))
      return false;
  }
  return true;
}

bool UnixFilesystem::IsFolder(const Pathname &path) {
  struct stat st;
  if (stat(path.pathname().c_str(), &st) < 0)
    return false;
  return S_ISDIR(st.st_mode);
}

bool UnixFilesystem::CopyFile(const Pathname &old_path,
                              const Pathname &new_path) {
  LOG(LS_VERBOSE) << "Copying " << old_path.pathname()
                  << " to " << new_path.pathname();
  char buf[256];
  size_t len;

  StreamInterface *source = OpenFile(old_path, "rb");
  if (!source)
    return false;

  StreamInterface *dest = OpenFile(new_path, "wb");
  if (!dest) {
    delete source;
    return false;
  }

  while (source->Read(buf, sizeof(buf), &len, NULL) == SR_SUCCESS)
    dest->Write(buf, len, NULL, NULL);

  delete source;
  delete dest;
  return true;
}

bool UnixFilesystem::IsTemporaryPath(const Pathname& pathname) {
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
  ASSERT(provided_app_temp_folder_ != NULL);
#endif

  const char* const kTempPrefixes[] = {
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
    provided_app_temp_folder_,
#else
    "/tmp/", "/var/tmp/",
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
    "/private/tmp/", "/private/var/tmp/", "/private/var/folders/",
#endif  // WEBRTC_MAC && !defined(WEBRTC_IOS)
#endif  // WEBRTC_ANDROID || WEBRTC_IOS
  };
  for (size_t i = 0; i < ARRAY_SIZE(kTempPrefixes); ++i) {
    if (0 == strncmp(pathname.pathname().c_str(), kTempPrefixes[i],
                     strlen(kTempPrefixes[i])))
      return true;
  }
  return false;
}

bool UnixFilesystem::IsFile(const Pathname& pathname) {
  struct stat st;
  int res = ::stat(pathname.pathname().c_str(), &st);
  
  return res == 0 && !S_ISDIR(st.st_mode);
}

bool UnixFilesystem::IsAbsent(const Pathname& pathname) {
  struct stat st;
  int res = ::stat(pathname.pathname().c_str(), &st);
  
  
  return res != 0 && ENOENT == errno;
}

bool UnixFilesystem::GetFileSize(const Pathname& pathname, size_t *size) {
  struct stat st;
  if (::stat(pathname.pathname().c_str(), &st) != 0)
    return false;
  *size = st.st_size;
  return true;
}

bool UnixFilesystem::GetFileTime(const Pathname& path, FileTimeType which,
                                 time_t* time) {
  struct stat st;
  if (::stat(path.pathname().c_str(), &st) != 0)
    return false;
  switch (which) {
  case FTT_CREATED:
    *time = st.st_ctime;
    break;
  case FTT_MODIFIED:
    *time = st.st_mtime;
    break;
  case FTT_ACCESSED:
    *time = st.st_atime;
    break;
  default:
    return false;
  }
  return true;
}

bool UnixFilesystem::GetAppPathname(Pathname* path) {
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  ProcessSerialNumber psn = { 0, kCurrentProcess };
  CFDictionaryRef procinfo = ProcessInformationCopyDictionary(&psn,
      kProcessDictionaryIncludeAllInformationMask);
  if (NULL == procinfo)
    return false;
  CFStringRef cfpath = (CFStringRef) CFDictionaryGetValue(procinfo,
      kIOBundleExecutableKey);
  std::string path8;
  bool success = ToUtf8(cfpath, &path8);
  CFRelease(procinfo);
  if (success)
    path->SetPathname(path8);
  return success;
#elif defined(__native_client__)
  return false;
#elif IOS
  IOSAppName(path);
  return true;
#else  
  char buffer[PATH_MAX + 2];
  ssize_t len = readlink("/proc/self/exe", buffer, ARRAY_SIZE(buffer) - 1);
  if ((len <= 0) || (len == PATH_MAX + 1))
    return false;
  buffer[len] = '\0';
  path->SetPathname(buffer);
  return true;
#endif  
}

bool UnixFilesystem::GetAppDataFolder(Pathname* path, bool per_user) {
  ASSERT(!organization_name_.empty());
  ASSERT(!application_name_.empty());

  
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  if (per_user) {
    
    FSRef fr;
    if (0 != FSFindFolder(kUserDomain, kApplicationSupportFolderType,
                          kCreateFolder, &fr))
      return false;
    unsigned char buffer[NAME_MAX+1];
    if (0 != FSRefMakePath(&fr, buffer, ARRAY_SIZE(buffer)))
      return false;
    path->SetPathname(reinterpret_cast<char*>(buffer), "");
  } else {
    
    return false;
  }
#elif defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)  
  ASSERT(provided_app_data_folder_ != NULL);
  path->SetPathname(provided_app_data_folder_, "");
#elif defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)  
  if (per_user) {
    
    
    
    
    
    const char* xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home) {
      path->SetPathname(xdg_config_home, "");
    } else {
      
      
      const char* homedir;
      if (const char* home = getenv("HOME")) {
        homedir = home;
      } else if (const char* dotdir = getenv("DOTDIR")) {
        homedir = dotdir;
      } else if (passwd* pw = getpwuid(geteuid())) {
        homedir = pw->pw_dir;
      } else {
        return false;
      }
      path->SetPathname(homedir, "");
      path->AppendFolder(".config");
    }
  } else {
    
    
    path->SetPathname("/var/cache/", "");
  }
#endif  

  
#if defined(WEBRTC_MAC) || defined(WEBRTC_ANDROID)
  path->AppendFolder(organization_name_);
  path->AppendFolder(application_name_);
#elif defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
  
  
  
  std::string subdir(organization_name_);
  subdir.append("-");
  subdir.append(application_name_);
  replace_substrs(" ", 1, "", 0, &subdir);
  std::transform(subdir.begin(), subdir.end(), subdir.begin(), ::tolower);
  path->AppendFolder(subdir);
#endif
  if (!CreateFolder(*path, 0700)) {
    return false;
  }
#if !defined(__native_client__)
  
  
  
  if (0 != ::chmod(path->pathname().c_str(), 0700)) {
    LOG_ERR(LS_ERROR) << "Can't set mode on " << path;
    return false;
  }
#endif
  return true;
}

bool UnixFilesystem::GetAppTempFolder(Pathname* path) {
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
  ASSERT(provided_app_temp_folder_ != NULL);
  path->SetPathname(provided_app_temp_folder_);
  return true;
#else
  ASSERT(!application_name_.empty());
  
  if (app_temp_path_ != NULL && strlen(app_temp_path_) > 0) {
    path->SetPathname(app_temp_path_);
    return true;
  }

  
  char buffer[128];
  sprintfn(buffer, ARRAY_SIZE(buffer), "-%d-%d",
           static_cast<int>(getpid()),
           static_cast<int>(time(0)));
  std::string folder(application_name_);
  folder.append(buffer);
  if (!GetTemporaryFolder(*path, true, &folder))
    return false;

  delete [] app_temp_path_;
  app_temp_path_ = CopyString(path->pathname());
  
  return true;
#endif
}

bool UnixFilesystem::GetDiskFreeSpace(const Pathname& path, int64 *freebytes) {
#ifdef __native_client__
  return false;
#else  
  ASSERT(NULL != freebytes);
  
  
  
  Pathname existing_path(path.folder(), "");
  while (!existing_path.folder().empty() && IsAbsent(existing_path)) {
    existing_path.SetFolder(existing_path.parent_folder());
  }
#if defined(WEBRTC_ANDROID)
  struct statfs vfs;
  memset(&vfs, 0, sizeof(vfs));
  if (0 != statfs(existing_path.pathname().c_str(), &vfs))
    return false;
#else
  struct statvfs vfs;
  memset(&vfs, 0, sizeof(vfs));
  if (0 != statvfs(existing_path.pathname().c_str(), &vfs))
    return false;
#endif  
#if defined(WEBRTC_LINUX)
  *freebytes = static_cast<int64>(vfs.f_bsize) * vfs.f_bavail;
#elif defined(WEBRTC_MAC)
  *freebytes = static_cast<int64>(vfs.f_frsize) * vfs.f_bavail;
#endif

  return true;
#endif  
}

Pathname UnixFilesystem::GetCurrentDirectory() {
  Pathname cwd;
  char buffer[PATH_MAX];
  char *path = getcwd(buffer, PATH_MAX);

  if (!path) {
    LOG_ERR(LS_ERROR) << "getcwd() failed";
    return cwd;  
  }
  cwd.SetFolder(std::string(path));

  return cwd;
}

char* UnixFilesystem::CopyString(const std::string& str) {
  size_t size = str.length() + 1;

  char* buf = new char[size];
  if (!buf) {
    return NULL;
  }

  strcpyn(buf, size, str.c_str());
  return buf;
}

}  

#if defined(__native_client__)
extern "C" int __attribute__((weak))
link(const char* oldpath, const char* newpath) {
  errno = EACCES;
  return -1;
}
#endif
