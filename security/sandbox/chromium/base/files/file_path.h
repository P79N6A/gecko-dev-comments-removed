




































































































#ifndef BASE_FILES_FILE_PATH_H_
#define BASE_FILES_FILE_PATH_H_

#include <stddef.h>
#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/containers/hash_tables.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"  
#include "build/build_config.h"





#if defined(OS_WIN)
#define FILE_PATH_USES_DRIVE_LETTERS
#define FILE_PATH_USES_WIN_SEPARATORS
#endif  

class Pickle;
class PickleIterator;

namespace base {



class BASE_EXPORT FilePath {
 public:
#if defined(OS_POSIX)
  
  
  
  typedef std::string StringType;
#elif defined(OS_WIN)
  
  
  typedef std::wstring StringType;
#endif  

  typedef StringType::value_type CharType;

  
  
  
  
  static const CharType kSeparators[];

  
  static const size_t kSeparatorsLength;

  
  static const CharType kCurrentDirectory[];

  
  static const CharType kParentDirectory[];

  
  static const CharType kExtensionSeparator;

  FilePath();
  FilePath(const FilePath& that);
  explicit FilePath(const StringType& path);
  ~FilePath();
  FilePath& operator=(const FilePath& that);

  bool operator==(const FilePath& that) const;

  bool operator!=(const FilePath& that) const;

  
  bool operator<(const FilePath& that) const {
    return path_ < that.path_;
  }

  const StringType& value() const { return path_; }

  bool empty() const { return path_.empty(); }

  void clear() { path_.clear(); }

  
  static bool IsSeparator(CharType character);

  
  
  
  
  
  
  
  
  
  
  void GetComponents(std::vector<FilePath::StringType>* components) const;

  
  
  
  
  
  bool IsParent(const FilePath& child) const;

  
  
  
  
  
  
  
  
  bool AppendRelativePath(const FilePath& child, FilePath* path) const;

  
  
  
  
  
  FilePath DirName() const WARN_UNUSED_RESULT;

  
  
  
  
  FilePath BaseName() const WARN_UNUSED_RESULT;

  
  
  
  
  
  
  
  
  
  
  StringType Extension() const;

  
  
  
  
  
  
  
  StringType FinalExtension() const;

  
  
  
  FilePath RemoveExtension() const WARN_UNUSED_RESULT;

  
  
  FilePath RemoveFinalExtension() const WARN_UNUSED_RESULT;

  
  
  
  
  
  
  
  FilePath InsertBeforeExtension(
      const StringType& suffix) const WARN_UNUSED_RESULT;
  FilePath InsertBeforeExtensionASCII(
      const base::StringPiece& suffix) const WARN_UNUSED_RESULT;

  
  
  FilePath AddExtension(
      const StringType& extension) const WARN_UNUSED_RESULT;

  
  
  
  
  FilePath ReplaceExtension(
      const StringType& extension) const WARN_UNUSED_RESULT;

  
  
  bool MatchesExtension(const StringType& extension) const;

  
  
  
  
  
  
  FilePath Append(const StringType& component) const WARN_UNUSED_RESULT;
  FilePath Append(const FilePath& component) const WARN_UNUSED_RESULT;

  
  
  
  
  
  
  FilePath AppendASCII(const base::StringPiece& component)
      const WARN_UNUSED_RESULT;

  
  
  
  
  bool IsAbsolute() const;

  
  bool EndsWithSeparator() const WARN_UNUSED_RESULT;

  
  
  FilePath AsEndingWithSeparator() const WARN_UNUSED_RESULT;

  
  
  FilePath StripTrailingSeparators() const WARN_UNUSED_RESULT;

  
  
  bool ReferencesParent() const;

  
  
  
  
  string16 LossyDisplayName() const;

  
  
  
  std::string MaybeAsASCII() const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  std::string AsUTF8Unsafe() const;

  
  string16 AsUTF16Unsafe() const;

  
  
  
  
  
  
  
  
  static FilePath FromUTF8Unsafe(const std::string& utf8);

  
  static FilePath FromUTF16Unsafe(const string16& utf16);

  void WriteToPickle(Pickle* pickle) const;
  bool ReadFromPickle(PickleIterator* iter);

  
  
  FilePath NormalizePathSeparators() const;

  
  
  FilePath NormalizePathSeparatorsTo(CharType separator) const;

  
  
  
  
  
  
  
  
  static int CompareIgnoreCase(const StringType& string1,
                               const StringType& string2);
  static bool CompareEqualIgnoreCase(const StringType& string1,
                                     const StringType& string2) {
    return CompareIgnoreCase(string1, string2) == 0;
  }
  static bool CompareLessIgnoreCase(const StringType& string1,
                                    const StringType& string2) {
    return CompareIgnoreCase(string1, string2) < 0;
  }

#if defined(OS_MACOSX)
  
  
  
  
  
  static StringType GetHFSDecomposedForm(const FilePath::StringType& string);

  
  
  
  
  static int HFSFastUnicodeCompare(const StringType& string1,
                                   const StringType& string2);
#endif

#if defined(OS_ANDROID)
  
  
  
  
  
  bool IsContentUri() const;
#endif

 private:
  
  
  
  
  
  void StripTrailingSeparatorsInternal();

  StringType path_;
};

}  


BASE_EXPORT extern void PrintTo(const base::FilePath& path, std::ostream* out);



#if defined(OS_POSIX)
#define FILE_PATH_LITERAL(x) x
#define PRFilePath "s"
#define PRFilePathLiteral "%s"
#elif defined(OS_WIN)
#define FILE_PATH_LITERAL(x) L ## x
#define PRFilePath "ls"
#define PRFilePathLiteral L"%ls"
#endif  



namespace BASE_HASH_NAMESPACE {

template<>
struct hash<base::FilePath> {
  size_t operator()(const base::FilePath& f) const {
    return hash<base::FilePath::StringType>()(f.value());
  }
};

}  

#endif  
