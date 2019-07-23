



#ifndef BASE_SCOPED_TEMP_DIR_H_
#define BASE_SCOPED_TEMP_DIR_H_







#include "base/file_path.h"

class ScopedTempDir {
 public:
  
  ScopedTempDir();

  
  ~ScopedTempDir();

  
  
  bool CreateUniqueTempDir();

  
  
  bool Set(const FilePath& path);

  
  
  FilePath Take();

  const FilePath& path() const { return path_; }

  
  bool IsValid() const;

 private:
  FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTempDir);
};

#endif  
