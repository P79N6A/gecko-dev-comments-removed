









#ifndef WEBRTC_BASE_FILEUTILS_H_
#define WEBRTC_BASE_FILEUTILS_H_

#include <string>

#if !defined(WEBRTC_WIN)
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "webrtc/base/basictypes.h"
#include "webrtc/base/common.h"
#include "webrtc/base/platform_file.h"
#include "webrtc/base/scoped_ptr.h"

namespace rtc {

class FileStream;
class Pathname;









class DirectoryIterator {
  friend class Filesystem;
 public:
  
  DirectoryIterator();
  
  virtual ~DirectoryIterator();

  
  
  
  
  virtual bool Iterate(const Pathname &path);

  
  
  virtual bool Next();

  
  virtual bool IsDirectory() const;

  
  virtual std::string Name() const;

  
  virtual size_t FileSize() const;

  
  virtual bool OlderThan(int seconds) const;

  
  bool IsDots() const {
    std::string filename(Name());
    return (filename.compare(".") == 0) || (filename.compare("..") == 0);
  }

 private:
  std::string directory_;
#if defined(WEBRTC_WIN)
  WIN32_FIND_DATA data_;
  HANDLE handle_;
#else
  DIR *dir_;
  struct dirent *dirent_;
  struct stat stat_;
#endif
};

enum FileTimeType { FTT_CREATED, FTT_MODIFIED, FTT_ACCESSED };

class FilesystemInterface {
 public:
  virtual ~FilesystemInterface() {}

  
  
  virtual DirectoryIterator *IterateDirectory() {
    return new DirectoryIterator();
  }

  
  
  
  
  virtual FileStream *OpenFile(const Pathname &filename,
                               const std::string &mode) = 0;

  
  
  
  
  
  
  
  virtual bool CreatePrivateFile(const Pathname &filename) = 0;

  
  
  
  virtual bool DeleteFile(const Pathname &filename) = 0;

  
  
  
  
  virtual bool DeleteEmptyFolder(const Pathname &folder) = 0;

  
  
  
  virtual bool DeleteFolderContents(const Pathname &folder);

  
  
  virtual bool DeleteFolderAndContents(const Pathname &folder) {
    return DeleteFolderContents(folder) && DeleteEmptyFolder(folder);
  }

  
  
  
  bool DeleteFileOrFolder(const Pathname &path) {
    if (IsFolder(path))
      return DeleteFolderAndContents(path);
    else
      return DeleteFile(path);
  }

  
  
  virtual bool CreateFolder(const Pathname &pathname) = 0;

  
  
  
  
  
  virtual bool MoveFolder(const Pathname &old_path,
                          const Pathname &new_path) = 0;

  
  
  
  
  
  virtual bool MoveFile(const Pathname &old_path, const Pathname &new_path) = 0;

  
  
  bool MoveFileOrFolder(const Pathname &old_path, const Pathname &new_path) {
    if (IsFile(old_path)) {
      return MoveFile(old_path, new_path);
    } else {
      return MoveFolder(old_path, new_path);
    }
  }

  
  
  
  virtual bool CopyFile(const Pathname &old_path, const Pathname &new_path) = 0;

  
  bool CopyFolder(const Pathname &old_path, const Pathname &new_path);

  bool CopyFileOrFolder(const Pathname &old_path, const Pathname &new_path) {
    if (IsFile(old_path))
      return CopyFile(old_path, new_path);
    else
      return CopyFolder(old_path, new_path);
  }

  
  virtual bool IsFolder(const Pathname& pathname) = 0;

  
  virtual bool IsFile(const Pathname& pathname) = 0;

  
  
  virtual bool IsAbsent(const Pathname& pathname) = 0;

  
  virtual bool IsTemporaryPath(const Pathname& pathname) = 0;

  
  
  virtual bool GetTemporaryFolder(Pathname &path, bool create,
                                  const std::string *append) = 0;

  virtual std::string TempFilename(const Pathname &dir,
                                   const std::string &prefix) = 0;

  
  virtual bool GetFileSize(const Pathname& path, size_t* size) = 0;

  
  virtual bool GetFileTime(const Pathname& path, FileTimeType which,
                           time_t* time) = 0;

  
  
  
  virtual bool GetAppPathname(Pathname* path) = 0;

  
  
  
  virtual bool GetAppDataFolder(Pathname* path, bool per_user) = 0;

  
  
  
  
  virtual bool GetAppTempFolder(Pathname* path) = 0;

  
  bool CleanAppTempFolder();

  virtual bool GetDiskFreeSpace(const Pathname& path, int64 *freebytes) = 0;

  
  virtual Pathname GetCurrentDirectory() = 0;

  
  
  void SetOrganizationName(const std::string& organization) {
    organization_name_ = organization;
  }
  void GetOrganizationName(std::string* organization) {
    ASSERT(NULL != organization);
    *organization = organization_name_;
  }
  void SetApplicationName(const std::string& application) {
    application_name_ = application;
  }
  void GetApplicationName(std::string* application) {
    ASSERT(NULL != application);
    *application = application_name_;
  }

 protected:
  std::string organization_name_;
  std::string application_name_;
};

class Filesystem {
 public:
  static FilesystemInterface *default_filesystem() {
    ASSERT(default_filesystem_ != NULL);
    return default_filesystem_;
  }

  static void set_default_filesystem(FilesystemInterface *filesystem) {
    default_filesystem_ = filesystem;
  }

  static FilesystemInterface *swap_default_filesystem(
      FilesystemInterface *filesystem) {
    FilesystemInterface *cur = default_filesystem_;
    default_filesystem_ = filesystem;
    return cur;
  }

  static DirectoryIterator *IterateDirectory() {
    return EnsureDefaultFilesystem()->IterateDirectory();
  }

  static bool CreateFolder(const Pathname &pathname) {
    return EnsureDefaultFilesystem()->CreateFolder(pathname);
  }

  static FileStream *OpenFile(const Pathname &filename,
                              const std::string &mode) {
    return EnsureDefaultFilesystem()->OpenFile(filename, mode);
  }

  static bool CreatePrivateFile(const Pathname &filename) {
    return EnsureDefaultFilesystem()->CreatePrivateFile(filename);
  }

  static bool DeleteFile(const Pathname &filename) {
    return EnsureDefaultFilesystem()->DeleteFile(filename);
  }

  static bool DeleteEmptyFolder(const Pathname &folder) {
    return EnsureDefaultFilesystem()->DeleteEmptyFolder(folder);
  }

  static bool DeleteFolderContents(const Pathname &folder) {
    return EnsureDefaultFilesystem()->DeleteFolderContents(folder);
  }

  static bool DeleteFolderAndContents(const Pathname &folder) {
    return EnsureDefaultFilesystem()->DeleteFolderAndContents(folder);
  }

  static bool MoveFolder(const Pathname &old_path, const Pathname &new_path) {
    return EnsureDefaultFilesystem()->MoveFolder(old_path, new_path);
  }

  static bool MoveFile(const Pathname &old_path, const Pathname &new_path) {
    return EnsureDefaultFilesystem()->MoveFile(old_path, new_path);
  }

  static bool CopyFolder(const Pathname &old_path, const Pathname &new_path) {
    return EnsureDefaultFilesystem()->CopyFolder(old_path, new_path);
  }

  static bool CopyFile(const Pathname &old_path, const Pathname &new_path) {
    return EnsureDefaultFilesystem()->CopyFile(old_path, new_path);
  }

  static bool IsFolder(const Pathname& pathname) {
    return EnsureDefaultFilesystem()->IsFolder(pathname);
  }

  static bool IsFile(const Pathname &pathname) {
    return EnsureDefaultFilesystem()->IsFile(pathname);
  }

  static bool IsAbsent(const Pathname &pathname) {
    return EnsureDefaultFilesystem()->IsAbsent(pathname);
  }

  static bool IsTemporaryPath(const Pathname& pathname) {
    return EnsureDefaultFilesystem()->IsTemporaryPath(pathname);
  }

  static bool GetTemporaryFolder(Pathname &path, bool create,
                                 const std::string *append) {
    return EnsureDefaultFilesystem()->GetTemporaryFolder(path, create, append);
  }

  static std::string TempFilename(const Pathname &dir,
                                  const std::string &prefix) {
    return EnsureDefaultFilesystem()->TempFilename(dir, prefix);
  }

  static bool GetFileSize(const Pathname& path, size_t* size) {
    return EnsureDefaultFilesystem()->GetFileSize(path, size);
  }

  static bool GetFileTime(const Pathname& path, FileTimeType which,
                          time_t* time) {
    return EnsureDefaultFilesystem()->GetFileTime(path, which, time);
  }

  static bool GetAppPathname(Pathname* path) {
    return EnsureDefaultFilesystem()->GetAppPathname(path);
  }

  static bool GetAppDataFolder(Pathname* path, bool per_user) {
    return EnsureDefaultFilesystem()->GetAppDataFolder(path, per_user);
  }

  static bool GetAppTempFolder(Pathname* path) {
    return EnsureDefaultFilesystem()->GetAppTempFolder(path);
  }

  static bool CleanAppTempFolder() {
    return EnsureDefaultFilesystem()->CleanAppTempFolder();
  }

  static bool GetDiskFreeSpace(const Pathname& path, int64 *freebytes) {
    return EnsureDefaultFilesystem()->GetDiskFreeSpace(path, freebytes);
  }

  
  
  static Pathname GetCurrentDirectory();

  static void SetOrganizationName(const std::string& organization) {
    EnsureDefaultFilesystem()->SetOrganizationName(organization);
  }

  static void GetOrganizationName(std::string* organization) {
    EnsureDefaultFilesystem()->GetOrganizationName(organization);
  }

  static void SetApplicationName(const std::string& application) {
    EnsureDefaultFilesystem()->SetApplicationName(application);
  }

  static void GetApplicationName(std::string* application) {
    EnsureDefaultFilesystem()->GetApplicationName(application);
  }

 private:
  static FilesystemInterface* default_filesystem_;

  static FilesystemInterface *EnsureDefaultFilesystem();
  DISALLOW_IMPLICIT_CONSTRUCTORS(Filesystem);
};

class FilesystemScope{
 public:
  explicit FilesystemScope(FilesystemInterface *new_fs) {
    old_fs_ = Filesystem::swap_default_filesystem(new_fs);
  }
  ~FilesystemScope() {
    Filesystem::set_default_filesystem(old_fs_);
  }
 private:
  FilesystemInterface* old_fs_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(FilesystemScope);
};







bool CreateUniqueFile(Pathname& path, bool create_empty);

}  

#endif  
