






































#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_FILEPATH_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_FILEPATH_H_

#include "gtest/internal/gtest-string.h"

namespace testing {
namespace internal {












class GTEST_API_ FilePath {
 public:
  FilePath() : pathname_("") { }
  FilePath(const FilePath& rhs) : pathname_(rhs.pathname_) { }

  explicit FilePath(const std::string& pathname) : pathname_(pathname) {
    Normalize();
  }

  FilePath& operator=(const FilePath& rhs) {
    Set(rhs);
    return *this;
  }

  void Set(const FilePath& rhs) {
    pathname_ = rhs.pathname_;
  }

  const std::string& string() const { return pathname_; }
  const char* c_str() const { return pathname_.c_str(); }

  
  static FilePath GetCurrentDir();

  
  
  
  
  static FilePath MakeFileName(const FilePath& directory,
                               const FilePath& base_name,
                               int number,
                               const char* extension);

  
  
  
  static FilePath ConcatPaths(const FilePath& directory,
                              const FilePath& relative_path);

  
  
  
  
  
  
  
  
  static FilePath GenerateUniqueFileName(const FilePath& directory,
                                         const FilePath& base_name,
                                         const char* extension);

  
  bool IsEmpty() const { return pathname_.empty(); }

  
  
  
  FilePath RemoveTrailingPathSeparator() const;

  
  
  
  
  
  
  FilePath RemoveDirectoryName() const;

  
  
  
  
  
  
  FilePath RemoveFileName() const;

  
  
  
  
  FilePath RemoveExtension(const char* extension) const;

  
  
  
  
  bool CreateDirectoriesRecursively() const;

  
  
  
  
  bool CreateFolder() const;

  
  
  bool FileOrDirectoryExists() const;

  
  
  bool DirectoryExists() const;

  
  
  
  bool IsDirectory() const;

  
  
  bool IsRootDirectory() const;

  
  bool IsAbsolutePath() const;

 private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  void Normalize();

  
  
  
  const char* FindLastPathSeparator() const;

  std::string pathname_;
};  

}  
}  

#endif  
