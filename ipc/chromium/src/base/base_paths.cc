



#include "base/base_paths.h"

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/path_service.h"

namespace base {

bool PathProvider(int key, FilePath* result) {
  

  FilePath cur;
  switch (key) {
    case base::DIR_EXE:
      PathService::Get(base::FILE_EXE, &cur);
      cur = cur.DirName();
      break;
    case base::DIR_MODULE:
      PathService::Get(base::FILE_MODULE, &cur);
      cur = cur.DirName();
      break;
    case base::DIR_TEMP:
      if (!file_util::GetTempDir(&cur))
        return false;
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  
