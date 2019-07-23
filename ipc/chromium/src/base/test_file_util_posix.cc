



#include "base/test_file_util.h"

#include <errno.h>
#include <fts.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>

#include "base/logging.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/string_util.h"

namespace file_util {

bool CopyRecursiveDirNoCache(const std::wstring& source_dir,
                             const std::wstring& dest_dir) {
  const FilePath from_path(FilePath::FromWStringHack(source_dir));
  const FilePath to_path(FilePath::FromWStringHack(dest_dir));

  char top_dir[PATH_MAX];
  if (base::strlcpy(top_dir, from_path.value().c_str(),
                    arraysize(top_dir)) >= arraysize(top_dir)) {
    return false;
  }

  char* dir_list[] = { top_dir, NULL };
  FTS* fts = fts_open(dir_list, FTS_PHYSICAL | FTS_NOSTAT, NULL);
  if (!fts) {
    LOG(ERROR) << "fts_open failed: " << strerror(errno);
    return false;
  }

  int error = 0;
  FTSENT* ent;
  while (!error && (ent = fts_read(fts)) != NULL) {
    
    
    std::string suffix(&ent->fts_path[from_path.value().size()]);
    
    if (!suffix.empty()) {
      DCHECK_EQ('/', suffix[0]);
      suffix.erase(0, 1);
    }
    const FilePath target_path = to_path.Append(suffix);
    switch (ent->fts_info) {
      case FTS_D:  
        
        
        if (mkdir(target_path.value().c_str(), 0777) != 0) {
          if (errno != EEXIST)
            error = errno;
        }
        break;
      case FTS_F:     
      case FTS_NSOK:  
        {
          errno = 0;
          FilePath source_path(ent->fts_path);
          if (CopyFile(source_path, target_path)) {
            bool success = EvictFileFromSystemCache(
                target_path.Append(source_path.BaseName()));
            DCHECK(success);
          } else {
            error = errno ? errno : EINVAL;
          }
        }
        break;
      case FTS_DP:   
      case FTS_DOT:  
        
        continue;
      case FTS_DC:   
        
        if (fts_set(fts, ent, FTS_SKIP) != 0)
          error = errno;
        break;
      case FTS_DNR:  
      case FTS_ERR:  
      case FTS_NS:   
        
        error = ent->fts_errno;
        break;
      case FTS_SL:      
      case FTS_SLNONE:  
        LOG(WARNING) << "skipping symbolic link: " << ent->fts_path;
        continue;
      case FTS_DEFAULT:  
        LOG(WARNING) << "skipping file of unknown type: " << ent->fts_path;
        continue;
      default:
        NOTREACHED();
        continue;  
    }
  }
  
  if (!error && errno != 0)
    error = errno;

  if (!fts_close(fts)) {
    
    
    if (!error)
      error = errno;
  }

  if (error) {
    LOG(ERROR) << strerror(error);
    return false;
  }
  return true;
}

}  
