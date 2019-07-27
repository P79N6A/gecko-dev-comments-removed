









#ifndef _WEBRTC_BASE_WIN32FILESYSTEM_H__
#define _WEBRTC_BASE_WIN32FILESYSTEM_H__

#include "fileutils.h"

namespace rtc {

class Win32Filesystem : public FilesystemInterface {
 public:
  
  
  virtual FileStream *OpenFile(const Pathname &filename, 
                               const std::string &mode);

  
  
  virtual bool CreatePrivateFile(const Pathname &filename);

  
  
  virtual bool DeleteFile(const Pathname &filename);

  
  
  virtual bool DeleteEmptyFolder(const Pathname &folder);

  
  
  
  virtual bool CreateFolder(const Pathname &pathname);
  
  
  
  
  
  virtual bool MoveFile(const Pathname &old_path, const Pathname &new_path);
  
  
  
  
  virtual bool MoveFolder(const Pathname &old_path, const Pathname &new_path);
  
  
  
  virtual bool CopyFile(const Pathname &old_path, const Pathname &new_path);

  
  virtual bool IsFolder(const Pathname& pathname);
  
  
  virtual bool IsFile(const Pathname &path);

  
  
  virtual bool IsAbsent(const Pathname& pathname);

  
  virtual bool IsTemporaryPath(const Pathname& pathname);

  
  
  
  

  virtual std::string TempFilename(const Pathname &dir, const std::string &prefix);

  virtual bool GetFileSize(const Pathname& path, size_t* size);
  virtual bool GetFileTime(const Pathname& path, FileTimeType which,
                           time_t* time);
 
  
  
  virtual bool GetTemporaryFolder(Pathname &path, bool create,
                                 const std::string *append);

  
  virtual bool GetAppPathname(Pathname* path);

  virtual bool GetAppDataFolder(Pathname* path, bool per_user);

  
  virtual bool GetAppTempFolder(Pathname* path);

  virtual bool GetDiskFreeSpace(const Pathname& path, int64 *freebytes);

  virtual Pathname GetCurrentDirectory();
};

}  

#endif  
