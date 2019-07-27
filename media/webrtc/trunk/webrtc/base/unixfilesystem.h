









#ifndef WEBRTC_BASE_UNIXFILESYSTEM_H_
#define WEBRTC_BASE_UNIXFILESYSTEM_H_

#include <sys/types.h>

#include "webrtc/base/fileutils.h"

namespace rtc {

class UnixFilesystem : public FilesystemInterface {
 public:
  UnixFilesystem();
  virtual ~UnixFilesystem();

#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
  
  
  
  
  
  
  
  static void SetAppDataFolder(const std::string& folder);
  static void SetAppTempFolder(const std::string& folder);
#endif

  
  
  virtual FileStream *OpenFile(const Pathname &filename,
                               const std::string &mode);

  
  
  virtual bool CreatePrivateFile(const Pathname &filename);

  
  
  virtual bool DeleteFile(const Pathname &filename);

  
  
  
  virtual bool DeleteEmptyFolder(const Pathname &folder);

  
  
  
  
  virtual bool CreateFolder(const Pathname &pathname, mode_t mode);

  
  virtual bool CreateFolder(const Pathname &pathname);

  
  
  
  virtual bool MoveFile(const Pathname &old_path, const Pathname &new_path);
  virtual bool MoveFolder(const Pathname &old_path, const Pathname &new_path);

  
  
  
  virtual bool CopyFile(const Pathname &old_path, const Pathname &new_path);

  
  virtual bool IsFolder(const Pathname& pathname);

  
  virtual bool IsTemporaryPath(const Pathname& pathname);

  
  virtual bool IsFile(const Pathname& pathname);

  
  
  virtual bool IsAbsent(const Pathname& pathname);

  virtual std::string TempFilename(const Pathname &dir,
                                   const std::string &prefix);

  
  
  virtual bool GetTemporaryFolder(Pathname &path, bool create,
                                 const std::string *append);

  virtual bool GetFileSize(const Pathname& path, size_t* size);
  virtual bool GetFileTime(const Pathname& path, FileTimeType which,
                           time_t* time);

  
  virtual bool GetAppPathname(Pathname* path);

  virtual bool GetAppDataFolder(Pathname* path, bool per_user);

  
  virtual bool GetAppTempFolder(Pathname* path);

  virtual bool GetDiskFreeSpace(const Pathname& path, int64 *freebytes);

  
  virtual Pathname GetCurrentDirectory();

 private:
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
  static char* provided_app_data_folder_;
  static char* provided_app_temp_folder_;
#else
  static char* app_temp_path_;
#endif

  static char* CopyString(const std::string& str);
};

}  

#endif  
