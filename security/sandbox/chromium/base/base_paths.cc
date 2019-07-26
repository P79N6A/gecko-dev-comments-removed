



#include "base/base_paths.h"

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/path_service.h"

namespace base {

bool PathProvider(int key, FilePath* result) {
  

  FilePath cur;
  switch (key) {
    case DIR_EXE:
      PathService::Get(FILE_EXE, &cur);
      cur = cur.DirName();
      break;
    case DIR_MODULE:
      PathService::Get(FILE_MODULE, &cur);
      cur = cur.DirName();
      break;
    case DIR_TEMP:
      if (!file_util::GetTempDir(&cur))
        return false;
      break;
    case DIR_TEST_DATA:
      if (!PathService::Get(DIR_SOURCE_ROOT, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("base"));
      cur = cur.Append(FILE_PATH_LITERAL("test"));
      cur = cur.Append(FILE_PATH_LITERAL("data"));
      if (!base::PathExists(cur))  
        return false;
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  
