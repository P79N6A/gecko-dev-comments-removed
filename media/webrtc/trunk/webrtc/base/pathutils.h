









#ifndef WEBRTC_BASE_PATHUTILS_H__
#define WEBRTC_BASE_PATHUTILS_H__

#include <string>

#include "webrtc/base/fileutils.h"

namespace rtc {





















class Pathname {
public:
  
  static bool IsFolderDelimiter(char ch);
  static char DefaultFolderDelimiter();

  Pathname();
  Pathname(const std::string& pathname);
  Pathname(const std::string& folder, const std::string& filename);

  
  char folder_delimiter() const { return folder_delimiter_; }
  void SetFolderDelimiter(char delimiter);

  
  void Normalize();

  
  void clear();

  
  
  bool empty() const;

  std::string url() const;

  
  
  
  std::string pathname() const;
  void SetPathname(const std::string& pathname);
  void SetPathname(const std::string& folder, const std::string& filename);

  
  
  void AppendPathname(const std::string& pathname);

  std::string folder() const;
  std::string folder_name() const;
  std::string parent_folder() const;
  
  void SetFolder(const std::string& folder);
  void AppendFolder(const std::string& folder);

  std::string basename() const;
  bool SetBasename(const std::string& basename);

  std::string extension() const;
  
  bool SetExtension(const std::string& extension);

  std::string filename() const;
  bool SetFilename(const std::string& filename);

#if defined(WEBRTC_WIN)
  bool GetDrive(char *drive, uint32 bytes) const;
  static bool GetDrive(char *drive, uint32 bytes,const std::string& pathname);
#endif

private:
  std::string folder_, basename_, extension_;
  char folder_delimiter_;
};





inline void SetOrganizationName(const std::string& organization) {
  Filesystem::SetOrganizationName(organization);
}
inline void SetApplicationName(const std::string& application) {
  Filesystem::SetApplicationName(application);
}
inline void GetOrganizationName(std::string* organization) {
  Filesystem::GetOrganizationName(organization);
}
inline void GetApplicationName(std::string* application) {
  Filesystem::GetApplicationName(application);
}
inline bool CreateFolder(const Pathname& path) {
  return Filesystem::CreateFolder(path);
}
inline bool FinishPath(Pathname& path, bool create, const std::string& append) {
  if (!append.empty())
    path.AppendFolder(append);
  return !create || CreateFolder(path);
}




inline bool GetTemporaryFolder(Pathname& path, bool create,
                               const std::string& append) {
  std::string application_name;
  Filesystem::GetApplicationName(&application_name);
  ASSERT(!application_name.empty());
  return Filesystem::GetTemporaryFolder(path, create, &application_name)
         && FinishPath(path, create, append);
}
inline bool GetAppDataFolder(Pathname& path, bool create,
                             const std::string& append) {
  ASSERT(!create); 
  return Filesystem::GetAppDataFolder(&path, true)
         && FinishPath(path, create, append);
}
inline bool CleanupTemporaryFolder() {
  Pathname path;
  if (!GetTemporaryFolder(path, false, ""))
    return false;
  if (Filesystem::IsAbsent(path))
    return true;
  if (!Filesystem::IsTemporaryPath(path)) {
    ASSERT(false);
    return false;
  }
  return Filesystem::DeleteFolderContents(path);
}



}  

#endif 
