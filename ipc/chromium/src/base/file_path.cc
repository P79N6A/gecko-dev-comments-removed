



#include "base/file_path.h"
#include "base/logging.h"



#include "base/string_piece.h"
#include "base/string_util.h"
#include "base/sys_string_conversions.h"

#if defined(FILE_PATH_USES_WIN_SEPARATORS)
const FilePath::CharType FilePath::kSeparators[] = FILE_PATH_LITERAL("\\/");
#else  
const FilePath::CharType FilePath::kSeparators[] = FILE_PATH_LITERAL("/");
#endif  

const FilePath::CharType FilePath::kCurrentDirectory[] = FILE_PATH_LITERAL(".");
const FilePath::CharType FilePath::kParentDirectory[] = FILE_PATH_LITERAL("..");

const FilePath::CharType FilePath::kExtensionSeparator = FILE_PATH_LITERAL('.');


namespace {






FilePath::StringType::size_type FindDriveLetter(
    const FilePath::StringType& path) {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  
  
  if (path.length() >= 2 && path[1] == L':' &&
      ((path[0] >= L'A' && path[0] <= L'Z') ||
       (path[0] >= L'a' && path[0] <= L'z'))) {
    return 1;
  }
#endif  
  return FilePath::StringType::npos;
}

bool IsPathAbsolute(const FilePath::StringType& path) {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  FilePath::StringType::size_type letter = FindDriveLetter(path);
  if (letter != FilePath::StringType::npos) {
    
    return path.length() > letter + 1 &&
        FilePath::IsSeparator(path[letter + 1]);
  }
  
  return path.length() > 1 &&
      FilePath::IsSeparator(path[0]) && FilePath::IsSeparator(path[1]);
#else  
  
  return path.length() > 0 && FilePath::IsSeparator(path[0]);
#endif  
}

}  

bool FilePath::IsSeparator(CharType character) {
  for (size_t i = 0; i < arraysize(kSeparators) - 1; ++i) {
    if (character == kSeparators[i]) {
      return true;
    }
  }

  return false;
}





FilePath FilePath::DirName() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  
  
  
  
  StringType::size_type letter = FindDriveLetter(new_path.path_);

  StringType::size_type last_separator =
      new_path.path_.find_last_of(kSeparators, StringType::npos,
                                  arraysize(kSeparators) - 1);
  if (last_separator == StringType::npos) {
    
    new_path.path_.resize(letter + 1);
  } else if (last_separator == letter + 1) {
    
    new_path.path_.resize(letter + 2);
  } else if (last_separator == letter + 2 &&
             IsSeparator(new_path.path_[letter + 1])) {
    
    
    new_path.path_.resize(letter + 3);
  } else if (last_separator != 0) {
    
    new_path.path_.resize(last_separator);
  }

  new_path.StripTrailingSeparatorsInternal();
  if (!new_path.path_.length())
    new_path.path_ = kCurrentDirectory;

  return new_path;
}

FilePath FilePath::BaseName() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  
  StringType::size_type letter = FindDriveLetter(new_path.path_);
  if (letter != StringType::npos) {
    new_path.path_.erase(0, letter + 1);
  }

  
  
  StringType::size_type last_separator =
      new_path.path_.find_last_of(kSeparators, StringType::npos,
                                  arraysize(kSeparators) - 1);
  if (last_separator != StringType::npos &&
      last_separator < new_path.path_.length() - 1) {
    new_path.path_.erase(0, last_separator + 1);
  }

  return new_path;
}

FilePath::StringType FilePath::Extension() const {
  
  StringType base = BaseName().value();

  
  if (base == kCurrentDirectory || base == kParentDirectory)
    return StringType();

  const StringType::size_type last_dot = base.rfind(kExtensionSeparator);
  if (last_dot == StringType::npos)
    return StringType();
  return StringType(base, last_dot);
}

FilePath FilePath::RemoveExtension() const {
  StringType ext = Extension();
  
  
  if (ext.empty())
    return FilePath(path_);
  
  
  const StringType::size_type last_dot = path_.rfind(kExtensionSeparator);
  return FilePath(path_.substr(0, last_dot));
}

FilePath FilePath::InsertBeforeExtension(const StringType& suffix) const {
  if (suffix.empty())
    return FilePath(path_);

  if (path_.empty())
    return FilePath();

  StringType base = BaseName().value();
  if (base.empty())
    return FilePath();
  if (*(base.end() - 1) == kExtensionSeparator) {
    
    if (base == kCurrentDirectory || base == kParentDirectory) {
      return FilePath();
    }
  }

  StringType ext = Extension();
  StringType ret = RemoveExtension().value();
  ret.append(suffix);
  ret.append(ext);
  return FilePath(ret);
}

FilePath FilePath::ReplaceExtension(const StringType& extension) const {
  if (path_.empty())
    return FilePath();

  StringType base = BaseName().value();
  if (base.empty())
    return FilePath();
  if (*(base.end() - 1) == kExtensionSeparator) {
    
    if (base == kCurrentDirectory || base == kParentDirectory) {
      return FilePath();
    }
  }

  FilePath no_ext = RemoveExtension();
  
  if (extension.empty() || extension == StringType(1, kExtensionSeparator))
    return no_ext;

  StringType str = no_ext.value();
  if (extension[0] != kExtensionSeparator)
    str.append(1, kExtensionSeparator);
  str.append(extension);
  return FilePath(str);
}

FilePath FilePath::Append(const StringType& component) const {
  DCHECK(!IsPathAbsolute(component));
  if (path_.compare(kCurrentDirectory) == 0) {
    
    
    
    
    
    
    
    return FilePath(component);
  }

  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  
  
  
  if (component.length() > 0 && new_path.path_.length() > 0) {

    
    
    if (!IsSeparator(new_path.path_[new_path.path_.length() - 1])) {

      
      if (FindDriveLetter(new_path.path_) + 1 != new_path.path_.length()) {
        new_path.path_.append(1, kSeparators[0]);
      }
    }
  }

  new_path.path_.append(component);
  return new_path;
}

FilePath FilePath::Append(const FilePath& component) const {
  return Append(component.value());
}

FilePath FilePath::AppendASCII(const std::string& component) const {
  DCHECK(IsStringASCII(component));
#if defined(OS_WIN)
  return Append(ASCIIToWide(component));
#elif defined(OS_POSIX)
  return Append(component);
#endif
}

bool FilePath::IsAbsolute() const {
  return IsPathAbsolute(path_);
}

#if defined(OS_POSIX)






FilePath FilePath::FromWStringHack(const std::wstring& wstring) {
  return FilePath(base::SysWideToNativeMB(wstring));
}
std::wstring FilePath::ToWStringHack() const {
  return base::SysNativeMBToWide(path_);
}
#elif defined(OS_WIN)

FilePath FilePath::FromWStringHack(const std::wstring& wstring) {
  return FilePath(wstring);
}
std::wstring FilePath::ToWStringHack() const {
  return path_;
}
#endif

FilePath FilePath::StripTrailingSeparators() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  return new_path;
}

void FilePath::StripTrailingSeparatorsInternal() {
  
  
  
  
  
  StringType::size_type start = FindDriveLetter(path_) + 2;

  StringType::size_type last_stripped = StringType::npos;
  for (StringType::size_type pos = path_.length();
       pos > start && IsSeparator(path_[pos - 1]);
       --pos) {
    
    
    if (pos != start + 1 || last_stripped == start + 2 ||
        !IsSeparator(path_[start - 1])) {
      path_.resize(pos - 1);
      last_stripped = pos;
    }
  }
}
