































































#ifndef BASE_FILE_PATH_H_
#define BASE_FILE_PATH_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/hash_tables.h"





#if defined(OS_WIN)
#define FILE_PATH_USES_DRIVE_LETTERS
#define FILE_PATH_USES_WIN_SEPARATORS
#endif  



class FilePath {
 public:
#if defined(OS_POSIX)
  
  
  
  typedef std::string StringType;
#elif defined(OS_WIN)
  
  
  typedef std::wstring StringType;
#endif  

  typedef StringType::value_type CharType;

  
  
  
  
  static const CharType kSeparators[];

  
  static const CharType kCurrentDirectory[];

  
  static const CharType kParentDirectory[];

  
  static const CharType kExtensionSeparator;

  FilePath() {}
  FilePath(const FilePath& that) : path_(that.path_) {}
  explicit FilePath(const StringType& path) : path_(path) {}

  FilePath& operator=(const FilePath& that) {
    path_ = that.path_;
    return *this;
  }

  bool operator==(const FilePath& that) const {
    return path_ == that.path_;
  }

  bool operator!=(const FilePath& that) const {
    return path_ != that.path_;
  }

  
  bool operator<(const FilePath& that) const {
    return path_ < that.path_;
  }

  const StringType& value() const { return path_; }

  bool empty() const { return path_.empty(); }

  
  static bool IsSeparator(CharType character);

  
  
  
  
  
  FilePath DirName() const;

  
  
  
  
  FilePath BaseName() const;

  
  
  
  
  
  
  
  
  StringType Extension() const;

  
  
  
  FilePath RemoveExtension() const;

  
  
  
  
  
  
  
  FilePath InsertBeforeExtension(const StringType& suffix) const;

  
  
  
  
  FilePath ReplaceExtension(const StringType& extension) const;

  
  
  
  
  
  
  FilePath Append(const StringType& component) const WARN_UNUSED_RESULT;
  FilePath Append(const FilePath& component) const WARN_UNUSED_RESULT;

  
  
  
  
  
  
  FilePath AppendASCII(const std::string& component) const WARN_UNUSED_RESULT;

  
  
  
  
  bool IsAbsolute() const;

  
  
  FilePath StripTrailingSeparators() const;

  
  
  
  
  
  static FilePath FromWStringHack(const std::wstring& wstring);

  
  
  
  
  
  std::wstring ToWStringHack() const;

 private:
  
  
  
  
  
  void StripTrailingSeparatorsInternal();

  StringType path_;
};


#if defined(OS_POSIX)
#define FILE_PATH_LITERAL(x) x
#elif defined(OS_WIN)
#define FILE_PATH_LITERAL(x) L ## x
#endif  


#if defined(COMPILER_GCC)
namespace __gnu_cxx {

template<>
struct hash<FilePath> {
  size_t operator()(const FilePath& f) const {
    return std::tr1::hash<FilePath::StringType>()(f.value());
  }
};

}  
#elif defined(COMPILER_MSVC)
namespace stdext {

inline size_t hash_value(const FilePath& f) {
  return hash_value(f.value());
}

}  
#endif  

#endif  
