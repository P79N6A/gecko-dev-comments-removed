



#include "base/scoped_temp_dir.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/string_util.h"

ScopedTempDir::ScopedTempDir() {
}

ScopedTempDir::~ScopedTempDir() {
  if (!path_.empty() && !file_util::Delete(path_, true))
    LOG(ERROR) << "ScopedTempDir unable to delete " << path_.value();
}

bool ScopedTempDir::CreateUniqueTempDir() {
  
  
  if (!file_util::CreateNewTempDirectory(FILE_PATH_LITERAL("scoped_dir"),
                                         &path_))
    return false;

  return true;
}

bool ScopedTempDir::Set(const FilePath& path) {
  DCHECK(path_.empty());
  if (!file_util::DirectoryExists(path) &&
      !file_util::CreateDirectory(path)) {
    return false;
  }
  path_ = path;
  return true;
}

FilePath ScopedTempDir::Take() {
  FilePath ret = path_;
  path_ = FilePath();
  return ret;
}

bool ScopedTempDir::IsValid() const {
  return !path_.empty() && file_util::DirectoryExists(path_);
}
